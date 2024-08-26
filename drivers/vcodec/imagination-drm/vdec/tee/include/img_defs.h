/*!
 *****************************************************************************
 *
 * @File       img_defs.h
 * @Title      Base type definitions using C99 headers
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

#ifndef __IMG_DEFS__
#define __IMG_DEFS__

#include <img_types.h>
#include <img_sysdefs.h> // system specific definitions

#ifdef __cplusplus
extern "C" {
#endif

/*
 * maybe endianness could be guessed:
 * #define is_bigendian() ( ((char)1) == 0 )
 * #define BYTE_ORDER (is_bigendian() ? BIG : SMALL)
 */

/*
 * 3 different allocation
 * - default: use of SYSMALLOC and SYSFREE
 * - check: count the number of calls to malloc, calloc and free
 * - test: uses a global variable to know if malloc or calloc will fail
 */


/*
 * default use system allocation
 */
#ifndef IMG_MALLOC 
#define IMG_MALLOC(size)              IMG_SYSMALLOC(size)
#endif
#ifndef IMG_CALLOC
#define IMG_CALLOC(nelem, elem_size)  IMG_SYSCALLOC(nelem, elem_size)
#endif
#ifndef IMG_FREE
#define IMG_FREE(ptr)                 IMG_SYSFREE(ptr)
#endif
#ifndef IMG_REALLOC
#define IMG_REALLOC(ptr, size)        IMG_SYSREALLOC(ptr, size)
#endif
#ifndef IMG_STRDUP
#define IMG_STRDUP(ptr)               IMG_SYS_STRDUP(ptr)
#endif
#ifndef IMG_BIGALLOC
#define IMG_BIGALLOC(size)          IMG_SYSBIGALLOC(size)
#endif
#ifndef IMG_BIGFREE
#define IMG_BIGFREE(ptr)             IMG_SYSBIGFREE(ptr)
#endif

#define IMG_BIGORSMALL_THRESHOLD (8192)  // arbitrary size boundary: 2 pages
/*!
******************************************************************************
 
 @Function IMG_BIGORSMALL_ALLOC

 @Description

 Allocate using either IMG_MALLOC or IMG_BIGALLOC, depending on the size

 @Input ui32Size   : number of bytes to be allocated
 @Return pointer to allocated memory
 
******************************************************************************/
static IMG_INLINE void * IMG_BIGORSMALL_ALLOC(IMG_UINT32 ui32Size)
{
    if (ui32Size <= IMG_BIGORSMALL_THRESHOLD)
        return IMG_MALLOC(ui32Size);
    else
        return IMG_BIGALLOC(ui32Size);
}

/*!
******************************************************************************
 
 @Function IMG_BIGORSMALL_FREE

 @Description

 Free using either IMG_FREE or IMG_BIGFREE, depending on the size

 @Input ui32Size   : number of bytes to be allocated
 @Input ptr :        memory to be freed
 @Return none
 
******************************************************************************/
static IMG_INLINE void IMG_BIGORSMALL_FREE(IMG_UINT32 ui32Size, void* ptr)
{
    if (ui32Size <= IMG_BIGORSMALL_THRESHOLD)
        IMG_FREE(ptr);
    else
        IMG_BIGFREE(ptr);
}


/// @note maybe this should be done in a function to know if it worked with another way than just assert...
#define IMG_UINT64_TO_UINT32(ui64Check) (IMG_ASSERT(((ui64Check) >> 32) == 0), (IMG_UINT32)(ui64Check))

/**
 * @brief Maximum of two integers without branches
 *
 * From http://graphics.stanford.edu/~seander/bithacks.html#IntegerMinOrMax
 */
#define IMG_MAX_INT(x, y) ( (x) ^ ( ((x) ^ (y)) & -((x) < (y)) ) )

/**
 * @brief Minimum of two integers without branches
 *
 * From http://graphics.stanford.edu/~seander/bithacks.html#IntegerMinOrMax
 */
#define IMG_MIN_INT(x, y) ( (y) ^ ( ((x) ^ (y)) & -((x) < (y)) ) )

/// compile time assert (e.g. for enum values)
#define IMG_STATIC_ASSERT( condition, name )\
	typedef char assert_failed_ ## name [ (condition) ? 1 : -1 ];

#ifdef __cplusplus
}
#endif

#endif // __IMG_DEFS__
