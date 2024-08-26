/*!
 *****************************************************************************
 *
 * @File       msvdx_io.c
 * @Title      Low-level MSVDX secure hardware access component
 * @Description    This file contains functions to communicate with an MSVDX core.
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

#include "img_include.h"
#include "img_defs.h"

#include "lst.h"
#include "tal.h"

#include "report_api.h"
#include "mtxio.h"
#include "msvdx_io.h"
#include "vxd.h"
#include "vdecfw.h"
#include "target.h"
#include "dma_ll.h"
#include "vxd_msvdx.h"
#include "secure_device.h"

#include "reg_io2.h"
#include "msvdx_mtx_reg_io2.h"
#include "msvdx_vec_reg_io2.h"
#include "msvdx_vdmc_reg_io2.h"

#include <securemem.h>
#include <sysos_api.h>

//Enable test mode for fpgas that support it.
#ifdef SECURE_DEVICE_FPGA_TEST

#include "msvdx_test_3_regs.h"

//#define SECURE_CPU_REG_ACCESS
#define SECURE_MEM_ACCESS

#define SECURE_REG_ACCESS \
    ((bSecEnable << MSVDX_TEST_3_SECURE_CPU_CTRL_0_SECURE_CPU_ENABLE_SHIFT) |\
    (1 << MSVDX_TEST_3_SECURE_CPU_CTRL_0_SECURE_CPU_TRANSACTION_SHIFT))

static IMG_BOOL bSecEnable = IMG_FALSE;

#endif // SECURE_DEVICE_FPGA_TEST

#define MSVDXIO_MEMSPACE_PREFIX_MAX     4
#define MSVDXIO_MEMSPACE_NAME_MAX       60

#define MSVDXIO_MTX_CORE_MEMORY  (0x18) /*!< 0x10 for code, 0x18 for data. */
#define MSVDXIO_TIMEOUT_COUNTER  (1000) /*!< Iteration time out counter for MTX I/0. */

#define MSVDXIO_MTX_CLK_MHZ      (150)  /*!< Set to the actual clock speed the MTX is running at. */

// Limit the bank size search.
#define MSVDXIO_MTX_MIN_BANK_SIZE  256
#define MSVDXIO_MTX_MAX_BANK_SIZE  1048576

// Default register write mask
#define MSVDXIO_DEF_WR_MASK (-1)

#define DMA_ReadRegMacro(_pMemSpceCtx, channel, reg)          REGIO_READ_TABLE_REGISTER(_pMemSpceCtx, DMAC, reg, channel)
#define DMA_WriteRegMacro(_pMemSpceCtx, channel, reg, val)    REGIO_WRITE_TABLE_REGISTER(_pMemSpceCtx, DMAC, reg, channel, val)

#undef MSVDX_CORE_WriteRegister
#undef MSVDX_CORE_ReadRegister

#undef MSVDX_VEC_WriteRegister
#undef MSVDX_VEC_ReadRegister

#undef DMAC_WriteRegister
#undef DMAC_ReadRegister

#undef MSVDX_VDMC_WriteRegister
#undef MSVDX_VDMC_ReadRegister

#undef MSVDX_VDEB_WriteRegister
#undef MSVDX_VDEB_ReadRegister

#undef MSVDX_VLR_WriteRegister
#undef MSVDX_VLR_ReadRegister

#undef MTX_CORE_WriteRegister
#undef MTX_CORE_ReadRegister

#undef MSVDX_TEST_WriteRegister
#undef MSVDX_TEST_ReadRegister

#undef MSVDX_TEST_3_WriteRegister
#undef MSVDX_TEST_3_ReadRegister

#define MSVDX_CORE_WriteRegister(h, o, v) \
        MSVDXIO_SEC_WriteRegister(h, REGION_VXDSYS_REGSPACE, o, v, \
                                  MSVDXIO_DEF_WR_MASK)
#define MSVDX_CORE_ReadRegister(h, o) \
        MSVDXIO_SEC_ReadRegister(h, REGION_VXDSYS_REGSPACE, o)

#define MSVDX_VEC_WriteRegister(h, o, v) \
        MSVDXIO_SEC_WriteRegister(h, REGION_VEC_REGSPACE, o, v, \
                                  MSVDXIO_DEF_WR_MASK)
#define MSVDX_VEC_ReadRegister(h, o) \
        MSVDXIO_SEC_ReadRegister(h, REGION_VEC_REGSPACE, o)

#define DMAC_WriteRegister(h, o, v) \
        MSVDXIO_SEC_WriteRegister(h, REGION_DMAC_REGSPACE, o, v, \
                                  MSVDXIO_DEF_WR_MASK)
#define DMAC_ReadRegister(h, o) \
        MSVDXIO_SEC_ReadRegister(h, REGION_DMAC_REGSPACE, o)

#define MSVDX_VDMC_WriteRegister(h, o, v) \
        MSVDXIO_SEC_WriteRegister(h, REGION_VDMC_REGSPACE, o, v, \
                                  MSVDXIO_DEF_WR_MASK)
#define MSVDX_VDMC_ReadRegister(h, o) \
        MSVDXIO_SEC_ReadRegister(h, REGION_VDMC_REGSPACE, o)

#define MSVDX_VDEB_WriteRegister(h, o, v) \
        MSVDXIO_SEC_WriteRegister(h, REGION_VDEB_REGSPACE, o, v, \
                                  MSVDXIO_DEF_WR_MASK)
#define MSVDX_VDEB_ReadRegister(h, o) \
        MSVDXIO_SEC_ReadRegister(h, REGION_VDEB_REGSPACE, o)

#define MSVDX_VLR_WriteRegister(h, o, v) \
        MSVDXIO_SEC_WriteRegister(h, REGION_VLR_REGSPACE, o, v, \
                                  MSVDXIO_DEF_WR_MASK)
#define MSVDX_VLR_ReadRegister(h, o) \
        MSVDXIO_SEC_ReadRegister(h, REGION_VLR_REGSPACE, o)

#define MTX_CORE_WriteRegister(h, o, v) \
        MSVDXIO_SEC_WriteRegister(h, REGION_VXDMTX_REGSPACE, o, v, \
                                  MSVDXIO_DEF_WR_MASK)
#define MTX_CORE_ReadRegister(h, o) \
        MSVDXIO_SEC_ReadRegister(h, REGION_VXDMTX_REGSPACE, o)

#define MSVDX_TEST_WriteRegister(h, o, v) \
        MSVDXIO_SEC_WriteRegister(h, REGION_VXDTEST_REGSPACE, o, v, \
                                  MSVDXIO_DEF_WR_MASK)
#define MSVDX_TEST_ReadRegister(h, o) \
        MSVDXIO_SEC_ReadRegister(h, REGION_VXDTEST_REGSPACE, o)

#define MSVDX_TEST_3_WriteRegister(h, o, v) \
        MSVDXIO_SEC_WriteRegister(h, REGION_VXDTEST_3_REGSPACE, o, v, \
                                  MSVDXIO_DEF_WR_MASK)
#define MSVDX_TEST_3_ReadRegister(h, o) \
        MSVDXIO_SEC_ReadRegister(h, REGION_VXDTEST_3_REGSPACE, o, MSVDX_DEF_WR_PIPE)

#define UPDATE_DEVICE_MEM(psBufInfo) \
        SECMEM_MemoryCpuToDevice(((psBufInfo)->eMemAttrib & SYS_MEMATTRIB_SECURE) == SYS_MEMATTRIB_SECURE, \
                                 (psBufInfo)->paSecPhysAddr);

typedef struct
{
    IMG_BOOL                bInitialised;
    IMG_BOOL                bClocks;                    /*!< Clocks are on/off for the core.        */
    IMG_HANDLE              pahMemSpace[REGION_MAX];    /*!< Array of mem spaces handles            */
    IMG_UINT32              ui32MemSpaceCount;          /*!< Count of mem spaces in pahMemSpace     */

    MSVDXIO_sMTXRamInfo     sMTXRamInfo;                /*!< MTX RAM info.                          */

    MTXIO_sContext          sMsgContext;                /*!< MTXIO specific data.                   */

    VXDIO_sDdBufInfo        sSecFwBufInfo;

#ifdef POST_TEST
    IMG_BOOL                bPost;                      /*!< POST test is needed if TRUE            */
#endif
#ifdef STACK_USAGE_TEST
    IMG_BOOL                bStackUsageTest;            /*!< Stack usage test is needed if TRUE     */
#endif

} MSVDXIO_sContext;

/*!
******************************************************************************

 @Function              msvdxio_WaitForMTX

******************************************************************************/
static IMG_RESULT
msvdxio_WaitForMTX(
    const IMG_HANDLE    hContext
)
{
    MSVDXIO_sContext  * psContext = (MSVDXIO_sContext *)hContext;
    IMG_RESULT          ui32Result;

    IMG_ASSERT(psContext);
    if (hContext == IMG_NULL)
    {
        REPORT(REPORT_MODULE_VXDIO, REPORT_ERR,
               "An MSVDX context handle must be provided");
        ui32Result = IMG_ERROR_INVALID_PARAMETERS;
        goto error;
    }

    ui32Result = MSVDXIO_SEC_Poll(psContext,
                                  REGION_VXDSYS_MEMSPACE,
                                  MSVDX_CORE_CR_MTX_DEBUG_OFFSET,
                                  0,
                                  MSVDX_CORE_CR_MTX_DEBUG_CR_MTX_DBG_GPIO_OUT_MASK,
                                  MSVDXIO_POLL_EQUAL);
    IMG_ASSERT(ui32Result == IMG_SUCCESS);
    if (ui32Result != IMG_SUCCESS)
    {
        REPORT(REPORT_MODULE_VXDIO, REPORT_ERR, "Timeout waiting for MTX.");
        goto error;
    }

error:
    return ui32Result;

}


/*!
******************************************************************************

 @Function              msvdxio_MTXWriteWords

******************************************************************************/
static IMG_RESULT
ui32GetDataChecksum(
    MSVDXIO_sContext  * psContext,
    IMG_UINT32          ui32Addr,
    IMG_UINT32          ui32Size,
    IMG_UINT32          ui32MTXBankSize,
    IMG_UINT32        * pui32DataChecksum
)
{
    IMG_UINT32 ui32CalcChecksum = 0;
    IMG_UINT32 ui32Reg;
    IMG_UINT32 ui32RamId;
    IMG_UINT32 ui32MtxTimeOut;
    IMG_RESULT ui32Result = IMG_SUCCESS;

    /* Initiate read access to RAM block...*/
    ui32RamId = MSVDXIO_MTX_CORE_MEMORY + (ui32Addr / ui32MTXBankSize);
    ui32Reg = 0;
    REGIO_WRITE_FIELD(ui32Reg, MTX_CORE, CR_MTX_RAM_ACCESS_CONTROL, MTX_MCMID, ui32RamId);
    REGIO_WRITE_FIELD(ui32Reg, MTX_CORE, CR_MTX_RAM_ACCESS_CONTROL, MTX_MCM_ADDR, (ui32Addr>>2));

    if (ui32Size > 1)
    {
        REGIO_WRITE_FIELD(ui32Reg, MTX_CORE, CR_MTX_RAM_ACCESS_CONTROL, MTX_MCMAI, 1);
    }

    REGIO_WRITE_FIELD(ui32Reg, MTX_CORE, CR_MTX_RAM_ACCESS_CONTROL, MTX_MCMR, 1);
    REGIO_WRITE_REGISTER(psContext, MTX_CORE, CR_MTX_RAM_ACCESS_CONTROL, ui32Reg);

    /* Repeat for all words...*/
    while ((ui32Size > 0)
       &&  (IMG_ERROR_TIMEOUT != ui32Result))
    {
        /* Wait for read to complete...*/
        ui32MtxTimeOut = MSVDXIO_TIMEOUT_COUNTER;

        do
        {
            /* Check MTX is OK...*/
            ui32Reg = REGIO_READ_REGISTER(psContext, MTX_CORE, CR_MTX_FAULT0);
            if (ui32Reg != 0)
            {
                ui32Result = IMG_ERROR_TIMEOUT;
            }

            ui32MtxTimeOut--;
            ui32Reg = REGIO_READ_REGISTER(psContext, MTX_CORE, CR_MTX_RAM_ACCESS_STATUS);
        }
        while ((REGIO_READ_FIELD(ui32Reg, MTX_CORE, CR_MTX_RAM_ACCESS_STATUS, MTX_MTX_MCM_STAT) == 0)
           &&  (ui32MtxTimeOut != 0)
           &&  (IMG_ERROR_TIMEOUT != ui32Result));

        /* go ahead if all ok */
        if(IMG_ERROR_TIMEOUT != ui32Result)
        {

            IMG_ASSERT(ui32MtxTimeOut != 0);

            /* calculate checksum */
            ui32CalcChecksum += REGIO_READ_REGISTER(psContext, MTX_CORE, CR_MTX_RAM_ACCESS_DATA_TRANSFER);

            /* decrement remaining bytes */
            ui32Size--;

            /* If we have move from one RAM to the next...*/
            ui32Addr += 4;

            if ((MSVDXIO_MTX_CORE_MEMORY + (ui32Addr / ui32MTXBankSize)) != ui32RamId)
            {
                /* Initiate read access to RAM block...*/
                ui32RamId = (MSVDXIO_MTX_CORE_MEMORY + (ui32Addr / ui32MTXBankSize));
                REGIO_WRITE_FIELD(ui32Reg, MTX_CORE, CR_MTX_RAM_ACCESS_CONTROL, MTX_MCMID, ui32RamId);
                REGIO_WRITE_FIELD(ui32Reg, MTX_CORE, CR_MTX_RAM_ACCESS_CONTROL, MTX_MCM_ADDR, (ui32Addr>>2));

                if (ui32Size > 1)
                {
                    REGIO_WRITE_FIELD(ui32Reg, MTX_CORE, CR_MTX_RAM_ACCESS_CONTROL, MTX_MCMAI, 1);
                }

                REGIO_WRITE_FIELD(ui32Reg, MTX_CORE, CR_MTX_RAM_ACCESS_CONTROL, MTX_MCMR, 1);
                REGIO_WRITE_REGISTER(psContext, MTX_CORE, CR_MTX_RAM_ACCESS_CONTROL, ui32Reg);
            }
        }
    }

    /* copy calculated value to pointed data */
    *pui32DataChecksum = ui32CalcChecksum;

    return ui32Result;
}


