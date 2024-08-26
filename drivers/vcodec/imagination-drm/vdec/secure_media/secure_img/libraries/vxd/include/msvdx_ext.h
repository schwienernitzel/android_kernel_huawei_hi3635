/*!
 *****************************************************************************
 *
 * @File       msvdx_ext.h
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

#if !defined (__MSVDX_EXT_H__)
#define __MSVDX_EXT_H__

#if defined(__cplusplus)
extern "C" {
#endif

#include "img_include.h"
#include "vdec.h"

#include "vdecfw.h"
#include "vdecfw_bin.h"

#include "mem_io.h"
#include "reg_io2.h"
#include "vxd_buf.h"
#include "vxd_ext.h"

/*! Maxmium number of DMA linked list elements:
 *  - max 3 elements of 64kB parts of firmware,
 *  - 1 potential element to fill gap between text and data section,
 *  - 2 elements for MTX enable,
 *  - 1 element to purge DMA state,
 *  - 1 element for storing MTX enable register values.
 *  See msvdxio_DMAMTX() */
#define VDEC_DMAC_LL_BUFS_COUNT 8

#define MSVDXIO_SECURE_VM_START         0xA6000000
#define MSVDXIO_SECURE_VM_SIZE          (0x100000000 - MSVDXIO_SECURE_VM_START)
#define MSVDXIO_SECURE_DEV_HEAP_SIZE    0x00C00000
#define MSVDXIO_SECURE_STR_HEAP_START   (MSVDXIO_SECURE_VM_START + MSVDXIO_SECURE_DEV_HEAP_SIZE)


#ifdef SECURE_USE_SYSOS

#include "sysos_api_km.h"
#define SEC_CreateEventObject(phEventHandle)                            SYSOSKM_CreateEventObject(phEventHandle)
#define SEC_WaitEventObject(hEventHandle, bUninterruptible)             SYSOSKM_WaitEventObject(hEventHandle, bUninterruptible)
#define SEC_SignalEventObject(hEventHandle)                             SYSOSKM_SignalEventObject(hEventHandle)
#define SEC_DestroyEventObject(hEventHandle)                            SYSOSKM_DestroyEventObject(hEventHandle)
#define SEC_CreateTimer(pfnTimer, pvParam, ui32TimeOut, phTimerHandle)  SYSOSKM_CreateTimer(pfnTimer, pvParam, ui32TimeOut, phTimerHandle)
#define SEC_DestroyTimer(hTimerHandle)                                  SYSOSKM_DestroyTimer(hTimerHandle)

#else /* not SECURE_USE_SYSOS */

#define SEC_CreateEventObject(phEventHandle)                            IMG_ERROR_NOT_SUPPORTED
#define SEC_WaitEventObject(hEventHandle, bUninterruptible)             IMG_ERROR_NOT_SUPPORTED
#define SEC_SignalEventObject(hEventHandle)                             IMG_ERROR_NOT_SUPPORTED
#define SEC_DestroyEventObject(hEventHandle)                            IMG_ERROR_NOT_SUPPORTED
#define SEC_CreateTimer(pfnTimer, pvParam, ui32TimeOut, phTimerHandle)  IMG_ERROR_NOT_SUPPORTED
#define SEC_DestroyTimer(hTimerHandle)                                  IMG_ERROR_NOT_SUPPORTED

#endif /* not SECURE_USE_SYSOS */


typedef enum
{
    BUFFER_INIT_RENDEC_0 = 0,
    BUFFER_INIT_RENDEC_1,

    BUFFER_INIT_MAX

} MSVDXIO_eInitBuffers;


