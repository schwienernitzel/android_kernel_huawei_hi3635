/*!
 *****************************************************************************
 *
 * @File       pixel_api.h
 * @Description    This file contains the header information for the generic pixel manipulation library.
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

#if !defined (__PIXEL_API_H__)
#define __PIXEL_API_H__

#include <img_types.h>
#include <img_defs.h>
#include <img_errors.h>
#include <img_pixfmts.h>
#include "img_structs.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifdef DOXYGEN_CREATE_MODULES
/**
 * @defgroup IMGLIB_PIXEL Pixel API: pixel abstraction knowledge and access
 * @{
 */
/*-------------------------------------------------------------------------
 * Following elements are in the IMGLIB_PIXEL documentation module
 *------------------------------------------------------------------------*/
#endif

#define PIXEL_MAX_YUV_COMPONENTS	(32)					/**< @brief Max YUV components packet in one BOPs    */
/** @brief Return code to indicate end of image has been reached, when using Get/Set First/Next pixel functions.
 */
#define	PIXEL_END_OF_IMAGE_REACHED	(IMG_SUCCESS + 1)
#define	PIXEL_MAX_NAME_LENGTH 		30

typedef IMG_VOID * (*pixel_pfnMemoryTwiddlerFunction)(IMG_VOID *);

#define PIXEL_MULTICHROME   IMG_TRUE
#define PIXEL_MONOCHROME    IMG_FALSE
#define PIXEL_INVALID_BDC   8
#define IMG_MAX_NUM_PLANES		4
typedef struct
{
	IMG_BOOL abPlanes[IMG_MAX_NUM_PLANES];			/*! Booleans indicating which planes are in use. */
	IMG_UINT32 ui32BOPDenom;						/*! Common denominator for bytes per pixel calculation. */
	IMG_UINT32 aui32BOPNumer[IMG_MAX_NUM_PLANES];	/*! Per plane numerators for bytes per pixel calculation. */
	IMG_UINT32 ui32HDenom;							/*! Common denominator for horizontal pixel sub-sampling calculation. */
	IMG_UINT32 ui32VDenom;							/*! Common denominator for vertical pixel sub-sampling calculation. */
	IMG_UINT32 aui32HNumer[IMG_MAX_NUM_PLANES];		/*! Per plane numerators for horizontal pixel sub-sampling calculation. */
	IMG_UINT32 aui32VNumer[IMG_MAX_NUM_PLANES];		/*! Per plane numerators for vertical pixel sub-sampling calculation. */
} IMG_sPixelFormatDesc;

/*!
******************************************************************************

 @brief This type defines memory chroma interleaved order

******************************************************************************/
typedef enum
{
    PIXEL_INVALID_CI  = 0,
	PIXEL_UV_ORDER    = 1,
    PIXEL_VU_ORDER    = 2,
	PIXEL_YAYB_ORDER  = 4,
	PIXEL_AYBY_ORDER  = 8
} PIXEL_eChromaInterleaved;
/*!
******************************************************************************

 @brief This macro translates PIXEL_eChromaInterleaved values into value that can be used to write HW registers directly.

******************************************************************************/
#define PIXEL_GET_HW_CHROMA_INTERLEAVED(value)  ((value)&PIXEL_VU_ORDER ? IMG_TRUE : IMG_FALSE)

/*!
******************************************************************************

 @brief This type defines memory packing types

******************************************************************************/
typedef enum
{
    PIXEL_BIT8_MP		= 0, /**< @brief 8 bits format: no packing, whole byte is used */
	PIXEL_BIT10_MSB_MP  = 1, /**< @brief 10 bits format using most siginficant bits of a word (16b) */
    PIXEL_BIT10_LSB_MP  = 2, /**< @brief 10 bits format using least significant bits of a word (16b) */
	/**
	 * @brief 10 bits packed format using the 3 least significant 10 bits of a double word (32b)
	 *
	 * The double word is composed of 0b00CCCCCCCCCBBBBBBBBBBAAAAAAAAAA where A, B and C are the bits used to store the pixels' information.
	 */
    PIXEL_BIT10_MP		= 3
} PIXEL_eMemoryPacking;

/*!
******************************************************************************

 @brief This macro translates PIXEL_eMemoryPacking values into value that can be
 used to write HW registers directly.

******************************************************************************/
#define PIXEL_GET_HW_MEMORY_PACKING(value)   (   \
(value == PIXEL_BIT8_MP) 	   ? IMG_FALSE :     \
(value == PIXEL_BIT10_MSB_MP)  ? IMG_FALSE :     \
(value == PIXEL_BIT10_LSB_MP)  ? IMG_FALSE :     \
(value == PIXEL_BIT10_MP)      ? IMG_TRUE  :     \
IMG_FALSE)

