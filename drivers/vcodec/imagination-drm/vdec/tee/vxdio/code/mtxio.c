/*!
 *****************************************************************************
 *
 * @File       mtxio.c
 * @Title      Low-level MTX interface component
 * @Description    This file contains functions to communicate with MTX firmware.  This
 *  includes sending and receiving messages and state indicators.
 *  It is a subcomponent of the Decoder block and operates on the Decoder
 *  block state.
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

#include <img_errors.h>
#include <img_types.h>
#include <lst.h>
#include <report_api.h>

#include <vdecfw_msg_mem_io.h>
#include <msvdx_mtx_reg_io2.h>
#include <vdecfw.h>

#include "mtxio.h"
#include "msvdx_io.h"

#define EXCLUDE_PDUMP

/*! Value to reset all of VEC RAM too at start */
#define MTXIO_VEC_RAM_RESET_VAL   (0x0)


/*! Value for ui32TimeOut in call to TALREG_CircBufPoll32 - should approximate to 1ms */
#define MTXIO_TAL_NUM_CYCLES_IN_1MS     (100)
/*! Value for ui32TimeOut in call to TALREG_Poll32 - should approximate to 100us */
#define MTXIO_TAL_NUM_CYCLES_IN_100US   (MTXIO_TAL_NUM_CYCLES_IN_1MS/10)
/*! Value for ui32PollCount in call to TALREG_Poll32 - should be long enough for FW to write a message
 *  If ui32TimeOut is correctly set to 100us, 0x0013_0000 should equate to just over 2mins
 *  Against real silicon we expect messages be produced at an interval around 30ms */
#define MTXIO_TAL_POLL_REPEAT_COUNT     (0x00130000)



/*!
******************************************************************************
 Enum describing fields in the #VDECFW_sCommsHeader structure
******************************************************************************/
typedef enum
{
    MTXIO_FIELD_SIZE = 0,
    MTXIO_FIELD_RD_INDEX,
    MTXIO_FIELD_WRT_INDEX,
    MTXIO_FIELD_OFFSET_INDEX,

    MTXIO_FIELD_MAX,            //!< end marker

} MTXIO_eFieldId;


/*!
******************************************************************************
 Type for a portion of memory equal to the largest message buffer
******************************************************************************/
typedef struct
{
    union {
        IMG_UINT32 aui32ControlMsgHdr[VDECFW_CONTROL_COMMS_BUF_SIZE];
        IMG_UINT32 aui3DecodeMsgHdr[VDECFW_DECODE_COMMS_BUF_SIZE];
        IMG_UINT32 aui3CompletionMsgHdr[VDECFW_COMPLETION_COMMS_BUF_SIZE];
    };

}MTXIO_sCommsBuf;


/*!
******************************************************************************
 Offset in VLR of each of the comms areas
******************************************************************************/
static const IMG_UINT32 gaui32VlrOffset[MTXIO_AREA_MAX] =
{
    VLR_CONTROL_COMMS_AREA_BASE_ADDR,        // MTXIO_AREA_CONTROL
    VLR_DECODE_COMMS_AREA_BASE_ADDR,         // MTXIO_AREA_DECODE
    VLR_COMPLETION_COMMS_AREA_BASE_ADDR,     // MTXIO_AREA_COMPLETION
};



/*!
******************************************************************************

 @Function              mtxio_commsWriteWords

******************************************************************************/
static IMG_RESULT
mtxio_commsWriteWords(
    const MTXIO_sContext  * psContext,
    VXD_eCommsArea          eCommsArea,
    IMG_UINT32              ui32Offset,
    IMG_UINT32              ui32NumWords,
    const IMG_UINT32      * pui32Values
)
{
    return MSVDXIO_SEC_VLRWriteWords(psContext->hMsvdxIoCtx,
                                     psContext->asComms[eCommsArea].eMemSpace,
                                     gaui32VlrOffset[eCommsArea] + psContext->asComms[eCommsArea].ui32BufOffset + ui32Offset,
                                     ui32NumWords,
                                     pui32Values);
}


