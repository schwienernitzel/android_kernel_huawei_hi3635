/*!
 *****************************************************************************
 *
 * @File       pixel_api.c
 * @Description    This file contains generic pixel manipulation utility functions.
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


#include <img_types.h>
#include <img_defs.h>
#include <img_errors.h>
#include <img_pixfmts.h>
#include <img_structs.h>

#include <pixel_api.h>

#define NUM_OF_FORMATS 103


/**
 * @brief Pointer to the default format in the asPixelFormats array - default format is an invalid format
 * @note pointer set by initSearch()
 *
 * This pointer is also used to know if the arrays were sorted
 */
static PIXEL_sPixelInfo *pDefaultFormat = NULL;
/**
 * @brief Actual array storing the pixel formats information.
 *
 * @warning if PIXEL_USE_SORT is defined this array is sorted by pixel enum
 */
static PIXEL_sPixelInfo asPixelFormats[NUM_OF_FORMATS] =
{
  //{ePixelFormat,                  eChromaInterleaved, bChromaFormat,        eMemoryPacking,       eChromaFormatIdc,   ui32BitDepthY,  ui32BitDepthC,      ui32NoPlanes},
    {IMG_PIXFMT_PL12Y8,             PIXEL_INVALID_CI,   PIXEL_MONOCHROME,     PIXEL_BIT8_MP,        PIXEL_FORMAT_MONO,  8,              PIXEL_INVALID_BDC,  1           },
    {IMG_PIXFMT_PL12Y10,            PIXEL_INVALID_CI,   PIXEL_MONOCHROME,     PIXEL_BIT10_MP,       PIXEL_FORMAT_MONO,  10,             PIXEL_INVALID_BDC,  1           },
    {IMG_PIXFMT_PL12Y10_MSB,        PIXEL_INVALID_CI,   PIXEL_MONOCHROME,     PIXEL_BIT10_MSB_MP,   PIXEL_FORMAT_MONO,  10,             PIXEL_INVALID_BDC,  1           },
    {IMG_PIXFMT_PL12Y10_LSB,        PIXEL_INVALID_CI,   PIXEL_MONOCHROME,     PIXEL_BIT10_LSB_MP,   PIXEL_FORMAT_MONO,  10,             PIXEL_INVALID_BDC,  1           },
    {IMG_PIXFMT_PL12IMC2,           PIXEL_INVALID_CI,   PIXEL_MULTICHROME,    PIXEL_BIT8_MP,        PIXEL_FORMAT_420,   8,              8,                  2           },

    {IMG_PIXFMT_411PL111YUV8,       PIXEL_INVALID_CI,   PIXEL_MULTICHROME,    PIXEL_BIT8_MP,        PIXEL_FORMAT_411,   8,              8,                  3           },
    {IMG_PIXFMT_411PL12YUV8,        PIXEL_UV_ORDER,     PIXEL_MULTICHROME,    PIXEL_BIT8_MP,        PIXEL_FORMAT_411,   8,              8,                  2           },
    {IMG_PIXFMT_411PL12YVU8,        PIXEL_VU_ORDER,     PIXEL_MULTICHROME,    PIXEL_BIT8_MP,        PIXEL_FORMAT_411,   8,              8,                  2           },

    {IMG_PIXFMT_420PL8YUV8,         PIXEL_INVALID_CI,   PIXEL_MULTICHROME,    PIXEL_BIT8_MP,        PIXEL_FORMAT_420,   8,              8,                  3           },
    {IMG_PIXFMT_420PL8YUV10,        PIXEL_INVALID_CI,   PIXEL_MULTICHROME,    PIXEL_BIT10_MP,       PIXEL_FORMAT_420,   10,             10,                 3           },
    {IMG_PIXFMT_420PL111YUV10_MSB,  PIXEL_INVALID_CI,   PIXEL_MULTICHROME,    PIXEL_BIT10_MSB_MP,   PIXEL_FORMAT_420,   10,             10,                 3           },
    {IMG_PIXFMT_420PL111YUV10_LSB,  PIXEL_INVALID_CI,   PIXEL_MULTICHROME,    PIXEL_BIT10_LSB_MP,   PIXEL_FORMAT_420,   10,             10,                 3           },
    {IMG_PIXFMT_420PL111Y8UV10,     PIXEL_INVALID_CI,   PIXEL_MULTICHROME,    PIXEL_BIT10_MP,       PIXEL_FORMAT_420,   8,              10,                 3           },
    {IMG_PIXFMT_420PL111Y8UV10_MSB, PIXEL_INVALID_CI,   PIXEL_MULTICHROME,    PIXEL_BIT10_MSB_MP,   PIXEL_FORMAT_420,   8,              10,                 3           },
    {IMG_PIXFMT_420PL111Y8UV10_LSB, PIXEL_INVALID_CI,   PIXEL_MULTICHROME,    PIXEL_BIT10_LSB_MP,   PIXEL_FORMAT_420,   8,              10,                 3           },
    {IMG_PIXFMT_420PL111Y10UV8,     PIXEL_INVALID_CI,   PIXEL_MULTICHROME,    PIXEL_BIT10_MP,       PIXEL_FORMAT_420,   10,             8,                  3           },
    {IMG_PIXFMT_420PL111Y10UV8_MSB, PIXEL_INVALID_CI,   PIXEL_MULTICHROME,    PIXEL_BIT10_MSB_MP,   PIXEL_FORMAT_420,   10,             8,                  3           },
    {IMG_PIXFMT_420PL111Y10UV8_LSB, PIXEL_INVALID_CI,   PIXEL_MULTICHROME,    PIXEL_BIT10_LSB_MP,   PIXEL_FORMAT_420,   10,             8,                  3           },
    {IMG_PIXFMT_420PL12YUV8,        PIXEL_UV_ORDER,     PIXEL_MULTICHROME,    PIXEL_BIT8_MP,        PIXEL_FORMAT_420,   8,              8,                  2           },
    {IMG_PIXFMT_420PL12YVU8,        PIXEL_VU_ORDER,     PIXEL_MULTICHROME,    PIXEL_BIT8_MP,        PIXEL_FORMAT_420,   8,              8,                  2           },
    {IMG_PIXFMT_420PL12YUV10,       PIXEL_UV_ORDER,     PIXEL_MULTICHROME,    PIXEL_BIT10_MP,       PIXEL_FORMAT_420,   10,             10,                 2           },
    {IMG_PIXFMT_420PL12YVU10,       PIXEL_VU_ORDER,     PIXEL_MULTICHROME,    PIXEL_BIT10_MP,       PIXEL_FORMAT_420,   10,             10,                 2           },
    {IMG_PIXFMT_420PL12YUV10_MSB,   PIXEL_UV_ORDER,     PIXEL_MULTICHROME,    PIXEL_BIT10_MSB_MP,   PIXEL_FORMAT_420,   10,             10,                 2           },
    {IMG_PIXFMT_420PL12YVU10_MSB,   PIXEL_VU_ORDER,     PIXEL_MULTICHROME,    PIXEL_BIT10_MSB_MP,   PIXEL_FORMAT_420,   10,             10,                 2           },
    {IMG_PIXFMT_420PL12YUV10_LSB,   PIXEL_UV_ORDER,     PIXEL_MULTICHROME,    PIXEL_BIT10_LSB_MP,   PIXEL_FORMAT_420,   10,             10,                 2           },
    {IMG_PIXFMT_420PL12YVU10_LSB,   PIXEL_VU_ORDER,     PIXEL_MULTICHROME,    PIXEL_BIT10_LSB_MP,   PIXEL_FORMAT_420,   10,             10,                 2           },
    {IMG_PIXFMT_420PL12Y8UV10,      PIXEL_UV_ORDER,     PIXEL_MULTICHROME,    PIXEL_BIT10_MP,       PIXEL_FORMAT_420,   8,              10,                 2           },
    {IMG_PIXFMT_420PL12Y8VU10,      PIXEL_VU_ORDER,     PIXEL_MULTICHROME,    PIXEL_BIT10_MP,       PIXEL_FORMAT_420,   8,              10,                 2           },
    {IMG_PIXFMT_420PL12Y8UV10_MSB,  PIXEL_UV_ORDER,     PIXEL_MULTICHROME,    PIXEL_BIT10_MSB_MP,   PIXEL_FORMAT_420,   8,              10,                 2           },
    {IMG_PIXFMT_420PL12Y8VU10_MSB,  PIXEL_VU_ORDER,     PIXEL_MULTICHROME,    PIXEL_BIT10_MSB_MP,   PIXEL_FORMAT_420,   8,              10,                 2           },
    {IMG_PIXFMT_420PL12Y8UV10_LSB,  PIXEL_UV_ORDER,     PIXEL_MULTICHROME,    PIXEL_BIT10_LSB_MP,   PIXEL_FORMAT_420,   8,              10,                 2           },
    {IMG_PIXFMT_420PL12Y8VU10_LSB,  PIXEL_VU_ORDER,     PIXEL_MULTICHROME,    PIXEL_BIT10_LSB_MP,   PIXEL_FORMAT_420,   8,              10,                 2           },
    {IMG_PIXFMT_420PL12Y10UV8,      PIXEL_UV_ORDER,     PIXEL_MULTICHROME,    PIXEL_BIT10_MP,       PIXEL_FORMAT_420,   10,             8,                  2           },
    {IMG_PIXFMT_420PL12Y10VU8,      PIXEL_VU_ORDER,     PIXEL_MULTICHROME,    PIXEL_BIT10_MP,       PIXEL_FORMAT_420,   10,             8,                  2           },
    {IMG_PIXFMT_420PL12Y10UV8_MSB,  PIXEL_UV_ORDER,     PIXEL_MULTICHROME,    PIXEL_BIT10_MSB_MP,   PIXEL_FORMAT_420,   10,             8,                  2           },
    {IMG_PIXFMT_420PL12Y10VU8_MSB,  PIXEL_VU_ORDER,     PIXEL_MULTICHROME,    PIXEL_BIT10_MSB_MP,   PIXEL_FORMAT_420,   10,             8,                  2           },
    {IMG_PIXFMT_420PL12Y10UV8_LSB,  PIXEL_UV_ORDER,     PIXEL_MULTICHROME,    PIXEL_BIT10_LSB_MP,   PIXEL_FORMAT_420,   10,             8,                  2           },
    {IMG_PIXFMT_420PL12Y10VU8_LSB,  PIXEL_VU_ORDER,     PIXEL_MULTICHROME,    PIXEL_BIT10_LSB_MP,   PIXEL_FORMAT_420,   10,             8,                  2           },
    {IMG_PIXFMT_422PL8YUV8,         PIXEL_INVALID_CI,   PIXEL_MULTICHROME,    PIXEL_BIT8_MP,        PIXEL_FORMAT_422,   8,              8,                  3           },
    {IMG_PIXFMT_422PL8YUV10,        PIXEL_INVALID_CI,   PIXEL_MULTICHROME,    PIXEL_BIT10_MP,       PIXEL_FORMAT_422,   10,             10,                 3           },
    {IMG_PIXFMT_422PL111YUV10_MSB,  PIXEL_INVALID_CI,   PIXEL_MULTICHROME,    PIXEL_BIT10_MSB_MP,   PIXEL_FORMAT_422,   10,             10,                 3           },
    {IMG_PIXFMT_422PL111YUV10_LSB,  PIXEL_INVALID_CI,   PIXEL_MULTICHROME,    PIXEL_BIT10_LSB_MP,   PIXEL_FORMAT_422,   10,             10,                 3           },
    {IMG_PIXFMT_422PL111Y8UV10,     PIXEL_INVALID_CI,   PIXEL_MULTICHROME,    PIXEL_BIT10_MP,       PIXEL_FORMAT_422,   8,              10,                 3           },
    {IMG_PIXFMT_422PL111Y8UV10_MSB, PIXEL_INVALID_CI,   PIXEL_MULTICHROME,    PIXEL_BIT10_MSB_MP,   PIXEL_FORMAT_422,   8,              10,                 3           },
    {IMG_PIXFMT_422PL111Y8UV10_LSB, PIXEL_INVALID_CI,   PIXEL_MULTICHROME,    PIXEL_BIT10_LSB_MP,   PIXEL_FORMAT_422,   8,              10,                 3           },
    {IMG_PIXFMT_422PL111Y10UV8,     PIXEL_INVALID_CI,   PIXEL_MULTICHROME,    PIXEL_BIT10_MP,       PIXEL_FORMAT_422,   10,             8,                  3           },
    {IMG_PIXFMT_422PL111Y10UV8_MSB, PIXEL_INVALID_CI,   PIXEL_MULTICHROME,    PIXEL_BIT10_MSB_MP,   PIXEL_FORMAT_422,   10,             8,                  3           },
    {IMG_PIXFMT_422PL111Y10UV8_LSB, PIXEL_INVALID_CI,   PIXEL_MULTICHROME,    PIXEL_BIT10_LSB_MP,   PIXEL_FORMAT_422,   10,             8,                  3           },
    {IMG_PIXFMT_422PL12YUV8,        PIXEL_UV_ORDER,     PIXEL_MULTICHROME,    PIXEL_BIT8_MP,        PIXEL_FORMAT_422,   8,              8,                  2           },
    {IMG_PIXFMT_422PL12YVU8,        PIXEL_VU_ORDER,     PIXEL_MULTICHROME,    PIXEL_BIT8_MP,        PIXEL_FORMAT_422,   8,              8,                  2           },
    {IMG_PIXFMT_422PL12YUV10,       PIXEL_UV_ORDER,     PIXEL_MULTICHROME,    PIXEL_BIT10_MP,       PIXEL_FORMAT_422,   10,             10,                 2           },
    {IMG_PIXFMT_422PL12YVU10,       PIXEL_VU_ORDER,     PIXEL_MULTICHROME,    PIXEL_BIT10_MP,       PIXEL_FORMAT_422,   10,             10,                 2           },
    {IMG_PIXFMT_422PL12YUV10_MSB,   PIXEL_UV_ORDER,     PIXEL_MULTICHROME,    PIXEL_BIT10_MSB_MP,   PIXEL_FORMAT_422,   10,             10,                 2           },
    {IMG_PIXFMT_422PL12YVU10_MSB,   PIXEL_VU_ORDER,     PIXEL_MULTICHROME,    PIXEL_BIT10_MSB_MP,   PIXEL_FORMAT_422,   10,             10,                 2           },
    {IMG_PIXFMT_422PL12YUV10_LSB,   PIXEL_UV_ORDER,     PIXEL_MULTICHROME,    PIXEL_BIT10_LSB_MP,   PIXEL_FORMAT_422,   10,             10,                 2           },
    {IMG_PIXFMT_422PL12YVU10_LSB,   PIXEL_VU_ORDER,     PIXEL_MULTICHROME,    PIXEL_BIT10_LSB_MP,   PIXEL_FORMAT_422,   10,             10,                 2           },
    {IMG_PIXFMT_422PL12Y8UV10,      PIXEL_UV_ORDER,     PIXEL_MULTICHROME,    PIXEL_BIT10_MP,       PIXEL_FORMAT_422,   8,              10,                 2           },
    {IMG_PIXFMT_422PL12Y8VU10,      PIXEL_VU_ORDER,     PIXEL_MULTICHROME,    PIXEL_BIT10_MP,       PIXEL_FORMAT_422,   8,              10,                 2           },
    {IMG_PIXFMT_422PL12Y8UV10_MSB,  PIXEL_UV_ORDER,     PIXEL_MULTICHROME,    PIXEL_BIT10_MSB_MP,   PIXEL_FORMAT_422,   8,              10,                 2           },
    {IMG_PIXFMT_422PL12Y8VU10_MSB,  PIXEL_VU_ORDER,     PIXEL_MULTICHROME,    PIXEL_BIT10_MSB_MP,   PIXEL_FORMAT_422,   8,              10,                 2           },
    {IMG_PIXFMT_422PL12Y8UV10_LSB,  PIXEL_UV_ORDER,     PIXEL_MULTICHROME,    PIXEL_BIT10_LSB_MP,   PIXEL_FORMAT_422,   8,              10,                 2           },
    {IMG_PIXFMT_422PL12Y8VU10_LSB,  PIXEL_VU_ORDER,     PIXEL_MULTICHROME,    PIXEL_BIT10_LSB_MP,   PIXEL_FORMAT_422,   8,              10,                 2           },
    {IMG_PIXFMT_422PL12Y10UV8,      PIXEL_UV_ORDER,     PIXEL_MULTICHROME,    PIXEL_BIT10_MP,       PIXEL_FORMAT_422,   10,             8,                  2           },
    {IMG_PIXFMT_422PL12Y10VU8,      PIXEL_VU_ORDER,     PIXEL_MULTICHROME,    PIXEL_BIT10_MP,       PIXEL_FORMAT_422,   10,             8,                  2           },
    {IMG_PIXFMT_422PL12Y10UV8_MSB,  PIXEL_UV_ORDER,     PIXEL_MULTICHROME,    PIXEL_BIT10_MSB_MP,   PIXEL_FORMAT_422,   10,             8,                  2           },
    {IMG_PIXFMT_422PL12Y10VU8_MSB,  PIXEL_VU_ORDER,     PIXEL_MULTICHROME,    PIXEL_BIT10_MSB_MP,   PIXEL_FORMAT_422,   10,             8,                  2           },
    {IMG_PIXFMT_422PL12Y10UV8_LSB,  PIXEL_UV_ORDER,     PIXEL_MULTICHROME,    PIXEL_BIT10_LSB_MP,   PIXEL_FORMAT_422,   10,             8,                  2           },
    {IMG_PIXFMT_422PL12Y10VU8_LSB,  PIXEL_VU_ORDER,     PIXEL_MULTICHROME,    PIXEL_BIT10_LSB_MP,   PIXEL_FORMAT_422,   10,             8,                  2           },
    {IMG_PIXFMT_444PL111YUV8,       PIXEL_INVALID_CI,   PIXEL_MULTICHROME,    PIXEL_BIT8_MP,        PIXEL_FORMAT_444,   8,              8,                  3           },
    {IMG_PIXFMT_444PL111YUV10,      PIXEL_INVALID_CI,   PIXEL_MULTICHROME,    PIXEL_BIT10_MP,       PIXEL_FORMAT_444,   10,             10,                 3           },
    {IMG_PIXFMT_444PL111YUV10_MSB,  PIXEL_INVALID_CI,   PIXEL_MULTICHROME,    PIXEL_BIT10_MSB_MP,   PIXEL_FORMAT_444,   10,             10,                 3           },
    {IMG_PIXFMT_444PL111YUV10_LSB,  PIXEL_INVALID_CI,   PIXEL_MULTICHROME,    PIXEL_BIT10_LSB_MP,   PIXEL_FORMAT_444,   10,             10,                 3           },
    {IMG_PIXFMT_444PL111Y8UV10,     PIXEL_INVALID_CI,   PIXEL_MULTICHROME,    PIXEL_BIT10_MP,       PIXEL_FORMAT_444,   8,              10,                 3           },
    {IMG_PIXFMT_444PL111Y8UV10_MSB, PIXEL_INVALID_CI,   PIXEL_MULTICHROME,    PIXEL_BIT10_MSB_MP,   PIXEL_FORMAT_444,   8,              10,                 3           },
    {IMG_PIXFMT_444PL111Y8UV10_LSB, PIXEL_INVALID_CI,   PIXEL_MULTICHROME,    PIXEL_BIT10_LSB_MP,   PIXEL_FORMAT_444,   8,              10,                 3           },
    {IMG_PIXFMT_444PL111Y10UV8,     PIXEL_INVALID_CI,   PIXEL_MULTICHROME,    PIXEL_BIT10_MP,       PIXEL_FORMAT_444,   10,             8,                  3           },
    {IMG_PIXFMT_444PL111Y10UV8_MSB, PIXEL_INVALID_CI,   PIXEL_MULTICHROME,    PIXEL_BIT10_MSB_MP,   PIXEL_FORMAT_444,   10,             8,                  3           },
    {IMG_PIXFMT_444PL111Y10UV8_LSB, PIXEL_INVALID_CI,   PIXEL_MULTICHROME,    PIXEL_BIT10_LSB_MP,   PIXEL_FORMAT_444,   10,             8,                  3           },
    {IMG_PIXFMT_444PL12YUV8,        PIXEL_UV_ORDER,     PIXEL_MULTICHROME,    PIXEL_BIT8_MP,        PIXEL_FORMAT_444,   8,              8,                  2           },
    {IMG_PIXFMT_444PL12YVU8,        PIXEL_VU_ORDER,     PIXEL_MULTICHROME,    PIXEL_BIT8_MP,        PIXEL_FORMAT_444,   8,              8,                  2           },
    {IMG_PIXFMT_444PL12YUV10,       PIXEL_UV_ORDER,     PIXEL_MULTICHROME,    PIXEL_BIT10_MP,       PIXEL_FORMAT_444,   10,             10,                 2           },
    {IMG_PIXFMT_444PL12YVU10,       PIXEL_VU_ORDER,     PIXEL_MULTICHROME,    PIXEL_BIT10_MP,       PIXEL_FORMAT_444,   10,             10,                 2           },
    {IMG_PIXFMT_444PL12YUV10_MSB,   PIXEL_UV_ORDER,     PIXEL_MULTICHROME,    PIXEL_BIT10_MSB_MP,   PIXEL_FORMAT_444,   10,             10,                 2           },
    {IMG_PIXFMT_444PL12YVU10_MSB,   PIXEL_VU_ORDER,     PIXEL_MULTICHROME,    PIXEL_BIT10_MSB_MP,   PIXEL_FORMAT_444,   10,             10,                 2           },
    {IMG_PIXFMT_444PL12YUV10_LSB,   PIXEL_UV_ORDER,     PIXEL_MULTICHROME,    PIXEL_BIT10_LSB_MP,   PIXEL_FORMAT_444,   10,             10,                 2           },
    {IMG_PIXFMT_444PL12YVU10_LSB,   PIXEL_VU_ORDER,     PIXEL_MULTICHROME,    PIXEL_BIT10_LSB_MP,   PIXEL_FORMAT_444,   10,             10,                 2           },
    {IMG_PIXFMT_444PL12Y8UV10,      PIXEL_UV_ORDER,     PIXEL_MULTICHROME,    PIXEL_BIT10_MP,       PIXEL_FORMAT_444,   8,              10,                 2           },
    {IMG_PIXFMT_444PL12Y8VU10,      PIXEL_VU_ORDER,     PIXEL_MULTICHROME,    PIXEL_BIT10_MP,       PIXEL_FORMAT_444,   8,              10,                 2           },
    {IMG_PIXFMT_444PL12Y8UV10_MSB,  PIXEL_UV_ORDER,     PIXEL_MULTICHROME,    PIXEL_BIT10_MSB_MP,   PIXEL_FORMAT_444,   8,              10,                 2           },
    {IMG_PIXFMT_444PL12Y8VU10_MSB,  PIXEL_VU_ORDER,     PIXEL_MULTICHROME,    PIXEL_BIT10_MSB_MP,   PIXEL_FORMAT_444,   8,              10,                 2           },
    {IMG_PIXFMT_444PL12Y8UV10_LSB,  PIXEL_UV_ORDER,     PIXEL_MULTICHROME,    PIXEL_BIT10_LSB_MP,   PIXEL_FORMAT_444,   8,              10,                 2           },
    {IMG_PIXFMT_444PL12Y8VU10_LSB,  PIXEL_VU_ORDER,     PIXEL_MULTICHROME,    PIXEL_BIT10_LSB_MP,   PIXEL_FORMAT_444,   8,              10,                 2           },
    {IMG_PIXFMT_444PL12Y10UV8,      PIXEL_UV_ORDER,     PIXEL_MULTICHROME,    PIXEL_BIT10_MP,       PIXEL_FORMAT_444,   10,             8,                  2           },
    {IMG_PIXFMT_444PL12Y10VU8,      PIXEL_VU_ORDER,     PIXEL_MULTICHROME,    PIXEL_BIT10_MP,       PIXEL_FORMAT_444,   10,             8,                  2           },
    {IMG_PIXFMT_444PL12Y10UV8_MSB,  PIXEL_UV_ORDER,     PIXEL_MULTICHROME,    PIXEL_BIT10_MSB_MP,   PIXEL_FORMAT_444,   10,             8,                  2           },
    {IMG_PIXFMT_444PL12Y10VU8_MSB,  PIXEL_VU_ORDER,     PIXEL_MULTICHROME,    PIXEL_BIT10_MSB_MP,   PIXEL_FORMAT_444,   10,             8,                  2           },
    {IMG_PIXFMT_444PL12Y10UV8_LSB,  PIXEL_UV_ORDER,     PIXEL_MULTICHROME,    PIXEL_BIT10_LSB_MP,   PIXEL_FORMAT_444,   10,             8,                  2           },
    {IMG_PIXFMT_444PL12Y10VU8_LSB,  PIXEL_VU_ORDER,     PIXEL_MULTICHROME,    PIXEL_BIT10_LSB_MP,   PIXEL_FORMAT_444,   10,             8,                  2           },
    {IMG_PIXFMT_YUYV8888,           PIXEL_UV_ORDER |
                                    PIXEL_YAYB_ORDER,   PIXEL_MULTICHROME,    PIXEL_BIT8_MP,        PIXEL_FORMAT_422,   8,              8,                  1           },
    {IMG_PIXFMT_YVYU8888,           PIXEL_VU_ORDER |
                                    PIXEL_YAYB_ORDER,   PIXEL_MULTICHROME,    PIXEL_BIT8_MP,        PIXEL_FORMAT_422,   8,              8,                  1           },
    {IMG_PIXFMT_UYVY8888,           PIXEL_UV_ORDER |
                                    PIXEL_AYBY_ORDER,   PIXEL_MULTICHROME,    PIXEL_BIT8_MP,        PIXEL_FORMAT_422,   8,              8,                  1           },
    {IMG_PIXFMT_VYUY8888,           PIXEL_VU_ORDER |
                                    PIXEL_AYBY_ORDER,   PIXEL_MULTICHROME,    PIXEL_BIT8_MP,        PIXEL_FORMAT_422,   8,              8,                  1           },

    {IMG_PIXFMT_UNDEFINED,          PIXEL_INVALID_CI,   0,                    0,                    PIXEL_FORMAT_INVALID,0,             0,                  0           }
};