typedef enum
{
    BUFFER_DEC_BATCH = 0,

    // Batch message.
    BUFFER_DEC_BITSTREAM,           // assuming single bitstream buffer per picture
    BUFFER_DEC_STARTCODE,
    BUFFER_DEC_ENDBYTES,
    BUFFER_DEC_TRANSACTION,
    BUFFER_DEC_STRPTD,
    BUFFER_DEC_PSRMOD,
    
    // Transaction.
    BUFFER_DEC_FWCTXLOAD,
    BUFFER_DEC_FWCTXSAVE,
    BUFFER_DEC_FWCTRLSAVE,
    BUFFER_DEC_VLCTAB,
    BUFFER_DEC_VLCIDX,
    BUFFER_DEC_FWPSRHDR,
    BUFFER_DEC_SEQHDRINFO,
    BUFFER_DEC_PPSHDRINFO,
    BUFFER_DEC_PPSHDRINFO2,

    // Picture Commands.
    BUFFER_DEC_IMAGE,
    BUFFER_DEC_ALTIMAGE,
    BUFFER_DEC_INTRA,
    BUFFER_DEC_AUXLINE,
    BUFFER_DEC_AUXMSB,
    
    BUFFER_DEC_MBPARAMS,

    // Standard-specific buffers.
    BUFFER_DEC_H264_SGM,
    BUFFER_DEC_MPEG4_DP0,
    BUFFER_DEC_MPEG4_DP1,
    BUFFER_DEC_MPEG4_FEVLRBCKUPLOAD,
    BUFFER_DEC_MPEG4_FEVLRBCKUPSTORE,
    BUFFER_DEC_VP8_MBFLAGS,
    BUFFER_DEC_VP8_SEGID,
    BUFFER_DEC_VP8_FIRSTPART,
    BUFFER_DEC_VP8_CURPICT,
    BUFFER_DEC_VP8_DCT,
    BUFFER_DEC_VC1_BP0,
    BUFFER_DEC_VC1_BP1,
    BUFFER_DEC_VC1_BP2,

    BUFFER_DEC_MAX

} MSVDXIO_eDecodeBuffers;



/*!
******************************************************************************
 This structure contains MTX software information.
 @brief  MTX Software Information
******************************************************************************/
typedef struct
{
    IMG_UINT32    ui32TextOrigin;
    IMG_UINT32    ui32TextAddr;
    IMG_UINT32 *  pui32Text;
    IMG_UINT32    ui32TextDmaSize;
    IMG_UINT32    ui32TextBufSize;
    IMG_UINT32    ui32DataOrigin;
    IMG_UINT32    ui32DataAddr;
    IMG_UINT32 *  pui32Data;
    IMG_UINT32    ui32DataSize;
    IMG_UINT32 *  pui32TextReloc;
    IMG_UINT8 *   pui8TextRelocType;
    IMG_UINT32 *  pui32TextRelocFullAddr;
    IMG_UINT32    ui32TextRelocSize;
    IMG_UINT32 *  pui32DataReloc;
    IMG_UINT32    ui32DataRelocSize;

} MSVDXIO_sMTXSwInfo;

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
 This structure contains information about secure firmware binary and buffer
 used to store it.

 @brief  Secure firmware info

******************************************************************************/
typedef struct
{
    VXDIO_sDdBufInfo         sBufInfo;                       /*!< Secure firmware binary buffer info                        */
    VDECFW_sSecFwBinInfo     sBinInfo;                       /*!< Secure firmware binary info                               */

} MSVDXIO_sSecFw;


typedef struct
{
    VDECFW_sFirmwareBinInfo  sFwBaseBinInfo;                 /*!< Firmware base text, data and relocation sections info.    */
    VDECFW_sFirmwareBinInfo  asFwBinModInfo[VDEC_STD_MAX];   /*!< Firmware module text, data and relocation sections info.  */

    VXDIO_sDdBufInfo         sFwBaseBufInfo;                 /*!< Firmware base text and data buffer info.                  */

    VXDIO_sDdBufInfo         asFwTextBufInfo[VDEC_STD_MAX];  /*!< Firmware modules text buffer info.                        */
    VXDIO_sDdBufInfo         asFwDataBufInfo[VDEC_STD_MAX];  /*!< Firmware modules data buffer info.                        */

    VXDIO_sDdBufInfo         sDmaLLBufInfo;                  /*!< DMA linked list buffer info.                              */

    VXDIO_sDdBufInfo         sPsrModInfo;                    /*!< Parser Module buffer info.                                */

    MSVDXIO_sSecFw           sSecFw;                         /*!< Secure firmware info.                                     */

} MSVDXIO_sFw;

