/*!
 *****************************************************************************
 *
 * @File       swsr.h
 * @Description    This file contains the prototypes for the Software Shift Register
 *  Access Functions.
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

#if !defined(__SWSR_H__)
#define __SWSR_H__


#include "img_types.h"
#include "img_defs.h"

#ifdef SWSR_INJECT_ERRORS
#include "stream_error_injector.h"
#endif

#if defined(__cplusplus)
extern "C" {
#endif

//!< Maximum delimiter length in bits.
#define SWSR_MAX_DELIM_LENGTH   (8 * 8)


/*!
******************************************************************************
  This type defines the Shift Register exceptions
******************************************************************************/
typedef enum
{
    SWSR_EXCEPT_NO_EXCEPTION = 0x00,		//<! No error since the last call to SWSR_CheckException()
											//   or the caller supplied exception handler
											//   see #SWSR_pfnExceptionHandler
	SWSR_EXCEPT_ENCAPULATION_ERROR1,	    //<! Encapsulation error - 0x00, 0x00, 0x03 at
											//   end of bitstream section
	SWSR_EXCEPT_ENCAPULATION_ERROR2,	    //<! Encapsulation error - 0x00, 0x00, 0x03 followed
											//   by a value which is > 0x03
	SWSR_EXCEPT_ACCESS_INTO_SCP,			//<! Read or peek made into next Start-Code-Prefix
	SWSR_EXCEPT_ACCESS_BEYOND_EOD,			//<! Read or peek made beyond the End-Of-Data
	SWSR_EXCEPT_EXPGOULOMB_ERROR,		    //<! Error reading Exp-Goulomb value
    SWSR_EXCEPT_WRONG_CODEWORD_ERROR,       //<! Error in reading the codeword, read unexpected value
    SWSR_EXCEPT_NO_SCP,                     //<! SCP is not detetced at the start of data unit
    SWSR_EXCEPT_INVALID_CONTEXT,

} SWSR_eException;


typedef enum
{
    SWSR_EVENT_INPUT_BUFFER_START = 0,
    SWSR_EVENT_OUTPUT_BUFFER_END,
    SWSR_EVENT_DELIMITER_NAL_TYPE,

} SWSR_eCbEvent;


typedef enum
{
    SWSR_FOUND_NONE = 0,
    SWSR_FOUND_EOD,
    SWSR_FOUND_DELIM,
    SWSR_FOUND_DATA,

} SWSR_eFound;


typedef enum
{
    SWSR_DELIM_NONE = 0,    //!< SR will ignore SCPs.
    SWSR_DELIM_SCP,         //!< SR will always stop on SCP detection.
    SWSR_DELIM_SIZE,

    SWSR_DELIM_MAX,

} SWSR_eDelimType;


/*!
******************************************************************************
  This type defines the emulation prevention modes
******************************************************************************/
typedef enum
{
	SWSR_EMPREVENT_NONE = 0x00,
	SWSR_EMPREVENT_00000300,
	SWSR_EMPREVENT_ff00,
	SWSR_EMPREVENT_000002,

    SWSR_EMPREVENT_MAX,

} SWSR_eEmPrevent;


typedef struct
{
    SWSR_eDelimType     eDelimType;             //!< Type of delimiter used to separate bitstream units.
    IMG_UINT32          ui32DelimLength;        //!< Length of delimiter (in bits). See #SWSR_eDelimType.
    IMG_UINT64          ui64ScpValue;           //!< Start code prefix value when masking all bits in delimiter length.

} SWSR_sConfig;