/**
 * @brief Array containing string lookup of pixel format IDC.
 *
 * @warning this must be kept in step with PIXEL_FormatIdc.
 */
IMG_CHAR acPixelFormatIdcNames[6][16] =
{
    "Monochrome",
    "4:1:1",
    "4:2:0",
    "4:2:2",
    "4:4:4",
    "Invalid",
};


/**
 * @brief Array containing string lookup of pixel memory packing.
 *
 * @warning this must be kept in step with PIXEL_eMemoryPacking.
 */
IMG_CHAR acPixelMemoryPackingNames[4][64] =
{
    "8-bit (no packing)",
    "10-bit in MSB of 16b word",
    "10-bit in LSB of 16b word",
    "10-bit packed 3 to one 32b double word",
};


/**
 * @brief Array containing string lookup of pixel chroma interleaving.
 *
 * @warning this must be kept in step with PIXEL_eChromaInterleaved.
 */
IMG_CHAR acPixelChromaInterleavedNames[5][16] =
{
    "Invalid",
    "UV order",
    "VU order",
    "YAYB order",
    "AYBY order",
};


static int PIXEL_ComparePixelFormats(const void * inA, const void * inB)
{
    return  ((PIXEL_sPixelInfo*)inA)->ePixelFormat - ((PIXEL_sPixelInfo*)inB)->ePixelFormat;
}