/*!
******************************************************************************

 @Function              msvdxio_MTXWriteWords

******************************************************************************/
static IMG_RESULT
msvdxio_MTXWriteWords(
    const IMG_HANDLE    hContext,
    IMG_UINT32          ui32Addr,         // Byte offset into MTX memory (starts at 0).
    IMG_UINT32          ui32Size,         // Number of 32-bit words to write.
    IMG_UINT32          ui32MTXBankSize,
    const IMG_UINT32  * pui32Values,
    IMG_BOOL            bDataCheckReq
)
{
    MSVDXIO_sContext  * psContext = (MSVDXIO_sContext *)hContext;
    IMG_RESULT          ui32Result = IMG_SUCCESS;
    IMG_INT32           ui32Reg;
    IMG_UINT32          ui32MtxTimeOut;
    IMG_UINT32          ui32RamId;
    IMG_UINT32          ui32AccessControl;

    IMG_UINT32          ui32WrDataChecksum = 0;
    IMG_UINT32          ui32DataChecksum = 0;
    IMG_UINT32          ui32SizeCopy = ui32Size;
    IMG_UINT32          ui32AddrCopy = ui32Addr;

    if (hContext == IMG_NULL)
    {
        REPORT(REPORT_MODULE_VXDIO, REPORT_ERR,
               "An MSVDX context handle must be provided");
        ui32Result = IMG_ERROR_INVALID_PARAMETERS;
        goto error;
    }

    if ((ui32Addr & 0x3) != 0 ||
        ui32Size == 0)
    {
        REPORT(REPORT_MODULE_VXDIO, REPORT_ERR,
               "A valid MTX RAM location (offset word aligned) must be provided");
        ui32Result = IMG_ERROR_INVALID_PARAMETERS;
        goto error;
    }

    if (pui32Values == IMG_NULL)
    {
        REPORT(REPORT_MODULE_VXDIO, REPORT_ERR,
               "A valid container for write words not provided");
        ui32Result = IMG_ERROR_INVALID_PARAMETERS;
        goto error;
    }

    /* Request the bus from the Dash...*/
    ui32Reg = 0;
    REGIO_WRITE_FIELD(ui32Reg, MSVDX_CORE, CR_MTX_DEBUG, CR_MTX_DBG_IS_SLAVE, 1);
    REGIO_WRITE_FIELD(ui32Reg, MSVDX_CORE, CR_MTX_DEBUG, CR_MTX_DBG_GPIO_IN, 0x02);
    REGIO_WRITE_REGISTER(psContext, MSVDX_CORE, CR_MTX_DEBUG, ui32Reg);
    msvdxio_WaitForMTX(psContext);

    /* Set HOST_ACCESS so Codescape knows the host is using the bus. */
    REGIO_WRITE_FIELD(ui32Reg, MSVDX_CORE, CR_MTX_DEBUG, CR_MTX_DBG_GPIO_IN, 0x03);
    REGIO_WRITE_REGISTER(psContext, MSVDX_CORE, CR_MTX_DEBUG, ui32Reg);

    /* Save the access control register...*/
    ui32AccessControl = REGIO_READ_REGISTER(psContext, MTX_CORE, CR_MTX_RAM_ACCESS_CONTROL);

    /* Initiate write access to RAM block...*/
    ui32RamId = MSVDXIO_MTX_CORE_MEMORY + (ui32Addr / ui32MTXBankSize);
    ui32Reg = 0;
    REGIO_WRITE_FIELD(ui32Reg, MTX_CORE, CR_MTX_RAM_ACCESS_CONTROL, MTX_MCMID, ui32RamId);
    REGIO_WRITE_FIELD(ui32Reg, MTX_CORE, CR_MTX_RAM_ACCESS_CONTROL, MTX_MCM_ADDR, (ui32Addr>>2));

    if (ui32Size > 1)
    {
        REGIO_WRITE_FIELD(ui32Reg, MTX_CORE, CR_MTX_RAM_ACCESS_CONTROL, MTX_MCMAI, 1);
    }

    REGIO_WRITE_REGISTER(psContext, MTX_CORE, CR_MTX_RAM_ACCESS_CONTROL, ui32Reg);

    /* Repeat for all words...*/
    while (ui32Size > 0)
    {
        ui32WrDataChecksum += (*pui32Values);
        /* Write the next word...*/
        REGIO_WRITE_REGISTER(psContext, MTX_CORE, CR_MTX_RAM_ACCESS_DATA_TRANSFER, *pui32Values);
        pui32Values++;

        /* Wait for write to complete...*/
        ui32MtxTimeOut = MSVDXIO_TIMEOUT_COUNTER;
        do
        {
             /* Check MTX is OK...*/
            ui32Reg = REGIO_READ_REGISTER(psContext, MTX_CORE, CR_MTX_FAULT0);

            if (ui32Reg != 0)
            {
                ui32Result = IMG_ERROR_TIMEOUT;
                goto ErrorExit;
            }

            ui32MtxTimeOut--;
            ui32Reg = REGIO_READ_REGISTER(psContext, MTX_CORE, CR_MTX_RAM_ACCESS_STATUS);
        }
        while ((REGIO_READ_FIELD(ui32Reg, MTX_CORE, CR_MTX_RAM_ACCESS_STATUS, MTX_MTX_MCM_STAT) == 0) &&
               (ui32MtxTimeOut != 0));

        IMG_ASSERT(ui32MtxTimeOut != 0);

        ui32Size--;

        /* If we have move from one RAM to the next...*/
        ui32Addr += 4;
        if ((MSVDXIO_MTX_CORE_MEMORY + (ui32Addr / ui32MTXBankSize)) != ui32RamId)
        {
            /* Initiate write access to RAM block...*/
            ui32RamId = (MSVDXIO_MTX_CORE_MEMORY + (ui32Addr / ui32MTXBankSize));
            ui32Reg = 0;
            REGIO_WRITE_FIELD(ui32Reg, MTX_CORE, CR_MTX_RAM_ACCESS_CONTROL, MTX_MCMID, ui32RamId);
            REGIO_WRITE_FIELD(ui32Reg, MTX_CORE, CR_MTX_RAM_ACCESS_CONTROL, MTX_MCM_ADDR, (ui32Addr>>2));
            if (ui32Size > 1)
            {
                REGIO_WRITE_FIELD(ui32Reg, MTX_CORE, CR_MTX_RAM_ACCESS_CONTROL, MTX_MCMAI, 1);
            }
            REGIO_WRITE_REGISTER(psContext, MTX_CORE, CR_MTX_RAM_ACCESS_CONTROL, ui32Reg);
        }
    }

    if(IMG_TRUE == bDataCheckReq)
    {
        ui32Result = ui32GetDataChecksum(psContext,
                                         ui32AddrCopy,
                                         ui32SizeCopy,
                                         ui32MTXBankSize,
                                         &ui32DataChecksum);
        if(ui32WrDataChecksum != ui32DataChecksum)
        {
            ui32Result = IMG_ERROR_TEST_MISMATCH;
        }
        else
        {
        }
    }
    else
    {
    }
ErrorExit:
    /* Restore the access control register...*/
    REGIO_WRITE_REGISTER(psContext, MTX_CORE, CR_MTX_RAM_ACCESS_CONTROL, ui32AccessControl);

    /* Release the bus...*/
    ui32Reg = 0;
    REGIO_WRITE_FIELD(ui32Reg, MSVDX_CORE, CR_MTX_DEBUG, CR_MTX_DBG_IS_SLAVE, 1);
    REGIO_WRITE_REGISTER(psContext, MSVDX_CORE, CR_MTX_DEBUG, ui32Reg);

    IMG_ASSERT(REGIO_READ_REGISTER(psContext, MTX_CORE, CR_MTX_FAULT0) == 0);

error:
    return ui32Result;
}


/*!
******************************************************************************

 @Function              msvdxio_MTXReadWords

******************************************************************************/
static IMG_RESULT
msvdxio_MTXReadWords(
    const IMG_HANDLE    hContext,
    IMG_UINT32          ui32Addr,         // Byte address.
    IMG_UINT32          ui32Size,         // Number of 32-bit words to read.
    IMG_UINT32          ui32MTXBankSize,
    IMG_UINT32 *        pui32Values
)
{
    MSVDXIO_sContext  * psContext = (MSVDXIO_sContext *)hContext;
    IMG_RESULT          ui32Result = IMG_SUCCESS;
    IMG_UINT32          ui32Reg;
    IMG_UINT32          ui32MtxTimeOut;
    IMG_UINT32          ui32RamId;
    IMG_UINT32          ui32AccessControl;

    if (hContext == IMG_NULL)
    {
        REPORT(REPORT_MODULE_VXDIO, REPORT_ERR,
               "An MSVDX context handle must be provided");
        ui32Result = IMG_ERROR_INVALID_PARAMETERS;
        goto error;
    }

    if ((ui32Addr & 0x3) != 0 ||
        ui32Size == 0)
    {
        REPORT(REPORT_MODULE_VXDIO, REPORT_ERR,
               "A valid MTX RAM location (offset word aligned) must be provided");
        ui32Result = IMG_ERROR_INVALID_PARAMETERS;
        goto error;
    }

    if (pui32Values == IMG_NULL)
    {
        REPORT(REPORT_MODULE_VXDIO, REPORT_ERR,
               "A valid container for read words not provided");
        ui32Result = IMG_ERROR_INVALID_PARAMETERS;
        goto error;
    }

    /* Request the bus from the Dash...*/
    ui32Reg = 0;
    REGIO_WRITE_FIELD(ui32Reg, MSVDX_CORE, CR_MTX_DEBUG, CR_MTX_DBG_IS_SLAVE, 1);
    REGIO_WRITE_FIELD(ui32Reg, MSVDX_CORE, CR_MTX_DEBUG, CR_MTX_DBG_GPIO_IN, 0x02);
    REGIO_WRITE_REGISTER(psContext, MSVDX_CORE, CR_MTX_DEBUG, ui32Reg);
    msvdxio_WaitForMTX(psContext);

    /* Set HOST_ACCESS so Codescape knows the host is using the bus. */
    REGIO_WRITE_FIELD(ui32Reg, MSVDX_CORE, CR_MTX_DEBUG, CR_MTX_DBG_GPIO_IN, 0x03);
    REGIO_WRITE_REGISTER(psContext, MSVDX_CORE, CR_MTX_DEBUG, ui32Reg);

    /* Save the access control register...*/
    ui32AccessControl = REGIO_READ_REGISTER(psContext, MTX_CORE, CR_MTX_RAM_ACCESS_CONTROL);

    /* Initiate read access to RAM block...*/
    ui32RamId = MSVDXIO_MTX_CORE_MEMORY + (ui32Addr / ui32MTXBankSize);
    ui32Reg = 0;
    REGIO_WRITE_FIELD(ui32Reg, MTX_CORE, CR_MTX_RAM_ACCESS_CONTROL, MTX_MCMID, ui32RamId);
    REGIO_WRITE_FIELD(ui32Reg, MTX_CORE, CR_MTX_RAM_ACCESS_CONTROL, MTX_MCM_ADDR, (ui32Addr>>2));

    if (ui32Size > 1)
    {
        REGIO_WRITE_FIELD(ui32Reg, MTX_CORE, CR_MTX_RAM_ACCESS_CONTROL, MTX_MCMAI, 1);
    }

    REGIO_WRITE_FIELD(ui32Reg, MTX_CORE, CR_MTX_RAM_ACCESS_CONTROL, MTX_MCMR, 1);
    REGIO_WRITE_REGISTER(psContext, MTX_CORE, CR_MTX_RAM_ACCESS_CONTROL, ui32Reg);

    /* Repeat for all words...*/
    while (ui32Size > 0)
    {
        /* Wait for read to complete...*/
        ui32MtxTimeOut = MSVDXIO_TIMEOUT_COUNTER;

        do
        {
            /* Check MTX is OK...*/
            ui32Reg = REGIO_READ_REGISTER(psContext, MTX_CORE, CR_MTX_FAULT0);
            if (ui32Reg != 0)
            {
                ui32Result = IMG_ERROR_TIMEOUT;
                goto ErrorExit;
            }

            ui32MtxTimeOut--;
            ui32Reg = REGIO_READ_REGISTER(psContext, MTX_CORE, CR_MTX_RAM_ACCESS_STATUS);
        }
        while ((REGIO_READ_FIELD(ui32Reg, MTX_CORE, CR_MTX_RAM_ACCESS_STATUS, MTX_MTX_MCM_STAT) == 0) &&
               (ui32MtxTimeOut != 0));

        IMG_ASSERT(ui32MtxTimeOut != 0);

        *pui32Values = REGIO_READ_REGISTER(psContext, MTX_CORE, CR_MTX_RAM_ACCESS_DATA_TRANSFER);

        pui32Values++;
        ui32Size--;

        /* If we have move from one RAM to the next...*/
        ui32Addr += 4;

        if ((MSVDXIO_MTX_CORE_MEMORY + (ui32Addr / ui32MTXBankSize)) != ui32RamId)
        {
            /* Initiate read access to RAM block...*/
            ui32RamId = (MSVDXIO_MTX_CORE_MEMORY + (ui32Addr / ui32MTXBankSize));
            REGIO_WRITE_FIELD(ui32Reg, MTX_CORE, CR_MTX_RAM_ACCESS_CONTROL, MTX_MCMID, ui32RamId);
            REGIO_WRITE_FIELD(ui32Reg, MTX_CORE, CR_MTX_RAM_ACCESS_CONTROL, MTX_MCM_ADDR, (ui32Addr>>2));

            if (ui32Size > 1)
            {
                REGIO_WRITE_FIELD(ui32Reg, MTX_CORE, CR_MTX_RAM_ACCESS_CONTROL, MTX_MCMAI, 1);
            }

            REGIO_WRITE_FIELD(ui32Reg, MTX_CORE, CR_MTX_RAM_ACCESS_CONTROL, MTX_MCMR, 1);
            REGIO_WRITE_REGISTER(psContext, MTX_CORE, CR_MTX_RAM_ACCESS_CONTROL, ui32Reg);
        }
    }

ErrorExit:
    /* Restore the access control register...*/
    REGIO_WRITE_REGISTER(psContext, MTX_CORE, CR_MTX_RAM_ACCESS_CONTROL, ui32AccessControl);

    /* Release the bus...*/
    ui32Reg = 0;
    REGIO_WRITE_FIELD(ui32Reg, MSVDX_CORE, CR_MTX_DEBUG, CR_MTX_DBG_IS_SLAVE, 1);
    REGIO_WRITE_REGISTER(psContext, MSVDX_CORE, CR_MTX_DEBUG, ui32Reg);

    /* Ensure the MTX is OK....*/
    IMG_ASSERT(REGIO_READ_REGISTER(psContext, MTX_CORE, CR_MTX_FAULT0) == 0);

error:
    return ui32Result;
}


/*!
******************************************************************************

 @Function              msvdxio_MTXGetRamInfo

 @Description

 This is a non-destructive test (if the bank size can be determined); however
 the MTX must not be running.

******************************************************************************/
static IMG_RESULT
msvdxio_MTXGetRamInfo(
    const IMG_HANDLE        hContext,
    MSVDXIO_sMTXRamInfo   * psMTXRamInfo
)
{
    IMG_UINT32 ui32RamInfo, ui32RamBanks;
    MSVDXIO_sContext * psContext = (MSVDXIO_sContext *)hContext;

    /* Set default RAM config. */
    psMTXRamInfo->ui32MTXRamSize  = 0;       /* measured total MTX memory size.                                   */
    psMTXRamInfo->ui32MTXBankSize = 0x8000;  /* measured MTX RAM bank size (initial safe value for FPGA: 32768).  */
    psMTXRamInfo->ui32MTXRamMask  = 0;       /* mask of valid memory physical address bits.                       */

    /* Try and read the CR_MTX_DEBUG register as if it exists it will tell us everything. */
    ui32RamInfo = REGIO_READ_REGISTER(psContext, MSVDX_CORE, CR_MTX_DEBUG);
    ui32RamBanks = REGIO_READ_FIELD(ui32RamInfo, MSVDX_CORE, CR_MTX_DEBUG, CR_MTX_RAM_BANKS);

    /* Only derive information from this register if banks > 0 (ie. if the register exists). */
    if (ui32RamBanks > 0)
    {
        IMG_UINT32 ui32LastBankSize;

        psMTXRamInfo->ui32MTXBankSize = 1 << (REGIO_READ_FIELD(ui32RamInfo, MSVDX_CORE, CR_MTX_DEBUG, CR_MTX_RAM_BANK_SIZE) + 2);
        ui32LastBankSize = 1 << (REGIO_READ_FIELD(ui32RamInfo, MSVDX_CORE, CR_MTX_DEBUG, CR_MTX_LAST_RAM_BANK_SIZE) + 2);
        psMTXRamInfo->ui32MTXRamSize = (psMTXRamInfo->ui32MTXBankSize * (ui32RamBanks - 1)) + ui32LastBankSize;
        psMTXRamInfo->ui32MTXRamMask = psMTXRamInfo->ui32MTXRamSize - 1;
        psMTXRamInfo->ui32MTXRamMask |= psMTXRamInfo->ui32MTXRamMask >> 1;
        psMTXRamInfo->ui32MTXRamMask |= psMTXRamInfo->ui32MTXRamMask >> 2;
        psMTXRamInfo->ui32MTXRamMask |= psMTXRamInfo->ui32MTXRamMask >> 4;
        psMTXRamInfo->ui32MTXRamMask |= psMTXRamInfo->ui32MTXRamMask >> 8;
        psMTXRamInfo->ui32MTXRamMask |= psMTXRamInfo->ui32MTXRamMask >> 16;
    }
    else  /* No register - probe the memory to find the bank size. */
    {
        IMG_UINT32 ui32ThisSize, ui32NextSize;
        IMG_UINT32 ui32SaveLo, ui32SaveHi;
        IMG_UINT32 ui32Detect, ui32Temp, ui32MaxRamSize;
        IMG_UINT32 ui32ValueLo = 0x12345678;
        IMG_UINT32 ui32ValueHi = 0xaa55aa55;

        /* Set the bank size large so the read and write functions always use bank 0. */
        psMTXRamInfo->ui32MTXBankSize = MSVDXIO_MTX_MAX_BANK_SIZE << 1;

        /* Save the initial value replace it with test word 1. */
        msvdxio_MTXReadWords(hContext,
                             MSVDXIO_MTX_MIN_BANK_SIZE - 4,
                             1,
                             psMTXRamInfo->ui32MTXBankSize,
                             &ui32SaveLo);
        msvdxio_MTXWriteWords(hContext,
                              MSVDXIO_MTX_MIN_BANK_SIZE - 4,
                              1,
                              psMTXRamInfo->ui32MTXBankSize,
                              &ui32ValueLo,
                              IMG_FALSE);

        for (ui32ThisSize = MSVDXIO_MTX_MIN_BANK_SIZE;
             ui32ThisSize <= MSVDXIO_MTX_MAX_BANK_SIZE;
             ui32ThisSize = ui32NextSize)
        {
            ui32NextSize = ui32ThisSize << 1;

            /* Save the next word and replace it with test word 2. */
            msvdxio_MTXReadWords(hContext,
                                 ui32NextSize - 4,
                                 1,
                                 psMTXRamInfo->ui32MTXBankSize,
                                 &ui32SaveHi);
            msvdxio_MTXWriteWords(hContext,
                                  ui32NextSize - 4,
                                  1,
                                  psMTXRamInfo->ui32MTXBankSize,
                                  &ui32ValueHi,
                                  IMG_FALSE);

            /* Read the word from the end of the current bank sized bank. */
            msvdxio_MTXReadWords(hContext,
                                 ui32ThisSize - 4,
                                 1,
                                 psMTXRamInfo->ui32MTXBankSize,
                                 &ui32Detect);

            /* Replace the lower word. */
            msvdxio_MTXWriteWords(hContext,
                                  ui32ThisSize - 4,
                                  1,
                                  psMTXRamInfo->ui32MTXBankSize,
                                  &ui32SaveLo,
                                  IMG_FALSE);

            if (ui32Detect == ui32ValueHi)
            {
                psMTXRamInfo->ui32MTXBankSize = ui32ThisSize;
                break;
            }

            /* Swap the stored information for the next iteration. */
            ui32SaveLo = ui32SaveHi;  /*  We have written back ui32SaveLo already. */
            ui32Temp = ui32ValueLo;
            ui32ValueLo = ui32ValueHi;
            ui32ValueHi = ui32Temp;
        }

        // Fail if the bank size has not been determined.
        IMG_ASSERT(psMTXRamInfo->ui32MTXBankSize <= MSVDXIO_MTX_MAX_BANK_SIZE);

        ui32MaxRamSize = psMTXRamInfo->ui32MTXBankSize * 4;
        psMTXRamInfo->ui32MTXRamMask = ui32MaxRamSize - 1;

        /* Assume all banks are fully populated. */
        psMTXRamInfo->ui32MTXRamSize = ui32MaxRamSize;
    }

    return IMG_SUCCESS;
}


#ifdef USE_MTX_DMA_LOAD

