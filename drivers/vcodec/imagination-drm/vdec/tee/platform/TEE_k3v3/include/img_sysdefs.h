/*!
 *****************************************************************************
 *
 * @File       img_sysdefs.h
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

#ifndef __IMG_SYSDEFS__
#define __IMG_SYSDEFS__

#include <stddef.h>
#include <string.h>
#include <malloc.h>
#include <stdio.h>

#include "tee_mem_mgmt_api.h"
#include "tee_log.h"

#ifdef __cplusplus
extern "C" {
#endif

/* misc */

#define IMG_SIZEPR "z"

/* language abstraction */
 
/* HACK to remove linx-isums*/
#define __user		

#define IMG_CONST const
#define IMG_INLINE 
	
#define	IMG_MEMCPY(dest,src,size)	TEE_MemMove(dest,src,size)
#define IMG_MEMCMP(A,B,size)		TEE_MemCompare(A,B,size)
#define IMG_MEMMOVE(dest,src,size)	TEE_MemMove(dest,src,size)
#define	IMG_MEMSET(ptr,val,size)	TEE_MemFill(ptr,val,size)

	
#define IMG_SYSCALLOC(nelem, elem_size)		img_sys_calloc(nelem, elem_size) 

static void* img_sys_calloc(size_t num, size_t size)
{ 
    void *ptr=TEE_Malloc(num*size, 0);

    if(ptr)
    {
		TEE_MemFill(ptr, num*size, 0);
    }

    return ptr;
}


#define IMG_SYSMALLOC(size)			TEE_Malloc(size,0)
#define IMG_SYSFREE(ptr)			TEE_Free(ptr)

#define IMG_SYSBIGALLOC(size)		TEE_Malloc(size,0);
#define IMG_SYSBIGFREE(ptr)			TEE_Free(ptr)


#define IMG_SYS_STRDUP(ptr)			strdup(ptr)

#define IMG_STRCMP(A,B)				strcmp(A,B)
#define IMG_STRCPY(dest,src)		strcpy(dest,src)
#define IMG_STRNCPY(dest,src,size)	strncpy(dest,src,size)
#define IMG_STRLEN(ptr)				strlen(ptr)
 
#define IMG_ASSERT(expected) (void)( (expected) || (SLog("Assertion failed: %s, file %s, line %d\n", #expected, __FILE__, __LINE__), TEE_Panic(TEE_ERROR_BAD_STATE),0) )


#pragma warning(disable : 4996) //Temp fix for bspp.c using strncat. Needs fixing in BSPP.C

#ifdef __cplusplus
}
#endif

#endif // __IMG_SYSDEFS__