/**
 * @brief Search a pixel format based on its attributes rather than its format enum
 * @warning use PIXEL_ComparePixelFormats to search by enum
 */
static int PIXEL_ComparePixelInfo(
    const void * inA,
    const void * inB
)
{
    int result = 0;
    const PIXEL_sPixelInfo *fmtA = (PIXEL_sPixelInfo*)inA;
    const PIXEL_sPixelInfo *fmtB = (PIXEL_sPixelInfo*)inB;

    //if ( (result = fmtA->ePixelFormat - fmtB->ePixelFormat) != 0 ) return result;

    if( (result = fmtA->eChromaFormatIdc - fmtB->eChromaFormatIdc) != 0) return result;

    if( (result = fmtA->eMemoryPacking - fmtB->eMemoryPacking) != 0) return result;

    if( (result = fmtA->eChromaInterleaved - fmtB->eChromaInterleaved) != 0) return result;

    if( (result = fmtA->ui32BitDepthY - fmtB->ui32BitDepthY) != 0) return result;

    if( (result = fmtA->ui32BitDepthC - fmtB->ui32BitDepthC) != 0) return result;

    if( (result = fmtA->ui32NoPlanes - fmtB->ui32NoPlanes) != 0) return result;

    return result;
}

static IMG_VOID pixel_InitSearch(IMG_VOID)
{
    static IMG_UINT32 ui32SearchInitialized = 0;

    ui32SearchInitialized++;
    if(1 == ui32SearchInitialized)
    {
        if(IMG_NULL == pDefaultFormat)
        {
            IMG_INT32 ui32I = 0;

            ui32I = NUM_OF_FORMATS - 1;
            while(ui32I >= 0)
            {
                if(IMG_PIXFMT_UNDEFINED == asPixelFormats[ui32I].ePixelFormat)
                {
                    pDefaultFormat = &(asPixelFormats[ui32I]);
                    break;
                }
            }
            IMG_ASSERT(IMG_NULL != pDefaultFormat);
        }
    }
    else
    {
        ui32SearchInitialized--;
    }
}

