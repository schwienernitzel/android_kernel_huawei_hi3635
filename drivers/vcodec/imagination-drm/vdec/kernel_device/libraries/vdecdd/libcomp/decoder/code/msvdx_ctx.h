/*!
 *****************************************************************************
 *
 * @File       msvdx_ctx.h
 * @Title      MSVDX internal core context
 * @Description    This file contains the MSVDX internal core context declaration.
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

#if !defined (__MSVDXCTX_H__)
#define __MSVDXCTX_H__

#if defined(__cplusplus)
extern "C" {
#endif

#include "img_defs.h"
#include "vdecdd_int.h"
#include "vxd_ext.h"
#include "vxd.h"
#include "hwctrl_int.h"

#include "secureapi_ree.h"
#include "secure_defs.h"
#include "secure_msg.h"

#define VXD_SEC_MTX_MSGS_BUF_SIZE (MSVDX_SIZE_MSG_BUFFER)

/*!
******************************************************************************
 This structure is used to retain MSVDXIO specific data for a particular core
******************************************************************************/
typedef struct
{
    IMG_BOOL                bInitialised;           /*!<                                            */
    IMG_UINT32              ui32SecureId;           /*!< Secure device ID.                          */
    IMG_VIDEO_CORE          eSecureCore;            /*!<                                            */
    VXD_sSendFwMsgArgs      sFwMsgArgs;             /*!<                                            */
    IMG_BOOL                bSecureStream;          /*!< The device is running in secure mode       */

    IMG_HANDLE              hMsvdxIo;               /* Handle to MSVDXIO Context for non-secure no
                                                       bridging systems                             */

#ifndef SYSBRG_BRIDGING
    IMG_HANDLE              hSysRegSpace;           /*!< This is only for non-secure no bridging
                                                         systems where interrupts are polled.       */
    
#endif
    VXD_sCoreProps          sProps;                 /*!< MSVDX core properties.                     */
    HWCTRL_sRendec          sRendec;                /*!<                                            */

    IMG_UINT32              ui32CoreNum;            /*!<                                            */

    IMG_BOOL                bFakeMtx;               /*!<                                            */
    IMG_BOOL                bCSim;                  /*!<                                            */

    VDECDD_eDecodeLevel     eDecodeLevel;           /*!< Level in device to which decoding should take place.       */

    VXDIO_sPtdInfo          sPtdInfo;               /*!< Pointer to memory structure which holds
                                                         Device Page Table Directory address.       */
#ifdef POST_TEST
    IMG_BOOL                bPost;                  /*!< if 1 execute POST                          */
#endif
#ifdef STACK_USAGE_TEST
    IMG_BOOL                bStackUsageTest;        /*!< if 1 execute Stack Usage Test              */
#endif

    IMG_UINT32              aui32MtxMsgs[VXD_SEC_MTX_MSGS_BUF_SIZE];
                                                    /*!< Array used to store messages received
                                                         from the TEE                               */
    IMG_UINT32              ui32MsgsSizeWrds;       /*!< Number of words stored in the aui32MtxMsgs
                                                         array                                      */
    IMG_UINT32              ui32MtxMsgsOffset;      /*!< Number of words already processed from the
                                                         aui32MtxMsgs array                         */
    IMG_BOOL                bFlushMmu;              /*!< Whether MMU cache flush should be executed */

} MSVDX_sContext;

#if defined(__cplusplus)
}
#endif

#endif /* __MSVDXCTX_H__ */



