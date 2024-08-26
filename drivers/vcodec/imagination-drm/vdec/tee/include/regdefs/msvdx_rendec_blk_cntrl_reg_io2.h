/*!
 *****************************************************************************
 *
 * @File       msvdx_rendec_blk_cntrl_reg_io2.h
 * @Description    This file contains the MSVDX_RENDEC_BLK_CNTRL_REG_IO2_H Definitions.
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


#if !defined (__MSVDX_RENDEC_BLK_CNTRL_REG_IO2_H__)
#define __MSVDX_RENDEC_BLK_CNTRL_REG_IO2_H__

#ifdef __cplusplus
extern "C" {
#endif


#define BLOCK_CONTROL_BLOCK_HEADER_OFFSET		(0x0000)

/* BLOCK_CONTROL, BLOCK_HEADER, BLK_TYPE
*/
#define BLOCK_CONTROL_BLOCK_HEADER_BLK_TYPE_MASK		(0xFFF00000)
#define BLOCK_CONTROL_BLOCK_HEADER_BLK_TYPE_LSBMASK		(0x00000FFF)
#define BLOCK_CONTROL_BLOCK_HEADER_BLK_TYPE_SHIFT		(20)
#define BLOCK_CONTROL_BLOCK_HEADER_BLK_TYPE_LENGTH		(12)
#define BLOCK_CONTROL_BLOCK_HEADER_BLK_TYPE_DEFAULT		(0)
#define BLOCK_CONTROL_BLOCK_HEADER_BLK_TYPE_SIGNED_FIELD	(IMG_FALSE)

/* BLOCK_CONTROL, BLOCK_HEADER, BLK_MTX_TARGET
*/
#define BLOCK_CONTROL_BLOCK_HEADER_BLK_MTX_TARGET_MASK		(0x00080000)
#define BLOCK_CONTROL_BLOCK_HEADER_BLK_MTX_TARGET_LSBMASK		(0x00000001)
#define BLOCK_CONTROL_BLOCK_HEADER_BLK_MTX_TARGET_SHIFT		(19)
#define BLOCK_CONTROL_BLOCK_HEADER_BLK_MTX_TARGET_LENGTH		(1)
#define BLOCK_CONTROL_BLOCK_HEADER_BLK_MTX_TARGET_DEFAULT		(0)
#define BLOCK_CONTROL_BLOCK_HEADER_BLK_MTX_TARGET_SIGNED_FIELD	(IMG_FALSE)

/* BLOCK_CONTROL, BLOCK_HEADER, BLK_COMPRESSION
*/
#define BLOCK_CONTROL_BLOCK_HEADER_BLK_COMPRESSION_MASK		(0x00040000)
#define BLOCK_CONTROL_BLOCK_HEADER_BLK_COMPRESSION_LSBMASK		(0x00000001)
#define BLOCK_CONTROL_BLOCK_HEADER_BLK_COMPRESSION_SHIFT		(18)
#define BLOCK_CONTROL_BLOCK_HEADER_BLK_COMPRESSION_LENGTH		(1)
#define BLOCK_CONTROL_BLOCK_HEADER_BLK_COMPRESSION_DEFAULT		(0)
#define BLOCK_CONTROL_BLOCK_HEADER_BLK_COMPRESSION_SIGNED_FIELD	(IMG_FALSE)

/* BLOCK_CONTROL, BLOCK_HEADER, BLK_ENCODING_METHOD
*/
#define BLOCK_CONTROL_BLOCK_HEADER_BLK_ENCODING_METHOD_MASK		(0x00030000)
#define BLOCK_CONTROL_BLOCK_HEADER_BLK_ENCODING_METHOD_LSBMASK		(0x00000003)
#define BLOCK_CONTROL_BLOCK_HEADER_BLK_ENCODING_METHOD_SHIFT		(16)
#define BLOCK_CONTROL_BLOCK_HEADER_BLK_ENCODING_METHOD_LENGTH		(2)
#define BLOCK_CONTROL_BLOCK_HEADER_BLK_ENCODING_METHOD_DEFAULT		(0)
#define BLOCK_CONTROL_BLOCK_HEADER_BLK_ENCODING_METHOD_SIGNED_FIELD	(IMG_FALSE)

