/*!
 *****************************************************************************
 *
 * @File       bspp_km_server.c
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

#include "sysbrg_api.h"
#include "sysbrg_api_km.h"
#include "sysos_api_km.h"
#include "bspp_km.h"
#include "bspp_km_rpc.h"


IMG_VOID BSPP_KM_dispatch(SYSBRG_sPacket *psPacket)
{
	BSPP_KM_sCmdMsg sCommandMsg;
	BSPP_KM_sRespMsg sResponseMsg;

	if(SYSOSKM_CopyFromUser(&sCommandMsg, psPacket->pvCmdData, sizeof(sCommandMsg)))
		IMG_ASSERT(!"failed to copy from user");

	switch (sCommandMsg.eFuncId)
	{
	
      case BSPP_SecureStreamCreate_ID:
      
#if 0
#ifdef CONFIG_COMPAT      
      printk("bridge %d %s %s\n", is_compat_task(), __FUNCTION__, "BSPP_SecureStreamCreate");
#else
      printk("bridge %s %s\n", __FUNCTION__, "BSPP_SecureStreamCreate");
#endif
#endif
      
	sResponseMsg.sResp.sBSPP_SecureStreamCreateResp.xBSPP_SecureStreamCreateResp =
      		BSPP_SecureStreamCreate(
      
	  sCommandMsg.sCmd.sBSPP_SecureStreamCreateCmd.psStrConfigData,
	  sCommandMsg.sCmd.sBSPP_SecureStreamCreateCmd.pui32StrId,
	  sCommandMsg.sCmd.sBSPP_SecureStreamCreateCmd.asFWSequence,
	  sCommandMsg.sCmd.sBSPP_SecureStreamCreateCmd.asFWPPS
      );
      break;
      
    
      case BSPP_SecureStreamDestroy_ID:
      
#if 0
#ifdef CONFIG_COMPAT      
      printk("bridge %d %s %s\n", is_compat_task(), __FUNCTION__, "BSPP_SecureStreamDestroy");
#else
      printk("bridge %s %s\n", __FUNCTION__, "BSPP_SecureStreamDestroy");
#endif
#endif
      
	sResponseMsg.sResp.sBSPP_SecureStreamDestroyResp.xBSPP_SecureStreamDestroyResp =
      		BSPP_SecureStreamDestroy(
      
	  sCommandMsg.sCmd.sBSPP_SecureStreamDestroyCmd.ui32StrId
      );
      break;
      
    
      case BSPP_SecureStreamSubmitBuffer_ID:
      
#if 0
#ifdef CONFIG_COMPAT      
      printk("bridge %d %s %s\n", is_compat_task(), __FUNCTION__, "BSPP_SecureStreamSubmitBuffer");
#else
      printk("bridge %s %s\n", __FUNCTION__, "BSPP_SecureStreamSubmitBuffer");
#endif
#endif
      
	sResponseMsg.sResp.sBSPP_SecureStreamSubmitBufferResp.xBSPP_SecureStreamSubmitBufferResp =
      		BSPP_SecureStreamSubmitBuffer(
      
	  sCommandMsg.sCmd.sBSPP_SecureStreamSubmitBufferCmd.ui32StrId,
	  sCommandMsg.sCmd.sBSPP_SecureStreamSubmitBufferCmd.ui32BufMapId,
	  sCommandMsg.sCmd.sBSPP_SecureStreamSubmitBufferCmd.ui32DataSize,
	  sCommandMsg.sCmd.sBSPP_SecureStreamSubmitBufferCmd.eBstrElementType,
	  sCommandMsg.sCmd.sBSPP_SecureStreamSubmitBufferCmd.pvPictTagParam
      );
      break;
      
    
      case BSPP_SecureSubmitPictureDecoded_ID:
      
#if 0
#ifdef CONFIG_COMPAT      
      printk("bridge %d %s %s\n", is_compat_task(), __FUNCTION__, "BSPP_SecureSubmitPictureDecoded");
#else
      printk("bridge %s %s\n", __FUNCTION__, "BSPP_SecureSubmitPictureDecoded");
#endif
#endif
      
	sResponseMsg.sResp.sBSPP_SecureSubmitPictureDecodedResp.xBSPP_SecureSubmitPictureDecodedResp =
      		BSPP_SecureSubmitPictureDecoded(
      
	  sCommandMsg.sCmd.sBSPP_SecureSubmitPictureDecodedCmd.ui32StrId,
	  sCommandMsg.sCmd.sBSPP_SecureSubmitPictureDecodedCmd.ui32SequHdrId,
	  sCommandMsg.sCmd.sBSPP_SecureSubmitPictureDecodedCmd.ui32PPSId,
	  sCommandMsg.sCmd.sBSPP_SecureSubmitPictureDecodedCmd.ui32SecondPPSId
      );
      break;
      
    
      case BSPP_SecureStreamPreParseBuffers_ID:
      
#if 0
#ifdef CONFIG_COMPAT      
      printk("bridge %d %s %s\n", is_compat_task(), __FUNCTION__, "BSPP_SecureStreamPreParseBuffers");
#else
      printk("bridge %s %s\n", __FUNCTION__, "BSPP_SecureStreamPreParseBuffers");
#endif
#endif
      
	sResponseMsg.sResp.sBSPP_SecureStreamPreParseBuffersResp.xBSPP_SecureStreamPreParseBuffersResp =
      		BSPP_SecureStreamPreParseBuffers(
      
	  sCommandMsg.sCmd.sBSPP_SecureStreamPreParseBuffersCmd.ui32StrId,
	  sCommandMsg.sCmd.sBSPP_SecureStreamPreParseBuffersCmd.ui32NumOfSegs,
	  sCommandMsg.sCmd.sBSPP_SecureStreamPreParseBuffersCmd.psPreParsedData,
	  sCommandMsg.sCmd.sBSPP_SecureStreamPreParseBuffersCmd.ui32ContiguousBufMapId
      );
      break;
      
    
	}
	if(SYSOSKM_CopyToUser(psPacket->pvRespData, &sResponseMsg, sizeof(sResponseMsg)))
		IMG_ASSERT(!"failed to copy to user");
}