#ifdef POST_TEST
/*!
******************************************************************************

 @Function              checkDMAIsRunning

 @Description

 Function to check DMA is running.

******************************************************************************/
IMG_RESULT checkDMAIsRunning(
    MSVDXIO_sContext  * psContext
)
{
    IMG_HANDLE hContext = (IMG_HANDLE)psContext;
    IMG_RESULT ui32Result = IMG_SUCCESS;
    IMG_UINT32 ui32Reg;
    IMG_UINT16 ui16CntValue = 0;
    IMG_UINT16 ui16CntValueOld = 0;
    IMG_UINT32 ui32TimeOut = 1000;
    /* DMA channel used for transfer */
    IMG_UINT32 ui32Channel = 0;

    if(psContext->bPost == IMG_TRUE)
    {
        /* check if number of DMA transfers remaining is decreasing until timeout*/
        do
        {
            /* update old value */
            ui16CntValueOld = ui16CntValue;
            /* read DMAC_COUNT register */
            ui32Reg = REGIO_READ_TABLE_REGISTER(hContext, DMAC,
                                                DMAC_COUNT, ui32Channel);
            /* get CNT value */
            ui16CntValue = REGIO_READ_FIELD(ui32Reg, DMAC, DMAC_COUNT, CNT);
            /* decrement timeout */
            ui32TimeOut--;
        }
        while((ui16CntValue >= ui16CntValueOld) && (ui32TimeOut > 0));

        /* check */
        if(0 == ui32TimeOut)
        {
            /* test FAILED */
            REPORT(REPORT_MODULE_VXDIO, REPORT_ERR,
                    "***** POST TEE: Start DMA check FAILED");
            ui32Result = IMG_ERROR_FATAL;
        }
        else
        {
            /* test PASSED */
            REPORT(REPORT_MODULE_VXDIO, REPORT_NOTICE,
                    "***** POST TEE: Start DMA check PASSED");
            ui32Result = IMG_SUCCESS;
        }
    }
    return ui32Result;
}

#endif /* POST_TEST */

/*!
******************************************************************************

 @Function              msvdxio_sec_DMAMTX

 @Description

 Configures and starts DMA transfer.

******************************************************************************/
static IMG_RESULT
msvdxio_sec_DMAMTX(
    MSVDXIO_sContext      * psContext
)
{
    IMG_UINT32 ui32Reg = 0;
    /* DMA channel used for transfer */
    IMG_UINT32 ui32Channel = 0;
    /* Virtual address of device buffer as seen by MSVDX */
    IMG_UINT32 ui32BufDevVirtAddr = psContext->sSecFwBufInfo.ui32DevVirt;
    IMG_UINT32 ui32SizeDwords =
        psContext->sSecFwBufInfo.ui32BufSize/sizeof(IMG_UINT32);
    IMG_HANDLE hContext = (IMG_HANDLE)psContext;

    ui32Reg = 0;
    /* DMAC MEM_TAG - secure DMAC buffers */
    REGIO_WRITE_FIELD(ui32Reg, MSVDX_CORE, CR_MEM_PROT_CTRL_2,
                      CR_MEM_PROT_ID1_CTRL, 1);
    REGIO_WRITE_REGISTER(hContext, MSVDX_CORE, CR_MEM_PROT_CTRL_2,
                         ui32Reg);

    ui32Reg = 0;
    /* MMU MEM_TAG - insecure page tables */
    REGIO_WRITE_FIELD(ui32Reg, MSVDX_CORE, CR_MEM_PROT_CTRL_0,
                      CR_MEM_PROT_ID0_CTRL, 0);
    REGIO_WRITE_REGISTER(hContext, MSVDX_CORE, CR_MEM_PROT_CTRL_0,
                         ui32Reg);

    /* Before starting the transfer, set the flow control */
    ui32Reg = REGIO_READ_REGISTER(hContext, MSVDX_CORE, CR_MSVDX_CONTROL);
    REGIO_WRITE_FIELD(ui32Reg, MSVDX_CORE, CR_MSVDX_CONTROL, DMAC_CH0_SELECT, 0);
    REGIO_WRITE_REGISTER(hContext, MSVDX_CORE, CR_MSVDX_CONTROL, ui32Reg);

    /* Begin transfer. */
    ui32Reg = 0;
    REGIO_WRITE_FIELD_LITE(ui32Reg, MTX_CORE, CR_MTX_SYSC_CDMAC, ENABLE, 1);
    /* Size in 32-bit words. */
    REGIO_WRITE_FIELD_LITE(ui32Reg, MTX_CORE, CR_MTX_SYSC_CDMAC, LENGTH,
                           ui32SizeDwords);
    REGIO_WRITE_REGISTER(hContext, MTX_CORE, CR_MTX_SYSC_CDMAC, ui32Reg);

    /* Set mtx core DMA start address. */
    REGIO_WRITE_REGISTER(hContext, MTX_CORE, CR_MTX_SYSC_CDMAA, 0);

    DMA_WriteRegMacro(hContext, ui32Channel, DMAC_IRQ_STAT, 0);

    /* Specify the holdover period to use for the channel */
    ui32Reg = 0;
    REGIO_WRITE_FIELD(ui32Reg, DMAC, DMAC_PER_HOLD, PER_HOLD, 16);
    DMA_WriteRegMacro(hContext, ui32Channel, DMAC_PER_HOLD, ui32Reg);

    /* Device virtual address of buffer to DMA */
    DMA_WriteRegMacro(hContext, ui32Channel, DMAC_SETUP, ui32BufDevVirtAddr);

    /* Clear linked list status and peripheral register address registers */
    DMA_WriteRegMacro(hContext, ui32Channel, DMAC_IRQ_STAT, 0);
    ui32Reg = 0;
    REGIO_WRITE_FIELD(ui32Reg, DMAC, DMAC_PERIPH, BURST, DMAC_BURST_1);
    DMA_WriteRegMacro(hContext, ui32Channel, DMAC_PERIPH, ui32Reg);

    ui32Reg = 0;
    REGIO_WRITE_FIELD(ui32Reg, DMAC, DMAC_PERIPHERAL_ADDR, ADDR,
                      MTX_CORE_CR_MTX_SYSC_CDMAT_OFFSET);
    DMA_WriteRegMacro(hContext, ui32Channel, DMAC_PERIPHERAL_ADDR, ui32Reg);
    /* Configure the transfer */
    ui32Reg = 0;
    REGIO_WRITE_FIELD(ui32Reg, DMAC, DMAC_COUNT, TRANSFER_IEN, 1);
    REGIO_WRITE_FIELD(ui32Reg, DMAC, DMAC_COUNT, PW, DMAC_PWIDTH_32_BIT);
    REGIO_WRITE_FIELD(ui32Reg, DMAC, DMAC_COUNT, DIR, DMAC_DIR_MEM_TO_PERI);
    REGIO_WRITE_FIELD(ui32Reg, DMAC, DMAC_COUNT, PI, DMA_PERIPH_INCR_4);
    REGIO_WRITE_FIELD(ui32Reg, DMAC, DMAC_COUNT, LIST_FIN_CTL, DMAC_LIST_FIN_OFF);
    REGIO_WRITE_FIELD(ui32Reg, DMAC, DMAC_COUNT, LIST_EN, 0);
    REGIO_WRITE_FIELD(ui32Reg, DMAC, DMAC_COUNT, ENABLE_2D_MODE, DMAC_2DMODE_DISABLE);
    REGIO_WRITE_FIELD(ui32Reg, DMAC, DMAC_COUNT, CNT, ui32SizeDwords);
    DMA_WriteRegMacro(hContext, ui32Channel, DMAC_COUNT, ui32Reg);

    /* Enable the transfer */
    ui32Reg = DMA_ReadRegMacro(hContext, ui32Channel, DMAC_COUNT);
    REGIO_WRITE_FIELD(ui32Reg, DMAC, DMAC_COUNT, EN, 1);
    DMA_WriteRegMacro(hContext, ui32Channel, DMAC_COUNT, ui32Reg);

    return IMG_SUCCESS;
}

#endif  // USE_MTX_DMA_LOAD


/*!
******************************************************************************

 @Function              msvdxio_AuthenticateFirmware

******************************************************************************/
static IMG_RESULT
msvdxio_AuthenticateFirmware(
    IMG_BYTE           * pbFwBin,
    IMG_UINT32           ui32FwBinSize,
    IMG_BYTE          ** ppbAuthFwBin,
    IMG_UINT32         * pui32AuthFwBinSize
)
{
    *ppbAuthFwBin = pbFwBin;
    *pui32AuthFwBinSize = ui32FwBinSize;

    return IMG_SUCCESS;
}


/*!
******************************************************************************

 @Function              MSVDXIO_SEC_Poll

******************************************************************************/
IMG_RESULT
MSVDXIO_SEC_Poll(
    const IMG_HANDLE    hContext,
    IMG_UINT32          ui32MemRegion,
    IMG_UINT32          ui32Offset,
    IMG_UINT32          ui32RequValue,
    IMG_UINT32          ui32Enable,
    MSVDXIO_ePollMode   ePollMode
)
{
    MSVDXIO_sContext  * psContext = (MSVDXIO_sContext *)hContext;
    IMG_RESULT          ui32Result = IMG_SUCCESS;
    IMG_UINT32          ui32Wait;
    IMG_UINT32          ui32ReadCount = 0x1000UL;
    IMG_UINT32          ui32PollMode;

    if (hContext == IMG_NULL)
    {
        REPORT(REPORT_MODULE_VXDIO, REPORT_ERR,
               "An MSVDX context handle must be provided");
        ui32Result = IMG_ERROR_INVALID_PARAMETERS;
        goto error;
    }

    if (ui32MemRegion >= REGION_MAX)
    {
        REPORT(REPORT_MODULE_VXDIO, REPORT_ERR,
               "A valid VXD memory space must be provided");
        ui32Result = IMG_ERROR_INVALID_PARAMETERS;
        goto error;
    }


    switch(ePollMode)
    {
    case MSVDXIO_POLL_EQUAL:
        ui32PollMode = TAL_CHECKFUNC_ISEQUAL;
        break;
    default:
        IMG_ASSERT("Invalid poll mode" == 0);
        REPORT(REPORT_MODULE_VXDIO, REPORT_ERR, "Invalid poll mode");
        ui32Result = IMG_ERROR_INVALID_PARAMETERS;
        goto error;
    }

    ui32Wait = 0x1UL;


    if (ui32MemRegion == REGION_VXDSYS_REGSPACE &&
        (ui32Offset == MSVDX_CORE_CR_MSVDX_CONTROL_OFFSET ||
         ui32Offset == MSVDX_CORE_CR_MMU_MEM_REQ_OFFSET))
    {
        ui32ReadCount = 10000;
        ui32Wait = 100000;
    }

#ifdef SECURE_DEVICE_FPGA_TEST
    {
        IMG_UINT32 ui32Count, ui32ReadVal;
        for( ui32Count = 0; ui32Count < ui32ReadCount; ui32Count++ )
        {
#ifdef SECURE_CPU_REG_ACCESS
            TALREG_WriteWord32(TAL_GetMemSpaceHandle("C0_REG_MSVDX_TEST_3"),
                       MSVDX_TEST_3_SECURE_CPU_CTRL_0_OFFSET,
                       1 | (1 << 1));
#endif /* SECURE_CPU_REG_ACCESS */

            /* Read from the device */
            ui32Result = TALREG_ReadWord32(psContext->pahMemSpace[ui32MemRegion], ui32Offset, &ui32ReadVal);
            if (IMG_SUCCESS != ui32Result)
            {
                break;
            }

            if ((ui32ReadVal & ui32Enable) == ui32RequValue)
            {
                ui32Result = IMG_SUCCESS;
                break;
            }

            //Make the current task sleep until timeout jiffies have elapsed.
            SYSOS_SEC_uSleep(1000);
        }

        if ((IMG_SUCCESS != ui32Result) || (ui32Count >= ui32ReadCount))
        {
            ui32Result = IMG_ERROR_TIMEOUT;
        }
    }

#else /* not SECURE_DEVICE_FPGA_TEST */

    ui32Result = TALREG_Poll32(psContext->pahMemSpace[ui32MemRegion],
                               ui32Offset,
                               ui32PollMode,
                               ui32RequValue,
                               ui32Enable,
                               ui32ReadCount,
                               ui32Wait);
#endif /* not SECURE_DEVICE_FPGA_TEST */
    if (ui32Result != IMG_SUCCESS)
    {
        REPORT(REPORT_MODULE_VXDIO, REPORT_ERR, "Poll timeout");
    }

error:

    return ui32Result;
}



/*!
******************************************************************************

 @Function              MSVDXIO_SEC_GetCoreState

******************************************************************************/
IMG_RESULT
MSVDXIO_SEC_GetCoreState(
    const IMG_HANDLE    hContext,
    VXDIO_sState      * psState
)
{
    MSVDXIO_sContext  * psContext = (MSVDXIO_sContext *)hContext;
    IMG_UINT32          ui32Reg;

    if (hContext == IMG_NULL)
    {
        REPORT(REPORT_MODULE_VXDIO, REPORT_ERR,
               "An MSVDX context handle must be provided");
        return IMG_ERROR_INVALID_PARAMETERS;
    }

    /* There's only one decoding pipe available in MSVDX. */
    psState->sFwState.asPipeState[0].bPipePresent = IMG_TRUE;

    if (psContext->bClocks)
    {
        IMG_UINT32             ui32RegVal;
        VDECFW_sFirmwareState  sFirmwareState;

        //Reading only channel 2 (DMA_CHANNEL_SR1)
        ui32RegVal = REGIO_READ_TABLE_REGISTER(psContext, DMAC, DMAC_COUNT, 2);
        psState->sFwState.asPipeState[0].aui32DMACStatus[0] = REGIO_READ_FIELD(ui32RegVal, DMAC, DMAC_COUNT, CNT);

        // FE current MB number.
        // Obtain the last processed MB on front-end HW expressed as an X,Y offset into the frame.
        ui32RegVal = REGIO_READ_REGISTER(psContext, MSVDX_VEC, CR_VEC_ENTDEC_INFORMATION);
        psState->sFwState.asPipeState[0].sFeMb.ui32X = REGIO_READ_FIELD(ui32RegVal, MSVDX_VEC, CR_VEC_ENTDEC_INFORMATION, FE_ENTDEC_LATEST_MB_ADDR_X);
        psState->sFwState.asPipeState[0].sFeMb.ui32Y = REGIO_READ_FIELD(ui32RegVal, MSVDX_VEC, CR_VEC_ENTDEC_INFORMATION, FE_ENTDEC_LATEST_MB_ADDR_Y);

        // BE current MB number.
        // Obtain the last processed MB on back-end HW expressed as an X,Y offset into the frame.
        ui32RegVal = REGIO_READ_REGISTER(psContext, MSVDX_VDMC, CR_VDMC_MACROBLOCK_NUMBER);
        psState->sFwState.asPipeState[0].sBeMb.ui32X = REGIO_READ_FIELD(ui32RegVal, MSVDX_VDMC, CR_VDMC_MACROBLOCK_NUMBER, CR_VDMC_MACROBLOCK_X_OFFSET);
        psState->sFwState.asPipeState[0].sBeMb.ui32Y = REGIO_READ_FIELD(ui32RegVal, MSVDX_VDMC, CR_VDMC_MACROBLOCK_NUMBER, CR_VDMC_MACROBLOCK_Y_OFFSET);

        // MSVDX interrupt status.
        psState->sFwState.asPipeState[0].ui32IntStatus = REGIO_READ_REGISTER(psContext, MSVDX_CORE, CR_MSVDX_INTERRUPT_STATUS);

        // Determine the latest transaction to have passed each checkpoint in the firmware.
        // Read the firmware state from VEC Local RAM
        MSVDXIO_SEC_VLRReadWords(psContext,
                                 REGION_VLRFE_REGSPACE,
                                 VLR_FIRMWARE_STATE_AREA_BASE_ADDR,
                                 (sizeof(VDECFW_sFirmwareState)>>2),
                                 (IMG_UINT32*)&sFirmwareState,
                                 IMG_FALSE);
        // Update firmware state fields.
        IMG_MEMCPY(psState->sFwState.aui8ControlFenceID,
                   sFirmwareState.aui8ControlFenceID,
                   sizeof(psState->sFwState.aui8ControlFenceID));
        IMG_MEMCPY(psState->sFwState.aui8DecodeFenceID,
                   sFirmwareState.aui8DecodeFenceID,
                   sizeof(psState->sFwState.aui8DecodeFenceID));
        IMG_MEMCPY(psState->sFwState.aui8CompletionFenceID,
                   sFirmwareState.aui8CompletionFenceID,
                   sizeof(psState->sFwState.aui8CompletionFenceID));
        IMG_MEMCPY(psState->sFwState.asPipeState[0].aui32CheckPoint,
                   sFirmwareState.aui32CheckPoint,
                   sizeof(psState->sFwState.asPipeState[0].aui32CheckPoint));
        psState->sFwState.asPipeState[0].eFirmwareAction = sFirmwareState.eFirmwareAction;
        psState->sFwState.asPipeState[0].ui32FenceValue  = sFirmwareState.ui32FenceValue;
        psState->sFwState.asPipeState[0].ui8CurCodec     = sFirmwareState.ui8CurCodec;
        psState->sFwState.asPipeState[0].ui32FeSlices    = sFirmwareState.ui32FESlices;
        psState->sFwState.asPipeState[0].ui32BeSlices    = sFirmwareState.ui32FESlices;


        // Get runtime information from MSVDX.
        // MTX status.
        ui32Reg = 0;
        REGIO_WRITE_FIELD(ui32Reg, MTX_CORE, CR_MTX_REGISTER_READ_WRITE_REQUEST, MTX_RNW, 1);         // Read
        REGIO_WRITE_FIELD(ui32Reg, MTX_CORE, CR_MTX_REGISTER_READ_WRITE_REQUEST, MTX_USPECIFIER, 5);  // PC or PCX
        REGIO_WRITE_FIELD(ui32Reg, MTX_CORE, CR_MTX_REGISTER_READ_WRITE_REQUEST, MTX_RSPECIFIER, 0);  // PC
        REGIO_WRITE_REGISTER(psContext, MTX_CORE, CR_MTX_REGISTER_READ_WRITE_REQUEST, ui32Reg);
        psState->sEPRuntimeStatus.ui32MTX_PC = REGIO_READ_REGISTER(psContext, MTX_CORE, CR_MTX_REGISTER_READ_WRITE_DATA);

        REGIO_WRITE_FIELD(ui32Reg, MTX_CORE, CR_MTX_REGISTER_READ_WRITE_REQUEST, MTX_RNW, 1);         // Read
        REGIO_WRITE_FIELD(ui32Reg, MTX_CORE, CR_MTX_REGISTER_READ_WRITE_REQUEST, MTX_USPECIFIER, 5);  // PC or PCX
        REGIO_WRITE_FIELD(ui32Reg, MTX_CORE, CR_MTX_REGISTER_READ_WRITE_REQUEST, MTX_RSPECIFIER, 1);  // PCX
        REGIO_WRITE_REGISTER(psContext, MTX_CORE, CR_MTX_REGISTER_READ_WRITE_REQUEST, ui32Reg);
        psState->sEPRuntimeStatus.ui32MTX_PCX = REGIO_READ_REGISTER(psContext, MTX_CORE, CR_MTX_REGISTER_READ_WRITE_DATA);

        REGIO_WRITE_FIELD(ui32Reg, MTX_CORE, CR_MTX_REGISTER_READ_WRITE_REQUEST, MTX_RNW, 1);         // Read
        REGIO_WRITE_FIELD(ui32Reg, MTX_CORE, CR_MTX_REGISTER_READ_WRITE_REQUEST, MTX_USPECIFIER, 3);  // A0StP
        REGIO_WRITE_FIELD(ui32Reg, MTX_CORE, CR_MTX_REGISTER_READ_WRITE_REQUEST, MTX_RSPECIFIER, 0);
        REGIO_WRITE_REGISTER(psContext, MTX_CORE, CR_MTX_REGISTER_READ_WRITE_REQUEST, ui32Reg);
        psState->sEPRuntimeStatus.ui32MTX_A0StP = REGIO_READ_REGISTER(psContext, MTX_CORE, CR_MTX_REGISTER_READ_WRITE_DATA);

        REGIO_WRITE_FIELD(ui32Reg, MTX_CORE, CR_MTX_REGISTER_READ_WRITE_REQUEST, MTX_RNW, 1);         // Read
        REGIO_WRITE_FIELD(ui32Reg, MTX_CORE, CR_MTX_REGISTER_READ_WRITE_REQUEST, MTX_USPECIFIER, 3);  // A0FrP
        REGIO_WRITE_FIELD(ui32Reg, MTX_CORE, CR_MTX_REGISTER_READ_WRITE_REQUEST, MTX_RSPECIFIER, 1);
        REGIO_WRITE_REGISTER(psContext, MTX_CORE, CR_MTX_REGISTER_READ_WRITE_REQUEST, ui32Reg);
        psState->sEPRuntimeStatus.ui32MTX_A0FrP = REGIO_READ_REGISTER(psContext, MTX_CORE, CR_MTX_REGISTER_READ_WRITE_DATA);

        psState->sEPRuntimeStatus.ui32MTX_ENABLE = REGIO_READ_REGISTER(psContext, MTX_CORE, CR_MTX_ENABLE);
        psState->sEPRuntimeStatus.ui32MTX_STATUS_BITS = REGIO_READ_REGISTER(psContext, MTX_CORE, CR_MTX_STATUS);
        psState->sEPRuntimeStatus.ui32MTX_FAULT0 = REGIO_READ_REGISTER(psContext, MTX_CORE, CR_MTX_FAULT0);

        psState->sEPRuntimeStatus.ui32DMA_COUNT[0] = REGIO_READ_TABLE_REGISTER(psContext,  DMAC, DMAC_COUNT, 0);
        psState->sEPRuntimeStatus.ui32DMA_COUNT[1] = REGIO_READ_TABLE_REGISTER(psContext,  DMAC, DMAC_COUNT, 1);
        psState->sEPRuntimeStatus.ui32DMA_COUNT[2] = REGIO_READ_TABLE_REGISTER(psContext,  DMAC, DMAC_COUNT, 2);

        psState->sEPRuntimeStatus.ui32DMA_SETUP[0] = REGIO_READ_TABLE_REGISTER(psContext,  DMAC, DMAC_SETUP, 0);
        psState->sEPRuntimeStatus.ui32DMA_SETUP[1] = REGIO_READ_TABLE_REGISTER(psContext,  DMAC, DMAC_SETUP, 1);
        psState->sEPRuntimeStatus.ui32DMA_SETUP[2] = REGIO_READ_TABLE_REGISTER(psContext,  DMAC, DMAC_SETUP, 2);

        psState->sEPRuntimeStatus.ui32DMA_PERIPHERAL_ADDR[0] =
            REGIO_READ_TABLE_REGISTER(psContext, DMAC, DMAC_PERIPHERAL_ADDR, 0);
        psState->sEPRuntimeStatus.ui32DMA_PERIPHERAL_ADDR[1] =
            REGIO_READ_TABLE_REGISTER(psContext, DMAC, DMAC_PERIPHERAL_ADDR, 1);
        psState->sEPRuntimeStatus.ui32DMA_PERIPHERAL_ADDR[2] =
            REGIO_READ_TABLE_REGISTER(psContext, DMAC, DMAC_PERIPHERAL_ADDR, 2);
    }

    return IMG_SUCCESS;
}


