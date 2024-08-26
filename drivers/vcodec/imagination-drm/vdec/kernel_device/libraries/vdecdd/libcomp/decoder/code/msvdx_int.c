/*!
 *****************************************************************************
 *
 * @File       msvdx_int.c
 * @Title      Low-level MSVDX interface component
 * @Description    This file contains functions to interface with an MSVDX core.
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

#include "img_defs.h"
#include "msvdx_int.h"
#include "msvdx_ctx.h"
#include "msvdx_msg.h"
#include "report_api.h"
#include "secure_defs.h"
#include "secure_msg.h"
#include "dbgopt_api_km.h"
#include "sysos_api_km.h"
#include "vdecdd_mmu.h"
#include "msvdx_ext_reg.h"
#include "msvdx_ext.h"

#include "secureapi_ree.h"
/* No secure path */
#include "hwctrl_int.h"

#include "reg_io2.h"
#include "msvdx_cmds_io2.h"
#include "msvdx_core_regs_io2.h"
#include "msvdx_vec_reg_io2.h"
#include "msvdx_vdmc_reg_io2.h"
#include "msvdx_mtx_reg_io2.h"
#include "img_soc_dmac_regs_io2.h"
#include "msvdx_test_reg_io2.h"
#include "msvdx_pin_test_regs.h"
#include "msvdx_vdeb_reg_io2.h"
#include "vdecfw_msg_mem_io.h"

#ifdef __FAKE_MTX_INTERFACE__
    #include "vdecfw_fakemtx.h"
#endif

#include "vdecfw_bin.h"
#ifdef POST_TEST
    /* CORE test definitions */
    #define GROUP_ID_DEFAULT_VALUE            ((IMG_UINT32)3)
    #define CORE_ID_DEFAULT_VALUE             ((IMG_UINT32)3)
    #define INVALID_32BIT_REG_VALUE           ((IMG_UINT32)0xFFFFFFFF)

    /* Registers test definitions */
    /* Test fields values */
    #define TEST_REG_FIELD_1BIT_VALUE         ((IMG_UINT32)1)
    #define TEST_REG_FIELD_2BITS_VALUE        ((IMG_UINT32)3)
    #define TEST_REG_FIELD_3BITS_VALUE        ((IMG_UINT32)7)
    #define TEST_REG_FIELD_8BITS_VALUE        ((IMG_UINT32)0xFF)
    #define TEST_REG_FIELD_26BITS_VALUE       ((IMG_UINT32)0x03FFFFFF)
    /* Default registers values */
    #define CR_MTX_REG_RW_REQ_DEF_VALUE       ((IMG_UINT32)0x80000000)
    #define CR_VEC_RENDEC_CONTROL1_DEF_VALUE  ((IMG_UINT32)0x00050001)
    #define CR_MMU_BANK_INDEX_DEF_VALUE       ((IMG_UINT32)0x00000000)
    #define CR_VDMC_2D_FLT_DATA_DEF_VALUE     ((IMG_UINT32)0x00000000)
    #define CR_VDEB_VIRT_CTRL_DEF_VALUE       ((IMG_UINT32)0x00000000)
#endif

#define GET_BITS(v, lb, n)   ((v >> lb) & ((1 << n) - 1))


/*!
******************************************************************************
 This struct contains REAL RPR parameters
******************************************************************************/
typedef struct
{
    IMG_INT32 i32AxInitial;
    IMG_INT32 i32AxIncrement;
    IMG_INT32 i32AyInitial;
    IMG_INT32 i32AyIncrement;

} TRANSLATION_sRprParams;


/*!
******************************************************************************
 MSVDX core version information.
 NOTE: It is assumed that all cores on a device are the same revision.
******************************************************************************/
static IMG_UINT32  ui32GroupId;   /*!< IMG Group ID.                   */
static IMG_UINT32  ui32CoreId;    /*!< VXD Core ID.                    */
static IMG_UINT32  ui32MajRev;    /*!< VXD Core major revision.        */
static IMG_UINT32  ui32MinRev;    /*!< VXD Core minor revision.        */
static IMG_UINT32  ui32MaintRev;  /*!< VXD Core maintenance revision.  */
static IMG_UINT32  ui32IntRev;    /*!< VXD internal revision.          */


/*!
******************************************************************************
 This array defines the properties of each video standard for various core
 versions (and on). These are defaults and may be overriden by features signalled
 by specific hardware cores.

 VXD380
 VXD390     3.0.0
 VXD391     4.0.0
 VXD392     5.0.0
 VXD393     5.5.0
 D4500      6.0.0
 D5500      7.0.0

*****************************************************************************/
static VXD_sVidStdProps asStdProps[] =
{
    { VDEC_STD_MPEG2,       CORE_REVISION(0,0,0), 80, 16, 2048, 2048, 0,        8, 8, PIXEL_FORMAT_420 },
    { VDEC_STD_MPEG2,       CORE_REVISION(5,0,0), 96, 16, 4096, 4096, 0,        8, 8, PIXEL_FORMAT_420 },
    { VDEC_STD_MPEG2,       CORE_REVISION(6,0,0), 64, 16, 4096, 4096, 0,        8, 8, PIXEL_FORMAT_420 },

    { VDEC_STD_MPEG4,       CORE_REVISION(0,0,0), 80, 16, 2048, 2048, 0,        8, 8, PIXEL_FORMAT_420 },
    { VDEC_STD_MPEG4,       CORE_REVISION(5,0,0), 96, 16, 4096, 4096, 0,        8, 8, PIXEL_FORMAT_420 },
    { VDEC_STD_MPEG4,       CORE_REVISION(6,0,0), 64, 16, 4096, 4096, 0,        8, 8, PIXEL_FORMAT_420 },

    { VDEC_STD_H263,        CORE_REVISION(0,0,0), 80, 16, 2048, 2048, 0,        8, 8, PIXEL_FORMAT_420 },
    { VDEC_STD_H263,        CORE_REVISION(5,0,0), 96, 16, 4096, 4096, 0,        8, 8, PIXEL_FORMAT_420 },
    { VDEC_STD_H263,        CORE_REVISION(6,0,0), 64, 16, 4096, 4096, 0,        8, 8, PIXEL_FORMAT_420 },

    { VDEC_STD_H264,        CORE_REVISION(0,0,0), 80, 16, 1920, 2048, 0x2000,   8, 8, PIXEL_FORMAT_420 },
    { VDEC_STD_H264,        CORE_REVISION(5,0,0), 96, 16, 4096, 4096, 0x10000,  8, 8, PIXEL_FORMAT_420 },
    { VDEC_STD_H264,        CORE_REVISION(6,0,0), 64, 16, 4096, 4096, 0x10000,  8, 8, PIXEL_FORMAT_420 },

    { VDEC_STD_VC1,         CORE_REVISION(0,0,0), 80, 16, 2048, 2048, 0,        8, 8, PIXEL_FORMAT_420 },
    { VDEC_STD_VC1,         CORE_REVISION(5,0,0), 96, 16, 4096, 4096, 0,        8, 8, PIXEL_FORMAT_420 },
    { VDEC_STD_VC1,         CORE_REVISION(6,0,0), 80, 16, 4096, 4096, 0,        8, 8, PIXEL_FORMAT_420 },

    { VDEC_STD_AVS,         CORE_REVISION(0,0,0), 80, 16, 2048, 2048, 0,        8, 8, PIXEL_FORMAT_420 },
    { VDEC_STD_AVS,         CORE_REVISION(5,0,0), 96, 16, 4096, 4096, 0,        8, 8, PIXEL_FORMAT_420 },
    { VDEC_STD_AVS,         CORE_REVISION(6,0,0), 64, 16, 4096, 4096, 0,        8, 8, PIXEL_FORMAT_420 },

    { VDEC_STD_REAL,        CORE_REVISION(4,0,0), 80, 16, 2048, 2048, 0,        8, 8, PIXEL_FORMAT_420 },
    { VDEC_STD_REAL,        CORE_REVISION(5,0,0), 96, 16, 4096, 4096, 0,        8, 8, PIXEL_FORMAT_420 },
    { VDEC_STD_REAL,        CORE_REVISION(6,0,0), 64, 16, 4096, 4096, 0,        8, 8, PIXEL_FORMAT_420 },

    { VDEC_STD_JPEG,        CORE_REVISION(4,0,0), 80, 16, 32768, 32768, 0,      8, 8, PIXEL_FORMAT_444 },
    { VDEC_STD_JPEG,        CORE_REVISION(5,0,0), 96, 16, 32768, 32768, 0,      8, 8, PIXEL_FORMAT_444 },
    { VDEC_STD_JPEG,        CORE_REVISION(6,0,0), 64, 16, 32768, 32768, 0,      8, 8, PIXEL_FORMAT_444 },

    { VDEC_STD_VP6,         CORE_REVISION(4,0,0), 80, 16, 2048, 2048, 0,        8, 8, PIXEL_FORMAT_420 },
    { VDEC_STD_VP6,         CORE_REVISION(5,0,0), 96, 16, 4096, 4096, 0,        8, 8, PIXEL_FORMAT_420 },
    { VDEC_STD_VP6,         CORE_REVISION(6,0,0), 64, 16, 4096, 4096, 0,        8, 8, PIXEL_FORMAT_420 },

    { VDEC_STD_VP8,         CORE_REVISION(5,0,0), 96, 16, 4096, 4096, 0,        8, 8, PIXEL_FORMAT_420 },
    { VDEC_STD_VP8,         CORE_REVISION(6,0,0), 64, 16, 4096, 4096, 0,        8, 8, PIXEL_FORMAT_420 },

    { VDEC_STD_SORENSON,    CORE_REVISION(0,0,0), 80, 16, 2048, 2048, 0,        8, 8, PIXEL_FORMAT_420 },
    { VDEC_STD_SORENSON,    CORE_REVISION(5,0,0), 96, 16, 4096, 4096, 0,        8, 8, PIXEL_FORMAT_420 },
    { VDEC_STD_SORENSON,    CORE_REVISION(6,0,0), 64, 16, 4096, 4096, 0,        8, 8, PIXEL_FORMAT_420 },

    { VDEC_STD_HEVC,        CORE_REVISION(7,0,0), 64, 16, 8192, 8192, 0,        8, 8, PIXEL_FORMAT_420 },
};


#ifndef IMG_KERNEL_MODULE
/*!
******************************************************************************

 @Function              msvdx_PollPdump

******************************************************************************/
static IMG_RESULT
msvdx_PollPdump(
    const MSVDX_sContext  * psContext,
    MEM_REGIONS             eMemRegion,
    IMG_UINT32              ui32Offset,
    IMG_UINT32              ui32RequValue,
    IMG_UINT32              ui32Enable
)
{
    IMG_UINT32      ui32Result = IMG_SUCCESS;

    if (psContext->bSecureStream == IMG_FALSE)
    {
        ui32Result = VXDIO_PDUMPVerifPoll(psContext->hMsvdxIo,
                                        eMemRegion,
                                        ui32Offset,
                                        ui32RequValue,
                                        ui32Enable);
    }

    IMG_ASSERT(ui32Result == IMG_SUCCESS);

    return ui32Result;
}
#endif



