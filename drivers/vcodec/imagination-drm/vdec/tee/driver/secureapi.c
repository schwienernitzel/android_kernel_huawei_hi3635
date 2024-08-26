/*!
 *****************************************************************************
 *
 * @File       secureapi.c
 * @Title      Secure Decode API
 * @Description    This file contains the Secure Decode API.
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

#include <vdec_api.h>
#include <bspp_km.h>
#include <msvdx_io.h>
#include <secureapi.h>
#include <securemem.h>
#include <report_api.h>

#include "vxd_secure_msg.h"

#define SEC_CopyFromKmToSecure(pSmAddr, pKmAddr, size)  (IMG_MEMCPY(pSmAddr, pKmAddr, size))
#define SEC_CopyFromSecureToKm(pKmAddr, pSmAddr, size)  (IMG_MEMCPY(pKmAddr, pSmAddr, size))


typedef struct
{
    IMG_VIDEO_CORE      nCore;
    IMG_UINT32          ui32RefCount;
    IMG_VOID          * pvData;
    IMG_UINT32          ui32Id;

} SECURE_sCoreContext;


SECURE_sCoreContext asContext[SECURE_CORE_MAX] =
{
    {VXD_CORE0,      0, IMG_NULL, 0},
    {VXD_CORE1,      0, IMG_NULL, 0},
    {BITSTREAM_SCAN, 0, IMG_NULL, 0},
    {VXE0,           0, IMG_NULL, 0},
    {FELIX,          0, IMG_NULL, 0},
};


typedef struct
{
    IMG_UINT32      ui32CoreNum;
    IMG_HANDLE      hVxdIo;

} SECURE_sVXDContext;


#include "idgen_api.h"
static IMG_HANDLE  ghBsppIdGen = IMG_NULL;
static IMG_HANDLE  ghSecureIdGen = IMG_NULL;


/*!
******************************************************************************

 @Function              SECURE_GetId

******************************************************************************/
IMG_RESULT SECURE_GetId(
    IMG_VIDEO_CORE nCore,
    IMG_UINT32    *pui32SecureId
)
{
    SECURE_sCoreContext   * psCoreContext;
    IMG_UINT32              ui32CoreIdx;
    static IMG_BOOL         bFirst = IMG_TRUE;
    IMG_UINT32              ui32Result;

    if (pui32SecureId == IMG_NULL)
    {
        ui32Result = IMG_ERROR_INVALID_PARAMETERS;
        goto error;
    }

    // Obtain the context of the first core found.
    ui32CoreIdx = 0; 
    while (((nCore >> ui32CoreIdx) & 1) == 0)
    {
        ui32CoreIdx++;
    }
    
    // Only one core should be specified.
    if (nCore != (1 << ui32CoreIdx))
    {
        ui32Result = IMG_ERROR_INVALID_PARAMETERS;
        goto error;
    }
    
    if (bFirst)
    {
        ui32Result = IDGEN_CreateContext(8, 8, IMG_FALSE, &ghSecureIdGen);
        if (ui32Result != IMG_SUCCESS)
            goto error;

        bFirst = IMG_FALSE;
    }

    psCoreContext = &asContext[ui32CoreIdx];

    if (psCoreContext->ui32Id == 0)
    {
        ui32Result = IDGEN_AllocId(ghSecureIdGen, psCoreContext, &psCoreContext->ui32Id);
        if (ui32Result != IMG_SUCCESS)
        {
            goto error;
        }
    }
    
    switch (nCore)
    {
    case BITSTREAM_SCAN:
        break;

    case VXD_CORE0:
    case VXD_CORE1:
        if (psCoreContext->ui32RefCount == 0)
        {
            SECURE_sVXDContext * psVxdCtx;

            // Allocate a core context.
            psVxdCtx = IMG_MALLOC(sizeof(*psVxdCtx));
            IMG_ASSERT(psVxdCtx);
            if (psVxdCtx == IMG_NULL)
            {
                REPORT(REPORT_MODULE_VXDIO, REPORT_ERR,
                    "Failed to allocate memory for secure VXD context");

                ui32Result = IMG_ERROR_OUT_OF_MEMORY;
                goto error;
            }
            IMG_MEMSET(psVxdCtx, 0, sizeof(*psVxdCtx));

            switch (nCore)
            {
            case VXD_CORE0:
                psVxdCtx->ui32CoreNum = 0;
                break;
            case VXD_CORE1:
                psVxdCtx->ui32CoreNum = 1;
                break;
            default:
                break;
            }

            psCoreContext->pvData = psVxdCtx;
        }

        break;

    default:
        ui32Result = IMG_ERROR_INVALID_PARAMETERS;
        goto error;
        break;
    }

    psCoreContext->ui32RefCount++;

    *pui32SecureId = psCoreContext->ui32Id;

    return IMG_SUCCESS;

error:
    return ui32Result;
}


