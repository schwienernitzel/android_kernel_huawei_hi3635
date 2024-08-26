/*!
 *****************************************************************************
 *
 * @File       sysdev.c
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

#include <sysdev_utils.h>
#include <linux/kobject.h>
#include <linux/interrupt.h>
#include <asm/io.h>
#include <asm/page.h>
#include <linux/mm.h>
#include <linux/version.h>
#include <linux/module.h>
#include <img_defs.h>
#include <report_api.h>
#include "target.h"
#include <sysenv_api_km.h>
#include <system.h>
#include <sysmem_utils.h>
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3,2,0)
#include <linux/export.h>
#endif
#include <linux/clk.h>
#include <linux/regulator/consumer.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/of_irq.h>
#include <linux/of_address.h>

#include <linux/firmware.h>
#include <linux/file.h>
#include <linux/fs.h>

#include <vdec.h>
#include <secureapi_ree.h>

static int module_irq = -1;

IMG_VOID *gpvVdecRegAddr = IMG_NULL;
IMG_UINT32 gui32VdecRegSize = 0;

IMG_PHYSADDR gpaVdecSecureMemAddr = 0x200000 + 2772UL*1024UL*1024UL + SECURE_MEMORY_OFFSET;
IMG_UINT32 gui32VdecSecureMemSize = 128*1024*1024 - SECURE_MEMORY_OFFSET;

//Device information
static IMG_CHAR *   gpszDevName = IMG_NULL;
static SYSDEVU_sInfo *  psSysDev = IMG_NULL;

struct clk *gvdec_clk = NULL;
struct regulator_bulk_data gvdec_regulator = {0};
static IMG_BOOL gbDevDetected = IMG_FALSE;

static struct list_head fw_list;

struct endpoint_fw {
    struct firmware *fw;
    char *name;
    struct list_head head;
};

static int driver_probe(struct platform_device *pdev)
{
    struct device *dev = &pdev->dev;
    struct device_node *np = dev->of_node;
    struct resource res;
    int ret = 0;

    if (NULL == np)
    {
        printk(KERN_ERR "VDEC SYSDEV the device node is null\n");
        return -1;
    }

    module_irq =  irq_of_parse_and_map(np,0);
    DEBUG_REPORT(REPORT_MODULE_SYSDEV, "VDEC SYSDEV module_irq %d", module_irq);

    ret = of_address_to_resource(np,0,&res);
    if (ret == 0)
    {
        gui32VdecRegSize = resource_size(&res);
        DEBUG_REPORT(REPORT_MODULE_SYSDEV, "VDEC SYSDEV register CPU phys %#llx size %#x",
                     (unsigned long long)res.start, gui32VdecRegSize);
    } else {
        return -1;
    }

    gpvVdecRegAddr = of_iomap(np, 0);
    if (NULL == gpvVdecRegAddr) {
        printk(KERN_ERR "VDEC SYSDEV failed to ioremap registers\n");
        return -1;
    }
    DEBUG_REPORT(REPORT_MODULE_SYSDEV, "VDEC SYSDEV ioremap registers. virt 0x%p", gpvVdecRegAddr);

    gvdec_regulator.supply = "ldo_vdec";
    ret = regulator_bulk_get(dev, 1, &gvdec_regulator);
    if (ret) {
        printk(KERN_ERR "VDEC SYSDEV couldn't get regulators %d\n", ret);
        return -1;
    }

    gvdec_clk = of_clk_get(np,0);
    if (IS_ERR(gvdec_clk))
    {
        printk(KERN_ERR "VDEC SYSDEV get venc clock failed\n");
        ret =  PTR_ERR(gvdec_clk);
        regulator_put(gvdec_regulator.consumer);
        memset(&gvdec_regulator,0,sizeof(gvdec_regulator));
        return -1;
    }

    psSysDev->native_device = (void *)&pdev->dev;

    gbDevDetected = IMG_TRUE;
    return 0;
}

static const struct of_device_id vdec_of_match[] = {
    { .compatible = "hisi,k3v3-vdec", },
    { }
};

static struct platform_driver local_driver = {
    .probe = driver_probe,
    .driver = {
        .name = "vdec_msvxd", 
        .owner = THIS_MODULE,
        .of_match_table = vdec_of_match
     },
};

/*!
******************************************************************************

@Function IsrCb

******************************************************************************/
static irqreturn_t IsrCb(int irq, void *dev_id)
{
    IMG_BOOL  bHandled;

    if ( (psSysDev != IMG_NULL) && (psSysDev->pfnDevKmLisr != IMG_NULL) )
    {
        //Call it
        SYSOSKM_DisableInt();
        bHandled = psSysDev->pfnDevKmLisr(psSysDev->pvParam);
        SYSOSKM_EnableInt();

        //If the LISR handled the interrupt
        if (bHandled)
        {
			//Disable IRQ. Will be enabled when we service it.
			disable_irq_nosync(module_irq);
            //Signal this
            return IRQ_HANDLED;
        }
    }

    return IRQ_NONE;
}


