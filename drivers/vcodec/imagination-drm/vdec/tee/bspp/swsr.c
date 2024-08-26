/*!
 *****************************************************************************
 *
 * @File       swsr.c
 * ---------------------------------------------------------------------------
 *
 * Copyright (c) Imagination Technologies Ltd.
 * 
 * The contents of this file are subject to the MIT license as set out below.
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a 
 * copy of this software and associated documentation files (the "Software"), 
 * to deal in the Software without restriction, including without limitation 
 * the rights to use, copy, modify, merge, publish, distribute, sublicense, 
 * and/or sell copies of the Software, and to permit persons to whom the 
 * Software is furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in 
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR 
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE 
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER 
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, 
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN 
 * THE SOFTWARE.
 * 
 * Alternatively, the contents of this file may be used under the terms of the 
 * GNU General Public License Version 2 ("GPL")in which case the provisions of
 * GPL are applicable instead of those above. 
 * 
 * If you wish to allow use of your version of this file only under the terms 
 * of GPL, and not to allow others to use your version of this file under the 
 * terms of the MIT license, indicate your decision by deleting the provisions 
 * above and replace them with the notice and other provisions required by GPL 
 * as set out in the file called "GPLHEADER" included in this distribution. If 
 * you do not delete the provisions above, a recipient may use your version of 
 * this file under the terms of either the MIT license or GPL.
 * 
 * This License is also included in this distribution in the file called 
 * "MIT_COPYING".
 *
 *****************************************************************************/

#include <img_types.h>
#include <img_defs.h>
#include <img_errors.h>
#include <lst.h>
#include <swsr.h>
#include <report_api.h>

#if !defined (IMG_ASSERT)
    #include <assert.h>
    #define IMG_ASSERT(A)        assert    (A)
#endif

#ifdef _INJECT_ERROR
    #define         ERROR_OFFSET_LENGTH         4
    #define         ERROR_LENGTH                6
    IMG_UINT32      ui32ErroredByteLength;
#endif


#define NBIT_8BYTE_MASK(n)    (((IMG_UINT64)1 << (n)) - 1)

#define LEFT_ALIGNED_NBIT_8BYTE_MASK(mask, nbits)    \
    (((IMG_UINT64)mask << (64-nbits)) | (IMG_UINT64)NBIT_8BYTE_MASK(64-nbits))

//! Input FIFO length (in bytes).
#define SWSR_INPUT_FIFO_LENGTH      8

//! Output FIFO length (in bits).
#define SWSR_OUTPUT_FIFO_LENGTH     64

#define SWSR_NALTYPE_LENGTH         8

#define SWSR_MAX_SYNTAX_LENGTH      32

#if (((SWSR_MAX_DELIM_LENGTH + 7) / 8) > SWSR_INPUT_FIFO_LENGTH)
#error delimiter must fit entirely within the input FIFO
#endif


typedef struct
{
    LST_LINK;

    // Input.
    IMG_UINT8 * pui8Data;                   //!< Pointer to bitstream data.
    IMG_UINT64  ui64NumBytes;               //!< Number of bytes of bitstream.

    // State.
    IMG_UINT64  ui64ByteOffset;             //!< Index (in bytes) to next data within the buffer.

    IMG_UINT64  ui64NumBytesRead;            //!< number of bytes read from input FIFO.

} SWSR_sBuffer;


typedef struct
{
    // Data.
    IMG_UINT64          ui64FIFO;               //!< Bitstream data (byte-based and pre emu prev) - left aligned.
    IMG_UINT32          ui32NumBytes;           //!< Number of *bytes* in Input FIFO.

    // Configuration.
    SWSR_sConfig        sConfig;
    SWSR_eEmPrevent     eEmPrevent;             //!< Emulation prevention mode used to process data in Input FIFO.
    IMG_UINT32          ui32EmPrevSeqLen;       //!< Number of bytes in emulation prevention sequence.
    IMG_UINT64          ui64BitstreamSize;      //!< Size of bitstream declared at initialisation.

    // State.
    IMG_UINT32          ui32BytesForNextSequ;   //!< Number of bytes required from input buffer before checking next emulation prevention sequence.
    IMG_UINT64          ui64ByteCount;          //!< Byte count read from size delimiter.
    IMG_UINT64          ui64BytesReadSinceDelim;
    IMG_UINT64          ui64BitstreamOffset;    //!< Cumulative offset (in bytes) into input buffer data.
    IMG_BOOL            bDelimFound;            //!< Bitstream delimiter found (see #SWSR_eDelimType).
    IMG_BOOL            bNoMoreData;            //!< No More Valid Data before next delimiter. Set only for SWSR_EMPREVENT_00000300.

    SWSR_sBuffer      * psBuf;                  //!< Pointer to current input buffer in the context of Input FIFO.

} SWSR_sInput;


typedef struct
{
    IMG_UINT64          ui64FIFO;               /*!< Bitstream data (post emulation prevention removal/
                                                     delimiter checking) - left aligned.                 */
    IMG_UINT32          ui32NumBits;            //!< Number of *bits* in Output FIFO

    IMG_UINT64          ui64TotalBitsConsumed;  /*!< */

} SWSR_sOutput;


typedef struct
{
    SWSR_pfnCallback    pfnCb;                  /*!< Callback function to notify event and provide/request data.
                                                         See #SWSR_eCbEvent for event types and description of CB argument usage. */
    IMG_VOID          * pvCbParam;              //!< Caller supplied pointer for callback.

    LST_T               sFreeBufferList;        /*!< List of buffers    */
    LST_T               sUsedBufferList;        /*!< List of buffers (#SWSR_sBufferCtx) whose data reside in the
                                                     Input/Output FIFOs.                                            */

} SWSR_sBufferCtx;


/*!
******************************************************************************
 SR Context Structure
******************************************************************************/
typedef struct
{
    IMG_BOOL                bInitialised;           //!< IMG_TRUE if the context is initiliased

    // Exception Handling
    SWSR_pfnExceptHandler   pfnExceptionHandler;    //!< A pointer to an exception handler.
    IMG_VOID *              pvExceptionParam;       //!< Caller supplied pointer
    SWSR_eException         eException;             //!< Last recorded exception.

    SWSR_sBufferCtx         sBufferCtx;             //!< Buffer context data.

    SWSR_sInput             sInput;                 //!< Context of shift register input.
    SWSR_sOutput            sOutput;                //!< Context of shift register output.

} SWSR_sContext;


#ifdef SWSR_INJECT_ERRORS
typedef struct
{
    IMG_BOOL    bSkipSCP;            /*!< If set to #IMG_TRUE, the Injector will try to skip SCP
                                          at the start of the bitstream segment.                    */
    IMG_UINT32  ui32SCPBytesToSkip;  /*!< The number of bytes of the initial bitstream segment SCP
                                          left to be skipped.                                       */

} swsr_sErrorData;
#endif


