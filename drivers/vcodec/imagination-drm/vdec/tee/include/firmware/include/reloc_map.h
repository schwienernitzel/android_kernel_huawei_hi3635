/*!
 *****************************************************************************
 *
 * @File       reloc_map.h
 * @Title      Mapping of framework functions
 * @Description    The mapping of framework functions to entry macros is performed here.
 *  All parsers should include this file if they call framework functions.
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

#ifndef RELOC_MAP_H
#define RELOC_MAP_H

#include "img_types.h"
#include "reloc_api.h"

#if defined (__cplusplus)
extern "C" {
#endif


// These function types exist to allow convenient dispatching to the framework
// entry points from function calls with different numbers of parameters.
typedef IMG_UINT32 (*RELOC_pfnCallFramework0)(RELOC_eFunction eFunction);

typedef IMG_UINT32 (*RELOC_pfnCallFramework1)(IMG_UINT32 ui32Param1,
                                              RELOC_eFunction eFunction);

typedef IMG_UINT32 (*RELOC_pfnCallFramework2)(IMG_UINT32 ui32Param1,
                                              IMG_UINT32 ui32Param2,
                                              RELOC_eFunction eFunction);

typedef IMG_UINT32 (*RELOC_pfnCallFramework3)(IMG_UINT32 ui32Param1,
                                              IMG_UINT32 ui32Param2,
                                              IMG_UINT32 ui32Param3,
                                              RELOC_eFunction eFunction);

typedef IMG_UINT32 (*RELOC_pfnCallFramework4)(IMG_UINT32 ui32Param1,
                                              IMG_UINT32 ui32Param2,
                                              IMG_UINT32 ui32Param3,
                                              IMG_UINT32 ui32Param4,
                                              RELOC_eFunction eFunction);

typedef IMG_UINT32 (*RELOC_pfnCallFramework5)(IMG_UINT32 ui32Param1,
                                              IMG_UINT32 ui32Param2,
                                              IMG_UINT32 ui32Param3,
                                              IMG_UINT32 ui32Param4,
                                              IMG_UINT32 ui32Param5,
                                              RELOC_eFunction eFunction);


// The entry points for the framework.
extern RELOC_pfnCallFramework0 RELOC_CallFramework0;
extern RELOC_pfnCallFramework1 RELOC_CallFramework1;
extern RELOC_pfnCallFramework2 RELOC_CallFramework2;
extern RELOC_pfnCallFramework3 RELOC_CallFramework3;
extern RELOC_pfnCallFramework4 RELOC_CallFramework4;
extern RELOC_pfnCallFramework5 RELOC_CallFramework5;


// Macros to map framework functions.
#define RENDEC_SliceStart()                         RELOC_CallFramework0(RELOC_RENDEC_SLICESTART)
#define RENDEC_ChunkStart(a)                        RELOC_CallFramework1((IMG_UINT32)(a), RELOC_RENDEC_CHUNKSTART)
#define RENDEC_QueueWord(a)                         RELOC_CallFramework1((IMG_UINT32)(a), RELOC_RENDEC_QUEUEWORD)
#define RENDEC_SliceEnd()                           RELOC_CallFramework0(RELOC_RENDEC_SLICEEND)
#define RENDEC_InsertSecondPassMarker(a,b,c,d)      RELOC_CallFramework4((IMG_UINT32)(a), (IMG_UINT32)(b), (IMG_UINT32)(c), (IMG_UINT32)(d), RELOC_RENDEC_INSERTSECONDPASSMARKER)
#define RENDEC_WriteBuffer(a, b, c)                 RELOC_CallFramework3((IMG_UINT32)(a), (IMG_UINT32)(b), (IMG_UINT32)(c), RELOC_RENDEC_WRITEBUF)
#define SR_SetMaster(a)                             RELOC_CallFramework1((IMG_UINT32)(a), RELOC_SR_SETMASTER)
#define SR_SeekScpOrEod()                           RELOC_CallFramework0(RELOC_SR_SEEKSCPOREOD)
#define SR_ReadScp()                                RELOC_CallFramework0(RELOC_SR_READSCP)
#define SR_CheckScpOrEod()                          RELOC_CallFramework0(RELOC_SR_CHECKSCPOREOD)
#define SR_OutputCmdCheckResp(a)                    RELOC_CallFramework1((IMG_UINT32)(a), RELOC_SR_OUTPUTCMDCHECKRESP)
#define SR_OutputCmdCheckRespExpGoulomb(a)          RELOC_CallFramework1((IMG_UINT32)(a), RELOC_SR_OUTPUTCMDCHECKRESPEXPGOULOMB)
#define SR_Config(a, b, c, d)                       RELOC_CallFramework4((IMG_UINT32)(a), (IMG_UINT32)(b), (IMG_UINT32)(c), (IMG_UINT32)(d), RELOC_SR_CONFIG)
#define SR_WaitForValid()                           RELOC_CallFramework0(RELOC_SR_WAITFORVALID)
#if defined (MTXG) && defined (FW_PRINT)
#define VDECFW_Print_S(a)                           RELOC_CallFramework1((IMG_UINT32)(a), RELOC_VDECFW_PRINT_S)
#define VDECFW_Print_SV(a,b)                        RELOC_CallFramework2((IMG_UINT32)(a), (IMG_UINT32)(b), RELOC_VDECFW_PRINT_SV)
#endif
#ifndef NO_BOOL_CODER
#define SR_BoolCoderReadBitProb(a)                  RELOC_CallFramework1((IMG_UINT32)(a), RELOC_SR_BOOLCODERREADBITPROB)
#define SR_BoolCoderReadBits128(a)                  RELOC_CallFramework1((IMG_UINT32)(a), RELOC_SR_BOOLCODERREADBITS128)
#define SR_SetByteCount(a)                          RELOC_CallFramework1((IMG_UINT32)(a), RELOC_SR_SETBYTECOUNT)
#define SR_BoolCoderConfig(a, b, c)                 RELOC_CallFramework3((IMG_UINT32)(a), (IMG_UINT32)(b), (IMG_UINT32)(c), RELOC_SR_BOOLCODERCONFIG)
#define SR_BoolCoderGetContext(a, b)                RELOC_CallFramework2((IMG_UINT32)(a), (IMG_UINT32)(b), RELOC_SR_BOOLCODERGETCONTEXT)
#define SR_BoolCoderSetMaster(a)                    RELOC_CallFramework1((IMG_UINT32)(a), RELOC_SR_BOOLCODERSETMASTER)
#define SR_QueryByteCount()                         RELOC_CallFramework0(RELOC_SR_QUERYBYTECOUNT)
#endif /* NO_BOOL_CODER */
#define DMA_FlushChannelLinkedListNotCb(a)          RELOC_CallFramework1((IMG_UINT32)(a), RELOC_DMA_FLUSHCHANNELLINKEDLISTNOTCB)
#define DMA_ModifyLinkedListHead(a, b, c, d)        RELOC_CallFramework4((IMG_UINT32)(a), (IMG_UINT32)(b), (IMG_UINT32)(c), (IMG_UINT32)(d), RELOC_DMA_MODIFYLINKEDLISTHEAD)
#define DMA_SetSecureSyncTransfer(a, b, c, d, e)    RELOC_CallFramework5((IMG_UINT32)(a), (IMG_UINT32)(b), (IMG_UINT32)(c), (IMG_UINT32)(d), (IMG_UINT32)(e), RELOC_DMA_SETSECURESYNCTRANSFER)
#define DMA_SetInsecureSyncTransfer(a, b, c, d, e)  RELOC_CallFramework5((IMG_UINT32)(a), (IMG_UINT32)(b), (IMG_UINT32)(c), (IMG_UINT32)(d), (IMG_UINT32)(e), RELOC_DMA_SETINSECURESYNCTRANSFER)
#define VDECFW_setFlagsWord(a, b)                   RELOC_CallFramework2((IMG_UINT32)(a), (IMG_UINT32)(b), RELOC_VDECFW_SETFLAGSWORD)
#define VDECFW_clearFlagsWord(a, b)                 RELOC_CallFramework2((IMG_UINT32)(a), (IMG_UINT32)(b), RELOC_VDECFW_CLEARFLAGSWORD)
#define VDECFW_GetExcBuf()                          RELOC_CallFramework0(RELOC_FRAMEWORL_VDECFW_GETEXCBUF)
#if defined (__cplusplus)
}
#endif

#endif  // RELOC_MAP_H