/*!
******************************************************************************

 @Function              mtxio_commsReadWords

******************************************************************************/
static IMG_RESULT
mtxio_commsReadWords(
    const MTXIO_sContext  * psContext,
    VXD_eCommsArea          eCommsArea,
    IMG_UINT32              ui32Offset,
    IMG_UINT32              ui32NumWords,
    IMG_UINT32            * pui32Values,
    IMG_BOOL                bValidate
)
{
    return MSVDXIO_SEC_VLRReadWords(psContext->hMsvdxIoCtx,
                                    psContext->asComms[eCommsArea].eMemSpace,
                                    gaui32VlrOffset[eCommsArea] + ui32Offset,
                                    ui32NumWords,
                                    pui32Values,
                                    bValidate);
}


/*!
******************************************************************************

 @Function              mtxio_commsAreaSet

******************************************************************************/
static IMG_RESULT
mtxio_commsAreaSet(
    const MTXIO_sContext  * psContext,
    VXD_eCommsArea          eArea,
    MTXIO_eFieldId          eFieldId,
    IMG_UINT32              ui32WriteValue
)
{
    const MTXIO_sCommsInfo    * psCommsInfo;
    IMG_UINT32                  ui32Address = 0;
    IMG_UINT32                  ui32AreaOffset;
    IMG_UINT32                  ui32Result;

    if (eArea >= MTXIO_AREA_MAX)
    {
        REPORT(REPORT_MODULE_MTXIO, REPORT_ERR,
               "Trying to set field in invalid area %u!", eArea);
        return IMG_ERROR_INVALID_PARAMETERS;
    }

    /* get comms area information and offset for it in VLR */
    psCommsInfo = &psContext->asComms[eArea];
    ui32AreaOffset = gaui32VlrOffset[eArea];

    switch (eFieldId)
    {
    case MTXIO_FIELD_RD_INDEX:
        ui32Address = ui32AreaOffset + psCommsInfo->ui32RdIndexOffset;
        break;
    case MTXIO_FIELD_WRT_INDEX:
        ui32Address = ui32AreaOffset + psCommsInfo->ui32WrtIndexOffset;
        break;
    case MTXIO_FIELD_SIZE:
        // Setting the size field is permitted as this is required at start-up
        // but this is a constant and must no change
        ui32Address = ui32AreaOffset + psCommsInfo->ui32SizeOffset;
        ui32WriteValue = psCommsInfo->ui32Size;
        break;
    case MTXIO_FIELD_OFFSET_INDEX:
    default:
        REPORT(REPORT_MODULE_MTXIO, REPORT_ERR,
               "Can not write to field %u!", eFieldId);
        return IMG_ERROR_INVALID_PARAMETERS;
    }

    ui32Result = MSVDXIO_SEC_VLRWriteWords(psContext->hMsvdxIoCtx,
                                           psCommsInfo->eMemSpace,
                                           ui32Address,
                                           1,
                                           &ui32WriteValue);
    return ui32Result;
}


/*!
******************************************************************************

 @Function              mtxio_commsAreaGet

******************************************************************************/
static IMG_RESULT
mtxio_commsAreaGet(
    const MTXIO_sContext  * psContext,
    VXD_eCommsArea          eArea,
    MTXIO_eFieldId          eFieldId,
    IMG_UINT32            * pui32ReadValue
)
{
    const MTXIO_sCommsInfo    * psCommsInfo;
    IMG_UINT32                  ui32Address = 0;
    IMG_UINT32                  ui32AreaOffset = 0;
    IMG_UINT32                  ui32ReadValue = 0;
    IMG_UINT32                  ui32Result;

    if (eArea >= MTXIO_AREA_MAX)
    {
        REPORT(REPORT_MODULE_MTXIO, REPORT_ERR,
               "Trying to set field in invalid area %u!", eArea);
        return IMG_ERROR_INVALID_PARAMETERS;
    }

    /* get comms area information and offset for it in VLR */
    psCommsInfo = &psContext->asComms[eArea];
    ui32AreaOffset = gaui32VlrOffset[eArea];

    switch ( eFieldId )
    {
    case MTXIO_FIELD_RD_INDEX:
        ui32Address = ui32AreaOffset + psCommsInfo->ui32RdIndexOffset;
        break;
    case MTXIO_FIELD_WRT_INDEX:
        ui32Address = ui32AreaOffset + psCommsInfo->ui32WrtIndexOffset;
        break;
    case MTXIO_FIELD_SIZE:
        ui32Address = ui32AreaOffset + psCommsInfo->ui32SizeOffset;
        break;
    case MTXIO_FIELD_OFFSET_INDEX:
    default:
        REPORT(REPORT_MODULE_MTXIO, REPORT_ERR,
               "Can not write to field %u!", eFieldId);
        return IMG_ERROR_INVALID_PARAMETERS;
    }

    ui32Result = MSVDXIO_SEC_VLRReadWords(psContext->hMsvdxIoCtx,
                                          psCommsInfo->eMemSpace,
                                          ui32Address,
                                          1,
                                          &ui32ReadValue,
                                          IMG_FALSE);
    *pui32ReadValue = ui32ReadValue;

    return ui32Result;
}


