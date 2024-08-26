/*!
 *****************************************************************************
 *
 * @File       bspp_km.c
 * @Title      VXD Bitstream Buffer Pre-Parser Secure Interface
 * @Description    This file contains the secure interface of VXD Bitstream Buffer Pre-Parser.
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

#include "img_defs.h"
#include "bspp_km.h"
#include "sysos_api_km.h"
#include "vdecdd_int.h"
#include "rman_api.h"
#include "sysmem_utils.h"

#include "secure_defs.h"
#include "secure_msg.h"
#include "secureapi_ree.h"

#ifdef SECURE_MEDIA_REPORTING
#include "report_api.h"
#else
#define REPORT(MODULE, LEVEL, fmt, ...)
#define DEBUG_REPORT(MODULE, fmt, ...)
#endif

static IMG_UINT32 ui32SecureBsppId = 0;

/*!
******************************************************************************

 @Function              BSPP_SecureStreamCreate

******************************************************************************/
IMG_RESULT BSPP_SecureStreamCreate(
    SYSBRG_POINTER_ARG(VDEC_sStrConfigData)  psStrConfigData,
    SYSBRG_POINTER_ARG(IMG_UINT32)           pui32StrId,
    SYSBRG_POINTER_ARG(BSPP_sDdBufArrayInfo) asFWSequence,
    SYSBRG_POINTER_ARG(BSPP_sDdBufArrayInfo) asFWPPS
)
{
    IMG_UINT32                      ui32Result;
    BSPP_SECURE_sStreamCreateArgs * psArgs;
    VDEC_sStrConfigData             sKmStrConfigData;
    IMG_UINT32 i;

    if (SYSBRG_POINTER_FROM_USER(psStrConfigData) == IMG_NULL ||
        SYSBRG_POINTER_FROM_USER(pui32StrId) == IMG_NULL ||
        SYSBRG_POINTER_FROM_USER(asFWSequence) == IMG_NULL ||
        SYSBRG_POINTER_FROM_USER(asFWPPS) == IMG_NULL)
    {
        ui32Result = IMG_ERROR_INVALID_PARAMETERS;
        printk("%s:%d Invalid parameters\n", __FUNCTION__, __LINE__);
        return ui32Result;
    }

    if (ui32SecureBsppId == 0)
    {
        ui32Result = SECURE_REE_GetId(BITSTREAM_SCAN, &ui32SecureBsppId);
        IMG_ASSERT(ui32Result == IMG_SUCCESS);
        if (ui32Result != IMG_SUCCESS)
        {
            REPORT(REPORT_MODULE_BSPP, REPORT_ERR, 
                   "%s:%d Failed to obtain secure BSPP handle (return: %d)", 
                   __FUNCTION__, __LINE__, 
                   ui32Result);
            return ui32Result;
        }
    }

    ui32Result = SYSOSKM_CopyFromUser(&sKmStrConfigData, psStrConfigData, sizeof(sKmStrConfigData));
    IMG_ASSERT(ui32Result == IMG_SUCCESS);
    if (ui32Result != IMG_SUCCESS)
    {
        printk("%s:%d Failed to copy from user (size: %zu, return: %d)\n", __FUNCTION__, __LINE__, sizeof(sKmStrConfigData), ui32Result);
        return ui32Result;
    }

	psArgs = SECURE_REE_GetMsgBuffer(ui32SecureBsppId, sizeof(*psArgs),ENDPOINT_BSPP_STREAM_CREATE);

    psArgs->sStrConfigData      = sKmStrConfigData;
	    
    ui32Result = SYSOSKM_CopyFromUser(&psArgs->asFWSequence[0], asFWSequence, MAX_SEQUENCES_SECURE*sizeof(psArgs->asFWSequence[0]));
    IMG_ASSERT(ui32Result == IMG_SUCCESS);
    if (ui32Result != IMG_SUCCESS)
    {
        printk("%s:%d Failed to copy from user\n", __FUNCTION__, __LINE__);
		goto err;
    }

    if( sKmStrConfigData.eVidStd == VDEC_STD_H264 )
    {
        ui32Result = SYSOSKM_CopyFromUser(&psArgs->asFWPPS[0], asFWPPS, MAX_PPSS_SECURE*sizeof(psArgs->asFWPPS[0]));
        IMG_ASSERT(ui32Result == IMG_SUCCESS);
        if (ui32Result != IMG_SUCCESS)
        {
            printk("%s:%d Failed to copy from user\n", __FUNCTION__, __LINE__);
            goto err;
        }
    }

    /*Obtain buffer information only accesible from kernel*/
    for (i=0; i<MAX_SEQUENCES_SECURE; i++)
    {
        VDECDD_sDdBufMapInfo  * psDdBufMapInfo;
        
        ui32Result = RMAN_GetResource(psArgs->asFWSequence[i].sDdBufInfo.ui32BufMapId, VDECDD_BUFMAP_TYPE_ID, (IMG_VOID **)&psDdBufMapInfo, IMG_NULL);
        IMG_ASSERT(ui32Result == IMG_SUCCESS);
        if (ui32Result != IMG_SUCCESS)
        {
            goto err;
        }

        // This memory should not be mapped in either the UM or KM address space.
        IMG_ASSERT(psArgs->asFWSequence[i].sDdBufInfo.pvCpuVirt == 0);
        IMG_ASSERT(psDdBufMapInfo->sDdBufInfo.pvCpuVirt == 0);

        psArgs->asFWSequence[i].sDdBufInfo.ppaPhysAddr = psDdBufMapInfo->sDdBufInfo.ppaPhysAddr;
        psArgs->asFWSequence[i].sDdBufInfo.pvCpuVirt = psDdBufMapInfo->sDdBufInfo.pvCpuVirt;
        if (psDdBufMapInfo->sDdBufInfo.eMemAttrib & SYS_MEMATTRIB_SECURE)
            psArgs->asFWSequence[i].sDdBufInfo.paSecPhysAddr = psDdBufMapInfo->sDdBufInfo.paSecPhysAddr;
        else
            psArgs->asFWSequence[i].sDdBufInfo.hMemoryAlloc = psDdBufMapInfo->hExtImportHandle;
    }

    if( sKmStrConfigData.eVidStd == VDEC_STD_H264 )
    {
        for (i=0; i<MAX_PPSS_SECURE; i++)
        {
            VDECDD_sDdBufMapInfo  * psDdBufMapInfo;

            ui32Result = RMAN_GetResource(psArgs->asFWPPS[i].sDdBufInfo.ui32BufMapId, VDECDD_BUFMAP_TYPE_ID, (IMG_VOID **)&psDdBufMapInfo, IMG_NULL);
            IMG_ASSERT(ui32Result == IMG_SUCCESS);
            if (ui32Result != IMG_SUCCESS)
            {
                goto err;
            }

            // This memory should not be mapped in either the UM or KM address space.
            IMG_ASSERT(psArgs->asFWPPS[i].sDdBufInfo.pvCpuVirt == 0);
            IMG_ASSERT(psDdBufMapInfo->sDdBufInfo.pvCpuVirt == 0);

            psArgs->asFWPPS[i].sDdBufInfo.ppaPhysAddr = psDdBufMapInfo->sDdBufInfo.ppaPhysAddr;
            psArgs->asFWPPS[i].sDdBufInfo.pvCpuVirt = psDdBufMapInfo->sDdBufInfo.pvCpuVirt;
            if (psArgs->asFWPPS[i].sDdBufInfo.eMemAttrib & SYS_MEMATTRIB_SECURE)
                psArgs->asFWPPS[i].sDdBufInfo.paSecPhysAddr = psDdBufMapInfo->sDdBufInfo.paSecPhysAddr;
            else
                psArgs->asFWPPS[i].sDdBufInfo.hMemoryAlloc = psDdBufMapInfo->hExtImportHandle;
        }
    }

    ui32Result = SECURE_REE_SendMessage(ui32SecureBsppId, (IMG_BYTE *)psArgs, sizeof(*psArgs), ENDPOINT_BSPP_STREAM_CREATE);
    IMG_ASSERT(ui32Result == IMG_SUCCESS);
    if (ui32Result != IMG_SUCCESS)
    {
        printk("%s:%d Failed to create stream\n", __FUNCTION__, __LINE__);
        goto err;
    }

    ui32Result = SYSOSKM_CopyToUser(pui32StrId, &psArgs->ui32StrId, sizeof(IMG_UINT32));
    IMG_ASSERT(ui32Result == IMG_SUCCESS);
    if (ui32Result != IMG_SUCCESS)
    {
        printk("%s:%d Failed to copy from kernel\n", __FUNCTION__, __LINE__);
        goto err;
    }

err:

	SECURE_REE_ReleaseMsgBuffer(ui32SecureBsppId, psArgs);

    return ui32Result;
}