/*!
******************************************************************************

 @Function              MSVDXIO_SEC_HandleInterrupts

******************************************************************************/
IMG_RESULT
MSVDXIO_SEC_HandleInterrupts(
    const IMG_HANDLE    hContext,
    VXD_sIntStatus    * psIntStatus,
    IMG_UINT32        * pui32MtxMsgs,
    IMG_UINT32        * pui32MsgsSizeWrds,
    IMG_BOOL          * pbMoreData
)
{
    MSVDXIO_sContext  * psContext = (MSVDXIO_sContext *)hContext;
    IMG_UINT32          ui32Enable;
    IMG_UINT32          ui32Reg;
    IMG_UINT32          ui32Result = IMG_SUCCESS;

    if (hContext == IMG_NULL)
    {
        REPORT(REPORT_MODULE_VXDIO, REPORT_ERR,
               "An MSVDX context handle must be provided");
        return IMG_ERROR_INVALID_PARAMETERS;
    }

    if (psIntStatus == IMG_NULL)
    {
        REPORT(REPORT_MODULE_VXDIO, REPORT_ERR,
               "A valid interrupt status context must be provided");
        return IMG_ERROR_INVALID_PARAMETERS;
    }

    if (pui32MtxMsgs == IMG_NULL || pui32MsgsSizeWrds == IMG_NULL ||
        *pui32MsgsSizeWrds == 0)
    {
        REPORT(REPORT_MODULE_VXDIO, REPORT_ERR,
               "Invalid buffer for MTX messages");
        return IMG_ERROR_INVALID_PARAMETERS;
    }

    psIntStatus->ui32Pending = REGIO_READ_REGISTER(psContext, MSVDX_CORE, CR_MSVDX_INTERRUPT_STATUS);
    ui32Enable = REGIO_READ_REGISTER(psContext, MSVDX_CORE, CR_MSVDX_HOST_INTERRUPT_ENABLE);

    psIntStatus->ui32Pending &= ui32Enable;

    if (psIntStatus->ui32Pending & MSVDX_CORE_CR_MSVDX_INTERRUPT_STATUS_CR_MMU_FAULT_IRQ_MASK)
    {
        ui32Reg = REGIO_READ_REGISTER(psContext, MSVDX_CORE, CR_MMU_STATUS);
        psIntStatus->ui32Requestor = (psIntStatus->ui32Pending & MSVDX_CORE_CR_MSVDX_INTERRUPT_STATUS_CR_MMU_FAULT_IRQ_MASK)
                                            >> MSVDX_CORE_CR_MSVDX_INTERRUPT_STATUS_CR_MMU_FAULT_IRQ_SHIFT;
        psIntStatus->MMU_FAULT_ADDR = REGIO_READ_FIELD(ui32Reg, MSVDX_CORE, CR_MMU_STATUS, CR_MMU_FAULT_ADDR) << 12 ;
        psIntStatus->MMU_FAULT_RNW = REGIO_READ_FIELD(ui32Reg, MSVDX_CORE, CR_MMU_STATUS, CR_MMU_FAULT_RNW);
        psIntStatus->MMU_PF_N_RW = REGIO_READ_FIELD(ui32Reg, MSVDX_CORE, CR_MMU_STATUS, CR_MMU_PF_N_RW);

        // Disable MMU interrupt until reset has completed since the
        // hardware may continue to raise fault interrupts.
        ui32Reg = REGIO_READ_REGISTER(psContext, MSVDX_CORE, CR_MSVDX_HOST_INTERRUPT_ENABLE);
        REGIO_WRITE_REGISTER(psContext, MSVDX_CORE, CR_MSVDX_HOST_INTERRUPT_ENABLE,
                             ui32Reg & ~MSVDX_CORE_CR_MSVDX_INTERRUPT_STATUS_CR_MMU_FAULT_IRQ_MASK);
        /*
         * We're now expecting rest of the stack to eventually:
         * - read the interrupt status
         * - reset the core (depending on status- maybe fixing MMU PF)
         * - enable MMU PF interrupt
         */
    }

    if (psIntStatus->ui32Pending & MSVDX_CORE_CR_MSVDX_INTERRUPT_STATUS_CR_MTX_IRQ_MASK)
    {
        /* Clear interrupt first to allow for others coming.
         * Interrupt has to be cleared before reading comms buffer, as MTX can be currently
         * waiting for free space in this buffer. If we tried to read the buffer first,
         * following scenario could occur (on assumption that comms buffer is full):
         * -> host reads buffer
         * -> MTX starts adding new messages, as there is a space for messages now
         * -> host ends reading buffer and checks if there are any new messages
         *    (there are none as MTX did not finish yet)
         * -> MTX ends writing to buffer and signal IRQ
         * -> host clears IRQ
         * Lockup occurs as host skipped interrupt. */
        // Clear firmware message interrupt.
        REGIO_WRITE_REGISTER(psContext, MSVDX_CORE, CR_MSVDX_INTERRUPT_CLEAR,
                             MSVDX_CORE_CR_MSVDX_INTERRUPT_STATUS_CR_MTX_IRQ_MASK);
    }

    // Copy all the messages into provided buffer.
    ui32Result = MTXIO_ProcessMTXMsgs(&psContext->sMsgContext,
                                      pui32MtxMsgs,
                                      pui32MsgsSizeWrds,
                                      pbMoreData);

    return ui32Result;
}


/*!
******************************************************************************

 @Function              MSVDXIO_SEC_SendFirmwareMessage

******************************************************************************/
IMG_RESULT
MSVDXIO_SEC_SendFirmwareMessage(
    const IMG_HANDLE            hContext,
    VXD_eCommsArea              eArea,
    const IMG_VOID            * psMsgHdr,
    IMG_UINT32                  ui32MsgSize
)
{
    MSVDXIO_sContext          * psContext = (MSVDXIO_sContext *)hContext;
    IMG_UINT32                  ui32Reg;
    IMG_UINT32                  ui32Result;

    if (hContext == IMG_NULL)
    {
        REPORT(REPORT_MODULE_VXDIO, REPORT_ERR,
               "An MSVDX context handle must be provided");
        return IMG_ERROR_INVALID_PARAMETERS;
    }

    if (psMsgHdr == IMG_NULL)
    {
        REPORT(REPORT_MODULE_VXDIO, REPORT_ERR,
               "A valid message buffer must be provided");
        return IMG_ERROR_INVALID_PARAMETERS;
    }

    // Send message to HW.
    ui32Result = MTXIO_SendMTXMsg(&psContext->sMsgContext,
                                  eArea,
                                  psMsgHdr,
                                  ui32MsgSize);
    if (ui32Result != IMG_SUCCESS)
    {
        REPORT(REPORT_MODULE_VXDIO, REPORT_ERR,
               "Message send failed (%s:%d)",
               __FUNCTION__,
               __LINE__);
        return ui32Result;
    }

    // Kick the MTX. Must write to fake MTX register space in memory.
    ui32Reg = 0;
    REGIO_WRITE_FIELD(ui32Reg, MTX_CORE, CR_MTX_KICKI, MTX_KICKI, 1);

#ifdef SECURE_CPU_REG_ACCESS
    TALREG_WriteWord32(psContext->pahMemSpace[REGION_VXDTEST_3_REGSPACE],
                       MSVDX_TEST_3_SECURE_CPU_CTRL_0_OFFSET,
                       bSecEnable | SECURE_REG_ACCESS);
#endif /* SECURE_CPU_REG_ACCESS */

    TALREG_WriteWord32(psContext->pahMemSpace[REGION_VXDMTX_MEMSPACE],
                       MTX_CORE_CR_MTX_KICKI_OFFSET, ui32Reg);
    return IMG_SUCCESS;
}


#ifdef POST_TEST
/*!
******************************************************************************

 @Function              checkMTXStart

 @Description

 Function to check MTX starts.

******************************************************************************/
IMG_RESULT checkMTXStart(
        IMG_HANDLE hMsvdxContext
)
{
    IMG_RESULT eResult;
    IMG_UINT32 ui32Register;
    IMG_UINT32 ui32TimeoutCounter = 1000;
    MSVDXIO_sContext *psContext = (MSVDXIO_sContext *)hMsvdxContext;

    /* wait MTX starts until timeout */
    do
    {
        /* read register. ATTENTION: an eventual invalid returned value is not managed */
        ui32Register = REGIO_READ_REGISTER(psContext, MTX_CORE, CR_MTX_ENABLE);

        /* decrement timeout counter */
        ui32TimeoutCounter--;
    }
    while((1 == REGIO_READ_FIELD(ui32Register, MTX_CORE, CR_MTX_ENABLE, MTX_TOFF))
       && (ui32TimeoutCounter > 0));

    /* set result */
    if(0 == ui32TimeoutCounter)
    {
        /* test FAILED */
        REPORT(REPORT_MODULE_VXDIO, REPORT_ERR,
               "***** POST TEE: Start MTX check FAILED");
        eResult = IMG_ERROR_FATAL;
    }
    else
    {
        /* test PASSED */
        REPORT(REPORT_MODULE_VXDIO, REPORT_NOTICE,
               "***** POST TEE: Start MTX check PASSED");
        eResult = IMG_SUCCESS;
    }

    return eResult;
}

#endif /* POST_TEST */