/*!
******************************************************************************

 @Function              msvdx_GetCoreProperties

******************************************************************************/
static IMG_RESULT
msvdx_GetCoreProperties(
    const MSVDX_sContext  * psContext,
    VXD_sCoreProps        * psCoreProps,
    VXD_sInitialiseArgs   * psIntArgs
    )
{
    IMG_UINT32      ui32Reg ,ui32RegInternal;
    IMG_UINT32      ui32CoreConfig;
    IMG_UINT32      ui32LatencyConfig;
    IMG_UINT32      ui32MmuStatusReg;

    IMG_BOOL        bBifWithMmu = IMG_FALSE;
    IMG_BOOL        bMmu36bitCapable;
    IMG_BOOL        bMmu40bitCapable;

    DBGOPT_sValue sVal;
    DBGOPT_eType eType;
    VXD_sCoreProps sCoreProps;
    IMG_UINT32 i;

    /* Core Revision Information */
    ui32Reg = psIntArgs->ui32CoreRev;
    ui32MajRev = REGIO_READ_FIELD(ui32Reg, MSVDX_CORE, CR_MSVDX_CORE_REV, CR_MSVDX_MAJOR_REV);
    ui32MinRev = REGIO_READ_FIELD(ui32Reg, MSVDX_CORE, CR_MSVDX_CORE_REV, CR_MSVDX_MINOR_REV);
    ui32MaintRev = REGIO_READ_FIELD(ui32Reg, MSVDX_CORE, CR_MSVDX_CORE_REV, CR_MSVDX_MAINT_REV);

    /* MSVDX IMG Internal */
    ui32RegInternal = psIntArgs->ui32Internal;
    ui32IntRev = REGIO_READ_FIELD(ui32RegInternal,
        MSVDX_CORE, CR_MSVDX_INTERNAL, CR_MSVDX_INTERNAL);

    /* MSVDX latency config */
    ui32LatencyConfig = psIntArgs->ui32Latency;

    /* MMU status */
    ui32MmuStatusReg = psIntArgs->ui32MmuStatus;

    /* Core ID */
    ui32Reg = psIntArgs->ui32CoreId;
    ui32GroupId = REGIO_READ_FIELD(ui32Reg,
        MSVDX_CORE, CR_MSVDX_CORE_ID, CR_GROUP_ID);
    ui32CoreId = REGIO_READ_FIELD(ui32Reg,
        MSVDX_CORE, CR_MSVDX_CORE_ID, CR_CORE_ID);

    // Ensure that the core is IMG Video Decoder (MSVDX).
    if (ui32GroupId != 3 || ui32CoreId != 3)
    {
        REPORT(REPORT_MODULE_VXDIO, REPORT_ERR,
            "IMG Video Decoder (VXD) not detected");
        return IMG_ERROR_DEVICE_NOT_FOUND;
    }

    ui32CoreConfig = REGIO_READ_FIELD(ui32Reg,
        MSVDX_CORE, CR_MSVDX_CORE_ID, CR_MSVDX_CORE_CONFIG);

    if (DBGOPTKM_GetSize("CoreConfig", DBGOPT_TYPE_BUF) == sizeof(sCoreProps) &&
        DBGOPTKM_Get("CoreConfig", &eType, &sVal))
    {
        *psCoreProps = *((VXD_sCoreProps *) sVal.sBufVal.pvVal);

        DEBUG_REPORT(REPORT_MODULE_VXDIO,
                     "Setting core configuration from debug options");

        goto setting_props_ending;
    }

    VDEC_BZERO(psCoreProps);

    // Construct core version name.
    sprintf(psCoreProps->acVersion, "%d.%d.%d",
            ui32MajRev, ui32MinRev, ui32MaintRev);

    //////////////////////////////////////
    //  Default settings                //
    //////////////////////////////////////

    // Default latency.
    psCoreProps->ui32Latency = 128;

    // Default VDMC cache size.
    psCoreProps->ui32VdmcCacheSize = 1;
    psCoreProps->bVdmcBurst4 = IMG_FALSE;

    // SR DMA burst size
    // Value must be remap`ed for hardware use (2 = 1, 4 = 2, 8 = 3). CR_VEC_CONTROL_2, SR_BURST_SIZE,
    psCoreProps->ui32SrDmaBurstSize = 4;

    // Default master.
    psCoreProps->ui32NumCores = 1;
    psCoreProps->ui32NumPixelPipes = 1;

    // Populate the core properties.
    if (GET_BITS(ui32CoreConfig, 8, 1))
    {
        // Bit implemented in 3.0.2 but
        // some earlier cores (2.x.x onwards)
        // may have rotation capability.
        psCoreProps->abRotationSupport[0] = IMG_TRUE;
    }
    if (GET_BITS(ui32CoreConfig, 9, 1))
    {
        // Bit implemented in 3.0.2
        psCoreProps->abScalingSupport[0] = IMG_TRUE;
    }
    if (GET_BITS(ui32CoreConfig, 11, 1))
    {
        // Bit implemented in 2.0.3
        psCoreProps->bHdSupport = IMG_TRUE;
    }
    if (GET_BITS(ui32CoreConfig, 15, 1))
    {
        bBifWithMmu = IMG_TRUE;
    }

    psCoreProps->aui32NumStreams[0] = GET_BITS(ui32CoreConfig, 12, 2) + 1;

    bMmu36bitCapable = REGIO_READ_FIELD(ui32MmuStatusReg, MSVDX_CORE, CR_MMU_STATUS, CR_MMU_36BIT_CAPABLE);
    bMmu40bitCapable = GET_BITS(ui32LatencyConfig, 26, 1);

    if (bBifWithMmu)
    {
        psCoreProps->eMmuType = MMU_TYPE_32BIT;

        if (bMmu40bitCapable)
        {
            psCoreProps->eMmuType = MMU_TYPE_40BIT;
        }
        else if (bMmu36bitCapable)
        {
            psCoreProps->eMmuType = MMU_TYPE_36BIT;
        }
    }
    else
    {
        psCoreProps->eMmuType = MMU_TYPE_NONE;
    }


#define CORE_ID_SUPPORT(n)      (GET_BITS(ui32CoreConfig, n, 1) ? IMG_TRUE : IMG_FALSE)
#define INTERNAL_SUPPORT(n)     (GET_BITS(ui32RegInternal, n, 1) ? IMG_TRUE : IMG_FALSE)
#define LATENCY_SUPPORT(n)      (GET_BITS(ui32LatencyConfig, n, 1) ? IMG_TRUE : IMG_FALSE)

    // Video standards.
    if (FROM_REV(1,0,9))
    {
        // Features from MSVDX_CORE_ID register.
        psCoreProps->abMpeg2[0] |= CORE_ID_SUPPORT(0);
        psCoreProps->abMpeg4[0] |= CORE_ID_SUPPORT(1);
        psCoreProps->abH264[0]  |= CORE_ID_SUPPORT(2);
        psCoreProps->abVc1[0]   |= CORE_ID_SUPPORT(3);
        psCoreProps->abReal[0] = GET_BITS(ui32RegInternal, 23, 1) ? IMG_TRUE : IMG_FALSE;
        psCoreProps->abReal[0] = GET_BITS(ui32LatencyConfig, 30, 1) ? IMG_TRUE : IMG_FALSE;

        if (BEFORE_REV(3,0,2))
        {
            // Used for REAL from 3.0.2.
            psCoreProps->abJpeg[0]  |= CORE_ID_SUPPORT(5);
        }

        // Features from MSVDX_INTERNAL register.
        psCoreProps->abMpeg2[0] |= INTERNAL_SUPPORT(24);
        psCoreProps->abMpeg4[0] |= INTERNAL_SUPPORT(25);
        psCoreProps->abH264[0]  |= INTERNAL_SUPPORT(26);
        psCoreProps->abVc1[0]   |= INTERNAL_SUPPORT(27);
        psCoreProps->abJpeg[0]  |= INTERNAL_SUPPORT(29);
    }

    if (FROM_REV(2,0,3))
    {
        psCoreProps->abAvs[0]  |= INTERNAL_SUPPORT(31);
    }

    if (FROM_REV(3,0,2))
    {
        if (BEFORE_REV(5,1,0))
        {
            // Deprecated from 5.1.0.
            psCoreProps->abReal[0] |= CORE_ID_SUPPORT(5);
        }
        psCoreProps->abReal[0] |= INTERNAL_SUPPORT(23);

        psCoreProps->abAvs[0]  |= CORE_ID_SUPPORT(10);
    }

    if (FROM_REV(4,1,3))
    {
        psCoreProps->abVp6[0]  |= INTERNAL_SUPPORT(22);
    }

    if (FROM_REV(5,1,0))
    {
        // Features from MSVDX_LATENCY_CONFIG register.
        psCoreProps->abVp8[0]       |= LATENCY_SUPPORT(27);
        psCoreProps->abSorenson[0]  |= LATENCY_SUPPORT(28);
        psCoreProps->abVp6[0]       |= LATENCY_SUPPORT(29);
        psCoreProps->abReal[0]      |= LATENCY_SUPPORT(30);
        psCoreProps->abJpeg[0]      |= LATENCY_SUPPORT(31);

        // Features from MSVDX_INTERNAL register.
        psCoreProps->abSorenson[0]  |= INTERNAL_SUPPORT(20);
        psCoreProps->abVp8[0]       |= INTERNAL_SUPPORT(21);
        psCoreProps->abReal[0]      |= INTERNAL_SUPPORT(23);
    }
    else
    {
        // No flag to signal sorenson before 5.1.0 therefore use MPEG4.
        psCoreProps->abSorenson[0]  |= CORE_ID_SUPPORT(1);        // MPEG4.
        psCoreProps->abSorenson[0]  |= INTERNAL_SUPPORT(25);      // MPEG4.
    }


    if (psCoreProps->abMpeg4[0] && !psCoreProps->abSorenson[0])
    {
        REPORT(REPORT_MODULE_VXDIO, REPORT_WARNING,
            "MPEG4 enabled without Sorenson");
    }

    //////////////////////////////////////
    //  Version specific settings       //
    //////////////////////////////////////

    if (FROM_REV(2,0,0))
    {
        psCoreProps->bAutoClockGatingSupport = IMG_TRUE;
    }

    if (AT_REV(2,1,3))
    {
        psCoreProps->abRotationSupport[0] = IMG_TRUE;
    }

    if (FROM_REV(3,0,0))
    {
        psCoreProps->b64ByteFixedStrides = IMG_TRUE;
        psCoreProps->bExtendedStrides = IMG_TRUE;
        psCoreProps->ui32CmdBufferSize = 64;
    }
    else
    {
        psCoreProps->BRN26832 = IMG_TRUE;
        psCoreProps->ui32CmdBufferSize = 16;
    }

    if (AT_REV(3,0,2) || AT_REV(3,0,5))
    {
        psCoreProps->BRN29797 = IMG_TRUE;
        psCoreProps->BRN29870 = IMG_TRUE;
        psCoreProps->BRN29871 = IMG_TRUE;
        psCoreProps->BRN30095 = IMG_TRUE;
        psCoreProps->BRN30178 = IMG_TRUE;
        psCoreProps->BRN30306 = IMG_TRUE;
    }

    if (BEFORE_REV(3,0,4))
    {
        psCoreProps->BRN28888 = IMG_TRUE;

        if (AT_REV(3,0,3) && ui32IntRev >= 515)
        {
            psCoreProps->BRN28888 = IMG_FALSE;
        }
    }

    if (FROM_REV(3,0,5))
    {
        psCoreProps->bErrorHandling = IMG_TRUE;
        psCoreProps->bErrorConcealment = IMG_TRUE;
    }

    if (BEFORE_REV(4,0,0))
    {
        psCoreProps->BRN32651 = IMG_TRUE;
    }

    if (FROM_REV(4,0,3))
    {
        psCoreProps->bNewTestRegSpace = IMG_TRUE;
    }

    if (FROM_REV(5,0,0))
    {
        psCoreProps->b4kScalingCoeffs = IMG_TRUE;
        psCoreProps->bScalingWithOold = IMG_TRUE;
        psCoreProps->bExtClockGating = IMG_TRUE;
        psCoreProps->bAuxLineBufSupported = IMG_TRUE;

        psCoreProps->bImprovedErrorConcealment = IMG_TRUE;

        psCoreProps->ui32Latency = 384;
        psCoreProps->ui32VdmcCacheSize = 2;
    }
    else
    {
        psCoreProps->BRN29688 = IMG_TRUE;
    }

    // Tourmaline, VXD393
    if (FROM_REV(5,5,0))
    {
        psCoreProps->bNewCacheSettings = IMG_TRUE;
        psCoreProps->ui32VdmcCacheSize = 4;
        psCoreProps->bVdmcBurst4 = IMG_TRUE;
        psCoreProps->bLossless = IMG_TRUE;
    }

    if (FROM_REV(5,5,2))
    {
        psCoreProps->ui32SrDmaBurstSize = 8;
    }

    //Enabling multicore from rev 5.6.x
    if (FROM_REV(5,6,0))
    {
        psCoreProps->ui32ScalingPrecision = 12;

        psCoreProps->bMultiCoreSupport = IMG_TRUE;

        // This may not work/be invalid for slave cores.
        ui32Reg = psIntArgs->ui32MultiCore;
        psCoreProps->ui32NumCores += REGIO_READ_FIELD(ui32Reg,
            MSVDX_CORE, CR_MSVDX_MULTICORE_STATUS, CR_MSVDX_NUM_SLAVES);
    }
    else
    {
        psCoreProps->ui32ScalingPrecision = 11;
    }

    if(FROM_REV(5,6,0) && BEFORE_REV(6,0,0))
    {
        //SunStone/Verdalite
        psCoreProps->bChromaUpsample = IMG_TRUE;
    }

    if (FROM_REV(6,0,0))
    {
        psCoreProps->BRN40493 = IMG_FALSE;
    }
    else
    {
        psCoreProps->BRN40493 = IMG_TRUE;
    }

    //////////////////////////////////////
    //  Override settings               //
    //////////////////////////////////////

    for (i = 0; i < sizeof(asStdProps)/sizeof(asStdProps[0]); i++)
    {
        // Update video standard properties if the core is beyond specified version and
        // the properties are for newer cores than the previous.
        if (FROM_REV(MAJOR_REVISION(asStdProps[i].ui32CoreRev),
                     MINOR_REVISION(asStdProps[i].ui32CoreRev),
                     MAINT_REVISION(asStdProps[i].ui32CoreRev)) &&
            asStdProps[i].ui32CoreRev >= psCoreProps->asVidStdProps[asStdProps[i].eVidStd].ui32CoreRev)
        {
            psCoreProps->asVidStdProps[asStdProps[i].eVidStd] = asStdProps[i];
        }
    }

    // Override video standard-specific properties with anything signalled explicitly by core.
    if (FROM_REV(6,0,0))
    {
        if (LATENCY_SUPPORT(23))
        {
            psCoreProps->asVidStdProps[VDEC_STD_H264].ui32MaxLumaBitDepth = 10;
            psCoreProps->asVidStdProps[VDEC_STD_H264].ui32MaxChromaBitDepth = 10;
        }

        if (LATENCY_SUPPORT(25))
        {
            psCoreProps->asVidStdProps[VDEC_STD_H264].eMaxChromaFormat = PIXEL_FORMAT_422;
        }
        if (LATENCY_SUPPORT(24))
        {
            psCoreProps->asVidStdProps[VDEC_STD_H264].eMaxChromaFormat = PIXEL_FORMAT_444;
        }
    }

    if (!psCoreProps->bAuxLineBufSupported)
    {
        psCoreProps->BRN31777 = IMG_TRUE;
    }

    if (psCoreProps->ui32Latency > 512 && psCoreProps->ui32Latency <= 1024)
    {
        psCoreProps->ui32SrDmaBurstSize = 8; //DMAC_BURST_8;
    }

setting_props_ending:
    // Write trusted host pin
    //TALREG_WriteWord32(psContext->hPinTestMemSpace, 0x148, 1);

    return IMG_SUCCESS;
}