static PIXEL_sPixelInfo* pixel_SearchFormat(
    const PIXEL_sPixelInfo *key,
    IMG_BOOL8               bEnumOnly
)
{
    PIXEL_sPixelInfo *formatFound = IMG_NULL;
    int (*compar)(const void *, const void *);

    if ( bEnumOnly == IMG_TRUE )
    {
        compar = &PIXEL_ComparePixelFormats;
    }
    else
    {
        compar = &PIXEL_ComparePixelInfo;
    }

    {
        IMG_UINT32 ui32I;

        for(ui32I = 0; ui32I < NUM_OF_FORMATS; ui32I++)
        {
            if(compar(key, &asPixelFormats[ui32I]) == 0)
            {
                formatFound = &asPixelFormats[ui32I];
                break;
            }
        }
    }

    return formatFound;
}


IMG_ePixelFormat PIXEL_GetPixelFormat(
    PIXEL_FormatIdc eChromaFormatIdc,
    PIXEL_eChromaInterleaved eChromaInterleaved,
    PIXEL_eMemoryPacking eMemoryPacking,
    IMG_UINT32 ui32BitDepthY,
    IMG_UINT32 ui32BitDepthC,
    IMG_UINT32 ui32NoPlanes
)
{
    IMG_UINT32 ui32internalNoPlanes = (ui32NoPlanes == 0 || ui32NoPlanes > 4)? 2 : ui32NoPlanes;
    PIXEL_sPixelInfo key;
    PIXEL_sPixelInfo *formatFound = NULL;

    /*We want to use invalid eChromaFormatIdc*/
    //IMG_ASSERT(eChromaFormatIdc==PIXEL_FORMAT_MONO ||
             //  eChromaFormatIdc==PIXEL_FORMAT_411  ||
    //           eChromaFormatIdc==PIXEL_FORMAT_420  ||
    //           eChromaFormatIdc==PIXEL_FORMAT_422  ||
    //           eChromaFormatIdc==PIXEL_FORMAT_444 );

    if (eChromaFormatIdc!=PIXEL_FORMAT_MONO &&
        eChromaFormatIdc!=PIXEL_FORMAT_411  &&
        eChromaFormatIdc!=PIXEL_FORMAT_420  &&
        eChromaFormatIdc!=PIXEL_FORMAT_422  &&
        eChromaFormatIdc!=PIXEL_FORMAT_444)
    {
        return IMG_PIXFMT_UNDEFINED;
    }

    IMG_ASSERT(ui32BitDepthY>=8 && ui32BitDepthY<=10);   // valid bit depth 8, 9, 10, or 16/0 for 422
    if (eChromaFormatIdc!=PIXEL_FORMAT_MONO)
    {
        IMG_ASSERT(ui32BitDepthC>=8 && ui32BitDepthC<=10);   // valid bit depth 8, 9, 10, or 16/0 for 422
    }

    // valid bit depth 8, 9, 10, or 16/0 for 422
    if (ui32BitDepthY < 8 || ui32BitDepthY > 10)
    {
        return IMG_PIXFMT_UNDEFINED;
    }

    // valid bit depth 8, 9, 10, or 16/0 for 422
    if (ui32BitDepthC < 8 || ui32BitDepthC > 10)
    {
        return IMG_PIXFMT_UNDEFINED;
    }

    key.ePixelFormat = IMG_PIXFMT_UNDEFINED;
    key.eChromaFormatIdc = eChromaFormatIdc;
    key.eChromaInterleaved = eChromaInterleaved;
    key.eMemoryPacking = eMemoryPacking;
    key.ui32BitDepthY = ui32BitDepthY;
    key.ui32BitDepthC = ui32BitDepthC;
    key.ui32NoPlanes = ui32internalNoPlanes;

    //9 and 10 bits formats are handled in the same way, and there is only one entry in the PixelFormat table
    if(key.ui32BitDepthY == 9)
    {
        key.ui32BitDepthY = 10;
    }
    //9 and 10 bits formats are handled in the same way, and there is only one entry in the PixelFormat table
    if(key.ui32BitDepthC == 9)
    {
        key.ui32BitDepthC = 10;
    }

    pixel_InitSearch();

    // do not search by format
    if( (formatFound = pixel_SearchFormat(&key, IMG_FALSE)) == NULL )
    {
        return IMG_PIXFMT_UNDEFINED;
    }

    return formatFound->ePixelFormat;
}

