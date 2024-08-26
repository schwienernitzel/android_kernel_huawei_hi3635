/*!
 *****************************************************************************
 *
 * @File       securemem.c
 * @Title      Common secdev code for linux
 * @Description    This file contains the interface to a secure API for VXE/TOPAZ
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

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/version.h>
#include <asm/cacheflush.h>
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3,4,0)
  #include <asm/barrier.h>
#endif
#include <linux/mm.h>
#include <asm/io.h>

#include <img_include.h>
#include <report_api.h>

#include <securemem.h>

static IMG_VOID *
tee_map(IMG_PHYSADDR physaddr, IMG_UINT32 ui32Size)
{
    struct page **pages;
    IMG_PHYSADDR paTmp;
    pgprot_t pageProt;
    int npages;
    IMG_VOID *pvAddr;
    int i;

    npages = PAGE_ALIGN(ui32Size) / PAGE_SIZE;

    pages = vmalloc(sizeof(struct page *) * npages);
    if (pages == IMG_NULL) {
        IMG_ASSERT(pages != NULL);
        return IMG_NULL;
    }

    paTmp = physaddr;
    for (i = 0; i < npages; i++) {
        pages[i] = pfn_to_page(PFN_DOWN(paTmp));
        paTmp += PAGE_SIZE;
    }

    /* Write combined implies non-cached in Linux x86 */
    pageProt = pgprot_writecombine(PAGE_KERNEL);
#if !defined(CONFIG_X86)
    pageProt = pgprot_noncached(PAGE_KERNEL);
#endif

    pvAddr = vmap(pages, npages, VM_MAP, pageProt);
    vfree(pages);

    IMG_ASSERT(pvAddr != NULL);
    return pvAddr;
}

static IMG_VOID
tee_unmap(IMG_VOID * ptr)
{
    vunmap(ptr);
}

/*!
******************************************************************************

 @Function              SECMEM_MapSecureMemory

******************************************************************************/
IMG_RESULT 
SECMEM_MapSecureMemory(
    IMG_PHYSADDR    paPhysAddr,
    IMG_UINT32      ui32Size,
    IMG_VOID     ** ppvCpuVirt
)
{
    IMG_VOID * pvVirtAddr;

    // disabled because it fails for ION buffers
    //pvVirtAddr = ioremap(paPhysAddr, ui32Size);
    pvVirtAddr = tee_map(paPhysAddr, ui32Size);
    IMG_ASSERT(pvVirtAddr);
    if (pvVirtAddr == IMG_NULL)
    {
        REPORT(REPORT_MODULE_SECMEM, REPORT_ERR,
               "Failed to ioremap addr 0x%llx size %d",
               (unsigned long long)paPhysAddr, ui32Size);
		return IMG_ERROR_GENERIC_FAILURE;
    }
    DEBUG_REPORT(REPORT_MODULE_SECMEM, "%s CPU VIRT %p CPU PHYS %llx size %d",
                 __FUNCTION__, 
                 pvVirtAddr, (unsigned long long)paPhysAddr, ui32Size);

    *ppvCpuVirt = pvVirtAddr;

    return IMG_SUCCESS;
}

/*!
******************************************************************************

 @Function              SECMEM_UnmapSecureMemory

******************************************************************************/
IMG_VOID
SECMEM_UnmapSecureMemory(
    IMG_VOID     * pvCpuVirt
)
{
    DEBUG_REPORT(REPORT_MODULE_SECMEM, "%s cpu virt %p",
                 __FUNCTION__, pvCpuVirt);

    // disabled because it fails for ION buffers
    //iounmap(pvCpuVirt);
    tee_unmap(pvCpuVirt);
}

/*!
******************************************************************************

 @Function              SECMEM_MemoryCpuToDevice

******************************************************************************/
IMG_RESULT 
SECMEM_MemoryCpuToDevice(
    IMG_BOOL        isSecure,
    IMG_PHYSADDR    paPhysAddr
)
{

    if (!isSecure) {
        DEBUG_REPORT(REPORT_MODULE_VXDIO, "Sync non-secure memory CPU -> DEV");
#ifdef CONFIG_ARM
        /* ARM Cortex write buffer needs to be synchronised before device can access it */
        dmb();
#else
        mb();
#endif
        return IMG_SUCCESS;
    }

    /* Something more meaningfull should be done here */
    if (paPhysAddr) {
        DEBUG_REPORT(REPORT_MODULE_VXDIO, "Sync secure memory CPU -> DEV : 0x%llx",
                     (unsigned long long)paPhysAddr);
    } else {
        DEBUG_REPORT(REPORT_MODULE_VXDIO, "Sync secure memory CPU -> DEV");
    }

    return IMG_SUCCESS;
}

/*!
******************************************************************************

 @Function              SECMEM_MemoryDeviceToCpu

******************************************************************************/
IMG_RESULT 
SECMEM_MemoryDeviceToCpu(
    IMG_BOOL        isSecure,
    IMG_PHYSADDR    paPhysAddr
)
{
    /* Device memory is uncached */
    return IMG_SUCCESS;
}