/*!
******************************************************************************

 @Function              msvdx_GetCoreInit

 @Description

 This function prepares data for core initialisation in the firmware.

 @Input     p               : Pointer to MSVDX core properties.

 @Input     psRendec        : Pointer to rendec information.

 @Output    psCoreInitData  : Pointer to core initialisation data.

 @Return    IMG_RESULT      : This function returns either IMG_SUCCESS or an
                              error code.

******************************************************************************/
static IMG_RESULT
msvdx_GetCoreInit(
    const VXD_sCoreProps      * psCoreProps,
    HWCTRL_sRendec            * psRendec,
    VDECFW_sCoreInitData      * psCoreInitData
)
{
    IMG_UINT32              ui32SrBurstSize = 2;
    VDECFW_sMmuConfig     * psMmuConfig;
    VDECFW_sRendecConfig  * psRendecData;

    IMG_ASSERT(psCoreInitData);

    psMmuConfig = &psCoreInitData->sMmuConfig;
    psRendecData = &psCoreInitData->sRendecConfig;

    VDEC_BZERO(psMmuConfig);
    VDEC_BZERO(psRendecData);

#ifdef IMG_KERNEL_MODULE
    IMG_ASSERT(psCoreProps->eMmuType != MMU_TYPE_NONE);
#endif

    if (psCoreProps->eMmuType == MMU_TYPE_40BIT)
    {
        REGIO_WRITE_FIELD(psMmuConfig->ui32MmuControl2,
            MSVDX_CORE, CR_MMU_CONTROL2, CR_MMU_ENABLE_40BIT_ADDRESSING,
            1);
    }
    else if (psCoreProps->eMmuType == MMU_TYPE_36BIT)
    {
        //TCONF_sWrapperControl sWrapperControl;

        REGIO_WRITE_FIELD(psMmuConfig->ui32MmuControl2,
            MSVDX_CORE, CR_MMU_CONTROL2, CR_MMU_ENABLE_36BIT_ADDRESSING,
            1);

        /*if (DBGOPTKM_GetSize("36BitTwiddle", DBGOPT_TYPE_BOOL) &&
            DBGOPTKM_Get("36BitTwiddle", &eType, &sVal))
        {
            psMmuConfig->b36bitMemTwiddle = sVal.bVal;
        }*/

        // Check 36bit memory twiddle in config file and apply test bit accordingly (in FW)
        // NOTE: this might need to be moved up to the application
        //TCONF_GetWrapperControlInfo(&sWrapperControl);
        /*if (sWrapperControl.ui32Flags & TAL_WRAPFLAG_MEM_TWIDDLE)
        {
            psMmuConfig->b36bitMemTwiddle = IMG_TRUE;
        }*/
    }


    // Rendec Configuration.
    REGIO_WRITE_FIELD(psRendecData->ui32RegVecRendecBufferSize,
        MSVDX_VEC, CR_VEC_RENDEC_BUFFER_SIZE, RENDEC_BUFFER_SIZE0,
        psRendec->aui32BufferSize[0] / (4*1024));
    REGIO_WRITE_FIELD(psRendecData->ui32RegVecRendecBufferSize,
        MSVDX_VEC, CR_VEC_RENDEC_BUFFER_SIZE, RENDEC_BUFFER_SIZE1,
        psRendec->aui32BufferSize[1] / (4*1024));

    REGIO_WRITE_FIELD(psRendecData->ui32RegVecRendecControl1,
        MSVDX_VEC, CR_VEC_RENDEC_CONTROL1, RENDEC_DECODE_START_SIZE,
        psRendec->ui32DecodeStartSize);
    REGIO_WRITE_FIELD(psRendecData->ui32RegVecRendecControl1,
        MSVDX_VEC, CR_VEC_RENDEC_CONTROL1, RENDEC_BURST_SIZE_W,
        psRendec->ui8BurstSizeWrite);
    REGIO_WRITE_FIELD(psRendecData->ui32RegVecRendecControl1,
        MSVDX_VEC, CR_VEC_RENDEC_CONTROL1, RENDEC_BURST_SIZE_R,
        psRendec->ui8BurstSizeRead);
    REGIO_WRITE_FIELD(psRendecData->ui32RegVecRendecControl1,
        MSVDX_VEC, CR_VEC_RENDEC_CONTROL1, RENDEC_EXTERNAL_MEMORY,
        psRendec->bInUse);

    psRendecData->aui32RendecInitialContext[0] = psRendec->aui32InitialContext[0];
    psRendecData->aui32RendecInitialContext[1] = psRendec->aui32InitialContext[1];
    psRendecData->aui32RendecInitialContext[2] = psRendec->aui32InitialContext[2];
    psRendecData->aui32RendecInitialContext[3] = psRendec->aui32InitialContext[3];
    psRendecData->aui32RendecInitialContext[4] = psRendec->aui32InitialContext[4];
    psRendecData->aui32RendecInitialContext[5] = psRendec->aui32InitialContext[5];

    REGIO_WRITE_FIELD(psRendecData->ui32RegVecRendecControl0,
        MSVDX_VEC, CR_VEC_RENDEC_CONTROL0, RENDEC_INITIALISE,
        1);

    /* calculate the value to write */
    switch (psCoreProps->ui32SrDmaBurstSize)
    {
    case 2:
        ui32SrBurstSize = 1;
        break;

    case 4:
        ui32SrBurstSize = 2;
        break;

    case 8:
        ui32SrBurstSize = 3;
        break;

    default:
        IMG_ASSERT(0);  // this burst size should not be used
        break;
    }

    psCoreInitData->ui32RegVecControl2 = 0;
    REGIO_WRITE_FIELD(psCoreInitData->ui32RegVecControl2,
        MSVDX_VEC, CR_VEC_CONTROL_2, SR_BURST_SIZE,
        ui32SrBurstSize);

    return IMG_SUCCESS;
}

/*!
******************************************************************************

 @Function              msvdx_CoreReset

******************************************************************************/
static IMG_RESULT
msvdx_CoreReset(
    const IMG_HANDLE  hVxd,
    IMG_BOOL          bClocksEnable
)
{
    MSVDX_sContext  * psContext = (MSVDX_sContext *)hVxd;
    IMG_UINT32        ui32Result = IMG_SUCCESS;

    if (psContext == IMG_NULL)
    {
        REPORT(REPORT_MODULE_VXDIO, REPORT_ERR,
               "An MSVDX context handle must be provided");
        return IMG_ERROR_INVALID_PARAMETERS;
    }

    DEBUG_REPORT(REPORT_MODULE_VXDIO, "Doing MSVDX Reset");

    /*
     * At this point psHwCtx->bPower is already set to false,
     * so there is no need to disable interrupts by SYSOSKM_DisableInt
     * (even if triggered, it won't do a thing)
     */
    if(psContext->bSecureStream == IMG_TRUE)
    {
        //Using Secure Interface
        VXD_sResetArgs sResetArgs = {
            .bClocksEnable = bClocksEnable,
            .bAutoClockGating = psContext->sProps.bAutoClockGatingSupport,
            .bExtClockGating = psContext->sProps.bExtClockGating,
        };
        ui32Result = SECURE_REE_SendMessage(psContext->ui32SecureId,
                                            (IMG_BYTE *)&sResetArgs,
                                            sizeof(sResetArgs),
                                            ENDPOINT_VXD_RESET);

    }
    else
    {
        ui32Result = MSVDXIO_ResetCore(psContext->hMsvdxIo,
                                       psContext->sProps.bAutoClockGatingSupport,
                                       psContext->sProps.bExtClockGating,
                                       bClocksEnable);
    }

    DEBUG_REPORT(REPORT_MODULE_VXDIO, "MSVDX Reset Completed: %u",
                 ui32Result);
    return ui32Result;
}


/*!
******************************************************************************

 @Function              msvdx_RtmWriteRead

******************************************************************************/
static IMG_RESULT
msvdx_RtmWriteRead(
    const MSVDX_sContext  * psContext,
    IMG_UINT32              ui32Select,
    IMG_UINT32            * pui32Val
)
{
    REGIO_WRITE_REGISTER(psContext, MSVDX_CORE, CR_MSVDX_RTM, ui32Select);

    *pui32Val = REGIO_READ_REGISTER(psContext, MSVDX_CORE, CR_MSVDX_RTM);

    return IMG_SUCCESS;
}


#ifndef SYSBRG_BRIDGING
/*
******************************************************************************

 @Function              msvdx_CheckInterruptFunc

 @Description

 Check interrupt function.


******************************************************************************/
static IMG_BOOL
msvdx_CheckInterruptFunc(
    IMG_HANDLE  ui32MemSpaceId,
    IMG_VOID *  pCallbackParameter
)
{
    IMG_UINT32      ui32IntStatus = 0;
    IMG_HANDLE      hCoreContext = (IMG_HANDLE)pCallbackParameter;

    /* Disable interrupts.  */
    SYSOSKM_DisableInt();

    /* Read the status register. */
    ui32IntStatus = VXDIO_ReadRegister(hCoreContext, REGION_VXDSYS_REGSPACE, MSVDX_CORE_CR_MSVDX_INTERRUPT_STATUS_OFFSET, MSVDX_DEF_WR_PIPE);

    /* Re enable interrupts */
    SYSOSKM_EnableInt();

    return (ui32IntStatus != 0);
}

#endif /* not SYSBRG_BRIDGING */


/*!
******************************************************************************

 @Function              MSVDX_DeInitialise

******************************************************************************/
IMG_RESULT
MSVDX_DeInitialise(
    IMG_HANDLE          hVxd
)
{
    MSVDX_sContext    * psContext = (MSVDX_sContext *)hVxd;
    IMG_UINT32          ui32Result = IMG_SUCCESS;

    if (psContext == IMG_NULL)
    {
        REPORT(REPORT_MODULE_VXDIO, REPORT_ERR,
               "An MSVDX context handle must be provided");
        ui32Result = IMG_ERROR_INVALID_PARAMETERS;
        goto error;
    }

#ifdef VDEC_MSVDX_HARDWARE
    /* Reset the MTX-MSVDX first. */
    /* When running against CSim, we don't have to reset the core when deinitializing. */
    if(!psContext->bCSim)
    {
        ui32Result = msvdx_CoreReset(hVxd, IMG_FALSE);
        IMG_ASSERT(ui32Result == IMG_SUCCESS);
        if (ui32Result != IMG_SUCCESS)
        {
            goto error;
        }
    }

#ifndef SYSBRG_BRIDGING
    ui32Result = TALINTERRUPT_UnRegisterCheckFunc(psContext->hSysRegSpace);
    IMG_ASSERT(ui32Result == IMG_SUCCESS);
    if (ui32Result != IMG_SUCCESS)
    {
        goto error;
    }
#endif /* not SYSBRG_BRIDGING */

#endif /* VDEC_MSVDX_HARDWARE */

    return IMG_SUCCESS;

error:
    return ui32Result;
}



/*!
******************************************************************************

 @Function              MSVDX_GetMmuTileConfig

******************************************************************************/
IMG_RESULT MSVDX_GetMmuTileConfig(
    VDEC_eTileScheme        eTileScheme,
    IMG_BOOL                bByteInterleaved,
    VDECFW_sMmuTileConfig * psMmuTileConfig
)
{
    IMG_RESULT              ui32Result;
    MMU_sHeapTilingInfo     asTilingInfo[MMU_HEAP_MAX];
    IMG_UINT32              ui32NumTiledHeaps;
    IMG_UINT32              i;
    IMG_BOOL                bTiling = (eTileScheme == VDEC_TS_NONE) ? IMG_FALSE : IMG_TRUE;

    VDEC_BZERO(psMmuTileConfig);

    // Tile scehem. This is a field inside control2 mmu register (defaults to 0)
    // and will be written in the firmware when updating tile config
    psMmuTileConfig->ui8TilingScheme = (eTileScheme == VDEC_TS1_512x8) ? 1 : 0;
    if(bTiling)
    {
        ui32Result = MMU_GetTiledHeaps(eTileScheme, &ui32NumTiledHeaps, asTilingInfo);
        IMG_ASSERT(ui32Result == IMG_SUCCESS);
        if (ui32Result != IMG_SUCCESS)
        {
            return ui32Result;
        }

        if(ui32NumTiledHeaps > MSVDX_CORE_CR_MMU_TILE_NO_ENTRIES)
        {
            return IMG_ERROR_GENERIC_FAILURE;
        }

        for (i = 0; i < ui32NumTiledHeaps; i++)
        {
            IMG_UINT32 ui32Reg = 0;

            REGIO_WRITE_FIELD(ui32Reg,
                MSVDX_CORE, CR_MMU_TILE, CR_TILE_CFG,
                (bByteInterleaved << 0x4) |(bTiling << 0x3) | asTilingInfo[i].ui32HwTileStride);
            REGIO_WRITE_FIELD(ui32Reg,
                MSVDX_CORE, CR_MMU_TILE, CR_TILE_MAX_ADDR,
                (asTilingInfo[i].ui32StartOffset + asTilingInfo[i].ui32Size - 1) >> 20);
            REGIO_WRITE_FIELD(ui32Reg,
                MSVDX_CORE, CR_MMU_TILE, CR_TILE_MIN_ADDR,
                asTilingInfo[i].ui32StartOffset >> 20);

            psMmuTileConfig->aui32MmuTiling[i] = ui32Reg;
        }
    }

    return IMG_SUCCESS;
}



#ifdef USE_MTX_BREAK_TRAP

/*
******************************************************************************

 @Function              msvdx_SetMTXBreakTrap

 @Description

 This function is used to enable/disable MTX breakpoint trap.
 When trap is enabled, it is signalled to MTX with VDECFW_sFirmwareState
 in VLR RAM, that it should enter NOP loop just after starting. MTX breakpoints
 can be set then. When trap is disabled, MTX jump out of the loop and starts
 normal operation.
 Trap can be enabled only before starting MTX and can be disabled only once
 after MTX is started. Can not be used in any other circumstances.

 @Input     psContext :     A pointer to the device context.

 @Input     bEnable   :     IMG_TRUE to enable trap, IMG_FALSE to tell MTX to
                            jump out of the trap loop.


******************************************************************************/
static IMG_VOID
msvdx_SetMTXBreakTrap(
    MSVDX_sContext    * psContext,
    IMG_BOOL            bEnable
)
{
    IMG_UINT32 ui32Result = IMG_SUCCESS;
    IMG_UINT32 ui32Val = bEnable ? VDECFE_FWACT_BREAK_TRAP : 0;

    if (psContext->bSecureStream == IMG_TRUE)
    {
		ui32Result = IMG_ERROR_FATAL;
    } 
    else 
    {
        ui32Result = VXDIO_VLRWriteWords(psContext->hMsvdxIo,
                                         REGION_VLRFE_REGSPACE,
                                         VLR_FIRMWARE_STATE_AREA_BASE_ADDR + offsetof(VDECFW_sFirmwareState, eFirmwareAction),
                                         sizeof(((VDECFW_sFirmwareState *)0)->eFirmwareAction)>>2,
                                         &ui32Val);
    }

    IMG_ASSERT(ui32Result == IMG_SUCCESS);
}

#endif /* USE_MTX_BREAK_TRAP */



/*!
******************************************************************************

 @Function              MSVDX_ReadVLR

******************************************************************************/
IMG_RESULT MSVDX_ReadVLR(
    const IMG_HANDLE    hVxd,
    IMG_UINT32          ui32Offset,
    IMG_UINT32        * pui32,
    IMG_UINT32          ui32Words
)
{	
    MSVDX_sContext * psCtx = (MSVDX_sContext *)hVxd;
    IMG_UINT32 ui32Result = IMG_SUCCESS;

    if (psCtx->bSecureStream == IMG_TRUE)
    {
#ifndef POST_TEST
		ui32Result = IMG_ERROR_FATAL;
#else /* POST_TEST */
        VXD_SECURE_sReadNRegsArgs sArgs = { 0 };

        sArgs.ui32Region = REGION_VLRFE_REGSPACE;
        sArgs.ui32NumRegs = ui32Words;
        sArgs.ui32Offset = ui32Offset;

        ui32Result =
            SECURE_REE_SendMessageWithBuf(psCtx->ui32SecureId,
                                          (IMG_BYTE *)&sArgs,
                                          sizeof(sArgs),
                                          ENDPOINT_VXD_READN_REGS,
                                          (IMG_BYTE *)pui32,
                                          ui32Words*sizeof(IMG_UINT32));
#endif /* POST_TEST */
    }
    else
    {
        ui32Result = MSVDXIO_VLRReadWords(psCtx->hMsvdxIo,
                                          REGION_VLRFE_REGSPACE,
                                          ui32Offset,
                                          ui32Words,
                                          pui32,
                                          IMG_TRUE);
    }
    return ui32Result;
}

