/*!
 *****************************************************************************
 *
 * @File       target.c
 * @Description    Dummy Target for PDMUP testing.
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

#include "target.h"
#include "img_errors.h"
#include "img_errors.h"
#include "tal.h"

/*!
******************************************************************************

 @Function                TARGET_LoadConfig

******************************************************************************/
static IMG_RESULT target_LoadConfig(TARGET_sTargetConfig * psTargetConfig)
{
    IMG_UINT32 ui32Result = IMG_SUCCESS;

    {
        IMG_UINT32 ui32Idx = 0;

        //Constructing Device and Memspace Lists from the static config header
        LST_init(&psTargetConfig->lDeviceItem);
        for(ui32Idx = 0; ui32Idx < psTargetConfig->ui32DevNum; ui32Idx++)
        {
            TARGET_sDeviceItem * psDeviceItem = (TARGET_sDeviceItem *)IMG_MALLOC(sizeof(TARGET_sDeviceItem));
            if (psDeviceItem == IMG_NULL)
            {
                return IMG_ERROR_MALLOC_FAILED;
            }
            IMG_MEMCPY(&psDeviceItem->sDevice, &(psTargetConfig->pasDevices[ui32Idx]), sizeof(TARGET_sDevice));
            psDeviceItem->sDevice.pszDeviceName = psTargetConfig->pasDevices[ui32Idx].pszDeviceName;
            LST_add(&psTargetConfig->lDeviceItem, psDeviceItem);
        }
        LST_init(&psTargetConfig->lMemorySpaceItem);
        for(ui32Idx = 0; ui32Idx < psTargetConfig->ui32MemSpceNum; ui32Idx++)
        {
            TARGET_sMemorySpaceItem * psMemorySpaceItem = (TARGET_sMemorySpaceItem *)IMG_MALLOC(sizeof(TARGET_sMemorySpaceItem));
            if (psMemorySpaceItem == IMG_NULL)
            {
                return IMG_ERROR_MALLOC_FAILED;
            }
            IMG_MEMCPY(&psMemorySpaceItem->sMemorySpace, &(psTargetConfig->pasMemSpces[ui32Idx]), sizeof(TARGET_sMemorySpace));
            psMemorySpaceItem->sMemorySpace.pszMemorySpaceName = psTargetConfig->pasMemSpces[ui32Idx].pszMemorySpaceName;
            LST_add(&psTargetConfig->lMemorySpaceItem, psMemorySpaceItem);
        }
        ui32Result = IMG_SUCCESS;

    }
    return ui32Result;
}

/*!
******************************************************************************

 @Function                TARGET_Initialise

 ******************************************************************************/
IMG_RESULT TARGET_Initialise(TARGET_sTargetConfig * psExtTargetConfig)
{
    TARGET_sTargetConfig * psTargetConfig; 
    IMG_UINT32 ui32Result = IMG_SUCCESS;

    psTargetConfig = psExtTargetConfig;

    ui32Result = target_LoadConfig(psTargetConfig);

    if(IMG_SUCCESS == ui32Result)
    {
        TARGET_sDeviceItem      * psDeviceItem;
        TARGET_sMemorySpaceItem * psMemorySpaceItem;

        // Register devices.
        psDeviceItem = LST_first(&psTargetConfig->lDeviceItem);
        while (IMG_NULL != psDeviceItem)
        {
            TAL_DeviceRegister(&psDeviceItem->sDevice);
            psDeviceItem = (TARGET_sDeviceItem*)LST_next(psDeviceItem);
        }

        // Setup TAL Memspaces
        psMemorySpaceItem = LST_first(&psTargetConfig->lMemorySpaceItem);
        while (IMG_NULL != psMemorySpaceItem)
        {
            TAL_MemSpaceRegister(&psMemorySpaceItem->sMemorySpace);
            psMemorySpaceItem = (TARGET_sMemorySpaceItem*)LST_next(psMemorySpaceItem);
        }
    }

    return ui32Result;
}


/*!
******************************************************************************

 @Function                TARGET_Deinitialise

******************************************************************************/
IMG_RESULT TARGET_Deinitialise(TARGET_sTargetConfig * psExtTargetConfig)
{
    IMG_UINT32 ui32Result = IMG_SUCCESS;
    TARGET_sTargetConfig * psTargetConfig;

    psTargetConfig = psExtTargetConfig;

    {
        //Always freeing the device and memspace lists
        {
            TARGET_sMemorySpaceItem * psMemorySpaceItem = LST_removeHead(&psTargetConfig->lMemorySpaceItem);
            while (psMemorySpaceItem)
            {
                //The device is just a pointer, will be freed after clearing the device list

                IMG_FREE(psMemorySpaceItem);
                psMemorySpaceItem = LST_removeHead(&psTargetConfig->lMemorySpaceItem);
            }
            {
                TARGET_sDeviceItem * psDeviceItem = LST_removeHead(&psTargetConfig->lDeviceItem);
                while (IMG_NULL != psDeviceItem)
                {
                    IMG_FREE(psDeviceItem);
                    psDeviceItem = LST_removeHead(&psTargetConfig->lDeviceItem);
                }
                IMG_ASSERT(LST_empty(&psTargetConfig->lDeviceItem));
            }
            IMG_ASSERT(LST_empty(&psTargetConfig->lMemorySpaceItem));
        }
    }
    return ui32Result;
}