#ifdef STACK_USAGE_TEST
/*!
******************************************************************************

 @Function              MSVDXIO_GetStackUsage

 @Description

 Function to get stack usage. It read stack memory space to calculated the
 overwritten filled part.

******************************************************************************/
IMG_RESULT
MSVDXIO_GetStackUsage(
    const IMG_HANDLE    hContext,
    IMG_UINT32        * paui32StackInfoArray
)
{
    IMG_UINT32 ui32Reg;
    IMG_UINT32 ui32ReadData;
    IMG_UINT32 ui32RamId;
    IMG_UINT32 ui32MtxTimeOut;
    IMG_UINT32 ui32Addr;
    IMG_UINT32 ui32MTXBankSize;
    IMG_UINT32 ui32Size;
    IMG_UINT32 ui32NumOfOverwLoc;         /* number of overwritten locations */
    IMG_UINT32 ui32LastOverwAdd;          /* last overwritten address */
    IMG_UINT32 ui32StackUsage;            /* stack usage in words */
    IMG_RESULT ui32Result = IMG_SUCCESS;
    MSVDXIO_sContext  * pMsvdxioCtx = (MSVDXIO_sContext *)hContext;
    VXDIO_sContext * pVxdioCtx = &(pMsvdxioCtx);

    /* init variables */
    ui32Addr = STACK_START_ADDRESS;
    ui32Size = STACK_SIZE;
    ui32MTXBankSize = pVxdioCtx->sMTXRamInfo.ui32MTXBankSize;
    ui32ReadData = 0;
    ui32StackUsage = 0;
    ui32NumOfOverwLoc = 0;
    ui32LastOverwAdd = 0;

    /* Initiate read access to RAM block...*/
    ui32RamId = MSVDXIO_MTX_CORE_MEMORY + (ui32Addr / ui32MTXBankSize);
    ui32Reg = 0;
    REGIO_WRITE_FIELD(ui32Reg, MTX_CORE, CR_MTX_RAM_ACCESS_CONTROL, MTX_MCMID, ui32RamId);
    REGIO_WRITE_FIELD(ui32Reg, MTX_CORE, CR_MTX_RAM_ACCESS_CONTROL, MTX_MCM_ADDR, (ui32Addr>>2));

    if (ui32Size > 1)
    {
        REGIO_WRITE_FIELD(ui32Reg, MTX_CORE, CR_MTX_RAM_ACCESS_CONTROL, MTX_MCMAI, 1);
    }

    REGIO_WRITE_FIELD(ui32Reg, MTX_CORE, CR_MTX_RAM_ACCESS_CONTROL, MTX_MCMR, 1);
    REGIO_WRITE_REGISTER(pVxdioCtx, MTX_CORE, CR_MTX_RAM_ACCESS_CONTROL, ui32Reg);

    /* Repeat for all words...*/
    while ((ui32Size > 0)
       &&  (IMG_ERROR_TIMEOUT != ui32Result))
    {
        /* Wait for read to complete...*/
        ui32MtxTimeOut = MSVDXIO_TIMEOUT_COUNTER;

        do
        {
            /* Check MTX is OK...*/
            ui32Reg = REGIO_READ_REGISTER(pVxdioCtx, MTX_CORE, CR_MTX_FAULT0);
            if (ui32Reg != 0)
            {
                ui32Result = IMG_ERROR_TIMEOUT;
            }

            ui32MtxTimeOut--;
            ui32Reg = REGIO_READ_REGISTER(pVxdioCtx, MTX_CORE, CR_MTX_RAM_ACCESS_STATUS);
        }
        while ((REGIO_READ_FIELD(ui32Reg, MTX_CORE, CR_MTX_RAM_ACCESS_STATUS, MTX_MTX_MCM_STAT) == 0)
           &&  (ui32MtxTimeOut != 0)
           &&  (IMG_ERROR_TIMEOUT != ui32Result));

        /* go ahead if all ok */
        if(IMG_ERROR_TIMEOUT != ui32Result)
        {

            IMG_ASSERT(ui32MtxTimeOut != 0);

            /* get data */
            ui32ReadData = REGIO_READ_REGISTER(pVxdioCtx, MTX_CORE, CR_MTX_RAM_ACCESS_DATA_TRANSFER);
            /* if value is not a fill value */
            if(STACK_FILL_VALUE != ui32ReadData)
            {
                /* increment number of overwritten locations */
                ui32NumOfOverwLoc++;

                /* update last overwritten address */
                ui32LastOverwAdd = ui32Addr;
            }
            else
            {
                /* do nothing */
            }

            /* decrement remaining bytes */
            ui32Size--;

            /* If we have move from one RAM to the next...*/
            ui32Addr += 4;

            if ((MSVDXIO_MTX_CORE_MEMORY + (ui32Addr / ui32MTXBankSize)) != ui32RamId)
            {
                /* Initiate read access to RAM block...*/
                ui32RamId = (MSVDXIO_MTX_CORE_MEMORY + (ui32Addr / ui32MTXBankSize));
                REGIO_WRITE_FIELD(ui32Reg, MTX_CORE, CR_MTX_RAM_ACCESS_CONTROL, MTX_MCMID, ui32RamId);
                REGIO_WRITE_FIELD(ui32Reg, MTX_CORE, CR_MTX_RAM_ACCESS_CONTROL, MTX_MCM_ADDR, (ui32Addr>>2));

                if (ui32Size > 1)
                {
                    REGIO_WRITE_FIELD(ui32Reg, MTX_CORE, CR_MTX_RAM_ACCESS_CONTROL, MTX_MCMAI, 1);
                }

                REGIO_WRITE_FIELD(ui32Reg, MTX_CORE, CR_MTX_RAM_ACCESS_CONTROL, MTX_MCMR, 1);
                REGIO_WRITE_REGISTER(pVxdioCtx, MTX_CORE, CR_MTX_RAM_ACCESS_CONTROL, ui32Reg);
            }
        }
    }

    /* calculate stack usage in words */
    ui32StackUsage = (IMG_UINT32)((ui32LastOverwAdd - STACK_START_ADDRESS) / 4);

    /* copy stack usage in words to array location 1 */
    paui32StackInfoArray[0] = (IMG_UINT32)ui32StackUsage;

    /* calculate and copy stack usage in percentage to array location 2 */
    paui32StackInfoArray[1] = (IMG_UINT32)((ui32StackUsage * 100) / STACK_SIZE);

    /* copy overwritten space of used stack (effective stack usage) in words to array location 3 */
    paui32StackInfoArray[2] = (IMG_UINT32)ui32NumOfOverwLoc;

    /* calculate and copy overwritten space of used stack in percentage to array location 4 */
    paui32StackInfoArray[3] = (IMG_UINT32)((ui32NumOfOverwLoc * 100) / ui32StackUsage);

    /* calculate and copy overwritten space of entire stack in percentage (effective stack usage) to array location 5 */
    paui32StackInfoArray[4] = (IMG_UINT32)((ui32NumOfOverwLoc * 100) / STACK_SIZE);

    /* copy total stack size to array location 6 */
    paui32StackInfoArray[5] = STACK_SIZE;

    return ui32Result;
}


/*!
******************************************************************************

 @Function              msvdxio_MTXFillStack

 @Description

 Function to fill the MTX stack.

******************************************************************************/
static IMG_RESULT
msvdxio_MTXFillStack(
    const IMG_HANDLE    hContext,
    IMG_UINT32          ui32Addr,         // Byte offset into MTX memory (starts at 0).
    IMG_UINT32          ui32Size,         // Number of 32-bit words to write.
    IMG_UINT32          ui32MTXBankSize,
    IMG_UINT32          ui32FillValue
)
{
    MSVDXIO_sContext  * psContext = (MSVDXIO_sContext *)hContext;
    IMG_RESULT          ui32Result = IMG_SUCCESS;
    IMG_INT32           ui32Reg;
    IMG_UINT32          ui32MtxTimeOut;
    IMG_UINT32          ui32RamId;
    IMG_UINT32          ui32AccessControl;

    /* report the start of stack filling */
    REPORT(REPORT_MODULE_VXDIO, REPORT_NOTICE,
           "***** STACK USAGE TEST - Filling stack space...");

    /* Request the bus from the Dash...*/
    ui32Reg = 0;
    REGIO_WRITE_FIELD(ui32Reg, MSVDX_CORE, CR_MTX_DEBUG, CR_MTX_DBG_IS_SLAVE, 1);
    REGIO_WRITE_FIELD(ui32Reg, MSVDX_CORE, CR_MTX_DEBUG, CR_MTX_DBG_GPIO_IN, 0x02);
    REGIO_WRITE_REGISTER(psContext, MSVDX_CORE, CR_MTX_DEBUG, ui32Reg);
    msvdxio_WaitForMTX(m);

    /* Set HOST_ACCESS so Codescape knows the host is using the bus. */
    REGIO_WRITE_FIELD(ui32Reg, MSVDX_CORE, CR_MTX_DEBUG, CR_MTX_DBG_GPIO_IN, 0x03);
    REGIO_WRITE_REGISTER(psContext, MSVDX_CORE, CR_MTX_DEBUG, ui32Reg);

    /* Save the access control register...*/
    ui32AccessControl = REGIO_READ_REGISTER(psContext, MTX_CORE, CR_MTX_RAM_ACCESS_CONTROL);

    /* Initiate write access to RAM block...*/
    ui32RamId = MSVDXIO_MTX_CORE_MEMORY + (ui32Addr / ui32MTXBankSize);
    ui32Reg = 0;
    REGIO_WRITE_FIELD(ui32Reg, MTX_CORE, CR_MTX_RAM_ACCESS_CONTROL, MTX_MCMID, ui32RamId);
    REGIO_WRITE_FIELD(ui32Reg, MTX_CORE, CR_MTX_RAM_ACCESS_CONTROL, MTX_MCM_ADDR, (ui32Addr>>2));

    if (ui32Size > 1)
    {
        REGIO_WRITE_FIELD(ui32Reg, MTX_CORE, CR_MTX_RAM_ACCESS_CONTROL, MTX_MCMAI, 1);
    }

    REGIO_WRITE_REGISTER(psContext, MTX_CORE, CR_MTX_RAM_ACCESS_CONTROL, ui32Reg);

    /* Repeat for all words...*/
    while (ui32Size > 0)
    {
        /* Write the next word...*/
        REGIO_WRITE_REGISTER(psContext, MTX_CORE, CR_MTX_RAM_ACCESS_DATA_TRANSFER, ui32FillValue);

        /* Wait for write to complete...*/
        ui32MtxTimeOut = MSVDXIO_TIMEOUT_COUNTER;
        do
        {
             /* Check MTX is OK...*/
            ui32Reg = REGIO_READ_REGISTER(psContext, MTX_CORE, CR_MTX_FAULT0);

            if (ui32Reg != 0)
            {
                ui32Result = IMG_ERROR_TIMEOUT;
                goto ErrorExit;
            }

            ui32MtxTimeOut--;
            ui32Reg = REGIO_READ_REGISTER(psContext, MTX_CORE, CR_MTX_RAM_ACCESS_STATUS);
        }
        while ((REGIO_READ_FIELD(ui32Reg, MTX_CORE, CR_MTX_RAM_ACCESS_STATUS, MTX_MTX_MCM_STAT) == 0) &&
               (ui32MtxTimeOut != 0));

        IMG_ASSERT(ui32MtxTimeOut != 0);

        ui32Size--;

        /* If we have move from one RAM to the next...*/
        ui32Addr += 4;
        if ((MSVDXIO_MTX_CORE_MEMORY + (ui32Addr / ui32MTXBankSize)) != ui32RamId)
        {
            /* Initiate write access to RAM block...*/
            ui32RamId = (MSVDXIO_MTX_CORE_MEMORY + (ui32Addr / ui32MTXBankSize));
            ui32Reg = 0;
            REGIO_WRITE_FIELD(ui32Reg, MTX_CORE, CR_MTX_RAM_ACCESS_CONTROL, MTX_MCMID, ui32RamId);
            REGIO_WRITE_FIELD(ui32Reg, MTX_CORE, CR_MTX_RAM_ACCESS_CONTROL, MTX_MCM_ADDR, (ui32Addr>>2));
            if (ui32Size > 1)
            {
                REGIO_WRITE_FIELD(ui32Reg, MTX_CORE, CR_MTX_RAM_ACCESS_CONTROL, MTX_MCMAI, 1);
            }
            REGIO_WRITE_REGISTER(psContext, MTX_CORE, CR_MTX_RAM_ACCESS_CONTROL, ui32Reg);
        }
    }

ErrorExit:
    /* Restore the access control register...*/
    REGIO_WRITE_REGISTER(psContext, MTX_CORE, CR_MTX_RAM_ACCESS_CONTROL, ui32AccessControl);

    /* Release the bus...*/
    ui32Reg = 0;
    REGIO_WRITE_FIELD(ui32Reg, MSVDX_CORE, CR_MTX_DEBUG, CR_MTX_DBG_IS_SLAVE, 1);
    REGIO_WRITE_REGISTER(psContext, MSVDX_CORE, CR_MTX_DEBUG, ui32Reg);

    /* Ensure the MTX is OK....*/
    IMG_ASSERT(REGIO_READ_REGISTER(psContext, MTX_CORE, CR_MTX_FAULT0) == 0);

    /* report the end of stack filling */
    REPORT(REPORT_MODULE_VXDIO, REPORT_NOTICE,
           "***** STACK USAGE TEST - Stack space filling done: %d",
           ui32Result);

    return ui32Result;
}
#else /* not STACK_USAGE_TEST */
/*!
******************************************************************************

 @Function              MSVDXIO_GetStackUsage

 @Description

 Function to get stack usage. It read stack memory space to calculated the
 overwritten filled part.

******************************************************************************/
IMG_RESULT
MSVDXIO_GetStackUsage(
    const IMG_HANDLE    hContext,
    IMG_UINT32        * paui32StackInfoArray
)
{
    IMG_RESULT rResult;
    rResult = IMG_SUCCESS;

    /* do nothing at the moment */

    /* fake values */
    paui32StackInfoArray[0] = 0;
    paui32StackInfoArray[1] = 0;
    paui32StackInfoArray[2] = 0;
    paui32StackInfoArray[3] = 0;
    paui32StackInfoArray[4] = 0;
    paui32StackInfoArray[5] = 0;

    return rResult;
}
#endif /* STACK_USAGE_TEST */

#ifdef POST_TEST
/*!
******************************************************************************

 @Function              msvdxio_initVLRCheckPoints

******************************************************************************/
static IMG_RESULT
msvdxio_initVLRCheckPoints(
    MSVDXIO_sContext  * psContext
)
{
    IMG_RESULT ui32Result;
    IMG_UINT32            aui32FWCheckPoints[NUM_OF_FW_INIT_CHECKPOINTS];
    IMG_UINT8            ui8Index;



    if(psContext->bPost == IMG_TRUE)
    {
        /* Prepare FW VLR check-points */
        /* prepare values array */
        for(ui8Index = 0; ui8Index < NUM_OF_FW_INIT_CHECKPOINTS; ui8Index++)
        {
            aui32FWCheckPoints[ui8Index] = ((IMG_UINT32)(ui8Index + 1)) |
                                            CHKPNT_HOST_HIGHER_MASK_VALUE;
        }

        /* write values array in VLR */
        ui32Result = MSVDXIO_SEC_VLRWriteWords(psContext,
                                               REGION_VLRFE_REGSPACE,
                                               (VLR_FIRMWARE_STATE_AREA_BASE_ADDR +
                                                   (VDECFW_CHECKPOINT_FI_START * 4)),
                                               NUM_OF_FW_INIT_CHECKPOINTS,
                                               &aui32FWCheckPoints[0]);
        if(IMG_SUCCESS != ui32Result)
        {
            /* Fail to write checkpoints values */

            REPORT(REPORT_MODULE_VXDIO, REPORT_ERR,
                    "***** POST TEE: VLR Check-points area initialisation FAILED");

            return ui32Result;
        }
        else
        {
            /* checkpoints values initialised fine */

            REPORT(REPORT_MODULE_VXDIO, REPORT_NOTICE,
                    "***** POST TEE: VLR Check-points area initialisation PASSED");
        }
    }

    return IMG_SUCCESS;
}
#endif /* POST_TEST */


/*!
******************************************************************************

 @Function              msvdxio_SEC_ConfigureMMU

******************************************************************************/
static IMG_VOID
msvdxio_SEC_ConfigureMMU(
    const IMG_HANDLE        hContext,
    IMG_UINT32              ui32MmuCtrl2,
    IMG_UINT32              ui32PtdPhysAddr
)
{
    /* Configure the MMU to DMA the firmware base component. */
    REGIO_WRITE_REGISTER(hContext, MSVDX_CORE, CR_MMU_CONTROL2, ui32MmuCtrl2);
    /* Set base address of PTD */
    REGIO_WRITE_REGISTER(hContext, MSVDX_CORE, CR_MMU_DIR_LIST_BASE,
                         ui32PtdPhysAddr);
    /* Enable mmu for all requestors */
    REGIO_WRITE_REGISTER(hContext, MSVDX_CORE, CR_MMU_CONTROL0, 0);
}
/*!
******************************************************************************

 @Function              MSVDXIO_SEC_LoadBaseFirmware

******************************************************************************/
IMG_RESULT
MSVDXIO_SEC_LoadBaseFirmware(
    const IMG_HANDLE        hContext,
    IMG_UINT32              ui32MmuCtrl2,
    IMG_UINT32              ui32PtdPhysAddr
)
{
    MSVDXIO_sContext  * psContext = (MSVDXIO_sContext *)hContext;
    IMG_UINT32          ui32Reg;
    IMG_UINT32          ui32Result = IMG_SUCCESS;
    if (hContext == IMG_NULL)
    {
        REPORT(REPORT_MODULE_VXDIO, REPORT_ERR,
               "An MSVDX context handle must be provided");
        return IMG_ERROR_INVALID_PARAMETERS;
    }
    /* Need to init mtx comms area  */
    ui32Result = MTXIO_InitMTXComms(psContext,
                                    &psContext->sMsgContext);
    if(IMG_SUCCESS != ui32Result)
    {
        REPORT(REPORT_MODULE_VXDIO, REPORT_ERR,
                "Failed to initialise MTX comms area");
        return ui32Result;
    }
#ifdef POST_TEST
    ui32Result = msvdxio_initVLRCheckPoints(psContext);
    if (ui32Result != IMG_SUCCESS)
    {
       return ui32Result;
    }
#endif /* POST_TEST */

#ifdef SECUREIMG_TESTS
    return IMG_SUCCESS;
#endif /* SECUREIMG_TESTS */

    /* Set the MTX timer divide register - the value used is system dependent.*/
    ui32Reg = 0;
    REGIO_WRITE_FIELD(ui32Reg, MTX_CORE, CR_MTX_SYSC_TIMERDIV,
                      TIMER_EN, 1); /* Enabled. */
    REGIO_WRITE_FIELD(ui32Reg, MTX_CORE, CR_MTX_SYSC_TIMERDIV,
                      TIMER_DIV, MSVDXIO_MTX_CLK_MHZ - 1);  /* Set for 1MHz timer. */
    REGIO_WRITE_REGISTER(psContext, MTX_CORE,
                         CR_MTX_SYSC_TIMERDIV, ui32Reg);

    /* The MTX must not be enabled at this point, we are going to
     * upload firmware soon. */
    REGIO_WRITE_REGISTER(psContext, MTX_CORE, CR_MTX_ENABLE, 0);

    // Ensure that the MTX is ready to have firmware loaded.
    ui32Result = msvdxio_WaitForMTX(psContext);
    if (ui32Result != IMG_SUCCESS)
    {
        REPORT(REPORT_MODULE_VXDIO, REPORT_NOTICE,
               "Failed to wait for MTX to become ready!");
        return ui32Result;
    }

#ifdef STACK_USAGE_TEST
    if(IMG_TRUE == psContext->bStackUsageTest)
    {

        /* fill the stack */
        ui32Result =  msvdxio_MTXFillStack(psContext,
                                           STACK_START_ADDRESS,
                                           STACK_SIZE,
                                           psContext->sMTXRamInfo.ui32MTXBankSize,
                                           STACK_FILL_VALUE);
        if (ui32Result != IMG_SUCCESS)
        {
            return ui32Result;
        }
    }
#endif /* STACK_USAGE_TEST */


#ifndef USE_MTX_DMA_LOAD
    /* Upload firmware using register interface. */
    ui32Result =
        msvdxio_MTXWriteWords(psContext,
                              0, // Offset within MTX RAM
                              psContext->sSecFwBufInfo.ui32BufSize,
                              psContext->sMTXRamInfo.ui32MTXBankSize,
                              psContext->sSecFwBufInfo.pvCpuVirt,
                              IMG_TRUE);
#else /* USE_MTX_DMA_LOAD */
    msvdxio_SEC_ConfigureMMU(psContext, ui32MmuCtrl2, ui32PtdPhysAddr);
    /* Upload firmare using DMA transfer. */
    ui32Result = msvdxio_sec_DMAMTX(psContext);
    if (ui32Result != IMG_SUCCESS)
    {
        DEBUG_REPORT(REPORT_MODULE_VXDIO,
                     "Failed to DMA secure firmware to the MTX!");
        return ui32Result;
    }

#ifdef POST_TEST
    /* Check if DMA is running */
    ui32Result = checkDMAIsRunning(psContext);
    if(IMG_SUCCESS != ui32Result)
    {
        return ui32Result;
    }
#endif /* POST_TEST */

    // Wait for DMA transfer to complete.
    ui32Result = MSVDXIO_SEC_Poll(psContext,
                                  REGION_DMAC_REGSPACE,
                                  DMAC_DMAC_IRQ_STAT_OFFSET,
                                  DMAC_DMAC_IRQ_STAT_TRANSFER_FIN_MASK,
                                  DMAC_DMAC_IRQ_STAT_TRANSFER_FIN_MASK,
                                  MSVDXIO_POLL_EQUAL);
    if (ui32Result != IMG_SUCCESS)
    {
        REPORT(REPORT_MODULE_VXDIO, REPORT_ERR,
              "Timeout waiting for firmware DMA.");
        return ui32Result;
    }

    DEBUG_REPORT(REPORT_MODULE_VXDIO,
        ">>>>>>>>>>>>>>>>>>>> Firmware base component successfully loaded.");
#endif /* USE_MTX_DMA_LOAD */

    /* The PC must be set before starting the MTX.
    Write the value (must be done before the register write request is issued).
    The HotStart address is 16 bytes after the normal cold entry point. */
    REGIO_WRITE_REGISTER(psContext, MTX_CORE,
                         CR_MTX_REGISTER_READ_WRITE_DATA, 0x80900000);

    /* Create register write request. */
    ui32Reg = 0;
    REGIO_WRITE_FIELD(ui32Reg, MTX_CORE, CR_MTX_REGISTER_READ_WRITE_REQUEST,
                      MTX_RNW, 0); // Write
    REGIO_WRITE_FIELD(ui32Reg, MTX_CORE, CR_MTX_REGISTER_READ_WRITE_REQUEST,
                      MTX_USPECIFIER, 5); // PC or PCX
    REGIO_WRITE_FIELD(ui32Reg, MTX_CORE, CR_MTX_REGISTER_READ_WRITE_REQUEST,
                      MTX_RSPECIFIER, 0); // PC
    REGIO_WRITE_REGISTER(psContext, MTX_CORE,
                         CR_MTX_REGISTER_READ_WRITE_REQUEST, ui32Reg);

    /* Set the MTX running. */
    REGIO_WRITE_REGISTER(psContext, MTX_CORE, CR_MTX_ENABLE, 1);

#ifdef POST_TEST
    if(IMG_TRUE == psContext->bPost)
    {
        /* Execute MTX start test: check if MTX starts */
        ui32Result = checkMTXStart(psContext);
        if(IMG_SUCCESS != ui32Result)
        {
            return ui32Result;
        }
    }
#endif /* POST_TEST */

    /* Enable MTX interrupts. */
    ui32Reg = REGIO_READ_REGISTER(psContext,
                                  MSVDX_CORE,
                                  CR_MSVDX_HOST_INTERRUPT_ENABLE);
    REGIO_WRITE_FIELD(ui32Reg, MSVDX_CORE,
                      CR_MSVDX_INTERRUPT_STATUS, CR_MTX_IRQ, 1);
    REGIO_WRITE_REGISTER(psContext, MSVDX_CORE,
                         CR_MSVDX_HOST_INTERRUPT_ENABLE, ui32Reg);

    return IMG_SUCCESS;
}