/*!
******************************************************************************

 @Function              MSVDX_WriteVLR

******************************************************************************/
IMG_RESULT MSVDX_WriteVLR(
    const IMG_HANDLE    hVxd,
    IMG_UINT32          ui32Offset,
    IMG_UINT32        * pui32,
    IMG_UINT32          ui32Words
)
{
    MSVDX_sContext * psCtx = (MSVDX_sContext *)hVxd;
    IMG_UINT32 ui32Result = IMG_SUCCESS;

    if (psCtx->bSecureStream == IMG_TRUE)
    {
		ui32Result = IMG_ERROR_FATAL;
    }
    else
    {
        ui32Result = MSVDXIO_VLRWriteWords(psCtx->hMsvdxIo,
                                           REGION_VLRFE_REGSPACE,
                                           ui32Offset,
                                           ui32Words,
                                           pui32);
    }

    return ui32Result;

}
/*
******************************************************************************

 @Function              msvdx_DumpRegSpace

******************************************************************************/
static IMG_RESULT
msvdx_DumpRegSpace(
    const MSVDX_sContext  * psContext,
    IMG_UINT32              ui32Start,
    IMG_UINT32              ui32End,
    const IMG_CHAR        * pszRegSpaceName,
    MEM_REGIONS             eMemRegion
)
{
    IMG_UINT32 i;
    IMG_UINT32 ui32Result = IMG_SUCCESS;

    if (psContext->bSecureStream == IMG_TRUE)
    {
        VXD_SECURE_sReadNRegsArgs sArgs = { 0 };
        IMG_UINT32 ui32NumRegs = ((ui32End - ui32Start) / sizeof(IMG_UINT32));
        IMG_UINT32 ui32DstBufSize = ui32NumRegs * sizeof(IMG_UINT32);
        IMG_UINT32 * pui32Dst = IMG_MALLOC(ui32DstBufSize);
        if (pui32Dst == IMG_NULL)
        {
            return IMG_ERROR_MALLOC_FAILED;
        }

        sArgs.ui32Region = eMemRegion;
        sArgs.ui32NumRegs = ui32NumRegs;
        sArgs.ui32Offset = ui32Start;

        ui32Result =
            SECURE_REE_SendMessageWithBuf(psContext->ui32SecureId,
                                          (IMG_BYTE *)&sArgs,
                                          sizeof(sArgs),
                                          ENDPOINT_VXD_READN_REGS,
                                          (IMG_BYTE *)pui32Dst,
                                          ui32DstBufSize);
        if (ui32Result != IMG_SUCCESS)
        {
            REPORT(REPORT_MODULE_VXDIO, REPORT_ERR,
                   "Failed to read secure registers (num: %u, reg: %u, off: %u)!",
                   ui32NumRegs, eMemRegion, ui32Start);
            IMG_FREE(pui32Dst);
            return ui32Result;
        }

        for (i = 0; i < ui32NumRegs; i++)
        {
            REPORT(REPORT_MODULE_VXDIO, REPORT_NOTICE,
                         "%s [0x%04zX]: 0x%08X",
                         pszRegSpaceName,
                         i * sizeof(IMG_UINT32),
                         pui32Dst[i]);
        }
        IMG_FREE(pui32Dst);
    }
    else
    {
        for (i = ui32Start; i < ui32End; i += 4)
        {
            ui32Result = MSVDXIO_ReadRegister(psContext->hMsvdxIo,
                                              eMemRegion,
                                              i);
    
            REPORT(REPORT_MODULE_VXDIO, REPORT_NOTICE,
                         "%s [0x%04X]: 0x%08X",
                         pszRegSpaceName,
                         i,
                         ui32Result);
        }
    }

    return ui32Result;
}


/*
******************************************************************************

 @Function              MSVDX_DumpRegisters

******************************************************************************/
IMG_RESULT MSVDX_DumpRegisters(
    const IMG_HANDLE  hVxd
)
{
    MSVDX_sContext    * psContext = (MSVDX_sContext *)hVxd;
    VXDIO_sState        sState;
    IMG_UINT32          ui32Result;

    if (psContext == IMG_NULL)
    {
        REPORT(REPORT_MODULE_VXDIO, REPORT_ERR,
               "An MSVDX context handle must be provided");
        ui32Result = IMG_ERROR_INVALID_PARAMETERS;
        goto error;
    }

#ifdef VDEC_MSVDX_HARDWARE

    // MSVDX Core
    REPORT(REPORT_MODULE_VXDIO, REPORT_NOTICE, "\nMSVDX Core Register Space 0x0600 - 0x06FF");
    msvdx_DumpRegSpace(psContext, 0, 0x0100, "MSVDX Core", REGION_VXDSYS_REGSPACE);

    // MTX Registers
    REPORT(REPORT_MODULE_VXDIO, REPORT_NOTICE, "\nMTX Register Space 0x0000 - 0x0354");
    msvdx_DumpRegSpace(psContext, 0, 0x0354, "MTX", REGION_VXDMTX_REGSPACE);

    ui32Result = MSVDX_GetCoreState(hVxd, &sState);
    IMG_ASSERT(ui32Result == IMG_SUCCESS);
    if (ui32Result != IMG_SUCCESS)
    {
        goto error;
    }

    REPORT(REPORT_MODULE_VXDIO, REPORT_NOTICE, "\nCORE %d STATUS\n", psContext->ui32CoreNum);
    REPORT(REPORT_MODULE_VXDIO, REPORT_NOTICE, "==========");
    REPORT(REPORT_MODULE_VXDIO, REPORT_NOTICE, "MTX PC:  0x%08X", sState.sEPRuntimeStatus.ui32MTX_PC);
    REPORT(REPORT_MODULE_VXDIO, REPORT_NOTICE, "MTX PCX: 0x%08X", sState.sEPRuntimeStatus.ui32MTX_PCX);
    REPORT(REPORT_MODULE_VXDIO, REPORT_NOTICE, "MTX Stack Pointer: 0x%08X", sState.sEPRuntimeStatus.ui32MTX_A0StP);
    REPORT(REPORT_MODULE_VXDIO, REPORT_NOTICE, "MTX Frame Pointer: 0x%08X", sState.sEPRuntimeStatus.ui32MTX_A0FrP);
    REPORT(REPORT_MODULE_VXDIO, REPORT_NOTICE, "MTX STATUS BITS: 0x%08X", sState.sEPRuntimeStatus.ui32MTX_STATUS_BITS);
    REPORT(REPORT_MODULE_VXDIO, REPORT_NOTICE, "MTX FAULT0: 0x%08X", sState.sEPRuntimeStatus.ui32MTX_FAULT0);
    REPORT(REPORT_MODULE_VXDIO, REPORT_NOTICE, "MTX ENABLE: 0x%08X", sState.sEPRuntimeStatus.ui32MTX_ENABLE);
    REPORT(REPORT_MODULE_VXDIO, REPORT_NOTICE, "MTX STATUS END");
    REPORT(REPORT_MODULE_VXDIO, REPORT_NOTICE, "==============");

    REPORT(REPORT_MODULE_VXDIO, REPORT_NOTICE, "\nDMAC STATUS");
    REPORT(REPORT_MODULE_VXDIO, REPORT_NOTICE, "===========");
    REPORT(REPORT_MODULE_VXDIO, REPORT_NOTICE, "DMAC SETUP[0]: 0x%08X", sState.sEPRuntimeStatus.ui32DMA_SETUP[0]);
    REPORT(REPORT_MODULE_VXDIO, REPORT_NOTICE, "DMAC SETUP[1]: 0x%08X", sState.sEPRuntimeStatus.ui32DMA_SETUP[1]);
    REPORT(REPORT_MODULE_VXDIO, REPORT_NOTICE, "DMAC SETUP[2]: 0x%08X", sState.sEPRuntimeStatus.ui32DMA_SETUP[2]);
    REPORT(REPORT_MODULE_VXDIO, REPORT_NOTICE, "DMAC COUNT[0]: 0x%08X", sState.sEPRuntimeStatus.ui32DMA_COUNT[0]);
    REPORT(REPORT_MODULE_VXDIO, REPORT_NOTICE, "DMAC COUNT[1]: 0x%08X", sState.sEPRuntimeStatus.ui32DMA_COUNT[1]);
    REPORT(REPORT_MODULE_VXDIO, REPORT_NOTICE, "DMAC COUNT[2]: 0x%08X", sState.sEPRuntimeStatus.ui32DMA_COUNT[2]);
    REPORT(REPORT_MODULE_VXDIO, REPORT_NOTICE, "DMAC PERIPHERAL ADDR[0]: 0x%08X", sState.sEPRuntimeStatus.ui32DMA_PERIPHERAL_ADDR[0]);
    REPORT(REPORT_MODULE_VXDIO, REPORT_NOTICE, "DMAC PERIPHERAL ADDR[1]: 0x%08X", sState.sEPRuntimeStatus.ui32DMA_PERIPHERAL_ADDR[1]);
    REPORT(REPORT_MODULE_VXDIO, REPORT_NOTICE, "DMAC PERIPHERAL ADDR[2]: 0x%08X", sState.sEPRuntimeStatus.ui32DMA_PERIPHERAL_ADDR[2]);
    REPORT(REPORT_MODULE_VXDIO, REPORT_NOTICE, "DMAC STATUS END");
    REPORT(REPORT_MODULE_VXDIO, REPORT_NOTICE, "===============");

    // VEC (excluding 0x98: rendec data)
    REPORT(REPORT_MODULE_VXDIO, REPORT_NOTICE, "\nVEC Register Space 0x0800 - 0x0FFF");
    msvdx_DumpRegSpace(psContext, 0, 0x98, "VEC", REGION_VEC_REGSPACE);
    msvdx_DumpRegSpace(psContext, 0x9c, 0x0100, "VEC", REGION_VEC_REGSPACE);
    msvdx_DumpRegSpace(psContext, 0x0100, 0x03FC, "VEC", REGION_VEC_REGSPACE);

    // VDMC
    REPORT(REPORT_MODULE_VXDIO, REPORT_NOTICE, "\nVDMC Register Space 0x0400 - 0x047F");
    msvdx_DumpRegSpace(psContext, 0, 0x58, "VDMC", REGION_VDMC_REGSPACE);

    // VDEB
    REPORT(REPORT_MODULE_VXDIO, REPORT_NOTICE, "\nVDEB Register Space 0x0480 - 0x04FF");
    msvdx_DumpRegSpace(psContext, 0, 0x50, "VDEB", REGION_VDEB_REGSPACE);

    // DMAC
    REPORT(REPORT_MODULE_VXDIO, REPORT_NOTICE, "\nDMAC Register Space 0x0500 - 0x05FF");
    msvdx_DumpRegSpace(psContext, 0, 0x0100, "DMAC", REGION_DMAC_REGSPACE);

    // VLR
    REPORT(REPORT_MODULE_VXDIO, REPORT_NOTICE, "\nVLR Register Space 0x2000 - 0x2FFF");
    msvdx_DumpRegSpace(psContext, 0, 0x01000, "VLR", REGION_VLR_REGSPACE);

    /* Also dump RTM stuff for hardware debugging. It's not supported in a
     * secure mode currently, as it is required to write registers in order
     * to dump RTM - registers writing is not available in secure mode. */
    if (psContext->bSecureStream == IMG_FALSE)
    {
        IMG_CHAR * apszModuleName[] = {"CORE", "VEC", "VDMC", "VDEB"};
        IMG_UINT32 x, y, z;
        IMG_UINT32 ui32Select, ui32Val;
        const IMG_UINT32 ui32ModASigMask = 0x00000007;
        const IMG_UINT32 ui32ModBSigMask = 0x00000038;

        REPORT(REPORT_MODULE_VXDIO, REPORT_NOTICE,  "\nRTM Dump");
        for (x=0; x<4; x++)
        {
            // select module A/B
            ui32Select = (x<<8) | (x<<6);

            for (y=0; y<8; y++)
            {
                // select module B signals
                ui32Select &= ~ui32ModBSigMask;
                ui32Select |= (y<<3);

                for (z=0; z<8; z++)
                {
                    // select module A signals
                    ui32Select &= ~ui32ModASigMask;
                    ui32Select |= z;

                    //TAL_WriteWord(i, j, ui32Select);
                    //TAL_ReadWord(i, j, &ui32Val);
                    msvdx_RtmWriteRead(psContext, ui32Select, &ui32Val);
                    IMG_ASSERT(apszModuleName[(ui32Select>>6)&0x3] == apszModuleName[(ui32Select>>8)&0x3]);
                    REPORT(REPORT_MODULE_VXDIO, REPORT_NOTICE,
                                 "MSVDX RTM: Select[%08x] Value[%08x] # RTM %s, A[0x%01X]:0x%02X & B[0x%01X]:0x%02X",
                                 ui32Select, ui32Val, apszModuleName[(ui32Select>>6)&0x3],
                                 z, (ui32Val>>16)&0xff, y, (ui32Val>>24)&0xff);
                }
            }
        }
    }

#endif /* VDEC_MSVDX_HARDWARE */

    return IMG_SUCCESS;

error:
    return ui32Result;
}



/*!
******************************************************************************

 @Function              MSVDX_GetCoreState

******************************************************************************/
IMG_RESULT
MSVDX_GetCoreState(
    const IMG_HANDLE    hVxd,
    VXDIO_sState      * psState
)
{
    MSVDX_sContext    * psContext = (MSVDX_sContext *)hVxd;
    IMG_UINT32          ui32Result;
    VXD_sGetStateArgs   sGetStateArgs;

    VDEC_BZERO((&sGetStateArgs));

    if (psContext == IMG_NULL)
    {
        REPORT(REPORT_MODULE_VXDIO, REPORT_ERR,
               "An MSVDX context handle must be provided");
        ui32Result = IMG_ERROR_INVALID_PARAMETERS;
        goto error;
    }

#ifdef VDEC_MSVDX_HARDWARE

    if (psContext->bSecureStream == IMG_TRUE)
    {
        ui32Result = SECURE_REE_SendMessage(psContext->ui32SecureId,
                                        (IMG_BYTE *)&sGetStateArgs,
                                        sizeof(sGetStateArgs),
                                        ENDPOINT_VXD_GET_STATE);
		if (ui32Result == IMG_SUCCESS)
		{
			*psState = sGetStateArgs.sState;
		}
    }
    else
    {
        ui32Result = MSVDXIO_GetCoreState(psContext->hMsvdxIo,
                                          psState);
    }

    IMG_ASSERT(ui32Result == IMG_SUCCESS);
    if (ui32Result != IMG_SUCCESS)
    {
        goto error;
    }
#endif

    return IMG_SUCCESS;

error:
    return ui32Result;
}


