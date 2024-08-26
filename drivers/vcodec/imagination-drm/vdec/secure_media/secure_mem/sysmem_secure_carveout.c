/*!
 *****************************************************************************
 *
 * @File       sysmem_secure_carveout.c
 * @Title      Secure Memory Allocator with carveout memory
 * @Description    This file contains the interface to a secure memory allocator
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

#include <linux/mm.h>
#include <asm/io.h>
#include <linux/genalloc.h>
#include <linux/sched.h> // for task_tgid_nr

#include "img_include.h"
#include "system.h"
#include <sysmem_secure.h>
#include <sysbrg_utils.h>
#include <report_api.h>

/* Exported in vdec platform sysdev */
extern IMG_PHYSADDR gpaVdecSecureMemAddr;
extern IMG_UINT32 gui32VdecSecureMemSize;

static struct gen_pool *gpPool = IMG_NULL;

/*!
******************************************************************************

 @Function              secure_InitialiseMemory

******************************************************************************/
static IMG_RESULT secure_InitialiseMemory(
    struct SYSMEM_Heap *  psHeap
)
{
    int ret;

    gpPool = gen_pool_create(12, -1);
    IMG_ASSERT(gpPool);

    ret = gen_pool_add(gpPool, gpaVdecSecureMemAddr, gui32VdecSecureMemSize, -1);
    IMG_ASSERT(ret == 0);

    REPORT(REPORT_MODULE_SYSMEM, REPORT_INFO, "%s physical addr 0x%llx size 0x%x",
           __FUNCTION__,
           (unsigned long long)gpaVdecSecureMemAddr, gui32VdecSecureMemSize);
           
    return IMG_SUCCESS;
}

/*!
******************************************************************************

 @Function              secure_DeInitialiseMemory

******************************************************************************/
static IMG_VOID secure_DeInitialiseMemory(
    struct SYSMEM_Heap *  psHeap
)
{
    if (gen_pool_avail(gpPool) != gen_pool_size(gpPool))
        REPORT(REPORT_MODULE_SYSMEM, REPORT_WARNING, "%s with allocated memory : %zu bytes",
               __FUNCTION__, gen_pool_size(gpPool) - gen_pool_avail(gpPool));

    REPORT(REPORT_MODULE_SYSMEM, REPORT_INFO, "%s", __FUNCTION__);
    gen_pool_destroy(gpPool);
}


/*!
******************************************************************************

 @Function              secure_AllocatePages

******************************************************************************/
static IMG_RESULT secure_AllocatePages(
    struct SYSMEM_Heap *  psHeap,
    IMG_UINT32            ui32Size,
    SYSMEMU_sPages *      psPages,
    SYS_eMemAttrib        eMemAttrib
)
{
    IMG_PHYSADDR *        ppaCpuPhysAddrs;
    IMG_PHYSADDR          paCpuPhysAddr, paOffset;
    size_t                numPages, pg_i;
    size_t                physAddrArrSize;

    /* allocate memory using genpool */
    paCpuPhysAddr = gen_pool_alloc(gpPool, ui32Size);
    IMG_ASSERT(paCpuPhysAddr);
    //printk("%s ----------- %llx\n", __FUNCTION__, paCpuPhysAddr);

    /* create physical page array */
    numPages = (ui32Size + HOST_MMU_PAGE_SIZE - 1)/HOST_MMU_PAGE_SIZE;
    physAddrArrSize = sizeof(*ppaCpuPhysAddrs) * numPages;
    ppaCpuPhysAddrs = IMG_BIGORSMALL_ALLOC(physAddrArrSize);
    IMG_ASSERT(ppaCpuPhysAddrs);
    for (pg_i = 0, paOffset = 0; pg_i < numPages; paOffset += HOST_MMU_PAGE_SIZE, ++pg_i)
    {
        ppaCpuPhysAddrs[pg_i] = paCpuPhysAddr + paOffset;
    }
    psPages->ppaPhysAddr = ppaCpuPhysAddrs;

#if 0
    if (eMemAttrib & SYS_MEMATTRIB_SECURE_INPUT)
    {
        IMG_VOID *pvAddr = ioremap(paCpuPhysAddr, ui32Size);
        DEBUG_REPORT(REPORT_MODULE_SYSMEM, "%s cpu virt %p", __FUNCTION__, pvAddr);
        IMG_ASSERT(pvAddr);
        psPages->pvCpuKmAddr = pvAddr;
    }
#endif

#ifdef MAP_SECURE_INPUT_BUFFERS
    if (eMemAttrib & SYS_MEMATTRIB_SECURE_INPUT)
    {
        IMG_RESULT ui32Result = SYSBRGU_CreateMappableRegion(paCpuPhysAddr, ui32Size, eMemAttrib, psPages, &psPages->hRegHandle);
        IMG_ASSERT(ui32Result == IMG_SUCCESS);
    }
    else
#endif
    {
        psPages->hRegHandle = IMG_NULL;
    }

    DEBUG_REPORT(REPORT_MODULE_SYSMEM, "%s (secure carveout) size %u phys 0x%llx attr 0x%x",
                 __FUNCTION__, ui32Size, psPages->ppaPhysAddr[0], eMemAttrib);

    psPages->pvCpuKmAddr = IMG_NULL;
    return IMG_SUCCESS;
}