/*!
******************************************************************************

 @Function				SWSR_pfnExceptHandler

 @Description

 This is the function prototype for the caller supplier exception handler.

 NOTE: The internally recorded exception is reset to #SWSR_EXCEPT_NO_EXCEPTION
 on return from SWSR_CheckException() or a call to the caller supplied exception
 handler see #SWSR_pfnExceptHandler.

 NOTE: By defining an exception handler the caller can handle Shift Register
 errors as they occur - for example, using a structure exception mechanism
 such as setjmp/longjmp.

 @Input	    eException  : The exception being signaled.

 @Input     pvCallbackParam : Caller supplied pointer - see SWSR_Initialise()

 @Return	None.

******************************************************************************/
typedef IMG_VOID ( * SWSR_pfnExceptHandler) (
    SWSR_eException     eException,
    IMG_VOID          * pvCallbackParam
);

/*!
******************************************************************************

 @Function				SWSR_pfnCheckMoreData

 @Description

 This is the function prototype for the caller supplier to retrieve the data from the application

 @Input	    psPrivData    : application private data

 @Input     ui8NalType     : 8-bit NAL type value

 @Input     ppsDataBuffer : buffer pointer

 @Input     pui64Datasize : buffer size

 @Return	None.

******************************************************************************/
typedef IMG_VOID (*SWSR_pfnCallback)(
    SWSR_eCbEvent   eEvent,
    IMG_VOID      * psPrivData,
    IMG_UINT8       ui8NalType,
    IMG_UINT8    ** ppsDataBuffer,
    IMG_UINT64    * pui64Datasize
);


#ifdef SWSR_INJECT_ERRORS
/*!
******************************************************************************
 Description of errors to be exercised on the SWSR data.
 NOTE: This structure contains only a subset of error descriptors available
       in the Error Injector. The set is limited to error types applicable
       to SWSR data.
******************************************************************************/
typedef struct
{
    IMG_UINT32                          ui32ErrorTypes;          /*!< Which error types to be injected.             */

    STREAMERRINJ_sBusErrorDescriptor    sBusErrorDescriptor;     /*!< Description of start errors to be exercised.
                                                                      Valid only if #ERRINJ_TYPE_BUS is set in
                                                                      #ui32ErrorTypes.                              */
    STREAMERRINJ_sMediaErrorDescriptor  sMediaErrorDescriptor;   /*!< Description of start errors to be exercised.
                                                                      Valid only if #ERRINJ_TYPE_MEDIA is set in
                                                                      #ui32ErrorTypes.                              */

    IMG_UINT32                          ui32RandomSeed;          /*!< Random seed to use in error generation.
                                                                      This allows to reproduce errors for various
                                                                      test runs on the same stream.
                                                                      NOTE: If set to INT_MAX, the current time()
                                                                      value will be used for setting the seed.      */

    IMG_BOOL                            bSkipSCP;                /*!< If set to IMG_TRUE, error injection will try
                                                                      not to mess up with the SCP at the start
                                                                      of the bitstream section.                     */

} SWSR_sErrorDescriptor;
#endif



#ifdef SWSR_INJECT_ERRORS
/*!
******************************************************************************

 @Function              SWSR_ConfigureErrorInjection

 @Description

 This function is used to configure error injection in the Shift Register.

 @Input     psContext         : A pointer to the Shift Register context structure.

 @Input     psErrorDescriptor : A pointer to error descriptor structure.

 @Return    None.

******************************************************************************/
extern IMG_VOID SWSR_ConfigureErrorInjection(
    IMG_HANDLE               hContext,
    SWSR_sErrorDescriptor *  psErrorDescriptor
);
#endif


/*!
******************************************************************************

 @Function				SWSR_GetTotalBitsConsumed

******************************************************************************/
extern IMG_RESULT
SWSR_GetTotalBitsConsumed(
    IMG_HANDLE          hContext,
    IMG_UINT64        * pui64TotalBitsConsumed
);


/*!
******************************************************************************

 @Function				SWSR_GetByteOffsetCurBuf

 @Description

 This function is used to return the offset into the current bitstream buffer
 on the shift-register output FIFO. Call after #SWSR_SeekDelimOrEOD to determine
 the offset of an delimiter.

 @Input     hContext    : Shift-register context handle.

 @Return    pui64ByteOffset : Current byte offset into the current bitstream buffer
                              on shift-register output FIFO.

 @Return    IMG_RESULT : IMG_SUCCESS or an error code.

******************************************************************************/
extern IMG_RESULT
SWSR_GetByteOffsetCurBuf(
    IMG_HANDLE          hContext,
    IMG_UINT64        * pui64ByteOffset
);