/* BLOCK_CONTROL, BLOCK_HEADER, BLK_PREFIX
*/
#define BLOCK_CONTROL_BLOCK_HEADER_BLK_PREFIX_MASK		(0x0000FE00)
#define BLOCK_CONTROL_BLOCK_HEADER_BLK_PREFIX_LSBMASK		(0x0000007F)
#define BLOCK_CONTROL_BLOCK_HEADER_BLK_PREFIX_SHIFT		(9)
#define BLOCK_CONTROL_BLOCK_HEADER_BLK_PREFIX_LENGTH		(7)
#define BLOCK_CONTROL_BLOCK_HEADER_BLK_PREFIX_DEFAULT		(0)
#define BLOCK_CONTROL_BLOCK_HEADER_BLK_PREFIX_SIGNED_FIELD	(IMG_FALSE)

/* BLOCK_CONTROL, BLOCK_HEADER, BLK_NUM_SYMBOLS_LESS1
*/
#define BLOCK_CONTROL_BLOCK_HEADER_BLK_NUM_SYMBOLS_LESS1_MASK		(0x000001FF)
#define BLOCK_CONTROL_BLOCK_HEADER_BLK_NUM_SYMBOLS_LESS1_LSBMASK		(0x000001FF)
#define BLOCK_CONTROL_BLOCK_HEADER_BLK_NUM_SYMBOLS_LESS1_SHIFT		(0)
#define BLOCK_CONTROL_BLOCK_HEADER_BLK_NUM_SYMBOLS_LESS1_LENGTH		(9)
#define BLOCK_CONTROL_BLOCK_HEADER_BLK_NUM_SYMBOLS_LESS1_DEFAULT		(0)
#define BLOCK_CONTROL_BLOCK_HEADER_BLK_NUM_SYMBOLS_LESS1_SIGNED_FIELD	(IMG_FALSE)

#define BLOCK_CONTROL_BLOCK_SEPARATOR_OFFSET		(0x0004)

/* BLOCK_CONTROL, BLOCK_SEPARATOR, BLK_SEP_PREFIX
*/
#define BLOCK_CONTROL_BLOCK_SEPARATOR_BLK_SEP_PREFIX_MASK		(0x0000FFFC)
#define BLOCK_CONTROL_BLOCK_SEPARATOR_BLK_SEP_PREFIX_LSBMASK		(0x00003FFF)
#define BLOCK_CONTROL_BLOCK_SEPARATOR_BLK_SEP_PREFIX_SHIFT		(2)
#define BLOCK_CONTROL_BLOCK_SEPARATOR_BLK_SEP_PREFIX_LENGTH		(14)
#define BLOCK_CONTROL_BLOCK_SEPARATOR_BLK_SEP_PREFIX_DEFAULT		(0)
#define BLOCK_CONTROL_BLOCK_SEPARATOR_BLK_SEP_PREFIX_SIGNED_FIELD	(IMG_FALSE)

/* BLOCK_CONTROL, BLOCK_SEPARATOR, BLK_SEP_SUFFIX
*/
#define BLOCK_CONTROL_BLOCK_SEPARATOR_BLK_SEP_SUFFIX_MASK		(0x00000003)
#define BLOCK_CONTROL_BLOCK_SEPARATOR_BLK_SEP_SUFFIX_LSBMASK		(0x00000003)
#define BLOCK_CONTROL_BLOCK_SEPARATOR_BLK_SEP_SUFFIX_SHIFT		(0)
#define BLOCK_CONTROL_BLOCK_SEPARATOR_BLK_SEP_SUFFIX_LENGTH		(2)
#define BLOCK_CONTROL_BLOCK_SEPARATOR_BLK_SEP_SUFFIX_DEFAULT		(0)
#define BLOCK_CONTROL_BLOCK_SEPARATOR_BLK_SEP_SUFFIX_SIGNED_FIELD	(IMG_FALSE)



#ifdef __cplusplus
}
#endif

#endif /* __MSVDX_RENDEC_BLK_CNTRL_REG_IO2_H__ */
