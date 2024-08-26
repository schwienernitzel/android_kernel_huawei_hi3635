/*!
 *****************************************************************************
 *
 * @File       vxd_secure_msg.h
 * @Title      Low-level secure MSVDX interface component
 * @Description    This file contains the interface to a secure API for VXD
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

#if !defined (__VXD_SECUREMSG_H__)
#define __VXD_SECUREMSG_H__

#if defined(__cplusplus)
extern "C" {
#endif

#include "img_include.h"
#include "vxd_ext.h"

typedef enum _VXD_MSGS_
{
    MSG_SUBMIT_CODED_STREAM=0,  /*!<  Submits an encrypted buffer to the driver, it will be decoded as needed and any headers required will be extracted          */
    MSG_DECODE_STREAM,          /*!<  Sends a stream to the hardware for decoding          */
    MSG_HEADER_ELEMENTS,        /*!<  A response message from the submit coded stream, contains any extracted header elements, there is a 1:1 correlation of submits and header elements  */
    MSG_DECODED_FRAME,          /*!<  A response to a submit/decode message, contains decoded references and any sideband data      */
}VXD_MSGS;

typedef struct
{
    IMG_UINT32      ui32CoreRev;
    IMG_UINT32      ui32Internal;
    IMG_UINT32      ui32Latency;
    IMG_UINT32      ui32MmuStatus;
    IMG_UINT32      ui32CoreId;
    IMG_UINT32      ui32MultiCore;
    
#ifdef POST_TEST
    IMG_BOOL        bPost;
#endif
#ifdef STACK_USAGE_TEST
    IMG_BOOL        bStackUsageTest;
#endif

} VXD_sInitialiseArgs;

typedef struct
{
    IMG_UINT32      ui32PlaceHolder;

} VXD_sDeInitialiseArgs;

typedef struct
{
    IMG_BOOL        bClocksEnable;
    IMG_BOOL        bAutoClockGating;
    IMG_BOOL        bExtClockGating;

} VXD_sResetArgs;

typedef struct
{
    VXD_eCommsArea   eArea;
    IMG_BOOL         bFlushMmu;

} VXD_sSendFwMsgArgs;

typedef struct
{
    IMG_UINT32        ui32Pending;
    IMG_UINT32        ui32Requestor;
    IMG_UINT32        MMU_FAULT_ADDR;
    IMG_BOOL          MMU_FAULT_RNW;
    IMG_BOOL          MMU_PF_N_RW;
    IMG_UINT32        ui32MsgsSizeWrds;
    IMG_BOOL          bMoreData;

} VXD_sHandleInterruptsArgs;

typedef struct
{
    VXDIO_sState      sState;

} VXD_sGetStateArgs;

#if defined(__cplusplus)
}
#endif

#endif /* __VXD_SECUREMSG_H__ */