/*!
******************************************************************************

 @Function              MSVDX_FlushMmuCache

******************************************************************************/
IMG_RESULT
MSVDX_FlushMmuCache(
    const IMG_HANDLE  hVxd
)
{
    MSVDX_sContext * psContext = (MSVDX_sContext *)hVxd;

    if (psContext == IMG_NULL)
    {
        REPORT(REPORT_MODULE_VXDIO, REPORT_ERR,
               "An MSVDX context handle must be provided");
        return IMG_ERROR_INVALID_PARAMETERS;
    }

    if (psContext->bSecureStream == IMG_TRUE)
    {
        psContext->bFlushMmu = IMG_TRUE;
    }
    else
    {
#ifdef VDEC_MSVDX_HARDWARE
        IMG_UINT32 ui32Reg;
        ui32Reg = REGIO_READ_REGISTER(psContext, MSVDX_CORE, CR_MMU_CONTROL0);
        REGIO_WRITE_FIELD(ui32Reg, MSVDX_CORE, CR_MMU_CONTROL0, CR_MMU_INVALDC, 1);
        REGIO_WRITE_REGISTER(psContext, MSVDX_CORE, CR_MMU_CONTROL0, ui32Reg);

        REGIO_WRITE_FIELD(ui32Reg, MSVDX_CORE, CR_MMU_CONTROL0, CR_MMU_INVALDC, 0);
        REGIO_WRITE_REGISTER(psContext, MSVDX_CORE, CR_MMU_CONTROL0, ui32Reg);
#endif /* VDEC_MSVDX_HARDWARE */
    }

    return IMG_SUCCESS;

}

/*!
******************************************************************************

 @Function              msvdx_ProcessSecMsgs

 @Description

 This function parses message buffer obtained from the TEE and copies messages
 into the host buffer used for message processing by upper layers (further
 called destination buffer). It also creates metadata (size and offset of
 each message within bufer) required by these layers.

 @Input     psContext      : MSVDX component context

 @In/output psMsgQueue     : Contains buffer used by upper layers for message
                             processing and queues to store message metadata.

 @Input     pui32MsgBuf    : Buffer with messages obtained from the TEE

 @Input     ui32BufWrds    : Size of pui32MsgBuf in words.

 @Return                   : None

******************************************************************************/
static IMG_VOID
msvdx_ProcessSecMsgs(
    MSVDX_sContext    * psContext,
    VXD_sMsgQueue     * psMsgQueue, 
    IMG_UINT32        * pui32MsgBuf,
    IMG_UINT32          ui32BufWrds
)
{
    VXDIO_sHISRMsg * psMessage;
    IMG_UINT32 ui32CurWrd = psContext->ui32MtxMsgsOffset;

    psMsgQueue->bEmpty = IMG_FALSE;
    psMsgQueue->bQueued = IMG_FALSE;

    while ((ui32CurWrd < ui32BufWrds) && LST_first(&psMsgQueue->sFreeMsgList))
    {
        IMG_UINT32 ui32MsgSize =
            ((MEMIO_READ_FIELD(&pui32MsgBuf[ui32CurWrd], V2_PADMSG_SIZE)) +
                (sizeof(IMG_UINT32) - 1)) / sizeof(IMG_UINT32);

        /* Switch to the beginning of the destination buffer if
         * message won't fit into remaining space. */
        if (psMsgQueue->ui32WriteIdx + ui32MsgSize > MSVDX_SIZE_MSG_BUFFER)
        {
            psMsgQueue->ui32WriteIdx = 0;
        }

        /* If there's not enough space in the destination buffer,
         * postpone the processing. */
        if ((psMsgQueue->ui32ReadIdx > psMsgQueue->ui32WriteIdx) &&
            (psMsgQueue->ui32WriteIdx + ui32MsgSize > psMsgQueue->ui32ReadIdx))
        {
            break;
        }

        psMessage = LST_removeHead(&psMsgQueue->sFreeMsgList);

        /* Copy data from the secure message into a destination buffer */
        IMG_MEMCPY(&psMsgQueue->aui32MsgBuf[psMsgQueue->ui32WriteIdx],
                   &pui32MsgBuf[ui32CurWrd],
                   ui32MsgSize * sizeof(IMG_UINT32));

        /* Offset in the secure message buffer */
        ui32CurWrd += ui32MsgSize;

        /* Create metadata to be used for message processing. */
        psMessage->ui32MsgIndex = psMsgQueue->ui32WriteIdx;
        psMessage->ui32MsgSize = ui32MsgSize;

        /* Add the message to the list processed by upper layers */
        LST_add(&psMsgQueue->sNewMsgList, psMessage);

        /* Update index of destination buffer and singal that there are messages
         * to be processed. */
        psMsgQueue->ui32WriteIdx += ui32MsgSize;

        psMsgQueue->bQueued = IMG_TRUE;
    }

    if (ui32CurWrd < ui32BufWrds)
    {
        /* We haven't processed whole buffer received from the TEE,
         * signal that there is data pending in secure message buffer*/
        psContext->ui32MtxMsgsOffset = ui32CurWrd;
    }
    else
    {
        psContext->ui32MtxMsgsOffset = 0;
    }
}

/*!
******************************************************************************

 @Function              msvdx_HandleSecInterrupts

******************************************************************************/
static IMG_RESULT
msvdx_HandleSecInterrupts(
    MSVDX_sContext    * psContext,
    VXD_sIntStatus    * psIntStatus
)
{
    VXD_sHandleInterruptsArgs sIntsArgs;
    IMG_RESULT ui32Result;
    IMG_UINT32 ui32MtxMsgsBufWrds = sizeof(psContext->aui32MtxMsgs)/
                                    sizeof(psContext->aui32MtxMsgs[0]);

    if (LST_empty(&psIntStatus->psMsgQueue->sFreeMsgList))
    {
        REPORT(REPORT_MODULE_VXDIO, REPORT_ERR,
               "There are no empty messages in the quueue!");
        return IMG_ERROR_UNEXPECTED_STATE;
    }

    if (psContext->ui32MtxMsgsOffset)
    {
        /* We haven't processed all messages stored in an internal context
         * last time */
        msvdx_ProcessSecMsgs(psContext,
                             psIntStatus->psMsgQueue,
                             psContext->aui32MtxMsgs,
                             psContext->ui32MsgsSizeWrds);

        psIntStatus->ui32Pending = MSVDX_CORE_CR_MSVDX_INTERRUPT_STATUS_CR_MTX_IRQ_MASK;
        psIntStatus->ui32Requestor = 0;
        psIntStatus->MMU_FAULT_ADDR = 0;
        psIntStatus->MMU_FAULT_RNW = 0;
        psIntStatus->MMU_PF_N_RW = 0;

        /* We haven't fetched any messages from the MTX -> HOST buffer
         * this time. Signal that the function should be called again. */
        psIntStatus->psMsgQueue->bEmpty = IMG_FALSE;
    }
    else
    {
        /* Internal context is empty, call to the TEE to fetch new
         * messages from the MTX */
        IMG_MEMSET(&sIntsArgs, 0, sizeof(sIntsArgs));
        sIntsArgs.ui32MsgsSizeWrds = ui32MtxMsgsBufWrds;

        ui32Result = SECURE_REE_SendMessageWithBuf(psContext->ui32SecureId,
                                                   (IMG_BYTE *)&sIntsArgs,
                                                   sizeof(sIntsArgs),
                                                   ENDPOINT_VXD_HANDLE_INTERRUPTS,
                                                   (IMG_BYTE *)psContext->aui32MtxMsgs,
                                                   sizeof(psContext->aui32MtxMsgs));
        if (ui32Result != IMG_SUCCESS)
        {
            REPORT(REPORT_MODULE_VXDIO, REPORT_ERR,
                   "ENDPOINT_VXD_HANDLE_INTERRUPTS call failed.");
            return ui32Result;
        }

        if (sIntsArgs.ui32MsgsSizeWrds > ui32MtxMsgsBufWrds)
        {
            REPORT(REPORT_MODULE_VXDIO, REPORT_ERR,
                   "ENDPOINT_VXD_HANDLE_INTERRUPTS call returned invalid data!"
                   "(ret buf size: %u, orig: %u)",
                   sIntsArgs.ui32MsgsSizeWrds,
                   ui32MtxMsgsBufWrds);
            return IMG_ERROR_UNEXPECTED_STATE;
        }
        psContext->ui32MsgsSizeWrds = sIntsArgs.ui32MsgsSizeWrds;

        msvdx_ProcessSecMsgs(psContext,
                             psIntStatus->psMsgQueue,
                             psContext->aui32MtxMsgs,
                             psContext->ui32MsgsSizeWrds);

        /* The interrupt processing will be called again if all messages
         * weren't processed: either there are still messgaes in MTX -> HOST
         * buffer, or we haven't processed all messages stored in an internal
         * context */
        psIntStatus->psMsgQueue->bEmpty = !(sIntsArgs.bMoreData ||
                                           (psContext->ui32MtxMsgsOffset != 0));

        /* Copy output from secure message */
        psIntStatus->ui32Pending = sIntsArgs.ui32Pending;
        psIntStatus->ui32Requestor = sIntsArgs.ui32Requestor;
        psIntStatus->MMU_FAULT_ADDR = sIntsArgs.MMU_FAULT_ADDR;
        psIntStatus->MMU_FAULT_RNW = sIntsArgs.MMU_FAULT_RNW;
        psIntStatus->MMU_PF_N_RW = sIntsArgs.MMU_PF_N_RW;
    }

    return IMG_SUCCESS;
}


/*!
******************************************************************************

 @Function              MSVDX_HandleInterrupts

******************************************************************************/
IMG_RESULT
MSVDX_HandleInterrupts(
    const IMG_HANDLE    hVxd,
    VXD_sIntStatus    * psIntStatus
)
{
    MSVDX_sContext            * psContext = (MSVDX_sContext *)hVxd;
    IMG_UINT32                  ui32Result = IMG_SUCCESS;

    if (psContext == IMG_NULL)
    {
        REPORT(REPORT_MODULE_VXDIO, REPORT_ERR,
               "An MSVDX context handle must be provided");
        ui32Result = IMG_ERROR_INVALID_PARAMETERS;
        goto error;
    }

#ifdef VDEC_MSVDX_HARDWARE
    /* Disable interrupts */
    //SYSOSKM_DisableInt();

    if (psContext->bSecureStream == IMG_TRUE)
    {
        //Using Secure Interface
        ui32Result = msvdx_HandleSecInterrupts(psContext,
                                               psIntStatus);
    }
    else
    {
        ui32Result = MSVDXIO_HandleInterrupts(psContext->hMsvdxIo,
                                              psIntStatus);
    }                                        

    IMG_ASSERT(ui32Result == IMG_SUCCESS);
    if (ui32Result != IMG_SUCCESS)
    {
        goto error_inthandle;
    }

error_inthandle:
    /* Re enable interrupts */
    //SYSOSKM_EnableInt();
#endif /* VDEC_MSVDX_HARDWARE */

error:
    return ui32Result;
}

/*!
******************************************************************************

 @Function              MSVDX_HandleInterrupts

******************************************************************************/
IMG_RESULT
MSVDX_GetIntStatus(
    const IMG_HANDLE    hVxd,
    VXD_sIntStatus    * psIntStatus
)
{
    IMG_UINT32        ui32Reg;
    IMG_UINT32        ui32Enable;
    IMG_UINT32        ui32Result = IMG_SUCCESS;
    MSVDX_sContext  * psContext = (MSVDX_sContext *)hVxd;

    if (psContext == IMG_NULL)
    {
        REPORT(REPORT_MODULE_VXDIO, REPORT_ERR,
               "An MSVDX context handle must be provided");
        ui32Result = IMG_ERROR_INVALID_PARAMETERS;
        goto error;
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
    }

error:
    return ui32Result;
}

/*!
******************************************************************************

 @Function              msvdx_fnTimer

******************************************************************************/
static IMG_BOOL
msvdx_fnTimer(
    IMG_VOID * pvParam
)
{
    IMG_HANDLE hEventHandle = (IMG_HANDLE)pvParam;

    /* Wake up the suspended process...*/
    SEC_SignalEventObject(hEventHandle);

    /* Indiacte that we want to be rescheduled...*/
    return IMG_TRUE;
}