/*!
******************************************************************************

 @Function              secure_FreePages

******************************************************************************/
static IMG_VOID secure_FreePages(
    struct SYSMEM_Heap *  psHeap,
    IMG_HANDLE            hPagesHandle
)
{
    SYSMEMU_sPages *psPages;
    IMG_UINT32 ui32NoPages;
    IMG_UINT32 physAddrArrSize;

    psPages = (SYSMEMU_sPages *)hPagesHandle;

    IMG_ASSERT(!psPages->bImported);

    if (psPages->bDuplicated)
    {
        return;
    }

#ifdef MAP_SECURE_INPUT_BUFFERS
    if (psPages->eMemAttrib & SYS_MEMATTRIB_SECURE_INPUT) {
        SYSBRGU_DestroyMappableRegion(psPages->hRegHandle);
    }
#endif

    DEBUG_REPORT(REPORT_MODULE_SYSMEM, "%s (secure carveout) size %u phys 0x%llx",
                 __FUNCTION__, psPages->ui32Size, psPages->ppaPhysAddr[0]);

    gen_pool_free(gpPool, psPages->ppaPhysAddr[0], psPages->ui32Size);

    ui32NoPages = (psPages->ui32Size + (HOST_MMU_PAGE_SIZE-1)) / HOST_MMU_PAGE_SIZE;
    physAddrArrSize = sizeof(psPages->ppaPhysAddr[0]) * ui32NoPages;
    IMG_BIGORSMALL_FREE(physAddrArrSize, psPages->ppaPhysAddr);

    psPages->ppaPhysAddr = IMG_NULL;
    psPages->hRegHandle = IMG_NULL;
}

/*!
******************************************************************************

 @Function              secure_GetCpuKmAddr

******************************************************************************/
static IMG_RESULT secure_GetCpuKmAddr(
    struct SYSMEM_Heap *  psHeap,
    IMG_VOID **           ppvCpuKmAddr,
    IMG_HANDLE            hPagesHandle
)
{
#ifdef SYSBRG_BRIDGING
#if 0
    SYSMEMU_sPages *  psPages = hPagesHandle;

    if(psPages->pvCpuKmAddr == IMG_NULL)
    {
        DEBUG_REPORT(REPORT_MODULE_SYSMEM, "%s size %u phys 0x%llx attr 0x%x",
                     __FUNCTION__, psPages->ui32Size, psPages->ppaPhysAddr[0], psPages->eMemAttrib);
        IMG_VOID *pvAddr = ioremap(psPages->ppaPhysAddr[0], psPages->ui32Size);
        DEBUG_REPORT(REPORT_MODULE_SYSMEM, "%s cpu virt %p", __FUNCTION__, pvAddr);
        IMG_ASSERT(pvAddr);
        if (pvAddr == IMG_NULL)
            return IMG_ERROR_FATAL;
        psPages->pvCpuKmAddr = pvAddr;
    }

    *ppvCpuKmAddr = psPages->pvCpuKmAddr;

    return IMG_SUCCESS;
#else
    IMG_ASSERT(!"not allowed to map secure pages into host (kernel)");
    return IMG_ERROR_FATAL;
#endif
#else
    SYSMEMU_sPages *  psPages;
    
    psPages = (SYSMEMU_sPages *)hPagesHandle;

    IMG_ASSERT(0);

    *ppvCpuKmAddr = (IMG_VOID*)((IMG_UINTPTR)psPages->ppaPhysAddr[0]);

    return IMG_SUCCESS;
#endif
}