/*!
******************************************************************************

@Function release_fw_entry

******************************************************************************/
static IMG_VOID release_fw_entry(
    struct endpoint_fw * entry
)
{
    printk(KERN_INFO "VDEC SYSDEV releasing firmware %s\n", entry->name);
    kfree(entry->name);
    release_firmware(entry->fw);
    list_del(&entry->head);
    kfree(entry);
}

/*!
******************************************************************************

@Function free_device

******************************************************************************/
static IMG_VOID free_device(SYSDEVU_sInfo *psInfo)
{
    struct endpoint_fw *entry, *tmp;

    if (IMG_TRUE == gbDevDetected)
    {
        platform_driver_unregister(&local_driver);
        gbDevDetected = IMG_FALSE;
    }
    if (NULL != gvdec_clk) {
        clk_put( gvdec_clk);
        gvdec_clk = NULL;
    }

    regulator_put(gvdec_regulator.consumer);
    memset(&gvdec_regulator,0,sizeof(gvdec_regulator));

    if(psSysDev != psInfo)
    {
        printk(KERN_ERR "VDEC SYSDEV Failed to free the Device\n");
        return;
    }

    if (gpvVdecRegAddr)
    {
        iounmap(gpvVdecRegAddr);
        DEBUG_REPORT(REPORT_MODULE_SYSDEV, "VDEC SYSDEV iounmap registers 0x%p", gpvVdecRegAddr);
        gpvVdecRegAddr = IMG_NULL;
    }

    //Free device name
    if (gpszDevName != IMG_NULL)
    {
        IMG_FREE(gpszDevName);
        gpszDevName = IMG_NULL;
    }

    if(IMG_NULL != psSysDev)
    {
        free_irq(module_irq, psSysDev);
    }

    list_for_each_entry_safe(entry, tmp, &fw_list, head)
    {
        release_fw_entry(entry);
    }

    psSysDev = IMG_NULL;

    return;
}


/*!
******************************************************************************

@Function handleResume

******************************************************************************/
static IMG_VOID handle_resume(SYSDEVU_sInfo *psInfo, IMG_BOOL forAPM)
{
    int ret = -1;    

    // printk removed for performace reasons.
    if (!forAPM) /*Add by hwx235964 at 2015.11.06*/
        printk("VXD %s,%d\n",__FUNCTION__,__LINE__);

    ret = regulator_bulk_enable(1, &(gvdec_regulator));
    if (ret)
        printk(KERN_ERR "VDEC SYSDEV failed to enable regulators %d\n", ret);

    if (gvdec_clk)
    {
        ret = clk_prepare_enable(gvdec_clk);
        if (ret)
        {
            printk(KERN_ERR "VDEC SYSDEV enable clock failed\n");
            ret = -EINVAL;
        }
    }
}


/*!
******************************************************************************

@Function handleSuspend

******************************************************************************/
static IMG_VOID handle_suspend(SYSDEVU_sInfo *psInfo, IMG_BOOL forAPM)
{
    // printk removed for performace reasons.
    if (!forAPM) /*Add by hwx235964 at 2015.11.06*/
        printk("VXD %s,%d\n",__FUNCTION__,__LINE__);

    if (gvdec_clk)
    {
        clk_disable_unprepare(gvdec_clk);
    }

    regulator_bulk_disable(1, &(gvdec_regulator));

}

#define GET_FW_ADDR(bGetPhyAddr, entry) \
    ((bGetPhyAddr) ? \
    /* fw buf is physically continuous mem as allocated with kmalloc(GFP_ATOMIC) */ \
    ((IMG_PUINT8)(IMG_UINTPTR)virt_to_phys((volatile void *)entry->fw->data)) : \
    ((IMG_PUINT8)entry->fw->data))