/*!
******************************************************************************

 @Function              swsr_ExtractByte

 @Description

 Extract the next byte from the bitstream. A new buffer is requested if the current
 buffer has been exhausted and there is still more bytes declared in bitstream.

 @Input     psInput : Pointer to shift-register input context.

 @Input     psFreeBufList : Pointer to list of available buffer containers.

 @Output    psUsedBufferList : Pointer to in-use buffer list which will have a
                               new buffer added if the current gets exhausted.

 @Output    pui8Byte  : Pointer to hold byte value extracted from buffer.

 @Return   IMG_RESULT : IMG_SUCCESS or an error code.

******************************************************************************/
static IMG_RESULT
swsr_ExtractByte(
    SWSR_sInput       * psInput,
    SWSR_sBufferCtx   * psBufCtx,
    IMG_UINT8         * pui8Byte
)
{
    IMG_UINT8   ui8Byte = 0;
    IMG_UINT64  ui64CurByteOffset;
    IMG_UINT32  ui32Result = IMG_SUCCESS;

    IMG_ASSERT(psInput);
    IMG_ASSERT(psBufCtx);
    IMG_ASSERT(pui8Byte);

    ui64CurByteOffset = psInput->ui64BitstreamOffset;

    if (psInput->psBuf &&
        psInput->psBuf->ui64ByteOffset < psInput->psBuf->ui64NumBytes)
    {
        psInput->ui64BitstreamOffset++;
        ui8Byte = psInput->psBuf->pui8Data[psInput->psBuf->ui64ByteOffset++];
    }
    else if (psInput->ui64BitstreamOffset < psInput->ui64BitstreamSize)
    {
        SWSR_sBuffer * psBuffer;

        psBuffer = LST_removeHead(&psBufCtx->sFreeBufferList);
        IMG_ASSERT(psBuffer);

        psBuffer->ui64NumBytesRead = 0;
        psBuffer->ui64ByteOffset = 0;

        psBufCtx->pfnCb(SWSR_EVENT_INPUT_BUFFER_START,
                        psBufCtx->pvCbParam,
                        0,
                        &psBuffer->pui8Data,
                        &psBuffer->ui64NumBytes);
        IMG_ASSERT(psBuffer->pui8Data != IMG_NULL &&
                   psBuffer->ui64NumBytes > 0);

        if (psBuffer->pui8Data != IMG_NULL &&
            psBuffer->ui64NumBytes > 0)
        {
            psInput->psBuf = psBuffer;

            // Add input buffer to output buffer list.
            LST_add(&psBufCtx->sUsedBufferList, psInput->psBuf);

            psInput->ui64BitstreamOffset++;
            ui8Byte = psInput->psBuf->pui8Data[psInput->psBuf->ui64ByteOffset++];
        }
    }

    // If the bitstream offset hasn't increased we failed to read a byte.
    if (ui64CurByteOffset == psInput->ui64BitstreamOffset)
    {
        psInput->psBuf = IMG_NULL;
        ui32Result = IMG_ERROR_COULD_NOT_OBTAIN_RESOURCE;
    }

    *pui8Byte = ui8Byte;

    return ui32Result;
}


static IMG_BOOL
swsr_CheckForDelimiter(
    SWSR_sInput   * psInput
)
{
    IMG_BOOL bDelimFound = IMG_FALSE;

    // Check for delimiter.
    if (psInput->sConfig.eDelimType == SWSR_DELIM_SCP)
    {
        IMG_UINT32 ui32Shift = (SWSR_INPUT_FIFO_LENGTH * 8) - psInput->sConfig.ui32DelimLength;
        IMG_UINT64 ui64Sequ = psInput->ui64FIFO >> ui32Shift;

        // Check if the SCP value is matched outside of emulation prevention data.
        if (ui64Sequ == psInput->sConfig.ui64ScpValue &&
            psInput->ui32BytesForNextSequ == 0)
        {
            bDelimFound = IMG_TRUE;
        }
    }
    else if (psInput->sConfig.eDelimType == SWSR_DELIM_SIZE)
    {
        bDelimFound =
            (psInput->ui64BytesReadSinceDelim >= psInput->ui64ByteCount) ?
                IMG_TRUE : IMG_FALSE;
    }

    return bDelimFound;
}


/*!
******************************************************************************

 @Function              swsr_IncrementCurBufOffset

 @Description

******************************************************************************/
static IMG_RESULT
swsr_IncrementCurBufOffset(
    SWSR_sBufferCtx   * psBufCtx
)
{
    SWSR_sBuffer  * psCurBuf;

    // Update the number of bytes read from input FIFO for current buffer.
    psCurBuf = LST_first(&psBufCtx->sUsedBufferList);
    if (psCurBuf->ui64NumBytesRead >= psCurBuf->ui64NumBytes)
    {
        // Mark current bitstream buffer as fully consumed.
        psCurBuf->ui64NumBytesRead = psCurBuf->ui64NumBytes;

        // Notify the application that the old buffer is exhausted.
        psBufCtx->pfnCb(SWSR_EVENT_OUTPUT_BUFFER_END,
                        psBufCtx->pvCbParam,
                        0,
                        IMG_NULL,
                        IMG_NULL);

        // Discard the buffer whose data was at the head of the input FIFO.
        psCurBuf = LST_removeHead(&psBufCtx->sUsedBufferList);
        // Add the buffer container to free list.
        LST_add(&psBufCtx->sFreeBufferList, psCurBuf);

        // Since the byte that we read was actually from the next
        // buffer increment it's counter.
        psCurBuf = LST_first(&psBufCtx->sUsedBufferList);
        psCurBuf->ui64NumBytesRead++;
    }
    else
    {
        psCurBuf->ui64NumBytesRead++;
    }

    return IMG_SUCCESS;
}


/*!
******************************************************************************

 @Function              swsr_ReadByteFromInputFIFO

 @Description           Read one byte in  bitstream

 @Input                 psContext:  parser context

 @Output                read one byte.

******************************************************************************/
static SWSR_eFound
swsr_ReadByteFromInputFIFO(
    SWSR_sContext     * psContext,
    SWSR_sInput       * psInput,
    IMG_UINT8         * pui8Byte
)
{
    SWSR_eFound  eFound = SWSR_FOUND_NONE;
    IMG_UINT32   ui32Result = IMG_SUCCESS;

    IMG_ASSERT(psContext);
    IMG_ASSERT(psInput);
    IMG_ASSERT(pui8Byte);

    psInput->bDelimFound |= swsr_CheckForDelimiter(psInput);

    // Refill the input FIFO before checking for emulation prevention etc.
    // The only exception is when there are no more bytes left to extract from input buffer.
    while (psInput->ui32NumBytes < SWSR_INPUT_FIFO_LENGTH && ui32Result == IMG_SUCCESS)
    {
        IMG_UINT8 ui8Byte;

        ui32Result = swsr_ExtractByte(psInput, &psContext->sBufferCtx, &ui8Byte);
        if (ui32Result == IMG_SUCCESS)
        {
            psInput->ui64FIFO |= ((IMG_UINT64)ui8Byte << ((SWSR_INPUT_FIFO_LENGTH - 1 - psInput->ui32NumBytes) * 8));
            psInput->ui32NumBytes += 1;
        }
    }

    if (psInput->ui32NumBytes == 0)
    {
        eFound = SWSR_FOUND_EOD;
    }
    else if (!psInput->bDelimFound)
    {
        // Check for emulation prevention when enabled and enough bytes are remaining in input FIFO.
        if (psInput->eEmPrevent != SWSR_EMPREVENT_NONE &&
            psInput->ui32NumBytes >= psInput->ui32EmPrevSeqLen &&     // ensure you have enough bytes to check for emulation prevention.
               ( psInput->sConfig.eDelimType != SWSR_DELIM_SIZE ||
                ((psInput->ui64BytesReadSinceDelim + psInput->ui32EmPrevSeqLen) < psInput->ui64ByteCount) ) &&     // ensure that you don't remove emu bytes beyond current delimited unit.
            psInput->ui32BytesForNextSequ == 0)
        {
            IMG_BOOL    bEmPrevRemoved = IMG_FALSE;
            IMG_UINT32  ui32Shift = (SWSR_INPUT_FIFO_LENGTH - psInput->ui32EmPrevSeqLen) * 8;
            IMG_UINT64  ui64Sequ = psInput->ui64FIFO >> ui32Shift;

            if (psInput->eEmPrevent == SWSR_EMPREVENT_00000300)
            {
                if ((ui64Sequ & 0xffffff00) == 0x00000300)
                {
                    if ((ui64Sequ & 0x000000ff) > 0x03)
                    {
                        REPORT(REPORT_MODULE_SWSR,REPORT_WARNING,
                               "Invalid start code emulation prevention bytes found");
                    }

                    // Instead of trying to remove the emulation prevention byte from the middle of the FIFO
                    // simply make it zero and drop the next byte from the FIFO which will also be zero.
                    psInput->ui64FIFO &= LEFT_ALIGNED_NBIT_8BYTE_MASK(0xffff00ff, psInput->ui32EmPrevSeqLen * 8);
                    psInput->ui64FIFO <<= 8;

                    bEmPrevRemoved = IMG_TRUE;
                }
                else if ((ui64Sequ & 0xffffffff) == 0x00000000 ||
                         (ui64Sequ & 0xffffffff) == 0x00000001)
                {
                    psInput->bNoMoreData = IMG_TRUE;
                }
            }
            else if (psInput->eEmPrevent == SWSR_EMPREVENT_ff00)
            {
                if (ui64Sequ == 0xff00)
                {
                    // Remove the zero byte.
                    psInput->ui64FIFO <<= 8;
                    psInput->ui64FIFO |= ((IMG_UINT64)0xff00 << ui32Shift);
                    bEmPrevRemoved = IMG_TRUE;
                }
            }
            else if (psInput->eEmPrevent == SWSR_EMPREVENT_000002)
            {
                // Remove the emulation prevention bytes
                // if we find 22 consecutive 0 bits (from a byte-aligned position?!).
                if (ui64Sequ == 0x000002)
                {
                    // Appear to "remove" the 0x02 byte by clearing it and then dropping the top (zero) byte.
                    psInput->ui64FIFO &= LEFT_ALIGNED_NBIT_8BYTE_MASK(0xffff00, psInput->ui32EmPrevSeqLen * 8);
                    psInput->ui64FIFO <<= 8;
                    bEmPrevRemoved = IMG_TRUE;
                }
            }

            if (bEmPrevRemoved)
            {
                psInput->ui32NumBytes--;
                psInput->ui64BytesReadSinceDelim++;

                // Increment the buffer offset for the byte that has been removed.
                swsr_IncrementCurBufOffset(&psContext->sBufferCtx);

                // Signal that two more new bytes in the emulation prevention sequence are
                // required before another match can be made.
                psInput->ui32BytesForNextSequ = psInput->ui32EmPrevSeqLen - 2;
            }
        }

        if (psInput->ui32BytesForNextSequ > 0)
        {
            psInput->ui32BytesForNextSequ--;
        }

        /* return the first bytes from read data */
        *pui8Byte = (IMG_UINT8)(psInput->ui64FIFO >> ((SWSR_INPUT_FIFO_LENGTH-1) * 8));
        psInput->ui64FIFO <<= 8;

        psInput->ui32NumBytes--;
        psInput->ui64BytesReadSinceDelim++;

        // Increment the buffer offset for byte that has been read.
        swsr_IncrementCurBufOffset(&psContext->sBufferCtx);

        //IMG_ASSERT(psInput->ui32NumBytes >= 0);

        eFound = SWSR_FOUND_DATA;
    }
    else
    {
        eFound = SWSR_FOUND_DELIM;
    }

#ifdef _INJECT_ERROR
    if (ERROR_OFFSET_LENGTH < ui32ErroredByteLength)
    {
        if ((ui32ErroredByteLength % ERROR_LENGTH) == 0)
        {
           if ((ui32ErroredByteLength % ERROR_LENGTH) == 0)
            *pui8Byte &= (rand()%255); // corrupt the byte with random number in the range 0-255
        }
    }
    ui32ErroredByteLength++;
#endif


#ifdef SWSR_INJECT_ERRORS
    {
        IMG_RESULT         ui32Result;
        swsr_sErrorData *  psErrorData = NULL;

        ui32Result = STREAMERRINJ_GetUserData((IMG_HANDLE)psContext, (IMG_VOID **)(&psErrorData));
        if (IMG_SUCCESS == ui32Result)
        {
            if (SWSR_FOUND_DELIM == eFound)
            {
                if ((SWSR_DELIM_SCP == psInput->sConfig.eDelimType) && psErrorData->bSkipSCP)
                {
                    psErrorData->ui32SCPBytesToSkip = psInput->sConfig.ui32DelimLength >> 3;
                }
            }
            else if (SWSR_FOUND_DATA == eFound)
            {
                if(psErrorData->ui32SCPBytesToSkip == 0)
                {
                    IMG_SIZE  ui32Size = 1;
                    STREAMERRINJ_InsertErrors((IMG_HANDLE)psContext, pui8Byte, &ui32Size);
                }
                else
                {
                    psErrorData->ui32SCPBytesToSkip--;
                }
            }
        }
    }
#endif

    return eFound;
}