/*
******************************************************************************

 @Function              MTXIO_InitMTXComms

******************************************************************************/
IMG_RESULT
MTXIO_InitMTXComms(
    const IMG_HANDLE    hMsvdxIoCtx,
    MTXIO_sContext    * psMtxIoCtx
)
{
    IMG_UINT32  ui32Offset;
    IMG_UINT32  ui32VecRamVal = 0;
    IMG_UINT32  ui32Result;

    /* setup buffer pointers, sizes and handles */
    VDECFW_sCommsControl * psControlArea = IMG_NULL;
    VDECFW_sCommsDecode * psDecodeArea = IMG_NULL;
    VDECFW_sCommsCompletion * psCompletionArea = IMG_NULL;

    IMG_MEMSET(psMtxIoCtx, 0, sizeof(*psMtxIoCtx));

    psMtxIoCtx->hMsvdxIoCtx = hMsvdxIoCtx;

    /* Initially set all areas to general VLRFE_REGSPACE so the setup of VLR memory comes to FE Pdump Context */
    psMtxIoCtx->asComms[MTXIO_AREA_CONTROL].eMemSpace = REGION_VLRFE_REGSPACE;
    psMtxIoCtx->asComms[MTXIO_AREA_DECODE].eMemSpace = REGION_VLRFE_REGSPACE;
    psMtxIoCtx->asComms[MTXIO_AREA_COMPLETION].eMemSpace = REGION_VLRFE_REGSPACE;

    /* Set field offsets */
    psMtxIoCtx->asComms[MTXIO_AREA_CONTROL].ui32SizeOffset =
        (IMG_UINT32)((IMG_UINTPTR)&psControlArea->sHeader.ui32BufSize - (IMG_UINTPTR)&psControlArea->sHeader);
    psMtxIoCtx->asComms[MTXIO_AREA_CONTROL].ui32RdIndexOffset =
        (IMG_UINT32)((IMG_UINTPTR)&psControlArea->sHeader.ui32RdIndex - (IMG_UINTPTR)&psControlArea->sHeader);
    psMtxIoCtx->asComms[MTXIO_AREA_CONTROL].ui32WrtIndexOffset =
        (IMG_UINT32)((IMG_UINTPTR)&psControlArea->sHeader.ui32WrtIndex - (IMG_UINTPTR)&psControlArea->sHeader);
    psMtxIoCtx->asComms[MTXIO_AREA_CONTROL].ui32BufOffset =
        (IMG_UINT32)((IMG_UINTPTR)&psControlArea->sHeader.aui32Buf - (IMG_UINTPTR)&psControlArea->sHeader);

    psMtxIoCtx->asComms[MTXIO_AREA_DECODE].ui32SizeOffset =
        (IMG_UINT32)((IMG_UINTPTR)&psDecodeArea->sHeader.ui32BufSize - (IMG_UINTPTR)&psDecodeArea->sHeader);
    psMtxIoCtx->asComms[MTXIO_AREA_DECODE].ui32RdIndexOffset =
        (IMG_UINT32)((IMG_UINTPTR)&psDecodeArea->sHeader.ui32RdIndex - (IMG_UINTPTR)&psDecodeArea->sHeader);
    psMtxIoCtx->asComms[MTXIO_AREA_DECODE].ui32WrtIndexOffset =
        (IMG_UINT32)((IMG_UINTPTR)&psDecodeArea->sHeader.ui32WrtIndex - (IMG_UINTPTR)&psDecodeArea->sHeader);
    psMtxIoCtx->asComms[MTXIO_AREA_DECODE].ui32BufOffset =
        (IMG_UINT32)((IMG_UINTPTR)&psDecodeArea->sHeader.aui32Buf - (IMG_UINTPTR)&psDecodeArea->sHeader);

    psMtxIoCtx->asComms[MTXIO_AREA_COMPLETION].ui32SizeOffset =
        (IMG_UINT32)((IMG_UINTPTR)&psCompletionArea->sHeader.ui32BufSize - (IMG_UINTPTR)&psCompletionArea->sHeader);
    psMtxIoCtx->asComms[MTXIO_AREA_COMPLETION].ui32RdIndexOffset =
        (IMG_UINT32)((IMG_UINTPTR)&psCompletionArea->sHeader.ui32RdIndex - (IMG_UINTPTR)&psCompletionArea->sHeader);
    psMtxIoCtx->asComms[MTXIO_AREA_COMPLETION].ui32WrtIndexOffset =
        (IMG_UINT32)((IMG_UINTPTR)&psCompletionArea->sHeader.ui32WrtIndex - (IMG_UINTPTR)&psCompletionArea->sHeader);
    psMtxIoCtx->asComms[MTXIO_AREA_COMPLETION].ui32BufOffset =
        (IMG_UINT32)((IMG_UINTPTR)&psCompletionArea->sHeader.aui32Buf - (IMG_UINTPTR)&psCompletionArea->sHeader);

    /* set buffer sizes - in 32-bit words */
    psMtxIoCtx->asComms[MTXIO_AREA_CONTROL].ui32Size =
        (VLR_CONTROL_COMMS_AREA_SIZE - psMtxIoCtx->asComms[MTXIO_AREA_CONTROL].ui32BufOffset) >> 2;
    psMtxIoCtx->asComms[MTXIO_AREA_DECODE].ui32Size =
        (VLR_DECODE_COMMS_AREA_SIZE - psMtxIoCtx->asComms[MTXIO_AREA_DECODE].ui32BufOffset) >> 2;
    psMtxIoCtx->asComms[MTXIO_AREA_COMPLETION].ui32Size =
        (VLR_COMPLETION_COMMS_AREA_SIZE - psMtxIoCtx->asComms[MTXIO_AREA_COMPLETION].ui32BufOffset) >> 2;

    /* clear VLR */
    ui32VecRamVal = MTXIO_VEC_RAM_RESET_VAL;
    for (ui32Offset = 0; ui32Offset < VLR_SIZE; ui32Offset += 4)
    {
        ui32Result = MSVDXIO_SEC_VLRWriteWords(hMsvdxIoCtx,
                                               REGION_VLRFE_REGSPACE,
                                               ui32Offset,
                                               1,
                                               &ui32VecRamVal);
        if (ui32Result != IMG_SUCCESS)
        {
            REPORT(REPORT_MODULE_MTXIO, REPORT_ERR,
                   "Failed to clear comms RAM!");
            return ui32Result;
        }
    }

    /* set up message buffers ready to use */
    /* size is forced to a value derived from system.h in mtxio_initCommsInfo() */
    ui32Result = mtxio_commsAreaSet(psMtxIoCtx, MTXIO_AREA_CONTROL,
                                    MTXIO_FIELD_SIZE, 0);
    if (ui32Result != IMG_SUCCESS)
    {
        REPORT(REPORT_MODULE_MTXIO, REPORT_ERR,
               "Failed to configure message buffer size");
        return ui32Result;
    }
    ui32Result = mtxio_commsAreaSet(psMtxIoCtx, MTXIO_AREA_DECODE,
                                    MTXIO_FIELD_SIZE, 0);
    if (ui32Result != IMG_SUCCESS)
    {
        REPORT(REPORT_MODULE_MTXIO, REPORT_ERR,
               "Failed to configure message buffer size");
        return ui32Result;
    }
    ui32Result = mtxio_commsAreaSet(psMtxIoCtx, MTXIO_AREA_COMPLETION,
                                    MTXIO_FIELD_SIZE, 0);
    if (ui32Result != IMG_SUCCESS)
    {
        REPORT(REPORT_MODULE_MTXIO, REPORT_ERR,
               "Failed to configure message buffer size");
        return ui32Result;
    }

    /* After setup set each area to the right memspace so they come up to the right Pdump context */
    psMtxIoCtx->asComms[MTXIO_AREA_COMPLETION].eMemSpace = REGION_VLRBE_REGSPACE;

    return IMG_SUCCESS;
}

