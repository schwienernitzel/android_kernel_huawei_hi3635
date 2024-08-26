/*!
 *****************************************************************************
 *
 * @File       bspp_km_rpc.h
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

#ifndef __BSPP_KM_RPC_H__
#define __BSPP_KM_RPC_H__

#include "img_defs.h"
#include "sysbrg_api.h"
#include "bspp_km.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
	BSPP_SecureStreamCreate_ID,
	BSPP_SecureStreamDestroy_ID,
	BSPP_SecureStreamSubmitBuffer_ID,
	BSPP_SecureSubmitPictureDecoded_ID,
	BSPP_SecureStreamPreParseBuffers_ID,

} BSPP_KM_eFuncId;

typedef struct
{
	BSPP_KM_eFuncId	eFuncId;
    union
	{
	
		struct
		{
			 sysbrg_user_pointer psStrConfigData;
                          		 sysbrg_user_pointer pui32StrId;
                          		 sysbrg_user_pointer asFWSequence;
                          		 sysbrg_user_pointer asFWPPS;
                          
		} sBSPP_SecureStreamCreateCmd;
	
		struct
		{
			 IMG_UINT32 ui32StrId;
                          
		} sBSPP_SecureStreamDestroyCmd;
	
		struct
		{
			 IMG_UINT32 ui32StrId;
                          		 IMG_UINT32 ui32BufMapId;
                          		 IMG_UINT32 ui32DataSize;
                          		 VDEC_eBstrElementType eBstrElementType;
                          		 sysbrg_user_pointer pvPictTagParam;
                          
		} sBSPP_SecureStreamSubmitBufferCmd;
	
		struct
		{
			 IMG_UINT32 ui32StrId;
                          		 IMG_UINT32 ui32SequHdrId;
                          		 IMG_UINT32 ui32PPSId;
                          		 IMG_UINT32 ui32SecondPPSId;
                          
		} sBSPP_SecureSubmitPictureDecodedCmd;
	
		struct
		{
			 IMG_UINT32 ui32StrId;
                          		 IMG_UINT32 ui32NumOfSegs;
                          		 sysbrg_user_pointer psPreParsedData;
                          		 IMG_UINT32 ui32ContiguousBufMapId;
                          
		} sBSPP_SecureStreamPreParseBuffersCmd;
	
	} sCmd;
} BSPP_KM_sCmdMsg;

typedef struct
{
    union
	{
	
		struct
		{
			IMG_RESULT		xBSPP_SecureStreamCreateResp;
		} sBSPP_SecureStreamCreateResp;
            
		struct
		{
			IMG_RESULT		xBSPP_SecureStreamDestroyResp;
		} sBSPP_SecureStreamDestroyResp;
            
		struct
		{
			IMG_RESULT		xBSPP_SecureStreamSubmitBufferResp;
		} sBSPP_SecureStreamSubmitBufferResp;
            
		struct
		{
			IMG_RESULT		xBSPP_SecureSubmitPictureDecodedResp;
		} sBSPP_SecureSubmitPictureDecodedResp;
            
		struct
		{
			IMG_RESULT		xBSPP_SecureStreamPreParseBuffersResp;
		} sBSPP_SecureStreamPreParseBuffersResp;
            
	} sResp;
} BSPP_KM_sRespMsg;



extern IMG_VOID BSPP_KM_dispatch(SYSBRG_sPacket __user *psPacket);

#ifdef __cplusplus
}
#endif

#endif