/*!
******************************************************************************

 @brief This type defines chroma formats

******************************************************************************/
typedef enum
{
    PIXEL_FORMAT_MONO = 0,
	PIXEL_FORMAT_411  = 1,
    PIXEL_FORMAT_420  = 2,
    PIXEL_FORMAT_422  = 3,
    PIXEL_FORMAT_444  = 4,
    PIXEL_FORMAT_INVALID = 0xFF
} PIXEL_FormatIdc;

/*!
******************************************************************************

 @brief This macro translates PIXEL_FormatIdc values into value that can be
 used to write HW registers directly.

******************************************************************************/
#define PIXEL_GET_HW_CHROMA_FORMAT_IDC(value)   (   \
(value == PIXEL_FORMAT_MONO) ? 0 :                  \
(value == PIXEL_FORMAT_420)  ? 1 :                  \
(value == PIXEL_FORMAT_422)  ? 2 :                  \
(value == PIXEL_FORMAT_444)  ? 3 :                  \
PIXEL_FORMAT_INVALID )

/*!
******************************************************************************

 @brief This structure contains information about the pixel formats

******************************************************************************/
typedef struct
{
    IMG_ePixelFormat			ePixelFormat;
    PIXEL_eChromaInterleaved	eChromaInterleaved;
    IMG_BOOL					bChromaFormat;
    PIXEL_eMemoryPacking		eMemoryPacking;
    PIXEL_FormatIdc				eChromaFormatIdc;
    IMG_UINT32					ui32BitDepthY;
    IMG_UINT32					ui32BitDepthC;
    IMG_UINT32					ui32NoPlanes;
} PIXEL_sPixelInfo;

/*!
******************************************************************************

 @brief This macro can be used to fill a PIXEL control block automatically, from an
 IMG_sImageBufCB control block.

******************************************************************************/
#define	PIXEL_FILL_CONTROL_BLOCK_FROM_IMAGEBUFCB(PointerToPixelControlBlock,PointerToImageBufCB)		\
{																										\
	IMG_ASSERT ( PointerToPixelControlBlock != IMG_NULL );												\
	IMG_ASSERT ( PointerToImageBufCB != IMG_NULL );														\
																										\
	(PointerToPixelControlBlock)->ePixelColourFormat	= (PointerToImageBufCB)->ePixelFormat;			\
	(PointerToPixelControlBlock)->pvYBufBaseAddr		= (PointerToImageBufCB)->pvYBufAddr;			\
	(PointerToPixelControlBlock)->pvUVBufBaseAddr		= (PointerToImageBufCB)->pvUVBufAddr;			\
	(PointerToPixelControlBlock)->pvVBufBaseAddr		= (PointerToImageBufCB)->pvVBufAddr;			\
	(PointerToPixelControlBlock)->pvAlphaBufBaseAddr	= (PointerToImageBufCB)->pvAlphaBufAddr;		\
	(PointerToPixelControlBlock)->ui32ImageWidth		= (PointerToImageBufCB)->ui32ImageWidth;		\
	(PointerToPixelControlBlock)->ui32ImageHeight		= (PointerToImageBufCB)->ui32ImageHeight;		\
	(PointerToPixelControlBlock)->ui32YImageStride		= (PointerToImageBufCB)->ui32ImageStride;		\
}

/*!
******************************************************************************

 @brief This type defines the access mode

******************************************************************************/
typedef enum
{
    /** @brief The image in the buffer is treated as a progressive image and data
        from all lines is accessible.										*/
    PIXEL_MODE_PROGRESSIVE,
    /** @brief The image in the buffer is treated as two interlaced fields
        (field 0 and field 1), starting with the first line of
        field 0.  Only the field 0 lines are accessible. Heights should be
		specified in terms of a single field (i.e.: 'line 3' will access
		the third field 0 line, skipping all field 1 lines)					*/
    PIXEL_MODE_FIELD_0,
    /** @brief The image in the buffer is treated as two interlaced fields.
        Only the field 1 lines are accessible.	Heights should be
		specified in terms of a single field (i.e.: 'line 3' will access
		the third field 1 line, skipping all field 0 lines)					*/
    PIXEL_MODE_FIELD_1

} PIXEL_eMode;