/*!
******************************************************************************

 @Function              MSVDXIO_Initialise

 @Description

 Initialises an MSVDXIO context.

 @Input     bPostReq            : Whether post bootup tests should be executed.

 @Input     bStackUsageTestReq  : Whether MTX stack usage tests should be
                                  executed.

 @Output    phContext           : A pointer to handle to the MSVDX IO context.

 @Return    IMG_RESULT          : Returns either IMG_SUCCESS or an error code.

******************************************************************************/
extern IMG_RESULT
MSVDXIO_Initialise(
    IMG_BOOL                    bPostReq,
    IMG_BOOL                    bStackUsageTestReq,
    IMG_HANDLE                * phContext
);

/*!
******************************************************************************

 @Function              MSVDXIO_DeInitialise

******************************************************************************/
IMG_RESULT 
MSVDXIO_DeInitialise(
    IMG_HANDLE          hContext
);


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

 @Input     hContext             : Handle to MSVDX IO context.

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

 @Function              MSVDXIO_Poll

 @Description

 Poll for a specific register value.

 @Input     hContext        : Handle to MSVDX IO context.

 @Input     ui32MemRegion   : Memory region of register.

 @Input     ui32Offset      : Register offset within register/memory space.

 @Input     ui32RequValue   : Required register value.

 @Input     ui32Enable      : Mask of live bits to apply to register.

 @Input     ePollMode       : Check function to be used (equals, !equals ect)

 @Return    IMG_RESULT      : Returns either IMG_SUCCESS or an error code.