/*!
******************************************************************************

 @Function				SWSR_ReadSignedExpGoulomb

 @Description

 This function is used to read a signed Exp-Goulomb value from the Shift
 Register.

 NOTE: If this function is used to attempt to read into a Start-Code-Prefix
 or beyond the End-Of-Data then and exception is generated which can be
 handled by the caller supplied exception handler see #SWSR_pfnExceptionHandler.
 If no exception handler has been supplied (or the exception handler returns)
 then the exception is recorded and can be obtained using SWSR_CheckException().
 In this event the function returns 0.

 @Input     hContext    : Shift-register context handle.

 @Return	IMG_INT32   : The value read from the Shift Register.

******************************************************************************/
extern IMG_INT32 SWSR_ReadSignedExpGoulomb(
    IMG_HANDLE               hContext
);


/*!
******************************************************************************

 @Function				SWSR_ReadUnsignedExpGoulomb

 @Description

 This function is used to read a unsigned Exp-Goulomb value from the Shift
 Register.

 NOTE: If this function is used to attempt to read into a Start-Code-Prefix
 or beyond the End-Of-Data then and exception is generated which can be
 handled by the caller supplied exception handler see #SWSR_pfnExceptionHandler.
 If no exception handler has been supplied (or the exception handler returns)
 then the exception is recorded and can be obtained using SWSR_CheckException().
 In this event the function returns 0.

 @Input     hContext    : Shift-register context handle.

 @Return	IMG_UINT32  : The value read from the Shift Register.

******************************************************************************/
extern IMG_UINT32 SWSR_ReadUnsignedExpGoulomb(
    IMG_HANDLE               hContext
);


/*!
******************************************************************************

 @Function				SWSR_CheckException

 @Description

 This function is used to check for exceptions.

 NOTE: The internally recorded exception is reset to #SWSR_EXCEPT_NO_EXCEPTION
 on return from SWSR_CheckException() or a call to the caller supplied exception
 handler see #SWSR_pfnExceptionHandler.

 @Input     hContext    : Shift-register context handle.

 @Return	eException  : The last recorded exception.

******************************************************************************/
extern SWSR_eException SWSR_CheckException(
        IMG_HANDLE               hContext
);


/*!
******************************************************************************

 @Function				SWSR_CheckMoreRbspData

 @Description

 This function is used to check for bitstream data with SWSR_EMPREVENT_00000300
 whether more RBSP data is present.

 @Input     hContext       : Shift-register context handle.

 @Output    pbMoreRbspData : Pointer to more RBSP data flag.

 @Return    IMG_RESULT : IMG_SUCCESS or an error code.

******************************************************************************/
extern IMG_RESULT
SWSR_CheckMoreRbspData(
    IMG_HANDLE          hContext,
    IMG_BOOL          * pbMoreRbspData
);


/*!
******************************************************************************

 @Function				SWSR_ReadOneBit

 @Description

 This function is used to read a single bit from the Shift Register.

 NOTE: If this function is used to attempt to read into a Start-Code-Prefix
 or beyond the End-Of-Data then and exception is generated which can be
 handled by the caller supplied exception handler see #SWSR_pfnExceptionHandler.
 If no exception handler has been supplied (or the exception handler returns)
 then the exception is recorded and can be obtained using SWSR_CheckException().
 In this event the function returns 0.

 @Input     hContext    : Shift-register context handle.

 @Return	IMG_UINT32  : The bit read from the Shift Register.

******************************************************************************/
extern IMG_UINT32 SWSR_ReadOneBit(
    IMG_HANDLE               hContext
);