/*!
******************************************************************************

 @brief This type defines the image in memory

******************************************************************************/
typedef struct
{
	/* Text representation of pixel type */
	#if defined PIXEL_LIB_INCLUDE_PIXEL_NAME_IN_INFO_STRUCT
	    char	acPixelFormatName [PIXEL_MAX_NAME_LENGTH];
	#endif

	/** @brief The number of pixels comprising a 'Block of pixels' (BOP), the smallest discrete element of data in this pixel colour format. */
	IMG_UINT32	ui32PixelsInBOP;
	/* All BOPs have the same number of pixels, but varying byte counts per component: */
	/** @brief The number of luma or RGB bytes in a BOP */
	IMG_UINT32	ui32YBytesInBOP;
	/** @brief Number of chrome/U bytes in a BOP (see details)
	 *
	 * The number of chroma bytes in a BOP - Only applies to pixel colour formats with a separate planar chroma component.
	 *
     * --- OR ---
	 *
     * The number of U bytes in a BOP - Only applies to pixel colour formats with a separate Y, U, V components.
	 */
	IMG_UINT32	ui32UVBytesInBOP;
	/** @brief The number of V bytes in a BOP -	Only applies to pixel colour formats with a separate Y, U, V components. */
    IMG_UINT32	ui32VBytesInBOP;
	/** @brief The number of alpha bytes in a BOP -	Only applies to pixel colour formats with a separate planar alpha component. */
	IMG_UINT32	ui32AlphaBytesInBOP;

	/*-------------------------------------------------------------------------*/

	/** @brief Set to IMG_TRUE if this buffer has a planar chroma component or has separate Y, U, V components. */
	IMG_BOOL	bIsPlanar;
	/** @brief Set to IMG_TRUE if the height (in lines) of the planar chroma component of this buffer is half the height of the planar luma component.
	 *
	 * This should only ever be non zero if 'bIsPlanar' is set to IMG_TRUE.
	 */
	IMG_BOOL	bUVHeightHalved;
	/** @brief Describes the UV stride in comparison to Y stride, multiplied by 4.
	 *
	 * Example, if UV stride is half the Y stride, then: ui32UVStrideRatioTimes4 = 0.5 * 4 = 2
	 */
    IMG_UINT32  ui32UVStrideRatioTimes4;
	/** @brief Set to IMG_TRUE if this buffer has a planar alpha component. */
	IMG_BOOL	bHasAlpha;

} PIXEL_sInfo;