/*!
******************************************************************************

 @Function              SECURE_ReleaseId

******************************************************************************/
IMG_RESULT SECURE_ReleaseId(
    IMG_UINT32 ui32SecureId
)
{
    SECURE_sCoreContext   * psCoreContext;
    IMG_UINT32              ui32Result;

    ui32Result = IDGEN_GetHandle(ghSecureIdGen, ui32SecureId, (IMG_VOID**)&psCoreContext);
    if (ui32Result != IMG_SUCCESS)
    {
        return IMG_ERROR_INVALID_PARAMETERS;
    }
	
    if (psCoreContext == IMG_NULL)
    {
        return IMG_ERROR_INVALID_PARAMETERS;
    }

    if (psCoreContext->ui32RefCount > 0)
    {
        psCoreContext->ui32RefCount--;
    }

    if (psCoreContext->ui32RefCount == 0)
    {
        switch (psCoreContext->nCore)
        {
        case BITSTREAM_SCAN:
            break;

        case VXD_CORE0:
        case VXD_CORE1:
            if (psCoreContext->pvData)
            {
                IMG_FREE(psCoreContext->pvData);
                psCoreContext->pvData = IMG_NULL;
            }
            break;

        default:
            IMG_ASSERT(IMG_FALSE);
			return IMG_ERROR_INVALID_PARAMETERS;
            break;
        }
    }

    return IMG_SUCCESS;
}


/*!
******************************************************************************

 @Function              SECURE_PrepareFirmware

******************************************************************************/
static IMG_RESULT SECURE_PrepareFirmware(
    SECURE_sCoreContext   * psCoreContext,
    IMG_BYTE              * pbFwBin,
    IMG_UINT32              ui32FwBinSizeBytes,
    VXDIO_sDdBufInfo        sSecFwBufInfo
)
{
    IMG_UINT32 ui32Result;
    SECURE_sVXDContext * psVxdCtx;

    if (psCoreContext == IMG_NULL)
    {
        return IMG_ERROR_INVALID_PARAMETERS;
    }

    psVxdCtx = (SECURE_sVXDContext *)psCoreContext->pvData;

    if (psVxdCtx == IMG_NULL)
    {
        return IMG_ERROR_UNEXPECTED_STATE;
    }

    if (pbFwBin == IMG_NULL || ui32FwBinSizeBytes == 0)
    {
        REPORT(REPORT_MODULE_VXDIO, REPORT_ERR,
               "Invalid firmware binary buffer");
        return IMG_ERROR_INVALID_PARAMETERS;

    }

    if (ui32FwBinSizeBytes > sSecFwBufInfo.ui32BufSize)
    {
        REPORT(REPORT_MODULE_VXDIO, REPORT_ERR,
               "Size of firmware binary exceeds provided buffer size");
        return IMG_ERROR_INVALID_PARAMETERS;
    }

    // Map device buffer into Secure CPU.
    DEBUG_REPORT(REPORT_MODULE_VXDIO,
                 "buffer trace %s:%d SECMEM_MapSecureMemory cpu phys 0x%llx size %d",
                 __FUNCTION__, __LINE__,
                 (unsigned long long)sSecFwBufInfo.paSecPhysAddr,
                 sSecFwBufInfo.ui32BufSize);
    ui32Result = SECMEM_MapSecureMemory(sSecFwBufInfo.paSecPhysAddr,
                                        sSecFwBufInfo.ui32BufSize,
                                        &sSecFwBufInfo.pvCpuVirt);
    if (ui32Result != IMG_SUCCESS)
    {
        REPORT(REPORT_MODULE_VXDIO, REPORT_ERR,
               "Failed to map secure firmware device buffer");
        return ui32Result;
    }

    ui32Result = MSVDXIO_SEC_PrepareFirmware(psVxdCtx->hVxdIo,
                                             pbFwBin,
                                             ui32FwBinSizeBytes,
                                             &sSecFwBufInfo);
    if (ui32Result != IMG_SUCCESS)
    {
        REPORT(REPORT_MODULE_VXDIO, REPORT_ERR,
               "Failed to prepare firmware");
        return ui32Result;
    }

    return IMG_SUCCESS;
}