/*!
******************************************************************************

 @Function              swsr_ConsumeByteFromInputFIFO

******************************************************************************/
static SWSR_eFound
swsr_ConsumeByteFromInputFIFO(
    SWSR_sContext * psContext,
    IMG_UINT8     * pui8Byte
)
{
    SWSR_eFound eFound;

    eFound = swsr_ReadByteFromInputFIFO(psContext,
                                        &psContext->sInput,
                                        pui8Byte);

    if (eFound == SWSR_FOUND_DATA)
    {
        // Only whole bytes can be read from Input FIFO.
        psContext->sOutput.ui64TotalBitsConsumed += 8;
    }

    return eFound;
}


/*!
******************************************************************************

 @Function              swsr_FillOutputFIFO

 @Description           extract the bits from bitstream

 @Input                 psContext:  parser context

 @Output                read bits.

******************************************************************************/
static IMG_RESULT swsr_FillOutputFIFO(
    SWSR_sContext  * psContext
)
{
    IMG_UINT8   ui8Byte;
    SWSR_eFound eFound = SWSR_FOUND_DATA;

    // Fill output FIFO with whole bytes up to (but not over) max length.
    while (psContext->sOutput.ui32NumBits <= (SWSR_OUTPUT_FIFO_LENGTH - 8) && eFound == SWSR_FOUND_DATA)
    {
        eFound = swsr_ReadByteFromInputFIFO(psContext, &psContext->sInput, &ui8Byte);
        if (eFound == SWSR_FOUND_DATA)
        {
            psContext->sOutput.ui64FIFO |= ((IMG_UINT64)ui8Byte << (SWSR_OUTPUT_FIFO_LENGTH - 8 - psContext->sOutput.ui32NumBits));
            psContext->sOutput.ui32NumBits += 8;
        }
    }

    return IMG_SUCCESS;
}


/*!
******************************************************************************

 @Function              swsr_GetBitsFromOutputFIFO


******************************************************************************/
static IMG_UINT32
swsr_GetBitsFromOutputFIFO(
    SWSR_sContext     * psContext,
    IMG_UINT32          ui32NumBits,
    IMG_BOOL            bConsume
)
{
    IMG_UINT32  ui32BitsRead;
    IMG_UINT32  ui32Result;

    IMG_ASSERT(psContext->bInitialised);

    // Fetch more bits from the input FIFO if the output FIFO
    // doesn't have enough bits to satisfy the request on its own.
    if (ui32NumBits > psContext->sOutput.ui32NumBits)
    {
        ui32Result = swsr_FillOutputFIFO(psContext);
        IMG_ASSERT(ui32Result == IMG_SUCCESS);
    }

    // Ensure that are now enough bits in the output FIFO.
    if (ui32NumBits > psContext->sOutput.ui32NumBits)
    {
        if (psContext->sInput.bDelimFound)
        {
            // Tried to access into an SCP or other delimiter.
            psContext->eException = SWSR_EXCEPT_ACCESS_INTO_SCP;
        }
        else
        {
            // Data has been exhausted if after extracting bits there are still
            // not enough bits in the internal storage to fulfil the number requested.
            psContext->eException = SWSR_EXCEPT_ACCESS_BEYOND_EOD;
        }

        psContext->pfnExceptionHandler(psContext->eException, psContext->pvExceptionParam);

        // Return zero if the bits couldn't be obtained.
        ui32BitsRead = 0;
    }
    else
    {
        IMG_UINT32  ui32Shift;

        // Extract all the bits from the output FIFO.
        ui32Shift = (SWSR_OUTPUT_FIFO_LENGTH - ui32NumBits);
        ui32BitsRead = (IMG_UINT32)(psContext->sOutput.ui64FIFO >> ui32Shift);

        if (bConsume)
        {
            // Update output FIFO.
            psContext->sOutput.ui64FIFO <<= ui32NumBits;
            psContext->sOutput.ui32NumBits -= ui32NumBits;
        }
    }

    if (bConsume && psContext->eException == SWSR_EXCEPT_NO_EXCEPTION)
    {
        psContext->sOutput.ui64TotalBitsConsumed += ui32NumBits;
    }

    /* Return the bits...*/
    return ui32BitsRead;
}