/*!
******************************************************************************

 @Function              msvdx_SendSecFirmwareMessage

 @Description

 This functions submits the firmware message to the ENDPOINT_VXD_INPUT
 endpoint in the TEE, where it's written to the MTX comms RAM.
 If there's not enough space in the comms RAM, endpoint is supposed to return
 IMG_ERROR_BUSY error code. In such case, this function retries to send the
 message MSVDX_SEC_MSG_RETRY_COUNT times, with MSVDX_SEC_MSG_INTERVAL_MS
 interval between each attempt.

******************************************************************************/
#define MSVDX_SEC_MSG_RETRY_COUNT 10
#define MSVDX_SEC_MSG_INTERVAL_MS 10
static IMG_RESULT
msvdx_SendSecFirmwareMessage(
    MSVDX_sContext   * psContext,
    VXD_eCommsArea     eArea,
    const IMG_VOID   * psMsgHdr,
    IMG_UINT32         ui32MsgBufSize
)
{
    IMG_RESULT ui32Result;
    IMG_UINT32 ui32WaitRetry = MSVDX_SEC_MSG_RETRY_COUNT;
    IMG_HANDLE hEventHandle = IMG_NULL;
    IMG_HANDLE hTimerHandle = IMG_NULL;

    if (ui32MsgBufSize == 0)
    {
        /* The optional ui32MsgBufSize parameter is used when message
         * buffer contains more than one message. If it's not set, simply
         * obtain the size of the message from the message itself. */
        ui32MsgBufSize = MEMIO_READ_FIELD(psMsgHdr, V2_PADMSG_SIZE);
    }

    psContext->sFwMsgArgs.eArea = eArea;
    psContext->sFwMsgArgs.bFlushMmu = psContext->bFlushMmu;
    psContext->bFlushMmu = IMG_FALSE;

    ui32Result = SECURE_REE_SendMessageWithBuf(psContext->ui32SecureId,
                                               (IMG_BYTE *)&psContext->sFwMsgArgs,
                                               sizeof(psContext->sFwMsgArgs),
                                               ENDPOINT_VXD_INPUT,
                                               (IMG_BYTE *)psMsgHdr,
                                               ui32MsgBufSize);
    if (ui32Result != IMG_ERROR_BUSY)
    {
        /* Either success or some non-recoverable error. */
        return ui32Result;
    }

    /* IMG_ERROR_BUSY returned by ENDPOINT_VXD_INPUT means that there wasn't
     * enough space in comms RAM to submit the firmware message. Retry. */

    /* Create an event object */
    ui32Result = SEC_CreateEventObject(&hEventHandle);
    if (ui32Result != IMG_SUCCESS)
    {
        return ui32Result;
    }

    /* Create a timer and attach to the event object */
    ui32Result = SEC_CreateTimer(msvdx_fnTimer, hEventHandle,
                                 MSVDX_SEC_MSG_INTERVAL_MS, &hTimerHandle);
    if (ui32Result != IMG_SUCCESS)
    {
        SEC_DestroyEventObject(hEventHandle);
        return ui32Result;
    }

    while (ui32WaitRetry--)
    {
        /* wait for MSVDX_SEC_MSG_INTERVAL_MS */
        SEC_WaitEventObject(hEventHandle, IMG_TRUE);

        ui32Result =
            SECURE_REE_SendMessageWithBuf(psContext->ui32SecureId,
                                          (IMG_BYTE *)&psContext->sFwMsgArgs,
                                          sizeof(psContext->sFwMsgArgs),
                                          ENDPOINT_VXD_INPUT,
                                          (IMG_BYTE *)psMsgHdr,
                                          ui32MsgBufSize);
        if (ui32Result != IMG_ERROR_BUSY)
        {
            /* Either success or some non-recoverable error ... */
            break;
        }
    }

    /* remove the timer before the event object */
    /* remove wait object */
    if (hTimerHandle)
    {
        SEC_DestroyTimer(hTimerHandle);
    }

    /* remove wait object */
    if (hEventHandle)
    {
        SEC_DestroyEventObject(hEventHandle);
    }

    return ui32Result;
}

/*!
******************************************************************************

 @Function              MSVDX_SendFirmwareMessage

******************************************************************************/
IMG_RESULT
MSVDX_SendFirmwareMessage(
    const IMG_HANDLE   hVxd,
    VXD_eCommsArea     eArea,
    const IMG_VOID   * psMsgHdr,
    IMG_UINT32         ui32MsgBufSize
)
{
    MSVDX_sContext  * psContext = (MSVDX_sContext *)hVxd;
    IMG_UINT32        ui32Result;

    if (psContext == IMG_NULL)
    {
        REPORT(REPORT_MODULE_VXDIO, REPORT_ERR,
               "An MSVDX context handle must be provided");
        ui32Result = IMG_ERROR_INVALID_PARAMETERS;
        goto error;
    }

#ifdef VDEC_MSVDX_HARDWARE
    if (psContext->bSecureStream == IMG_TRUE)
    {
        ui32Result = msvdx_SendSecFirmwareMessage(psContext,
                                                  eArea,
                                                  psMsgHdr,
                                                  ui32MsgBufSize);
    }
    else
    {
        ui32Result = MSVDXIO_SendFirmwareMessage(psContext->hMsvdxIo,
                                                 eArea,
                                                 psMsgHdr);
    }

    IMG_ASSERT(ui32Result == IMG_SUCCESS);
    if (ui32Result != IMG_SUCCESS)
    {
        goto error;
    }
#endif

    return IMG_SUCCESS;

error:
    return ui32Result;
}


#ifdef POST_TEST
/*!
******************************************************************************

 @Function              checkCoreRevision

 @Description

 This function checks the core revision. If the core is the expected one
 than return IMG_SUCCESS else IMG_ERROR_DEVICE_NOT_FOUND

******************************************************************************/
IMG_RESULT
checkCoreRevision(
    IMG_HANDLE hVxd
)
{
    IMG_RESULT ui32Result;
    IMG_UINT32 ui32Register;
    IMG_UINT32 ui32RegField;
    MSVDX_sContext *psContext = (MSVDX_sContext *)hVxd;

    /* Init result to PASSED */
    ui32Result = IMG_SUCCESS;

    /* Get Core Revision Information */
    ui32Register = REGIO_READ_REGISTER(psContext, MSVDX_CORE, CR_MSVDX_CORE_REV);
    /* Check if the register is different than 0xFFFFFFFF at least */
    if(INVALID_32BIT_REG_VALUE == ui32Register)
    {
        /* the test is FAILED */
        ui32Result = IMG_ERROR_DEVICE_NOT_FOUND;
    }
    else
    {
        /* do nothing */
    }

    /* Get MSVDX IMG Internal register */
    ui32Register = REGIO_READ_REGISTER(psContext, MSVDX_CORE, CR_MSVDX_INTERNAL);
    /* Check if the register is different than 0xFFFFFFFF at least */
    if(INVALID_32BIT_REG_VALUE == ui32Register)
    {
        /* the test is FAILED */
        ui32Result = IMG_ERROR_DEVICE_NOT_FOUND;
    }
    else
    {
        /* do nothing */
    }

    /* Get MSVDX latency config */
    ui32Register = REGIO_READ_REGISTER(psContext, MSVDX_CORE, CR_MSVDX_LATENCY_CONFIG);
    /* Check if the register is different than 0xFFFFFFFF at least */
    if(INVALID_32BIT_REG_VALUE == ui32Register)
    {
        /* the test is FAILED */
        ui32Result = IMG_ERROR_DEVICE_NOT_FOUND;
    }
    else
    {
        /* do nothing */
    }

    /* Get Core ID register */
    ui32Register = REGIO_READ_REGISTER(psContext, MSVDX_CORE, CR_MSVDX_CORE_ID);
    /* Get and check Group ID field */
    ui32RegField = REGIO_READ_FIELD(ui32Register, MSVDX_CORE, CR_MSVDX_CORE_ID, CR_GROUP_ID);
    /* Check the value */
    if(GROUP_ID_DEFAULT_VALUE != ui32RegField)
    {
        /* the test is FAILED */
        ui32Result = IMG_ERROR_DEVICE_NOT_FOUND;
    }
    else
    {
        /* do nothing */
    }
    /* Get and check Core ID field */
    ui32RegField = REGIO_READ_FIELD(ui32Register, MSVDX_CORE, CR_MSVDX_CORE_ID, CR_CORE_ID);
    /* Check the value */
    if(CORE_ID_DEFAULT_VALUE != ui32RegField)
    {
        /* the test is FAILED */
        ui32Result = IMG_ERROR_DEVICE_NOT_FOUND;
    }
    else
    {
        /* do nothing */
    }

    /* Return result value */
    return ui32Result;
}




/*!
******************************************************************************

 @Function              executeRegTest

 @Description

 This function executes POST registers test. It returns IMG_SUCCESS if
 test passed else IMG_ERROR_TEST_MISMATCH

******************************************************************************/
IMG_RESULT
executeRegTest(
    IMG_HANDLE hVxd
)
{
    IMG_UINT32 ui32RegValue;
    IMG_UINT32 ui32OldRegValue;
    IMG_RESULT ui32Result;
    MSVDX_sContext *psContext = (MSVDX_sContext *)hVxd;

    /* init test result to pass */
    ui32Result = IMG_SUCCESS;

    /* TEST ON MTX_ENABLE register */

    /* read the CR_MTX_SYSC_CDMAA register and store the default value */
    ui32OldRegValue = REGIO_READ_REGISTER(psContext, MTX_CORE, CR_MTX_REGISTER_READ_WRITE_REQUEST);
    /* check the read value with the default value */
    if(CR_MTX_REG_RW_REQ_DEF_VALUE != ui32OldRegValue)
    {
        /* register test fail */
        ui32Result = IMG_ERROR_DEVICE_UNAVAILABLE;
    }
    else
    {
        /* register test passed: do nothing */
    }

    /* reset register value to update: can be equal to old value */
    ui32RegValue = 0;
    /* write the test value to the writable register field */
    REGIO_WRITE_FIELD(ui32RegValue, MTX_CORE, CR_MTX_REGISTER_READ_WRITE_REQUEST, MTX_RSPECIFIER, TEST_REG_FIELD_3BITS_VALUE);
    /* write the register */
    REGIO_WRITE_REGISTER(psContext, MTX_CORE, CR_MTX_REGISTER_READ_WRITE_REQUEST, ui32RegValue);

    /* read the register again */
    ui32RegValue = REGIO_READ_REGISTER(psContext, MTX_CORE, CR_MTX_REGISTER_READ_WRITE_REQUEST);
    /* read the register field */
    ui32RegValue = REGIO_READ_FIELD(ui32RegValue, MTX_CORE, CR_MTX_REGISTER_READ_WRITE_REQUEST, MTX_RSPECIFIER);
    /* check the read value with the expected written value */
    if(TEST_REG_FIELD_3BITS_VALUE != ui32RegValue)
    {
        /* register test fail */
        ui32Result = IMG_ERROR_DEVICE_UNAVAILABLE;
    }
    else
    {
        /* register test passed: do nothing */
    }

    /* re-write the register with the default value */
    REGIO_WRITE_REGISTER(psContext, MTX_CORE, CR_MTX_REGISTER_READ_WRITE_REQUEST, ui32OldRegValue);




    /* TEST ON CR_VEC_RENDEC_CONTROL1 register */

    /* read the VEC_RENDEC_CONTROL1 register and store the default value */
    ui32OldRegValue = REGIO_READ_REGISTER(psContext, MSVDX_VEC, CR_VEC_RENDEC_CONTROL1);
    /* check the read value with the default value */
    if(CR_VEC_RENDEC_CONTROL1_DEF_VALUE != ui32OldRegValue)
    {
        /* register test fail */
        ui32Result = IMG_ERROR_DEVICE_UNAVAILABLE;
    }
    else
    {
        /* register test passed: do nothing */
    }

    /* reset register value to update: can be equal to old value */
    ui32RegValue = 0;
    /* write the test value to the writable register field */
    REGIO_WRITE_FIELD(ui32RegValue, MSVDX_VEC, CR_VEC_RENDEC_CONTROL1, RENDEC_DECODE_START_SIZE, TEST_REG_FIELD_8BITS_VALUE);
    /* write the test value to the register */
    REGIO_WRITE_REGISTER(psContext, MSVDX_VEC, CR_VEC_RENDEC_CONTROL1, ui32RegValue);

    /* read the register again */
    ui32RegValue = REGIO_READ_REGISTER(psContext, MSVDX_VEC, CR_VEC_RENDEC_CONTROL1);
    /* read the register field */
    ui32RegValue = REGIO_READ_FIELD(ui32RegValue, MSVDX_VEC, CR_VEC_RENDEC_CONTROL1, RENDEC_DECODE_START_SIZE);
    /* check the read value with the expected written value */
    if(TEST_REG_FIELD_8BITS_VALUE != ui32RegValue)
    {
        /* register test fail */
        ui32Result = IMG_ERROR_DEVICE_UNAVAILABLE;
    }
    else
    {
        /* register test passed: Do nothing */
    }

    /* re-write the register with the default value */
    REGIO_WRITE_REGISTER(psContext, MSVDX_VEC, CR_VEC_RENDEC_CONTROL1, ui32OldRegValue);




    /* TEST ON CR_MMU_BANK_INDEX register */

    /* read the CR_MMU_BANK_INDEX register and store the default value */
    ui32OldRegValue = REGIO_READ_REGISTER(psContext, MSVDX_CORE, CR_MMU_BANK_INDEX);
    /* check the read value with the default value */
    if(CR_MMU_BANK_INDEX_DEF_VALUE != ui32OldRegValue)
    {
        /* register test fail */
        ui32Result = IMG_ERROR_DEVICE_UNAVAILABLE;
    }
    else
    {
        /* register test passed: do nothing */
    }

    /* reset register value to update: can be equal to old value */
    ui32RegValue = 0;
    /* write the test value to the writable register field */
    REGIO_WRITE_FIELD(ui32RegValue, MSVDX_CORE, CR_MMU_BANK_INDEX, CR_MMU_BANK_N_INDEX_M, TEST_REG_FIELD_2BITS_VALUE);
    /* write the test value to the register */
    REGIO_WRITE_REGISTER(psContext, MSVDX_CORE, CR_MMU_BANK_INDEX, ui32RegValue);

    /* read the register again */
    ui32RegValue = REGIO_READ_REGISTER(psContext, MSVDX_CORE, CR_MMU_BANK_INDEX);
    /* read the register field */
    ui32RegValue = REGIO_READ_FIELD(ui32RegValue, MSVDX_CORE, CR_MMU_BANK_INDEX, CR_MMU_BANK_N_INDEX_M);
    /* check the read value with the expected written value */
    if(TEST_REG_FIELD_2BITS_VALUE != ui32RegValue)
    {
        /* register test fail */
        ui32Result = IMG_ERROR_DEVICE_UNAVAILABLE;
    }
    else
    {
        /* register test passed: Do nothing */
    }

    /* re-write the register with the default value */
    REGIO_WRITE_REGISTER(psContext, MSVDX_CORE, CR_MMU_BANK_INDEX, ui32OldRegValue);




    /* TEST ON CR_VDMC_CHROMA_ERROR_BASE_ADDR register */

    /* read the CR_VDMC_CHROMA_ERROR_BASE_ADDR register and store the default value */
    ui32OldRegValue = REGIO_READ_REGISTER(psContext, MSVDX_VDMC, CR_VDMC_CHROMA_ERROR_BASE_ADDR);
    /* check the read value with the default value */
    if(CR_VDMC_2D_FLT_DATA_DEF_VALUE != ui32OldRegValue)
    {
        /* register test fail */
        ui32Result = IMG_ERROR_DEVICE_UNAVAILABLE;
    }
    else
    {
        /* register test passed: do nothing */
    }

    /* reset register value to update: can be equal to old value */
    ui32RegValue = 0;
    /* write the test value to the writable register field */
    REGIO_WRITE_FIELD(ui32RegValue, MSVDX_VDMC, CR_VDMC_CHROMA_ERROR_BASE_ADDR, CR_VDMC_CHROMA_ERROR_BASE, TEST_REG_FIELD_26BITS_VALUE);
    /* write the test value to the register */
    REGIO_WRITE_REGISTER(psContext, MSVDX_VDMC, CR_VDMC_CHROMA_ERROR_BASE_ADDR, ui32RegValue);

    /* read the register again */
    ui32RegValue = REGIO_READ_REGISTER(psContext, MSVDX_VDMC, CR_VDMC_CHROMA_ERROR_BASE_ADDR);

    /* read the register field */
    ui32RegValue = REGIO_READ_FIELD(ui32RegValue, MSVDX_VDMC, CR_VDMC_CHROMA_ERROR_BASE_ADDR, CR_VDMC_CHROMA_ERROR_BASE);
    /* check the read value with the expected written value */
    if(TEST_REG_FIELD_26BITS_VALUE != ui32RegValue)
    {
        /* register test fail */
        ui32Result = IMG_ERROR_DEVICE_UNAVAILABLE;
    }
    else
    {
        /* register test passed: Do nothing */
    }

    /* re-write the register with the default value */
    REGIO_WRITE_REGISTER(psContext, MSVDX_VDMC, CR_VDMC_CHROMA_ERROR_BASE_ADDR, ui32OldRegValue);




    /* TEST ON CR_VDEB_VIRTUAL_FIFO_CONTROL register */

    /* read the CR_VDEB_VIRTUAL_FIFO_CONTROL register and store the default value */
    ui32OldRegValue = REGIO_READ_REGISTER(psContext, MSVDX_VDEB, CR_VDEB_VIRTUAL_FIFO_CONTROL);
    /* check the read value with the default value */
    if(CR_VDEB_VIRT_CTRL_DEF_VALUE != ui32OldRegValue)
    {
        /* register test fail */
        ui32Result = IMG_ERROR_DEVICE_UNAVAILABLE;
    }
    else
    {
        /* register test passed: do nothing */
    }

    /* reset register value to update: can be equal to old value */
    ui32RegValue = 0;
    /* write the test value to the writable register field */
    REGIO_WRITE_FIELD(ui32RegValue, MSVDX_VDEB, CR_VDEB_VIRTUAL_FIFO_CONTROL, CR_VDEB_VF_FLOWRATE_DIV, TEST_REG_FIELD_8BITS_VALUE);
    /* write the test value to the register */
    REGIO_WRITE_REGISTER(psContext, MSVDX_VDEB, CR_VDEB_VIRTUAL_FIFO_CONTROL, ui32RegValue);

    /* read the register again */
    ui32RegValue = REGIO_READ_REGISTER(psContext, MSVDX_VDEB, CR_VDEB_VIRTUAL_FIFO_CONTROL);
    /* read the register field */
    ui32RegValue = REGIO_READ_FIELD(ui32RegValue, MSVDX_VDEB, CR_VDEB_VIRTUAL_FIFO_CONTROL, CR_VDEB_VF_FLOWRATE_DIV);
    /* check the read value with the expected written value */
    if(TEST_REG_FIELD_8BITS_VALUE != ui32RegValue)
    {
        /* register test fail */
        ui32Result = IMG_ERROR_DEVICE_UNAVAILABLE;
    }
    else
    {
        /* register test passed: Do nothing */
    }

    /* re-write the register with the default value */
    REGIO_WRITE_REGISTER(psContext, MSVDX_VDEB, CR_VDEB_VIRTUAL_FIFO_CONTROL, ui32OldRegValue);

    return ui32Result;
}
#endif


