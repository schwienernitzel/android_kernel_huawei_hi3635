/*!
 *****************************************************************************
 *
 * @File       dma_ll.h
 * @Title      Utilities for MSVDX DMA linked list.
 * @Description    This file contains hardware related definitions and macros
 *  for MSVDX DMA linked list. DMA linked list is a feature which
 *  allows to transfer set of consecutive blocks without requestor
 *  intervention after initial setup. See the reference manual for
 *  further informations.
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

#if !defined (__DMA_LL_H__)
#define __DMA_LL_H__

#if defined(__cplusplus)
extern "C" {
#endif

#include "img_defs.h"
#include "img_types.h"
#ifndef VDEC_USE_PVDEC
#include "img_soc_dmac_regs_io2.h"
#include "img_soc_dmac_linked_list_io2.h"
#else /* def VDEC_USE_PVDEC */
#include "hwdefs/dmac_linked_list.h"
#include "hwdefs/dmac_regs.h"
#endif /* def VDEC_USE_PVDEC */
#include "reg_io2.h"

/*!
******************************************************************************

 This type defines the byte swap settings (see TRM "Transfer Sequence
 Linked-list - BSWAP").

******************************************************************************/
typedef enum
{
    DMAC_BSWAP_NO_SWAP = 0x0,   /*!< No byte swapping will be performed. */
    DMAC_BSWAP_REVERSE = 0x1,   /*!< Byte order will be reversed. */

} DMAC_eBSwap;

/*!
******************************************************************************

 This type defines the direction of the DMA transfer (see TRM "Transfer Sequence
 Linked-list - DIR").

******************************************************************************/
typedef enum
{
    DMAC_DIR_MEM_TO_PERI = 0x0, /*!< Data from memory to peripheral. */
    DMAC_DIR_PERI_TO_MEM = 0x1, /*!< Data from peripheral to memory. */

} DMAC_eDir;

/*!
******************************************************************************

 This type defines the peripheral width settings (see TRM "Transfer Sequence
 Linked-list - PW").

******************************************************************************/
typedef enum
{
    DMAC_PWIDTH_32_BIT = 0x0,       /*!< Peripheral width 32-bit. */
    DMAC_PWIDTH_16_BIT = 0x1,       /*!< Peripheral width 16-bit. */
    DMAC_PWIDTH_8_BIT  = 0x2,       /*!< Peripheral width 8-bit. */

} DMAC_ePW;

/*!
******************************************************************************

 This type defines whether the linked list element is last. (see TRM
 "Transfer Sequence Linked-list - List_FIN").

******************************************************************************/
typedef enum
{
    DMAC_LIST_FIN_OFF = 0,          /*!< No action. */
    DMAC_LIST_FIN_ON = 1            /*!< Generate interrupt, must be set for
                                         last element. */

} DMAC_eListFin;

/*!
******************************************************************************

 This type defines whether the peripheral address is static or
 auto-incremented.(see TRM "Transfer Sequence Linked-list - INCR").

******************************************************************************/
typedef enum
{
    DMAC_INCR_OFF = 0,          /*!< No action, no increment. */
    DMAC_INCR_ON  = 1           /*!< Generate address increment. */

} DMAC_eIncr;

/*!
******************************************************************************
 This type defines how much the peripheral address is incremented by
******************************************************************************/
typedef enum
{
    DMA_PERIPH_INCR_1    = 0x2,        //!< Increment peripheral address by 1
    DMA_PERIPH_INCR_2    = 0x1,        //!< Increment peripheral address by 2
    DMA_PERIPH_INCR_4    = 0x0,        //!< Increment peripheral address by 4

} DMA_ePeriphIncrSize;

/*!
******************************************************************************

 This type defines the access delay settings (see TRM "Transfer Sequence
 Linked-list - ACC_DEL").

******************************************************************************/
typedef enum
{
    DMAC_ACC_DEL_0      = 0x0,      /*!< Access delay zero clock cycles */
    DMAC_ACC_DEL_256    = 0x1,      /*!< Access delay 256 clock cycles */
    DMAC_ACC_DEL_512    = 0x2,      /*!< Access delay 512 clock cycles */
    DMAC_ACC_DEL_768    = 0x3,      /*!< Access delay 768 clock cycles */
    DMAC_ACC_DEL_1024   = 0x4,      /*!< Access delay 1024 clock cycles */
    DMAC_ACC_DEL_1280   = 0x5,      /*!< Access delay 1280 clock cycles */
    DMAC_ACC_DEL_1536   = 0x6,      /*!< Access delay 1536 clock cycles */
    DMAC_ACC_DEL_1792   = 0x7,      /*!< Access delay 1792 clock cycles */

} DMAC_eAccDel;

/*!
******************************************************************************

 This type defines the burst size settings (see TRM "Transfer Sequence
 Linked-list - BURST").

******************************************************************************/
typedef enum
{
    DMAC_BURST_0    = 0x0,      //!< burst size of 0
    DMAC_BURST_1    = 0x1,      //!< burst size of 1
    DMAC_BURST_2    = 0x2,      //!< burst size of 2
    DMAC_BURST_3    = 0x3,      //!< burst size of 3
    DMAC_BURST_4    = 0x4,      //!< burst size of 4
    DMAC_BURST_5    = 0x5,      //!< burst size of 5
    DMAC_BURST_6    = 0x6,      //!< burst size of 6
    DMAC_BURST_7    = 0x7,      //!< burst size of 7
    DMAC_BURST_8    = 0x8,      //!< burst size of 8

} DMAC_eBurst;


/*!
******************************************************************************

 This type defines the additional burst size settings (see TRM "Transfer
 Sequence Linked-list - EXT_BURST").

******************************************************************************/
typedef enum
{
    DMAC_EXT_BURST_0    = 0x0,      //!< no extension
    DMAC_EXT_BURST_1    = 0x1,      //!< extension of 8
    DMAC_EXT_BURST_2    = 0x2,      //!< extension of 16
    DMAC_EXT_BURST_3    = 0x3,      //!< extension of 24
    DMAC_EXT_BURST_4    = 0x4,      //!< extension of 32
    DMAC_EXT_BURST_5    = 0x5,      //!< extension of 40
    DMAC_EXT_BURST_6    = 0x6,      //!< extension of 48
    DMAC_EXT_BURST_7    = 0x7,      //!< extension of 56
    DMAC_EXT_BURST_8    = 0x8,      //!< extension of 64
    DMAC_EXT_BURST_9    = 0x9,      //!< extension of 72
    DMAC_EXT_BURST_10   = 0xa,      //!< extension of 80
    DMAC_EXT_BURST_11   = 0xb,      //!< extension of 88
    DMAC_EXT_BURST_12   = 0xc,      //!< extension of 96
    DMAC_EXT_BURST_13   = 0xd,      //!< extension of 104
    DMAC_EXT_BURST_14   = 0xe,      //!< extension of 112
    DMAC_EXT_BURST_15   = 0xf,      //!< extension of 120

} DMAC_eExtBurst;

/*!
******************************************************************************

 This type defines the 2D mode settings (see TRM "Transfer Sequence
 Linked-list - 2D_MODE").

******************************************************************************/
typedef enum
{
    DMAC_2DMODE_DISABLE     = 0x0,      /*!< Disable 2D mode */
    DMAC_2DMODE_ENABLE      = 0x1,      /*!< Enable 2D mode */

} DMAC_e2DMode;

#if defined(__cplusplus)
}
#endif

#endif /* __DMA_LL_H__ */