#ifdef SWSR_INJECT_ERRORS
/*!
******************************************************************************

 @Function              SWSR_ConfigureErrorInjection

******************************************************************************/
IMG_VOID SWSR_ConfigureErrorInjection(
    IMG_HANDLE              hSwSrContext,
    SWSR_sErrorDescriptor * psErrorDescriptor
)
{
    STREAMERRINJ_sErrorDescriptor  sErrorDescriptor;
    swsr_sErrorData              * psErrorData = IMG_NULL;
    IMG_RESULT                     ui32Result;

    /* Setup the error descriptor. */
    IMG_MEMSET(&sErrorDescriptor, 0, sizeof(sErrorDescriptor));
    sErrorDescriptor.ui32ErrorTypes        = psErrorDescriptor->ui32ErrorTypes;
    sErrorDescriptor.sBusErrorDescriptor   = psErrorDescriptor->sBusErrorDescriptor;
    sErrorDescriptor.sMediaErrorDescriptor = psErrorDescriptor->sMediaErrorDescriptor;
    sErrorDescriptor.ui32RandomSeed        = psErrorDescriptor->ui32RandomSeed;

    /* Allocate SWSR error data. */
    psErrorData = IMG_MALLOC(sizeof(*psErrorData));
    IMG_ASSERT(psErrorData);
    if (IMG_NULL == psErrorData)
    {
        return;
    }

    /* Setup SWSR error data. */
    psErrorData->bSkipSCP           = psErrorDescriptor->bSkipSCP;
    psErrorData->ui32SCPBytesToSkip = 0;

    /* Create Error Injector instance. */
    ui32Result = STREAMERRINJ_Create(hSwSrContext);
    IMG_ASSERT(IMG_SUCCESS == ui32Result);
    if (IMG_SUCCESS != ui32Result)
    {
        return;
    }

    /* Configure error injection. */
    ui32Result = STREAMERRINJ_Configure(hSwSrContext, &sErrorDescriptor, (IMG_VOID *)psErrorData);
    IMG_ASSERT(IMG_SUCCESS == ui32Result);
    if (IMG_SUCCESS != ui32Result)
    {
        STREAMERRINJ_Destroy(hSwSrContext);
        return;
    }
}
#endif


/*!
******************************************************************************

 @Function              SWSR_ReadSignedExpGoulomb

 @Description           read signed exp-goulomb bits

 @Input                 psContext:  parser context

 @Output                no of leading zeros.

 @ Return               return value.

******************************************************************************/
IMG_INT32 SWSR_ReadSignedExpGoulomb(
    IMG_HANDLE          hContext
)
{
    SWSR_sContext * psContext = (SWSR_sContext *)hContext;
    IMG_UINT32      ui32ExpGoulomb;
    IMG_BOOL        bUnSigned;

    /* Validate input arguments. */
    if (psContext == IMG_NULL)
    {
        REPORT(REPORT_MODULE_SWSR, REPORT_ERR,
            "Invalid arguments to function: %s",
            __FUNCTION__);

        return IMG_ERROR_INVALID_PARAMETERS;
    }

    if (!psContext->bInitialised)
    {
        REPORT(REPORT_MODULE_SWSR, REPORT_ERR,
            "SWSR not yet intialised: %s",
            __FUNCTION__);

        return IMG_ERROR_NOT_INITIALISED;
    }

    /* Read unsigned value then convert to signed value...*/
    ui32ExpGoulomb = SWSR_ReadUnsignedExpGoulomb(psContext);

    bUnSigned = ui32ExpGoulomb & 1;
    ui32ExpGoulomb >>= 1;
    ui32ExpGoulomb = (bUnSigned) ? ui32ExpGoulomb+1 : -(IMG_INT32)ui32ExpGoulomb;

    if (psContext->eException != SWSR_EXCEPT_NO_EXCEPTION)
    {
        psContext->pfnExceptionHandler(psContext->eException, psContext->pvExceptionParam);
    }
    /* Return the signed value...*/
    return ui32ExpGoulomb;
}


/*!
******************************************************************************

 @Function              SWSR_ReadUnsignedExpGoulomb

 @Description           read unsigned exp-goulomb bits

 @Input                 psContext:  parser context

 @Output                no of leading zeros.

 @ Return               return value.

******************************************************************************/
//static IMG_UINT8   ui8Adjustment[16] = {
//    0,  // 0000
//    1,  // 0001
//    2,  // 0010
//    2,  // 0011
//    3,  // 0100
//    3,  // 0101
//    3,  // 0110
//    3,  // 0111
//    4,  // 1000
//    4,  // 1001
//    4,  // 1010
//    4,  // 1011
//    4,  // 1100
//    4,  // 1101
//    4,  // 1110
//    4,  // 1111
//};


static IMG_UINT32
swsr_ReadUnsignedExpGoulomb(
    SWSR_sContext * psContext
)
{
    IMG_UINT32      ui32NumBits = 0;
    IMG_UINT32      ui32BitPeeked;
    IMG_UINT32      ui32BitRead;
    IMG_UINT32      ui32SetBits;
    IMG_UINT32      ui32ExpGoulomb;

    /* Loop until we have found a non-zero nibble or reached 31 0-bits...*/
    /* first read is 3 bits only to prevent an illegal 32-bit peek */
    ui32NumBits = 1;//(psContext->sOutput.ui32NumBits>4)?4:psContext->sOutput.ui32NumBits;  // Log2 of ui8Adjustment size should be the max
    do
    {
        ui32BitPeeked = SWSR_PeekBits(psContext, ui32NumBits);
        /* Check for non-zero nibble...*/
        if (ui32BitPeeked != 0)
        {
            break;
        }
        ui32NumBits++;

    } while (ui32NumBits < 32);

    /* Correct the number of leading zero bits...*/
    ui32NumBits--;//-= ui8Adjustment[ui32BitPeeked];

    if ( ui32BitPeeked )
    {
        /* read leading zeros and 1-bit */
        ui32BitRead = SWSR_ReadBits(psContext, ui32NumBits+1);
        if( 1 != ui32BitRead )
            psContext->eException = SWSR_EXCEPT_EXPGOULOMB_ERROR;
    }
    else
    {
        /* read 31 zero bits - special case to deal with 31 or 32 leading zeros */
        ui32BitRead = SWSR_ReadBits(psContext, 31);
        if( 0 != ui32BitRead )
            psContext->eException = SWSR_EXCEPT_EXPGOULOMB_ERROR;

        /* next 3 bits make either 31 0-bit code:'1xx', or 32 0-bit code:'010' */
        /* only valid 32 0-bit code is:'0..010..0' and results in 0xffffffff */
        ui32BitPeeked = SWSR_PeekBits(psContext, 3);

        if ( SWSR_EXCEPT_NO_EXCEPTION == psContext->eException )
        {
            if ( 0x4 & ui32BitPeeked )
            {
                ui32BitRead = SWSR_ReadBits(psContext, 1);
                IMG_ASSERT( 1 == ui32BitRead );
                ui32NumBits = 31;
            }
            else
            {
                if ( 2 != ui32BitPeeked )
                {
                    psContext->eException = SWSR_EXCEPT_EXPGOULOMB_ERROR;
                }
                ui32BitRead = SWSR_ReadBits(psContext, 3);
                ui32BitRead = SWSR_ReadBits(psContext, 31);
                if ( 0 != ui32BitRead )
                {
                    psContext->eException = SWSR_EXCEPT_EXPGOULOMB_ERROR;
                }
                return 0xffffffff;
            }
        }
        else
        {
            /* encountered an exception while reading code */
            /* just return a valid value */
            return 0;
        }
    }

    /* read data bits */
    ui32BitRead = 0;
    if ( ui32NumBits )
    {
        ui32BitRead = SWSR_ReadBits(psContext, ui32NumBits);
    }
    /* convert exp-goulomb to value */
    ui32SetBits = (1<<ui32NumBits) - 1;
    ui32ExpGoulomb = ui32SetBits + ui32BitRead;
    /* Return the value...*/
    return ui32ExpGoulomb;
}


/*!
******************************************************************************

 @Function                SWSR_ReadUnsignedExpGoulomb

******************************************************************************/
IMG_UINT32 SWSR_ReadUnsignedExpGoulomb(
    IMG_HANDLE          hContext
)
{
    SWSR_sContext * psContext = (SWSR_sContext *)hContext;
    IMG_UINT32      ui32Value;

    /* Validate input arguments. */
    if (psContext == IMG_NULL)
    {
        REPORT(REPORT_MODULE_SWSR, REPORT_ERR,
            "Invalid arguments to function: %s",
            __FUNCTION__);

        return IMG_ERROR_INVALID_PARAMETERS;
    }

    if (!psContext->bInitialised)
    {
        REPORT(REPORT_MODULE_SWSR, REPORT_ERR,
            "SWSR not yet intialised: %s",
            __FUNCTION__);

        return IMG_ERROR_NOT_INITIALISED;
    }

    ui32Value = swsr_ReadUnsignedExpGoulomb(psContext);

    if (psContext->eException != SWSR_EXCEPT_NO_EXCEPTION)
    {
        psContext->pfnExceptionHandler(psContext->eException, psContext->pvExceptionParam);
    }
    return ui32Value;
}