/*!
******************************************************************************

 @Function              MSVDXIO_SEC_PrepareFirmware

******************************************************************************/
IMG_RESULT
MSVDXIO_SEC_PrepareFirmware(
    const IMG_HANDLE            hContext,
    IMG_BYTE                  * pbFwBin,
    IMG_UINT32                  ui32FwBinSize,
    VXDIO_sDdBufInfo          * psSecFwBufInfo
)
{
    MSVDXIO_sContext      * psContext = (MSVDXIO_sContext *)hContext;
    IMG_RESULT              ui32Result = IMG_SUCCESS;
    IMG_BYTE              * pbAuthFwBin = IMG_NULL;
    IMG_UINT32              ui32AuthFwBinSize = 0;

    if (hContext == IMG_NULL)
    {
        REPORT(REPORT_MODULE_VXDIO, REPORT_ERR,
               "An MSVDX context handle must be provided!");
        return IMG_ERROR_INVALID_PARAMETERS;
    }

    ui32Result = msvdxio_AuthenticateFirmware(pbFwBin,
                                              ui32FwBinSize,
                                              &pbAuthFwBin,
                                              &ui32AuthFwBinSize);
    if (ui32Result != IMG_SUCCESS)
    {
        REPORT(REPORT_MODULE_VXDIO, REPORT_ERR,
               "Failed to authenticate firmware");
        return ui32Result;
    }

    if (ui32AuthFwBinSize > psSecFwBufInfo->ui32BufSize)
    {
        REPORT(REPORT_MODULE_VXDIO, REPORT_ERR,
               "Authenticated firmware exceeds provided buffer size!");
        return IMG_ERROR_GENERIC_FAILURE;
    }

    if (psSecFwBufInfo->pvCpuVirt == IMG_NULL)
    {
        REPORT(REPORT_MODULE_VXDIO, REPORT_ERR,
               "CPU address of firmware buffer is null!");
        return IMG_ERROR_GENERIC_FAILURE;
    }

    // Copy the firmware into secure buffer accessible by VXD.
    IMG_MEMCPY(psSecFwBufInfo->pvCpuVirt,
               pbAuthFwBin,
               ui32AuthFwBinSize);

    UPDATE_DEVICE_MEM(psSecFwBufInfo);

    // Take a copy of the firmware information.
    psContext->sSecFwBufInfo = *psSecFwBufInfo;

    return IMG_SUCCESS;
}


/*!
******************************************************************************

 @Function              MSVDXIO_SEC_DisableClocks

 @Description

 This function disables MSVDX clocks.

 @Input     hContext    : Handle to MSVDX IO context.

 @Return    IMG_RESULT      : This function returns either IMG_SUCCESS or an
                              error code.

******************************************************************************/
IMG_RESULT
MSVDXIO_SEC_DisableClocks(
    const IMG_HANDLE    hContext
)
{
    MSVDXIO_sContext  * psContext = (MSVDXIO_sContext *)hContext;
    IMG_UINT32          ui32Reg;

    if (hContext == IMG_NULL)
    {
        REPORT(REPORT_MODULE_VXDIO, REPORT_ERR,
               "An MSVDX context handle must be provided");
        return IMG_ERROR_INVALID_PARAMETERS;
    }

    REGIO_WRITE_REGISTER(psContext, MSVDX_CORE, CR_MSVDX_MAN_CLK_ENABLE,
        MSVDX_CORE_CR_MSVDX_MAN_CLK_ENABLE_CR_CORE_MAN_CLK_ENABLE_MASK);
    ui32Reg = REGIO_READ_REGISTER(psContext, MSVDX_CORE, CR_MSVDX_MAN_CLK_ENABLE);
    // Make sure all the clocks are off except core
    //IMG_ASSERT(ui32Reg == MSVDX_CORE_CR_MSVDX_MAN_CLK_ENABLE_CR_CORE_MAN_CLK_ENABLE_MASK);

    REGIO_WRITE_REGISTER(psContext, MSVDX_CORE, CR_MSVDX_MAN_CLK_ENABLE,
        ui32Reg & ~(MSVDX_CORE_CR_MSVDX_MAN_CLK_ENABLE_CR_CORE_MAN_CLK_ENABLE_MASK));

    psContext->bClocks = IMG_FALSE;

    DEBUG_REPORT(REPORT_MODULE_VXDIO, "MSVDX Clocks Disabled");

    return IMG_SUCCESS;
}


/*!
******************************************************************************

 @Function              MSVDXIO_SEC_EnableClocks

******************************************************************************/
IMG_RESULT
MSVDXIO_SEC_EnableClocks(
    const IMG_HANDLE    hContext,
    IMG_BOOL            bAutoClockGatingSupport,
    IMG_BOOL            bExtClockGating,
    IMG_BOOL            bForceManual
)
{
    MSVDXIO_sContext  * psContext = (MSVDXIO_sContext *)hContext;
    IMG_UINT32          ui32RegMsvdxManClkEnable = 0;
    IMG_UINT32          ui32RegMsvdxManClkEnable2 = 0;

    if (hContext == IMG_NULL)
    {
        REPORT(REPORT_MODULE_VXDIO, REPORT_ERR,
               "An MSVDX context handle must be provided");
        return IMG_ERROR_INVALID_PARAMETERS;
    }

    // Core and MTX must ALWAYS be manual.
    REGIO_WRITE_FIELD(ui32RegMsvdxManClkEnable,
        MSVDX_CORE, CR_MSVDX_MAN_CLK_ENABLE, CR_CORE_MAN_CLK_ENABLE,
        1);
    REGIO_WRITE_FIELD(ui32RegMsvdxManClkEnable,
        MSVDX_CORE, CR_MSVDX_MAN_CLK_ENABLE, CR_MTX_MAN_CLK_ENABLE,
        1);

    // Only use auto clock-gating on more recent cores.
    if (bAutoClockGatingSupport && !bForceManual)
    {
        // Auto gating.
        if (!bExtClockGating)
        {
            REGIO_WRITE_FIELD(ui32RegMsvdxManClkEnable,
                MSVDX_CORE, CR_MSVDX_MAN_CLK_ENABLE, CR_VDEB_PROCESS_AUTO_CLK_ENABLE,
                1);
            REGIO_WRITE_FIELD(ui32RegMsvdxManClkEnable,
                MSVDX_CORE, CR_MSVDX_MAN_CLK_ENABLE, CR_VDEB_ACCESS_AUTO_CLK_ENABLE,
                1);
            REGIO_WRITE_FIELD(ui32RegMsvdxManClkEnable,
                MSVDX_CORE, CR_MSVDX_MAN_CLK_ENABLE, CR_VDMC_AUTO_CLK_ENABLE,
                1);
        }
        else
        {
            REGIO_WRITE_FIELD(ui32RegMsvdxManClkEnable2,
                MSVDX_CORE, CR_MSVDX_MAN_CLK_ENABLE2, CR_VEC_REGS_AUTO_CLK_ENABLE,
                1);
            REGIO_WRITE_FIELD(ui32RegMsvdxManClkEnable2,
                MSVDX_CORE, CR_MSVDX_MAN_CLK_ENABLE2, CR_MEM_AUTO_CLK_ENABLE,
                1);
            REGIO_WRITE_FIELD(ui32RegMsvdxManClkEnable2,
                MSVDX_CORE, CR_MSVDX_MAN_CLK_ENABLE2, CR_VDEB_SCALE_BE_AUTO_CLK_ENABLE,
                1);
            REGIO_WRITE_FIELD(ui32RegMsvdxManClkEnable2,
                MSVDX_CORE, CR_MSVDX_MAN_CLK_ENABLE2, CR_VDEB_SCALE_FE_AUTO_CLK_ENABLE,
                1);
            REGIO_WRITE_FIELD(ui32RegMsvdxManClkEnable2,
                MSVDX_CORE, CR_MSVDX_MAN_CLK_ENABLE2, CR_VDEB_PROCESS2_AUTO_CLK_ENABLE,
                1);
            REGIO_WRITE_FIELD(ui32RegMsvdxManClkEnable2,
                MSVDX_CORE, CR_MSVDX_MAN_CLK_ENABLE2, CR_VDEB_PROCESS1_AUTO_CLK_ENABLE,
                1);
            REGIO_WRITE_FIELD(ui32RegMsvdxManClkEnable2,
                MSVDX_CORE, CR_MSVDX_MAN_CLK_ENABLE2, CR_VDEB_ACCESS2_AUTO_CLK_ENABLE,
                1);
            REGIO_WRITE_FIELD(ui32RegMsvdxManClkEnable2,
                MSVDX_CORE, CR_MSVDX_MAN_CLK_ENABLE2, CR_VDEB_ACCESS1_AUTO_CLK_ENABLE,
                1);
            REGIO_WRITE_FIELD(ui32RegMsvdxManClkEnable2,
                MSVDX_CORE, CR_MSVDX_MAN_CLK_ENABLE2, CR_VDMC2_AUTO_CLK_ENABLE,
                1);
            REGIO_WRITE_FIELD(ui32RegMsvdxManClkEnable2,
                MSVDX_CORE, CR_MSVDX_MAN_CLK_ENABLE2, CR_VDMC1_AUTO_CLK_ENABLE,
                1);
            REGIO_WRITE_FIELD(ui32RegMsvdxManClkEnable2,
                MSVDX_CORE, CR_MSVDX_MAN_CLK_ENABLE2, CR_VEC_FE_GRP2_AUTO_CLK_ENABLE,
                1);
            REGIO_WRITE_FIELD(ui32RegMsvdxManClkEnable2,
                MSVDX_CORE, CR_MSVDX_MAN_CLK_ENABLE2, CR_VEC_FE_GRP1_AUTO_CLK_ENABLE,
                1);
            REGIO_WRITE_FIELD(ui32RegMsvdxManClkEnable2,
                MSVDX_CORE, CR_MSVDX_MAN_CLK_ENABLE2, CR_REG_AUTO_CLK_ENABLE,
                1);
            REGIO_WRITE_FIELD(ui32RegMsvdxManClkEnable2,
                MSVDX_CORE, CR_MSVDX_MAN_CLK_ENABLE2, CR_DMAC_AUTO_CLK_ENABLE,
                1);
        }

        REGIO_WRITE_FIELD(ui32RegMsvdxManClkEnable,
            MSVDX_CORE, CR_MSVDX_MAN_CLK_ENABLE, CR_VEC_ENTDEC_AUTO_CLK_ENABLE,
            1);
        REGIO_WRITE_FIELD(ui32RegMsvdxManClkEnable,
            MSVDX_CORE, CR_MSVDX_MAN_CLK_ENABLE, CR_VEC_ITRANS_AUTO_CLK_ENABLE,
            1);
        REGIO_WRITE_FIELD(ui32RegMsvdxManClkEnable,
            MSVDX_CORE, CR_MSVDX_MAN_CLK_ENABLE, CR_VEC_FE_AUTO_CLK_ENABLE,
            1);
        REGIO_WRITE_FIELD(ui32RegMsvdxManClkEnable,
            MSVDX_CORE, CR_MSVDX_MAN_CLK_ENABLE, CR_VEC_BE_AUTO_CLK_ENABLE,
            1);
        REGIO_WRITE_FIELD(ui32RegMsvdxManClkEnable,
            MSVDX_CORE, CR_MSVDX_MAN_CLK_ENABLE, CR_VEC_DEC_AUTO_CLK_ENABLE,
            1);
    }
    else
    {
        // Manual gating.
        if (bExtClockGating)
        {
            REGIO_WRITE_FIELD(ui32RegMsvdxManClkEnable2,
                MSVDX_CORE, CR_MSVDX_MAN_CLK_ENABLE2, CR_VEC_REGS_MAN_CLK_ENABLE,
                1);
            REGIO_WRITE_FIELD(ui32RegMsvdxManClkEnable2,
                MSVDX_CORE, CR_MSVDX_MAN_CLK_ENABLE2, CR_MEM_MAN_CLK_ENABLE,
                1);
            REGIO_WRITE_FIELD(ui32RegMsvdxManClkEnable2,
                MSVDX_CORE, CR_MSVDX_MAN_CLK_ENABLE2, CR_VDEB_SCALE_BE_MAN_CLK_ENABLE,
                1);
            REGIO_WRITE_FIELD(ui32RegMsvdxManClkEnable2,
                MSVDX_CORE, CR_MSVDX_MAN_CLK_ENABLE2, CR_VDEB_SCALE_FE_MAN_CLK_ENABLE,
                1);
            REGIO_WRITE_FIELD(ui32RegMsvdxManClkEnable2,
                MSVDX_CORE, CR_MSVDX_MAN_CLK_ENABLE2, CR_VDEB_PROCESS2_MAN_CLK_ENABLE,
                1);
            REGIO_WRITE_FIELD(ui32RegMsvdxManClkEnable2,
                MSVDX_CORE, CR_MSVDX_MAN_CLK_ENABLE2, CR_VDEB_PROCESS1_MAN_CLK_ENABLE,
                1);
            REGIO_WRITE_FIELD(ui32RegMsvdxManClkEnable2,
                MSVDX_CORE, CR_MSVDX_MAN_CLK_ENABLE2, CR_VDEB_ACCESS2_MAN_CLK_ENABLE,
                1);
            REGIO_WRITE_FIELD(ui32RegMsvdxManClkEnable2,
                MSVDX_CORE, CR_MSVDX_MAN_CLK_ENABLE2, CR_VDEB_ACCESS1_MAN_CLK_ENABLE,
                1);
            REGIO_WRITE_FIELD(ui32RegMsvdxManClkEnable2,
                MSVDX_CORE, CR_MSVDX_MAN_CLK_ENABLE2, CR_VDMC2_MAN_CLK_ENABLE,
                1);
            REGIO_WRITE_FIELD(ui32RegMsvdxManClkEnable2,
                MSVDX_CORE, CR_MSVDX_MAN_CLK_ENABLE2, CR_VDMC1_MAN_CLK_ENABLE,
                1);
            REGIO_WRITE_FIELD(ui32RegMsvdxManClkEnable2,
                MSVDX_CORE, CR_MSVDX_MAN_CLK_ENABLE2, CR_VEC_FE_GRP2_MAN_CLK_ENABLE,
                1);
            REGIO_WRITE_FIELD(ui32RegMsvdxManClkEnable2,
                MSVDX_CORE, CR_MSVDX_MAN_CLK_ENABLE2, CR_VEC_FE_GRP1_MAN_CLK_ENABLE,
                1);
            REGIO_WRITE_FIELD(ui32RegMsvdxManClkEnable2,
                MSVDX_CORE, CR_MSVDX_MAN_CLK_ENABLE2, CR_REG_MAN_CLK_ENABLE,
                1);
            REGIO_WRITE_FIELD(ui32RegMsvdxManClkEnable2,
                MSVDX_CORE, CR_MSVDX_MAN_CLK_ENABLE2, CR_DMAC_MAN_CLK_ENABLE,
                1);
        }

        REGIO_WRITE_FIELD(ui32RegMsvdxManClkEnable,
            MSVDX_CORE, CR_MSVDX_MAN_CLK_ENABLE, CR_VDEB_PROCESS_MAN_CLK_ENABLE,
            1);
        REGIO_WRITE_FIELD(ui32RegMsvdxManClkEnable,
            MSVDX_CORE, CR_MSVDX_MAN_CLK_ENABLE, CR_VDEB_ACCESS_MAN_CLK_ENABLE,
            1);
        REGIO_WRITE_FIELD(ui32RegMsvdxManClkEnable,
            MSVDX_CORE, CR_MSVDX_MAN_CLK_ENABLE, CR_VDMC_MAN_CLK_ENABLE,
            1);
        REGIO_WRITE_FIELD(ui32RegMsvdxManClkEnable,
            MSVDX_CORE, CR_MSVDX_MAN_CLK_ENABLE, CR_VEC_ENTDEC_MAN_CLK_ENABLE,
            1);
        REGIO_WRITE_FIELD(ui32RegMsvdxManClkEnable,
            MSVDX_CORE, CR_MSVDX_MAN_CLK_ENABLE, CR_VEC_ITRANS_MAN_CLK_ENABLE,
            1);
    }

    // We now can turn the clocks on flag for msvdxio context before tuning clocks on
    if(!psContext->bClocks)
    {
        psContext->bClocks = IMG_TRUE;
    }

    REGIO_WRITE_REGISTER(psContext, MSVDX_CORE, CR_MSVDX_MAN_CLK_ENABLE,
        MSVDX_CORE_CR_MSVDX_MAN_CLK_ENABLE_CR_CORE_MAN_CLK_ENABLE_MASK);

    // Make sure all the clocks are off except core
    //IMG_ASSERT(REGIO_READ_REGISTER(psContext, MSVDX_CORE, CR_MSVDX_MAN_CLK_ENABLE) == MSVDX_CORE_CR_MSVDX_MAN_CLK_ENABLE_CR_CORE_MAN_CLK_ENABLE_MASK);

    REGIO_WRITE_REGISTER(psContext, MSVDX_CORE, CR_MSVDX_MAN_CLK_ENABLE,
        ui32RegMsvdxManClkEnable);

    if (bExtClockGating)
    {
        REGIO_WRITE_REGISTER(psContext, MSVDX_CORE, CR_MSVDX_MAN_CLK_ENABLE2,
            ui32RegMsvdxManClkEnable2);
    }

    DEBUG_REPORT(REPORT_MODULE_VXDIO, "MSVDX Clocks Enabled");

    return IMG_SUCCESS;
}