/*!
******************************************************************************

 @Function              secure_CpuKmAddrToCpuPAddr

******************************************************************************/
static IMG_PHYSADDR secure_CpuKmAddrToCpuPAddr(
    struct SYSMEM_Heap *  psHeap,
    IMG_VOID *            pvCpuKmAddr
)
{
#ifdef SYSBRG_BRIDGING
    IMG_ASSERT(0);
    return 0LL;
#else
    return (IMG_UINTPTR)pvCpuKmAddr;
#endif
}

/*!
******************************************************************************

 @Function              secure_CpuPAddrToCpuKmAddr

******************************************************************************/
static IMG_VOID * secure_CpuPAddrToCpuKmAddr(
    struct SYSMEM_Heap *  psHeap,
    IMG_PHYSADDR          paCpuPAddr
)
{
#ifdef SYSBRG_BRIDGING
    IMG_ASSERT(0);
    return IMG_NULL;
#else
    return (IMG_VOID *)((IMG_UINTPTR)paCpuPAddr);
#endif
}

/*!
******************************************************************************

 @Function              secure_UpdateMemory

******************************************************************************/
static IMG_VOID secure_UpdateMemory(
    struct SYSMEM_Heap *    psHeap,
    IMG_HANDLE              hPagesHandle,
    SYSMEM_UpdateDirection  eDir)
{
    return;
}

static int secure_map_user(SYSMEMU_sPages *psPages, IMG_VOID *priv)
{
#ifdef MAP_SECURE_INPUT_BUFFERS
	struct vm_area_struct *vma = priv;
	int ret;

    if (psPages->eMemAttrib & SYS_MEMATTRIB_SECURE_INPUT)
    {
        ret =  remap_pfn_range(vma,
                               vma->vm_start,
                               vma->vm_pgoff,
                               vma->vm_end - vma->vm_start,
                               vma->vm_page_prot
                               );
        IMG_ASSERT(ret == 0);
        DEBUG_REPORT(REPORT_MODULE_SYSMEM,
                     "%s attr 0x%x physical addr 0x%llx size %lu cpu virt 0x%lx pid %d",
                     __FUNCTION__,
                     psPages->eMemAttrib, ((unsigned long long)vma->vm_pgoff) << PAGE_SHIFT,
                     vma->vm_end - vma->vm_start, vma->vm_start,
                     task_tgid_nr(current));

        return ret;
    }
    else
#endif
    {
        IMG_ASSERT(!"not allowed to map secure pages into host (user)");
        return -EFAULT;
    }
}

static SYSMEM_Ops sSecureMemOps = {

    secure_InitialiseMemory,
    secure_DeInitialiseMemory,

    // allocation/free
    secure_AllocatePages,
    secure_FreePages,

    // import
    IMG_NULL,
    IMG_NULL,

    // translation
    secure_GetCpuKmAddr,

    secure_CpuKmAddrToCpuPAddr,
    secure_CpuPAddrToCpuKmAddr,

    // maintenance
    secure_UpdateMemory,
    IMG_NULL,

    secure_map_user,
    IMG_NULL,
};

/*!
******************************************************************************

 @Function              SECURE_AddSecureMemory

******************************************************************************/
IMG_RESULT SECURE_AddSecureMemory(
    SYSDEVU_sInfo * psSysDevInfo,
    SYS_eMemPool  * peMemPool
)
{
    IMG_RESULT ui32Result;

    ui32Result = SYSMEMU_AddMemoryHeap(&sSecureMemOps, psSysDevInfo, IMG_TRUE, IMG_NULL, peMemPool);
    IMG_ASSERT(IMG_SUCCESS == ui32Result);

    return ui32Result;
}