/*
******************************************************************************

 @Function              mtxio_sendPadMsg

******************************************************************************/
static IMG_RESULT
mtxio_sendPadMsg(
    const MTXIO_sContext  * psContext,
    VXD_eCommsArea          eArea,
    IMG_UINT32              ui32BufferSize,
    IMG_UINT32              ui32ReadIdx,
    IMG_UINT32            * pui32WriteIdx
)
{
    IMG_UINT8       ui8PaddingID;
    IMG_RESULT      ui32Result;
    MTXIO_sCommsBuf sCommsBuf;
    IMG_UINT32    * pui32PaddingHeader = sCommsBuf.aui32ControlMsgHdr;
    IMG_UINT32      ui32PaddingWords, ui32PaddingSize;

    // prepare a padding message
    ui8PaddingID = VDECFW_MSGID_BASE_PADDING;
    if ( MTXIO_AREA_DECODE == eArea )
    {
        ui8PaddingID = VDECFW_MSGID_PSR_PADDING;
    }
    IMG_MEMSET(pui32PaddingHeader, 0, sizeof(MTXIO_sCommsBuf));

    ui32PaddingWords = ui32BufferSize - *pui32WriteIdx;
    ui32PaddingSize = (ui32PaddingWords * 4);

    if ((ui32PaddingSize > 0xFF) || (ui32BufferSize < *pui32WriteIdx))
    {
        REPORT(REPORT_MODULE_MTXIO, REPORT_ERR,
               "Comms area seems to be corrupted, wrong write index!"
               "buf: %u, pad: %u, wr idx: %u",
               ui32BufferSize, ui32PaddingWords, *pui32WriteIdx);
        return IMG_ERROR_GENERIC_FAILURE;
    }

    /* Make sure that there's enough space in comms RAM for padding message */
    if (*pui32WriteIdx < ui32ReadIdx)
    {
        REPORT(REPORT_MODULE_MTXIO, REPORT_WARNING,
               "Not enough space in comms RAM for padding message:"
               " w: %u, r: %u, buf: %u, pad: %u",
               *pui32WriteIdx, ui32ReadIdx, ui32BufferSize, ui32PaddingWords);
        return IMG_ERROR_BUSY;
    }

    MEMIO_WRITE_FIELD(pui32PaddingHeader, V2_PADMSG_SIZE, ui32PaddingSize);
    MEMIO_WRITE_FIELD(pui32PaddingHeader, V2_PADMSG_MID, ui8PaddingID);

    // write the message
    ui32Result = mtxio_commsWriteWords(psContext, eArea,
                                       (*pui32WriteIdx<<2), ui32PaddingWords,
                                       pui32PaddingHeader);
    if (ui32Result != IMG_SUCCESS) 
    {
        REPORT(REPORT_MODULE_MTXIO, REPORT_ERR,
               "Failed to write the padding message"
               ", area: %u, wr idx: %u, size: %u!",
               eArea, *pui32WriteIdx, ui32PaddingWords);
        return ui32Result;
    }

    *pui32WriteIdx = 0;
    ui32Result = mtxio_commsAreaSet(psContext, eArea,
                                    MTXIO_FIELD_WRT_INDEX, *pui32WriteIdx);
    if (ui32Result != IMG_SUCCESS)
    {
        REPORT(REPORT_MODULE_MTXIO, REPORT_ERR,
               "Failed to update write idx, area: %u, write idx: %u!",
               eArea, *pui32WriteIdx);
        return ui32Result;
    }

    return IMG_SUCCESS;
}
/*
******************************************************************************

 @Function              MTXIO_SendMTXMsg

******************************************************************************/
IMG_RESULT
MTXIO_SendMTXMsg(
    const MTXIO_sContext  * psContext,
    VXD_eCommsArea          eArea,
    const IMG_VOID        * psMsgHdr,
    IMG_UINT32              ui32MsgBufSize
)
{
    IMG_UINT32      ui32BufferSize, ui32WriteIdx, ui32ReadIdx;
    IMG_UINT32    * pui32Message = (IMG_UINT32*)psMsgHdr;
    IMG_RESULT      ui32Result;

    if ((eArea != MTXIO_AREA_CONTROL) && (eArea != MTXIO_AREA_DECODE))
    {
        REPORT(REPORT_MODULE_MTXIO, REPORT_ERR,
               "Invalid message area requested: %u!",
               eArea);
        return IMG_ERROR_INVALID_PARAMETERS;
    }

    /* Transform size to words */
    ui32MsgBufSize = (ui32MsgBufSize + 3) / 4;

    /* Obtain properties of a ring buffer in comms RAM: size ...  */
    ui32Result = mtxio_commsAreaGet(psContext, eArea,
                                    MTXIO_FIELD_SIZE, &ui32BufferSize);
    if (ui32Result != IMG_SUCCESS)
    {
        REPORT(REPORT_MODULE_MTXIO, REPORT_ERR,
               "Failed to get size of area %u!", eArea);
        return ui32Result;
    }

    /* ... write index ... */
    ui32Result = mtxio_commsAreaGet(psContext, eArea,
                                    MTXIO_FIELD_WRT_INDEX, &ui32WriteIdx);

    if (ui32Result != IMG_SUCCESS)
    {
        REPORT(REPORT_MODULE_MTXIO, REPORT_ERR,
               "Failed to get wrt idx for area %u!", eArea);
        return ui32Result;
    }

    /* ... and read index. */
    ui32Result = mtxio_commsAreaGet(psContext, eArea,
                                    MTXIO_FIELD_RD_INDEX, &ui32ReadIdx);
    if (ui32Result != IMG_SUCCESS)
    {
        REPORT(REPORT_MODULE_MTXIO, REPORT_ERR,
               "Failed to get read idx for area %u!", eArea);
        return ui32Result;
    }

    /* Validate the parameters */
    if (psContext->asComms[eArea].ui32Size != ui32BufferSize)
    {
        REPORT(REPORT_MODULE_MTXIO, REPORT_ERR,
               "Comms area seems to be corrupted, wrong buffer size!");
        return IMG_ERROR_GENERIC_FAILURE;
    }

    if (ui32MsgBufSize > ui32BufferSize)
    {
        REPORT(REPORT_MODULE_MTXIO, REPORT_ERR,
               "Enormous message submitted, msg: %u, buf: %u!",
               ui32MsgBufSize, ui32BufferSize);
        return IMG_ERROR_INVALID_PARAMETERS;
    }

    /* Start submitting the message to comms RAM */
    /* If there is no enough space in the buffer, send a padding message */
    if ((ui32WriteIdx + ui32MsgBufSize) > ui32BufferSize)
    {
        ui32Result = mtxio_sendPadMsg(psContext, eArea, ui32BufferSize,
                                      ui32ReadIdx, &ui32WriteIdx);
        if (ui32Result != IMG_SUCCESS)
        {
            return ui32Result;
        }
    }

    /* Make sure there's enough space to write the message */
    if ((ui32WriteIdx < ui32ReadIdx) &&
        (ui32WriteIdx + ui32MsgBufSize > ui32ReadIdx))
    {
        REPORT(REPORT_MODULE_MTXIO, REPORT_WARNING,
               "Not enough space in comms RAM for message:"
               " w: %u, r: %u, buf: %u, msg: %u",
               ui32WriteIdx, ui32ReadIdx, ui32BufferSize, ui32MsgBufSize);
        return IMG_ERROR_BUSY;
    }

    /* Write the message */
    ui32Result = mtxio_commsWriteWords(psContext, eArea,
                                       (ui32WriteIdx<<2), ui32MsgBufSize,
                                       pui32Message);
    if (ui32Result != IMG_SUCCESS) 
    {
        REPORT(REPORT_MODULE_MTXIO, REPORT_ERR,
               "Failed to write the message, area: %u, wr idx: %u, size: %u!",
               eArea, ui32WriteIdx, ui32MsgBufSize);
        return ui32Result;
    }

    /* Calculate and set the new write index */
    ui32WriteIdx += ui32MsgBufSize;
    if ( ui32WriteIdx >= ui32BufferSize )
    {
        ui32WriteIdx = 0;
    }
    ui32Result = mtxio_commsAreaSet(psContext, eArea,
                                    MTXIO_FIELD_WRT_INDEX, ui32WriteIdx);

    if (ui32Result != IMG_SUCCESS)
    {
        REPORT(REPORT_MODULE_MTXIO, REPORT_ERR,
               "Failed to update write idx, area: %u, write idx: %u!",
               eArea, ui32WriteIdx);
        return ui32Result;
    }

    return IMG_SUCCESS;
}


