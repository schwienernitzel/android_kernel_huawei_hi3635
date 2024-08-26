/*!
 *****************************************************************************
 *
 * @File       msvdx_io.h
 * @Title      Low-level MSVDX interface component
 * @Description    This file contains the interface to communicate with an MSVDX core.
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

#if !defined (__MSVDXIO_H__)
#define __MSVDXIO_H__

#if defined(__cplusplus)
extern "C" {
#endif

#include "img_include.h"
#include "vdec.h"

#include "secure_defs.h"

#include "vdecfw.h"
#include "vdecfw_bin.h"
#include "vxd_buf.h"
#include "vxd_ext.h"

#include "mem_io.h"
#include "reg_io2.h"


#ifdef STACK_USAGE_TEST
#ifdef SEC_USE_REAL_FW
	#define STACK_START_ADDRESS		((IMG_UINT32)0x000068E0)	/* VdecFwBase_non_oold_multi.elf.map */
	#define PARSER_START_ADDRESS	((IMG_UINT32)0x0000D200)	/* vdec/firmware/apps/multi/CMakeLists.txt */
	#define STACK_LAST_ADDRESS		((IMG_UINT32)(PARSER_START_ADDRESS - 4))
	#define STACK_SIZE				((IMG_UINT32)((PARSER_START_ADDRESS - STACK_START_ADDRESS)/4))
	#define STACK_FILL_VALUE		((IMG_UINT32)0x11111111)
#else
	/* nothing */
#endif
#endif

/*!
******************************************************************************
 This structure contains MTX RAM information.
 @brief  MTX RAM Information
******************************************************************************/
typedef struct
{
    IMG_UINT32 ui32MTXBankSize;
    IMG_UINT32 ui32MTXRamSize;
    IMG_UINT32 ui32MTXRamMask;

} MSVDXIO_sMTXRamInfo;

/*!
******************************************************************************
 @brief  MSVDXIO Poll function modes
******************************************************************************/
typedef enum
{
    MSVDXIO_POLL_EQUAL,
    MSVDXIO_POLL_NOT_EQUAL,
    MSVDXIO_POLL_MAX

} MSVDXIO_ePollMode;



/*!
******************************************************************************
 This structure contains registers holding main core properties.
 @brief  Core propertires registers
******************************************************************************/
typedef struct
{
    IMG_UINT32 ui32CoreRev;
    IMG_UINT32 ui32Internal;
    IMG_UINT32 ui32Latency;
    IMG_UINT32 ui32MmuStatus;
    IMG_UINT32 ui32CoreId;
    IMG_UINT32 ui32MultiCore;

} MSVDXIO_sCoreRegs;

#ifdef STACK_USAGE_TEST
/*!
******************************************************************************

 @Function              MSVDXIO_GetStackUsage

 @Description

 Get stack usage by checking the MTX memory stack space.

 @Input     hContext    : Handle to MSVDX IO context.

 @Input   * paui32StackInfoArray : Handle to stack usage info array to update.

******************************************************************************/
extern IMG_RESULT 
MSVDXIO_GetStackUsage(
	const IMG_HANDLE    hContext,
	IMG_UINT32        * paui32StackInfoArray
);
#endif


/*!
******************************************************************************

 @Function              MSVDXIO_SEC_Poll

 @Description

 Poll for a specific register value.

 @Input     hContext    : Handle to MSVDX IO context.

 @Input     ui32MemRegion   : Memory region of register.

 @Input     ui32Offset      : Register offset within register/memory space.

 @Input     ui32RequValue   : Required register value.

 @Input     ui32Enable      : Mask of live bits to apply to register.

 @Input     ePollMode       : Check function to be used (equals, !equals ect)

 @Return    IMG_RESULT      : Returns either IMG_SUCCESS or an error code.

******************************************************************************/
extern IMG_RESULT
MSVDXIO_SEC_Poll(
    const IMG_HANDLE    hContext,
    IMG_UINT32          ui32MemRegion,
    IMG_UINT32          ui32Offset,
    IMG_UINT32          ui32RequValue,
    IMG_UINT32          ui32Enable,
    MSVDXIO_ePollMode   ePollMode
);