/*!
******************************************************************************

 @Function              BSPP_SecureStreamDestroy

******************************************************************************/
IMG_RESULT BSPP_SecureStreamDestroy(
    IMG_UINT32            ui32StrId
)
{
    IMG_UINT32                      ui32Result;
    BSPP_SECURE_sStreamDestroyArgs  sArgs;

    sArgs.ui32StrId = ui32StrId;

    ui32Result = SECURE_REE_SendMessage(ui32SecureBsppId, (IMG_BYTE *)&sArgs, sizeof(sArgs), ENDPOINT_BSPP_STREAM_DESTROY);
    IMG_ASSERT(ui32Result == IMG_SUCCESS);
    if (ui32Result != IMG_SUCCESS)
    {
        printk("%s:%d Failed to destroy stream\n", __FUNCTION__, __LINE__);
        goto err;
    }

err:
    return ui32Result;
}


/*!
******************************************************************************

 @Function              BSPP_SecureStreamSubmitBuffer

******************************************************************************/
IMG_RESULT BSPP_SecureStreamSubmitBuffer(
    IMG_UINT32                      ui32StrId,
    IMG_UINT32                      ui32BufMapId,
    IMG_UINT32                      ui32DataSize,
    VDEC_eBstrElementType           eBstrElementType,
    SYSBRG_POINTER_ARG(IMG_VOID)    pvPictTagParam
)
{
    IMG_UINT32                              ui32Result;
    BSPP_SECURE_sStreamSubmitBufferArgs     sArgs;
    VDECDD_sDdBufMapInfo                  * psDdBufMapInfo;
    BSPP_sDdBufInfo                       * psBsppDdBufInfo;
        
    sArgs.eBstrElementType = eBstrElementType;
    sArgs.ui32StrId        = ui32StrId;
    sArgs.ui32DataSize     = ui32DataSize;
    // This is a userspace pointer. lets hope it is not used in kernel space
    sArgs.ui64PictTagParam   = (IMG_UINTPTR) SYSBRG_POINTER_FROM_USER(pvPictTagParam);
    sArgs.ui32BufMapId     = ui32BufMapId;

    psBsppDdBufInfo = IMG_MALLOC(sizeof(*psBsppDdBufInfo));
	IMG_ASSERT(psBsppDdBufInfo != IMG_NULL);
	if(psBsppDdBufInfo == IMG_NULL)
	{
        printk("%s:%d Failed to allocate buffer\n", __FUNCTION__, __LINE__);
		return IMG_ERROR_MALLOC_FAILED;
	}

    //Get access to map info context
    ui32Result = RMAN_GetResource(ui32BufMapId, VDECDD_BUFMAP_TYPE_ID, (IMG_VOID **)&psDdBufMapInfo, IMG_NULL);
    IMG_ASSERT(ui32Result == IMG_SUCCESS);
    if (ui32Result != IMG_SUCCESS)
    {
        IMG_FREE(psBsppDdBufInfo);
        return ui32Result;
    }

    IMG_ASSERT(psDdBufMapInfo->hExtImportHandle);

    psBsppDdBufInfo->ui32BufSize = psDdBufMapInfo->sDdBufInfo.ui32BufSize;
    psBsppDdBufInfo->pvCpuVirt = psDdBufMapInfo->sDdBufInfo.pvCpuVirt;
    psBsppDdBufInfo->ppaPhysAddr = psDdBufMapInfo->sDdBufInfo.ppaPhysAddr;
    psBsppDdBufInfo->eMemAttrib = psDdBufMapInfo->sDdBufInfo.eMemAttrib;
    if (psDdBufMapInfo->sDdBufInfo.eMemAttrib & SYS_MEMATTRIB_SECURE)
        psBsppDdBufInfo->paSecPhysAddr = psDdBufMapInfo->sDdBufInfo.paSecPhysAddr;
    else
        psBsppDdBufInfo->hMemoryAlloc = psDdBufMapInfo->hExtImportHandle;
    psBsppDdBufInfo->ui32BufMapId = ui32BufMapId;

	sArgs.sDdBufInfo = (*psBsppDdBufInfo);

    ui32Result = SECURE_REE_SendMessage(ui32SecureBsppId, (IMG_BYTE *)&sArgs, sizeof(sArgs), ENDPOINT_BSPP_STREAM_SUBMIT_BUFFER);
    IMG_ASSERT(ui32Result == IMG_SUCCESS);
    if (ui32Result != IMG_SUCCESS)
    {
        printk("%s:%d Failed to submit stream buffer\n", __FUNCTION__, __LINE__);
        goto err;
    }

err:
    IMG_FREE(psBsppDdBufInfo);

    return ui32Result;
}