#ifdef STACK_USAGE_TEST
/*!
******************************************************************************

 @Function              MSVDX_GetStackUsage

 @Description

 Function to get stack usage from MSVDXIO layer.

******************************************************************************/
IMG_RESULT
MSVDX_GetStackUsage(
    const IMG_HANDLE    hVxd,
    IMG_UINT32        * paui32StackInfo
)
{
    MSVDX_sContext  * psContext = (MSVDX_sContext *)hVxd;
    IMG_UINT32        ui32Result = IMG_SUCCESS;

    /* call VXDIO function to get stack usage */
    ui32Result = MSVDXIO_GetStackUsage(psContext->hMsvdxIo, paui32StackInfo);

    return ui32Result;
}
#endif


/*!
******************************************************************************

 @Function              MSVDX_Initialise

******************************************************************************/
IMG_RESULT
MSVDX_Initialise(
    const IMG_HANDLE        hVxd,
    VDECFW_sCoreInitData  * psInitConfig
)
{
    MSVDX_sContext    * psContext = (MSVDX_sContext *)hVxd;
    IMG_UINT32          ui32Result;

    if (psContext == IMG_NULL)
    {
        REPORT(REPORT_MODULE_VXDIO, REPORT_ERR,
               "An MSVDX context handle must be provided");
        ui32Result = IMG_ERROR_INVALID_PARAMETERS;
        goto error;
    }

    /* Initialise the core. */
    ui32Result = msvdx_GetCoreInit(&psContext->sProps,
                                   &psContext->sRendec,
                                   psInitConfig);
    if (IMG_SUCCESS != ui32Result)
    {
        REPORT(REPORT_MODULE_HWCTRL,
                REPORT_ERR,
                "Failed initialising core %d",
                psContext->ui32CoreNum);
        goto error;
    }

#ifdef VDEC_MSVDX_HARDWARE
    // Reset MSVDX: the core may have been powered down.
    ui32Result = msvdx_CoreReset(hVxd, IMG_TRUE);
    IMG_ASSERT(ui32Result == IMG_SUCCESS);
    if (ui32Result != IMG_SUCCESS)
    {
        goto error;
    }

#ifdef POST_TEST
    if(IMG_TRUE == psContext->bPost && !psContext->bSecureStream)
    {
        /* Execute core ID test */
        ui32Result = checkCoreRevision(psContext);
        if(IMG_SUCCESS == ui32Result)
        {
            REPORT(REPORT_MODULE_VXDIO, REPORT_NOTICE,
                "***** POST: Core ID Test PASSED");
        }
        else
        {
            REPORT(REPORT_MODULE_VXDIO, REPORT_ERR,
                "***** POST: Core ID Test FAILED");
            goto error;
        }

#ifdef SEC_USE_REAL_FW
        /* Execute registers test */
        /* ATTENTION: this tests must not be performed with CSIM. Elsewhere the software remains blocked. */
        ui32Result = executeRegTest(psContext);
        if(IMG_SUCCESS == ui32Result)
        {
            REPORT(REPORT_MODULE_VXDIO, REPORT_NOTICE,
                "***** POST: Register Test PASSED");
        }
        else
        {
            REPORT(REPORT_MODULE_VXDIO, REPORT_ERR,
                "***** POST: Register Test FAILED");
            goto error;
        }
#endif

    }
#endif


#ifdef USE_MTX_BREAK_TRAP
    msvdx_SetMTXBreakTrap(psContext, IMG_TRUE);
#endif /* USE_MTX_BREAK_TRAP */

    /* Load the firmware base component. */
    if (psContext->bSecureStream == IMG_TRUE)
	{
    //Using Secure Interface
		VXD_SECURE_LoadCoreFW sArgs = {
            .ui32MmuCtrl2 = psInitConfig->sMmuConfig.ui32MmuControl2,
            .ui32PtdPhysAddr = psContext->sPtdInfo.ui32PtdPhysAddr,
        };

		ui32Result = SECURE_REE_SendMessage(psContext->ui32SecureId,
                                    (IMG_BYTE *)&sArgs,
                                    sizeof(sArgs),
                                    ENDPOINT_VXD_LOAD_CORE_FW);
	}
	else
    {
#ifdef USE_MTX_DMA_LOAD
        /* Configure the MMU to DMA the firmware base component. */
        REGIO_WRITE_REGISTER(psContext, MSVDX_CORE, CR_MMU_CONTROL2, psInitConfig->sMmuConfig.ui32MmuControl2);
        /* Set base address of PTD */
        REGIO_WRITE_REGISTER(psContext, MSVDX_CORE, CR_MMU_DIR_LIST_BASE, psContext->sPtdInfo.ui32PtdPhysAddr);
        /* Enable mmu for all requestors */
        REGIO_WRITE_REGISTER(psContext, MSVDX_CORE, CR_MMU_CONTROL0, 0);
#endif /* USE_MTX_DMA_LOAD */

        ui32Result = MSVDXIO_LoadBaseFirmware(psContext->hMsvdxIo);
    }

    IMG_ASSERT(ui32Result == IMG_SUCCESS);
    if (ui32Result != IMG_SUCCESS)
    {
        REPORT(REPORT_MODULE_HWCTRL, REPORT_ERR,
                "Failed loading firmware base component for core %d",
                psContext->ui32CoreNum);
        goto error;
    }
    else
    {
        //REPORT(REPORT_MODULE_HWCTRL, REPORT_DEBUG,
        //        "Passed loading firmware base component for core %d",
        //        m->ui32CoreNum);
        DEBUG_REPORT(REPORT_MODULE_HWCTRL,
                "Passed loading firmware base component for core %d",
                psContext->ui32CoreNum);
    }

#ifdef USE_MTX_BREAK_TRAP
    // You can break on this function to set breakpoints in MTX.
    // Once it's executed, MTX will jump out of the break trap loop and begin
    // normal operation.
    msvdx_SetMTXBreakTrap(psContext, IMG_FALSE);
#endif /* USE_MTX_BREAK_TRAP */

#ifndef SYSBRG_BRIDGING
    ui32Result = TALINTERRUPT_RegisterCheckFunc(psContext->hSysRegSpace,
                                                msvdx_CheckInterruptFunc,
                                                psContext->hMsvdxIo);
    if (ui32Result != IMG_SUCCESS)
    {
        goto error;
    }
#endif /* not SYSBRG_BRIDGING */
#endif /* VDEC_MSVDX_HARDWARE */

    return IMG_SUCCESS;

error:
    return ui32Result;
}



/*!
******************************************************************************

 @Function              MSVDX_PrepareFirmware

******************************************************************************/
IMG_RESULT
MSVDX_PrepareFirmware(
    const IMG_HANDLE            hVxd,
    const MSVDXIO_sFw         * psFw,
    const VXDIO_sPtdInfo      * psPtdInfo
)
{
    MSVDX_sContext    * psContext = (MSVDX_sContext *)hVxd;
    IMG_UINT32          ui32Result;

    if (psContext == IMG_NULL)
    {
        REPORT(REPORT_MODULE_VXDIO, REPORT_ERR,
               "An MSVDX context handle must be provided");
        ui32Result = IMG_ERROR_INVALID_PARAMETERS;
        goto error;
    }

    /* Prepare the firmware for loading to MTX. */
#ifdef VDEC_MSVDX_HARDWARE
    if (psContext->bSecureStream == IMG_TRUE)
    {
        VXD_SECURE_sPrepareFirmwareArgs sArgs;

        sArgs.ui32FwBufCpuPhys = psFw->sSecFw.sBufInfo.paSecPhysAddr;
        sArgs.ui32FwBufSizeBytes = psFw->sSecFw.sBufInfo.ui32BufSize;
        sArgs.ui32DevVirtAddr = psFw->sSecFw.sBufInfo.ui32DevVirt;

        ui32Result =
            SECURE_REE_SendMessageWithBuf(psContext->ui32SecureId,
                                          (IMG_BYTE *)&sArgs,
                                          sizeof(sArgs),
                                          ENDPOINT_VXD_PREPARE_FIRMWARE,
                                          psFw->sSecFw.sBinInfo.pui8FwBin,
                                          psFw->sSecFw.sBinInfo.ui32FwBinSizeBytes);
    }
    else
    {
        ui32Result = MSVDXIO_PrepareFirmware(psContext->hMsvdxIo,
                                             (IMG_HANDLE)psFw);
    }

    IMG_ASSERT(ui32Result == IMG_SUCCESS);
    if (ui32Result != IMG_SUCCESS)
    {
        goto error;
    }
#endif /* VDEC_MSVDX_HARDWARE */

    // Copy the device PTD for power-up initialisation.
    psContext->sPtdInfo = *psPtdInfo;

    return IMG_SUCCESS;

error:
    return ui32Result;
}

/*!
******************************************************************************

 @Function              MSVDX_Destroy

******************************************************************************/
IMG_RESULT
MSVDX_Destroy(
    const IMG_HANDLE  hVxd
)
{
    MSVDX_sContext  * psContext = (MSVDX_sContext *)hVxd;
    IMG_UINT32        ui32Result;

    if (psContext == IMG_NULL)
    {
        REPORT(REPORT_MODULE_VXDIO, REPORT_ERR,
               "An MSVDX context handle must be provided");
        ui32Result = IMG_ERROR_INVALID_PARAMETERS;
        goto error;
    }

#ifdef VDEC_MSVDX_HARDWARE
    if (psContext->eDecodeLevel > VDECDD_DECODELEVEL_DECODER)
    {
        VXD_sDeInitialiseArgs   sDeInitialiseArgs;

        MSVDX_DeInitialise(hVxd);

        /* Attempt to initialise MSVDX if it is used. */
#ifdef __FAKE_MTX_INTERFACE__
        /* If fake MTX is used... */
        if (psContext->bFakeMtx)
        {
            /* ...stop it now. */
            ui32Result = VDECFW_StopFakeMTX(psContext->ui32CoreNum);
            if (ui32Result != IMG_SUCCESS)
            {
                goto error;
            }
        }
#endif /* __FAKE_MTX_INTERFACE__ */

        if (psContext->bSecureStream == IMG_TRUE)
        {
            //Using Secure Interface
            ui32Result = SECURE_REE_SendMessage(psContext->ui32SecureId,
                                            (IMG_BYTE *)&sDeInitialiseArgs,
                                            sizeof(sDeInitialiseArgs),
                                            ENDPOINT_VXD_DEINITIALISE);
            if (ui32Result != IMG_SUCCESS)
            {
                goto error;
            }

            ui32Result = SECURE_REE_ReleaseId(psContext->ui32SecureId);
        }
        else
        {
        	ui32Result = MSVDXIO_DeInitialise(psContext->hMsvdxIo);
        }

        if (ui32Result != IMG_SUCCESS)
        {
            goto error;
        }
    }
#endif

    IMG_FREE(psContext);

    return IMG_SUCCESS;

error:
    return ui32Result;
}


