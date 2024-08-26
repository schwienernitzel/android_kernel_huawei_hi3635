/*!
 *****************************************************************************
 *
 * @File       bspp_km.h
 * @Title      SecureMedia BSPP KM Interface
 * @Description    This file contains the SecureMedia KM interface to BSPP
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

#if !defined (__BSPP_KM_H__)
#define __BSPP_KM_H__

#if defined(__cplusplus)
extern "C" {
#endif

#include "img_include.h"
#include "bspp.h"
#include "secure_defs.h"

#include "secureapi.h"

//Wrapper structures for BSPP calls through SecureMedia api
typedef struct
{
	//Input
    VDEC_sStrConfigData   sStrConfigData;
	BSPP_sDdBufArrayInfo  asFWSequence[MAX_SEQUENCES_SECURE];
    BSPP_sDdBufArrayInfo  asFWPPS[MAX_PPSS_SECURE];
	
    //Output
    IMG_UINT32              ui32StrId;

} BSPP_SECURE_sStreamCreateArgs;

typedef struct
{
	//Input
    IMG_UINT32              ui32StrId;

} BSPP_SECURE_sStreamDestroyArgs;

typedef struct
{
	//Input
	IMG_UINT32              ui32StrId;
    IMG_UINT32              ui32SequHdrId;
    IMG_UINT32              ui32PPSId;
    IMG_UINT32              ui32SecondPPSId;

} BSPP_SECURE_sSubmitPictureDecodedArgs;

typedef struct
{
	//Input
	IMG_UINT32                  ui32StrId;
    BSPP_sDdBufInfo				sDdBufInfo;
	IMG_UINT32                  ui32BufMapId;
    IMG_UINT32                  ui32DataSize;
    VDEC_eBstrElementType       eBstrElementType;
    SYSBRG_UINT64					ui64PictTagParam;

} BSPP_SECURE_sStreamSubmitBufferArgs;

typedef struct
{
	//Input
	IMG_UINT32              ui32StrId;
	IMG_UINT32				ui32SegmentCount;
    BSPP_sDdBufInfo			sContiguousBufInfo;
    IMG_UINT32              ui32ContiguousBufMapId;
	//Output
    BSPP_sPreParsedData     sPreParsedData;

} BSPP_SECURE_sStreamPreParseBuffersArgs;

typedef struct
{
    /*
      Can't use SYSBRG_LST here. These pointers are actually used in kernel
      space (with copy from/to user) and have to be properly aligned on 32/64
    */
    SYSBRG_UPOINTER(BSPP_sBitStrSeg, first);
    SYSBRG_UPOINTER(BSPP_sBitStrSeg, last);
} BSPP_SECURE_sPreParseSegments;

/*!
******************************************************************************

 @Function              BSPP_SecureStreamCreate

******************************************************************************/
IMG_RESULT BSPP_SecureStreamCreate(
    SYSBRG_POINTER_ARG(VDEC_sStrConfigData)  psStrConfigData,
    SYSBRG_POINTER_ARG(IMG_UINT32)           pui32StrId,
    SYSBRG_POINTER_ARG(BSPP_sDdBufArrayInfo) asFWSequence,
    SYSBRG_POINTER_ARG(BSPP_sDdBufArrayInfo) asFWPPS
);

/*!
******************************************************************************

 @Function              BSPP_SecureStreamDestroy

******************************************************************************/
IMG_RESULT BSPP_SecureStreamDestroy(
    IMG_UINT32            ui32StrId
);
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
);
/*!
******************************************************************************

 @Function              BSPP_SecureSubmitPictureDecoded

 @Description

******************************************************************************/
IMG_RESULT
BSPP_SecureSubmitPictureDecoded(
    IMG_UINT32              ui32StrId,
    IMG_UINT32              ui32SequHdrId,
    IMG_UINT32              ui32PPSId,
    IMG_UINT32              ui32SecondPPSId
);
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
);

#if defined(__cplusplus)
}
#endif

#endif /* __BSPP_KM_H__ */