/*!
******************************************************************************

 @Function              SWSR_CheckException

 @Description           check Software shiftregister exception

 @Input                 psContext:  parser context

 @Return                true or false

******************************************************************************/
SWSR_eException SWSR_CheckException(
    IMG_HANDLE          hContext
)
{
    SWSR_sContext     * psContext = (SWSR_sContext *)hContext;
    SWSR_eException     eException;

    /* Validate input arguments. */
    if (psContext == IMG_NULL)
    {
        REPORT(REPORT_MODULE_SWSR, REPORT_ERR,
            "Invalid arguments to function: %s",
            __FUNCTION__);

        return IMG_ERROR_INVALID_PARAMETERS;
    }

    eException = psContext->eException;

    if (!psContext->bInitialised)
    {
        REPORT(REPORT_MODULE_SWSR, REPORT_ERR,
            "SWSR not yet intialised: %s",
            __FUNCTION__);

        return IMG_ERROR_NOT_INITIALISED;
    }

    psContext->eException = SWSR_EXCEPT_NO_EXCEPTION;
    return eException;
}



/*!
******************************************************************************

 @Function                SWSR_CheckMoreRbspData

******************************************************************************/
IMG_RESULT
SWSR_CheckMoreRbspData(
    IMG_HANDLE          hContext,
    IMG_BOOL          * pbMoreRbspData
)
{
    SWSR_sContext * psContext = (SWSR_sContext *)hContext;

    IMG_INT32   i32RemBitsInByte;
    IMG_UINT8   ui8CurrentByte;
    IMG_INT32   i32NumOfAlignedRemBits;
    IMG_UINT64  ui64RestAlignedBytes;
    IMG_BOOL    bMoreData = IMG_FALSE;

    /* Validate input arguments. */
    if (psContext == IMG_NULL)
    {
        REPORT(REPORT_MODULE_SWSR, REPORT_ERR,
            "Invalid arguments to function: %s",
            __FUNCTION__);

        return IMG_ERROR_INVALID_PARAMETERS;
    }

    if (!psContext->bInitialised)
    {
        REPORT(REPORT_MODULE_SWSR, REPORT_ERR,
            "SWSR not yet intialised: %s",
            __FUNCTION__);

        return IMG_ERROR_NOT_INITIALISED;
    }

    if (psContext->sInput.eEmPrevent != SWSR_EMPREVENT_00000300)
    {
        REPORT(REPORT_MODULE_SWSR, REPORT_ERR,
            "SWSR cannot determine More RBSP data for a stream without SWSR_EMPREVENT_00000300: %s",
            __FUNCTION__);

        return IMG_ERROR_OPERATION_PROHIBITED;
    }

    // Always fill the output FIFO to ensure the bNoMoreData flag is set
    // when there are enough remaining bytes
    {
        IMG_UINT32 ui32Result;
        ui32Result = swsr_FillOutputFIFO(psContext);
        IMG_ASSERT(ui32Result == IMG_SUCCESS);
    }

    if (psContext->sOutput.ui32NumBits != 0)
    {
        /* Calculate the number of bits in the MS byte...*/
        i32RemBitsInByte = (psContext->sOutput.ui32NumBits & 0x7);
        if (i32RemBitsInByte == 0)
        {
            i32RemBitsInByte = 8;
        }
        i32NumOfAlignedRemBits = (psContext->sOutput.ui32NumBits - i32RemBitsInByte);

        /* Peek the value of last byte..*/
        ui8CurrentByte = SWSR_PeekBits(psContext, i32RemBitsInByte);
        ui64RestAlignedBytes = (psContext->sOutput.ui64FIFO >> (64-psContext->sOutput.ui32NumBits)) & (((IMG_UINT64)1<<i32NumOfAlignedRemBits)-1);

        if ( (ui8CurrentByte == (1<<(i32RemBitsInByte-1))) &&
             ( (i32NumOfAlignedRemBits == 0) ||
               (ui64RestAlignedBytes == 0 &&
                 ((((((IMG_UINT32)i32NumOfAlignedRemBits>>3)) < psContext->sInput.ui32EmPrevSeqLen) && (psContext->sInput.ui32NumBytes == 0)) || psContext->sInput.bNoMoreData) ) ) )
        {
            bMoreData = IMG_FALSE;
        }
        else
        {
            bMoreData = IMG_TRUE;
        }
    }

    *pbMoreRbspData = bMoreData;

    return IMG_SUCCESS;
}


/*!
******************************************************************************

 @Function              SWSR_ReadOneBit

******************************************************************************/
IMG_UINT32 SWSR_ReadOneBit(
    IMG_HANDLE          hContext
)
{
    SWSR_sContext * psContext = (SWSR_sContext *)hContext;
    IMG_UINT32  ui32BitRead;

    /* Validate input arguments. */
    if (psContext == IMG_NULL)
    {
        REPORT(REPORT_MODULE_SWSR, REPORT_ERR,
            "Invalid arguments to function: %s",
            __FUNCTION__);

        return IMG_ERROR_INVALID_PARAMETERS;
    }

    if (!psContext->bInitialised)
    {
        REPORT(REPORT_MODULE_SWSR, REPORT_ERR,
            "SWSR not yet intialised: %s",
            __FUNCTION__);

        return IMG_ERROR_NOT_INITIALISED;
    }

    // Optimise with inline code (specific version of call below).
    ui32BitRead = SWSR_ReadBits(psContext, 1);

    return ui32BitRead;
}


/*!
******************************************************************************

 @Function                SWSR_ReadBits

******************************************************************************/
IMG_UINT32 SWSR_ReadBits(
    IMG_HANDLE          hContext,
    IMG_UINT32          ui32NumBits
)
{
    SWSR_sContext * psContext = (SWSR_sContext *)hContext;

    /* Validate input arguments. */
    if (psContext == IMG_NULL ||
        !psContext->bInitialised)
    {
        REPORT(REPORT_MODULE_SWSR, REPORT_ERR,
               "%s Invalid SWSR context",
               __FUNCTION__);
        psContext->eException = SWSR_EXCEPT_INVALID_CONTEXT;
        psContext->pfnExceptionHandler(psContext->eException, psContext->pvExceptionParam);

        return 0;
    }

    if (ui32NumBits > SWSR_MAX_SYNTAX_LENGTH)
    {
        REPORT(REPORT_MODULE_SWSR, REPORT_ERR,
               "Maximum symbol length exceeded");
        psContext->eException = SWSR_EXCEPT_WRONG_CODEWORD_ERROR;
        psContext->pfnExceptionHandler(psContext->eException, psContext->pvExceptionParam);

        return 0;
    }

    return swsr_GetBitsFromOutputFIFO(psContext, ui32NumBits, IMG_TRUE);
}


/*!
******************************************************************************

 @Function                SWSR_PeekBits

******************************************************************************/
IMG_UINT32 SWSR_PeekBits(
    IMG_HANDLE          hContext,
    IMG_UINT32          ui32NumBits
)
{
    SWSR_sContext * psContext = (SWSR_sContext *)hContext;

    /* Validate input arguments. */
    if (psContext == IMG_NULL ||
        !psContext->bInitialised)
    {
        REPORT(REPORT_MODULE_SWSR, REPORT_ERR,
               "%s Invalid SWSR context",
               __FUNCTION__);
        psContext->eException = SWSR_EXCEPT_INVALID_CONTEXT;
        psContext->pfnExceptionHandler(psContext->eException, psContext->pvExceptionParam);

        return 0;
    }

    if (ui32NumBits > SWSR_MAX_SYNTAX_LENGTH)
    {
        REPORT(REPORT_MODULE_SWSR, REPORT_ERR,
               "Maximum symbol length exceeded");
        psContext->eException = SWSR_EXCEPT_WRONG_CODEWORD_ERROR;
        psContext->pfnExceptionHandler(psContext->eException, psContext->pvExceptionParam);

        return 0;
    }

    return swsr_GetBitsFromOutputFIFO(psContext, ui32NumBits, IMG_FALSE);
}