/*!
******************************************************************************

 @Function              mtxio_copyMessageData

******************************************************************************/
static IMG_RESULT
mtxio_copyMessageData(
    const MTXIO_sContext  * psContext,
    VXD_eCommsArea          eArea,
    IMG_UINT32              ui32ReadIdx,
    IMG_UINT32              ui32MessageSize,
    IMG_UINT32              ui32BufferSize,
    IMG_UINT32            * pui32CopyMsgBuf,
    IMG_UINT32            * pui32NewReadIdx
)
{
    IMG_UINT32 ui32FirstPartSize = 0;
    IMG_RESULT ui32Result;

    /* if the message wraps in the MTX buffer, read the portion the the end of the buffer */
    if ( (ui32ReadIdx + ui32MessageSize) >= ui32BufferSize )
    {
        ui32FirstPartSize = ui32BufferSize - ui32ReadIdx;

        ui32Result = 
            mtxio_commsReadWords(psContext,
                                 eArea,
                                 psContext->asComms[eArea].ui32BufOffset +
                                    (ui32ReadIdx<<2),
                                 ui32FirstPartSize,
                                 pui32CopyMsgBuf,
                                 IMG_TRUE);

        if (ui32Result != IMG_SUCCESS)
        {
            REPORT(REPORT_MODULE_MTXIO, REPORT_ERR,
                   "Failed to read first part of the message!");
            return ui32Result;
        }

        ui32ReadIdx = 0;
        ui32MessageSize -= ui32FirstPartSize;
    }

    if (ui32MessageSize > 0)
    {
        /* read the (rest of the) message (if there is a rest) */
        ui32Result =
            mtxio_commsReadWords(psContext,
                                 eArea,
                                 psContext->asComms[eArea].ui32BufOffset +
                                    (ui32ReadIdx<<2),
                                 ui32MessageSize,
                                 &pui32CopyMsgBuf[ui32FirstPartSize],
                                 IMG_TRUE);
        if (ui32Result != IMG_SUCCESS)
        {
            REPORT(REPORT_MODULE_MTXIO, REPORT_ERR,
                   "Failed to read second part of the message!");
            return ui32Result;
        }
    }

    ui32ReadIdx += ui32MessageSize;

    *pui32NewReadIdx = ui32ReadIdx;

    return IMG_SUCCESS;
}