/*!
******************************************************************************

 @Function              MSVDXIO_GetCoreState

 @Description

 Obtain the MSVDX core state.

 @Input     hContext    : Handle to MSVDX IO context.

 @Output    psState         : Pointer to state information.

 @Return    IMG_RESULT      : Returns either IMG_SUCCESS or an error code.

******************************************************************************/
extern IMG_RESULT
MSVDXIO_GetCoreState(
    const IMG_HANDLE    hContext,
    VXDIO_sState      * psState
);


/*!
******************************************************************************

 @Function              MSVDXIO_SEC_HandleInterrupts

 @Description

 Handle any pending MSVDX interrupts and clear.

 @Input     hContext            : Handle to MSVDX IO context.

 @Output    psIntStatus         : Pointer to interrupt status information.

 @Output    pbyMtxMsgs          : Buffer to place MTX messages read out from
                                  comms RAM.

 @In/out    pui32MsgsSizeWrds   : Input: Size of buffer for MTX messages in
                                         32bit words
                                  Output: Number of words written to the
                                          buffer

 @Output    pbMoreData          : Whether there are more messages to be read.

 @Return    IMG_RESULT          : Returns either IMG_SUCCESS or an error code.

******************************************************************************/
extern IMG_RESULT
MSVDXIO_SEC_HandleInterrupts(
    const IMG_HANDLE    hContext,
    VXD_sIntStatus    * psIntStatus,
    IMG_UINT32        * pbyMtxMsgs,
    IMG_UINT32        * pui32MsgsSizeWrds,
    IMG_BOOL          * pbMoreData
);


/*!
******************************************************************************

 @Function              MSVDXIO_SEC_SendFirmwareMessage
 
 @Description

 Send a message to firmware.

 @Input     hContext        : Handle to MSVDX IO context.

 @Input     eArea           : Comms area into which the message should be written.

 @Input     psMsgHdr        : Pointer to buffer containing message to send.

 @Input     ui32MsgBufSize  : Size of the message buffer in bytes.

 @Return    IMG_RESULT      : Returns either IMG_SUCCESS or an error code.

******************************************************************************/
extern IMG_RESULT
MSVDXIO_SEC_SendFirmwareMessage(
    const IMG_HANDLE   hContext,
    VXD_eCommsArea     eArea,
    const IMG_VOID   * psMsgHdr,
    IMG_UINT32         ui32MsgBufSize
);


/*!
******************************************************************************

 @Function              MSVDXIO_SEC_LoadBaseFirmware

 @Description

 Load the secure firmware into MTX RAM.

 @Input     hContext        : Handle to MSVDX IO context.

 @Input     ui32MmuCtrl2    : Value of CR_MMU_CONTROL2 to be written.

 @Input     ui32PtdPhysAddr : Physicall address of a buffer containing PTD.

 @Return    IMG_RESULT      : Returns either IMG_SUCCESS or an error code.

******************************************************************************/
extern IMG_RESULT 
MSVDXIO_SEC_LoadBaseFirmware(
    const IMG_HANDLE        hContext,
    IMG_UINT32              ui32MmuCtrl2,
    IMG_UINT32              ui32PtdPhysAddr
);


/*!
******************************************************************************

 @Function              MSVDXIO_SEC_PrepareFirmware

 @Description

 Prepares the secure firmware for loading to MTX.

 @Input     hContext    : Handle to MSVDX IO context.

 @Input     pbFwBin         : Pointer to firmware binary.

 @Input     ui32FwBinSize   : Size of firmware binary image.

 @Input     psSecFwBufInfo  : Secure device buffer that will be used to store
                              firmware. It's CPU virtual address should be valid,
                              but it should not be yet mapped in device MMU.

 @Return    IMG_RESULT      : Returns either IMG_SUCCESS or an error code.

******************************************************************************/
extern IMG_RESULT
MSVDXIO_SEC_PrepareFirmware(
    const IMG_HANDLE            hContext,
    IMG_BYTE                  * pbFwBin,
    IMG_UINT32                  ui32FwBinSize,
    VXDIO_sDdBufInfo          * psSecFwBufInfo
);