/*
******************************************************************************

 @Function              MSVDXIO_SEC_DeInitialise

******************************************************************************/
IMG_RESULT
MSVDXIO_SEC_DeInitialise(
    IMG_HANDLE          hContext
)
{
    MSVDXIO_sContext  * psContext = (MSVDXIO_sContext *)hContext;
    IMG_RESULT          ui32Result;

    if (hContext == IMG_NULL)
    {
        REPORT(REPORT_MODULE_VXDIO, REPORT_ERR,
               "An MSVDX context handle must be provided");
        return IMG_ERROR_INVALID_PARAMETERS;
    }

    if (psContext->sSecFwBufInfo.pvCpuVirt)
    {
        SECMEM_UnmapSecureMemory(psContext->sSecFwBufInfo.pvCpuVirt);
        psContext->sSecFwBufInfo.pvCpuVirt = IMG_NULL;
    }

    ui32Result = TAL_DeviceUnRegister("MSVDX");
    IMG_ASSERT(ui32Result == IMG_SUCCESS);
    if (ui32Result != IMG_SUCCESS)
    {
        goto error;
    }

    ui32Result = TARGET_Deinitialise(&gsTargetConfig);
    IMG_ASSERT(ui32Result == IMG_SUCCESS);

error:
    IMG_FREE(psContext);

    return ui32Result;
}

/*
******************************************************************************

 @Function              MSVDXIO_SEC_Initialise

******************************************************************************/
IMG_RESULT
MSVDXIO_SEC_Initialise(
    IMG_BOOL               bPostReq,
    IMG_BOOL               bStackUsageTestReq,
    IMG_HANDLE           * phContext
)
{
    MSVDXIO_sContext  * psContext = IMG_NULL;
    IMG_UINT32          ui32Result;
    IMG_HANDLE        * pahMemSpace;
    IMG_UINT32          ui32Reg, ui32MajRev, ui32MinRev, ui32MaintRev;
  
    (void)gsTargetConfig; /* Avoid compiler warning. */

    REPORT(REPORT_MODULE_VXDIO, REPORT_INFO,
        "************** Stream is SECURE ***************");

    if (phContext == IMG_NULL)
    {
        REPORT(REPORT_MODULE_VXDIO, REPORT_ERR,
               "An MSVDX core context handle container must be provided");
        return IMG_ERROR_INVALID_PARAMETERS;
    }

    psContext = IMG_MALLOC(sizeof(*psContext));
    if (psContext == IMG_NULL)
    {
        REPORT(REPORT_MODULE_VXDIO, REPORT_ERR,
               "Failed to allocate memory for MSVDXIO context");
        return IMG_ERROR_OUT_OF_MEMORY;
    }
    IMG_MEMSET(psContext, 0, sizeof(*psContext));

    if (gsTargetConfig.ui32DevNum == 0)
    {
        REPORT(REPORT_MODULE_VXDIO, REPORT_ERR,
               "No devices found");
        ui32Result = IMG_ERROR_FATAL;
        goto error;
    }

    SECDEV_GetRegisterInfo(&gsTargetConfig.pasDevices[0].pvKmRegBase,
                           &gsTargetConfig.pasDevices[0].ui32RegSize);
    if (gsTargetConfig.pasDevices[0].pvKmRegBase == IMG_NULL ||
        gsTargetConfig.pasDevices[0].ui32RegSize == 0)
    {
        REPORT(REPORT_MODULE_VXDIO, REPORT_ERR,
               "Failed to retrieve register mapping info");
        ui32Result = IMG_ERROR_FATAL;
        goto error;
    }

    ui32Result = TARGET_Initialise(&gsTargetConfig);
    if (ui32Result != IMG_SUCCESS)
    {
        REPORT(REPORT_MODULE_VXDIO, REPORT_ERR,
               "Failed to intialise target context");
        goto error;
    }

    pahMemSpace = psContext->pahMemSpace;
    psContext->ui32MemSpaceCount = REGION_MAX;

    pahMemSpace[REGION_VXDSYS_REGSPACE] = TAL_GetMemSpaceHandle("REG_MSVDX_SYS");
    pahMemSpace[REGION_VEC_REGSPACE] = TAL_GetMemSpaceHandle("REG_MSVDX_VEC");
    pahMemSpace[REGION_DMAC_REGSPACE] = TAL_GetMemSpaceHandle("REG_MSVDX_DMAC");
    pahMemSpace[REGION_VDMC_REGSPACE] = TAL_GetMemSpaceHandle("REG_MSVDX_VDMC");
    pahMemSpace[REGION_VDEB_REGSPACE] = TAL_GetMemSpaceHandle("REG_MSVDX_VDEB");
    pahMemSpace[REGION_VLR_REGSPACE] = TAL_GetMemSpaceHandle("REG_MSVDX_VEC_RAM");
#if 0
    pahMemSpace[REGION_VXDPINTEST_REGSPACE] = TAL_GetMemSpaceHandle("REG_MSVDX_PIN_TEST");
#endif

    pahMemSpace[REGION_VXDMTX_REGSPACE] = TAL_GetMemSpaceHandle("REG_MSVDX_MTX");
    pahMemSpace[REGION_VXDMTX_MEMSPACE] = TAL_GetMemSpaceHandle("REG_MSVDX_MTX");
    pahMemSpace[REGION_VXDSYS_MEMSPACE] = TAL_GetMemSpaceHandle("REG_MSVDX_SYS");
    pahMemSpace[REGION_VLRBE_REGSPACE] = TAL_GetMemSpaceHandle("REG_MSVDX_VEC_RAM_BE");
    pahMemSpace[REGION_VLRFE_REGSPACE] = TAL_GetMemSpaceHandle("REG_MSVDX_VEC_RAM_FE");
#ifdef SECURE_DEVICE_FPGA_TEST
    pahMemSpace[REGION_VXDTEST_3_REGSPACE] = TAL_GetMemSpaceHandle(acMemSpcePrefix, "REG_MSVDX_TEST_3");
#endif /* SECURE_DEVICE_FPGA_TEST */
#ifdef POST_TEST
    psContext->bPost = bPostReq;
#endif /* POST_TEST */
#ifdef STACK_USAGE_TEST
    psContext->bStackUsageTest = bStackUsageTestReq;
#endif /* STACK_USAGE_TEST */

    psContext->bInitialised = IMG_TRUE;

    // Initially turn the clocks on so that
    // general registers can be accessed.
    ui32Result = MSVDXIO_SEC_EnableClocks(psContext, IMG_FALSE, IMG_FALSE, IMG_FALSE);
    IMG_ASSERT(ui32Result == IMG_SUCCESS);
    if (ui32Result != IMG_SUCCESS)
    {
        goto error;
    }

    /* Get MTX RAM info for this core. */
    ui32Result = msvdxio_MTXGetRamInfo(psContext, &psContext->sMTXRamInfo);
    IMG_ASSERT(ui32Result == IMG_SUCCESS);
    if (ui32Result != IMG_SUCCESS)
    {
        goto error;
    }

    /* Core Revision Information */
    ui32Reg = REGIO_READ_REGISTER(psContext, MSVDX_CORE, CR_MSVDX_CORE_REV);
    ui32MajRev = REGIO_READ_FIELD(ui32Reg, MSVDX_CORE, CR_MSVDX_CORE_REV, CR_MSVDX_MAJOR_REV);
    ui32MinRev = REGIO_READ_FIELD(ui32Reg, MSVDX_CORE, CR_MSVDX_CORE_REV, CR_MSVDX_MINOR_REV);
    ui32MaintRev = REGIO_READ_FIELD(ui32Reg, MSVDX_CORE, CR_MSVDX_CORE_REV, CR_MSVDX_MAINT_REV);

    // Use the new test register space from 4.0.3 onwards.
    if (ui32MajRev > 4 ||
        (ui32MajRev == 4 && ui32MinRev > 0) ||
        (ui32MajRev == 4 && ui32MinRev == 0 && ((IMG_INT32)(ui32MaintRev) >= 3)))
    {
        pahMemSpace[REGION_VXDTEST_REGSPACE] =
            TAL_GetMemSpaceHandle("REG_MSVDX_TEST_2");
    }
    else
    {
        pahMemSpace[REGION_VXDTEST_REGSPACE] =
            TAL_GetMemSpaceHandle("REG_MSVDX_TEST");
    }


#ifdef SECURE_MEM_ACCESS
    {
        IMG_UINT32 ui32Reg = 0;
        //Secure Memory
        REGIO_WRITE_FIELD(ui32Reg, MSVDX_TEST_3, SECURE_MEM_CTRL_0, SECURE_MEM_BOUNDARY,
                          SECURE_MEMORY_OFFSET / 0x1000);
        REGIO_WRITE_FIELD(ui32Reg, MSVDX_TEST_3, SECURE_MEM_CTRL_0, MEM_PROTECT_ENABLE, 1);
        REGIO_WRITE_REGISTER(psContext, MSVDX_TEST_3, SECURE_MEM_CTRL_0, ui32Reg);
    }
#endif /* SECURE_MEM_ACCESS */


#ifdef SECURE_CPU_REG_ACCESS
    {
        IMG_UINT32 ui32Reg = 0;
        //Secure Reg Access
        REGIO_WRITE_FIELD(ui32Reg, MSVDX_TEST_3, SECURE_CPU_CTRL_0, SECURE_CPU_ENABLE, 1);
        REGIO_WRITE_REGISTER(psContext, MSVDX_TEST_3, SECURE_CPU_CTRL_0, ui32Reg);
        bSecEnable = IMG_TRUE;
    }
#endif /* SECURE_CPU_REG_ACCESS */

    *phContext = psContext;

    return IMG_SUCCESS;

error:

    if(psContext)
    {
        IMG_FREE(psContext);
    }

    return ui32Result;
}

/*
******************************************************************************

 @Function              MSVDXIO_SEC_SetSecRegAccess

******************************************************************************/
IMG_VOID
MSVDXIO_SEC_SetSecRegAccess(
    const IMG_HANDLE       hContext
)
{
#ifdef SECURE_CPU_REG_ACCESS
    const MSVDXIO_sContext    * psContext = (const MSVDXIO_sContext *)hContext;
    IMG_UINT32                  ui32Reg;
    IMG_UINT32                  ui32SecRegVal = bSecEnable | SECURE_REG_ACCESS;
    IMG_UINT32                  ui32Result;

    if (hContext == IMG_NULL)
    {
        REPORT(REPORT_MODULE_VXDIO, REPORT_ERR,
               "An MSVDX context handle must be provided");
        return;
    }

    TALREG_WriteWord32(psContext->pahMemSpace[REGION_VXDTEST_3_REGSPACE],
                       MSVDX_TEST_3_SECURE_CPU_CTRL_0_OFFSET, ui32SecRegVal);
#endif /* SECURE_CPU_REG_ACCESS */
}

/*!
******************************************************************************

 @Function              MSVDXIO_SEC_VLRWriteWords

******************************************************************************/
IMG_RESULT
MSVDXIO_SEC_VLRWriteWords(
    const IMG_HANDLE    hContext,
    IMG_UINT32          ui32MemRegion,
    IMG_UINT32          ui32Addr,
    IMG_UINT32          ui32NumWords,
    const IMG_UINT32  * pui32Values
)
{
    const MSVDXIO_sContext    * psContext = (const MSVDXIO_sContext *)hContext;
    IMG_UINT32                  ui32WordCnt;
    IMG_UINT32                  ui32Result;

    if (hContext == IMG_NULL)
    {
        REPORT(REPORT_MODULE_VXDIO, REPORT_ERR,
               "An MSVDX context handle must be provided");
        return IMG_ERROR_INVALID_PARAMETERS;
    }

    if ((ui32Addr & 0x3) != 0 ||
        ui32NumWords == 0 ||
        pui32Values == IMG_NULL)
    {
        REPORT(REPORT_MODULE_VXDIO, REPORT_ERR,
               "A valid VLR location (offset word aligned) must be provided");
        return IMG_ERROR_INVALID_PARAMETERS;
    }

    // Repeat for all words.
    for (ui32WordCnt = 0; ui32WordCnt < ui32NumWords; ui32WordCnt++)
    {
#ifdef SECURE_CPU_REG_ACCESS
        {
            MSVDXIO_SetSecRegAccess(hContext);
        }
#endif /* SECURE_CPU_REG_ACCESS */
        ui32Result = TALREG_WriteWord32(psContext->pahMemSpace[ui32MemRegion],
                                        ui32Addr,
                                        *pui32Values);
        if (ui32Result != IMG_SUCCESS)
        {
            return ui32Result;
        }

        pui32Values++;
        ui32Addr += 4;
    }

    return IMG_SUCCESS;
}


/*!
******************************************************************************

 @Function              MSVDXIO_SEC_VLRReadWords

******************************************************************************/
IMG_RESULT
MSVDXIO_SEC_VLRReadWords(
    const IMG_HANDLE    hContext,
    IMG_UINT32          ui32MemRegion,
    IMG_UINT32          ui32Addr,
    IMG_UINT32          ui32NumWords,
    IMG_UINT32        * pui32Values,
    IMG_BOOL            bValidate
)
{
    const MSVDXIO_sContext    * psContext = (const MSVDXIO_sContext *)hContext;
    IMG_UINT32                  ui32WordCnt = 0;
    IMG_UINT32                  ui32Result = IMG_SUCCESS;

    if (hContext == IMG_NULL)
    {
        REPORT(REPORT_MODULE_VXDIO, REPORT_ERR,
               "An MSVDX context handle must be provided");
        return IMG_ERROR_INVALID_PARAMETERS;
    }

    if ((ui32Addr & 0x3) != 0 ||
        ui32NumWords == 0 ||
        pui32Values == IMG_NULL)
    {
        REPORT(REPORT_MODULE_VXDIO, REPORT_ERR,
               "A valid VLR location (offset word aligned) must be provided");
        return IMG_ERROR_INVALID_PARAMETERS;
    }

    // Repeat for all words.
    for (ui32WordCnt = 0; ui32WordCnt < ui32NumWords; ui32WordCnt++)
    {
#ifdef SECURE_CPU_REG_ACCESS
        {
            MSVDXIO_SetSecRegAccess(hContext);
        }
#endif /* SECURE_CPU_REG_ACCESS */

        ui32Result = TALREG_ReadWord32(psContext->pahMemSpace[ui32MemRegion],
                                       ui32Addr,
                                       pui32Values);
        if (ui32Result != IMG_SUCCESS)
        {
            return ui32Result;
        }

        pui32Values++;
        ui32Addr += 4;
    }

    return IMG_SUCCESS;
}