/*!
******************************************************************************

 @Function				SWSR_ReadBits

 @Description

 This function is used to consume a number of bits from the Shift Register.

 NOTE: If this function is used to attempt to read into a Start-Code-Prefix
 or beyond the End-Of-Data then and exception is generated which can be
 handled by the caller supplied exception handler see #SWSR_pfnExceptionHandler.
 If no exception handler has been supplied (or the exception handler returns)
 then the exception is recorded and can be obtained using SWSR_CheckException().
 In this event the function returns 0.

 @Input     hContext    : Shift-register context handle.

 @Input		ui32NoBits	    : The number of bits to read.

 @Return	IMG_UINT32	    : The bits read from the Shift Register.

******************************************************************************/
extern IMG_UINT32 SWSR_ReadBits(
    IMG_HANDLE               hContext,
    IMG_UINT32              ui32NoBits
);


/*!
******************************************************************************

 @Function				SWSR_PeekBits

 @Description

 This function is used to peek at number of bits from the Shift Register. The
 bits are not consumed.

 NOTE: If this function is used to attempt to read into a Start-Code-Prefix
 or beyond the End-Of-Data then and exception is generated which can be
 handled by the caller supplied exception handler see #SWSR_pfnExceptionHandler.
 If no exception handler has been supplied (or the exception handler returns)
 then the exception is recorded and can be obtained using SWSR_CheckException().
 In this event the function returns 0.

 @Input     hContext    : Shift-register context handle.

 @Input     ui32NoBits  : The number of bits to be peeked.

 @Return	IMG_UINT32	: The bits peeked from the Shift Register.

******************************************************************************/
extern IMG_UINT32 SWSR_PeekBits(
    IMG_HANDLE              hContext,
    IMG_UINT32              ui32NoBits
);



/*!
******************************************************************************

 @Function				SWSR_ByteAlign

 @Description

 Makes the shift-register output byte-aligned by consuming the remainder of the
 current partially read byte.

 @Input     hContext : Shift-register context handle.

 @Return    IMG_RESULT : IMG_SUCCESS or an error code.

******************************************************************************/
extern IMG_RESULT
SWSR_ByteAlign(
    IMG_HANDLE               hContext
);


/*!
******************************************************************************

 @Function				SWSR_CheckByteAligned

 @Description

 Signals whether the shift-register output is currently byte-aligned.

 @Input     hContext : Shift-register context handle.

 @Return	IMG_BOOL : IMG_TRUE if the Shift Register is byte aligned.

******************************************************************************/
extern IMG_BOOL	SWSR_CheckByteAligned(
    IMG_HANDLE               hContext
);


/*!
******************************************************************************

 @Function              SWSR_ConsumeDelim

 @Description

 Consume the next delimiter whose length should be specified if delimiter type
 is #SWSR_DELIM_SIZE. The emulation prevention detection/removal scheme can also be
 specified for this and subsequent units.

 Consumes the unit delimiter from the bitstream buffer. The delimiter type
 depends upon the bitstream format.

 Delimiters:

 Start Code     0000 0000 0000 0000 0000 0001  (eBstrFormat == VDEC_BSTRFORMAT_DEMUX_BYTESTREAM)
 Short header   0000 0000 0000 0000 [10 0000]   H.263 and Sorenson
 Size

 @Input     hContext : Shift-register context handle.

 @Input     eEmPrevent : Emulation prevention detection/removal scheme.

 @Input     ui32SizeDelimLength : Length (in bits) of size delimiter. If the data on
                                  from this point in the bitstream is not size delimited
                                  (but has a fixed length) set this argument to zero
                                  and provide a data length (in bytes) via argument
                                  pui64ByteCount.

 @InOut     pui64ByteCount : Only used if ui32SizeDelimLength is zero. Defines how many
                             bytes in the bitstream can be read before next delimiter.
                             Function always returns the number of bytes after this
                             delimiter is a pointer is provided.

 @Return   IMG_RESULT : IMG_SUCCESS or an error code.

******************************************************************************/
extern IMG_RESULT
SWSR_ConsumeDelim(
    IMG_HANDLE          hContext,
    SWSR_eEmPrevent     eEmPrevent,
    IMG_UINT32          ui32SizeDelimLength,
    IMG_UINT64        * pui64ByteCount
);