/*!
******************************************************************************

 @Function                SWSR_ByteAlign

******************************************************************************/
IMG_RESULT
SWSR_ByteAlign(
    IMG_HANDLE          hContext
)
{
    SWSR_sContext * psContext = (SWSR_sContext *)hContext;
    IMG_UINT32 ui32NumBits;     // Number of bits until byte-aligned.

    /* Validate input arguments. */
    if (psContext == IMG_NULL)
    {
        REPORT(REPORT_MODULE_SWSR, REPORT_ERR,
            "Invalid arguments to function: %s",
            __FUNCTION__);

        return IMG_ERROR_INVALID_PARAMETERS;
    }

    if (!psContext->bInitialised)
    {
        REPORT(REPORT_MODULE_SWSR, REPORT_ERR,
            "SWSR not yet intialised: %s",
            __FUNCTION__);

        return IMG_ERROR_NOT_INITIALISED;
    }

    ui32NumBits = (psContext->sOutput.ui32NumBits & 0x7);
    if (ui32NumBits != 0)
    {
        // Read the required number of bits if not already byte-aligned.
        SWSR_ReadBits(psContext, ui32NumBits);
    }

    IMG_ASSERT ((psContext->sOutput.ui32NumBits & 0x7) == 0);

    return IMG_SUCCESS;
}


/*!
******************************************************************************

 @Function                SWSR_CheckByteAligned

******************************************************************************/
IMG_BOOL    SWSR_CheckByteAligned(
    IMG_HANDLE          hContext
)
{
    SWSR_sContext * psContext = (SWSR_sContext *)hContext;

    /* Validate input arguments. */
    if (psContext == IMG_NULL)
    {
        REPORT(REPORT_MODULE_SWSR, REPORT_ERR,
            "Invalid arguments to function: %s",
            __FUNCTION__);

        return IMG_ERROR_INVALID_PARAMETERS;
    }

    if (!psContext->bInitialised)
    {
        REPORT(REPORT_MODULE_SWSR, REPORT_ERR,
            "SWSR not yet intialised: %s",
            __FUNCTION__);

        return IMG_ERROR_NOT_INITIALISED;
    }

    return ((psContext->sOutput.ui32NumBits & 0x7) == 0);
}


/*!
******************************************************************************

 @Function                SWSR_GetTotalBitsConsumed

******************************************************************************/
IMG_RESULT
SWSR_GetTotalBitsConsumed(
    IMG_HANDLE          hContext,
    IMG_UINT64        * pui64TotalBitsConsumed
)
{
    SWSR_sContext * psContext = (SWSR_sContext *)hContext;

    /* Validate input arguments. */
    if (psContext == IMG_NULL ||
        pui64TotalBitsConsumed == IMG_NULL)
    {
        REPORT(REPORT_MODULE_SWSR, REPORT_ERR,
            "Invalid arguments to function: %s",
            __FUNCTION__);

        return IMG_ERROR_INVALID_PARAMETERS;
    }

    if (!psContext->bInitialised)
    {
        REPORT(REPORT_MODULE_SWSR, REPORT_ERR,
            "SWSR not yet intialised: %s",
            __FUNCTION__);

        return IMG_ERROR_NOT_INITIALISED;
    }

    if (pui64TotalBitsConsumed)
    {
        *pui64TotalBitsConsumed = psContext->sOutput.ui64TotalBitsConsumed;
    }

    return IMG_SUCCESS;
}


/*!
******************************************************************************

 @Function                SWSR_GetByteOffsetCurBuf

******************************************************************************/
IMG_RESULT
SWSR_GetByteOffsetCurBuf(
    IMG_HANDLE          hContext,
    IMG_UINT64        * pui64ByteOffset
)
{
    SWSR_sContext * psContext = (SWSR_sContext *)hContext;

    /* Validate input arguments. */
    if (psContext == IMG_NULL ||
        pui64ByteOffset == IMG_NULL)
    {
        REPORT(REPORT_MODULE_SWSR, REPORT_ERR,
            "Invalid arguments to function: %s",
            __FUNCTION__);

        return IMG_ERROR_INVALID_PARAMETERS;
    }

    if (!psContext->bInitialised)
    {
        REPORT(REPORT_MODULE_SWSR, REPORT_ERR,
            "SWSR not yet intialised: %s",
            __FUNCTION__);

        return IMG_ERROR_NOT_INITIALISED;
    }

    if (psContext->sOutput.ui32NumBits != 0)
    {
        REPORT(REPORT_MODULE_SWSR, REPORT_ERR,
            "SWSR output FIFO not empty. First seek to next delimiter: %s",
            __FUNCTION__);

        return IMG_ERROR_OPERATION_PROHIBITED;
    }

    if (pui64ByteOffset)
    {
        SWSR_sBuffer  * psOutBuf;

        psOutBuf = LST_first(&psContext->sBufferCtx.sUsedBufferList);
        if (psOutBuf)
        {
            *pui64ByteOffset = psOutBuf->ui64NumBytesRead;
        }
        else
        {
            return IMG_ERROR_COULD_NOT_OBTAIN_RESOURCE;
        }
    }

    return IMG_SUCCESS;
}


static IMG_RESULT
swsr_UpdateEmPrevent(
    SWSR_eEmPrevent     eEmPrevent,
    SWSR_sInput       * psInput
)
{
    psInput->eEmPrevent = eEmPrevent;
    switch (psInput->eEmPrevent)
    {
    case SWSR_EMPREVENT_00000300:
        psInput->ui32EmPrevSeqLen = 4;
        break;

    case SWSR_EMPREVENT_ff00:
        psInput->ui32EmPrevSeqLen = 2;
        break;

    case SWSR_EMPREVENT_000002:
        psInput->ui32EmPrevSeqLen = 3;
        break;

    default:
        psInput->ui32EmPrevSeqLen = 0;
        break;
    }

    return IMG_SUCCESS;
}


