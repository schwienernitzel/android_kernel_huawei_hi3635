/*!
 *****************************************************************************
 *
 * @File       secure_msg.h
 * @Title      Secure Message Definitions
 * @Description    This file contains the definition required for using a secure API
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

#if !defined (__SECURE_MSG_H__)
#define __SECURE_MSG_H__

#if defined(__cplusplus)
extern "C" {
#endif

#include <img_include.h>

#include <bspp.h>
#include <vxd_ext.h>

/*
  ENDPOINT_VXD_INPUT
*/
typedef struct
{
    VXD_eCommsArea   eArea;
    IMG_BOOL         bFlushMmu;

} VXD_sSendFwMsgArgs;

/*
  ENDPOINT_BSPP_STREAM_CREATE
*/
typedef struct
{
	//Input
    VDEC_sStrConfigData   sStrConfigData;
	BSPP_sDdBufArrayInfo  asFWSequence[MAX_SEQUENCES_SECURE];
    BSPP_sDdBufArrayInfo  asFWPPS[MAX_PPSS_SECURE];
	
    //Output
    IMG_UINT32              ui32StrId;

} BSPP_SECURE_sStreamCreateArgs;


/*
  ENDPOINT_BSPP_STREAM_DESTROY
*/
typedef struct
{
	//Input
    IMG_UINT32              ui32StrId;

} BSPP_SECURE_sStreamDestroyArgs;

/*
  ENDPOINT_BSPP_SUBMIT_PICTURE_DECODED
*/
typedef struct
{
	//Input
	IMG_UINT32              ui32StrId;
    IMG_UINT32              ui32SequHdrId;
    IMG_UINT32              ui32PPSId;
    IMG_UINT32              ui32SecondPPSId;

} BSPP_SECURE_sSubmitPictureDecodedArgs;

/*
  ENDPOINT_BSPP_STREAM_SUBMIT_BUFFER
*/
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

/*
  ENDPOINT_BSPP_STREAM_PREPARSE_BUFFERS
*/
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

/*
  ENDPOINT_VXD_INITIALISE
*/
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

/*
  ENDPOINT_VXD_DEINITIALISE
*/
typedef struct
{
    IMG_UINT32      ui32PlaceHolder;

} VXD_sDeInitialiseArgs;

/*
  ENDPOINT_VXD_RESET
*/
typedef struct
{
    IMG_BOOL        bClocksEnable;
    IMG_BOOL        bAutoClockGating;
    IMG_BOOL        bExtClockGating;

} VXD_sResetArgs;

/*
  ENDPOINT_VXD_HANDLE_INTERRUPTS
*/
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

/*
  ENDPOINT_VXD_GET_STATE
*/
typedef struct
{
    VXDIO_sState      sState;

} VXD_sGetStateArgs;

/*
  ENDPOINT_VXD_PREPARE_FIRMWARE
*/
typedef struct
{
    //Input
    IMG_UINT32          ui32FwBufCpuPhys;
    IMG_UINT32          ui32FwBufSizeBytes;
    IMG_UINT32          ui32FwBufMemAttrib;
    IMG_UINT32          ui32DevVirtAddr;

} VXD_SECURE_sPrepareFirmwareArgs;

/*
  ENDPOINT_VXD_LOAD_CORE_FW
*/
typedef struct
{
    IMG_UINT32          ui32PtdPhysAddr;
    IMG_UINT32          ui32MmuCtrl2;

} VXD_SECURE_LoadCoreFW;

/*
  ENDPOINT_VXD_READN_REGS
*/
typedef struct
{
    IMG_UINT32          ui32Region;
    IMG_UINT32          ui32NumRegs;
    IMG_UINT32          ui32Offset;
    IMG_UINT32          ui32RegVal;

} VXD_SECURE_sReadNRegsArgs;


#if defined(__cplusplus)
}
#endif

#endif /* __SECURE_MSG_H__ */