******************************************************************************/
extern IMG_RESULT
MSVDXIO_Poll(
    const IMG_HANDLE    hContext,
    IMG_UINT32          ui32MemRegion,
    IMG_UINT32          ui32Offset,
    IMG_UINT32          ui32RequValue,
    IMG_UINT32          ui32Enable,
    MSVDXIO_ePollMode     ePollMode
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

 @Function              MSVDXIO_HandleInterrupts

 @Description

 Handle any pending MSVDX interrupts and clear.

 @Input     hContext        : Handle to MSVDX IO context.

 @Output    psIntStatus     : Pointer to interrupt status information.

 @Return    IMG_RESULT      : Returns either IMG_SUCCESS or an error code.

******************************************************************************/
extern IMG_RESULT
MSVDXIO_HandleInterrupts(
    const IMG_HANDLE    hContext,
    VXD_sIntStatus    * psIntStatus
);


/*!
******************************************************************************

 @Function              MSVDXIO_SendFirmwareMessage
 
 @Description

 Send a message to firmware.

 @Input     hContext        : Handle to MSVDX IO context.

 @Input     eArea           : Comms area into which the message should be written.

 @Input     psMsgHdr        : Pointer to message to send.

 @Return    IMG_RESULT      : Returns either IMG_SUCCESS or an error code.

******************************************************************************/
extern IMG_RESULT
MSVDXIO_SendFirmwareMessage(
    const IMG_HANDLE   hConntext,
    VXD_eCommsArea     eArea,
    const IMG_VOID   * psMsgHdr
);


/*!
******************************************************************************

 @Function              MSVDXIO_LoadBaseFirmware

 @Description

 Load the firmware base component into MTX RAM.

 @Input     hContext        : Handle to MSVDX IO context.

 @Return    IMG_RESULT      : Returns either IMG_SUCCESS or an error code.

******************************************************************************/
extern IMG_RESULT 
MSVDXIO_LoadBaseFirmware(
    const IMG_HANDLE        hContext
);


/*!
******************************************************************************

 @Function              MSVDXIO_PrepareFirmware

 @Description

 Prepares the firmware for loading to MTX.

 @Input     hContext        : Handle to MSVDX IO context.

 @Input     hFirmware       : Handle to structure containing core specific
                              firmware layout information.

 @Return    IMG_RESULT      : Returns either IMG_SUCCESS or an error code.

******************************************************************************/
extern IMG_RESULT
MSVDXIO_PrepareFirmware(
    const IMG_HANDLE            hContext,
    const IMG_HANDLE            hFirmware
);


/*!
******************************************************************************

 @Function              MSVDXIO_DisableClocks

 @Description

 Disables MSVDX clocks.

 @Input     hContext        : Handle to MSVDX IO context.

 @Return    IMG_RESULT      : Returns either IMG_SUCCESS or an error code.

******************************************************************************/
extern IMG_RESULT 
MSVDXIO_DisableClocks(
    const IMG_HANDLE    hContext
);


/*!
******************************************************************************

 @Function              MSVDXIO_EnableClocks

 @Description

 Enables MSVDX clocks.

 @Input     hContext                : Handle to MSVDX IO context.

 @Input     bAutoClockGatingSupport : Auto clock-gating is supported by the core.

 @Input     bExtClockGating         : Extended clock-gating register are present on core.

 @Input     bForceManual            : Force manual clock-gating.

 @Return    IMG_RESULT              : Returns either IMG_SUCCESS or an error code.

******************************************************************************/
extern IMG_RESULT
MSVDXIO_EnableClocks(
    const IMG_HANDLE    hContext,
    IMG_BOOL            bAutoClockGatingSupport,
    IMG_BOOL            bExtClockGating,
    IMG_BOOL            bForceManual
);


/*
******************************************************************************

 @Function              MSVDXIO_SetSecRegAccess

******************************************************************************/
extern IMG_VOID 
MSVDXIO_SetSecRegAccess(
    const IMG_HANDLE       hContext
);

/*!
******************************************************************************

 @Function              MSVDXIO_WriteRegister

******************************************************************************/
extern IMG_RESULT
MSVDXIO_WriteRegister(
    const IMG_HANDLE    hContext,
    IMG_UINT32          ui32MemRegion,
    IMG_UINT32          ui32Offset,
    IMG_UINT32          ui32Value,
    IMG_UINT32          ui32Mask
);

/*!
******************************************************************************

 @Function              MSVDXIO_ReadRegister

******************************************************************************/
extern IMG_UINT32
MSVDXIO_ReadRegister(
    const IMG_HANDLE    hContext,
    IMG_UINT32          ui32MemRegion,
    IMG_UINT32          ui32Offset
);

/*!
******************************************************************************

 @Function              MSVDXIO_VLRReadWords

******************************************************************************/
extern IMG_RESULT
MSVDXIO_VLRReadWords(
    const IMG_HANDLE    hContext,
    IMG_UINT32          ui32MemRegion,
    IMG_UINT32          ui32Addr,
    IMG_UINT32          ui32NumWords,
    IMG_UINT32        * pui32Values,
    IMG_BOOL            bValidate
);

/*!
******************************************************************************

 @Function              MSVDXIO_VLRWriteWords

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
MSVDXIO_VLRWriteWords(
    const IMG_HANDLE    hContext,
    IMG_UINT32          ui32MemRegion,
    IMG_UINT32          ui32Addr,
    IMG_UINT32          ui32NumWords,
    const IMG_UINT32  * pui32Values
);


/*!
******************************************************************************

 @Function              MSVDXIO_VLRReadWords

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
MSVDXIO_VLRReadWords(
    const IMG_HANDLE    hContext,
    IMG_UINT32          ui32MemRegion,
    IMG_UINT32          ui32Addr,
    IMG_UINT32          ui32NumWords,
    IMG_UINT32        * pui32Values,
    IMG_BOOL            bValidate
);


/*!
******************************************************************************

 @Function              MSVDXIO_ResetCore

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
MSVDXIO_ResetCore(
    const IMG_HANDLE    hContext,
    IMG_BOOL            bAutoClockGating,
    IMG_BOOL            bExtClockGating,
    IMG_BOOL            bClocksEnable
);




#if defined(__cplusplus)
}
#endif

#endif /* __MSVDXIO_H__ */