/*!
******************************************************************************

 @Function                SWSR_ConsumeDelim

 @Description

 Set the emulation prevention and delimiter size for this delimited unit.

******************************************************************************/
IMG_RESULT
SWSR_ConsumeDelim(
    IMG_HANDLE          hContext,
    SWSR_eEmPrevent     eEmPrevent,
    IMG_UINT32          ui32SizeDelimLength,
    IMG_UINT64        * pui64ByteCount
)
{
    SWSR_sContext * psContext = (SWSR_sContext *)hContext;
    SWSR_sInput   * psInput;
    IMG_UINT64      ui64Delimiter = 0;
    IMG_UINT32      ui32Result;

    /* Validate input arguments. */
    if (psContext == IMG_NULL ||
        eEmPrevent >= SWSR_EMPREVENT_MAX ||
        (psContext->sInput.sConfig.eDelimType == SWSR_DELIM_SIZE &&
        ui32SizeDelimLength > SWSR_MAX_DELIM_LENGTH))
    {
        REPORT(REPORT_MODULE_SWSR, REPORT_ERR,
            "Invalid arguments to function: %s",
            __FUNCTION__);

        return IMG_ERROR_INVALID_PARAMETERS;
    }

    if (!psContext->bInitialised)
    {
        REPORT(REPORT_MODULE_SWSR, REPORT_ERR,
            "SWSR not yet intialised: %s",
            __FUNCTION__);

        return IMG_ERROR_NOT_INITIALISED;
    }

    if (psContext->sInput.sConfig.eDelimType == SWSR_DELIM_SIZE &&
        ui32SizeDelimLength == 0 &&
        pui64ByteCount == IMG_NULL)
    {
        REPORT(REPORT_MODULE_SWSR, REPORT_ERR,
            "Byte count value must be provided when size delimiter is zero length: %s",
            __FUNCTION__);

        return IMG_ERROR_INVALID_PARAMETERS;
    }

    psInput = &psContext->sInput;

    // Ensure that the input is at a delimiter since emulation prevention
    // removal will not have spanned into this next unit.
    // This allows emulation prevention detection modes to be changed.
    // Now check for delimiter.
    psInput->bDelimFound = swsr_CheckForDelimiter(psInput);

    if (!psInput->bDelimFound)
    {
        return IMG_ERROR_UNEXPECTED_STATE;
    }

    // Output bitstream FIFOs should be empty.
    // NOTE: flush output queue using seek function.
    IMG_ASSERT(psContext->sOutput.ui32NumBits == 0);

    // Only update the delimiter length for size delimiters.
    if (psInput->sConfig.eDelimType == SWSR_DELIM_SIZE)
    {
        psInput->sConfig.ui32DelimLength = ui32SizeDelimLength;
    }

    // Update the emulation prevention detection/removal scheme.
    ui32Result = swsr_UpdateEmPrevent(eEmPrevent, psInput);
    IMG_ASSERT(ui32Result == IMG_SUCCESS);

    // Peek at the NAL type and return in callback only when delimiter is in bitstream.
    if (psInput->sConfig.ui32DelimLength)
    {
        IMG_UINT32 ui32Shift;
        IMG_UINT8  ui8NalType;

        // Peek at the next 8-bits after the delimiter that resides in internal FIFO.
        ui32Shift = SWSR_OUTPUT_FIFO_LENGTH - (psInput->sConfig.ui32DelimLength + SWSR_NALTYPE_LENGTH);
        ui8NalType = (psInput->ui64FIFO >> ui32Shift) & NBIT_8BYTE_MASK(SWSR_NALTYPE_LENGTH);

        // Notify caller of NAL type so that bitstream segmentation
        // can take place before the delimiter is consumed.
        psContext->sBufferCtx.pfnCb(SWSR_EVENT_DELIMITER_NAL_TYPE,
                                    psContext->sBufferCtx.pvCbParam,
                                    ui8NalType,
                                    IMG_NULL,
                                    IMG_NULL);
    }

    // Clear the delimiter found flag and reset bytes read to allow reading of data from input FIFO.
    psInput->bDelimFound = IMG_FALSE;

    if (psInput->sConfig.ui32DelimLength != 0)
    {
        IMG_UINT64  ui64ScpValue = psInput->sConfig.ui64ScpValue;
        IMG_UINT32 i;
        IMG_UINT8  ui8Byte = 0;

        // Ensure that delimiter is not detected while delimiter is read.
        if (psInput->sConfig.eDelimType == SWSR_DELIM_SIZE)
        {
            psInput->ui64BytesReadSinceDelim = 0;
            psInput->ui64ByteCount = (psInput->sConfig.ui32DelimLength + 7) / 8;
        }
        else if (psInput->sConfig.eDelimType == SWSR_DELIM_SCP)
        {
            psInput->sConfig.ui64ScpValue = 0xdeadbeefdeadbeef;
        }

        // Fill output FIFO only with bytes at least partially used for delimiter.
        for( i = 0; i < ((psInput->sConfig.ui32DelimLength + 7) / 8); i++)
        {
            swsr_ReadByteFromInputFIFO(psContext, psInput, &ui8Byte);

            psContext->sOutput.ui64FIFO |= ((IMG_UINT64)ui8Byte << (SWSR_OUTPUT_FIFO_LENGTH - 8 - psContext->sOutput.ui32NumBits));
            psContext->sOutput.ui32NumBits += 8;
        }

        // Read delimiter from output FIFO leaving any remaining non-byte-aligned bits behind.
        ui64Delimiter = swsr_GetBitsFromOutputFIFO(psContext,
                                                   psInput->sConfig.ui32DelimLength,
                                                   IMG_TRUE);

        // Restore SCP value.
        if (psInput->sConfig.eDelimType == SWSR_DELIM_SCP)
        {
            psInput->sConfig.ui64ScpValue = ui64ScpValue;
        }
    }
    else
    {
        // For size delimited bitstreams without a delimiter use the byte count provided.
        IMG_ASSERT(*pui64ByteCount > 0);
        ui64Delimiter = *pui64ByteCount;
        IMG_ASSERT(psInput->sConfig.eDelimType == SWSR_DELIM_SIZE);
    }

    if (psInput->sConfig.eDelimType == SWSR_DELIM_SCP)
    {
        IMG_ASSERT((ui64Delimiter & NBIT_8BYTE_MASK(psInput->sConfig.ui32DelimLength)) == psInput->sConfig.ui64ScpValue);
    }
    else if (psInput->sConfig.eDelimType == SWSR_DELIM_SIZE)
    {
        psInput->ui64ByteCount = ui64Delimiter;

        // Return byte count if argument provided.
        if (pui64ByteCount)
        {
            *pui64ByteCount = psInput->ui64ByteCount;
        }
    }

    psInput->ui64BytesReadSinceDelim = 0;

    psInput->bNoMoreData = IMG_FALSE;

    return IMG_SUCCESS;
}


/*!
******************************************************************************

 @Function                SWSR_SeekDelimOrEOD

******************************************************************************/
SWSR_eFound
SWSR_SeekDelimOrEOD(
    IMG_HANDLE          hContext
)
{
    SWSR_sContext * psContext = (SWSR_sContext *)hContext;
    IMG_UINT8   ui8Byte;
    SWSR_eFound eFound = SWSR_FOUND_DATA;

    /* Validate input arguments. */
    if (psContext == IMG_NULL)
    {
        REPORT(REPORT_MODULE_SWSR, REPORT_ERR,
            "Invalid arguments to function: %s",
            __FUNCTION__);

        return IMG_ERROR_INVALID_PARAMETERS;
    }

    if (!psContext->bInitialised)
    {
        REPORT(REPORT_MODULE_SWSR, REPORT_ERR,
            "SWSR not yet intialised: %s",
            __FUNCTION__);

        return IMG_ERROR_NOT_INITIALISED;
    }

    // Read the residual contents of the output FIFO.
    SWSR_ByteAlign(psContext);
    while (psContext->sOutput.ui32NumBits > 0)
    {
        IMG_ASSERT((psContext->sOutput.ui32NumBits & 0x7) == 0);
        SWSR_ReadBits(psContext, 8);
    }
    IMG_ASSERT(psContext->sOutput.ui32NumBits == 0);

    // Extract data from input FIFO until data is not found either
    // because we have run out or a SCP has been detected.
    while (eFound == SWSR_FOUND_DATA)
    {
        eFound = swsr_ConsumeByteFromInputFIFO(psContext, &ui8Byte);
        IMG_ASSERT(eFound != SWSR_FOUND_NONE);
    }

    if (eFound == SWSR_FOUND_EOD)
    {
        // When the end of data has been reached there should be no
        // more data in the input FIFO.
        IMG_ASSERT(psContext->sInput.ui32NumBytes == 0);
    }

    IMG_ASSERT(eFound != SWSR_FOUND_DATA);
    return eFound;
}


/*!
******************************************************************************

 @Function                SWSR_CheckDelimOrEOD

******************************************************************************/
SWSR_eFound
SWSR_CheckDelimOrEOD(
    IMG_HANDLE          hContext
)
{
    SWSR_sContext * psContext = (SWSR_sContext *)hContext;
    SWSR_eFound eFound = SWSR_FOUND_DATA;

    /* Validate input arguments. */
    if (psContext == IMG_NULL)
    {
        REPORT(REPORT_MODULE_SWSR, REPORT_ERR,
            "Invalid arguments to function: %s",
            __FUNCTION__);

        return IMG_ERROR_INVALID_PARAMETERS;
    }

    if (!psContext->bInitialised)
    {
        REPORT(REPORT_MODULE_SWSR, REPORT_ERR,
            "SWSR not yet intialised: %s",
            __FUNCTION__);

        return IMG_ERROR_NOT_INITIALISED;
    }

    if (psContext->sOutput.ui32NumBits == 0 &&
        psContext->sInput.ui32NumBytes == 0 &&
        psContext->sInput.ui64BitstreamOffset >= psContext->sInput.ui64BitstreamSize)
    {
        // End of data when all FIFOs are empty and
        // there is nothing left to read from the input buffers.
        eFound = SWSR_FOUND_EOD;
    }
    else if (psContext->sOutput.ui32NumBits == 0 &&
             swsr_CheckForDelimiter(&psContext->sInput))
    {
        // Output queue is empty and delimiter is at the head of input queue.
        eFound = SWSR_FOUND_DELIM;
    }

    return eFound;
}