/*!
******************************************************************************

 @Function              BSPP_SecureSubmitPictureDecoded

 @Description

******************************************************************************/
IMG_RESULT
BSPP_SecureSubmitPictureDecoded(
    IMG_UINT32     ui32StrId,
    IMG_UINT32     ui32SequHdrId,
    IMG_UINT32     ui32PPSId,
    IMG_UINT32     ui32SecondPPSId
)
{
    IMG_UINT32                             ui32Result;
    BSPP_SECURE_sSubmitPictureDecodedArgs  sArgs;

    sArgs.ui32StrId       = ui32StrId;
    sArgs.ui32PPSId       = ui32PPSId;
    sArgs.ui32SecondPPSId = ui32SecondPPSId;
    sArgs.ui32SequHdrId   = ui32SequHdrId;

    ui32Result = SECURE_REE_SendMessage(ui32SecureBsppId, (IMG_BYTE *)&sArgs, sizeof(sArgs), ENDPOINT_BSPP_SUBMIT_PICTURE_DECODED);
    IMG_ASSERT(ui32Result == IMG_SUCCESS);
    if (ui32Result != IMG_SUCCESS)
    {
        printk("%s:%d Failed to submit picture decoded\n", __FUNCTION__, __LINE__);
        goto err;
    }

err:
    return ui32Result;
}