/*!
******************************************************************************

 @brief This structure describes an image contained in memory.

 Sections (A) and (B) should be completed by the user before calls to PIXEL_ functions.
 In some cases, a control block should not be modified at all between PIXEL_ function calls - see individual function descriptions for details.

******************************************************************************/
typedef struct
{
	/* A.) To be set up by caller to BUF functions */
	IMG_ePixelFormat	ePixelColourFormat;		/* The pixel colour format of	*/
												/* the image in memory.			*/
	IMG_UINT32			ui32ImageWidth;			/* Image width in pixels.		*/
	IMG_UINT32          ui32ImageHeight;		/* Image height in pixels.		*/
	IMG_UINT8 *         pvYBufBaseAddr;			/* The byte address of the		*/
												/* luma / RGB data of the first */
												/* active pixel in the image.	*/
	IMG_UINT32			ui32YBufSize;			/* Size of above buffer			*/
    IMG_UINT8 *         pvUVBufBaseAddr;		/* The byte address of the		*/
												/* chroma data of the first		*/
												/* active pixel in the image.	*/
												/* This should be set to zero	*/
												/* for non planar images.		*/
	IMG_UINT32			ui32UVBufSize;			/* Size of above buffer			*/
    IMG_UINT8 *         pvVBufBaseAddr;		   /* The byte address of the V		*/
												/* chroma data of the first		*/
												/* active pixel in the image.	*/
												/* This should be set to zero	*/
												/* for non planar images.		*/
	IMG_UINT32			ui32VBufSize;			/* Size of above buffer			*/
    IMG_UINT8 *         pvAlphaBufBaseAddr;		/* The byte address of the		*/
												/* alpha data of the first		*/
												/* active pixel in the image.	*/
												/* This should be set to zero	*/
												/* for images which do not 		*/
												/* contain planar alpha.		*/
	IMG_UINT32			ui32AlphaBufSize;			/* Size of above buffer			*/
    IMG_UINT32          ui32YImageStride;		/* The stride, in bytes, of the	*/
												/* luma / RGB data of the		*/
												/* image. For planar images,	*/
												/* the chroma / alpha strides	*/
												/* are calculated automatically.*/
	PIXEL_eMode			eMode;					/* Controls how the buffer is	*/
												/* accessed by some PIXEL_		*/
												/* functions. See definition of	*/
												/* PIXEL_eMode for more details.*/

	/* B.) Twiddler function - if addresses need to be modified according to 	*/
	/* some algorithm before they are used to access memory, then this function	*/
	/* pointer should be pointed at the function which performs this operation.	*/
	/* The function must be of the form :										*/
	/*		IMG_VOID * MyFunction ( IMG_VOID * );								*/
	/*																			*/
	/* This pointer should be set to 'IMG_NULL' if no memory twiddling is		*/
	/* required.																*/
	pixel_pfnMemoryTwiddlerFunction	pfnTwiddlerFunction;

	/***************************************************************/
	/* Used internally by BUF lib - should NOT be modified by user */

	/* Copies of user values, that can be modified internally */
	IMG_UINT8 *			pvYBufAddr;
	IMG_UINT8 *			pvUVBufAddr;
	IMG_UINT8 *			pvVBufAddr;
	IMG_UINT8 *			pvAlphaBufAddr;
	IMG_UINT32          ui32InternalImageHeight;
	IMG_UINT32			ui32InternalYImageStride;

	IMG_UINT32          ui32PixelIndex;
    IMG_UINT8 *         pvLineYBufAddr;
    IMG_UINT8 *         pvLineUVBufAddr;
    IMG_UINT8 *         pvLineVBufAddr;
    IMG_UINT8 *         pvLineAlphaBufAddr;
    IMG_UINT32          ui32BufPixelNo;
    IMG_UINT32          ui32BufLineNo;
	IMG_BOOL			bEndOfImageHit;
	IMG_BOOL			bInsufficientData;

	/* Data I/O - should be filled with input data for an input function 	*/
	/* OR will contain data AFTER call to an output function.				*/
    IMG_UINT32          ui32Y[PIXEL_MAX_YUV_COMPONENTS];
    IMG_UINT32          ui32U[PIXEL_MAX_YUV_COMPONENTS];
    IMG_UINT32          ui32V[PIXEL_MAX_YUV_COMPONENTS];
    IMG_UINT32          ui32Alpha[PIXEL_MAX_YUV_COMPONENTS];
	/***************************************************************/

} PIXEL_sGetSetCB;


extern IMG_CHAR acPixelFormatIdcNames[6][16];
extern IMG_CHAR acPixelMemoryPackingNames[4][64];
extern IMG_CHAR acPixelChromaInterleavedNames[5][16];


#ifdef DOXYGEN_CREATE_MODULES
/**
 * @name Pixel knowledge functions
 * @{
 */
/*-------------------------------------------------------------------------
 * Following elements are in the pixel knowledge group
 *------------------------------------------------------------------------*/
#endif


/*
******************************************************************************

 @brief Returns the correct pixel format based on the enum IMG_ePixelFormat

 @Input	eChromaFormatIdc
 @Input eChromaInterleaved
 @Input eMemoryPacking
 @Input ui32BitDepthY
 @Input ui32BitDepthC

 @Return member of the IMG_ePixelFormat enum

******************************************************************************/
extern IMG_ePixelFormat PIXEL_GetPixelFormat(
    PIXEL_FormatIdc eChromaFormatIdc,
    PIXEL_eChromaInterleaved eChromaInterleaved,
    PIXEL_eMemoryPacking eMemoryPacking,
    IMG_UINT32 ui32BitDepthY,
    IMG_UINT32 ui32BitDepthC,
    IMG_UINT32 ui32NoPlanes
);


#ifdef DOXYGEN_CREATE_MODULES
/**
 * @}
 */
/*-------------------------------------------------------------------------
 * end of the pixel knowledge group
 *------------------------------------------------------------------------*/

/**
 * @name Pixel access
 * @{
 */
/*-------------------------------------------------------------------------
 * Following elements are in the pixel access group
 *------------------------------------------------------------------------*/
#endif


#ifdef DOXYGEN_CREATE_MODULES
/**
 * @}
 */
/*-------------------------------------------------------------------------
 * end of the pixel access group
 *------------------------------------------------------------------------*/

/**
 * @}
 */
/*-------------------------------------------------------------------------
 * end of IMGLIB_PIXEL documentation module
 *------------------------------------------------------------------------*/
#endif

#ifdef __cplusplus
}
#endif

#endif	/* __PIXEL_API_H__ */