/*
******************************************************************************

 @Function              MTXIO_ProcessMTXMsgs

******************************************************************************/
IMG_RESULT
MTXIO_ProcessMTXMsgs(
    MTXIO_sContext    * psContext,
    IMG_UINT32        * pui32MtxMsgs,
    IMG_UINT32        * pui32MsgsSizeWrds,
    IMG_BOOL          * pbMoredata
)
{
    IMG_UINT32 ui32ReadIdx, ui32WriteIdx, ui32BufferSize;
    IMG_UINT32 ui32FirstWord, ui32MessageSize;
    IMG_UINT32 ui32MsgWrdsRead = 0;
    VDECFW_eMessageID eMessageID;
    IMG_RESULT ui32Result;

    /* we can only read messages from the Completion buffer */
    VXD_eCommsArea eArea = MTXIO_AREA_COMPLETION;

    ui32Result = mtxio_commsAreaGet(psContext, eArea,
                                    MTXIO_FIELD_RD_INDEX, &ui32ReadIdx);
    if (ui32Result != IMG_SUCCESS)
    {
        REPORT(REPORT_MODULE_MTXIO, REPORT_ERR,
               "Failed to get rd idx for area %u!", eArea);
        return ui32Result;
    }
    ui32Result = mtxio_commsAreaGet(psContext, eArea,
                                    MTXIO_FIELD_WRT_INDEX, &ui32WriteIdx);
    if (ui32Result != IMG_SUCCESS)
    {
        REPORT(REPORT_MODULE_MTXIO, REPORT_ERR,
               "Failed to get wr idx for area %u!", eArea);
        return ui32Result;
    }
    ui32Result = mtxio_commsAreaGet(psContext, eArea, MTXIO_FIELD_SIZE,
                                    &ui32BufferSize);
    if (ui32Result != IMG_SUCCESS)
    {
        REPORT(REPORT_MODULE_MTXIO, REPORT_ERR,
               "Failed to get size of area %u!", eArea);
        return ui32Result;
    }

    /* While there are messages to read and somewhere to put them. */
    while (ui32ReadIdx != ui32WriteIdx)
    {
        /* read the first word of the message */
        ui32Result = mtxio_commsReadWords(psContext,
                                          eArea,
                                          psContext->asComms[eArea].ui32BufOffset + (ui32ReadIdx<<2),
                                          1,
                                          &ui32FirstWord,
                                          IMG_TRUE);

        if (ui32Result != IMG_SUCCESS)
        {
            REPORT(REPORT_MODULE_MTXIO, REPORT_ERR,
                   "Failed to read first word of the message!");
            return ui32Result;
        }

        /* get the message size in words and ID */
        ui32MessageSize = (MEMIO_READ_FIELD(&ui32FirstWord, V2_PADMSG_SIZE) + 3) / 4;
        eMessageID = MEMIO_READ_FIELD(&ui32FirstWord, V2_PADMSG_MID);

        /* Make sure there's enough space in destination buffer */
        if (ui32MsgWrdsRead + ui32MessageSize > *pui32MsgsSizeWrds)
        {
            break;
        }

        if (ui32MessageSize > ui32BufferSize)
        {
            REPORT(REPORT_MODULE_MTXIO, REPORT_ERR,
                   "Enormous message received, msg: %u, buf: %u!",
                   ui32MessageSize, ui32BufferSize);
            return IMG_ERROR_UNEXPECTED_STATE;
        }

        if ((ui32ReadIdx <= ui32WriteIdx) && (ui32ReadIdx + ui32MessageSize) > ui32WriteIdx)
        {
            REPORT(REPORT_MODULE_MTXIO, REPORT_ERR,
                   "Received message corrupted write area, r: %u, w: %u, s: %u!",
                   ui32ReadIdx, ui32WriteIdx, ui32MessageSize);
            return IMG_ERROR_UNEXPECTED_STATE;
        }

        if (eMessageID < VDECFW_MSGID_BE_PADDING ||
            eMessageID > VDECFW_MSGID_COMPLETION_MAX)
        {
            REPORT(REPORT_MODULE_MTXIO, REPORT_ERR,
                   "Invalid message id received: %u!",
                   eMessageID);
            return IMG_ERROR_UNEXPECTED_STATE;
        }

        /* consume if the message is a padding message */
        if ( VDECFW_MSGID_BE_PADDING == eMessageID )
        {
            /* sanity check - message does infact pad to the end of the buffer */
            IMG_ASSERT( ui32ReadIdx > ui32WriteIdx );
            IMG_ASSERT( (ui32ReadIdx + ui32MessageSize) == ui32BufferSize );
            ui32ReadIdx = 0;
            ui32Result = mtxio_commsAreaSet(psContext, eArea,
                                            MTXIO_FIELD_RD_INDEX, ui32ReadIdx);
            if (ui32Result != IMG_SUCCESS)
            {
                REPORT(REPORT_MODULE_MTXIO, REPORT_ERR,
                       "Failed to update read idx, area: %u, read idx: %u!",
                       eArea, ui32ReadIdx);
                return ui32Result;
            }
            continue;
        }

        /* copy message into internal buffer and get new read index */
        ui32Result = mtxio_copyMessageData(psContext,
                                           eArea,
                                           ui32ReadIdx,
                                           ui32MessageSize,
                                           ui32BufferSize,
                                           &pui32MtxMsgs[ui32MsgWrdsRead],
                                           &ui32ReadIdx);
        if (ui32Result != IMG_SUCCESS)
        {
            REPORT(REPORT_MODULE_MTXIO, REPORT_ERR,
                   "Failed to get message data, r idx: %u, msg size: %u, buf size: %u!",
                   ui32ReadIdx, ui32MessageSize, ui32BufferSize);
            return ui32Result;
        }

        /* update the read index */
        ui32Result = mtxio_commsAreaSet(psContext, eArea,
                                        MTXIO_FIELD_RD_INDEX, ui32ReadIdx);
        if (ui32Result != IMG_SUCCESS)
        {
            REPORT(REPORT_MODULE_MTXIO, REPORT_ERR,
                   "Failed to update read idx, area: %u, read idx: %u!",
                   eArea, ui32ReadIdx);
            return ui32Result;
        }

        ui32MsgWrdsRead += ui32MessageSize;
    }

    /* return number of words written into provided buffer */
    *pui32MsgsSizeWrds = ui32MsgWrdsRead;
    /* return whether there's more data to read from MTX <-> HOST msg buffer */
    *pbMoredata = ui32WriteIdx != ui32ReadIdx;

    return IMG_SUCCESS;
}


/* EOF */