/*!
******************************************************************************

 @Function              SWSR_StartBitstream

******************************************************************************/
IMG_RESULT
SWSR_StartBitstream(
    IMG_HANDLE              hContext,
    const SWSR_sConfig    * psConfig,
    IMG_UINT64              ui64BitstreamSize,
    SWSR_eEmPrevent         eEmPrevent
)
{
    SWSR_sContext * psContext = (SWSR_sContext *)hContext;
    SWSR_sBuffer  * psBuffer;
    IMG_UINT32      ui32Result;

    /* Validate input arguments. */
    if (psContext == IMG_NULL ||
        psConfig == IMG_NULL ||
        psConfig->eDelimType >= SWSR_DELIM_MAX ||
        psConfig->ui32DelimLength > SWSR_MAX_DELIM_LENGTH ||
        psConfig->ui64ScpValue > NBIT_8BYTE_MASK(psConfig->ui32DelimLength) ||
        eEmPrevent >= SWSR_EMPREVENT_MAX)
    {
        REPORT(REPORT_MODULE_SWSR, REPORT_ERR,
            "Invalid arguments to function: %s",
            __FUNCTION__);

        return IMG_ERROR_INVALID_PARAMETERS;
    }

    if (!psContext->bInitialised)
    {
        REPORT(REPORT_MODULE_SWSR, REPORT_ERR,
            "SWSR not yet intialised: %s",
            __FUNCTION__);

        return IMG_ERROR_NOT_INITIALISED;
    }

    // Move all used buffers into free list.
    psBuffer = LST_removeHead(&psContext->sBufferCtx.sUsedBufferList);
    while (psBuffer)
    {
        LST_add(&psContext->sBufferCtx.sFreeBufferList, psBuffer);
        psBuffer = LST_removeHead(&psContext->sBufferCtx.sUsedBufferList);
    }

    // Clear all the shift-register state (except config).
    IMG_MEMSET(&psContext->sInput, 0, sizeof(psContext->sInput));
    IMG_MEMSET(&psContext->sOutput, 0, sizeof(psContext->sOutput));

    // Update input FIFO configuration.
    psContext->sInput.ui64BitstreamSize = ui64BitstreamSize;
    psContext->sInput.sConfig = *psConfig;
    ui32Result = swsr_UpdateEmPrevent(eEmPrevent, &psContext->sInput);
    IMG_ASSERT(ui32Result == IMG_SUCCESS);

    // Signal delimiter found to ensure that no data is read out of input FIFO
    // while fetching the first bitstream data into input FIFO.
    psContext->sInput.bDelimFound = IMG_TRUE;
    ui32Result = swsr_FillOutputFIFO(psContext);
    IMG_ASSERT(ui32Result == IMG_SUCCESS);

    // Now check for delimiter.
    psContext->sInput.bDelimFound = swsr_CheckForDelimiter(&psContext->sInput);

    return IMG_SUCCESS;
}


/*!
******************************************************************************

 @Function              SWSR_DeInitialise

 @Description           DeInitialise the parser

 @Input                 psContext:  parser context

 @Output                Deinitialised object.

******************************************************************************/
IMG_RESULT
SWSR_DeInitialise(
    IMG_HANDLE          hContext
)
{
    SWSR_sContext * psContext = (SWSR_sContext *)hContext;
    SWSR_sBuffer  * psBuffer;

    /* Validate input arguments. */
    if (psContext == IMG_NULL)
    {
        REPORT(REPORT_MODULE_SWSR, REPORT_ERR,
            "Invalid arguments to function: %s",
            __FUNCTION__);

        return IMG_ERROR_INVALID_PARAMETERS;
    }

    if (!psContext->bInitialised)
    {
        REPORT(REPORT_MODULE_SWSR, REPORT_ERR,
            "SWSR not yet intialised: %s",
            __FUNCTION__);

        return IMG_ERROR_NOT_INITIALISED;
    }

    // Free all used buffer containers.
    psBuffer = LST_removeHead(&psContext->sBufferCtx.sUsedBufferList);
    while (psBuffer)
    {
        IMG_FREE(psBuffer);
        psBuffer = LST_removeHead(&psContext->sBufferCtx.sUsedBufferList);
    }

    // Free all free buffer containers.
    psBuffer = LST_removeHead(&psContext->sBufferCtx.sFreeBufferList);
    while (psBuffer)
    {
        IMG_FREE(psBuffer);
        psBuffer = LST_removeHead(&psContext->sBufferCtx.sFreeBufferList);
    }

#ifdef SWSR_INJECT_ERRORS
    {
        swsr_sErrorData * psErrorData = NULL;

        if (IMG_SUCCESS == STREAMERRINJ_GetUserData((IMG_HANDLE)psContext, (IMG_VOID **)(&psErrorData)))
        {
            IMG_FREE(psErrorData);
        }
        STREAMERRINJ_Destroy((IMG_HANDLE)psContext);
    }
#endif

    psContext->bInitialised = IMG_FALSE;
    IMG_FREE(psContext);

    return IMG_SUCCESS;
}


/*!
******************************************************************************

 @Function              SWSR_Initialise


 @Output                initialised object.

 Contiguous section of bitstream.

******************************************************************************/
IMG_RESULT
SWSR_Initialise(
    SWSR_pfnExceptHandler   pfnExceptionHandler,
    IMG_VOID              * pvExceptionCbParam,
    SWSR_pfnCallback        pfnCallback,
    IMG_VOID              * pvCbParam,
    IMG_HANDLE            * phContext
)
{
    SWSR_sContext * psContext;
    SWSR_sBuffer  * psBuffer;
    IMG_UINT32      i;
    IMG_UINT32      ui32Result;

    /* Validate input arguments. */
    if (pfnExceptionHandler == IMG_NULL ||
        pvExceptionCbParam == IMG_NULL ||
        pfnCallback == IMG_NULL ||
        pvCbParam == IMG_NULL ||
        phContext == IMG_NULL)
    {
        REPORT(REPORT_MODULE_SWSR, REPORT_ERR,
            "Invalid arguments to function: %s",
            __FUNCTION__);

        return IMG_ERROR_INVALID_PARAMETERS;
    }

    // Allocate and initialise shift-register context.
    psContext = IMG_MALLOC(sizeof(*psContext));
    IMG_ASSERT(psContext);
    IMG_MEMSET(psContext, 0, sizeof(SWSR_sContext));

    // Setup shift-register context.
    psContext->pfnExceptionHandler = pfnExceptionHandler;
    psContext->pvExceptionParam = pvExceptionCbParam;

    psContext->sBufferCtx.pfnCb = pfnCallback;
    psContext->sBufferCtx.pvCbParam = pvCbParam;

    // Allocate a new buffer container for each byte in internal storage.
    // This is the theoretical maximum number of buffers in the SWSR at any one time.
    for (i = 0; i < SWSR_INPUT_FIFO_LENGTH + (SWSR_OUTPUT_FIFO_LENGTH/8); i++)
    {
        /* Allocate a buffer container...*/
        psBuffer = IMG_MALLOC(sizeof(*psBuffer));
        IMG_ASSERT(psBuffer != IMG_NULL);
        if (psBuffer == IMG_NULL)
        {
            REPORT(REPORT_MODULE_SWSR, REPORT_ERR,
                "Failed to allocate memory for buffer container");

            ui32Result = IMG_ERROR_OUT_OF_MEMORY;
            goto error;
        }
        IMG_MEMSET(psBuffer, 0, sizeof(*psBuffer));

        // Add container to free list.
        LST_add(&psContext->sBufferCtx.sFreeBufferList, psBuffer);
    }

    IMG_ASSERT(SWSR_MAX_SYNTAX_LENGTH <= (sizeof(IMG_UINT32) * 8));

#ifdef _INJECT_ERROR
    ui32ErroredByteLength = 0;
#endif

    psContext->bInitialised = IMG_TRUE;
    *phContext = psContext;

    return IMG_SUCCESS;

error:
    if(psContext)
    {
        psBuffer = LST_removeHead(&psContext->sBufferCtx.sFreeBufferList);
        while (psBuffer)
        {
            IMG_FREE(psBuffer);
            psBuffer = LST_removeHead(&psContext->sBufferCtx.sFreeBufferList);
        }
        IMG_FREE(psContext);
    }

    return ui32Result;
}