/*!
******************************************************************************

 @Function              BSPP_SecureStreamPreParseBuffers

 @Description

******************************************************************************/
IMG_RESULT BSPP_SecureStreamPreParseBuffers(
    IMG_UINT32                                        ui32StrId,
    IMG_UINT32                                        ui32NumOfSegs,
    SYSBRG_POINTER_ARG(BSPP_sPreParsedData)           psPreParsedData,
    IMG_UINT32                                        ui32ContiguousBufMapId
)
{
    IMG_UINT32                                  ui32Result;
    BSPP_SECURE_sStreamPreParseBuffersArgs    * psArgs;

    VDECDD_sDdBufMapInfo                      * psDdContiguousBufMapInfo = IMG_NULL;
    BSPP_sDdBufInfo                           * psBsppContiguousBufInfo = IMG_NULL;

    psArgs = SECURE_REE_GetMsgBuffer(ui32SecureBsppId, sizeof(*psArgs),ENDPOINT_BSPP_STREAM_PREPARSE_BUFFERS);
	if(psArgs == IMG_NULL)
	{
		ui32Result = IMG_ERROR_MALLOC_FAILED;
		goto error_malloc1;
	}
	
	psArgs->ui32StrId = ui32StrId;
	psArgs->ui32SegmentCount = ui32NumOfSegs;
	    
    ui32Result = SYSOSKM_CopyFromUser(&psArgs->sPreParsedData, psPreParsedData, sizeof(psArgs->sPreParsedData));
    IMG_ASSERT(ui32Result == IMG_SUCCESS);
    if (ui32Result != IMG_SUCCESS)
    {
        printk("%s:%d Failed to copy from user\n", __FUNCTION__, __LINE__);
		goto error;
    }

    
    if(ui32ContiguousBufMapId != 0)
    {
        psBsppContiguousBufInfo = IMG_MALLOC(sizeof(*psBsppContiguousBufInfo));
        IMG_ASSERT(psBsppContiguousBufInfo != IMG_NULL);
        if(psBsppContiguousBufInfo == IMG_NULL)
        {
            printk("%s:%d Failed to allocate buffer\n", __FUNCTION__, __LINE__);
            ui32Result = IMG_ERROR_MALLOC_FAILED;
			goto error;
        }

        //Get access to map info context
        ui32Result = RMAN_GetResource(ui32ContiguousBufMapId, VDECDD_BUFMAP_TYPE_ID, (IMG_VOID **)&psDdContiguousBufMapInfo, IMG_NULL);
        IMG_ASSERT(ui32Result == IMG_SUCCESS);
        if (ui32Result != IMG_SUCCESS)
        {
            goto error;
        }
        psBsppContiguousBufInfo->ui32BufSize = psDdContiguousBufMapInfo->sDdBufInfo.ui32BufSize;
        psBsppContiguousBufInfo->pvCpuVirt = psDdContiguousBufMapInfo->sDdBufInfo.pvCpuVirt;
        psBsppContiguousBufInfo->ppaPhysAddr = psDdContiguousBufMapInfo->sDdBufInfo.ppaPhysAddr;
        psBsppContiguousBufInfo->eMemAttrib = psDdContiguousBufMapInfo->sDdBufInfo.eMemAttrib;
        psBsppContiguousBufInfo->ui32BufMapId = ui32ContiguousBufMapId;      
    }

    if (psBsppContiguousBufInfo)
	{
		psArgs->sContiguousBufInfo = *psBsppContiguousBufInfo;
	}
    psArgs->ui32ContiguousBufMapId = ui32ContiguousBufMapId;

    ui32Result = SECURE_REE_SendMessage(ui32SecureBsppId, (IMG_BYTE *)psArgs, sizeof(*psArgs), ENDPOINT_BSPP_STREAM_PREPARSE_BUFFERS);
    IMG_ASSERT((ui32Result == IMG_SUCCESS) || (ui32Result == IMG_ERROR_NOT_SUPPORTED));
    if (ui32Result != IMG_SUCCESS)
    {
        printk("%s:%d Failed to pre-parse buffers\n", __FUNCTION__, __LINE__);
        goto error;
    }



    ui32Result = SYSOSKM_CopyToUser(psPreParsedData, &psArgs->sPreParsedData, sizeof(psArgs->sPreParsedData));
    IMG_ASSERT(ui32Result == IMG_SUCCESS);
    if (ui32Result != IMG_SUCCESS)
    {
        printk("%s:%d Failed to copy from kernel\n", __FUNCTION__, __LINE__);
        goto error;
    }

	SECURE_REE_ReleaseMsgBuffer(ui32SecureBsppId, psArgs);

    return IMG_SUCCESS;

error:
	SECURE_REE_ReleaseMsgBuffer(ui32SecureBsppId, psArgs);
error_malloc1:	

    return ui32Result;
}