/*!
******************************************************************************

 @Function              MSVDXIO_SEC_DisableClocks

 @Description

 Disables MSVDX clocks.

 @Input     hContext    : Handle to MSVDX IO context.

 @Return    IMG_RESULT      : Returns either IMG_SUCCESS or an error code.

******************************************************************************/
extern IMG_RESULT 
MSVDXIO_SEC_DisableClocks(
    const IMG_HANDLE    hContext
);


/*!
******************************************************************************

 @Function              MSVDXIO_SEC_EnableClocks

 @Description

 Enables MSVDX clocks.

 @Input     hContext            : Handle to MSVDX IO context.

 @Input     bAutoClockGatingSupport : Auto clock-gating is supported by the core.

 @Input     bExtClockGating         : Extended clock-gating register are present on core.

 @Input     bForceManual            : Force manual clock-gating.

 @Return    IMG_RESULT              : Returns either IMG_SUCCESS or an error code.

******************************************************************************/
extern IMG_RESULT
MSVDXIO_SEC_EnableClocks(
    const IMG_HANDLE    hContext,
    IMG_BOOL            bAutoClockGatingSupport,
    IMG_BOOL            bExtClockGating,
    IMG_BOOL            bForceManual
);

/*
******************************************************************************

 @Function              MSVDXIO_SEC_Initialise

******************************************************************************/
extern IMG_RESULT
MSVDXIO_SEC_Initialise(
    IMG_BOOL               bPostReq,
    IMG_BOOL               bStackUsageTestReq,
    IMG_HANDLE           * phContext
);

/*
******************************************************************************

 @Function              MSVDXIO_SEC_DeInitialise

******************************************************************************/
IMG_RESULT 
MSVDXIO_SEC_DeInitialise(
    IMG_HANDLE          hContext
);


/*
******************************************************************************

 @Function              MSVDXIO_SEC_SetSecRegAccess

******************************************************************************/
extern IMG_VOID 
MSVDXIO_SEC_SetSecRegAccess(
    const IMG_HANDLE       hContext
);

/*!
******************************************************************************

 @Function              MSVDXIO_SEC_ReadRegister

 @Description

 Read value from register.

 @Input     hContext        : Handle to video device IO context.

 @Input     ui32MemRegion   : Memory region of register.

 @Input     ui32Offset      : Register offset within register/memory space.

 @Return    IMG_RESULT      : Returns either IMG_SUCCESS or an error code.

******************************************************************************/
extern IMG_UINT32
MSVDXIO_SEC_ReadRegister(
    const IMG_HANDLE    hContext,
    IMG_UINT32          ui32MemRegion,
    IMG_UINT32          ui32Offset
);


/*!
******************************************************************************

 @Function              MSVDXIO_SEC_WriteRegister

 @Description

 Write value to register.

 @Input     hContext        : Handle to video device IO context.

 @Input     ui32MemRegion   : Memory region of register.

 @Input     ui32Offset      : Register offset within register/memory space.

 @Input     ui32Value       : Value to write to register.

 @Input     ui32Mask        : Mask of bits to modify to register.

 @Return    IMG_RESULT      : Returns either IMG_SUCCESS or an error code.

******************************************************************************/
extern IMG_RESULT
MSVDXIO_SEC_WriteRegister(
    const IMG_HANDLE    hContext,
    IMG_UINT32          ui32MemRegion,
    IMG_UINT32          ui32Offset,
    IMG_UINT32          ui32Value,
    IMG_UINT32          ui32Mask
);