/*!
******************************************************************************

 @Function              SECURE_LoadCoreFW

******************************************************************************/
static IMG_RESULT SECURE_LoadCoreFW(
    SECURE_sCoreContext * psCoreContext,
    IMG_UINT32            ui32MmuCtrl2,
    IMG_UINT32            ui32PtdPhysAddr
)
{
    IMG_UINT32 ui32Result;
    SECURE_sVXDContext * psVxdCtx;

    if (psCoreContext == IMG_NULL)
    {
        return IMG_ERROR_INVALID_PARAMETERS;
    }

    psVxdCtx = (SECURE_sVXDContext *)psCoreContext->pvData;

    if (psVxdCtx == IMG_NULL)
    {
        return IMG_ERROR_UNEXPECTED_STATE;
    }

    ui32Result = MSVDXIO_SEC_LoadBaseFirmware(psVxdCtx->hVxdIo,
                                              ui32MmuCtrl2,
                                              ui32PtdPhysAddr);
    if (ui32Result != IMG_SUCCESS)
    {
        REPORT(REPORT_MODULE_VXDIO, REPORT_ERR,
               "Failed to load firmware!");
        return ui32Result;
    }

    return IMG_SUCCESS;
}


static IMG_BOOL bFirst = IMG_TRUE;


/*!
******************************************************************************

 @Function              SECURE_SendMessage

******************************************************************************/
IMG_RESULT SECURE_SendMessage(
    IMG_UINT32 ui32SecureId,
    IMG_BYTE *pbyMsg,
    IMG_UINT16 ui16Size,
    MSG_ENDPOINT nEndpoint,
    IMG_BYTE *pbyBuf,
    IMG_UINT32 ui32BufSize
)
{
    SECURE_sCoreContext   * psCoreContext;
    IMG_RESULT              ui32Result;

    if (ui32SecureId == 0 ||
        pbyMsg == IMG_NULL)
    {
        return IMG_ERROR_INVALID_PARAMETERS;
    }

    ui32Result = IDGEN_GetHandle(ghSecureIdGen, ui32SecureId, (IMG_VOID**)&psCoreContext);
    if (ui32Result != IMG_SUCCESS)
    {
        return IMG_ERROR_INVALID_PARAMETERS;
    }

    switch (nEndpoint)
    {
    case ENDPOINT_BSPP_STREAM_CREATE: //[TODO] - Improve error handling in this section. If you get asserts exit the function cleanly!
        {
            BSPP_SECURE_sStreamCreateArgs * psArgs;
            IMG_HANDLE                      hStrContext;

            IMG_ASSERT(ui16Size == sizeof(BSPP_SECURE_sStreamCreateArgs));
			if(ui16Size != sizeof(BSPP_SECURE_sStreamCreateArgs))
			{
				ui32Result = IMG_ERROR_INVALID_PARAMETERS;
				goto error;
			}

            psArgs = (BSPP_SECURE_sStreamCreateArgs *)pbyMsg;

            if (bFirst)
            {
                ui32Result = IDGEN_CreateContext(1024, 128, IMG_FALSE, &ghBsppIdGen);
                if (ui32Result != IMG_SUCCESS)
                {
                    goto error;
                }

                bFirst = IMG_FALSE;
            }

            // [TODO] - Import error handling here. If you get asserts exit the function cleanly
            // Map the sequence device buffers into Secure CPU.
            // All elements contain the same buffer, so map only the first element
            DEBUG_REPORT(REPORT_MODULE_BSPP,
                         "buffer trace %s:%d SECMEM_MapSecureMemory cpu phys 0x%llx size %d",
                         __FUNCTION__, __LINE__,
                         (unsigned long long)psArgs->asFWSequence[0].sDdBufInfo.paSecPhysAddr,
                         psArgs->asFWSequence[0].sDdBufInfo.ui32BufSize);
            IMG_ASSERT(psArgs->asFWSequence[0].sDdBufInfo.eMemAttrib & SYS_MEMATTRIB_SECURE);
            ui32Result = SECMEM_MapSecureMemory(psArgs->asFWSequence[0].sDdBufInfo.paSecPhysAddr,
                                                psArgs->asFWSequence[0].sDdBufInfo.ui32BufSize,
                                                &psArgs->asFWSequence[0].sDdBufInfo.pvCpuVirt);

            IMG_ASSERT(ui32Result == IMG_SUCCESS);
            IMG_ASSERT(psArgs->asFWSequence[0].sDdBufInfo.pvCpuVirt);

            if( psArgs->sStrConfigData.eVidStd == VDEC_STD_H264 )
            {
                IMG_ASSERT(IMG_NULL != psArgs->asFWPPS);
                if(psArgs->asFWPPS)
                {
                    // [TODO] - Import error handling here. If you get asserts exit the function cleanly
                    // Map the sequence device buffers into Secure CPU.
                    // All elements contain the same buffer, so map only the first element
                    DEBUG_REPORT(REPORT_MODULE_BSPP,
                                 "buffer trace %s:%d SECMEM_MapSecureMemory cpu phys 0x%llx size %d",
                                 __FUNCTION__, __LINE__,
                                 (unsigned long long)psArgs->asFWPPS[0].sDdBufInfo.paSecPhysAddr,
                                 psArgs->asFWPPS[0].sDdBufInfo.ui32BufSize);
                    IMG_ASSERT(psArgs->asFWPPS[0].sDdBufInfo.eMemAttrib & SYS_MEMATTRIB_SECURE);
                    ui32Result = SECMEM_MapSecureMemory(psArgs->asFWPPS[0].sDdBufInfo.paSecPhysAddr,
                                                        psArgs->asFWPPS[0].sDdBufInfo.ui32BufSize,
                                                        &psArgs->asFWPPS[0].sDdBufInfo.pvCpuVirt);
                    IMG_ASSERT(ui32Result == IMG_SUCCESS);
                    IMG_ASSERT(psArgs->asFWPPS[0].sDdBufInfo.pvCpuVirt);
                }
            }

            ui32Result = BSPP_StreamCreate(&psArgs->sStrConfigData,
                                           &hStrContext,
                                           psArgs->asFWSequence,
                                           psArgs->asFWPPS);
            IMG_ASSERT(ui32Result == IMG_SUCCESS);
            IMG_ASSERT(hStrContext);

            ui32Result = IDGEN_AllocId(ghBsppIdGen, hStrContext, &psArgs->ui32StrId);
            if (ui32Result != IMG_SUCCESS)
            {
                goto error;
            }

            IMG_ASSERT(psArgs->ui32StrId != 0);
        }
        break;

	case ENDPOINT_BSPP_STREAM_DESTROY:
        {
            BSPP_SECURE_sStreamDestroyArgs    * psArgs;
            IMG_HANDLE                          hStrContext;

            IMG_ASSERT(ui16Size == sizeof(BSPP_SECURE_sStreamDestroyArgs));
			if(ui16Size != sizeof(BSPP_SECURE_sStreamDestroyArgs))
			{
				ui32Result = IMG_ERROR_INVALID_PARAMETERS;
				goto error;
			}

            psArgs = (BSPP_SECURE_sStreamDestroyArgs *)pbyMsg;

            ui32Result = IDGEN_GetHandle(ghBsppIdGen, psArgs->ui32StrId, &hStrContext);
            if (ui32Result != IMG_SUCCESS)
            {
                goto error;
            }

            ui32Result = IDGEN_FreeId(ghBsppIdGen, psArgs->ui32StrId);
            if (ui32Result != IMG_SUCCESS)
            {
                goto error;
            }

            ui32Result = BSPP_StreamDestroy(hStrContext);
            IMG_ASSERT(ui32Result == IMG_SUCCESS);
        }
        break;

    case ENDPOINT_BSPP_SUBMIT_PICTURE_DECODED:
        {
            BSPP_SECURE_sSubmitPictureDecodedArgs * psArgs;
            IMG_HANDLE                              hStrContext;

            IMG_ASSERT(ui16Size == sizeof(BSPP_SECURE_sSubmitPictureDecodedArgs));
			if(ui16Size != sizeof(BSPP_SECURE_sSubmitPictureDecodedArgs))
			{
				ui32Result = IMG_ERROR_INVALID_PARAMETERS;
				goto error;
			}

            psArgs = (BSPP_SECURE_sSubmitPictureDecodedArgs *)pbyMsg;

            ui32Result = IDGEN_GetHandle(ghBsppIdGen, psArgs->ui32StrId, &hStrContext);
            if (ui32Result != IMG_SUCCESS)
            {
                goto error;
            }

            ui32Result = BSPP_SubmitPictureDecoded(hStrContext,
                                                   psArgs->ui32SequHdrId,
                                                   psArgs->ui32PPSId,
                                                   psArgs->ui32SecondPPSId);
            IMG_ASSERT(ui32Result == IMG_SUCCESS);
        }
        break;

    case ENDPOINT_BSPP_STREAM_SUBMIT_BUFFER:
        {
            BSPP_SECURE_sStreamSubmitBufferArgs   * psArgs;
            IMG_HANDLE                              hStrContext;

            IMG_ASSERT(ui16Size == sizeof(BSPP_SECURE_sStreamSubmitBufferArgs));
			if(ui16Size != sizeof(BSPP_SECURE_sStreamSubmitBufferArgs))
			{
				ui32Result = IMG_ERROR_INVALID_PARAMETERS;
				goto error;
			}

            psArgs = (BSPP_SECURE_sStreamSubmitBufferArgs *)pbyMsg;

            ui32Result = IDGEN_GetHandle(ghBsppIdGen, psArgs->ui32StrId, &hStrContext);
            if (ui32Result != IMG_SUCCESS)
            {
                goto error;
            }

            // Map the bitstream buffer into Secure CPU.
            IMG_ASSERT(psArgs->sDdBufInfo.eMemAttrib & SYS_MEMATTRIB_SECURE);
            IMG_ASSERT(psArgs->sDdBufInfo.pvCpuVirt == IMG_NULL);
            DEBUG_REPORT(REPORT_MODULE_BSPP,
                         "buffer trace %s:%d SECMEM_MapSecureMemory cpu phys 0x%llx size %d",
                         __FUNCTION__, __LINE__,
                         (unsigned long long)psArgs->sDdBufInfo.paSecPhysAddr, psArgs->sDdBufInfo.ui32BufSize);
            ui32Result = SECMEM_MapSecureMemory(psArgs->sDdBufInfo.paSecPhysAddr,
                                                psArgs->sDdBufInfo.ui32BufSize,
                                                &psArgs->sDdBufInfo.pvCpuVirt);
            IMG_ASSERT(ui32Result == IMG_SUCCESS);
            IMG_ASSERT(psArgs->sDdBufInfo.pvCpuVirt);

            ui32Result = BSPP_StreamSubmitBuffer(hStrContext,
                                                 &psArgs->sDdBufInfo,
                                                 psArgs->ui32BufMapId,
                                                 psArgs->ui32DataSize,
                                                 psArgs->ui64PictTagParam,
                                                 psArgs->eBstrElementType);
            IMG_ASSERT(ui32Result == IMG_SUCCESS);
        }
        break;

    case ENDPOINT_BSPP_STREAM_PREPARSE_BUFFERS:
        {
            BSPP_SECURE_sStreamPreParseBuffersArgs    * psArgs;
            IMG_HANDLE                                  hStrContext;
			LST_T										sSegments;
			BSPP_sBitStrSeg						      * psSegment = IMG_NULL;
			IMG_UINT32									i;
			


			IMG_ASSERT(ui16Size == sizeof(BSPP_SECURE_sStreamPreParseBuffersArgs));
			if(ui16Size != sizeof(BSPP_SECURE_sStreamPreParseBuffersArgs))
			{
				ui32Result = IMG_ERROR_INVALID_PARAMETERS;
				goto error;
			}

            psArgs = (BSPP_SECURE_sStreamPreParseBuffersArgs *)pbyMsg;

            ui32Result = IDGEN_GetHandle(ghBsppIdGen, psArgs->ui32StrId, &hStrContext);
            if (ui32Result != IMG_SUCCESS)
            {
                goto error;
            }

            if (psArgs->ui32ContiguousBufMapId && psArgs->sContiguousBufInfo.ui32BufSize)
            {
                IMG_ASSERT(psArgs->sContiguousBufInfo.eMemAttrib & SYS_MEMATTRIB_SECURE);
                DEBUG_REPORT(REPORT_MODULE_BSPP,
                             "buffer trace %s:%d SECMEM_MapSecureMemory cpu phys 0x%llx size %d",
                             __FUNCTION__, __LINE__,
                             (unsigned long long)psArgs->sContiguousBufInfo.paSecPhysAddr,
                             psArgs->sContiguousBufInfo.ui32BufSize);
                ui32Result = SECMEM_MapSecureMemory(psArgs->sContiguousBufInfo.paSecPhysAddr,
                                                    psArgs->sContiguousBufInfo.ui32BufSize,
                                                    &psArgs->sContiguousBufInfo.pvCpuVirt);
                IMG_ASSERT(ui32Result == IMG_SUCCESS);
                IMG_ASSERT(psArgs->sContiguousBufInfo.pvCpuVirt);
            }
			
			LST_init(&sSegments);
			for (i = 0; i < psArgs->ui32SegmentCount; i++)
			{
				psSegment = IMG_MALLOC(sizeof(BSPP_sBitStrSeg));
				IMG_ASSERT(psSegment != IMG_NULL);
				if (psSegment == IMG_NULL)
				{
					ui32Result = IMG_ERROR_MALLOC_FAILED;
					goto error_pre_parse_buffer;
				}

				IMG_MEMSET(psSegment, 0, sizeof(BSPP_sBitStrSeg));
				LST_add(&sSegments, psSegment);
			}


			ui32Result = BSPP_StreamPreParseBuffers(hStrContext,
                                                    &psArgs->sContiguousBufInfo,
                                                    psArgs->ui32ContiguousBufMapId,
													&sSegments,
                                                    &psArgs->sPreParsedData);



            IMG_ASSERT((ui32Result == IMG_SUCCESS) || (ui32Result == IMG_ERROR_NOT_SUPPORTED));

error_pre_parse_buffer:
			// Empty the segment list and free.
			psSegment = LST_removeHead(&sSegments);
			while (psSegment)
			{
				IMG_FREE(psSegment);
				psSegment = LST_removeHead(&sSegments);
			}

        }
        break;

    case ENDPOINT_VXD_INITIALISE:
        {
            SECURE_sVXDContext    * psVxdCtx = (SECURE_sVXDContext *)psCoreContext->pvData;
            VXD_sInitialiseArgs   * psArgs;
            IMG_BOOL                bPostTest = IMG_FALSE;
            IMG_BOOL                bStackUsageTest = IMG_FALSE;

            psArgs = (VXD_sInitialiseArgs *)pbyMsg;
#ifdef POST_TEST
            bPostTest = psArgs->bPost;
#endif /* POST_TEST */
#ifdef STACK_USAGE_TEST
            bStackUsageTest = psArgs->bStackUsageTest;
#endif /* STACK_USAGE_TEST */

            IMG_ASSERT(ui16Size == sizeof(VXD_sInitialiseArgs));
			if(ui16Size != sizeof(VXD_sInitialiseArgs))
			{
				ui32Result = IMG_ERROR_INVALID_PARAMETERS;
				goto error;
			}

            if(IMG_NULL != psVxdCtx)
            {
                ui32Result = MSVDXIO_SEC_Initialise(bPostTest,
                                                    bStackUsageTest,
                                                    &psVxdCtx->hVxdIo);
            }
            if (ui32Result == IMG_SUCCESS)
            {
                MSVDXIO_sCoreRegs sCoreRegs = { 0 };

                ui32Result = MSVDXIO_SEC_GetCoreProps(psVxdCtx->hVxdIo,
                                                      &sCoreRegs);

                psArgs->ui32CoreRev = sCoreRegs.ui32CoreRev;
                psArgs->ui32Internal = sCoreRegs.ui32Internal;
                psArgs->ui32Latency = sCoreRegs.ui32Latency;
                psArgs->ui32MmuStatus = sCoreRegs.ui32MmuStatus;
                psArgs->ui32CoreId = sCoreRegs.ui32CoreId;
                psArgs->ui32MultiCore = sCoreRegs.ui32MultiCore;
            }
        }
        break;

    case ENDPOINT_VXD_DEINITIALISE:
        {
            SECURE_sVXDContext      * psVxdCtx = (SECURE_sVXDContext *)psCoreContext->pvData;

            IMG_ASSERT(ui16Size == sizeof(VXD_sDeInitialiseArgs));
			if(ui16Size != sizeof(VXD_sDeInitialiseArgs))
			{
				ui32Result = IMG_ERROR_INVALID_PARAMETERS;
				goto error;
			}

            if(IMG_NULL != psVxdCtx)
            {
                ui32Result = MSVDXIO_SEC_DeInitialise(psVxdCtx->hVxdIo);
            }
            IMG_ASSERT(ui32Result == IMG_SUCCESS);
        }
        break;

    case ENDPOINT_VXD_RESET:
        {
            SECURE_sVXDContext    * psVxdCtx = (SECURE_sVXDContext *)psCoreContext->pvData;
            VXD_sResetArgs        * psArgs;

            IMG_ASSERT(ui16Size == sizeof(VXD_sResetArgs));
			if(ui16Size != sizeof(VXD_sResetArgs))
			{
				ui32Result = IMG_ERROR_INVALID_PARAMETERS;
				goto error;
			}

            psArgs = (VXD_sResetArgs *)pbyMsg;
            if(IMG_NULL != psVxdCtx)
            {
                ui32Result = MSVDXIO_SEC_ResetCore(psVxdCtx->hVxdIo,
                                                   psArgs->bAutoClockGating,
                                                   psArgs->bExtClockGating,
                                                   psArgs->bClocksEnable);
            }
            IMG_ASSERT(ui32Result == IMG_SUCCESS);
        }
        break;

    case ENDPOINT_VXD_INPUT:
        {
            SECURE_sVXDContext    * psVxdCtx = (SECURE_sVXDContext *)psCoreContext->pvData;
            VXD_sSendFwMsgArgs    * psArgs;

            IMG_ASSERT(ui16Size == sizeof(VXD_sSendFwMsgArgs));
            if(ui16Size != sizeof(VXD_sSendFwMsgArgs))
            {
                ui32Result = IMG_ERROR_INVALID_PARAMETERS;
                goto error;
            }

            if (pbyBuf == IMG_NULL || ui32BufSize == 0)
            {
                ui32Result = IMG_ERROR_INVALID_PARAMETERS;
                goto error;
            }

            psArgs = (VXD_sSendFwMsgArgs *)pbyMsg;

            // SEC_CopyFromKmToSecure()

            if(IMG_NULL != psVxdCtx)
            {
                if (psArgs->bFlushMmu)
                {
                    ui32Result = MSVDXIO_SEC_FlushMmu(psVxdCtx->hVxdIo);
                    if (ui32Result != IMG_SUCCESS)
                    {
                        goto error;
                    }
                }
                ui32Result = MSVDXIO_SEC_SendFirmwareMessage(psVxdCtx->hVxdIo,
                                                             psArgs->eArea,
                                                             pbyBuf,
                                                             ui32BufSize);
            }
        }
        break;

    case ENDPOINT_VXD_HANDLE_INTERRUPTS:
        {
            SECURE_sVXDContext        * psVxdCtx;
            VXD_sHandleInterruptsArgs * psArgs;
            VXD_sIntStatus              sIntStatus = { 0 };

            psVxdCtx = (SECURE_sVXDContext *)psCoreContext->pvData;

            if (ui16Size != sizeof(VXD_sHandleInterruptsArgs))
            {
                ui32Result = IMG_ERROR_INVALID_PARAMETERS;
                goto error;
            }

            psArgs = (VXD_sHandleInterruptsArgs *)pbyMsg;

            if ((pbyBuf == IMG_NULL) || (ui32BufSize == 0) ||
                (ui32BufSize != psArgs->ui32MsgsSizeWrds*sizeof(IMG_UINT32)))
            {
                ui32Result = IMG_ERROR_INVALID_PARAMETERS;
                goto error;
            }

            if (IMG_NULL != psVxdCtx)
            {
                ui32Result = MSVDXIO_SEC_HandleInterrupts(psVxdCtx->hVxdIo,
                                                          &sIntStatus,
                                                          (IMG_UINT32 *)pbyBuf,
                                                          &psArgs->ui32MsgsSizeWrds,
                                                          &psArgs->bMoreData);
                IMG_ASSERT(ui32Result == IMG_SUCCESS);
            }

            psArgs->ui32Pending = sIntStatus.ui32Pending;
            psArgs->ui32Requestor = sIntStatus.ui32Requestor;
            psArgs->MMU_FAULT_ADDR = sIntStatus.MMU_FAULT_ADDR;
            psArgs->MMU_FAULT_RNW = sIntStatus.MMU_FAULT_RNW;
            psArgs->MMU_PF_N_RW = sIntStatus.MMU_PF_N_RW;

            // SEC_CopyFromSecureToKm()
        }
        break;

    case ENDPOINT_VXD_GET_STATE:
        {
            SECURE_sVXDContext    * psVxdCtx = (SECURE_sVXDContext *)psCoreContext->pvData;
            VXD_sGetStateArgs     * psArgs;

            IMG_ASSERT(ui16Size == sizeof(VXD_sGetStateArgs));
			if(ui16Size != sizeof(VXD_sGetStateArgs))
			{
				ui32Result = IMG_ERROR_INVALID_PARAMETERS;
				goto error;
			}
			
            psArgs = (VXD_sGetStateArgs *)pbyMsg;
            if(IMG_NULL != psVxdCtx)
            {
                ui32Result = MSVDXIO_SEC_GetCoreState(psVxdCtx->hVxdIo,
                                                      &psArgs->sState);
            }

            // SEC_CopyFromSecureToKm()
        }
        break;

    case ENDPOINT_VXD_PREPARE_FIRMWARE:
        {
            VXD_SECURE_sPrepareFirmwareArgs * psArgs;
            VXDIO_sDdBufInfo sSecFwBufInfo = { 0 };

            if(ui16Size != sizeof(VXD_SECURE_sPrepareFirmwareArgs))
            {
                ui32Result = IMG_ERROR_INVALID_PARAMETERS;
                goto error;
            }

            if (pbyBuf == IMG_NULL || ui32BufSize == 0)
            {
                ui32Result = IMG_ERROR_INVALID_PARAMETERS;
                goto error;
            }

            psArgs = (VXD_SECURE_sPrepareFirmwareArgs *)pbyMsg;

            sSecFwBufInfo.paSecPhysAddr = psArgs->ui32FwBufCpuPhys;
            sSecFwBufInfo.ui32BufSize = psArgs->ui32FwBufSizeBytes;
            sSecFwBufInfo.eMemAttrib = (SYS_eMemAttrib)psArgs->ui32FwBufMemAttrib;
            sSecFwBufInfo.ui32DevVirt = psArgs->ui32DevVirtAddr;

            ui32Result = SECURE_PrepareFirmware(psCoreContext,
                                                pbyBuf,
                                                ui32BufSize,
                                                sSecFwBufInfo);
        }
        break;

    case ENDPOINT_VXD_LOAD_CORE_FW:
        {
            VXD_SECURE_LoadCoreFW *psArgs;

            if (ui16Size != sizeof(VXD_SECURE_LoadCoreFW))
            {
                ui32Result = IMG_ERROR_INVALID_PARAMETERS;
                goto error;
            }

            psArgs = (VXD_SECURE_LoadCoreFW*)pbyMsg;
            ui32Result = SECURE_LoadCoreFW(psCoreContext,
                                           psArgs->ui32MmuCtrl2,
                                           psArgs->ui32PtdPhysAddr);
        }
        break;

    case ENDPOINT_VXD_READN_REGS:
        {
            VXD_SECURE_sReadNRegsArgs * psArgs;
            IMG_UINT32                * pui32Dst = IMG_NULL, ui32DstBufSize = 0;

            if (ui16Size != sizeof(VXD_SECURE_sReadNRegsArgs))
            {
                ui32Result = IMG_ERROR_INVALID_PARAMETERS;
                goto error;
            }

            psArgs = (VXD_SECURE_sReadNRegsArgs *)pbyMsg;

            if (psArgs->ui32NumRegs == 1 && pbyBuf == IMG_NULL)
            {
                pui32Dst = &psArgs->ui32RegVal;
                ui32DstBufSize = sizeof(IMG_UINT32);
            }
            else
            {
                pui32Dst = (IMG_UINT32 *)pbyBuf;
                ui32DstBufSize = ui32BufSize;
            }

            ui32Result = SECURE_ReadNRegs(ui32SecureId,
                                          psArgs->ui32NumRegs,
                                          psArgs->ui32Region,
                                          psArgs->ui32Offset,
                                          pui32Dst,
                                          ui32DstBufSize);
        }
        break;

    default:
        return IMG_ERROR_INVALID_PARAMETERS;
    }

error:
    return ui32Result;
}