/*!
******************************************************************************

 @Function              MSVDXIO_SEC_ReadRegister

******************************************************************************/
IMG_UINT32
MSVDXIO_SEC_ReadRegister(
    const IMG_HANDLE    hContext,
    IMG_UINT32          ui32MemRegion,
    IMG_UINT32          ui32Offset
)
{
    const MSVDXIO_sContext    * psContext = (const MSVDXIO_sContext *)hContext;
    IMG_UINT32                  ui32Value = 0;

    if (hContext == IMG_NULL)
    {
        REPORT(REPORT_MODULE_VXDIO, REPORT_ERR,
               "An MSVDX context handle must be provided");
        return IMG_ERROR_INVALID_PARAMETERS;
    }

    IMG_ASSERT(psContext->ui32MemSpaceCount != 0);
    if (ui32MemRegion >= psContext->ui32MemSpaceCount)
    {
        REPORT(REPORT_MODULE_VXDIO, REPORT_ERR,
               "A valid VXD memory region must be provided");
        return IMG_ERROR_INVALID_PARAMETERS;
    }

    IMG_ASSERT(psContext->bInitialised);

    if (psContext->bClocks)
    {
#ifdef SECURE_CPU_REG_ACCESS
        MSVDXIO_SEC_SetSecRegAccess(hContext);
#endif /* SECURE_CPU_REG_ACCESS */
        TALREG_ReadWord32(psContext->pahMemSpace[ui32MemRegion], ui32Offset, &ui32Value);
    }
    else
    {
        REPORT(REPORT_MODULE_VXDIO,
               REPORT_NOTICE,
               "Didn't READ register while clocks are turned off");
    }

    return ui32Value;
}


/*!
******************************************************************************

 @Function              MSVDXIO_SEC_WriteRegister

******************************************************************************/
IMG_RESULT
MSVDXIO_SEC_WriteRegister(
    const IMG_HANDLE    hContext,
    IMG_UINT32          ui32MemRegion,
    IMG_UINT32          ui32Offset,
    IMG_UINT32          ui32Value,
    IMG_UINT32          ui32Mask
)
{
    const MSVDXIO_sContext * psContext = (const MSVDXIO_sContext *)hContext;

    if (hContext == IMG_NULL)
    {
        REPORT(REPORT_MODULE_VXDIO, REPORT_ERR,
               "An MSVDX context handle must be provided");
        return IMG_ERROR_INVALID_PARAMETERS;
    }

    IMG_ASSERT(psContext->ui32MemSpaceCount != 0);
    if (ui32MemRegion >= psContext->ui32MemSpaceCount)
    {
        REPORT(REPORT_MODULE_VXDIO, REPORT_ERR,
               "A valid VXD memory region must be provided");
        return IMG_ERROR_INVALID_PARAMETERS;
    }

    IMG_ASSERT(psContext->bInitialised);

    if (psContext->bClocks)
    {
        IMG_UINT32 ui32RegValue;

        if (ui32Mask != -1)
        {
            TALREG_ReadWord32(psContext->pahMemSpace[ui32MemRegion], ui32Offset, &ui32RegValue);
            ui32RegValue &= ~ui32Mask;
            ui32RegValue |= ui32Value;
        }
        else
        {
            ui32RegValue = ui32Value;
        }
#ifdef SECURE_CPU_REG_ACCESS
        {
            MSVDXIO_SEC_SetSecRegAccess(hContext);
        }
#endif /* SECURE_CPU_REG_ACCESS */
        TALREG_WriteWord32(psContext->pahMemSpace[ui32MemRegion], ui32Offset, ui32RegValue);
    }
    else
    {
        REPORT(REPORT_MODULE_VXDIO, REPORT_NOTICE,
            "Didn't WRITE register while clocks are turned off");
        return IMG_ERROR_NOT_INITIALISED;
    }

    return IMG_SUCCESS;
}

/*!
******************************************************************************

 @Function              MSVDXIO_SEC_GetCoreProps

******************************************************************************/
IMG_RESULT
MSVDXIO_SEC_GetCoreProps(
    const IMG_HANDLE      hContext,
    MSVDXIO_sCoreRegs   * psCoreRegs
)
{
    if (hContext == IMG_NULL)
    {
        return IMG_ERROR_INVALID_PARAMETERS;
    }

    psCoreRegs->ui32CoreRev =
        REGIO_READ_REGISTER(hContext, MSVDX_CORE, CR_MSVDX_CORE_REV);
    psCoreRegs->ui32Internal =
        REGIO_READ_REGISTER(hContext, MSVDX_CORE, CR_MSVDX_INTERNAL);
    psCoreRegs->ui32Latency =
        REGIO_READ_REGISTER(hContext, MSVDX_CORE, CR_MSVDX_LATENCY_CONFIG);
    psCoreRegs->ui32MmuStatus =
        REGIO_READ_REGISTER(hContext, MSVDX_CORE, CR_MMU_STATUS);
    psCoreRegs->ui32CoreId =
        REGIO_READ_REGISTER(hContext, MSVDX_CORE, CR_MSVDX_CORE_ID);
    psCoreRegs->ui32MultiCore =
        REGIO_READ_REGISTER(hContext, MSVDX_CORE, CR_MSVDX_MULTICORE_STATUS);

    return IMG_SUCCESS;
}

/*!
******************************************************************************

 @Function              MSVDXIO_SEC_ResetCore

******************************************************************************/
IMG_RESULT
MSVDXIO_SEC_ResetCore(
    const IMG_HANDLE    hContext,
    IMG_BOOL            bAutoClockGating,
    IMG_BOOL            bExtClockGating,
    IMG_BOOL            bClocksEnable
)
{
    MSVDXIO_sContext  * psContext = (MSVDXIO_sContext *)hContext;
    IMG_UINT32          ui32Reg;
    IMG_UINT32          i;
    IMG_UINT32          ui32Result = IMG_SUCCESS;

    if (psContext == IMG_NULL)
    {
        REPORT(REPORT_MODULE_VXDIO, REPORT_ERR,
               "An MSVDX context handle must be provided");
        return IMG_ERROR_INVALID_PARAMETERS;
    }

    DEBUG_REPORT(REPORT_MODULE_VXDIO, "Doing MSVDX Reset, clocks: %u", bClocksEnable);

    ui32Result = MSVDXIO_SEC_EnableClocks(hContext,
                                          bAutoClockGating,
                                          bExtClockGating,
                                          IMG_TRUE);
    if (ui32Result != IMG_SUCCESS)
    {
        REPORT(REPORT_MODULE_VXDIO, REPORT_ERR,
               "Failed to enable clocks");
        return ui32Result;
    }

    /* Clear the interrupt enable to minimize chance of getting spurious
     * interrupts. */
    REGIO_WRITE_REGISTER(psContext, MSVDX_CORE, CR_MSVDX_HOST_INTERRUPT_ENABLE, 0);

    // Pause the MMU.
    ui32Reg = REGIO_READ_REGISTER(psContext, MSVDX_CORE, CR_MMU_CONTROL0);
    REGIO_WRITE_FIELD(ui32Reg, MSVDX_CORE, CR_MMU_CONTROL0, CR_MMU_PAUSE, 1);
    REGIO_WRITE_REGISTER(psContext, MSVDX_CORE, CR_MMU_CONTROL0, ui32Reg);

    // Stop MTX.
    ui32Reg = REGIO_READ_REGISTER(psContext, MTX_CORE, CR_MTX_SOFT_RESET);
    REGIO_WRITE_FIELD(ui32Reg, MTX_CORE, CR_MTX_SOFT_RESET, MTX_RESET, 1);
    REGIO_WRITE_REGISTER(psContext, MTX_CORE, CR_MTX_SOFT_RESET, ui32Reg);

    /* Handle MMU faults. */
    ui32Reg = REGIO_READ_REGISTER(psContext, MSVDX_CORE, CR_MSVDX_INTERRUPT_STATUS);
    if (ui32Reg & MSVDX_CORE_CR_MSVDX_INTERRUPT_STATUS_CR_MMU_FAULT_IRQ_MASK)
    {
        IMG_UINT32 ui32PageFault;

        /* Obtain MMU status. */
        ui32Reg = REGIO_READ_REGISTER(psContext, MSVDX_CORE, CR_MMU_STATUS);
        ui32PageFault = REGIO_READ_FIELD(ui32Reg, MSVDX_CORE, CR_MMU_STATUS, CR_MMU_PF_N_RW);

        if (ui32PageFault)
        {
            /* Invalidate the MMU. */
            ui32Reg = REGIO_READ_REGISTER(psContext, MSVDX_CORE, CR_MMU_CONTROL0);
            REGIO_WRITE_FIELD(ui32Reg, MSVDX_CORE, CR_MMU_CONTROL0, CR_MMU_FLUSH, 1);
            REGIO_WRITE_REGISTER(psContext, MSVDX_CORE, CR_MMU_CONTROL0, ui32Reg);

            /* Clear all MMU faults. */
            ui32Reg = 0;
            REGIO_WRITE_FIELD(ui32Reg, MSVDX_CORE, CR_MSVDX_INTERRUPT_STATUS, CR_MMU_FAULT_IRQ, 0xf);
            REGIO_WRITE_REGISTER(psContext, MSVDX_CORE, CR_MSVDX_INTERRUPT_CLEAR, ui32Reg);
        }
    }

    // Check that the outstanding read requests has stabilised at zero.
    for (i = 0; i < 10; i++)
    {
        /* Wait for all outstanding bus reads to complete. */
        ui32Result =
            MSVDXIO_SEC_Poll(psContext,
                             REGION_VXDSYS_REGSPACE,
                             MSVDX_CORE_CR_MMU_MEM_REQ_OFFSET,
                             0,
                             MSVDX_CORE_CR_MMU_MEM_REQ_CR_MEM_REQ_STAT_READS_MASK,
                             MSVDXIO_POLL_EQUAL);

        IMG_ASSERT((ui32Result == IMG_SUCCESS) || (ui32Result == IMG_ERROR_TIMEOUT));
        if ((ui32Result != IMG_SUCCESS) && (ui32Result != IMG_ERROR_TIMEOUT))
        {
            goto error_reset;
        }
    }
    if (ui32Result == IMG_ERROR_TIMEOUT)
    {
        goto error_reset;
    }

    // Disconnect RENDEC decoder from memory.
    ui32Reg = REGIO_READ_REGISTER(psContext, MSVDX_VEC, CR_VEC_RENDEC_CONTROL1);
    REGIO_WRITE_FIELD(ui32Reg, MSVDX_VEC, CR_VEC_RENDEC_CONTROL1, RENDEC_DEC_DISABLE, 1);
    REGIO_WRITE_REGISTER(psContext, MSVDX_VEC, CR_VEC_RENDEC_CONTROL1, ui32Reg);

    /* Issue a s/w reset...*/
    ui32Reg = 0;
    REGIO_WRITE_FIELD(ui32Reg, MSVDX_CORE, CR_MSVDX_CONTROL, CR_MSVDX_FE_SOFT_RESET, 1);
    REGIO_WRITE_FIELD(ui32Reg, MSVDX_CORE, CR_MSVDX_CONTROL, CR_MSVDX_BE_SOFT_RESET, 1);
    REGIO_WRITE_FIELD(ui32Reg, MSVDX_CORE, CR_MSVDX_CONTROL, CR_MSVDX_VDMC_VDEB_SOFT_RESET, 1);
    REGIO_WRITE_FIELD(ui32Reg, MSVDX_CORE, CR_MSVDX_CONTROL, CR_MSVDX_VEC_MEMIF_SOFT_RESET, 1);
    REGIO_WRITE_FIELD(ui32Reg, MSVDX_CORE, CR_MSVDX_CONTROL, CR_MSVDX_VEC_RENDEC_DEC_SOFT_RESET, 1);
    REGIO_WRITE_REGISTER(psContext, MSVDX_CORE, CR_MSVDX_CONTROL, ui32Reg);

    // Read back the MSVDX control register to allow the hardware to detect the reset.
    REGIO_READ_REGISTER(psContext, MSVDX_CORE, CR_MSVDX_CONTROL);

    /* Manually clear all the reset bits. */
    ui32Reg = REGIO_READ_REGISTER(psContext, MSVDX_CORE, CR_MSVDX_CONTROL);
    REGIO_WRITE_FIELD(ui32Reg, MSVDX_CORE, CR_MSVDX_CONTROL, CR_MSVDX_FE_SOFT_RESET, 0);
    REGIO_WRITE_FIELD(ui32Reg, MSVDX_CORE, CR_MSVDX_CONTROL, CR_MSVDX_BE_SOFT_RESET, 0);
    REGIO_WRITE_FIELD(ui32Reg, MSVDX_CORE, CR_MSVDX_CONTROL, CR_MSVDX_VDMC_VDEB_SOFT_RESET, 0);
    REGIO_WRITE_FIELD(ui32Reg, MSVDX_CORE, CR_MSVDX_CONTROL, CR_MSVDX_VEC_MEMIF_SOFT_RESET, 0);
    REGIO_WRITE_FIELD(ui32Reg, MSVDX_CORE, CR_MSVDX_CONTROL, CR_MSVDX_VEC_RENDEC_DEC_SOFT_RESET, 0);
    REGIO_WRITE_REGISTER(psContext, MSVDX_CORE, CR_MSVDX_CONTROL, ui32Reg);

    // Check that the outstanding read requests has stabilised at zero.
    for (i = 0; i < 10; i++)
    {
        /* Wait for all outstanding bus reads to complete. */
        ui32Result =
            MSVDXIO_SEC_Poll(psContext,
                             REGION_VXDSYS_REGSPACE,
                             MSVDX_CORE_CR_MMU_MEM_REQ_OFFSET,
                             0,
                             MSVDX_CORE_CR_MMU_MEM_REQ_CR_MEM_REQ_STAT_READS_MASK,
                             MSVDXIO_POLL_EQUAL);
        IMG_ASSERT((ui32Result == IMG_SUCCESS) || (ui32Result == IMG_ERROR_TIMEOUT));
        if ((ui32Result != IMG_SUCCESS) && (ui32Result != IMG_ERROR_TIMEOUT))
        {
            goto error_reset;
        }
    }
    if(ui32Result == IMG_ERROR_TIMEOUT)
    {
        goto error_reset;
    }

    /* Reset the core (whilst idle). */
    REGIO_WRITE_FIELD(ui32Reg, MSVDX_CORE, CR_MSVDX_CONTROL, CR_MSVDX_SOFT_RESET, 1);
    REGIO_WRITE_REGISTER(psContext, MSVDX_CORE, CR_MSVDX_CONTROL, ui32Reg);

    /* Wait for the reset to complete. */
    ui32Result =
        MSVDXIO_SEC_Poll(psContext,
                         REGION_VXDSYS_REGSPACE,
                         MSVDX_CORE_CR_MSVDX_CONTROL_OFFSET,
                         0,
                         MSVDX_CORE_CR_MSVDX_CONTROL_CR_MSVDX_SOFT_RESET_MASK,
                         MSVDXIO_POLL_EQUAL);
    IMG_ASSERT(ui32Result == IMG_SUCCESS);
    if (ui32Result != IMG_SUCCESS)
    {
        goto error_reset;
    }

    /* Set control register...*/
    ui32Reg = 0;
    REGIO_WRITE_FIELD(ui32Reg, MSVDX_CORE, CR_MSVDX_CONTROL, CR_ENDIAN, MSVDX_ENDIAN);
    REGIO_WRITE_REGISTER(psContext, MSVDX_CORE, CR_MSVDX_CONTROL, ui32Reg);

    /* Clear the interrupt enable...*/
    REGIO_WRITE_REGISTER(psContext, MSVDX_CORE, CR_MSVDX_HOST_INTERRUPT_ENABLE, 0);

    /* Clear any pending interrupts...*/
    REGIO_WRITE_REGISTER(psContext, MSVDX_CORE, CR_MSVDX_INTERRUPT_CLEAR, 0xFFFFFFFF);

error_reset:
    /* Registering for intercepting MMU interrupts on the host */
    ui32Reg = REGIO_READ_REGISTER(psContext, MSVDX_CORE, CR_MSVDX_HOST_INTERRUPT_ENABLE) ;
    REGIO_WRITE_REGISTER(psContext, MSVDX_CORE, CR_MSVDX_HOST_INTERRUPT_ENABLE,
        ui32Reg | MSVDX_CORE_CR_MSVDX_INTERRUPT_STATUS_CR_MMU_FAULT_IRQ_MASK );

    DEBUG_REPORT(REPORT_MODULE_VXDIO, "MSVDX Reset Completed, res: %u",
                 ui32Result);

    if (!bClocksEnable)
    {
        ui32Result = MSVDXIO_SEC_DisableClocks(hContext);
    }
    else
    {
        /* Re-enable clocks with force manual disabled. */
        ui32Result = MSVDXIO_SEC_EnableClocks(hContext,
                                              bAutoClockGating,
                                              bExtClockGating,
                                              IMG_FALSE);
    }

    return ui32Result;
}


/*!
******************************************************************************

 @Function              MSVDXIO_SEC_SendFirmwareMessage

******************************************************************************/
IMG_RESULT
MSVDXIO_SEC_FlushMmu(
    const IMG_HANDLE            hContext
)
{
    IMG_UINT32 ui32Reg;

    if (hContext == IMG_NULL)
    {
        return IMG_ERROR_INVALID_PARAMETERS;
    }

    ui32Reg = REGIO_READ_REGISTER(hContext, MSVDX_CORE, CR_MMU_CONTROL0);
    REGIO_WRITE_FIELD(ui32Reg, MSVDX_CORE, CR_MMU_CONTROL0, CR_MMU_INVALDC, 1);
    REGIO_WRITE_REGISTER(hContext, MSVDX_CORE, CR_MMU_CONTROL0, ui32Reg);

    REGIO_WRITE_FIELD(ui32Reg, MSVDX_CORE, CR_MMU_CONTROL0, CR_MMU_INVALDC, 0);
    REGIO_WRITE_REGISTER(hContext, MSVDX_CORE, CR_MMU_CONTROL0, ui32Reg);

    return IMG_SUCCESS;
}
/* EOF */