/*!
******************************************************************************

 @Function              MSVDX_Create

******************************************************************************/
IMG_RESULT
MSVDX_Create(
    const VDECDD_sDdDevConfig * psDdDevConfig,
    IMG_UINT32                  ui32CoreNum,
    VXD_sCoreProps            * psCoreProps,
    HWCTRL_sRendec            * psRendec,
    IMG_HANDLE                * phMsvdxContext,
    HWCTRL_sFunctions         * psVxdFuncs,
    IMG_BOOL                    bSecureStream
)
{
    MSVDX_sContext    * psContext = IMG_NULL;
    IMG_UINT32          ui32Result;
    VXD_sInitialiseArgs sInitialiseArgs = { 0 };

    IMG_ASSERT(psCoreProps);
    IMG_ASSERT(psRendec);

    if (phMsvdxContext == IMG_NULL)
    {
        REPORT(REPORT_MODULE_VXDIO, REPORT_ERR,
               "A pointer to MSVDX context handle must be provided");
        ui32Result = IMG_ERROR_INVALID_PARAMETERS;
        goto error;
    }

    VDEC_MALLOC(psContext);
    IMG_ASSERT(psContext);
    if(psContext == IMG_NULL)
    {
        REPORT(REPORT_MODULE_VXDIO, REPORT_ERR,
            "Failed to allocate memory for MSVDX context");
        return IMG_ERROR_OUT_OF_MEMORY;
    }
    VDEC_BZERO(psContext);
    *phMsvdxContext = psContext;

    /* Populating secure flag variable */
    psContext->bSecureStream = bSecureStream;

    psContext->ui32CoreNum = ui32CoreNum;
    psContext->bFakeMtx = psDdDevConfig->bFakeMtx;
    psContext->eDecodeLevel = psDdDevConfig->eDecodeLevel;
    psContext->bCSim = psDdDevConfig->bCSim;
#ifdef POST_TEST
    /* update POST flag */
    psContext->bPost = psDdDevConfig->bPost;
    sInitialiseArgs.bPost = psDdDevConfig->bPost;
#endif /* POST_TEST */
#ifdef STACK_USAGE_TEST
    /* update STACK USAGE TEST flag */
    psContext->bStackUsageTest = psDdDevConfig->bStackUsageTest;
    sInitialiseArgs.bStackUsageTest = psDdDevConfig->bStackUsageTest;
#endif /* STACK_USAGE_TEST */

#ifdef VDEC_MSVDX_HARDWARE
    if (psDdDevConfig->eDecodeLevel > VDECDD_DECODELEVEL_DECODER)
    {
        if (psContext->bSecureStream == IMG_TRUE)
        {

            switch (ui32CoreNum)
            {
            case 0:
                psContext->eSecureCore = VXD_CORE0;
                break;

            case 1:
                psContext->eSecureCore = VXD_CORE1;
                break;

            default:
                IMG_ASSERT(IMG_FALSE);
                break;
            }

            ui32Result = SECURE_REE_GetId(psContext->eSecureCore, &psContext->ui32SecureId);
            IMG_ASSERT(ui32Result == IMG_SUCCESS);

            /* Attempt to initialise MSVDX if it is used. */
            ui32Result = SECURE_REE_SendMessage(psContext->ui32SecureId,
                                            (IMG_BYTE *)&sInitialiseArgs,
                                            sizeof(sInitialiseArgs),
                                            ENDPOINT_VXD_INITIALISE);
            if (ui32Result != IMG_SUCCESS)
            {
                goto error;
            }

        }
        else
        {
            IMG_BOOL bPostTest = IMG_FALSE;
            IMG_BOOL bStackUsageTest = IMG_FALSE;
#ifdef POST_TEST
            bPostTest = psDdDevConfig->bPost;
#endif /* POST_TEST */
#ifdef STACK_USAGE_TEST
            bStackUsageTest = psDdDevConfig->bStackUsageTest;
#endif /* STACK_USAGE_TEST */
            ui32Result = MSVDXIO_Initialise(bPostTest,
                                            bStackUsageTest,
                                            &psContext->hMsvdxIo);
            if (ui32Result != IMG_SUCCESS)
            {
                goto error;
            }

            sInitialiseArgs.ui32CoreRev =
                REGIO_READ_REGISTER(psContext, MSVDX_CORE, CR_MSVDX_CORE_REV);
            sInitialiseArgs.ui32Internal =
                REGIO_READ_REGISTER(psContext, MSVDX_CORE, CR_MSVDX_INTERNAL);
            sInitialiseArgs.ui32Latency =
                REGIO_READ_REGISTER(psContext, MSVDX_CORE, CR_MSVDX_LATENCY_CONFIG);
            sInitialiseArgs.ui32MmuStatus =
                REGIO_READ_REGISTER(psContext, MSVDX_CORE, CR_MMU_STATUS);
            sInitialiseArgs.ui32CoreId =
                REGIO_READ_REGISTER(psContext, MSVDX_CORE, CR_MSVDX_CORE_ID);
            sInitialiseArgs.ui32MultiCore =
                REGIO_READ_REGISTER(psContext, MSVDX_CORE, CR_MSVDX_MULTICORE_STATUS);

        }

        ui32Result = msvdx_GetCoreProperties(psContext,
                                             &psContext->sProps,
                                             &sInitialiseArgs);

        if (ui32Result != IMG_SUCCESS)
        {
            goto error;
        }

        if (psDdDevConfig->ui32RendecSize)
        {
            /* Set Rendec Burst READ/WRITE sizes */
            psContext->sRendec.ui8BurstSizeRead = 1;
            psContext->sRendec.ui8BurstSizeWrite = 1;

            /* Create two buffers with a 4:1 split both multiples of 4k */
            psContext->sRendec.aui32BufferSize[1] = psDdDevConfig->ui32RendecSize / 5;
            psContext->sRendec.aui32BufferSize[1] &= ~0xfff;
            psContext->sRendec.aui32BufferSize[0] = psDdDevConfig->ui32RendecSize - psContext->sRendec.aui32BufferSize[1];
            psContext->sRendec.aui32BufferSize[0] &= ~0xfff;

            psContext->sRendec.aui32InitialContext[0] = 0x00101010;
            psContext->sRendec.aui32InitialContext[1] = 0x00101010;
            psContext->sRendec.aui32InitialContext[2] = 0x00101010;
            psContext->sRendec.aui32InitialContext[3] = 0x00101010;
            psContext->sRendec.aui32InitialContext[4] = 0x00101010;
            psContext->sRendec.aui32InitialContext[5] = 0x00101010;

            psContext->sRendec.bInUse = IMG_TRUE;
        }

        if (psDdDevConfig->ui32MemoryStallingMin &&
            psDdDevConfig->ui32MemoryStallingMax &&
            psDdDevConfig->ui32MemoryStallingRatio)
        {
            IMG_UINT32 ui32Reg;

            IMG_ASSERT(psDdDevConfig->ui32MemoryStallingMin <= psDdDevConfig->ui32MemoryStallingMax);
            IMG_ASSERT(psDdDevConfig->ui32MemoryStallingRatio < 10);

            // Configure MSVDX memory stalling with the min, max and ratio of accesses.
            // Ratio: Percentage of transactions to stall (in 10% steps, i.e. takes values 0-10).
            ui32Reg = REGIO_READ_REGISTER(psContext, MSVDX_TEST, MEM_CTRL);
            REGIO_WRITE_FIELD(ui32Reg, MSVDX_TEST, MEM_CTRL, MEM_LATENCY_MIN,
                              psDdDevConfig->ui32MemoryStallingMin);
            REGIO_WRITE_FIELD(ui32Reg, MSVDX_TEST, MEM_CTRL, MEM_LATENCY_MAX,
                              psDdDevConfig->ui32MemoryStallingMax);
            REGIO_WRITE_FIELD(ui32Reg, MSVDX_TEST, MEM_CTRL, MEM_STALL_RATIO,
                              psDdDevConfig->ui32MemoryStallingRatio);
            REGIO_WRITE_REGISTER(psContext, MSVDX_TEST, MEM_CTRL, ui32Reg);
        }
        /*else
        {
            // Disable memory stalling if not defined.
            ui32Reg = REGIO_READ_REGISTER(psContext, MSVDX_TEST, MEM_CTRL);
            REGIO_WRITE_FIELD(ui32Reg, MSVDX_TEST, MEM_CTRL, MEM_LATENCY_MIN, 0);
            REGIO_WRITE_FIELD(ui32Reg, MSVDX_TEST, MEM_CTRL, MEM_LATENCY_MAX, 0);
            REGIO_WRITE_FIELD(ui32Reg, MSVDX_TEST, MEM_CTRL, MEM_STALL_RATIO, 0);
            REGIO_WRITE_REGISTER(psContext, MSVDX_TEST, MEM_CTRL, ui32Reg);
        }*/
    }
    else
#endif /* #ifdef VDEC_MSVDX_HARDWARE */
    {
        DBGOPT_sValue sVal;
        DBGOPT_eType eType;

        if (DBGOPTKM_GetSize("CoreConfig", DBGOPT_TYPE_BUF) == sizeof(psContext->sProps) &&
            DBGOPTKM_Get("CoreConfig", &eType, &sVal))
        {
            psContext->sProps = *((VXD_sCoreProps *) sVal.sBufVal.pvVal);

            DEBUG_REPORT(REPORT_MODULE_HWCTRL,
                         "Setting core configuration from debug options");
        }
        else
        {
            VDEC_eVidStd eVidStd;

            psContext->sProps.abH264[0] = IMG_TRUE;
            psContext->sProps.bHdSupport = IMG_TRUE;
            psContext->sProps.abRotationSupport[0] = IMG_TRUE;
            psContext->sProps.abScalingSupport[0] = IMG_TRUE;
            psContext->sProps.bScalingWithOold = IMG_TRUE;
            psContext->sProps.bChromaUpsample = IMG_TRUE;
            psContext->sProps.b64ByteFixedStrides = IMG_TRUE;
            psContext->sProps.bExtendedStrides = IMG_TRUE;

            if (ui32CoreNum == VDECDD_MSVDX_CORE_MASTER)
            {
                psContext->sProps.abMpeg2[0] = IMG_TRUE;
                psContext->sProps.abMpeg4[0] = IMG_TRUE;
                psContext->sProps.abVc1[0] = IMG_TRUE;
                psContext->sProps.abAvs[0] = IMG_TRUE;
                psContext->sProps.abReal[0] = IMG_TRUE;
                psContext->sProps.abJpeg[0] = IMG_TRUE;
                psContext->sProps.abVp6[0] = IMG_TRUE;
                psContext->sProps.abVp8[0] = IMG_TRUE;
                psContext->sProps.abSorenson[0] = IMG_TRUE;
            }

            for (eVidStd = VDEC_STD_UNDEFINED; eVidStd < VDEC_STD_MAX; eVidStd++)
            {
                psContext->sProps.asVidStdProps[eVidStd].eVidStd = eVidStd;
                psContext->sProps.asVidStdProps[eVidStd].ui32CoreRev = CORE_REVISION(0,0,0);

                psContext->sProps.asVidStdProps[eVidStd].ui32MinWidth = 80;
                psContext->sProps.asVidStdProps[eVidStd].ui32MinHeight = 16;
                psContext->sProps.asVidStdProps[eVidStd].ui32MaxWidth = 4096;
                psContext->sProps.asVidStdProps[eVidStd].ui32MaxHeight = 4096;
                psContext->sProps.asVidStdProps[eVidStd].ui32MaxMacroblocks = 0x10000;

                psContext->sProps.asVidStdProps[eVidStd].ui32MaxLumaBitDepth = 10;
                psContext->sProps.asVidStdProps[eVidStd].ui32MaxChromaBitDepth = 10;
                psContext->sProps.asVidStdProps[eVidStd].eMaxChromaFormat = PIXEL_FORMAT_444;
            }

            psContext->sProps.ui32NumCores = 1;
            psContext->sProps.aui32NumStreams[0] = 1;
            psContext->sProps.eMmuType = MMU_TYPE_40BIT;
            psContext->sProps.ui32NumPixelPipes = 1;
        }
    }

    HWCTRL_PrintMSVDXProps(&psContext->sProps, psContext->ui32CoreNum);

#ifndef SYSBRG_BRIDGING
    {
        IMG_CHAR    acMemSpceName[32];

        // Create the memory space name prefix based upon the core number.
        sprintf(acMemSpceName, "C%d_", ui32CoreNum);
        strncat(acMemSpceName, "REG_MSVDX_SYS", 16);
        psContext->hSysRegSpace = TAL_GetMemSpaceHandle(acMemSpceName);
    }
#endif

    *psCoreProps = psContext->sProps;
    *psRendec = psContext->sRendec;

    psVxdFuncs->pfnReadVLR = MSVDX_ReadVLR;
    psVxdFuncs->pfnWriteVLR = MSVDX_WriteVLR;
    psVxdFuncs->pfnHandleInt = MSVDX_HandleInterrupts;
    psVxdFuncs->pfnGetIntStatus = MSVDX_GetIntStatus;
    psVxdFuncs->pfnDumpRegisters = MSVDX_DumpRegisters;
    psVxdFuncs->pfnSendFirmwareMessage = MSVDX_SendFirmwareMessage;
    psVxdFuncs->pfnGetCoreState = MSVDX_GetCoreState;
    psVxdFuncs->pfnFlushMmuCache = MSVDX_FlushMmuCache;
    psVxdFuncs->pfnInitialise = MSVDX_Initialise;
    psVxdFuncs->pfnDeInitialise = MSVDX_DeInitialise;
    psVxdFuncs->pfnDestroy = MSVDX_Destroy;
    psVxdFuncs->pfnPrepareFirmware = MSVDX_PrepareFirmware;
    psVxdFuncs->pfnGetRegsOffsets = IMG_NULL;
    psVxdFuncs->pfnGetInitMessage = MSVDX_GetInitMessage;
    psVxdFuncs->pfnGetDecodeMessage = MSVDX_GetDecodeMessage;
    psVxdFuncs->pfnGetBatchMessage = MSVDX_GetBatchMessage;
    psVxdFuncs->pfnGetEndBytesMessage = MSVDX_GetEndBytesMessage;
    psVxdFuncs->pfnGetFragmentMessage = MSVDX_GetFragmentMessage;
    psVxdFuncs->pfnReturnMsg = MSVDX_ReturnMsg;
    psVxdFuncs->pfnProcessMessage = MSVDX_ProcessMessage;

    return IMG_SUCCESS;

error:

    if(psContext)
    {
        IMG_FREE(psContext);
    }
    return ui32Result;
}


/* EOF */