/*!
******************************************************************************

 @Function              SWSR_SeekDelimOrEOD

 @Description

 Seek for the next delimiter or end of bitstream data if no delimiter is found.

 @Input     hContext : Shift-register context handle.

 @Return    SWSR_eFound.

******************************************************************************/
extern SWSR_eFound
SWSR_SeekDelimOrEOD(
    IMG_HANDLE               hContext
);


/*!
******************************************************************************

 @Function				SWSR_CheckDelimOrEOD

 @Description

 Check if shift-register is at a delimiter or end of data.

 @Input     hContext : Shift-register context handle.

 @Return    SWSR_eFound.

******************************************************************************/
extern SWSR_eFound
SWSR_CheckDelimOrEOD(
    IMG_HANDLE          hContext
);


/*!
******************************************************************************

 @Function              SWSR_StartBitstream

 @Description

 This function automatically fetches the first bitstream buffer (using callback
 with event type #SWSR_EVENT_INPUT_BUFFER_START) before returning.

******************************************************************************/
extern IMG_RESULT
SWSR_StartBitstream(
    IMG_HANDLE              hContext,
    const SWSR_sConfig    * psConfig,
    IMG_UINT64              ui64BitstreamSize,
    SWSR_eEmPrevent         eEmPrevent
);


/*!
******************************************************************************

 @Function              SWSR_DeInitialise

 @Description

 This function is used to de-initialise the Shift Register.

 @Input     hContext : Shift-register context handle.

 @Return    None.

******************************************************************************/
extern IMG_RESULT
SWSR_DeInitialise(
    IMG_HANDLE               hContext
);


/*!
******************************************************************************

 @Function				SWSR_Initialise

 @Description

 This function is used to initialise the Shift Register.

 NOTE: If no exception handler is provided (pfnExceptionHandler == IMG_NULL)
 then the caller must check for exceptions using the function
 SWSR_CheckException().

 NOTE: If pui8RbduBuffer is IMG_NULL then the bit stream is not encapsulated
 so the Shift Register needn't perform and de-encapsulation.  However,
 if this is not IMG_NULL then, from time to time, the Shift Register APIs
 will de-encapsulate portions of the bit stream into this intermediate buffer
 - the larger the buffer the less frequent the de-encapsulation function
 needs to be called.

 @Input     psContext       : A pointer to the Shift Register context structure.

 @Input     pfnExceptionHandler : A pointer to an exception handler.  IMG_NULL
                              if no exception handler is provided.

 @Input     pvCallbackParam : Caller supplied pointer passed to to the caller
                              supplied exception handler

 @Input		bScpDetection	: IMG_TRUE to enable Start-Code-Prefix detection.
							  Otherwise IMG_FALSE.

                              NOTE: Can only be set if RBDU extraction is
                              being performed and pui8RbduBuffer != IMG_NULL.

 @Input     pui8RbduBuffer  : A pointer to a buffer used for RBDU extraction.
                              IMG_NULL if the bit stream data is not
                              encapsulated.

 @Input     ui32SizeRbduBuffer : The size of of the RBDU buffer. Ignored if
                              pui8RbduBuffer is IMG_NULL.

 @Input     eEmPrevent      : The emulation prevention scheme to be used. Ignored if
                              pui8RbduBuffer is IMG_NULL;

 @Return	None.

******************************************************************************/
extern IMG_RESULT
SWSR_Initialise(
    SWSR_pfnExceptHandler   pfnExceptionHandler,
    IMG_VOID              * pvExceptionCbParam,
    SWSR_pfnCallback        pfnCallback,
    IMG_VOID              * pvCbParam,
    IMG_HANDLE            * phContext
);


#if defined(__cplusplus)
}
#endif

#endif  /* __SWSR_H__ */