/*!
******************************************************************************

 @Function              MSVDXIO_SEC_VLRWriteWords

 @Description

 This function writes an array of words to VEC local RAM (VLR).

 @Input     hContext         : Handle to video device IO context.

 @Input     ui32MemRegion    : VLR memory space.

 @Input     ui32Addr         : Address (byte) in VLR.

 @Input     ui32NumWords     : Number of 32-bits words to write.

 @Input     pui32Values      : Array of words to write.

 @Return    IMG_RESULT       : Returns either IMG_SUCCESS or an error code.

******************************************************************************/
extern IMG_RESULT
MSVDXIO_SEC_VLRWriteWords(
    const IMG_HANDLE    hContext,
    IMG_UINT32          ui32MemRegion,
    IMG_UINT32          ui32Addr,
    IMG_UINT32          ui32NumWords,
    const IMG_UINT32  * pui32Values
);


/*!
******************************************************************************

 @Function              MSVDXIO_SEC_VLRReadWords

 @Description

 This function reads an array of words from VEC local RAM (VLR).

 @Input     hContext         : Handle to video device IO context.

 @Input     ui32MemRegion    : VLR memory space.

 @Input     ui32Addr         : Address (byte) in VLR.

 @Input     ui32NumWords     : Number of 32-bits words to read.

 @Input     pui32Values      : Array of words to read.

 @Input     bValidate        : POL values read from VLR

 @Return    IMG_RESULT       : Returns either IMG_SUCCESS or an error code.

******************************************************************************/
extern IMG_RESULT
MSVDXIO_SEC_VLRReadWords(
    const IMG_HANDLE    hContext,
    IMG_UINT32          ui32MemRegion,
    IMG_UINT32          ui32Addr,
    IMG_UINT32          ui32NumWords,
    IMG_UINT32        * pui32Values,
    IMG_BOOL            bValidate
);

/*!
******************************************************************************

 @Function              MSVDXIO_SEC_GetCoreState

 Obtain the video decoder device core state.

 @Input     hContext        : Handle to video device IO context.

 @Output    psState         : Pointer to state information.

 @Return    IMG_RESULT      : Returns either IMG_SUCCESS or an error code.

******************************************************************************/
IMG_RESULT
MSVDXIO_SEC_GetCoreState(
    const IMG_HANDLE    hContext,
    VXDIO_sState      * psState
);

/*!
******************************************************************************

 @Function              MSVDXIO_SEC_GetCoreProps

 Obtain values of registers describing main core properties

 @Input     hContext        : Handle to video device IO context.

 @Output    psCoreRegs      : Values of registers holding main core
                              properties.

 @Return    IMG_RESULT      : Returns either IMG_SUCCESS or an error code.

******************************************************************************/
IMG_RESULT
MSVDXIO_SEC_GetCoreProps(
    const IMG_HANDLE      hContext,
    MSVDXIO_sCoreRegs   * psCoreRegs
);

/*!
******************************************************************************

 @Function              MSVDXIO_SEC_ResetCore

 Resets the MSVDX core and enables or disables clocks.

 @Input     hContext        : Handle to video device IO context.

 @Input     bAutoClockGating: Values of registers holding main core
                              properties.

 @Input     bExtClockGating : Extended clock-gating register are present on
                              core.

 @Input     bClocksEnable   : Whether clocks should be enable or disabled.

 @Return    IMG_RESULT      : Returns either IMG_SUCCESS or an error code.

******************************************************************************/
IMG_RESULT
MSVDXIO_SEC_ResetCore(
    const IMG_HANDLE    hContext,
    IMG_BOOL            bAutoClockGating,
    IMG_BOOL            bExtClockGating,
    IMG_BOOL            bClocksEnable
);

/*!
******************************************************************************

 @Function              MSVDXIO_SEC_FlushMmu

 Flush VXD's MMU cache

 @Input     hContext        : Handle to video device IO context.

 @Return    IMG_RESULT      : Returns either IMG_SUCCESS or an error code.

******************************************************************************/
IMG_RESULT
MSVDXIO_SEC_FlushMmu(
    const IMG_HANDLE      hContext
);


#if defined(__cplusplus)
}
#endif

#endif /* __MSVDXIO_H__ */