/*!
******************************************************************************

 @Function              SECURE_ReadNRegs

******************************************************************************/
IMG_RESULT SECURE_ReadNRegs(
    IMG_UINT32 ui32SecureId,
    IMG_UINT32 ui32NumRegs,
    IMG_UINT32 ui32Region,
    IMG_UINT32 ui32Offset,
    IMG_UINT32 * pui32DstBuf,
    IMG_UINT32 ui32DstBufSize
)
{
    SECURE_sCoreContext   * psCoreContext;
    SECURE_sVXDContext    * psVxdCtx;
    IMG_RESULT              ui32Result = IMG_SUCCESS;

    if (ui32SecureId == 0 || ui32NumRegs == 0)
    {
        REPORT(REPORT_MODULE_VXDIO, REPORT_ERR,
               "Wrong id (%u) or number of registers (%u)",
               ui32SecureId, ui32NumRegs);
        return IMG_ERROR_INVALID_PARAMETERS;
    }

    if (pui32DstBuf == IMG_NULL || ui32DstBufSize == 0)
    {
        REPORT(REPORT_MODULE_VXDIO, REPORT_ERR,
               "Got NULL dst buffer: %p, %u",
               pui32DstBuf, ui32DstBufSize);
        return IMG_ERROR_INVALID_PARAMETERS;
    }

    if ((ui32NumRegs*sizeof(IMG_UINT32) > ui32DstBufSize))
    {
        REPORT(REPORT_MODULE_VXDIO, REPORT_ERR,
               "Requested %u registers to be read, but dst buffer is too small: %u",
               ui32NumRegs, ui32DstBufSize);
        return IMG_ERROR_INVALID_PARAMETERS;
    }

    ui32Result = IDGEN_GetHandle(ghSecureIdGen, ui32SecureId,
                                 (IMG_VOID**)&psCoreContext);
    if (ui32Result != IMG_SUCCESS)
    {
        REPORT(REPORT_MODULE_VXDIO, REPORT_ERR,
               "Failed to obtain secure handle (%u) for id %u",
               ui32Result, ui32SecureId);
        return ui32Result;
    }

    psVxdCtx = (SECURE_sVXDContext *)psCoreContext->pvData;

    if (IMG_NULL != psVxdCtx)
    {
        while (ui32NumRegs--)
        {
            *pui32DstBuf++ = MSVDXIO_SEC_ReadRegister(psVxdCtx->hVxdIo,
                                                      ui32Region,
                                                      ui32Offset);
            ui32Offset += sizeof(IMG_UINT32);
        }
    }
    else
    {
        /* If this is called from msvdx_CheckInterruptFunc because
         * cb TALITR_TYP_INTERRUPT is received after the msvdxio stream destroy
         * then pvData will be null (being freed from SECURE_ReleaseHandle) */
        IMG_ASSERT(IMG_FALSE);
    }

    return IMG_SUCCESS;
}