/*!
******************************************************************************

@Function acquire_dev_firmware hook

******************************************************************************/
static IMG_RESULT acquire_dev_firmware(
    SYSDEVU_sInfo * psInfo,
    IMG_CHAR      * fwName,
    IMG_BOOL        bGetPhyAddr,
    IMG_PUINT8    * ppFwBuf,
    IMG_UINT32    * pFwSize
)
{
    struct device * dev;
    struct endpoint_fw *entry, *tmp;

    if (!psInfo)
    {
        printk(KERN_ERR "VDEC SYSDEV SYSDEVU_sInfo pointer is null!\n");
        return IMG_ERROR_INVALID_PARAMETERS;
    }

    if (!psInfo->native_device)
    {
        printk(KERN_ERR "VDEC SYSDEV native_device pointer is null!\n");
        return IMG_ERROR_NOT_INITIALISED;
    }

    dev = (struct device *)psInfo->native_device;

    /* Check if the firmware was already acquired */
    list_for_each_entry_safe(entry, tmp, &fw_list, head)
    {
        if (!strcmp(entry->name, fwName))
        {
            *ppFwBuf = GET_FW_ADDR(bGetPhyAddr, entry);
            *pFwSize = entry->fw->size;
            return IMG_SUCCESS;
        }
    }

    printk(KERN_INFO "VDEC SYSDEV requesting firmware %s\n", fwName);

    /* Create a placeholder for endpoint firmware */
    entry = (struct endpoint_fw *)kmalloc(sizeof(*entry), GFP_KERNEL);
    if (entry == IMG_NULL)
    {
        printk(KERN_ERR "VDEC SYSDEV firmware alloc failed\n");
        return IMG_ERROR_OUT_OF_MEMORY;
    }

    entry->name = (IMG_CHAR *)kmalloc(strlen(fwName)+1, GFP_KERNEL);
    if (entry->name == IMG_NULL)
    {
        printk(KERN_ERR "VDEC SYSDEV firmware name alloc failed\n");
        kfree(entry);
        return IMG_ERROR_OUT_OF_MEMORY;
    }

    if (request_firmware((const struct firmware **)&entry->fw,
                                fwName, dev))
    {
        printk(KERN_ERR "VDEC SYSDEV failed to load firmware: %s\n", fwName);
        kfree(entry->name);
        kfree(entry);
        return IMG_ERROR_GENERIC_FAILURE;
    }

    *ppFwBuf = GET_FW_ADDR(bGetPhyAddr, entry);
    *pFwSize = entry->fw->size;

    strcpy(entry->name, fwName);

    list_add_tail(&entry->head, &fw_list);

    return IMG_SUCCESS;
}

/*!
******************************************************************************

@Function release_dev_firmware hook

******************************************************************************/
static IMG_RESULT release_dev_firmware(
    SYSDEVU_sInfo * psInfo,
    IMG_CHAR      * fwName
)
{
    struct endpoint_fw *entry, *tmp;

    list_for_each_entry_safe(entry, tmp, &fw_list, head)
    {
        if (!strcmp(entry->name, fwName))
        {
            release_fw_entry(entry);
            return IMG_SUCCESS;
        }
    }

    return IMG_ERROR_INVALID_ID;
}

/*!
******************************************************************************

@Function enable_dev_irq hook

******************************************************************************/
static IMG_RESULT enable_dev_irq(
    SYSDEVU_sInfo * psInfo
)
{
	if (!psInfo)
	{
		printk(KERN_ERR "VDEC SYSDEV SYSDEVU_sInfo pointer is null!\n");
		return IMG_ERROR_INVALID_PARAMETERS;
	}

	enable_irq(module_irq);

    return IMG_SUCCESS;
}

static struct SYSDEV_ops device_ops = {
		.free_device = free_device,

		.resume_device = handle_resume,
		.suspend_device = handle_suspend,

        .acquire_device_firmware = acquire_dev_firmware,
        .release_device_firmware = release_dev_firmware,

		.enable_device_irq = enable_dev_irq,

};

/*!
******************************************************************************

@Function    SYSDEVU_VDECRegisterDriver

******************************************************************************/
IMG_RESULT SYSDEVU_VDECRegisterDriver(
    SYSDEVU_sInfo *  psInfo
)
{
    IMG_UINT32 ui32Result = IMG_ERROR_GENERIC_FAILURE;

    if(IMG_FALSE == gbDevDetected)
    {
        psSysDev = psInfo;

		ui32Result = platform_driver_register(&local_driver);
		if (ui32Result != 0) 
		{
			return IMG_ERROR_DEVICE_NOT_FOUND;
		}
        if (request_irq(module_irq, IsrCb, 0, "vdec_device", psSysDev)) 
        {
            printk(KERN_ERR "VDEC SYSDEV failed to get IRQ\n");
            return IMG_ERROR_GENERIC_FAILURE;
        }

        gbDevDetected = IMG_TRUE;

        SYSDEVU_SetDevMap(psInfo, 0, 0, 0, 0, 0, 0, 0);

        SYSDEVU_SetDeviceOps(psInfo, &device_ops);

        ui32Result = SYSMEMKM_AddSystemMemory(psInfo, &psInfo->sMemPool);
    }

    INIT_LIST_HEAD(&fw_list);

    SECURE_REE_Init();

    return ui32Result;
}
/*!
******************************************************************************

@Function    SYSDEVU_VDECUnRegisterDriver

******************************************************************************/
IMG_RESULT SYSDEVU_VDECUnRegisterDriver(
    SYSDEVU_sInfo *  psInfo
)
{
    SECURE_REE_DeInit();
    SYSMEMU_RemoveMemoryHeap(psInfo->sMemPool);
    psInfo->ops->free_device(psInfo);
    return IMG_SUCCESS;
}
