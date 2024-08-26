/*!
 *****************************************************************************
 *
 * @File       h264_secure_parser.c
 * @Title      h.264 secure data unit parsing
 * @Description    This file contains h.264 SPS/PPS/Slice header structure and parsing
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

#include "h264_secure_parser.h"
#include "h264_secure_sei_parser.h"
#include "swsr.h"


#define SL_MAX_REF_IDX        32
#define VUI_CPB_CNT_MAX       32
#define MAX_SPS_COUNT         32
#define MAX_PPS_COUNT         256
#define MAX_SLICE_GROUPMBS    65536        // changed from 810
#define MAX_SPS_COUNT         32
#define MAX_PPS_COUNT         256
#define MAX_SLICEGROUP_COUNT  8
#define MAX_WIDTH_IN_MBS      256
#define MAX_HEIGHT_IN_MBS     256
#define MAX_COLOR_PLANE       4
#define H264_MAX_SGM_SIZE     8196

#define H264_MAX_CHROMA_QP_INDEX_OFFSET (12)
#define H264_MIN_CHROMA_QP_INDEX_OFFSET (-12)

/*!
******************************************************************************
            Macros for Minimum and Maximum value
******************************************************************************/
#ifndef MIN
#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#endif

#ifndef MAX
#define MAX(a, b) (((a) > (b)) ? (a) : (b))
#endif

#define SET_IF_NOT_DETERMINED_YET(determined,condition,target,value)  \
    do{                                                               \
        if ((!(determined)) && (condition))                           \
        {                                                             \
            target = value;                                           \
            determined = IMG_TRUE;                                    \
        }                                                             \
    }while(0)

//seq_parameter_set_id is always in range 0-31, so we can add offset indicating subsequence header
#define GET_SUBSET_ID(eNalUnitType, id) ((eNalUnitType == H264_NALTYPE_SLICE_SCALABLE ||                \
                                          eNalUnitType == H264_NALTYPE_SLICE_IDR_SCALABLE ||            \
                                          eNalUnitType == H264_NALTYPE_SUBSET_SPS ) ? ((id)+32) : (id))

extern BSPP_sH264SEIInfo * BSPP_GetSEIDataInfo(IMG_HANDLE hStrRes);

/*!
******************************************************************************

                AVC Profile IDC definitions

******************************************************************************/
typedef enum {
    H264_Profile_CAVLC444   = 44,   //!< YUV 4:4:4/14 "CAVLC 4:4:4"
    H264_Profile_BASELINE   = 66,   //!< YUV 4:2:0/8  "Baseline"
    H264_Profile_MAIN       = 77,   //!< YUV 4:2:0/8  "Main"
    H264_Profile_SCALABLE   = 83,   //!< YUV 4:2:0/8  "Scalable"
    H264_Profile_EXTENDED   = 88,   //!< YUV 4:2:0/8  "Extended"
    H264_Profile_HIGH       = 100,  //!< YUV 4:2:0/8  "High"
    H264_Profile_HIGH10     = 110,  //!< YUV 4:2:0/10 "High 10"
    H264_Profile_MVC_HIGH   = 118,  //!< YUV 4:2:0/8  "Multiview High"
    H264_Profile_HIGH422    = 122,  //!< YUV 4:2:2/10 "High 4:2:2"
    H264_Profile_MVC_STEREO = 128,  //!< YUV 4:2:0/8  "Stereo High"
    H264_Profile_HIGH444    = 244,  //!< YUV 4:4:4/14 "High 4:4:4"
}   H264_Profile_IDC;


// Remap H.264 colour format into internal representation.
static const PIXEL_FormatIdc aePixelFormatIdc[] =
{
    PIXEL_FORMAT_MONO,
    PIXEL_FORMAT_420,
    PIXEL_FORMAT_422,
    PIXEL_FORMAT_444
};


/*!
******************************************************************************

                Pixel Aspect Ratio

******************************************************************************/
static const IMG_UINT16 aPixelAspect  [17][2]={
 {0, 1},
 {1, 1},
 {12, 11},
 {10, 11},
 {16, 11},
 {40, 33},
 {24, 11},
 {20, 11},
 {32, 11},
 {80, 33},
 {18, 11},
 {15, 11},
 {64, 33},
 {160,99},
 {4, 3},
 {3, 2},
 {2, 1},
};

/*!
******************************************************************************

 Table 7-3, 7-4: Default Scaling lists

******************************************************************************/
static const IMG_UINT8 Default_4x4_Intra[16] =
{
     6, 13, 13, 20,
    20, 20, 28, 28,
    28, 28, 32, 32,
    32, 37, 37, 42
};
static const IMG_UINT8 Default_4x4_Inter[16] =
{
    10, 14, 14, 20,
    20, 20, 24, 24,
    24, 24, 27, 27,
    27, 30, 30, 34
};
static const IMG_UINT8 Default_8x8_Intra[64] =
{
     6, 10, 10, 13, 11, 13, 16, 16,
    16, 16, 18, 18, 18, 18, 18, 23,
    23, 23, 23, 23, 23, 25, 25, 25,
    25, 25, 25, 25, 27, 27, 27, 27,
    27, 27, 27, 27, 29, 29, 29, 29,
    29, 29, 29, 31, 31, 31, 31, 31,
    31, 33, 33, 33, 33, 33, 36, 36,
    36, 36, 38, 38, 38, 40, 40, 42
};
static const IMG_UINT8 Default_8x8_Inter[64] =
{
     9, 13, 13, 15, 13, 15, 17, 17,
    17, 17, 19, 19, 19, 19, 19, 21,
    21, 21, 21, 21, 21, 22, 22, 22,
    22, 22, 22, 22, 24, 24, 24, 24,
    24, 24, 24, 24, 25, 25, 25, 25,
    25, 25, 25, 27, 27, 27, 27, 27,
    27, 28, 28, 28, 28, 28, 30, 30,
    30, 30, 32, 32, 32, 33, 33, 35
};

static const IMG_UINT8  Default_4x4_Org[16] = { //to be use if no q matrix is chosen
    16, 16, 16, 16,
    16, 16, 16, 16,
    16, 16, 16, 16,
    16, 16, 16, 16
};

static const IMG_UINT8  Default_8x8_Org[64] = { //to be use if no q matrix is chosen
    16, 16, 16, 16, 16, 16, 16, 16,
    16, 16, 16, 16, 16, 16, 16, 16,
    16, 16, 16, 16, 16, 16, 16, 16,
    16, 16, 16, 16, 16, 16, 16, 16,
    16, 16, 16, 16, 16, 16, 16, 16,
    16, 16, 16, 16, 16, 16, 16, 16,
    16, 16, 16, 16, 16, 16, 16, 16,
    16, 16, 16, 16, 16, 16, 16, 16
};
/*****************************************************************************/

/* source: ITU-T H.264 2010/03, page 20 Table 6-1 */
const IMG_INT32 bspp_ai32subHeightC[] =
{
    -1, 2, 1, 1
};

/* source: ITU-T H.264 2010/03, page 20 Table 6-1 */
const IMG_INT32 bspp_ai32subWidthC[] =
{
    -1, 2, 2, 1
};

static IMG_INT32 bspp_h264_get_SubWidthC(
    IMG_INT32  chroma_format_idc,
    IMG_BOOL   separate_colour_plane_flag
)
{
    return bspp_ai32subWidthC[chroma_format_idc];
}

static IMG_INT32 bspp_h264_get_SubHeightC(
    IMG_INT32  chroma_format_idc,
    IMG_BOOL   separate_colour_plane_flag
)
{
    return bspp_ai32subHeightC[chroma_format_idc];
}

/*****************************************************************************/

static IMG_UINT32 H264CeilLog2(
    IMG_UINT32  ui32Val
)
{
    IMG_UINT32 ui32Ret = 0;

    ui32Val -= 1;
    while( ui32Val > 0 )
    {
        ui32Val >>= 1;
        ui32Ret++;
    }
    return ui32Ret;
}

/******************************************************************************

 @Function              H264_SecureUnitParser_GetDefaultHRDParam

 @Description           Get default value of the HRD paramter

******************************************************************************/
static IMG_VOID bspp_H264GetDefaultHRDParam(
    BSPP_sH264HRDParamInfo *  psH264HRDParamInfo
)
{
    /* other parameters already set to '0' */
    psH264HRDParamInfo->ui32InitialCPBRemovalDelayLengthMinus1 = 23; /**/
    psH264HRDParamInfo->ui32CPBRemovalDelayLenghtMinus1        = 23; /**/
    psH264HRDParamInfo->ui32DPBOutputDelayLengthMinus1         = 23; /**/
    psH264HRDParamInfo->ui32TimeOffsetLength                   = 24; /**/
}

/******************************************************************************

 @Function              bspp_H264ScalingListParser

 @Description           Parse scaling list

******************************************************************************/
static BSPP_eErrorType bspp_H264ScalingListParser(
    IMG_HANDLE        hSwSrContext,
    IMG_UINT8       * scalingList,
    IMG_UINT8         sizeOfScalingList,
    IMG_UINT8       * pui8useDefaultScalingMatrixFlag
)
{
    BSPP_eErrorType eParseError = BSPP_ERROR_NONE;
    IMG_INT32 delta_scale;
    IMG_UINT32      ui32LastScale = 8;
    IMG_UINT32      ui32NextScale = 8;
    IMG_UINT32 j;

    IMG_ASSERT(hSwSrContext != IMG_NULL);
    IMG_ASSERT(scalingList != IMG_NULL);
    IMG_ASSERT(pui8useDefaultScalingMatrixFlag != IMG_NULL);

    if((scalingList == IMG_NULL) || (hSwSrContext == IMG_NULL) || (pui8useDefaultScalingMatrixFlag == IMG_NULL))
    {
        eParseError = BSPP_ERROR_UNRECOVERABLE;
        return eParseError;
    }

    /* 7.3.2.1.1 */
    for( j = 0; j < sizeOfScalingList; j++ )
    {
        if( ui32NextScale != 0 )
        {
            delta_scale    = SWSR_ReadSignedExpGoulomb( hSwSrContext );
            if( (-128 > delta_scale) || (delta_scale > 127) )
                eParseError |= BSPP_ERROR_INVALID_VALUE;
            ui32NextScale = ( ui32LastScale + delta_scale + 256 ) & 0xff; //% 256;
            *pui8useDefaultScalingMatrixFlag = ( j == 0 && ui32NextScale == 0 );
        }
        scalingList[j] = ( ui32NextScale == 0 )? ui32LastScale : ui32NextScale;
        ui32LastScale = scalingList[j];
    }
    return eParseError;
}

/******************************************************************************

 @Function              bspp_H264SetDefaultVUI

 @Description           Sets default values of the VUI info

******************************************************************************/
static IMG_VOID bspp_H264SetDefaultVUI( BSPP_sH264VUIInfo * psVUIInfo )
{
    IMG_UINT32 * pui32NALHRDBitRateValueMinus1 = IMG_NULL;
    IMG_UINT32 * pui32VCLHRDBitRateValueMinus1 = IMG_NULL;
    IMG_UINT32 * pui32NALHRDCPBSizeValueMinus1 = IMG_NULL;
    IMG_UINT32 * pui32VCLHRDCPBSizeValueMinus1 = IMG_NULL;
    IMG_UINT8 * pui32NALHRDCBRFlag = IMG_NULL;
    IMG_UINT8 * pui32VCLHRDCBRFlag = IMG_NULL;

	//Saving pointers
    pui32NALHRDBitRateValueMinus1 = psVUIInfo->sNALHRDParameters.pui32BitRateValueMinus1;
    pui32VCLHRDBitRateValueMinus1 = psVUIInfo->sVCLHRDParameters.pui32BitRateValueMinus1;

    pui32NALHRDCPBSizeValueMinus1 = psVUIInfo->sNALHRDParameters.pui32CPBSizeValueMinus1;
    pui32VCLHRDCPBSizeValueMinus1 = psVUIInfo->sVCLHRDParameters.pui32CPBSizeValueMinus1;

    pui32NALHRDCBRFlag = psVUIInfo->sNALHRDParameters.pui8CBRFlag;
    pui32VCLHRDCBRFlag = psVUIInfo->sVCLHRDParameters.pui8CBRFlag;

    //Cleaning sVUIInfo
    if(psVUIInfo->sNALHRDParameters.pui32BitRateValueMinus1)
    {
        IMG_MEMSET(psVUIInfo->sNALHRDParameters.pui32BitRateValueMinus1, 0x00, VDEC_H264_MAXIMUMVALUEOFCPB_CNT * sizeof(IMG_UINT32));
    }
    if(psVUIInfo->sNALHRDParameters.pui32CPBSizeValueMinus1)
    {
        IMG_MEMSET(psVUIInfo->sNALHRDParameters.pui32CPBSizeValueMinus1, 0x00, VDEC_H264_MAXIMUMVALUEOFCPB_CNT * sizeof(IMG_UINT32));
    }
    if(psVUIInfo->sVCLHRDParameters.pui32CPBSizeValueMinus1)
    {
        IMG_MEMSET(psVUIInfo->sVCLHRDParameters.pui32CPBSizeValueMinus1, 0x00, VDEC_H264_MAXIMUMVALUEOFCPB_CNT * sizeof(IMG_UINT32));
    }
    if(psVUIInfo->sNALHRDParameters.pui8CBRFlag)
    {
        IMG_MEMSET(psVUIInfo->sNALHRDParameters.pui8CBRFlag, 0x00, VDEC_H264_MAXIMUMVALUEOFCPB_CNT * sizeof(IMG_UINT8));
    }
    if(psVUIInfo->sVCLHRDParameters.pui8CBRFlag)
    {
        IMG_MEMSET(psVUIInfo->sVCLHRDParameters.pui8CBRFlag, 0x00, VDEC_H264_MAXIMUMVALUEOFCPB_CNT * sizeof(IMG_UINT8));
    }

    // Make sure you set default for everything
    IMG_MEMSET(psVUIInfo, 0, sizeof(BSPP_sH264VUIInfo));
    psVUIInfo->video_format                            = 5;
    psVUIInfo->colour_primaries                        = 2;
    psVUIInfo->transfer_characteristics                = 2;
    psVUIInfo->matrix_coefficients                     = 2;
    psVUIInfo->motion_vectors_over_pic_boundaries_flag = 1;
    psVUIInfo->max_bytes_per_pic_denom                 = 2;
    psVUIInfo->max_bits_per_mb_denom                   = 1;
    psVUIInfo->log2_max_mv_length_horizontal           = 16;
    psVUIInfo->log2_max_mv_length_vertical             = 16;
    psVUIInfo->max_dec_frame_buffering                 = 0;
    psVUIInfo->num_reorder_frames                      = psVUIInfo->max_dec_frame_buffering;

    //Restoring pointers
    psVUIInfo->sNALHRDParameters.pui32BitRateValueMinus1 = pui32NALHRDBitRateValueMinus1;
    psVUIInfo->sVCLHRDParameters.pui32BitRateValueMinus1 = pui32VCLHRDBitRateValueMinus1;

    psVUIInfo->sNALHRDParameters.pui32CPBSizeValueMinus1 = pui32NALHRDCPBSizeValueMinus1;
    psVUIInfo->sVCLHRDParameters.pui32CPBSizeValueMinus1 = pui32VCLHRDCPBSizeValueMinus1;

    psVUIInfo->sNALHRDParameters.pui8CBRFlag = pui32NALHRDCBRFlag;
    psVUIInfo->sVCLHRDParameters.pui8CBRFlag = pui32VCLHRDCBRFlag;
}

/******************************************************************************

 @Function              bspp_H264HRDParamParser

 @Description           Parse the HRD parameter

******************************************************************************/
static BSPP_eErrorType bspp_H264HRDParamParser(
    IMG_HANDLE                  hSwSrContext,
     BSPP_sH264HRDParamInfo   * psH264HRDParamInfo
)
{
    IMG_UINT32 SchedSelIdx;
    IMG_ASSERT(hSwSrContext != IMG_NULL);
    psH264HRDParamInfo->ui32CPBCntMinus1 = SWSR_ReadUnsignedExpGoulomb( hSwSrContext );   /**/
    if( psH264HRDParamInfo->ui32CPBCntMinus1 >= 32 )
    {
        REPORT(REPORT_MODULE_BSPP,REPORT_INFO,"pb_cnt_minus1 is not within the range");
    }
    psH264HRDParamInfo->ui32BitRateScale = SWSR_ReadBits(hSwSrContext, 4);
    psH264HRDParamInfo->ui32CPBSizeScale = SWSR_ReadBits(hSwSrContext, 4);

    if(IMG_NULL == psH264HRDParamInfo->pui32BitRateValueMinus1)
    {
        psH264HRDParamInfo->pui32BitRateValueMinus1 = IMG_MALLOC(VDEC_H264_MAXIMUMVALUEOFCPB_CNT * sizeof(IMG_UINT32));
        IMG_ASSERT(IMG_NULL != psH264HRDParamInfo->pui32BitRateValueMinus1);
        if(psH264HRDParamInfo->pui32BitRateValueMinus1 == IMG_NULL)
        {
            return BSPP_ERROR_OUT_OF_MEMORY;
        }
    }
    if(IMG_NULL == psH264HRDParamInfo->pui32CPBSizeValueMinus1)
    {
        psH264HRDParamInfo->pui32CPBSizeValueMinus1 = IMG_MALLOC(VDEC_H264_MAXIMUMVALUEOFCPB_CNT * sizeof(IMG_UINT32));
        IMG_ASSERT(IMG_NULL != psH264HRDParamInfo->pui32CPBSizeValueMinus1);
        if(psH264HRDParamInfo->pui32CPBSizeValueMinus1 == IMG_NULL)
        {
            return BSPP_ERROR_OUT_OF_MEMORY;
        }
    }
    if(IMG_NULL == psH264HRDParamInfo->pui8CBRFlag)
    {
        psH264HRDParamInfo->pui8CBRFlag = IMG_MALLOC(VDEC_H264_MAXIMUMVALUEOFCPB_CNT * sizeof(IMG_UINT8));
        IMG_ASSERT(IMG_NULL != psH264HRDParamInfo->pui8CBRFlag);
        if(psH264HRDParamInfo->pui8CBRFlag == IMG_NULL)
        {
            return BSPP_ERROR_OUT_OF_MEMORY;
        }
        IMG_MEMSET(psH264HRDParamInfo->pui8CBRFlag, 0x00, VDEC_H264_MAXIMUMVALUEOFCPB_CNT * sizeof(IMG_UINT8));
    }

    for( SchedSelIdx = 0; SchedSelIdx <= psH264HRDParamInfo->ui32CPBCntMinus1; SchedSelIdx++ )
    {
        psH264HRDParamInfo->pui32BitRateValueMinus1[ SchedSelIdx ] = SWSR_ReadUnsignedExpGoulomb( hSwSrContext );
        psH264HRDParamInfo->pui32CPBSizeValueMinus1[ SchedSelIdx ] = SWSR_ReadUnsignedExpGoulomb( hSwSrContext );
        if ( psH264HRDParamInfo->pui32CPBSizeValueMinus1[ SchedSelIdx ] == 0xffffffff )
        {
            // 65 bit pattern, 32 0's -1 - 32 0's then value should be 0
            psH264HRDParamInfo->pui32CPBSizeValueMinus1[ SchedSelIdx ] = 0;
        }
        psH264HRDParamInfo->pui8CBRFlag[ SchedSelIdx ] = SWSR_ReadBits(hSwSrContext, 1);
    }

    psH264HRDParamInfo->ui32InitialCPBRemovalDelayLengthMinus1 = SWSR_ReadBits(hSwSrContext, 5);
    psH264HRDParamInfo->ui32CPBRemovalDelayLenghtMinus1        = SWSR_ReadBits(hSwSrContext, 5);
    psH264HRDParamInfo->ui32DPBOutputDelayLengthMinus1         = SWSR_ReadBits(hSwSrContext, 5);
    psH264HRDParamInfo->ui32TimeOffsetLength                   = SWSR_ReadBits(hSwSrContext, 5);

    return BSPP_ERROR_NONE;
}

/******************************************************************************

 @Function              bspp_H264VUIParser

 @Description           Parse the VUI info

******************************************************************************/

static BSPP_eErrorType bspp_H264VUIParser(
    IMG_HANDLE          hSwSrContext,
    BSPP_sH264VUIInfo * psVUIInfo
)
{
    BSPP_eErrorType eVUIParseError = BSPP_ERROR_NONE;

    psVUIInfo->aspect_ratio_info_present_flag = SWSR_ReadBits( hSwSrContext, 1 );
    if( psVUIInfo->aspect_ratio_info_present_flag )
    {
        psVUIInfo->aspect_ratio_idc = SWSR_ReadBits( hSwSrContext, 8 );
        /* Extended SAR */
        if( psVUIInfo->aspect_ratio_idc == 255 )
        {
            psVUIInfo->sar_width = SWSR_ReadBits( hSwSrContext, 16 );
            psVUIInfo->sar_height = SWSR_ReadBits( hSwSrContext, 16 );
        }
        else if( psVUIInfo->aspect_ratio_idc < 17 )
        {
            psVUIInfo->sar_width = aPixelAspect[psVUIInfo->aspect_ratio_idc][0] ;
            psVUIInfo->sar_height = aPixelAspect[psVUIInfo->aspect_ratio_idc][1] ;
        }
        else
        {
            // we can consider this error as a aux data error
            eVUIParseError |= BSPP_ERROR_INVALID_VALUE;
        }
    }

    psVUIInfo->overscan_info_present_flag = SWSR_ReadBits( hSwSrContext, 1 );
    if( psVUIInfo->overscan_info_present_flag )
    {
        psVUIInfo->overscan_appropriate_flag = SWSR_ReadBits( hSwSrContext, 1 );
    }

    psVUIInfo->video_signal_type_present_flag = SWSR_ReadBits( hSwSrContext, 1 );
    if( psVUIInfo->video_signal_type_present_flag )
    {
        psVUIInfo->video_format = SWSR_ReadBits( hSwSrContext, 3 );
        psVUIInfo->video_full_range_flag = SWSR_ReadBits( hSwSrContext, 1 );
        psVUIInfo->colour_description_present_flag = SWSR_ReadBits( hSwSrContext, 1 );
        if( psVUIInfo->colour_description_present_flag )
        {
            psVUIInfo->colour_primaries = SWSR_ReadBits(hSwSrContext, 8);
            psVUIInfo->transfer_characteristics = SWSR_ReadBits(hSwSrContext, 8);
            psVUIInfo->matrix_coefficients = SWSR_ReadBits(hSwSrContext, 8);
        }
    }

    psVUIInfo->chroma_location_info_present_flag = SWSR_ReadBits(hSwSrContext, 1);
    if( psVUIInfo->chroma_location_info_present_flag )
    {
        psVUIInfo->chroma_sample_loc_type_top_field = SWSR_ReadUnsignedExpGoulomb( hSwSrContext );
        psVUIInfo->chroma_sample_loc_type_bottom_field = SWSR_ReadUnsignedExpGoulomb( hSwSrContext );
    }

    psVUIInfo->timing_info_present_flag = SWSR_ReadBits( hSwSrContext, 1 );
    if( psVUIInfo->timing_info_present_flag )
    {
        psVUIInfo->num_units_in_tick = SWSR_ReadBits( hSwSrContext, 16 );
        psVUIInfo->num_units_in_tick <<= 16;     /* SR can only do up to 31 bit reads */
        psVUIInfo->num_units_in_tick |= SWSR_ReadBits( hSwSrContext, 16 );
        psVUIInfo->time_scale = SWSR_ReadBits( hSwSrContext, 16 );
        psVUIInfo->time_scale <<= 16;     /* SR can only do up to 31 bit reads */
        psVUIInfo->time_scale |= SWSR_ReadBits( hSwSrContext, 16 );
        if( !psVUIInfo->num_units_in_tick || !psVUIInfo->time_scale )
        {
            eVUIParseError  |=  BSPP_ERROR_INVALID_VALUE;
        }
        psVUIInfo->fixed_frame_rate_flag = SWSR_ReadBits( hSwSrContext, 1 );
    }

    /* no default values */
    psVUIInfo->nal_hrd_parameters_present_flag = SWSR_ReadBits( hSwSrContext, 1 );
    if( psVUIInfo->nal_hrd_parameters_present_flag )
    {
        eVUIParseError |= bspp_H264HRDParamParser( hSwSrContext, &psVUIInfo->sNALHRDParameters );
    }
    else
    {
        bspp_H264GetDefaultHRDParam( &psVUIInfo->sNALHRDParameters );
    }

    psVUIInfo->vcl_hrd_parameters_present_flag = SWSR_ReadBits( hSwSrContext, 1 );
    if( psVUIInfo->vcl_hrd_parameters_present_flag )
    {
        eVUIParseError |= bspp_H264HRDParamParser( hSwSrContext, &psVUIInfo->sVCLHRDParameters );
    }
    else
    {
        bspp_H264GetDefaultHRDParam( &psVUIInfo->sVCLHRDParameters );
    }

    if( psVUIInfo->nal_hrd_parameters_present_flag || psVUIInfo->vcl_hrd_parameters_present_flag )
    {
        psVUIInfo->low_delay_hrd_flag = SWSR_ReadBits( hSwSrContext, 1);
    }

    psVUIInfo->pic_struct_present_flag    = SWSR_ReadBits( hSwSrContext, 1 );
    psVUIInfo->bitstream_restriction_flag = SWSR_ReadBits( hSwSrContext, 1 );
    if( psVUIInfo->bitstream_restriction_flag )
    {
        psVUIInfo->motion_vectors_over_pic_boundaries_flag = SWSR_ReadBits(hSwSrContext, 1);
        psVUIInfo->max_bytes_per_pic_denom = SWSR_ReadUnsignedExpGoulomb( hSwSrContext );
        psVUIInfo->max_bits_per_mb_denom = SWSR_ReadUnsignedExpGoulomb( hSwSrContext );
        psVUIInfo->log2_max_mv_length_horizontal = SWSR_ReadUnsignedExpGoulomb( hSwSrContext );
        psVUIInfo->log2_max_mv_length_vertical = SWSR_ReadUnsignedExpGoulomb( hSwSrContext );
        psVUIInfo->num_reorder_frames = SWSR_ReadUnsignedExpGoulomb( hSwSrContext );
        psVUIInfo->max_dec_frame_buffering = SWSR_ReadUnsignedExpGoulomb( hSwSrContext );
    }
    if( psVUIInfo->num_reorder_frames > 32 )
    {
        eVUIParseError |= BSPP_ERROR_UNSUPPORTED;
    }

    return eVUIParseError;
}

/*
******************************************************************************

 @Function              H264_UnitParser_MVCSPSExtParse

 @Description           Parse the SPS extension for MVC NAL unit

 @Input		            psParserContext   : Parser context.

******************************************************************************/
BSPP_eErrorType bspp_H264MVCSPSExtParse(IMG_HANDLE hSwSrContext, BSPP_sH264SPSInfo_MVCExt * psSPSMVCExtInfo)
{
    IMG_UINT32 i, j, temp, temp1;
    BSPP_eErrorType eMVCSPSParseError = BSPP_ERROR_NONE;

    IMG_ASSERT(psSPSMVCExtInfo != IMG_NULL);

    // skip one bit
    SWSR_ReadBits( hSwSrContext, 1 );

    psSPSMVCExtInfo->ui16NumViewsMinus1 =   SWSR_ReadUnsignedExpGoulomb( hSwSrContext );
    if(psSPSMVCExtInfo->ui16NumViewsMinus1 > 1023)
    {
        eMVCSPSParseError |= BSPP_ERROR_INVALID_VALUE;
    }

    if ( psSPSMVCExtInfo->ui16NumViewsMinus1 >=  VDEC_H264_MVC_MAX_VIEWS )
    {
        REPORT(REPORT_MODULE_BSPP,REPORT_ERR ,"Unsupported number of views: %d (max: %d)", psSPSMVCExtInfo->ui16NumViewsMinus1+1, VDEC_H264_MVC_MAX_VIEWS);
        eMVCSPSParseError |= BSPP_ERROR_UNSUPPORTED;
    }

    if(IMG_NULL == psSPSMVCExtInfo->pui16AnchorRefIndicies1X)
    {
        psSPSMVCExtInfo->pui16AnchorRefIndicies1X = (IMG_UINT16 *)IMG_MALLOC(sizeof(IMG_UINT16 [2][VDEC_H264_MVC_MAX_VIEWS][VDEC_H264_MVC_MAX_REFS]));
        IMG_ASSERT(IMG_NULL != psSPSMVCExtInfo->pui16AnchorRefIndicies1X);
    }

    if(IMG_NULL == psSPSMVCExtInfo->pui16NonAnchorRefIndicies1X)
    {
        psSPSMVCExtInfo->pui16NonAnchorRefIndicies1X = (IMG_UINT16 *)IMG_MALLOC(sizeof(IMG_UINT16 [2][VDEC_H264_MVC_MAX_VIEWS][VDEC_H264_MVC_MAX_REFS]));
        IMG_ASSERT(IMG_NULL != psSPSMVCExtInfo->pui16NonAnchorRefIndicies1X);
    }

    for( i = 0; i <= psSPSMVCExtInfo->ui16NumViewsMinus1; i++ )
    {
        temp = SWSR_ReadUnsignedExpGoulomb( hSwSrContext );
        if (i < VDEC_H264_MVC_MAX_VIEWS)
        {
            psSPSMVCExtInfo->aui16ViewId[i] = temp;
        }
    }

    {
        IMG_UINT16 (*AnchorRefIndicies1X)[2][VDEC_H264_MVC_MAX_VIEWS][VDEC_H264_MVC_MAX_REFS] = (IMG_UINT16 (*)[2][VDEC_H264_MVC_MAX_VIEWS][VDEC_H264_MVC_MAX_REFS])psSPSMVCExtInfo->pui16AnchorRefIndicies1X;
        IMG_UINT16 (*NonAnchorRefIndicies1X)[2][VDEC_H264_MVC_MAX_VIEWS][VDEC_H264_MVC_MAX_REFS] = (IMG_UINT16 (*)[2][VDEC_H264_MVC_MAX_VIEWS][VDEC_H264_MVC_MAX_REFS])psSPSMVCExtInfo->pui16NonAnchorRefIndicies1X;
        for( i = 1; i <= psSPSMVCExtInfo->ui16NumViewsMinus1; i++ )
        {
            temp = SWSR_ReadUnsignedExpGoulomb( hSwSrContext );
            if(temp >= VDEC_H264_MVC_MAX_REFS)
            {
                REPORT(REPORT_MODULE_BSPP,REPORT_ERR,"more than 16 num_anchor_refs_l0 references are not permitted" );
                eMVCSPSParseError |= BSPP_ERROR_UNSUPPORTED;
            }

            if (i < VDEC_H264_MVC_MAX_VIEWS)
            {
                psSPSMVCExtInfo->aui16NumAnchorRefs1X[VDEC_H264_MVC_REF_LIST_ANCHOR_L0][i] = (IMG_UINT16)temp;
            }

            for( j = 0; j < temp; j++ )
            {
                temp1 = SWSR_ReadUnsignedExpGoulomb( hSwSrContext );
                if (i < VDEC_H264_MVC_MAX_VIEWS && j < VDEC_H264_MVC_MAX_REFS)
                {
                    (*AnchorRefIndicies1X)[VDEC_H264_MVC_REF_LIST_ANCHOR_L0][i][j] = (IMG_UINT16)temp1;
                }
            }

            temp = SWSR_ReadUnsignedExpGoulomb( hSwSrContext );
            if(temp >= VDEC_H264_MVC_MAX_REFS)
            {
                REPORT(REPORT_MODULE_BSPP,REPORT_ERR,"more than 16 num_anchor_refs_l1 references are not permitted" );
                eMVCSPSParseError |= BSPP_ERROR_UNSUPPORTED;
            }

            if (i < VDEC_H264_MVC_MAX_VIEWS)
            {
                psSPSMVCExtInfo->aui16NumAnchorRefs1X[VDEC_H264_MVC_REF_LIST_ANCHOR_L1][i] = temp;
            }

            for( j = 0; j < temp; j++ )
            {
                temp1 = SWSR_ReadUnsignedExpGoulomb( hSwSrContext );
                if (i < VDEC_H264_MVC_MAX_VIEWS && j < VDEC_H264_MVC_MAX_REFS)
                {
                    (*AnchorRefIndicies1X)[VDEC_H264_MVC_REF_LIST_ANCHOR_L1][i][j] = (IMG_UINT16)temp1;
                }
            }
        }

        for( i = 1; i <= psSPSMVCExtInfo->ui16NumViewsMinus1; i++ )
        {
            temp = SWSR_ReadUnsignedExpGoulomb( hSwSrContext );

            if(temp >= VDEC_H264_MVC_MAX_REFS)
            {
                REPORT(REPORT_MODULE_BSPP,REPORT_ERR,"more than 16 num_non_anchor_refs_l0 references are not permitted" );
                eMVCSPSParseError |= BSPP_ERROR_UNSUPPORTED;
            }
            if (i < VDEC_H264_MVC_MAX_VIEWS)
            {
            	psSPSMVCExtInfo->aui16NumNonAnchorRefs1X[VDEC_H264_MVC_REF_LIST_NON_ANCHOR_L0][i] = (IMG_UINT16)temp;
            }

            for( j = 0; j < temp; j++ )
            {
                temp1 = SWSR_ReadUnsignedExpGoulomb( hSwSrContext );
                if (i < VDEC_H264_MVC_MAX_VIEWS && j < VDEC_H264_MVC_MAX_REFS)
                {
                    (*NonAnchorRefIndicies1X)[VDEC_H264_MVC_REF_LIST_NON_ANCHOR_L0][i][j] = (IMG_UINT16)temp1;
                }
            }

            temp = SWSR_ReadUnsignedExpGoulomb( hSwSrContext );
            if(temp >= VDEC_H264_MVC_MAX_REFS)
            {
                REPORT(REPORT_MODULE_BSPP,REPORT_ERR,"more than 16 num_non_anchor_refs_l1 references are not permitted" );
                eMVCSPSParseError |= BSPP_ERROR_UNSUPPORTED;
            }
            if (i < VDEC_H264_MVC_MAX_VIEWS)
            {
                psSPSMVCExtInfo->aui16NumNonAnchorRefs1X[VDEC_H264_MVC_REF_LIST_NON_ANCHOR_L1][i] = (IMG_UINT16)temp;
            }

            for( j = 0; j < temp; j++ )
            {
                temp1 = SWSR_ReadUnsignedExpGoulomb( hSwSrContext );
                if (i < VDEC_H264_MVC_MAX_VIEWS && j < VDEC_H264_MVC_MAX_REFS)
                {
                    (*NonAnchorRefIndicies1X)[VDEC_H264_MVC_REF_LIST_NON_ANCHOR_L1][i][j] = (IMG_UINT16)temp1;
                }
            }
        }
    }

    return eMVCSPSParseError;
}

/******************************************************************************

 @Function              bspp_H264SPSParser

 @Description           Parse the SPS NAL unit

******************************************************************************/
static BSPP_eErrorType bspp_H264SPSParser(
    IMG_HANDLE              hSwSrContext,
    BSPP_sH264SequHdrInfo * psH264SequHdrInfo
)
{
    IMG_UINT32  i;
    IMG_UINT8                   ui8ScalingListNum;
    BSPP_sH264SPSInfo        * psSPSInfo;
    BSPP_sH264VUIInfo        * psVUIInfo;
    BSPP_sH264SPSInfo_MVCExt * psSPSMVCExtInfo;
    BSPP_eErrorType eSPSParseError = BSPP_ERROR_NONE;
    BSPP_eErrorType eVUIParseError = BSPP_ERROR_NONE;
    BSPP_eErrorType eMVCExtParseError = BSPP_ERROR_NONE;

    psSPSInfo       = &psH264SequHdrInfo->sSPSInfo;
    psVUIInfo       = &psH264SequHdrInfo->sVUIInfo;
    psSPSMVCExtInfo = &psH264SequHdrInfo->sSPSMVCExtInfo;

    // Set always the default VUI/MVCExt, their values may be used even if VUI/MVCExt not present
    bspp_H264SetDefaultVUI(psVUIInfo);
    IMG_MEMSET(psSPSMVCExtInfo, 0, sizeof(BSPP_sH264SPSInfo_MVCExt));

    DEBUG_REPORT(REPORT_MODULE_BSPP,"Parsing Sequence Parameter Set");
    {
        psSPSInfo->profile_idc = SWSR_ReadBits( hSwSrContext, 8 );
        if( ( psSPSInfo->profile_idc != H264_Profile_BASELINE   ) &&
            ( psSPSInfo->profile_idc != H264_Profile_MAIN       ) &&
            ( psSPSInfo->profile_idc != H264_Profile_SCALABLE   ) &&
            ( psSPSInfo->profile_idc != H264_Profile_EXTENDED   ) &&
            ( psSPSInfo->profile_idc != H264_Profile_HIGH       ) &&
            ( psSPSInfo->profile_idc != H264_Profile_HIGH10     ) &&
            ( psSPSInfo->profile_idc != H264_Profile_MVC_HIGH   ) &&
            ( psSPSInfo->profile_idc != H264_Profile_HIGH422    ) &&
            ( psSPSInfo->profile_idc != H264_Profile_CAVLC444   ) &&
            ( psSPSInfo->profile_idc != H264_Profile_MVC_STEREO )  &&
            ( psSPSInfo->profile_idc != H264_Profile_HIGH444    )
            )
        {
            REPORT(REPORT_MODULE_BSPP,REPORT_WARNING,"Invalid Profile ID [%d],Parsed by BSPP",psSPSInfo->profile_idc);
            return BSPP_ERROR_UNSUPPORTED;
        }
        psSPSInfo->constraint_set_flags = SWSR_ReadBits( hSwSrContext, 8 );
        psSPSInfo->level_idc            = SWSR_ReadBits( hSwSrContext, 8 );
        // sequence paramter set id
        psSPSInfo->seq_parameter_set_id = SWSR_ReadUnsignedExpGoulomb( hSwSrContext );
        if ( MAX_SPS_COUNT < psSPSInfo->seq_parameter_set_id  )
        {
            REPORT(REPORT_MODULE_BSPP,REPORT_WARNING,"SPS ID [%d] goes beyind the limit",psSPSInfo->seq_parameter_set_id);
            return  BSPP_ERROR_UNSUPPORTED;
        }

        /* High profile settings */
        if( (psSPSInfo->profile_idc == H264_Profile_HIGH)      ||
            (psSPSInfo->profile_idc == H264_Profile_HIGH10)    ||
            (psSPSInfo->profile_idc == H264_Profile_HIGH422)   ||
            (psSPSInfo->profile_idc == H264_Profile_HIGH444)   ||
            (psSPSInfo->profile_idc == H264_Profile_CAVLC444)  ||
            (psSPSInfo->profile_idc == H264_Profile_MVC_HIGH)  ||
            (psSPSInfo->profile_idc == H264_Profile_MVC_STEREO)
            )
        {
            DEBUG_REPORT(REPORT_MODULE_BSPP,"This is High Profile Bitstream");
            psSPSInfo->chroma_format_idc = SWSR_ReadUnsignedExpGoulomb( hSwSrContext );
            if( psSPSInfo->chroma_format_idc > 3 )
            {
                REPORT(REPORT_MODULE_BSPP,REPORT_INFO,"chroma_format_idc[%d] is not within the range",psSPSInfo->chroma_format_idc);
                eSPSParseError |= BSPP_ERROR_INVALID_VALUE;
            }
            if( psSPSInfo->chroma_format_idc == 3 )
            {
                psSPSInfo->separate_colour_plane_flag = SWSR_ReadBits( hSwSrContext, 1 );
            }
            else
            {
                psSPSInfo->separate_colour_plane_flag = 0;
            }

            psSPSInfo->bit_depth_luma_minus8 = SWSR_ReadUnsignedExpGoulomb( hSwSrContext );
            if( psSPSInfo->bit_depth_luma_minus8 > 6 )
            {
                eSPSParseError |= BSPP_ERROR_INVALID_VALUE;
            }
            psSPSInfo->bit_depth_chroma_minus8 = SWSR_ReadUnsignedExpGoulomb( hSwSrContext );
            if( psSPSInfo->bit_depth_chroma_minus8 > 6 )
            {
                eSPSParseError |= BSPP_ERROR_INVALID_VALUE;
            }
            psSPSInfo->qpprime_y_zero_transform_bypass_flag = SWSR_ReadBits( hSwSrContext, 1 );
            psSPSInfo->seq_scaling_matrix_present_flag = SWSR_ReadBits( hSwSrContext, 1 );
            if ( psSPSInfo->seq_scaling_matrix_present_flag )
            {
                REPORT(REPORT_MODULE_BSPP,REPORT_INFO ,"seq_scaling_matrix_present_flag is available");
                ui8ScalingListNum = (psSPSInfo->chroma_format_idc!=3) ? 8 : 12;

                if(IMG_NULL == psSPSInfo->pui8ScalingList4x4_seq)
                {
                    DEBUG_REPORT(REPORT_MODULE_BSPP, "Allocating ScalingList4x4_seq size %zu bytes", H264FW_NUM_4X4_LISTS*H264FW_4X4_SIZE*sizeof(IMG_UINT8));
                    psSPSInfo->pui8ScalingList4x4_seq = (IMG_UINT8 *)IMG_MALLOC(sizeof(IMG_UINT8 [H264FW_NUM_4X4_LISTS][H264FW_4X4_SIZE]));
                    if(psSPSInfo->pui8ScalingList4x4_seq == IMG_NULL)
                    {
                        eSPSParseError |= BSPP_ERROR_OUT_OF_MEMORY;
                    }
                    else
                    {
                        IMG_ASSERT(IMG_NULL != psSPSInfo->pui8ScalingList4x4_seq);
                        IMG_MEMSET(psSPSInfo->pui8ScalingList4x4_seq, 0x00, sizeof(IMG_UINT8 [H264FW_NUM_4X4_LISTS][H264FW_4X4_SIZE]));
                    }
                }
                if(IMG_NULL == psSPSInfo->pui8ScalingList8x8_seq)
                {
                    DEBUG_REPORT(REPORT_MODULE_BSPP, "Allocating ScalingList8x8_seq size %zu bytes", H264FW_NUM_8X8_LISTS*H264FW_8X8_SIZE*sizeof(IMG_UINT8));
                    psSPSInfo->pui8ScalingList8x8_seq = (IMG_UINT8 *)IMG_MALLOC(sizeof(IMG_UINT8 [H264FW_NUM_8X8_LISTS][H264FW_8X8_SIZE]));
                    if(psSPSInfo->pui8ScalingList8x8_seq == IMG_NULL)
                    {
                        eSPSParseError |= BSPP_ERROR_OUT_OF_MEMORY;
                    }
                    else
                    {
                        IMG_ASSERT(IMG_NULL != psSPSInfo->pui8ScalingList8x8_seq);
                        IMG_MEMSET(psSPSInfo->pui8ScalingList8x8_seq, 0x00, sizeof(IMG_UINT8 [H264FW_NUM_8X8_LISTS][H264FW_8X8_SIZE]));
                    }
                }
                {
                    IMG_UINT8 (*ScalingList4x4_seq)[H264FW_NUM_4X4_LISTS][H264FW_4X4_SIZE] = (IMG_UINT8 (*)[H264FW_NUM_4X4_LISTS][H264FW_4X4_SIZE])psSPSInfo->pui8ScalingList4x4_seq;
                    IMG_UINT8 (*ScalingList8x8_seq)[H264FW_NUM_8X8_LISTS][H264FW_8X8_SIZE] = (IMG_UINT8 (*)[H264FW_NUM_8X8_LISTS][H264FW_8X8_SIZE])psSPSInfo->pui8ScalingList8x8_seq;

                    for ( i = 0; i < ui8ScalingListNum; i++ )
                    {
                        psSPSInfo->seq_scaling_list_present_flag[i] = SWSR_ReadBits( hSwSrContext, 1 );
                        if ( psSPSInfo->seq_scaling_list_present_flag[i] )
                        {
                            if ( i < 6 )
                            {
                                eSPSParseError |= bspp_H264ScalingListParser(
                                    hSwSrContext,
                                    (*ScalingList4x4_seq)[i],
                                    16,
                                    &psSPSInfo->UseDefaultScalingMatrixFlag_seq[i]
                                );
                            }
                            else
                            {
                                eSPSParseError  |=  bspp_H264ScalingListParser(
                                    hSwSrContext,
                                    (*ScalingList8x8_seq)[i-6],
                                    64,
                                    &psSPSInfo->UseDefaultScalingMatrixFlag_seq[i]
                                );
                            }
                        }
                    }
                }
            }
        }
        else
        {
            /* default values in here */
            psSPSInfo->chroma_format_idc = 1;
            psSPSInfo->bit_depth_luma_minus8 = 0;
            psSPSInfo->bit_depth_chroma_minus8 = 0;
            psSPSInfo->qpprime_y_zero_transform_bypass_flag = 0;
            psSPSInfo->seq_scaling_matrix_present_flag = 0;
        }

        psSPSInfo->log2_max_frame_num_minus4 = SWSR_ReadUnsignedExpGoulomb( hSwSrContext );
        if ( psSPSInfo->log2_max_frame_num_minus4 > 12 )
        {
            REPORT(REPORT_MODULE_BSPP,REPORT_WARNING,"log2_max_frame_num_minus4[%d] is not within range  [0 - 12]",psSPSInfo->log2_max_frame_num_minus4 );
            eSPSParseError |= BSPP_ERROR_INVALID_VALUE;
        }

        psSPSInfo->pic_order_cnt_type = SWSR_ReadUnsignedExpGoulomb( hSwSrContext );
        if ( psSPSInfo->pic_order_cnt_type > 2 )
        {
            REPORT(REPORT_MODULE_BSPP,REPORT_WARNING ,"pic_order_cnt_type[%d] is not within range  [0 - 2]",psSPSInfo->pic_order_cnt_type );
            eSPSParseError |= BSPP_ERROR_INVALID_VALUE;
        }

        if ( psSPSInfo->pic_order_cnt_type == 0 )
        {
            psSPSInfo->log2_max_pic_order_cnt_lsb_minus4 = SWSR_ReadUnsignedExpGoulomb( hSwSrContext );
            if ( psSPSInfo->log2_max_pic_order_cnt_lsb_minus4 > 12 )
            {
                REPORT(REPORT_MODULE_BSPP,REPORT_WARNING ,"log2_max_pic_order_cnt_lsb_minus4[%d] is not within range  [0 - 12]",psSPSInfo->log2_max_pic_order_cnt_lsb_minus4 );
                psSPSInfo->log2_max_pic_order_cnt_lsb_minus4 = 12;
                eSPSParseError |= BSPP_ERROR_CORRECTION_VALIDVALUE;
            }
        }
        else if ( psSPSInfo->pic_order_cnt_type == 1 )
        {
            psSPSInfo->delta_pic_order_always_zero_flag = SWSR_ReadBits( hSwSrContext, 1 );
            psSPSInfo->offset_for_non_ref_pic = SWSR_ReadSignedExpGoulomb( hSwSrContext );
            psSPSInfo->offset_for_top_to_bottom_field = SWSR_ReadSignedExpGoulomb( hSwSrContext );
            psSPSInfo->num_ref_frames_in_pic_order_cnt_cycle = SWSR_ReadUnsignedExpGoulomb( hSwSrContext );
            if( psSPSInfo->num_ref_frames_in_pic_order_cnt_cycle > 255 )
            {
                REPORT(REPORT_MODULE_BSPP,REPORT_WARNING,"num_ref_frames_in_pic_order_cnt_cycle[%d] is not within range  [0 - 256]",psSPSInfo->num_ref_frames_in_pic_order_cnt_cycle);
                eSPSParseError  |=  BSPP_ERROR_INVALID_VALUE;
            }

            if(IMG_NULL == psSPSInfo->pui32offset_for_ref_frame)
            {
                psSPSInfo->pui32offset_for_ref_frame = (IMG_UINT32 *)IMG_MALLOC(H264FW_MAX_CYCLE_REF_FRAMES * sizeof(IMG_UINT32));
                if(psSPSInfo->pui32offset_for_ref_frame == IMG_NULL)
                {
                    REPORT(REPORT_MODULE_BSPP,REPORT_ERR,"out of memory");
                    eSPSParseError |= BSPP_ERROR_OUT_OF_MEMORY;
                }
            }

            if (psSPSInfo->pui32offset_for_ref_frame)
            {
                IMG_ASSERT(H264FW_MAX_CYCLE_REF_FRAMES >= psSPSInfo->num_ref_frames_in_pic_order_cnt_cycle);
                IMG_MEMSET(psSPSInfo->pui32offset_for_ref_frame, 0x00, (H264FW_MAX_CYCLE_REF_FRAMES * sizeof(IMG_UINT32)));
                for ( i = 0; i < psSPSInfo->num_ref_frames_in_pic_order_cnt_cycle; i++ )
                {
                    //check the max value and if it crosses then exit from the loop
                    psSPSInfo->pui32offset_for_ref_frame[i] = SWSR_ReadSignedExpGoulomb( hSwSrContext );
                }
            }
        }
        else if ( psSPSInfo->pic_order_cnt_type != 2 )
        {
            eSPSParseError |= BSPP_ERROR_INVALID_VALUE;
        }
        psSPSInfo->max_num_ref_frames = SWSR_ReadUnsignedExpGoulomb( hSwSrContext );

        if ( psSPSInfo->max_num_ref_frames > 16 )
        {
            REPORT(REPORT_MODULE_BSPP,REPORT_WARNING,
                   "num_ref_frames[%d] is not within range [0 - 16]",
                   psSPSInfo->max_num_ref_frames);
            eSPSParseError |= BSPP_ERROR_INVALID_VALUE;
        }
        psSPSInfo->gaps_in_frame_num_value_allowed_flag = SWSR_ReadBits( hSwSrContext, 1 );
        psSPSInfo->pic_width_in_mbs_minus1 = SWSR_ReadUnsignedExpGoulomb( hSwSrContext);
        if(psSPSInfo->pic_width_in_mbs_minus1 >= MAX_WIDTH_IN_MBS)
        {
            REPORT(REPORT_MODULE_BSPP,REPORT_WARNING,
                   "pic_width_in_mbs_minus1[%d] is not within range",
                   psSPSInfo->pic_width_in_mbs_minus1);
            eSPSParseError |= BSPP_ERROR_INVALID_VALUE;
        }
        psSPSInfo->pic_height_in_map_units_minus1 = SWSR_ReadUnsignedExpGoulomb( hSwSrContext );
        if(psSPSInfo->pic_height_in_map_units_minus1 >= MAX_HEIGHT_IN_MBS)
        {
            REPORT(REPORT_MODULE_BSPP,REPORT_WARNING,
                   "pic_height_in_map_units_minus1[%d] is not within range",
                   psSPSInfo->pic_height_in_map_units_minus1);
            eSPSParseError |= BSPP_ERROR_INVALID_VALUE;
        }

        psSPSInfo->frame_mbs_only_flag = SWSR_ReadBits( hSwSrContext, 1 );
        if( !psSPSInfo->frame_mbs_only_flag )
        {
            psSPSInfo->mb_adaptive_frame_field_flag = SWSR_ReadBits( hSwSrContext, 1 );
        }
        else
        {
            psSPSInfo->mb_adaptive_frame_field_flag = 0;
        }
        psSPSInfo->direct_8x8_inference_flag = SWSR_ReadBits( hSwSrContext, 1 );

        psSPSInfo->frame_cropping_flag = SWSR_ReadBits( hSwSrContext, 1 );
        if( psSPSInfo->frame_cropping_flag )
        {
            DEBUG_REPORT(REPORT_MODULE_BSPP,"frame_cropping_flag is available");
            psSPSInfo->frame_crop_left_offset   = SWSR_ReadUnsignedExpGoulomb( hSwSrContext );
            psSPSInfo->frame_crop_right_offset  = SWSR_ReadUnsignedExpGoulomb( hSwSrContext );
            psSPSInfo->frame_crop_top_offset    = SWSR_ReadUnsignedExpGoulomb( hSwSrContext );
            psSPSInfo->frame_crop_bottom_offset = SWSR_ReadUnsignedExpGoulomb( hSwSrContext );
        }
        else
        {
            psSPSInfo->frame_crop_left_offset   = 0;
            psSPSInfo->frame_crop_right_offset  = 0;
            psSPSInfo->frame_crop_top_offset    = 0;
            psSPSInfo->frame_crop_bottom_offset = 0;
        }

        psSPSInfo->vui_parameters_present_flag = SWSR_ReadBits( hSwSrContext, 1 );
        // initialise matrix_coefficients to 2 (unspecified)
        psVUIInfo->matrix_coefficients = 2;

        if(psSPSInfo->vui_parameters_present_flag)
        {
            DEBUG_REPORT(REPORT_MODULE_BSPP,"vui_parameters_present_flag is available");
            // save the sps parse error in temp variable
            eVUIParseError = bspp_H264VUIParser( hSwSrContext, psVUIInfo );
            if( (eVUIParseError != BSPP_ERROR_NONE) )
                eSPSParseError  |= BSPP_ERROR_AUXDATA;
        }

        if( (H264_Profile_MVC_HIGH == psSPSInfo->profile_idc) || (H264_Profile_MVC_STEREO == psSPSInfo->profile_idc) )
        {
            DEBUG_REPORT(REPORT_MODULE_BSPP,"MVC SPS extension is parsed " );
            eMVCExtParseError = bspp_H264MVCSPSExtParse( hSwSrContext, psSPSMVCExtInfo);
            eSPSParseError |= eMVCExtParseError;
        }

        if( SWSR_EXCEPT_NO_EXCEPTION != SWSR_CheckException(hSwSrContext) )
             eSPSParseError |= BSPP_ERROR_INSUFFICIENT_DATA;
    }

    return eSPSParseError;
}


/******************************************************************************

 @Function              bspp_H264PPSParser

 @Description           Parse the PPS NAL unit

******************************************************************************/
static BSPP_eErrorType bspp_H264PPSParser(
    IMG_HANDLE          hSwSrContext,
    IMG_HANDLE          hStrRes,
    BSPP_sH264PPSInfo * psH264PPSInfo
)
{
    IMG_INT32        i, i32Group, chroma_format_idc;
    IMG_UINT32       ui32NumberBitsPerSliceGroupId;
    IMG_UINT8        n_ScalingList;
    IMG_BOOL         bMoreRbspData;
    IMG_UINT32       ui32Result;
    BSPP_eErrorType  ePPSParseError = BSPP_ERROR_NONE;

    IMG_ASSERT(hSwSrContext != IMG_NULL);
    {
        psH264PPSInfo->pic_parameter_set_id = SWSR_ReadUnsignedExpGoulomb( hSwSrContext );
        if ( MAX_PPS_COUNT <= psH264PPSInfo->pic_parameter_set_id )
        {
            REPORT(REPORT_MODULE_BSPP,REPORT_WARNING,"Picture Parameter Set(PPS) ID is not within the range");
            psH264PPSInfo->pic_parameter_set_id = BSPP_INVALID;
            return BSPP_ERROR_UNSUPPORTED;
        }
        psH264PPSInfo->seq_parameter_set_id = SWSR_ReadUnsignedExpGoulomb( hSwSrContext );
        if ( MAX_SPS_COUNT <= psH264PPSInfo->seq_parameter_set_id )
        {
            REPORT(REPORT_MODULE_BSPP,REPORT_WARNING,"Sequence Paramter Set(SPS) ID is not within the range");
            psH264PPSInfo->seq_parameter_set_id = BSPP_INVALID;
            return BSPP_ERROR_UNSUPPORTED;
        }

        {
            // Get the chroma_format_idc from sps. Because of MVC sharing sps and subset sps ids (H.7.4.1.2.1)
            // At this point is not clear if this pps refers to an sps or a subset sps. It should be fine however
            // for the case of chroma_format_idc to try and locate a subset sps if there isn't a normal one.
            BSPP_sH264SequHdrInfo * psH264SequHdrInfo;
            BSPP_sSequenceHdrInfo * psSequHdrInfo;

            psSequHdrInfo = BSPP_GetSequHdr(hStrRes, psH264PPSInfo->seq_parameter_set_id);
            if (psSequHdrInfo == IMG_NULL)
            {
                psSequHdrInfo = BSPP_GetSequHdr(hStrRes, GET_SUBSET_ID(H264_NALTYPE_SUBSET_SPS, psH264PPSInfo->seq_parameter_set_id));
                if(psSequHdrInfo == IMG_NULL)
                {
                    return BSPP_ERROR_NO_SEQUENCE_HDR;
                }
            }
            psH264SequHdrInfo = (BSPP_sH264SequHdrInfo *)psSequHdrInfo->hSecureSequenceInfo;

            chroma_format_idc = psH264SequHdrInfo->sSPSInfo.chroma_format_idc;
        }

        psH264PPSInfo->entropy_coding_mode_flag = SWSR_ReadBits( hSwSrContext, 1 );
        psH264PPSInfo->pic_order_present_flag = SWSR_ReadBits( hSwSrContext, 1 );
        psH264PPSInfo->num_slice_groups_minus1 = SWSR_ReadUnsignedExpGoulomb( hSwSrContext );
        if ((psH264PPSInfo->num_slice_groups_minus1+1) > MAX_SLICEGROUP_COUNT)
        {
            psH264PPSInfo->num_slice_groups_minus1 = MAX_SLICEGROUP_COUNT - 1;
            ePPSParseError |=  BSPP_ERROR_UNRECOVERABLE;
        }

        if ( psH264PPSInfo->num_slice_groups_minus1 > 0 )
        {
            psH264PPSInfo->slice_group_map_type = SWSR_ReadUnsignedExpGoulomb( hSwSrContext );
            DEBUG_REPORT(REPORT_MODULE_BSPP,"slice_group_map_type is : %d, Parsed by BSPP",psH264PPSInfo->slice_group_map_type);
            if ( psH264PPSInfo->slice_group_map_type > 6 )
            {
                REPORT(REPORT_MODULE_BSPP,REPORT_WARNING,"slice_group_map_type [%d] is not within the range [ 0- 6 ]",psH264PPSInfo->slice_group_map_type);
                ePPSParseError |= BSPP_ERROR_UNRECOVERABLE;
            }

            if ( psH264PPSInfo->slice_group_map_type == 0 )
            {
                for ( i32Group = 0; i32Group <= psH264PPSInfo->num_slice_groups_minus1; i32Group++ )
                {
                    psH264PPSInfo->run_length_minus1[i32Group] = SWSR_ReadUnsignedExpGoulomb( hSwSrContext );
                }
            }
            else if ( psH264PPSInfo->slice_group_map_type == 2 )
            {
                for ( i32Group = 0; i32Group < psH264PPSInfo->num_slice_groups_minus1; i32Group++ )
                {
                    psH264PPSInfo->top_left[i32Group]     = SWSR_ReadUnsignedExpGoulomb( hSwSrContext );
                    psH264PPSInfo->bottom_right[i32Group] = SWSR_ReadUnsignedExpGoulomb( hSwSrContext );
                }
            }
            else if ( psH264PPSInfo->slice_group_map_type == 3 ||
                      psH264PPSInfo->slice_group_map_type == 4 ||
                      psH264PPSInfo->slice_group_map_type == 5 )
            {
                psH264PPSInfo->slice_group_change_direction_flag = SWSR_ReadBits( hSwSrContext, 1 );
                psH264PPSInfo->slice_group_change_rate_minus1 = SWSR_ReadUnsignedExpGoulomb( hSwSrContext );
            }
            else if ( psH264PPSInfo->slice_group_map_type == 6 )
            {
                psH264PPSInfo->pic_size_in_map_unit = SWSR_ReadUnsignedExpGoulomb( hSwSrContext );
                if ( psH264PPSInfo->pic_size_in_map_unit >= H264_MAX_SGM_SIZE )
                {
                    REPORT(REPORT_MODULE_BSPP,REPORT_WARNING,"pic_size_in_map_units_minus1 [%d] is not within the range",psH264PPSInfo->pic_size_in_map_unit );
                    ePPSParseError |= BSPP_ERROR_UNRECOVERABLE;
                }
                ui32NumberBitsPerSliceGroupId = H264CeilLog2( psH264PPSInfo->num_slice_groups_minus1+1 );

                if((psH264PPSInfo->pic_size_in_map_unit+1) > psH264PPSInfo->sH264PPSSGMInfo.ui16SliceGroupIdNum)
                {
                    IMG_UINT8 * pui8SliceGroupId = (IMG_UINT8 *)IMG_MALLOC((psH264PPSInfo->pic_size_in_map_unit+1) * sizeof(IMG_UINT8));
                    if(pui8SliceGroupId == IMG_NULL)
                    {
                        REPORT(REPORT_MODULE_BSPP,REPORT_ERR,"out of memory");
                        ePPSParseError |= BSPP_ERROR_OUT_OF_MEMORY;
                    }
                    else
                    {
                        REPORT(REPORT_MODULE_BSPP,REPORT_WARNING,"reallocating SGM info from size %zu bytes to size %zu bytes", psH264PPSInfo->sH264PPSSGMInfo.ui16SliceGroupIdNum*sizeof(IMG_UINT8), (psH264PPSInfo->pic_size_in_map_unit+1)*sizeof(IMG_UINT8));
                        if(psH264PPSInfo->sH264PPSSGMInfo.slice_group_id)
                        {
                            IMG_MEMCPY(pui8SliceGroupId, psH264PPSInfo->sH264PPSSGMInfo.slice_group_id, psH264PPSInfo->sH264PPSSGMInfo.ui16SliceGroupIdNum * sizeof(IMG_UINT8));
                            IMG_FREE(psH264PPSInfo->sH264PPSSGMInfo.slice_group_id);
                        }
                        psH264PPSInfo->sH264PPSSGMInfo.ui16SliceGroupIdNum = (psH264PPSInfo->pic_size_in_map_unit+1);
                        psH264PPSInfo->sH264PPSSGMInfo.slice_group_id = pui8SliceGroupId;
                    }
                }

                IMG_ASSERT((psH264PPSInfo->pic_size_in_map_unit+1) <= psH264PPSInfo->sH264PPSSGMInfo.ui16SliceGroupIdNum);
                for ( i = 0; i <= psH264PPSInfo->pic_size_in_map_unit; i++ )
                {
                    psH264PPSInfo->sH264PPSSGMInfo.slice_group_id[i] = SWSR_ReadBits( hSwSrContext, ui32NumberBitsPerSliceGroupId );
                }
            }
        }

        for (i=0; i<H264FW_MAX_REFPIC_LISTS; i++)
        {
            psH264PPSInfo->num_ref_idx_lX_active_minus1[i] = SWSR_ReadUnsignedExpGoulomb( hSwSrContext );
            if(psH264PPSInfo->num_ref_idx_lX_active_minus1[i] >= SL_MAX_REF_IDX)
            {
                REPORT(REPORT_MODULE_BSPP,REPORT_WARNING,"num_ref_idx_lX_active_minus1[%d] [%d] is not within the range",i,psH264PPSInfo->num_ref_idx_lX_active_minus1[i]);
                ePPSParseError |= BSPP_ERROR_UNRECOVERABLE;
            }
        }

        psH264PPSInfo->weighted_pred_flag = SWSR_ReadBits( hSwSrContext, 1 );
        psH264PPSInfo->weighted_bipred_idc = SWSR_ReadBits( hSwSrContext, 2 );
        psH264PPSInfo->pic_init_qp_minus26 = SWSR_ReadSignedExpGoulomb( hSwSrContext );
        if( psH264PPSInfo->pic_init_qp_minus26 > 26 )
        {
            REPORT(REPORT_MODULE_BSPP,REPORT_WARNING ,"pic_init_qp_minus26[%d] is not within the range [-25 , 26]",psH264PPSInfo->pic_init_qp_minus26);
        }
        psH264PPSInfo->pic_init_qs_minus26 = SWSR_ReadSignedExpGoulomb( hSwSrContext );
        if( psH264PPSInfo->pic_init_qs_minus26 > 26 )
        {
            REPORT(REPORT_MODULE_BSPP,REPORT_WARNING,"pic_init_qs_minus26[%d] is not within the range [-25 , 26]",psH264PPSInfo->pic_init_qs_minus26);
        }
        psH264PPSInfo->chroma_qp_index_offset = SWSR_ReadSignedExpGoulomb( hSwSrContext );
        if(psH264PPSInfo->chroma_qp_index_offset > H264_MAX_CHROMA_QP_INDEX_OFFSET)
        {
            psH264PPSInfo->chroma_qp_index_offset = H264_MAX_CHROMA_QP_INDEX_OFFSET;
        }
        else if(psH264PPSInfo->chroma_qp_index_offset < H264_MIN_CHROMA_QP_INDEX_OFFSET)
        {
            psH264PPSInfo->chroma_qp_index_offset = H264_MIN_CHROMA_QP_INDEX_OFFSET;
        }

        psH264PPSInfo->deblocking_filter_control_present_flag = SWSR_ReadBits( hSwSrContext, 1 );
        psH264PPSInfo->constrained_intra_pred_flag = SWSR_ReadBits( hSwSrContext, 1 );
        psH264PPSInfo->redundant_pic_cnt_present_flag = SWSR_ReadBits( hSwSrContext, 1 );

        // Check for more rbsp data.
        ui32Result = SWSR_CheckMoreRbspData(hSwSrContext, &bMoreRbspData);
        if (ui32Result == IMG_SUCCESS && bMoreRbspData)
        {
            DEBUG_REPORT(REPORT_MODULE_BSPP,"More RBSP data is available");
            //Fidelity Range Extensions Stuff
            psH264PPSInfo->transform_8x8_mode_flag = SWSR_ReadBits( hSwSrContext, 1 );
            psH264PPSInfo->pic_scaling_matrix_present_flag = SWSR_ReadBits( hSwSrContext, 1 );
            if ( psH264PPSInfo->pic_scaling_matrix_present_flag )
            {
                if(IMG_NULL == psH264PPSInfo->pui8ScalingList4x4_pic)
                {
                    DEBUG_REPORT(REPORT_MODULE_BSPP, "Allocating ScalingList4x4_pic size %zu bytes", H264FW_NUM_4X4_LISTS*H264FW_4X4_SIZE*sizeof(IMG_UINT8));
                    psH264PPSInfo->pui8ScalingList4x4_pic = (IMG_UINT8 *)IMG_MALLOC(sizeof(IMG_UINT8 [H264FW_NUM_4X4_LISTS][H264FW_4X4_SIZE]));
                    if(psH264PPSInfo->pui8ScalingList4x4_pic == IMG_NULL)
                    {
                        ePPSParseError |= BSPP_ERROR_OUT_OF_MEMORY;
                    }
                    else
                    {
                        IMG_ASSERT(IMG_NULL != psH264PPSInfo->pui8ScalingList4x4_pic);
                        IMG_MEMSET(psH264PPSInfo->pui8ScalingList4x4_pic, 0x00, sizeof(IMG_UINT8 [H264FW_NUM_4X4_LISTS][H264FW_4X4_SIZE]));
                    }
                }
                if(IMG_NULL == psH264PPSInfo->pui8ScalingList8x8_pic)
                {
                    DEBUG_REPORT(REPORT_MODULE_BSPP, "Allocating ScalingList8x8_pic size %zu bytes", H264FW_NUM_8X8_LISTS*H264FW_8X8_SIZE*sizeof(IMG_UINT8));
                    psH264PPSInfo->pui8ScalingList8x8_pic = (IMG_UINT8 *)IMG_MALLOC(sizeof(IMG_UINT8 [H264FW_NUM_8X8_LISTS][H264FW_8X8_SIZE]));
                    if(psH264PPSInfo->pui8ScalingList8x8_pic == IMG_NULL)
                    {
                        ePPSParseError |= BSPP_ERROR_OUT_OF_MEMORY;
                    }
                    else
                    {
                        IMG_ASSERT(IMG_NULL != psH264PPSInfo->pui8ScalingList8x8_pic);
                        IMG_MEMSET(psH264PPSInfo->pui8ScalingList8x8_pic, 0x00, sizeof(IMG_UINT8 [H264FW_NUM_8X8_LISTS][H264FW_8X8_SIZE]));
                    }
                }

                {
                    IMG_UINT8 (*ScalingList4x4_pic)[H264FW_NUM_4X4_LISTS][H264FW_4X4_SIZE] = (IMG_UINT8 (*)[H264FW_NUM_4X4_LISTS][H264FW_4X4_SIZE])psH264PPSInfo->pui8ScalingList4x4_pic;
                    IMG_UINT8 (*ScalingList8x8_pic)[H264FW_NUM_8X8_LISTS][H264FW_8X8_SIZE] = (IMG_UINT8 (*)[H264FW_NUM_8X8_LISTS][H264FW_8X8_SIZE])psH264PPSInfo->pui8ScalingList8x8_pic;

                    DEBUG_REPORT(REPORT_MODULE_BSPP,"Scaling matrices are available");
                    // For chroma_format =3 (YUV444) total list would be 12 if transform_8x8_mode_flag is enabled else  6.
                    n_ScalingList = 6 +   ( chroma_format_idc != 3 ? 2:6) * psH264PPSInfo->transform_8x8_mode_flag;
                    if( n_ScalingList > 12 )
                    {
                        ePPSParseError |= BSPP_ERROR_UNRECOVERABLE;
                    }

                    IMG_ASSERT(IMG_NULL != psH264PPSInfo->pui8ScalingList4x4_pic);
                    IMG_ASSERT(IMG_NULL != psH264PPSInfo->pui8ScalingList8x8_pic);
                    for ( i = 0; i < n_ScalingList; i++ )
                    {
                        psH264PPSInfo->pic_scaling_list_present_flag[i] = SWSR_ReadBits( hSwSrContext, 1 );
                        if ( psH264PPSInfo->pic_scaling_list_present_flag[i] )
                        {
                            if ( i < 6 )
                            {
                                ePPSParseError |= bspp_H264ScalingListParser(hSwSrContext, (*ScalingList4x4_pic)[i], 16, &psH264PPSInfo->UseDefaultScalingMatrixFlag_pic[i] );
                            }
                            else
                            {
                                ePPSParseError |= bspp_H264ScalingListParser(hSwSrContext, (*ScalingList8x8_pic)[i-6], 64, &psH264PPSInfo->UseDefaultScalingMatrixFlag_pic[i] );
                            }
                        }
                    }
                }
            }
            psH264PPSInfo->second_chroma_qp_index_offset = SWSR_ReadSignedExpGoulomb( hSwSrContext);
            if(psH264PPSInfo->second_chroma_qp_index_offset > H264_MAX_CHROMA_QP_INDEX_OFFSET)
            {
                psH264PPSInfo->second_chroma_qp_index_offset = H264_MAX_CHROMA_QP_INDEX_OFFSET;
            }
            else if(psH264PPSInfo->second_chroma_qp_index_offset < H264_MIN_CHROMA_QP_INDEX_OFFSET)
            {
                psH264PPSInfo->second_chroma_qp_index_offset = H264_MIN_CHROMA_QP_INDEX_OFFSET;
            }
        }
        else
        {
            psH264PPSInfo->second_chroma_qp_index_offset = psH264PPSInfo->chroma_qp_index_offset;
        }

        if( SWSR_EXCEPT_NO_EXCEPTION != SWSR_CheckException(hSwSrContext) )
        {
            ePPSParseError |= BSPP_ERROR_INSUFFICIENT_DATA;
        }
    }

    return ePPSParseError;
}


/******************************************************************************

 @Function              BSPP_H264ResetSPSInfo

 @Description

******************************************************************************/
IMG_RESULT BSPP_H264ResetSequHdrInfo(
    IMG_HANDLE       hSecureSPSInfo
)
{
    BSPP_sH264SequHdrInfo * psH264SequHdrInfo = IMG_NULL;

    IMG_UINT16 * pui16AnchorRefIndicies1X = IMG_NULL;
    IMG_UINT16 * pui16NonAnchorRefIndicies1X = IMG_NULL;

    IMG_UINT32 * pui32NALHRDBitRateValueMinus1 = IMG_NULL;
    IMG_UINT32 * pui32VCLHRDBitRateValueMinus1 = IMG_NULL;
    IMG_UINT32 * pui32NALHRDCPBSizeValueMinus1 = IMG_NULL;
    IMG_UINT32 * pui32VCLHRDCPBSizeValueMinus1 = IMG_NULL;
    IMG_UINT8 * pui32NALHRDCBRFlag = IMG_NULL;
    IMG_UINT8 * pui32VCLHRDCBRFlag = IMG_NULL;

    IMG_UINT32 * pui32offset_for_ref_frame = IMG_NULL;
    IMG_UINT8 * pui8ScalingList4x4_seq = IMG_NULL;
    IMG_UINT8 * pui8ScalingList8x8_seq = IMG_NULL;

    if(IMG_NULL == hSecureSPSInfo)
    {
        return IMG_ERROR_INVALID_PARAMETERS;
    }
    psH264SequHdrInfo = (BSPP_sH264SequHdrInfo *)hSecureSPSInfo;

    //Storing temp values
    pui16AnchorRefIndicies1X = psH264SequHdrInfo->sSPSMVCExtInfo.pui16AnchorRefIndicies1X;
    pui16NonAnchorRefIndicies1X = psH264SequHdrInfo->sSPSMVCExtInfo.pui16NonAnchorRefIndicies1X;

    pui32NALHRDBitRateValueMinus1 = psH264SequHdrInfo->sVUIInfo.sNALHRDParameters.pui32BitRateValueMinus1;
    pui32VCLHRDBitRateValueMinus1 = psH264SequHdrInfo->sVUIInfo.sVCLHRDParameters.pui32BitRateValueMinus1;

    pui32NALHRDCPBSizeValueMinus1 = psH264SequHdrInfo->sVUIInfo.sNALHRDParameters.pui32CPBSizeValueMinus1;
    pui32VCLHRDCPBSizeValueMinus1 = psH264SequHdrInfo->sVUIInfo.sVCLHRDParameters.pui32CPBSizeValueMinus1;

    pui32NALHRDCBRFlag = psH264SequHdrInfo->sVUIInfo.sNALHRDParameters.pui8CBRFlag;
    pui32VCLHRDCBRFlag = psH264SequHdrInfo->sVUIInfo.sVCLHRDParameters.pui8CBRFlag;

    pui32offset_for_ref_frame = psH264SequHdrInfo->sSPSInfo.pui32offset_for_ref_frame;
    pui8ScalingList4x4_seq = psH264SequHdrInfo->sSPSInfo.pui8ScalingList4x4_seq;
    pui8ScalingList8x8_seq = psH264SequHdrInfo->sSPSInfo.pui8ScalingList8x8_seq;

    //Cleaning sSPSMVCExtInfo
    if(psH264SequHdrInfo->sSPSMVCExtInfo.pui16AnchorRefIndicies1X)
    {
        IMG_MEMSET(psH264SequHdrInfo->sSPSMVCExtInfo.pui16AnchorRefIndicies1X, 0x00, sizeof(IMG_UINT16 [2][VDEC_H264_MVC_MAX_VIEWS][VDEC_H264_MVC_MAX_REFS]));
    }
    if(psH264SequHdrInfo->sSPSMVCExtInfo.pui16NonAnchorRefIndicies1X)
    {
        IMG_MEMSET(psH264SequHdrInfo->sSPSMVCExtInfo.pui16NonAnchorRefIndicies1X, 0x00, sizeof(IMG_UINT16 [2][VDEC_H264_MVC_MAX_VIEWS][VDEC_H264_MVC_MAX_REFS]));
    }

    //Cleaning sVUIInfo
    if(psH264SequHdrInfo->sVUIInfo.sNALHRDParameters.pui32BitRateValueMinus1)
    {
        IMG_MEMSET(psH264SequHdrInfo->sVUIInfo.sNALHRDParameters.pui32BitRateValueMinus1, 0x00, VDEC_H264_MAXIMUMVALUEOFCPB_CNT * sizeof(IMG_UINT32));
    }

    if(psH264SequHdrInfo->sVUIInfo.sNALHRDParameters.pui32CPBSizeValueMinus1)
    {
        IMG_MEMSET(psH264SequHdrInfo->sVUIInfo.sNALHRDParameters.pui32CPBSizeValueMinus1, 0x00, VDEC_H264_MAXIMUMVALUEOFCPB_CNT * sizeof(IMG_UINT32));
    }

    if(psH264SequHdrInfo->sVUIInfo.sVCLHRDParameters.pui32CPBSizeValueMinus1)
    {
        IMG_MEMSET(psH264SequHdrInfo->sVUIInfo.sVCLHRDParameters.pui32CPBSizeValueMinus1, 0x00, VDEC_H264_MAXIMUMVALUEOFCPB_CNT * sizeof(IMG_UINT32));
    }
    if(psH264SequHdrInfo->sVUIInfo.sNALHRDParameters.pui8CBRFlag)
    {
        IMG_MEMSET(psH264SequHdrInfo->sVUIInfo.sNALHRDParameters.pui8CBRFlag, 0x00, VDEC_H264_MAXIMUMVALUEOFCPB_CNT * sizeof(IMG_UINT8));
    }
    if(psH264SequHdrInfo->sVUIInfo.sVCLHRDParameters.pui8CBRFlag)
    {
        IMG_MEMSET(psH264SequHdrInfo->sVUIInfo.sVCLHRDParameters.pui8CBRFlag, 0x00, VDEC_H264_MAXIMUMVALUEOFCPB_CNT * sizeof(IMG_UINT8));
    }

    //Cleaning sSPSInfo
    if(psH264SequHdrInfo->sSPSInfo.pui32offset_for_ref_frame)
    {
        IMG_MEMSET(psH264SequHdrInfo->sSPSInfo.pui32offset_for_ref_frame, 0x00, H264FW_MAX_CYCLE_REF_FRAMES * sizeof(IMG_UINT32));
    }
    if(psH264SequHdrInfo->sSPSInfo.pui8ScalingList4x4_seq)
    {
        IMG_MEMSET(psH264SequHdrInfo->sSPSInfo.pui8ScalingList4x4_seq, 0x00, sizeof(IMG_UINT8 [H264FW_NUM_4X4_LISTS][H264FW_4X4_SIZE]));
    }
    if(psH264SequHdrInfo->sSPSInfo.pui8ScalingList8x8_seq)
    {
        IMG_MEMSET(psH264SequHdrInfo->sSPSInfo.pui8ScalingList8x8_seq, 0x00, sizeof(IMG_UINT8 [H264FW_NUM_8X8_LISTS][H264FW_8X8_SIZE]));
    }

    //Erasing the structure
    IMG_MEMSET(psH264SequHdrInfo, 0x00, sizeof(*psH264SequHdrInfo));

    //Restoring pointers
    psH264SequHdrInfo->sSPSMVCExtInfo.pui16AnchorRefIndicies1X = pui16AnchorRefIndicies1X;
    psH264SequHdrInfo->sSPSMVCExtInfo.pui16NonAnchorRefIndicies1X = pui16NonAnchorRefIndicies1X;

    psH264SequHdrInfo->sVUIInfo.sNALHRDParameters.pui32BitRateValueMinus1 = pui32NALHRDBitRateValueMinus1;
    psH264SequHdrInfo->sVUIInfo.sVCLHRDParameters.pui32BitRateValueMinus1 = pui32VCLHRDBitRateValueMinus1;

    psH264SequHdrInfo->sVUIInfo.sNALHRDParameters.pui32CPBSizeValueMinus1 = pui32NALHRDCPBSizeValueMinus1;
    psH264SequHdrInfo->sVUIInfo.sVCLHRDParameters.pui32CPBSizeValueMinus1 = pui32VCLHRDCPBSizeValueMinus1;

    psH264SequHdrInfo->sVUIInfo.sNALHRDParameters.pui8CBRFlag = pui32NALHRDCBRFlag;
    psH264SequHdrInfo->sVUIInfo.sVCLHRDParameters.pui8CBRFlag = pui32VCLHRDCBRFlag;

    psH264SequHdrInfo->sSPSInfo.pui32offset_for_ref_frame = pui32offset_for_ref_frame;
    psH264SequHdrInfo->sSPSInfo.pui8ScalingList4x4_seq = pui8ScalingList4x4_seq;
    psH264SequHdrInfo->sSPSInfo.pui8ScalingList8x8_seq = pui8ScalingList8x8_seq;
    return IMG_SUCCESS;
}

/******************************************************************************

 @Function              BSPP_H264ResetPPSInfo

 @Description

******************************************************************************/
IMG_RESULT BSPP_H264ResetPPSInfo(
    IMG_HANDLE       hSecurePPSInfo
)
{
    BSPP_sH264PPSInfo * psH264PPSInfo = IMG_NULL;
    IMG_UINT16 ui16SliceGroupIdNum = 0;
    IMG_UINT8 * slice_group_id = IMG_NULL;
    IMG_UINT8 * pui8ScalingList4x4_pic = IMG_NULL;
    IMG_UINT8 * pui8ScalingList8x8_pic = IMG_NULL;

    if(IMG_NULL == hSecurePPSInfo)
    {
        return IMG_ERROR_INVALID_PARAMETERS;
    }
    psH264PPSInfo = (BSPP_sH264PPSInfo *)hSecurePPSInfo;

    //Storing temp values (we want to leave the SGM structure
    //it may be useful again instead of reallocating later
    slice_group_id = psH264PPSInfo->sH264PPSSGMInfo.slice_group_id;
    ui16SliceGroupIdNum = psH264PPSInfo->sH264PPSSGMInfo.ui16SliceGroupIdNum;
    pui8ScalingList4x4_pic = psH264PPSInfo->pui8ScalingList4x4_pic;
    pui8ScalingList8x8_pic = psH264PPSInfo->pui8ScalingList8x8_pic;

    if(psH264PPSInfo->sH264PPSSGMInfo.slice_group_id)
    {
        IMG_MEMSET(psH264PPSInfo->sH264PPSSGMInfo.slice_group_id, 0x00, psH264PPSInfo->sH264PPSSGMInfo.ui16SliceGroupIdNum * sizeof(IMG_UINT8));
    }
    if(psH264PPSInfo->pui8ScalingList4x4_pic)
    {
        IMG_MEMSET(psH264PPSInfo->pui8ScalingList4x4_pic, 0x00, sizeof(IMG_UINT8 [H264FW_NUM_4X4_LISTS][H264FW_4X4_SIZE]));
    }
    if(psH264PPSInfo->pui8ScalingList8x8_pic)
    {
        IMG_MEMSET(psH264PPSInfo->pui8ScalingList8x8_pic, 0x00, sizeof(IMG_UINT8 [H264FW_NUM_8X8_LISTS][H264FW_8X8_SIZE]));
    }

    //Erasing the structure
    IMG_MEMSET(psH264PPSInfo, 0x00, sizeof(*psH264PPSInfo));

    //Copy the temp variable back
    psH264PPSInfo->sH264PPSSGMInfo.ui16SliceGroupIdNum = ui16SliceGroupIdNum;
    psH264PPSInfo->sH264PPSSGMInfo.slice_group_id = slice_group_id;
    psH264PPSInfo->pui8ScalingList4x4_pic = pui8ScalingList4x4_pic;
    psH264PPSInfo->pui8ScalingList8x8_pic = pui8ScalingList8x8_pic;

    return IMG_SUCCESS;
}

/******************************************************************************

 @Function              bspp_H264PictHdrParser

 @Description           Parse the Picture Header NAL unit
                        (only initial part of slice header(s) for H264)

******************************************************************************/
static BSPP_eErrorType bspp_H264PictHdrParser(
    IMG_HANDLE                hSwSrContext,
    IMG_HANDLE                hStrRes,
    BSPP_sH264SliceHdrInfo  * psH264SliceHdrInfo,
    BSPP_sPPSInfo          ** ppsPPSInfo,
    BSPP_sSequenceHdrInfo  ** ppsSequHdrInfo,
    H264_eNalUnitType         eNalUnitType,
    IMG_UINT8                 nal_ref_idc
)
{
    BSPP_eErrorType eSliceParseError = BSPP_ERROR_NONE;
    BSPP_sH264PPSInfo * psH264PPSInfo;
    BSPP_sPPSInfo * psPPSInfo;
    BSPP_sH264SequHdrInfo * psH264SequHdrInfo;
    BSPP_sSequenceHdrInfo * psSequHdrInfo;
    IMG_ASSERT(hSwSrContext != IMG_NULL);
    {
        IMG_MEMSET(psH264SliceHdrInfo, 0, sizeof(*psH264SliceHdrInfo));

        psH264SliceHdrInfo->first_mb_in_slice = SWSR_ReadUnsignedExpGoulomb( hSwSrContext );
        psH264SliceHdrInfo->slice_type = SWSR_ReadUnsignedExpGoulomb( hSwSrContext );
        if(psH264SliceHdrInfo->slice_type > 9)
        {
            REPORT(REPORT_MODULE_BSPP,REPORT_WARNING,"Slice Type [%d] invalid, set to P",psH264SliceHdrInfo->slice_type);
            psH264SliceHdrInfo->slice_type = 0;
            eSliceParseError |= BSPP_ERROR_CORRECTION_VALIDVALUE;
        }
        psH264SliceHdrInfo->slice_type = (BSPP_eH264SliceType)(psH264SliceHdrInfo->slice_type % 5);

        psH264SliceHdrInfo->pic_parameter_set_id = SWSR_ReadUnsignedExpGoulomb( hSwSrContext );
        if(psH264SliceHdrInfo->pic_parameter_set_id >= MAX_PPS_COUNT)
        {
            REPORT(REPORT_MODULE_BSPP,REPORT_WARNING,"Picture Parameter ID [%d] invalid, set to 0",psH264SliceHdrInfo->pic_parameter_set_id);
            psH264SliceHdrInfo->pic_parameter_set_id = 0;
            eSliceParseError |= BSPP_ERROR_CORRECTION_VALIDVALUE;
        }

        /* set relevant PPS and SPS */
        psPPSInfo = BSPP_GetPpsHdr(hStrRes, psH264SliceHdrInfo->pic_parameter_set_id);
        if (psPPSInfo == IMG_NULL)
        {
            eSliceParseError |= BSPP_ERROR_NO_PPS;
            goto error;
        }
        psH264PPSInfo = (BSPP_sH264PPSInfo *)psPPSInfo->hSecurePPSInfo;
        if (psH264PPSInfo == IMG_NULL)
        {
            eSliceParseError |= BSPP_ERROR_NO_PPS;
            goto error;
        }
        IMG_ASSERT(psH264PPSInfo->pic_parameter_set_id == psH264SliceHdrInfo->pic_parameter_set_id);
        *ppsPPSInfo = psPPSInfo;

        psSequHdrInfo = BSPP_GetSequHdr(hStrRes, GET_SUBSET_ID(eNalUnitType, psH264PPSInfo->seq_parameter_set_id));
        if (psSequHdrInfo == IMG_NULL)
        {
            eSliceParseError |= BSPP_ERROR_NO_SEQUENCE_HDR;
            goto error;
        }
        psH264SequHdrInfo = (BSPP_sH264SequHdrInfo *)psSequHdrInfo->hSecureSequenceInfo;
        IMG_ASSERT((IMG_UINT32)psH264SequHdrInfo->sSPSInfo.seq_parameter_set_id == psH264PPSInfo->seq_parameter_set_id);
        *ppsSequHdrInfo = psSequHdrInfo;
    }

    // For MINIMAL parsing in secure mode, slice header parsing can stop here, may be problematic with field-coded streams and splitting fields

    {
        if (psH264SequHdrInfo->sSPSInfo.separate_colour_plane_flag)
        {
            psH264SliceHdrInfo->colour_plane_id = SWSR_ReadBits( hSwSrContext, 2 );
        }
        else
        {
            psH264SliceHdrInfo->colour_plane_id = 0;
        }

        psH264SliceHdrInfo->frame_num = SWSR_ReadBits( hSwSrContext, psH264SequHdrInfo->sSPSInfo.log2_max_frame_num_minus4 + 4 );

        IMG_ASSERT( psH264SliceHdrInfo->frame_num < ( 1UL << (psH264SequHdrInfo->sSPSInfo.log2_max_frame_num_minus4 + 4 ) ) );

        if( !psH264SequHdrInfo->sSPSInfo.frame_mbs_only_flag )
        {
            if( ( psH264SliceHdrInfo->slice_type == B_SLICE ) && !psH264SequHdrInfo->sSPSInfo.direct_8x8_inference_flag )
            {
                 eSliceParseError |= BSPP_ERROR_INVALID_VALUE;
            }
            psH264SliceHdrInfo->field_pic_flag = SWSR_ReadBits( hSwSrContext, 1 );
            if( psH264SliceHdrInfo->field_pic_flag )
            {
                psH264SliceHdrInfo->bottom_field_flag = SWSR_ReadBits( hSwSrContext, 1);
            }
            else
            {
                psH264SliceHdrInfo->bottom_field_flag = 0;
            }
        }
        else
        {
            psH264SliceHdrInfo->field_pic_flag = 0;
            psH264SliceHdrInfo->bottom_field_flag = 0;
        }
    }

    // At this point we have everything we need, but we still lack all the conditions for detecting new pictures (needed for error cases)

    {
        if (eNalUnitType == H264_NALTYPE_IDR_SLICE)
        {
            psH264SliceHdrInfo->idr_pic_id = SWSR_ReadUnsignedExpGoulomb( hSwSrContext );
        }

        if (psH264SequHdrInfo->sSPSInfo.pic_order_cnt_type == 0)
        {
            psH264SliceHdrInfo->pic_order_cnt_lsb = SWSR_ReadBits( hSwSrContext, psH264SequHdrInfo->sSPSInfo.log2_max_pic_order_cnt_lsb_minus4 + 4 );
            if (psH264PPSInfo->pic_order_present_flag && !psH264SliceHdrInfo->field_pic_flag)
            {
                psH264SliceHdrInfo->delta_pic_order_cnt_bottom = SWSR_ReadSignedExpGoulomb( hSwSrContext );
            }
        }

        if (psH264SequHdrInfo->sSPSInfo.pic_order_cnt_type == 1 && !psH264SequHdrInfo->sSPSInfo.delta_pic_order_always_zero_flag)
        {
            psH264SliceHdrInfo->delta_pic_order_cnt[0] = SWSR_ReadSignedExpGoulomb( hSwSrContext );
            if (psH264PPSInfo->pic_order_present_flag && !psH264SliceHdrInfo->field_pic_flag)
            {
                psH264SliceHdrInfo->delta_pic_order_cnt[1] = SWSR_ReadSignedExpGoulomb( hSwSrContext );
            }
        }

        if (psH264PPSInfo->redundant_pic_cnt_present_flag)
        {
            psH264SliceHdrInfo->redundant_pic_cnt = SWSR_ReadUnsignedExpGoulomb( hSwSrContext );
        }
    }

    // For FMO streams, we need to go further
    if( (psH264PPSInfo->num_slice_groups_minus1 > 0) && psH264PPSInfo->slice_group_map_type >= 3 && psH264PPSInfo->slice_group_map_type <= 5 )
    {
        if( psH264SliceHdrInfo->slice_type == B_SLICE )
        {
            SWSR_ReadBits( hSwSrContext, 1);
        }

        if( psH264SliceHdrInfo->slice_type == P_SLICE || psH264SliceHdrInfo->slice_type == SP_SLICE || psH264SliceHdrInfo->slice_type == B_SLICE)
        {
            psH264SliceHdrInfo->num_ref_idx_active_override_flag = SWSR_ReadBits( hSwSrContext, 1);
            if( psH264SliceHdrInfo->num_ref_idx_active_override_flag )
            {
                psH264SliceHdrInfo->num_ref_idx_lX_active_minus1[0] = SWSR_ReadUnsignedExpGoulomb( hSwSrContext );
                if( psH264SliceHdrInfo->slice_type == B_SLICE )
                {
                    psH264SliceHdrInfo->num_ref_idx_lX_active_minus1[1] = SWSR_ReadUnsignedExpGoulomb( hSwSrContext );
                }
            }
        }

        if(psH264SliceHdrInfo->slice_type != SI_SLICE && psH264SliceHdrInfo->slice_type != I_SLICE)
        {
            // Reference picture list modification
            /* parse reordering info and pack into commands */
            IMG_UINT32 i;
            IMG_UINT32 ui32CmdNum, ui32ListNum;
            IMG_UINT32 ui32Command;

            i = (psH264SliceHdrInfo->slice_type == B_SLICE) ? 2 : 1;

            for ( ui32ListNum = 0; ui32ListNum < i; ui32ListNum++ )
            {
                ui32CmdNum = 0;
                if ( SWSR_ReadBits(hSwSrContext,1) )
                {
                    do
                    {
                        ui32Command = SWSR_ReadUnsignedExpGoulomb(hSwSrContext);
                        if ( ui32Command != 3 )
                        {
                            SWSR_ReadUnsignedExpGoulomb(hSwSrContext);
                            ui32CmdNum++;
                        }
                    } while ( ui32Command != 3 && ui32CmdNum <= SL_MAX_REF_IDX);
                }
            }
        }

        if( (psH264PPSInfo->weighted_pred_flag && psH264SliceHdrInfo->slice_type == P_SLICE) ||
            (psH264PPSInfo->weighted_bipred_idc && psH264SliceHdrInfo->slice_type == B_SLICE)
            )
        {
            IMG_BOOL bMonochrome;
            IMG_UINT32 ui32List, i, j, k;

            bMonochrome = (0 == psH264SequHdrInfo->sSPSInfo.chroma_format_idc )? IMG_TRUE: IMG_FALSE;

            SWSR_ReadUnsignedExpGoulomb(hSwSrContext);
            if ( !bMonochrome )
            {
                SWSR_ReadUnsignedExpGoulomb(hSwSrContext);
            }

            k = (psH264SliceHdrInfo->slice_type == B_SLICE) ? 2 : 1;

            for ( ui32List = 0; ui32List < k; ui32List++ )
            {
                for ( i = 0; i <= psH264SliceHdrInfo->num_ref_idx_lX_active_minus1[ui32List]; i++ )
                {
                    if ( SWSR_ReadBits( hSwSrContext , 1) )
                    {
                        SWSR_ReadSignedExpGoulomb( hSwSrContext );
                        SWSR_ReadSignedExpGoulomb( hSwSrContext );
                    }

                    if ( !bMonochrome )
                    {
                        if ( SWSR_ReadBits( hSwSrContext , 1) )
                        {
                            for ( j = 0; j < 2; j++ )
                            {
                                SWSR_ReadSignedExpGoulomb( hSwSrContext );
                                SWSR_ReadSignedExpGoulomb( hSwSrContext );
                            }
                        }
                    }
                }
            }
        }

        if(nal_ref_idc != 0)
        {

            IMG_UINT32 ui32MemManOp;

            if ( eNalUnitType == H264_NALTYPE_IDR_SLICE )
            {
                SWSR_ReadBits( hSwSrContext,1 );
                SWSR_ReadBits( hSwSrContext,1 );
            }
            else
            {
                if ( SWSR_ReadBits( hSwSrContext,1 ) )
                {
                    do
                    {
                        // clamp 0--6
                        ui32MemManOp = SWSR_ReadUnsignedExpGoulomb( hSwSrContext );
                        if ( (0 != ui32MemManOp) && (5 != ui32MemManOp) )
                        {

                            if ( 3 == ui32MemManOp )
                            {
                                SWSR_ReadUnsignedExpGoulomb( hSwSrContext );
                                SWSR_ReadUnsignedExpGoulomb( hSwSrContext );
                            }
                            else
                            {
                                SWSR_ReadUnsignedExpGoulomb( hSwSrContext );
                            }
                        }
                    } while ( 0 != ui32MemManOp );
                }
            }

        }

        if( psH264PPSInfo->entropy_coding_mode_flag && psH264SliceHdrInfo->slice_type != I_SLICE )
        {
            SWSR_ReadUnsignedExpGoulomb( hSwSrContext );
        }

        SWSR_ReadSignedExpGoulomb( hSwSrContext );

        if ( psH264SliceHdrInfo->slice_type == SP_SLICE || psH264SliceHdrInfo->slice_type == SI_SLICE )
        {
            if ( psH264SliceHdrInfo->slice_type == SP_SLICE )
            {
                SWSR_ReadBits( hSwSrContext,1 ); // sp_for_switch_flag
            }
            SWSR_ReadSignedExpGoulomb( hSwSrContext ); // slice_qs_delta
        }

        if ( psH264PPSInfo->deblocking_filter_control_present_flag )
        {
            if ( SWSR_ReadUnsignedExpGoulomb( hSwSrContext ) !=  1 )
            {
                SWSR_ReadSignedExpGoulomb( hSwSrContext );
                SWSR_ReadSignedExpGoulomb( hSwSrContext );
            }
        }

        if( psH264PPSInfo->num_slice_groups_minus1 > 0 &&
            psH264PPSInfo->slice_group_map_type >= 3 && psH264PPSInfo->slice_group_map_type <= 5)
        {
            IMG_UINT32 ui32NumSliceGroupMapUnits = (psH264SequHdrInfo->sSPSInfo.pic_height_in_map_units_minus1+1) *
                                                       (psH264SequHdrInfo->sSPSInfo.pic_width_in_mbs_minus1+1);
            IMG_UINT32 ui32Width = H264CeilLog2(ui32NumSliceGroupMapUnits/(psH264PPSInfo->slice_group_change_rate_minus1+1) +
                                       (ui32NumSliceGroupMapUnits%(psH264PPSInfo->slice_group_change_rate_minus1+1) == 0 ? 0 : 1) + 1 ); /* (7-32) */
            psH264SliceHdrInfo->slice_group_change_cycle = SWSR_ReadBits( hSwSrContext, ui32Width );
        }
    }


error:
    return eSliceParseError;
}


/*!
******************************************************************************

 @Function              bspp_H264SelectScalingLists

******************************************************************************/
static IMG_VOID bspp_H264SelectScalingLists(
    H264FW_sPicturePS       * psH264FWPPSInfo,
    BSPP_sH264PPSInfo       * psH264PPSInfo,
    BSPP_sH264SequHdrInfo   * psH264SequHdrInfo
)
{
    IMG_UINT32 ui32Num8x8Lists;
    IMG_UINT32 i;
    const IMG_UINT8 * pui8QuantMatrix = IMG_NULL;
    IMG_UINT8 (*ScalingList4x4_pic)[H264FW_NUM_4X4_LISTS][H264FW_4X4_SIZE] = (IMG_UINT8 (*)[H264FW_NUM_4X4_LISTS][H264FW_4X4_SIZE])psH264PPSInfo->pui8ScalingList4x4_pic;
    IMG_UINT8 (*ScalingList8x8_pic)[H264FW_NUM_8X8_LISTS][H264FW_8X8_SIZE] = (IMG_UINT8 (*)[H264FW_NUM_8X8_LISTS][H264FW_8X8_SIZE])psH264PPSInfo->pui8ScalingList8x8_pic;

    IMG_UINT8 (*ScalingList4x4_seq)[H264FW_NUM_4X4_LISTS][H264FW_4X4_SIZE] = (IMG_UINT8 (*)[H264FW_NUM_4X4_LISTS][H264FW_4X4_SIZE])psH264SequHdrInfo->sSPSInfo.pui8ScalingList4x4_seq;
    IMG_UINT8 (*ScalingList8x8_seq)[H264FW_NUM_8X8_LISTS][H264FW_8X8_SIZE] = (IMG_UINT8 (*)[H264FW_NUM_8X8_LISTS][H264FW_8X8_SIZE])psH264SequHdrInfo->sSPSInfo.pui8ScalingList8x8_seq;

    if(psH264SequHdrInfo->sSPSInfo.seq_scaling_matrix_present_flag )
    {
        IMG_ASSERT(IMG_NULL != psH264SequHdrInfo->sSPSInfo.pui8ScalingList4x4_seq);
        IMG_ASSERT(IMG_NULL != psH264SequHdrInfo->sSPSInfo.pui8ScalingList8x8_seq);
    }

    if ( psH264PPSInfo->pic_scaling_matrix_present_flag )
    {
        for ( i = 0; i < H264FW_NUM_4X4_LISTS; i++ )
        {
            if ( psH264PPSInfo->pic_scaling_list_present_flag[i] )
            {
                if ( psH264PPSInfo->UseDefaultScalingMatrixFlag_pic[i] )
                {
                    pui8QuantMatrix = (i > 2)? Default_4x4_Inter: Default_4x4_Intra;
                }
                else
                {
                    pui8QuantMatrix = (*ScalingList4x4_pic)[i];
                }
            }
            else
            {
                if ( psH264SequHdrInfo->sSPSInfo.seq_scaling_matrix_present_flag )
                {
                    /* SPS matrix present - use fallback rule B */
                    if ( 0 == i )       // first 4x4 Intra list
                    {
                        if(psH264SequHdrInfo->sSPSInfo.seq_scaling_list_present_flag[i] && !psH264SequHdrInfo->sSPSInfo.UseDefaultScalingMatrixFlag_seq[i])
                        {
                            IMG_ASSERT(IMG_NULL != psH264SequHdrInfo->sSPSInfo.pui8ScalingList4x4_seq);
                            if(IMG_NULL != ScalingList4x4_seq)
                            {
                                pui8QuantMatrix = (*ScalingList4x4_seq)[i];
                            }
                        }
                        else
                        {
                            pui8QuantMatrix = Default_4x4_Intra;
                        }
                    }
                    else if ( 3 == i )  // first 4x4 Inter list
                    {
                        if(psH264SequHdrInfo->sSPSInfo.seq_scaling_list_present_flag[i] && !psH264SequHdrInfo->sSPSInfo.UseDefaultScalingMatrixFlag_seq[i])
                        {
                            IMG_ASSERT(IMG_NULL != psH264SequHdrInfo->sSPSInfo.pui8ScalingList4x4_seq);
                            if(IMG_NULL != ScalingList4x4_seq)
                            {
                                pui8QuantMatrix = (*ScalingList4x4_seq)[i];
                            }
                        }
                        else
                        {
                            pui8QuantMatrix = Default_4x4_Inter;
                        }
                    }
                    else
                    {
                        pui8QuantMatrix = psH264FWPPSInfo->ScalingList4x4[i-1];
                    }
                }
                else
                {
                    /* SPS matrix not present - use fallback rule A */
                    if ( 0 == i )       // first 4x4 Intra list
                    {
                        pui8QuantMatrix = Default_4x4_Intra;
                    }
                    else if ( 3 == i )  // first 4x4 Inter list
                    {
                        pui8QuantMatrix = Default_4x4_Inter;
                    }
                    else
                    {
                        pui8QuantMatrix = psH264FWPPSInfo->ScalingList4x4[i-1];
                    }
                }
            }
            /* copy correct 4x4 list to output - as selected by PPS */
            IMG_MEMCPY( psH264FWPPSInfo->ScalingList4x4[i], pui8QuantMatrix ,sizeof(psH264FWPPSInfo->ScalingList4x4[i]) );
        }
    }
    else
    {
        /* PPS matrix not present, use SPS information */
        if ( psH264SequHdrInfo->sSPSInfo.seq_scaling_matrix_present_flag )
        {
            for ( i = 0; i < H264FW_NUM_4X4_LISTS; i++ )
            {
                if ( psH264SequHdrInfo->sSPSInfo.seq_scaling_list_present_flag[i] )
                {
                    if ( psH264SequHdrInfo->sSPSInfo.UseDefaultScalingMatrixFlag_seq[i] )
                    {
                        pui8QuantMatrix = (i > 2)? Default_4x4_Inter: Default_4x4_Intra;
                    }
                    else
                    {
                        IMG_ASSERT(IMG_NULL != psH264SequHdrInfo->sSPSInfo.pui8ScalingList4x4_seq);
                        if(IMG_NULL != ScalingList4x4_seq)
                        {
                            pui8QuantMatrix = (*ScalingList4x4_seq)[i] ;
                        }
                    }
                }
                else
                {
                    /* SPS list not present - use fallback rule A */
                    if ( 0 == i )       // first 4x4 Intra list
                    {
                        pui8QuantMatrix = Default_4x4_Intra;
                    }
                    else if ( 3 == i )  // first 4x4 Inter list
                    {
                        pui8QuantMatrix = Default_4x4_Inter;
                    }
                    else
                    {
                        pui8QuantMatrix = psH264FWPPSInfo->ScalingList4x4[i-1];
                    }
                }
                /* copy correct 4x4 list to output - as selected by SPS */
                IMG_MEMCPY( psH264FWPPSInfo->ScalingList4x4[i], pui8QuantMatrix ,sizeof(psH264FWPPSInfo->ScalingList4x4[i]) );
            }
        }
        else
        {
            /* SPS matrix not present - use flat lists */
            pui8QuantMatrix = Default_4x4_Org;
            for ( i = 0; i < H264FW_NUM_4X4_LISTS; i++ )
            {
                IMG_MEMCPY( psH264FWPPSInfo->ScalingList4x4[i], pui8QuantMatrix ,sizeof(psH264FWPPSInfo->ScalingList4x4[i]) );
            }
        }
    }

    /* 8x8 matrices */
    ui32Num8x8Lists = (3 == psH264SequHdrInfo->sSPSInfo.chroma_format_idc) ? 6 : 2;
    if ( psH264PPSInfo->transform_8x8_mode_flag )
    {
        if ( psH264PPSInfo->pic_scaling_matrix_present_flag )
        {
            for ( i = 0; i < ui32Num8x8Lists; i++ )
            {
                if ( psH264PPSInfo->pic_scaling_list_present_flag[i+H264FW_NUM_4X4_LISTS] )
                {
                    if ( psH264PPSInfo->UseDefaultScalingMatrixFlag_pic[i+H264FW_NUM_4X4_LISTS] )
                    {
                        pui8QuantMatrix = (i & 0x1)? Default_8x8_Inter: Default_8x8_Intra;
                    }
                    else
                    {
                        IMG_ASSERT(IMG_NULL != psH264PPSInfo->pui8ScalingList8x8_pic);
                        if(IMG_NULL != ScalingList8x8_pic)
                        {
                            pui8QuantMatrix = (*ScalingList8x8_pic)[i];
                        }
                    }
                }
                else
                {
                    if ( psH264SequHdrInfo->sSPSInfo.seq_scaling_matrix_present_flag )
                    {
                        /* SPS matrix present - use fallback rule B */
                        if ( 0 == i )       // list 6 - first 8x8 Intra list
                        {
                            if(psH264SequHdrInfo->sSPSInfo.seq_scaling_list_present_flag[i + H264FW_NUM_4X4_LISTS] &&
                                !psH264SequHdrInfo->sSPSInfo.UseDefaultScalingMatrixFlag_seq[i + H264FW_NUM_4X4_LISTS])
                            {
                                IMG_ASSERT(IMG_NULL != psH264SequHdrInfo->sSPSInfo.pui8ScalingList8x8_seq);
                                if(IMG_NULL != ScalingList8x8_seq)
                                {
                                    pui8QuantMatrix = (*ScalingList8x8_seq)[i];
                                }
                            }
                            else
                            {
                                pui8QuantMatrix = Default_8x8_Intra;
                            }
                        }
                        else if ( 1 == i )  // list 7 - first 8x8 Inter list
                        {
                            if(psH264SequHdrInfo->sSPSInfo.seq_scaling_list_present_flag[i + H264FW_NUM_4X4_LISTS] &&
                                !psH264SequHdrInfo->sSPSInfo.UseDefaultScalingMatrixFlag_seq[i + H264FW_NUM_4X4_LISTS])
                            {
                                IMG_ASSERT(IMG_NULL != psH264SequHdrInfo->sSPSInfo.pui8ScalingList8x8_seq);
                                if(IMG_NULL != ScalingList8x8_seq)
                                {
                                    pui8QuantMatrix = (*ScalingList8x8_seq)[i];
                                }
                            }
                            else
                            {
                                pui8QuantMatrix = Default_8x8_Inter;
                            }
                        }
                        else
                        {
                            pui8QuantMatrix = psH264FWPPSInfo->ScalingList8x8[i-2];
                        }
                    }
                    else
                    {
                        /* SPS matrix not present - use fallback rule A */
                        if ( 0 == i )       // list 6 - first 8x8 Intra list
                        {
                            pui8QuantMatrix = Default_8x8_Intra;
                        }
                        else if ( 1 == i )  // list 7 - first 8x8 Inter list
                        {
                            pui8QuantMatrix = Default_8x8_Inter;
                        }
                        else
                        {
                            pui8QuantMatrix = psH264FWPPSInfo->ScalingList8x8[i-2];
                        }
                    }
                }
                /* copy correct 8x8 list to output - as selected by PPS */
                IMG_MEMCPY( psH264FWPPSInfo->ScalingList8x8[i], pui8QuantMatrix ,sizeof(psH264FWPPSInfo->ScalingList8x8[i]) );
            }
        }
        else
        {
            /* PPS matrix not present, use SPS information */
            if ( psH264SequHdrInfo->sSPSInfo.seq_scaling_matrix_present_flag )
            {
                for ( i = 0; i < ui32Num8x8Lists; i++ )
                {
                    if ( psH264SequHdrInfo->sSPSInfo.seq_scaling_list_present_flag[i+H264FW_NUM_4X4_LISTS] )
                    {
                        if ( psH264SequHdrInfo->sSPSInfo.UseDefaultScalingMatrixFlag_seq[i+H264FW_NUM_4X4_LISTS] )
                        {
                            pui8QuantMatrix = (i & 0x1)? Default_8x8_Inter: Default_8x8_Intra;
                        }
                        else
                        {
                            IMG_ASSERT(IMG_NULL != psH264SequHdrInfo->sSPSInfo.pui8ScalingList8x8_seq);
                            if(IMG_NULL != ScalingList8x8_seq)
                            {
                                pui8QuantMatrix = (*ScalingList8x8_seq)[i] ;
                            }
                        }
                    }
                    else
                    {
                        /* SPS list not present - use fallback rule A */
                        if ( 0 == i )       // list 6 - first 8x8 Intra list
                        {
                            pui8QuantMatrix = Default_8x8_Intra;
                        }
                        else if ( 1 == i )  // list 7 - first 8x8 Inter list
                        {
                            pui8QuantMatrix = Default_8x8_Inter;
                        }
                        else
                        {
                            pui8QuantMatrix = psH264FWPPSInfo->ScalingList8x8[i-2];
                        }
                    }
                    /* copy correct 8x8 list to output - as selected by SPS */
                    IMG_MEMCPY( psH264FWPPSInfo->ScalingList8x8[i], pui8QuantMatrix ,sizeof(psH264FWPPSInfo->ScalingList8x8[i]) );
                }
            }
            else
            {
                /* SPS matrix not present - use flat lists */
                pui8QuantMatrix = Default_8x8_Org;
                for ( i = 0; i < ui32Num8x8Lists; i++ )
                {
                    IMG_MEMCPY( psH264FWPPSInfo->ScalingList8x8[i], pui8QuantMatrix ,sizeof(psH264FWPPSInfo->ScalingList8x8[i]) );
                }
            }
        }
    }
}


/*!
******************************************************************************

 @Function              bspp_H264FWPPSPopulate

******************************************************************************/
static IMG_VOID bspp_H264FWPPSPopulate(
    BSPP_sH264PPSInfo   * psH264PPSInfo,
    H264FW_sPicturePS   * psH264FWPPSInfo
)
{
    psH264FWPPSInfo->deblocking_filter_control_present_flag = psH264PPSInfo->deblocking_filter_control_present_flag;
    psH264FWPPSInfo->transform_8x8_mode_flag                = psH264PPSInfo->transform_8x8_mode_flag;
    psH264FWPPSInfo->entropy_coding_mode_flag               = psH264PPSInfo->entropy_coding_mode_flag;
    psH264FWPPSInfo->redundant_pic_cnt_present_flag         = psH264PPSInfo->redundant_pic_cnt_present_flag;
    psH264FWPPSInfo->weighted_bipred_idc                    = psH264PPSInfo->weighted_bipred_idc;
    psH264FWPPSInfo->weighted_pred_flag                     = psH264PPSInfo->weighted_pred_flag;
    psH264FWPPSInfo->pic_order_present_flag                 = psH264PPSInfo->pic_order_present_flag;
    psH264FWPPSInfo->pic_init_qp                            = psH264PPSInfo->pic_init_qp_minus26 + 26;
    psH264FWPPSInfo->constrained_intra_pred_flag            = psH264PPSInfo->constrained_intra_pred_flag;
    IMG_ASSERT(sizeof(psH264FWPPSInfo->num_ref_lX_active_minus1) == sizeof(psH264PPSInfo->num_ref_idx_lX_active_minus1));
    IMG_ASSERT(sizeof(psH264FWPPSInfo->num_ref_lX_active_minus1) == sizeof(IMG_UINT8)*H264FW_MAX_REFPIC_LISTS);
    IMG_MEMCPY(
        psH264FWPPSInfo->num_ref_lX_active_minus1,
        psH264PPSInfo->num_ref_idx_lX_active_minus1,
        sizeof(psH264FWPPSInfo->num_ref_lX_active_minus1)
    );
    psH264FWPPSInfo->slice_group_map_type                   = psH264PPSInfo->slice_group_map_type;
    psH264FWPPSInfo->num_slice_groups_minus1                = psH264PPSInfo->num_slice_groups_minus1;
    psH264FWPPSInfo->slice_group_change_rate_minus1         = psH264PPSInfo->slice_group_change_rate_minus1;
    psH264FWPPSInfo->chroma_qp_index_offset                 = psH264PPSInfo->chroma_qp_index_offset;
    psH264FWPPSInfo->second_chroma_qp_index_offset          = psH264PPSInfo->second_chroma_qp_index_offset;

    // Do simple accurate case: calculate Scaling Lists at the first slice (First MB offset [0,0]) of each picture
}


/*!
******************************************************************************

 @Function              bspp_H264FWSequHdrPopulate

******************************************************************************/
static IMG_VOID bspp_H264FWSequHdrPopulate(
    BSPP_sH264SequHdrInfo   * psH264SequHdrInfo,
    H264FW_sSequencePS      * psH264FWSequHdrInfo
)
{
    // Basic SPS
    psH264FWSequHdrInfo->profile_idc                            = psH264SequHdrInfo->sSPSInfo.profile_idc;
    psH264FWSequHdrInfo->chroma_format_idc                      = psH264SequHdrInfo->sSPSInfo.chroma_format_idc;
    psH264FWSequHdrInfo->separate_colour_plane_flag             = psH264SequHdrInfo->sSPSInfo.separate_colour_plane_flag;
    psH264FWSequHdrInfo->bit_depth_luma_minus8                  = psH264SequHdrInfo->sSPSInfo.bit_depth_luma_minus8;
    psH264FWSequHdrInfo->bit_depth_chroma_minus8                = psH264SequHdrInfo->sSPSInfo.bit_depth_chroma_minus8;
    psH264FWSequHdrInfo->delta_pic_order_always_zero_flag       = psH264SequHdrInfo->sSPSInfo.delta_pic_order_always_zero_flag;
    psH264FWSequHdrInfo->log2_max_pic_order_cnt_lsb             = psH264SequHdrInfo->sSPSInfo.log2_max_pic_order_cnt_lsb_minus4 + 4;
    psH264FWSequHdrInfo->max_num_ref_frames                     = psH264SequHdrInfo->sSPSInfo.max_num_ref_frames;
    psH264FWSequHdrInfo->log2_max_frame_num                     = psH264SequHdrInfo->sSPSInfo.log2_max_frame_num_minus4 + 4;
    psH264FWSequHdrInfo->pic_order_cnt_type                     = psH264SequHdrInfo->sSPSInfo.pic_order_cnt_type;
    psH264FWSequHdrInfo->frame_mbs_only_flag                    = psH264SequHdrInfo->sSPSInfo.frame_mbs_only_flag;
    psH264FWSequHdrInfo->gaps_in_frame_num_value_allowed_flag   = psH264SequHdrInfo->sSPSInfo.gaps_in_frame_num_value_allowed_flag;
    psH264FWSequHdrInfo->constraint_set_flags                   = psH264SequHdrInfo->sSPSInfo.constraint_set_flags;
    psH264FWSequHdrInfo->level_idc                              = psH264SequHdrInfo->sSPSInfo.level_idc;
    psH264FWSequHdrInfo->num_ref_frames_in_pic_order_cnt_cycle  = psH264SequHdrInfo->sSPSInfo.num_ref_frames_in_pic_order_cnt_cycle;
    psH264FWSequHdrInfo->mb_adaptive_frame_field_flag           = psH264SequHdrInfo->sSPSInfo.mb_adaptive_frame_field_flag;
    psH264FWSequHdrInfo->offset_for_non_ref_pic                 = psH264SequHdrInfo->sSPSInfo.offset_for_non_ref_pic;
    psH264FWSequHdrInfo->offset_for_top_to_bottom_field         = psH264SequHdrInfo->sSPSInfo.offset_for_top_to_bottom_field;
    psH264FWSequHdrInfo->pic_width_in_mbs_minus1                = psH264SequHdrInfo->sSPSInfo.pic_width_in_mbs_minus1;
    psH264FWSequHdrInfo->pic_height_in_map_units_minus1         = psH264SequHdrInfo->sSPSInfo.pic_height_in_map_units_minus1;
    psH264FWSequHdrInfo->direct_8x8_inference_flag              = psH264SequHdrInfo->sSPSInfo.direct_8x8_inference_flag;
    psH264FWSequHdrInfo->qpprime_y_zero_transform_bypass_flag   = psH264SequHdrInfo->sSPSInfo.qpprime_y_zero_transform_bypass_flag;
    //IMG_ASSERT(sizeof(psH264FWSequHdrInfo->offset_for_ref_frame) == sizeof(psH264SequHdrInfo->sSPSInfo.offset_for_ref_frame));
    //IMG_ASSERT(sizeof(psH264FWSequHdrInfo->offset_for_ref_frame) == sizeof(IMG_INT32)*H264FW_MAX_CYCLE_REF_FRAMES);
    if(IMG_NULL != psH264SequHdrInfo->sSPSInfo.pui32offset_for_ref_frame)
    {
        IMG_MEMCPY(psH264FWSequHdrInfo->offset_for_ref_frame, psH264SequHdrInfo->sSPSInfo.pui32offset_for_ref_frame, sizeof(psH264FWSequHdrInfo->offset_for_ref_frame));
    }
    else
    {
        IMG_MEMSET(psH264FWSequHdrInfo->offset_for_ref_frame, 0x00, sizeof(psH264FWSequHdrInfo->offset_for_ref_frame));
    }

    // From VUI
    if (psH264SequHdrInfo->sVUIInfo.bitstream_restriction_flag)
    {
        IMG_ASSERT(psH264SequHdrInfo->sSPSInfo.vui_parameters_present_flag);
        psH264FWSequHdrInfo->max_dec_frame_buffering = psH264SequHdrInfo->sVUIInfo.max_dec_frame_buffering;
        psH264FWSequHdrInfo->num_reorder_frames = psH264SequHdrInfo->sVUIInfo.num_reorder_frames;
    }
    else
    {
        //psH264FWSequHdrInfo->max_dec_frame_buffering = ( (psH264SequHdrInfo->sSPSMVCExtInfo.ui16NumViewsMinus1 >= 2) ? 32 : 16 );  // This is the rule for MaxDpbFrames
        //psH264FWSequHdrInfo->num_reorder_frames = psH264FWSequHdrInfo->max_dec_frame_buffering;
        psH264FWSequHdrInfo->num_reorder_frames = 16;

    }
    // From MVC
    psH264FWSequHdrInfo->ui8NumViews = psH264SequHdrInfo->sSPSMVCExtInfo.ui16NumViewsMinus1 + 1;
    IMG_ASSERT(sizeof(psH264FWSequHdrInfo->aui16ViewIds) == sizeof(psH264SequHdrInfo->sSPSMVCExtInfo.aui16ViewId));
    IMG_ASSERT(sizeof(psH264FWSequHdrInfo->aui16ViewIds) == sizeof(IMG_UINT16)*VDEC_H264_MVC_MAX_VIEWS);
    IMG_MEMCPY(psH264FWSequHdrInfo->aui16ViewIds, psH264SequHdrInfo->sSPSMVCExtInfo.aui16ViewId, sizeof(psH264FWSequHdrInfo->aui16ViewIds));

    IMG_ASSERT(sizeof(psH264FWSequHdrInfo->aui16NumAnchorRefsX) == sizeof(psH264SequHdrInfo->sSPSMVCExtInfo.aui16NumAnchorRefs1X));
    IMG_ASSERT(sizeof(psH264FWSequHdrInfo->aui16NumNonAnchorRefsX) == sizeof(psH264SequHdrInfo->sSPSMVCExtInfo.aui16NumNonAnchorRefs1X));
    //IMG_ASSERT(sizeof(psH264FWSequHdrInfo->aui16AnchorInterViewReferenceIDLX) == sizeof(psH264SequHdrInfo->sSPSMVCExtInfo.AnchorRefIndicies1X));
    //IMG_ASSERT(sizeof(psH264FWSequHdrInfo->aui16NonAnchorInterViewReferenceIDLX) == sizeof(psH264SequHdrInfo->sSPSMVCExtInfo.Non(*AnchorRefIndicies1X)[0][));

    IMG_MEMCPY(psH264FWSequHdrInfo->aui16NumAnchorRefsX, psH264SequHdrInfo->sSPSMVCExtInfo.aui16NumAnchorRefs1X, sizeof(psH264FWSequHdrInfo->aui16NumAnchorRefsX));
    IMG_MEMCPY(psH264FWSequHdrInfo->aui16NumNonAnchorRefsX, psH264SequHdrInfo->sSPSMVCExtInfo.aui16NumNonAnchorRefs1X, sizeof(psH264FWSequHdrInfo->aui16NumNonAnchorRefsX));

    if(IMG_NULL != psH264SequHdrInfo->sSPSMVCExtInfo.pui16AnchorRefIndicies1X)
    {
        IMG_MEMCPY(psH264FWSequHdrInfo->aui16AnchorInterViewReferenceIDLX, psH264SequHdrInfo->sSPSMVCExtInfo.pui16AnchorRefIndicies1X, sizeof(psH264FWSequHdrInfo->aui16AnchorInterViewReferenceIDLX));
    }
    else
    {
        IMG_MEMSET(psH264FWSequHdrInfo->aui16AnchorInterViewReferenceIDLX, 0x00, sizeof(psH264FWSequHdrInfo->aui16AnchorInterViewReferenceIDLX));
    }

    if(IMG_NULL != psH264SequHdrInfo->sSPSMVCExtInfo.pui16NonAnchorRefIndicies1X)
    {
        IMG_MEMCPY(psH264FWSequHdrInfo->aui16NonAnchorInterViewReferenceIDLX, psH264SequHdrInfo->sSPSMVCExtInfo.pui16NonAnchorRefIndicies1X, sizeof(psH264FWSequHdrInfo->aui16NonAnchorInterViewReferenceIDLX));
    }
    else
    {
        IMG_MEMSET(psH264FWSequHdrInfo->aui16NonAnchorInterViewReferenceIDLX, 0x00, sizeof(psH264FWSequHdrInfo->aui16NonAnchorInterViewReferenceIDLX));
    }

    // From SEI - SEI may have not been processed or even met yet, so there is no point updating here, since it is not related to the SPS
    //psH264FWSequHdrInfo->bDisableVDMCFilt = 0;
    //psH264FWSequHdrInfo->b4x4TransformMBNotAvailable = 0;       // Sideband encoder data
}


/*!
******************************************************************************

 @Function              bspp_H264CommonSequHdrPopulate

******************************************************************************/
static IMG_VOID bspp_H264CommonSequHdrPopulate(
    BSPP_sH264SequHdrInfo   * psH264SequHdrInfo,
    VDEC_sComSequHdrInfo    * psComSequHdrInfo
)
{
    BSPP_sH264SPSInfo * psSPSInfo = &psH264SequHdrInfo->sSPSInfo;
    BSPP_sH264VUIInfo * psVUIInfo = &psH264SequHdrInfo->sVUIInfo;

    psComSequHdrInfo->ui32CodecProfile = psSPSInfo->profile_idc;
    psComSequHdrInfo->ui32CodecLevel = psSPSInfo->level_idc;

    if( (H264_Profile_MVC_HIGH == psSPSInfo->profile_idc) || (H264_Profile_MVC_STEREO == psSPSInfo->profile_idc) )
    {
        psComSequHdrInfo->ui32NumViews = psH264SequHdrInfo->sSPSMVCExtInfo.ui16NumViewsMinus1 + 1;
    }

    //psComSequHdrInfo->ui32Bitrate = ?
    if (psSPSInfo->vui_parameters_present_flag && psVUIInfo->timing_info_present_flag)
    {
        psComSequHdrInfo->ui32FrameRateNum = psVUIInfo->time_scale;
        psComSequHdrInfo->ui32FrameRateDen = 2*psVUIInfo->num_units_in_tick;
        // we do not support floating point in TEE, and framerate is not important for h264
		//psComSequHdrInfo->fFrameRate = 0;
    }
    else
    {
        //psComSequHdrInfo->ui32FrameRateNum = ?
        //psComSequHdrInfo->ui32FrameRateDen = ?
        //psComSequHdrInfo->fFrameRate = ?
    }

    /* ColorSpace Description was present in the VUI parameters.
       copy it in CommonSeqHdr info for use by application.*/
    if( (psVUIInfo->video_signal_type_present_flag ) 
        & (psVUIInfo->colour_description_present_flag))
    {
        psComSequHdrInfo->sColorSpaceInfo.bIsPresent = IMG_TRUE;
        psComSequHdrInfo->sColorSpaceInfo.ui8ColourPrimaries = psVUIInfo->colour_primaries;
        psComSequHdrInfo->sColorSpaceInfo.ui8TransferCharacteristics = psVUIInfo->transfer_characteristics;
        psComSequHdrInfo->sColorSpaceInfo.ui8MatrixCoefficients = psVUIInfo->matrix_coefficients;
    }

    if (psVUIInfo->aspect_ratio_info_present_flag)
    {
        psComSequHdrInfo->ui32AspectRatioNum = psVUIInfo->sar_width;
        psComSequHdrInfo->ui32AspectRatioDen = psVUIInfo->sar_height;
    }
    else
    {
        //psComSequHdrInfo->ui32AspectRatioNum = ?
        //psComSequHdrInfo->ui32AspectRatioDen = ?
    }
    psComSequHdrInfo->bInterlacedFrames = psSPSInfo->frame_mbs_only_flag ? IMG_FALSE : IMG_TRUE;
    {
        //<! sPixelInfo populate
        IMG_ASSERT(psSPSInfo->chroma_format_idc<4);
        psComSequHdrInfo->sPixelInfo.bChromaFormat = (psSPSInfo->chroma_format_idc==0) ? IMG_FALSE : IMG_TRUE;
        psComSequHdrInfo->sPixelInfo.eChromaFormatIdc = aePixelFormatIdc[psSPSInfo->chroma_format_idc];
        psComSequHdrInfo->sPixelInfo.eChromaInterleaved = ((psSPSInfo->chroma_format_idc==0) || (psSPSInfo->chroma_format_idc==3 && psSPSInfo->separate_colour_plane_flag))? PIXEL_INVALID_CI : PIXEL_UV_ORDER;
        psComSequHdrInfo->sPixelInfo.ui32NoPlanes =
            (psSPSInfo->chroma_format_idc==0) ? 1 :
            (psSPSInfo->chroma_format_idc==3 && psSPSInfo->separate_colour_plane_flag) ? 3 :
            2;
        psComSequHdrInfo->sPixelInfo.ui32BitDepthY = psSPSInfo->bit_depth_luma_minus8 + 8;
        psComSequHdrInfo->sPixelInfo.ui32BitDepthC = psSPSInfo->bit_depth_chroma_minus8 + 8;
        psComSequHdrInfo->sPixelInfo.eMemoryPacking =
            (psComSequHdrInfo->sPixelInfo.ui32BitDepthY>8 || psComSequHdrInfo->sPixelInfo.ui32BitDepthC>8) ? PIXEL_BIT10_MSB_MP : PIXEL_BIT8_MP;
        psComSequHdrInfo->sPixelInfo.ePixelFormat =
            PIXEL_GetPixelFormat(
                psComSequHdrInfo->sPixelInfo.eChromaFormatIdc,
                psComSequHdrInfo->sPixelInfo.eChromaInterleaved,
                psComSequHdrInfo->sPixelInfo.eMemoryPacking,
                psComSequHdrInfo->sPixelInfo.ui32BitDepthY,
                psComSequHdrInfo->sPixelInfo.ui32BitDepthC,
                psComSequHdrInfo->sPixelInfo.ui32NoPlanes
            );
    }
    {
        //<! sMaxFrameSize populate
        psComSequHdrInfo->sMaxFrameSize.ui32Width = (psSPSInfo->pic_width_in_mbs_minus1 + 1) * 16;
        //<! H264 has always coded size MB aligned. For sequences which *may* have Field-Coded pictures, as described by the frame_mbs_only_flag,
        //<! the pic_height_in_map_units_minus1 refers to field height in MBs, so to find the actual Frame height we need to do Field_MBs_InHeight * 32
        psComSequHdrInfo->sMaxFrameSize.ui32Height = (psSPSInfo->pic_height_in_map_units_minus1 + 1) * (psSPSInfo->frame_mbs_only_flag ? 1 : 2) * 16;
    }
    psComSequHdrInfo->bFieldCodedMBlocks = psSPSInfo->mb_adaptive_frame_field_flag;
    psComSequHdrInfo->ui32MinPicBufNum = psVUIInfo->max_dec_frame_buffering;
    //psComSequHdrInfo->bPictureReordering = ?
    //psComSequHdrInfo->bPostProcessing = ?
    {
        //<! sOrigDisplayRegion populate
        if (psSPSInfo->frame_cropping_flag)
        {
            IMG_INT32 i32SubWidthC, i32SubHeightC, i32CropUnitX, i32CropUnitY;
            IMG_INT32 i32FrameCropLeft, i32FrameCropRight, i32FrameCropTop, i32FrameCropBottom;
            i32SubWidthC = bspp_h264_get_SubWidthC(
                psSPSInfo->chroma_format_idc,
                psSPSInfo->separate_colour_plane_flag
                );
            i32SubHeightC = bspp_h264_get_SubHeightC(
                psSPSInfo->chroma_format_idc,
                psSPSInfo->separate_colour_plane_flag
                );
            /* equation source: ITU-T H.264 2010/03, page 77 */
            /* ChromaArrayType == 0 */
            if(psSPSInfo->separate_colour_plane_flag || psSPSInfo->chroma_format_idc==0)
            {
                i32CropUnitX = 1; /* (7-18) */
                i32CropUnitY = 2 - psSPSInfo->frame_mbs_only_flag; /* (7-19) */
            }
            /* ChromaArrayType == chroma_format_idc */
            else
            {
                i32CropUnitX = i32SubWidthC; /* (7-20) */
                i32CropUnitY = i32SubHeightC * (2 - psSPSInfo->frame_mbs_only_flag); /* (7-21) */
            }
            //i32PicWidthInSamplesL = psComSequHdrInfo->sMaxFrameSize.ui32Width;
            //i32FrameHeight = psComSequHdrInfo->sMaxFrameSize.ui32Height;
            /* source: ITU-T H.264 2010/03, page 77 */
            //IMG_ASSERT(psSPSInfo->frame_crop_left_offset >= 0);
            IMG_ASSERT(psSPSInfo->frame_crop_left_offset <= (psComSequHdrInfo->sMaxFrameSize.ui32Width/i32CropUnitX)-(psSPSInfo->frame_crop_right_offset+1));
            //IMG_ASSERT(psSPSInfo->frame_crop_top_offset >= 0);
            IMG_ASSERT(psSPSInfo->frame_crop_top_offset <= (psComSequHdrInfo->sMaxFrameSize.ui32Height/i32CropUnitY)-(psSPSInfo->frame_crop_bottom_offset+1));
            i32FrameCropLeft = i32CropUnitX * psSPSInfo->frame_crop_left_offset;
            i32FrameCropRight = psComSequHdrInfo->sMaxFrameSize.ui32Width - (i32CropUnitX * psSPSInfo->frame_crop_right_offset);
            i32FrameCropTop = i32CropUnitY * psSPSInfo->frame_crop_top_offset;
            i32FrameCropBottom = psComSequHdrInfo->sMaxFrameSize.ui32Height - (i32CropUnitY * psSPSInfo->frame_crop_bottom_offset);
            psComSequHdrInfo->sOrigDisplayRegion.ui32LeftOffset = (IMG_UINT32)i32FrameCropLeft;
            psComSequHdrInfo->sOrigDisplayRegion.ui32TopOffset = (IMG_UINT32)i32FrameCropTop;
            psComSequHdrInfo->sOrigDisplayRegion.ui32Width = ( i32FrameCropRight - i32FrameCropLeft );
            psComSequHdrInfo->sOrigDisplayRegion.ui32Height =( i32FrameCropBottom - i32FrameCropTop );
        }
        else
        {
            psComSequHdrInfo->sOrigDisplayRegion.ui32LeftOffset = 0;
            psComSequHdrInfo->sOrigDisplayRegion.ui32TopOffset = 0;
            psComSequHdrInfo->sOrigDisplayRegion.ui32Width = psComSequHdrInfo->sMaxFrameSize.ui32Width;
            psComSequHdrInfo->sOrigDisplayRegion.ui32Height = psComSequHdrInfo->sMaxFrameSize.ui32Height;
        }
    }

    if (psSPSInfo->vui_parameters_present_flag && psVUIInfo->bitstream_restriction_flag)
    {
        psComSequHdrInfo->ui32MaxReorderPicts = psVUIInfo->max_dec_frame_buffering;
    }
    else
    {
        psComSequHdrInfo->ui32MaxReorderPicts = 0;
    }
    psComSequHdrInfo->bSeparateChromaPlanes = psH264SequHdrInfo->sSPSInfo.separate_colour_plane_flag ? IMG_TRUE : IMG_FALSE;
}


/*!
******************************************************************************

 @Function              bspp_H264PictHdrPopulate

******************************************************************************/
static IMG_VOID bspp_H264PictHdrPopulate(
    H264_eNalUnitType         eNalUnitType,
    BSPP_sH264SliceHdrInfo  * psH264SliceHdrInfo,
    VDEC_sComSequHdrInfo    * psComSequHdrInfo,
    BSPP_sPictHdrInfo       * psPictHdrInfo
)
{
    psPictHdrInfo->bIntraCoded = (eNalUnitType == H264_NALTYPE_IDR_SLICE)
                                    ? IMG_TRUE : IMG_FALSE;     // H264 has slice coding type, not picture. The bReference contrary to the rest of the
                                                                // standards is set explicitly from the NAL externally (see just below the call to
                                                                // bspp_H264PictHdrPopulate)
    //psPictHdrInfo->bReference = ?                             // Set externally for H264
    psPictHdrInfo->bField = psH264SliceHdrInfo->field_pic_flag;
    //psPictHdrInfo->bEmulationPrevention = ?                   // This needs to be setup by the generic bspp code (standard independent)
    psPictHdrInfo->bPostProcessing = IMG_FALSE;
    //psPictHdrInfo->bDiscontinuousMbs = ?                      // Set only after parsing multiple slices, as a combination of first_mb_in_slice flags, accounting for different fields too, set externally
    //psPictHdrInfo->bFragmentedData = ?                        // This needs to be setup by the generic bspp code (standard independent)
    //psPictHdrInfo->ui32PicDataSize = ?                        // This needs to be setup by the generic bspp code (standard independent)
    psPictHdrInfo->sCodedFrameSize.ui32Width = psComSequHdrInfo->sMaxFrameSize.ui32Width;   //!< For H264 Maximum and Coded sizes are the same
    psPictHdrInfo->sCodedFrameSize.ui32Height = psComSequHdrInfo->sMaxFrameSize.ui32Height; //!< For H264 Maximum and Coded sizes are the same
    psPictHdrInfo->sDispInfo.sEncDispRegion = psComSequHdrInfo->sOrigDisplayRegion;         //!< For H264 Encoded Display size has been precomputed as part of the common sequence info
    psPictHdrInfo->sDispInfo.sDispRegion = psComSequHdrInfo->sOrigDisplayRegion;            //!< For H264 there is no resampling, so encoded and actual display regions are the same
    //psPictHdrInfo->sDispInfo.bTopFieldFirst = ?               // Set by the FW after the picture has been decoded
    //psPictHdrInfo->sDispInfo.bRepeatFirstField = ?            // This is set externally, using the SEI info
    //psPictHdrInfo->sDispInfo.ui32MaxFrmRepeat = ?             // This is set externally, using the SEI info
    psPictHdrInfo->sDispInfo.ui32NumPanScanWindows = 0;         // H264 does not have that
    IMG_MEMSET(psPictHdrInfo->sDispInfo.asPanScanWindows, 0, sizeof(psPictHdrInfo->sDispInfo.asPanScanWindows));
    //psPictHdrInfo->psPictAuxData = ?                          // This is set externally, refers to the FW PPS Data
    //psPictHdrInfo->ui32AuxId = ?                              // This is set externally, refers to the PPS Id
    //psPictHdrInfo->psPictSgmData = ?                          // Unsupported yet
    //psPictHdrInfo->ui32SgmId = ?                              // Unsupported yet
}


static IMG_RESULT
bspp_H264LogRawHrdParamInfo(
    BSPP_sH264HRDParamInfo * psHrdParam
)
{
    IMG_UINT32 i;

    BSPP_LOG_INT("sNALHRDParameters.ui32CPBCntMinus1", psHrdParam->ui32CPBCntMinus1);
    BSPP_LOG_INT("sNALHRDParameters.ui32BitRateScale", psHrdParam->ui32BitRateScale);
    BSPP_LOG_INT("sNALHRDParameters.ui32CPBSizeScale", psHrdParam->ui32CPBSizeScale);
    for (i = 0; i < VDEC_H264_MAXIMUMVALUEOFCPB_CNT; i++)
    {
        BSPP_LOG_INT("sNALHRDParameters.ui32BitRateValueMinus1", psHrdParam->pui32BitRateValueMinus1[i]);
    }
    for (i = 0; i < VDEC_H264_MAXIMUMVALUEOFCPB_CNT; i++)
    {
        BSPP_LOG_INT("sNALHRDParameters.ui32CPBSizeValueMinus1", psHrdParam->pui32CPBSizeValueMinus1[i]);
    }
    for (i = 0; i < VDEC_H264_MAXIMUMVALUEOFCPB_CNT; i++)
    {
        BSPP_LOG_INT("sNALHRDParameters.ui32CBRFlag", psHrdParam->pui8CBRFlag[i]);
    }
    BSPP_LOG_INT("sNALHRDParameters.ui32InitialCPBRemovalDelayLengthMinus1", psHrdParam->ui32InitialCPBRemovalDelayLengthMinus1);
    BSPP_LOG_INT("sNALHRDParameters.ui32CPBRemovalDelayLenghtMinus1", psHrdParam->ui32CPBRemovalDelayLenghtMinus1);
    BSPP_LOG_INT("sNALHRDParameters.ui32DPBOutputDelayLengthMinus1", psHrdParam->ui32DPBOutputDelayLengthMinus1);
    BSPP_LOG_INT("sNALHRDParameters.ui32TimeOffsetLength", psHrdParam->ui32TimeOffsetLength);

    return IMG_SUCCESS;
}



static IMG_RESULT
bspp_H264LogRawVUI(
    BSPP_sH264VUIInfo * psVUIInfo
)
{
    BSPP_LOG_INT("aspect_ratio_info_present_flag", psVUIInfo->aspect_ratio_info_present_flag);
    BSPP_LOG_INT("aspect_ratio_idc", psVUIInfo->aspect_ratio_idc);
    BSPP_LOG_INT("sar_width", psVUIInfo->sar_width);
    BSPP_LOG_INT("sar_height", psVUIInfo->sar_height);
    BSPP_LOG_INT("overscan_info_present_flag", psVUIInfo->overscan_info_present_flag);
    BSPP_LOG_INT("overscan_appropriate_flag", psVUIInfo->overscan_appropriate_flag);
    BSPP_LOG_INT("video_signal_type_present_flag", psVUIInfo->video_signal_type_present_flag);
    BSPP_LOG_INT("video_format", psVUIInfo->video_format);
    BSPP_LOG_INT("video_full_range_flag", psVUIInfo->video_full_range_flag);
    BSPP_LOG_INT("colour_description_present_flag", psVUIInfo->colour_description_present_flag);
    BSPP_LOG_INT("colour_primaries", psVUIInfo->colour_primaries);
    BSPP_LOG_INT("transfer_characteristics", psVUIInfo->transfer_characteristics);
    BSPP_LOG_INT("matrix_coefficients", psVUIInfo->matrix_coefficients);
    BSPP_LOG_INT("chroma_location_info_present_flag", psVUIInfo->chroma_location_info_present_flag);
    BSPP_LOG_INT("chroma_sample_loc_type_top_field", psVUIInfo->chroma_sample_loc_type_top_field);
    BSPP_LOG_INT("chroma_sample_loc_type_bottom_field", psVUIInfo->chroma_sample_loc_type_bottom_field);
    BSPP_LOG_INT("timing_info_present_flag", psVUIInfo->timing_info_present_flag);
    BSPP_LOG_INT("num_units_in_tick", psVUIInfo->num_units_in_tick);
    BSPP_LOG_INT("time_scale", psVUIInfo->time_scale);
    BSPP_LOG_INT("fixed_frame_rate_flag", psVUIInfo->fixed_frame_rate_flag);

    // NAL HRD Parameters.
    BSPP_LOG_INT("nal_hrd_parameters_present_flag", psVUIInfo->nal_hrd_parameters_present_flag);
    bspp_H264LogRawHrdParamInfo(&psVUIInfo->sNALHRDParameters);

    // VLC HRD Parameters.
    BSPP_LOG_INT("vcl_hrd_parameters_present_flag", psVUIInfo->vcl_hrd_parameters_present_flag);
    bspp_H264LogRawHrdParamInfo(&psVUIInfo->sVCLHRDParameters);

    BSPP_LOG_INT("low_delay_hrd_flag", psVUIInfo->low_delay_hrd_flag);
    BSPP_LOG_INT("pic_struct_present_flag", psVUIInfo->pic_struct_present_flag);
    BSPP_LOG_INT("bitstream_restriction_flag", psVUIInfo->bitstream_restriction_flag);
    BSPP_LOG_INT("motion_vectors_over_pic_boundaries_flag", psVUIInfo->motion_vectors_over_pic_boundaries_flag);
    BSPP_LOG_INT("max_bytes_per_pic_denom", psVUIInfo->max_bytes_per_pic_denom);
    BSPP_LOG_INT("max_bits_per_mb_denom", psVUIInfo->max_bits_per_mb_denom);
    BSPP_LOG_INT("log2_max_mv_length_vertical", psVUIInfo->log2_max_mv_length_vertical);
    BSPP_LOG_INT("log2_max_mv_length_horizontal", psVUIInfo->log2_max_mv_length_horizontal);
    BSPP_LOG_INT("num_reorder_frames", psVUIInfo->num_reorder_frames);
    BSPP_LOG_INT("max_dec_frame_buffering", psVUIInfo->max_dec_frame_buffering);

    return IMG_SUCCESS;
}



static IMG_RESULT
bspp_H264LogRawSPS(
    BSPP_sH264SPSInfo * psSPSInfo
)
{
    IMG_UINT32 i, j;
    BSPP_LOG_DECLARE(IMG_UINT8 (*ScalingList4x4_seq)[H264FW_NUM_4X4_LISTS][H264FW_4X4_SIZE] = (IMG_UINT8 (*)[H264FW_NUM_4X4_LISTS][H264FW_4X4_SIZE])psSPSInfo->pui8ScalingList4x4_seq);
    BSPP_LOG_DECLARE(IMG_UINT8 (*ScalingList8x8_seq)[H264FW_NUM_8X8_LISTS][H264FW_8X8_SIZE] = (IMG_UINT8 (*)[H264FW_NUM_8X8_LISTS][H264FW_8X8_SIZE])psSPSInfo->pui8ScalingList8x8_seq);

    BSPP_LOG_INT("profile_idc", psSPSInfo->profile_idc);
    BSPP_LOG_INT("constraint_set_flags", psSPSInfo->constraint_set_flags);
    BSPP_LOG_INT("level_idc", psSPSInfo->level_idc);
    BSPP_LOG_INT("seq_parameter_set_id", psSPSInfo->seq_parameter_set_id);
    BSPP_LOG_INT("chroma_format_idc", psSPSInfo->chroma_format_idc);
    BSPP_LOG_INT("separate_colour_plane_flag", psSPSInfo->separate_colour_plane_flag);
    BSPP_LOG_INT("bit_depth_luma_minus8", psSPSInfo->bit_depth_luma_minus8);
    BSPP_LOG_INT("bit_depth_chroma_minus8", psSPSInfo->bit_depth_chroma_minus8);
    BSPP_LOG_INT("qpprime_y_zero_transform_bypass_flag", psSPSInfo->qpprime_y_zero_transform_bypass_flag);
    BSPP_LOG_INT("seq_scaling_matrix_present_flag", psSPSInfo->seq_scaling_matrix_present_flag);

    for (i = 0; i < 12; i++)
    {
        BSPP_LOG_INT("seq_scaling_list_present_flag", psSPSInfo->seq_scaling_list_present_flag[i]);
    }

    BSPP_LOG_INT("log2_max_frame_num_minus4", psSPSInfo->log2_max_frame_num_minus4);
    BSPP_LOG_INT("pic_order_cnt_type", psSPSInfo->pic_order_cnt_type);
    BSPP_LOG_INT("log2_max_pic_order_cnt_lsb_minus4", psSPSInfo->log2_max_pic_order_cnt_lsb_minus4);
    BSPP_LOG_INT("delta_pic_order_always_zero_flag", psSPSInfo->delta_pic_order_always_zero_flag);
    BSPP_LOG_INT("offset_for_non_ref_pic", psSPSInfo->offset_for_non_ref_pic);
    BSPP_LOG_INT("offset_for_top_to_bottom_field", psSPSInfo->offset_for_top_to_bottom_field);
    BSPP_LOG_INT("num_ref_frames_in_pic_order_cnt_cycle", psSPSInfo->num_ref_frames_in_pic_order_cnt_cycle);

    for (i = 0; i < 256; i++)
    {
        BSPP_LOG_INT("offset_for_ref_frame", psSPSInfo->pui32offset_for_ref_frame[i]);
    }

    BSPP_LOG_INT("max_num_ref_frames", psSPSInfo->max_num_ref_frames);
    BSPP_LOG_INT("gaps_in_frame_num_value_allowed_flag", psSPSInfo->gaps_in_frame_num_value_allowed_flag);
    BSPP_LOG_INT("pic_width_in_mbs_minus1", psSPSInfo->pic_width_in_mbs_minus1);
    BSPP_LOG_INT("pic_height_in_map_units_minus1", psSPSInfo->pic_height_in_map_units_minus1);
    BSPP_LOG_INT("frame_mbs_only_flag", psSPSInfo->frame_mbs_only_flag);
    BSPP_LOG_INT("mb_adaptive_frame_field_flag", psSPSInfo->mb_adaptive_frame_field_flag);
    BSPP_LOG_INT("direct_8x8_inference_flag", psSPSInfo->direct_8x8_inference_flag);
    BSPP_LOG_INT("frame_cropping_flag", psSPSInfo->frame_cropping_flag);
    BSPP_LOG_INT("frame_crop_left_offset", psSPSInfo->frame_crop_left_offset);
    BSPP_LOG_INT("frame_crop_right_offset", psSPSInfo->frame_crop_right_offset);
    BSPP_LOG_INT("frame_crop_top_offset", psSPSInfo->frame_crop_top_offset);
    BSPP_LOG_INT("frame_crop_bottom_offset", psSPSInfo->frame_crop_bottom_offset);
    BSPP_LOG_INT("vui_parameters_present_flag", psSPSInfo->vui_parameters_present_flag);

    BSPP_LOG_INT("bMVCVUIParameterPresentFlag", psSPSInfo->bMVCVUIParameterPresentFlag);

    for (i = 0; i < H264FW_NUM_4X4_LISTS; i++)
    {
        for (j = 0; j < H264FW_4X4_SIZE; j++)
        {
            BSPP_LOG_INT("ScalingList4x4_seq", (*ScalingList4x4_seq)[i][j]);
        }
    }

    for (i = 0; i < H264FW_NUM_8X8_LISTS; i++)
    {
        for (j = 0; j < H264FW_8X8_SIZE; j++)
        {
            BSPP_LOG_INT("ScalingList8x8_seq", (*ScalingList8x8_seq)[i][j]);
        }
    }

    for (i = 0; i < 12; i++)
    {
        BSPP_LOG_INT("UseDefaultScalingMatrixFlag_seq", psSPSInfo->UseDefaultScalingMatrixFlag_seq[i]);
    }

    return IMG_SUCCESS;
}



static IMG_RESULT
bspp_H264LogRawSequHdrInfo(
    BSPP_sH264SequHdrInfo * psH264SequHdrInfo
)
{
    bspp_H264LogRawSPS(&psH264SequHdrInfo->sSPSInfo);

    bspp_H264LogRawVUI(&psH264SequHdrInfo->sVUIInfo);


    return IMG_SUCCESS;
}



static IMG_RESULT
bspp_H264LogFwPPSInfo(
    const H264FW_sPicturePS * psH264FwPps
)
{
    IMG_UINT32 i, j;

    BSPP_LOG_PRINT("%s%s", ">>>>>>>>>>>>>> PPS (FW)\n", "");

    BSPP_LOG_INT("deblocking_filter_control_present_flag", psH264FwPps->deblocking_filter_control_present_flag);
    BSPP_LOG_INT("transform_8x8_mode_flag", psH264FwPps->transform_8x8_mode_flag);
    BSPP_LOG_INT("entropy_coding_mode_flag", psH264FwPps->entropy_coding_mode_flag);
    BSPP_LOG_INT("redundant_pic_cnt_present_flag", psH264FwPps->redundant_pic_cnt_present_flag);

    BSPP_LOG_INT("weighted_bipred_idc", psH264FwPps->weighted_bipred_idc);
    BSPP_LOG_INT("weighted_pred_flag", psH264FwPps->weighted_pred_flag);
    BSPP_LOG_INT("pic_order_present_flag", psH264FwPps->pic_order_present_flag);

    BSPP_LOG_INT("pic_init_qp", psH264FwPps->pic_init_qp);
    BSPP_LOG_INT("constrained_intra_pred_flag", psH264FwPps->constrained_intra_pred_flag);
    for (i = 0; i < H264FW_MAX_REFPIC_LISTS; i++)
    {
        BSPP_LOG_INT("num_ref_lX_active_minus1", psH264FwPps->num_ref_lX_active_minus1[i]);
    }

    BSPP_LOG_INT("slice_group_map_type", psH264FwPps->slice_group_map_type);
    BSPP_LOG_INT("num_slice_groups_minus1", psH264FwPps->num_slice_groups_minus1);
    BSPP_LOG_INT("slice_group_change_rate_minus1", psH264FwPps->slice_group_change_rate_minus1);

    BSPP_LOG_INT("chroma_qp_index_offset", psH264FwPps->chroma_qp_index_offset);
    BSPP_LOG_INT("second_chroma_qp_index_offset", psH264FwPps->second_chroma_qp_index_offset);

    for (i = 0; i < H264FW_NUM_4X4_LISTS; i++)
    {
        for (j = 0; j < H264FW_4X4_SIZE; j++)
        {
            BSPP_LOG_INT("ScalingList4x4", psH264FwPps->ScalingList4x4[i][j]);
        }
    }

    for (i = 0; i < H264FW_NUM_8X8_LISTS; i++)
    {
        for (j = 0; j < H264FW_8X8_SIZE; j++)
        {
            BSPP_LOG_INT("ScalingList8x8", psH264FwPps->ScalingList8x8[i][j]);
        }
    }

    return IMG_SUCCESS;
}



static IMG_RESULT
bspp_H264LogRawPPSInfo(
    BSPP_sH264PPSInfo * psH264Pps
)
{
    IMG_UINT32 i, j;
    BSPP_LOG_DECLARE(IMG_UINT8 (*ScalingList4x4_pic)[H264FW_NUM_4X4_LISTS][H264FW_4X4_SIZE] = (IMG_UINT8 (*)[H264FW_NUM_4X4_LISTS][H264FW_4X4_SIZE])psH264Pps->pui8ScalingList4x4_pic);
    BSPP_LOG_DECLARE(IMG_UINT8 (*ScalingList8x8_pic)[H264FW_NUM_8X8_LISTS][H264FW_8X8_SIZE] = (IMG_UINT8 (*)[H264FW_NUM_8X8_LISTS][H264FW_8X8_SIZE])psH264Pps->pui8ScalingList8x8_pic);

    BSPP_LOG_PRINT("%s%s", ">>>>>>>>>>>>>> PPS (RAW)\n", "");

    BSPP_LOG_INT("pic_parameter_set_id", psH264Pps->pic_parameter_set_id);
    BSPP_LOG_INT("seq_parameter_set_id", psH264Pps->seq_parameter_set_id);
    BSPP_LOG_INT("entropy_coding_mode_flag", psH264Pps->entropy_coding_mode_flag);
    BSPP_LOG_INT("pic_order_present_flag", psH264Pps->pic_order_present_flag);
    BSPP_LOG_INT("num_slice_groups_minus1", psH264Pps->num_slice_groups_minus1);
    BSPP_LOG_INT("slice_group_map_type", psH264Pps->slice_group_map_type);

    for (i = 0; i < 8; i++)
    {
        BSPP_LOG_INT("run_length_minus1", psH264Pps->run_length_minus1[i]);
    }

    for (i = 0; i < 8; i++)
    {
        BSPP_LOG_INT("top_left", psH264Pps->top_left[i]);
        BSPP_LOG_INT("bottom_right", psH264Pps->bottom_right[i]);
    }

    BSPP_LOG_INT("slice_group_change_direction_flag", psH264Pps->slice_group_change_direction_flag);
    BSPP_LOG_INT("slice_group_change_rate_minus1", psH264Pps->slice_group_change_rate_minus1);
    BSPP_LOG_INT("pic_size_in_map_unit", psH264Pps->pic_size_in_map_unit);

    for (i = 0; i < psH264Pps->sH264PPSSGMInfo.ui16SliceGroupIdNum; i++)
    {
        BSPP_LOG_INT("slice_group_id", psH264Pps->sH264PPSSGMInfo.slice_group_id[i]);
    }

    for (i = 0; i < H264FW_MAX_REFPIC_LISTS; i++)
    {
        BSPP_LOG_INT("num_ref_idx_lX_active_minus1", psH264Pps->num_ref_idx_lX_active_minus1[i]);
    }

    BSPP_LOG_INT("weighted_pred_flag", psH264Pps->weighted_pred_flag);
    BSPP_LOG_INT("weighted_bipred_idc", psH264Pps->weighted_bipred_idc);
    BSPP_LOG_INT("pic_init_qp_minus26", psH264Pps->pic_init_qp_minus26);
    BSPP_LOG_INT("pic_init_qs_minus26", psH264Pps->pic_init_qs_minus26);
    BSPP_LOG_INT("chroma_qp_index_offset", psH264Pps->chroma_qp_index_offset);
    BSPP_LOG_INT("deblocking_filter_control_present_flag", psH264Pps->deblocking_filter_control_present_flag);
    BSPP_LOG_INT("constrained_intra_pred_flag", psH264Pps->constrained_intra_pred_flag);
    BSPP_LOG_INT("redundant_pic_cnt_present_flag", psH264Pps->redundant_pic_cnt_present_flag);
    BSPP_LOG_INT("transform_8x8_mode_flag", psH264Pps->transform_8x8_mode_flag);
    BSPP_LOG_INT("pic_scaling_matrix_present_flag", psH264Pps->pic_scaling_matrix_present_flag);

    for (i = 0; i < 12; i++)
    {
        BSPP_LOG_INT("pic_scaling_list_present_flag", psH264Pps->pic_scaling_list_present_flag[i]);
    }

    BSPP_LOG_INT("second_chroma_qp_index_offset", psH264Pps->second_chroma_qp_index_offset);

    for (i = 0; i < H264FW_NUM_4X4_LISTS; i++)
    {
        for (j = 0; j < H264FW_4X4_SIZE; j++)
        {
            BSPP_LOG_INT("ScalingList4x4_pic", (*ScalingList4x4_pic)[i][j]);
        }
    }

    for (i = 0; i < H264FW_NUM_8X8_LISTS; i++)
    {
        for (j = 0; j < H264FW_8X8_SIZE; j++)
        {
            BSPP_LOG_INT("ScalingList8x8_pic", (*ScalingList8x8_pic)[i][j]);
        }
    }

    for (i = 0; i < 12; i++)
    {
        BSPP_LOG_INT("UseDefaultScalingMatrixFlag_pic", psH264Pps->UseDefaultScalingMatrixFlag_pic[i]);
    }

    return IMG_SUCCESS;
}



/*!
******************************************************************************

 @Function              BSPP_H264LogPpsInfo

******************************************************************************/
IMG_RESULT
BSPP_H264LogPpsInfo(
    const H264FW_sPicturePS * psFwPps,
    const IMG_HANDLE          hSecurePpsInfo
)
{
    IMG_UINT32 ui32Result;

    if (hSecurePpsInfo)
    {
        ui32Result = bspp_H264LogRawPPSInfo(hSecurePpsInfo);
        IMG_ASSERT(ui32Result == IMG_SUCCESS);
        if (ui32Result != IMG_SUCCESS)
        {
            goto error;
        }
    }

    ui32Result = bspp_H264LogFwPPSInfo(psFwPps);
    IMG_ASSERT(ui32Result == IMG_SUCCESS);
    if (ui32Result != IMG_SUCCESS)
    {
        goto error;
    }

    return IMG_SUCCESS;

error:
    return ui32Result;
}

/*!
******************************************************************************

 @Function              BSPP_H264DestroySequHdrInfo

******************************************************************************/
IMG_RESULT BSPP_H264DestroySequHdrInfo(
    const IMG_HANDLE      hSecureSPSInfo
)
{
    BSPP_sH264SequHdrInfo * psH264SequHdrInfo = IMG_NULL;

    if (IMG_NULL == hSecureSPSInfo)
    {
        return IMG_ERROR_INVALID_PARAMETERS;
    }

    psH264SequHdrInfo = (BSPP_sH264SequHdrInfo *)hSecureSPSInfo;

    //Cleaning sSPSMVCExtInfo
    if(psH264SequHdrInfo->sSPSMVCExtInfo.pui16AnchorRefIndicies1X)
    {
        IMG_FREE(psH264SequHdrInfo->sSPSMVCExtInfo.pui16AnchorRefIndicies1X);
        psH264SequHdrInfo->sSPSMVCExtInfo.pui16AnchorRefIndicies1X = IMG_NULL;
    }
    if(psH264SequHdrInfo->sSPSMVCExtInfo.pui16NonAnchorRefIndicies1X)
    {
        IMG_FREE(psH264SequHdrInfo->sSPSMVCExtInfo.pui16NonAnchorRefIndicies1X);
        psH264SequHdrInfo->sSPSMVCExtInfo.pui16NonAnchorRefIndicies1X = IMG_NULL;
    }

    //Cleaning sVUIInfo
    if(psH264SequHdrInfo->sVUIInfo.sNALHRDParameters.pui32BitRateValueMinus1)
    {
        IMG_FREE(psH264SequHdrInfo->sVUIInfo.sNALHRDParameters.pui32BitRateValueMinus1);
        psH264SequHdrInfo->sVUIInfo.sNALHRDParameters.pui32BitRateValueMinus1 = IMG_NULL;
    }
    if(psH264SequHdrInfo->sVUIInfo.sNALHRDParameters.pui32CPBSizeValueMinus1)
    {
        IMG_FREE(psH264SequHdrInfo->sVUIInfo.sNALHRDParameters.pui32CPBSizeValueMinus1);
        psH264SequHdrInfo->sVUIInfo.sNALHRDParameters.pui32CPBSizeValueMinus1 = IMG_NULL;
    }
    if(psH264SequHdrInfo->sVUIInfo.sNALHRDParameters.pui8CBRFlag)
    {
        IMG_FREE(psH264SequHdrInfo->sVUIInfo.sNALHRDParameters.pui8CBRFlag);
        psH264SequHdrInfo->sVUIInfo.sNALHRDParameters.pui8CBRFlag = IMG_NULL;
    }
    if(psH264SequHdrInfo->sVUIInfo.sVCLHRDParameters.pui32BitRateValueMinus1)
    {
        IMG_FREE(psH264SequHdrInfo->sVUIInfo.sVCLHRDParameters.pui32BitRateValueMinus1);
        psH264SequHdrInfo->sVUIInfo.sVCLHRDParameters.pui32BitRateValueMinus1 = IMG_NULL;
    }
    if(psH264SequHdrInfo->sVUIInfo.sVCLHRDParameters.pui32CPBSizeValueMinus1)
    {
        IMG_FREE(psH264SequHdrInfo->sVUIInfo.sVCLHRDParameters.pui32CPBSizeValueMinus1);
        psH264SequHdrInfo->sVUIInfo.sVCLHRDParameters.pui32CPBSizeValueMinus1 = IMG_NULL;
    }
    if(psH264SequHdrInfo->sVUIInfo.sVCLHRDParameters.pui8CBRFlag)
    {
        IMG_FREE(psH264SequHdrInfo->sVUIInfo.sVCLHRDParameters.pui8CBRFlag);
        psH264SequHdrInfo->sVUIInfo.sVCLHRDParameters.pui8CBRFlag = IMG_NULL;
    }

    //Cleaning sSPSInfo
    if(psH264SequHdrInfo->sSPSInfo.pui32offset_for_ref_frame)
    {
        IMG_FREE(psH264SequHdrInfo->sSPSInfo.pui32offset_for_ref_frame);
        psH264SequHdrInfo->sSPSInfo.pui32offset_for_ref_frame = IMG_NULL;
    }
    if(psH264SequHdrInfo->sSPSInfo.pui8ScalingList4x4_seq)
    {
        IMG_FREE(psH264SequHdrInfo->sSPSInfo.pui8ScalingList4x4_seq);
        psH264SequHdrInfo->sSPSInfo.pui8ScalingList4x4_seq = IMG_NULL;
    }
    if(psH264SequHdrInfo->sSPSInfo.pui8ScalingList8x8_seq)
    {
        IMG_FREE(psH264SequHdrInfo->sSPSInfo.pui8ScalingList8x8_seq);
        psH264SequHdrInfo->sSPSInfo.pui8ScalingList8x8_seq = IMG_NULL;
    }
    return IMG_SUCCESS;
}

/*!
******************************************************************************

 @Function              BSPP_H264DestroyPPSInfo

******************************************************************************/
IMG_RESULT BSPP_H264DestroyPPSInfo(
    const IMG_HANDLE      hSecurePPSInfo
)
{
    BSPP_sH264PPSInfo * psH264PPSInfo = IMG_NULL;

    if (IMG_NULL == hSecurePPSInfo)
    {
        return IMG_ERROR_INVALID_PARAMETERS;
    }

    psH264PPSInfo = (BSPP_sH264PPSInfo *)hSecurePPSInfo;
    if(psH264PPSInfo->sH264PPSSGMInfo.slice_group_id)
    {
        IMG_FREE(psH264PPSInfo->sH264PPSSGMInfo.slice_group_id);
        psH264PPSInfo->sH264PPSSGMInfo.slice_group_id = IMG_NULL;
        psH264PPSInfo->sH264PPSSGMInfo.ui16SliceGroupIdNum = 0;
    }

    if(psH264PPSInfo->pui8ScalingList4x4_pic)
    {
        IMG_FREE(psH264PPSInfo->pui8ScalingList4x4_pic);
        psH264PPSInfo->pui8ScalingList4x4_pic = IMG_NULL;
    }
    if(psH264PPSInfo->pui8ScalingList8x8_pic)
    {
        IMG_FREE(psH264PPSInfo->pui8ScalingList8x8_pic);
        psH264PPSInfo->pui8ScalingList8x8_pic = IMG_NULL;
    }
    return IMG_SUCCESS;
}

/*!
******************************************************************************

 @Function              BSPP_H264LogSequHdrInfo

******************************************************************************/
IMG_RESULT
BSPP_H264LogSequHdrInfo(
    const BSPP_sSequHdrInfo * psSequHdrInfo,
    const IMG_HANDLE          hSecureSequenceInfo
)
{
    IMG_UINT32 ui32Result;

    if (hSecureSequenceInfo)
    {
        // Log the raw sequence information parsed from the bistream.
        ui32Result = bspp_H264LogRawSequHdrInfo(hSecureSequenceInfo);
        IMG_ASSERT(ui32Result == IMG_SUCCESS);
        if (ui32Result != IMG_SUCCESS)
        {
            goto error;
        }
    }

    //if (psSequHdrInfo)
    //{
    //    // Log the sequence information prepared for the firmware.
    //    ui32Result = bspp_H264LogFwSequHdrInfo(psSequHdrInfo->sFWSequence.pvCpuLinearAddr);
    //    IMG_ASSERT(ui32Result == IMG_SUCCESS);
    //    if (ui32Result != IMG_SUCCESS)
    //    {
    //        goto error;
    //    }
    //}

    return IMG_SUCCESS;

error:
    return ui32Result;
}


/*
******************************************************************************

 @Function              bspp_IsH264StreamX264

******************************************************************************/
static IMG_BOOL bspp_IsH264StreamX264(
    IMG_CHAR *  user_data_payload_byte
)
{
    static const IMG_CHAR X264[] = "X264";
    static const IMG_CHAR x264[] = "x264";
    return ( (strncmp(user_data_payload_byte, X264, 4) == 0) || (strncmp(user_data_payload_byte, x264, 4) == 0) );
}

/*
******************************************************************************

 @Function              bspp_IsH264StreamJM182

*****************************************************************************/
static IMG_BOOL bspp_IsH264StreamJM182(
    IMG_CHAR *  user_data_payload_byte
)
{
    IMG_BOOL bGenuineX264;
    IMG_BOOL bGenuineJM18;
    IMG_BOOL bDiamondInJM18Mode;

    static const IMG_CHAR JM182[] =           "H.264/AVC Encoder JM18.2";
    static const IMG_CHAR DiamondJM18Mode[] = "IMG: Cabac 4:4:4 4x4 transform MB not available";

   // when the stream is x264 is is considered also like JM18 which mean "b4x4TransformMBNotAvailable" must be set.
   // In both JM18 and x264 the alternative version of CABAC shall be used.
    bGenuineX264         = bspp_IsH264StreamX264( user_data_payload_byte );
    bGenuineJM18         = (strncmp( user_data_payload_byte, JM182          , 24 ) == 0);
    bDiamondInJM18Mode   = (strncmp( user_data_payload_byte, DiamondJM18Mode, 42 ) == 0);
    return( bGenuineX264 || bDiamondInJM18Mode || bGenuineJM18);
}

/*
******************************************************************************

 @Function              bspp_H264UnitParser_CheckFMO

*****************************************************************************/
static IMG_BOOL bspp_H264UnitParser_CheckFMO(
    IMG_PUINT8      pui8SGMBuffer,
    IMG_UINT32      ui32SGMMapSize
)
{
    IMG_UINT32 i =0;
    IMG_BOOL bReturn = IMG_FALSE;
    IMG_ASSERT( pui8SGMBuffer!= IMG_NULL);
    IMG_ASSERT(ui32SGMMapSize);
    for( i =0; i<(ui32SGMMapSize-1);i++)
    {
        if(pui8SGMBuffer[i] > pui8SGMBuffer[i+1])
        {
            bReturn = IMG_TRUE;
            break;
        }
    }
    return bReturn;
}


/*
******************************************************************************

 @Function              bspp_H264GenerateSliceGroupMap

*****************************************************************************/
static IMG_VOID bspp_H264GenerateSliceGroupMap(
    BSPP_sH264SliceHdrInfo  * psH264SliceHdrInfo,
    BSPP_sH264SequHdrInfo   * psH264SequHdrInfo,
    BSPP_sH264PPSInfo       * psH264PPSInfo,
    IMG_UINT8*  pui8MapUnitToSliceGroupMap,
    IMG_UINT32 ui32MapSize
)
{
    IMG_INT32   i32Group;
    IMG_UINT32 ui32NumSliceGroupMapUnits;
    IMG_UINT32 i = 0 ,j ,k=0;

    ui32NumSliceGroupMapUnits   =    ui32MapSize ;
    if (psH264PPSInfo->slice_group_map_type == 6)
    {
        if ((IMG_UINT32)(psH264PPSInfo->num_slice_groups_minus1+1) != ui32NumSliceGroupMapUnits)
        {
            IMG_ASSERT ("wrong pps->num_slice_group_map_units_minus1 for used SPS and FMO type 6");
            if(ui32NumSliceGroupMapUnits > psH264PPSInfo->sH264PPSSGMInfo.ui16SliceGroupIdNum)
            {
                ui32NumSliceGroupMapUnits = psH264PPSInfo->sH264PPSSGMInfo.ui16SliceGroupIdNum;
            }
        }
    }


    if (psH264PPSInfo->num_slice_groups_minus1 == 0)    // only one slice group
    {
        IMG_MEMSET (pui8MapUnitToSliceGroupMap, 0, ui32MapSize * sizeof (IMG_UINT8));
        return ;
    }
    if ( psH264PPSInfo->num_slice_groups_minus1  >= MAX_SLICEGROUP_COUNT )
    {
        IMG_MEMSET (pui8MapUnitToSliceGroupMap, 0, ui32MapSize * sizeof (IMG_UINT8));
        return ;
    }
    if(psH264PPSInfo->slice_group_map_type == 0)
    {
        do
            {
            for( i32Group = 0;
                 (i32Group <= psH264PPSInfo->num_slice_groups_minus1) && (i < ui32NumSliceGroupMapUnits);
                 i += psH264PPSInfo->run_length_minus1[i32Group++] + 1 )
            {
              for( j = 0; j <= psH264PPSInfo->run_length_minus1[ i32Group ] && i + j < ui32NumSliceGroupMapUnits; j++ )
                    pui8MapUnitToSliceGroupMap[i+j] = i32Group;
            }
        } while( i < ui32NumSliceGroupMapUnits );

    }
    else if(psH264PPSInfo->slice_group_map_type == 1)
    {
        for( i = 0; i < ui32NumSliceGroupMapUnits; i++ )
        {

            pui8MapUnitToSliceGroupMap[i] = ((i%(psH264SequHdrInfo->sSPSInfo.pic_width_in_mbs_minus1+1))+(((i/(psH264SequHdrInfo->sSPSInfo.pic_width_in_mbs_minus1+1))*(psH264PPSInfo->num_slice_groups_minus1+1))/2))
                                    %(psH264PPSInfo->num_slice_groups_minus1+1);
        }
    }
    else if(psH264PPSInfo->slice_group_map_type == 2)
    {
        IMG_UINT32 yTopLeft, xTopLeft, yBottomRight, xBottomRight ,x,y;
        for( i = 0; i <  ui32NumSliceGroupMapUnits; i++ )
        pui8MapUnitToSliceGroupMap[i] = psH264PPSInfo->num_slice_groups_minus1;

        for( i32Group = psH264PPSInfo->num_slice_groups_minus1 - 1 ; i32Group >= 0; i32Group-- )
        {
            yTopLeft = psH264PPSInfo->top_left[ i32Group ] / (psH264SequHdrInfo->sSPSInfo.pic_width_in_mbs_minus1+1);
            xTopLeft = psH264PPSInfo->top_left[ i32Group ] % (psH264SequHdrInfo->sSPSInfo.pic_width_in_mbs_minus1+1);
            yBottomRight = psH264PPSInfo->bottom_right[ i32Group ] / (psH264SequHdrInfo->sSPSInfo.pic_width_in_mbs_minus1+1);
            xBottomRight = psH264PPSInfo->bottom_right[ i32Group ] % (psH264SequHdrInfo->sSPSInfo.pic_width_in_mbs_minus1+1);
            for( y = yTopLeft; y <= yBottomRight; y++ )
            for( x = xTopLeft; x <= xBottomRight; x++ )
            {
                  if ((psH264PPSInfo->top_left[i32Group] > psH264PPSInfo->bottom_right[i32Group]) || (psH264PPSInfo->bottom_right[i32Group] >= ui32NumSliceGroupMapUnits))
                      continue;
                pui8MapUnitToSliceGroupMap[ y * (psH264SequHdrInfo->sSPSInfo.pic_width_in_mbs_minus1+1) + x ] = i32Group;
            }
        }
    }
    else if(psH264PPSInfo->slice_group_map_type == 3)
    {
        IMG_INT32 leftBound, topBound, rightBound, bottomBound;
        IMG_INT32 x, y, xDir, yDir;
        IMG_INT32 mapUnitVacant;

        IMG_UINT32 mapUnitsInSliceGroup0 = MIN ((IMG_UINT32)((psH264PPSInfo->slice_group_change_rate_minus1 + 1) * psH264SliceHdrInfo->slice_group_change_cycle), (IMG_UINT32)ui32NumSliceGroupMapUnits);

        for( i = 0; i < ui32NumSliceGroupMapUnits; i++ )
         pui8MapUnitToSliceGroupMap[ i ] = 2;

        x = ( psH264SequHdrInfo->sSPSInfo.pic_width_in_mbs_minus1 +1 - psH264PPSInfo->slice_group_change_direction_flag ) / 2;
        y = ( psH264SequHdrInfo->sSPSInfo.pic_height_in_map_units_minus1 +1 - psH264PPSInfo->slice_group_change_direction_flag ) / 2;

        leftBound   = x;
        topBound    = y;
        rightBound  = x;
        bottomBound = y;

        xDir =  psH264PPSInfo->slice_group_change_direction_flag - 1;
        yDir =  psH264PPSInfo->slice_group_change_direction_flag;

        for( k = 0; k < ui32NumSliceGroupMapUnits; k += mapUnitVacant )
        {
            mapUnitVacant = ( pui8MapUnitToSliceGroupMap[ y * (psH264SequHdrInfo->sSPSInfo.pic_width_in_mbs_minus1 +1) + x ]  ==  2 );
            if( mapUnitVacant )
                pui8MapUnitToSliceGroupMap[ y * (psH264SequHdrInfo->sSPSInfo.pic_width_in_mbs_minus1 +1)  + x ] = ( k >= mapUnitsInSliceGroup0 );

            if( xDir  ==  -1  &&  x  ==  leftBound )
            {
                leftBound = MAX( leftBound - 1, 0 );
                x = leftBound;
                xDir = 0;
                yDir = 2 * psH264PPSInfo->slice_group_change_direction_flag - 1;
            }
            else
            if( xDir  ==  1  &&  x  ==  rightBound )
            {
                rightBound = MIN( rightBound + 1, (int)(psH264SequHdrInfo->sSPSInfo.pic_width_in_mbs_minus1 +1) - 1 );
                x = rightBound;
                xDir = 0;
                yDir = 1 - 2 * psH264PPSInfo->slice_group_change_direction_flag;
            }
            else
            if( yDir  ==  -1  &&  y  ==  topBound )
            {
                topBound = MAX( topBound - 1, 0 );
                y = topBound;
                xDir = 1 - 2 * psH264PPSInfo->slice_group_change_direction_flag;
                yDir = 0;
            }
            else
            if( yDir  ==  1  &&  y  ==  bottomBound )
            {
                bottomBound = MIN( bottomBound + 1, (int)(psH264SequHdrInfo->sSPSInfo.pic_height_in_map_units_minus1+1) - 1 );
                y = bottomBound;
                xDir = 2 * psH264PPSInfo->slice_group_change_direction_flag - 1;
                yDir = 0;
            }
            else
            {
                x = x + xDir;
                y = y + yDir;
            }
        }
    }
    else if(psH264PPSInfo->slice_group_map_type == 4)
    {
        IMG_UINT32 mapUnitsInSliceGroup0 = MIN((IMG_UINT32)((psH264PPSInfo->slice_group_change_rate_minus1 + 1) *
                                                                 psH264SliceHdrInfo->slice_group_change_cycle),
                                               (IMG_UINT32)ui32NumSliceGroupMapUnits);
        IMG_UINT32 sizeOfUpperLeftGroup = psH264PPSInfo->slice_group_change_direction_flag ?
                                              ( ui32NumSliceGroupMapUnits - mapUnitsInSliceGroup0 ) :
                                              mapUnitsInSliceGroup0;
         for( i = 0; i < ui32NumSliceGroupMapUnits; i++ )
            if( i < sizeOfUpperLeftGroup )
                pui8MapUnitToSliceGroupMap[ i ] = psH264PPSInfo->slice_group_change_direction_flag;
            else
                pui8MapUnitToSliceGroupMap[ i ] = 1 - psH264PPSInfo->slice_group_change_direction_flag;
    }
    else if(psH264PPSInfo->slice_group_map_type == 5)
    {
        IMG_UINT32 mapUnitsInSliceGroup0 = MIN((IMG_UINT32)((psH264PPSInfo->slice_group_change_rate_minus1 + 1) *
                                                                 psH264SliceHdrInfo->slice_group_change_cycle),
                                               (IMG_UINT32)ui32NumSliceGroupMapUnits);
        IMG_UINT32 sizeOfUpperLeftGroup = psH264PPSInfo->slice_group_change_direction_flag ?
                                              ( ui32NumSliceGroupMapUnits - mapUnitsInSliceGroup0 ) :
                                              mapUnitsInSliceGroup0;

        for( j = 0; j < (IMG_UINT32)(psH264SequHdrInfo->sSPSInfo.pic_width_in_mbs_minus1 +1); j++ )
            for( i = 0; i < (IMG_UINT32)(psH264SequHdrInfo->sSPSInfo.pic_height_in_map_units_minus1+1); i++ )
                if( k++ < sizeOfUpperLeftGroup )
                    pui8MapUnitToSliceGroupMap[ i * (psH264SequHdrInfo->sSPSInfo.pic_width_in_mbs_minus1 +1)+ j ] =
                                                                            psH264PPSInfo->slice_group_change_direction_flag;
                else
                    pui8MapUnitToSliceGroupMap[ i * (psH264SequHdrInfo->sSPSInfo.pic_width_in_mbs_minus1 +1) + j ] =
                                                                            1 - psH264PPSInfo->slice_group_change_direction_flag;

    }
    else if(psH264PPSInfo->slice_group_map_type == 6)
    {
        IMG_ASSERT(ui32NumSliceGroupMapUnits <= psH264PPSInfo->sH264PPSSGMInfo.ui16SliceGroupIdNum);
        for (i=0; i<ui32NumSliceGroupMapUnits; i++)
        {
            pui8MapUnitToSliceGroupMap[i] = psH264PPSInfo->sH264PPSSGMInfo.slice_group_id[i];
        }
    }

}

/*
******************************************************************************

 @Function              bspp_H264ParseMvcSlieceExtension

 @Description           returns true if slice extension is mvc, false otherwise

*****************************************************************************/
IMG_BOOL bspp_H264ParseMvcSlieceExtension(IMG_HANDLE hSwSrContext, BSPP_sH264InterPictCtx   * psInterPictCtx)
{
    if(!SWSR_ReadBits(hSwSrContext, 1))
    {
                                            SWSR_ReadBits(hSwSrContext, 7);
        psInterPictCtx->ui16CurrentViewId = SWSR_ReadBits(hSwSrContext, 10);
                                            SWSR_ReadBits(hSwSrContext, 6);

        return IMG_TRUE;
    }
    return IMG_FALSE;
}

/*
******************************************************************************

 @Function              bspp_H264UnitParser_CompileSGMData

*****************************************************************************/
static IMG_RESULT bspp_H264UnitParser_CompileSGMData(
    BSPP_sH264SliceHdrInfo  * psH264SliceHdrInfo,
    BSPP_sH264SequHdrInfo   * psH264SequHdrInfo,
    BSPP_sH264PPSInfo       * psH264PPSInfo,
    BSPP_sPictHdrInfo       * psPictHdrInfo
)
{
    IMG_MEMSET(&psPictHdrInfo->sPictSgmData, 0, sizeof(psPictHdrInfo->sPictSgmData));

    psPictHdrInfo->sPictSgmData.ui32Id = 1;

    /* Allocate memory for SGM. */
    psPictHdrInfo->sPictSgmData.ui32Size = (psH264SequHdrInfo->sSPSInfo.pic_height_in_map_units_minus1 + 1) *
                                               (psH264SequHdrInfo->sSPSInfo.pic_width_in_mbs_minus1 + 1);

    psPictHdrInfo->sPictSgmData.pvData = IMG_MALLOC(psPictHdrInfo->sPictSgmData.ui32Size);
    IMG_ASSERT(psPictHdrInfo->sPictSgmData.pvData);
    if (psPictHdrInfo->sPictSgmData.pvData == IMG_NULL)
    {
        psPictHdrInfo->sPictSgmData.ui32Id = BSPP_INVALID;
        return IMG_ERROR_OUT_OF_MEMORY;
    }

    bspp_H264GenerateSliceGroupMap(psH264SliceHdrInfo,
                                   psH264SequHdrInfo,
                                   psH264PPSInfo,
                                   psPictHdrInfo->sPictSgmData.pvData,
                                   psPictHdrInfo->sPictSgmData.ui32Size);

    // check the bDiscontinuousMbsinCurrFrame flag for FMO
    psPictHdrInfo->bDiscontinuousMbs = bspp_H264UnitParser_CheckFMO(psPictHdrInfo->sPictSgmData.pvData,
                                                                    psPictHdrInfo->sPictSgmData.ui32Size);

    return IMG_SUCCESS;
}

/*!
******************************************************************************

 @Function              BSPP_H264UnitParser

******************************************************************************/
IMG_RESULT BSPP_H264UnitParser(
    IMG_HANDLE        hSwSrContext,
    BSPP_sUnitData  * psUnitData
)
{
    IMG_UINT32          ui32Result  = IMG_SUCCESS;
    BSPP_eErrorType     eParseError = BSPP_ERROR_NONE;
    H264_eNalUnitType   eNalUnitType;
    IMG_UINT8           nal_ref_idc;
    BSPP_sH264InterPictCtx   * psInterPictCtx;

    psInterPictCtx = &psUnitData->psParseState->psInterPictCtx->sH264Ctx;

    // At this point we should be EXACTLY at the NALTYPE byte
    // parse the nal header type
    SWSR_ReadBits( hSwSrContext, 1 );
    nal_ref_idc = SWSR_ReadBits( hSwSrContext, 2 );
    eNalUnitType = (H264_eNalUnitType)SWSR_ReadBits( hSwSrContext, 5 );

    switch (psUnitData->eUnitType)
    {
    case BSPP_UNIT_SEQUENCE:
        IMG_ASSERT(eNalUnitType == H264_NALTYPE_SEQUENCE_PARAMETER_SET || eNalUnitType == H264_NALTYPE_SUBSET_SPS );
        {
            // Parse SPS structure
            BSPP_sH264SequHdrInfo * psH264SequHdrInfo = (BSPP_sH264SequHdrInfo *)(psUnitData->out.psSequHdrInfo->hSecureSequenceInfo);
            // FW SPS Data structure
            H264FW_sSequencePS * psH264FWSequHdrInfo  = (H264FW_sSequencePS *)GET_ELEMENT_FROM_PACKED_DEVICE_BUFFER(&psUnitData->out.psSequHdrInfo->sFWSequence);

            // Common Sequence Header Info
            VDEC_sComSequHdrInfo * psComSequHdrInfo = &psUnitData->out.psSequHdrInfo->sSequHdrInfo.sComSequHdrInfo;                                     // Common Sequence Header Info

            DEBUG_REPORT(REPORT_MODULE_BSPP,"Unit Parser:Found SEQUENCE_PARAMETER_SET NAL unit");
            IMG_ASSERT(psH264SequHdrInfo);
            IMG_ASSERT(psH264FWSequHdrInfo);
            if(psH264SequHdrInfo == IMG_NULL)
            {
                return IMG_ERROR_ALREADY_COMPLETE;
            }
              if(psH264FWSequHdrInfo == IMG_NULL)
            {
                return IMG_ERROR_ALREADY_COMPLETE;
            }
            // Call SPS parser to populate the "Parse SPS Structure"
            psUnitData->eParseError |= bspp_H264SPSParser(hSwSrContext, psH264SequHdrInfo);
            // From "Parse SPS Structure" populate the "FW SPS Data Structure"
            bspp_H264FWSequHdrPopulate(psH264SequHdrInfo, psH264FWSequHdrInfo);
            // From "Parse SPS Structure" populate the "Common Sequence Header Info"
            bspp_H264CommonSequHdrPopulate(psH264SequHdrInfo, psComSequHdrInfo);
            // Set the SPS ID
            psUnitData->out.psSequHdrInfo->sSequHdrInfo.ui32SequHdrId = GET_SUBSET_ID(eNalUnitType, psH264SequHdrInfo->sSPSInfo.seq_parameter_set_id);
            // Set the first SPS ID as Active SPS ID for SEI parsing to cover the case of not having SeiBufferingPeriod to give us the SPS ID
            if (psInterPictCtx->ui32ActiveSpsForSeiParsing == BSPP_INVALID)
            {
                psInterPictCtx->ui32ActiveSpsForSeiParsing = psH264SequHdrInfo->sSPSInfo.seq_parameter_set_id;
            }
        }
        break;

    case BSPP_UNIT_PPS_H264:
        IMG_ASSERT(eNalUnitType == H264_NALTYPE_PICTURE_PARAMETER_SET);
        {
            // Parse PPS structure
            BSPP_sH264PPSInfo * psH264PPSInfo = (BSPP_sH264PPSInfo *)(psUnitData->out.psPPSInfo->hSecurePPSInfo);
            // FW PPS Data structure
            H264FW_sPicturePS * psH264FWPPSInfo = (H264FW_sPicturePS *)GET_ELEMENT_FROM_PACKED_DEVICE_BUFFER(&psUnitData->out.psPPSInfo->sFWPPS);

            DEBUG_REPORT(REPORT_MODULE_BSPP,"Unit Parser:Found PICTURE_PARAMETER_SET NAL unit");
            IMG_ASSERT(psH264PPSInfo);
            IMG_ASSERT(psH264FWPPSInfo);

            // Call PPS parser to populate the "Parse PPS Structure"
            psUnitData->eParseError |= bspp_H264PPSParser(hSwSrContext, psUnitData->hStrRes, psH264PPSInfo);
            // From "Parse PPS Structure" populate the "FW PPS Data Structure" - the scaling lists
            bspp_H264FWPPSPopulate(psH264PPSInfo, psH264FWPPSInfo);
            // Set the PPS ID
            psUnitData->out.psPPSInfo->ui32PPSId = psH264PPSInfo->pic_parameter_set_id;
        }
        break;

    case BSPP_UNIT_PICTURE:
        if (eNalUnitType == H264_NALTYPE_SLICE_PREFIX)
        {
            IMG_UINT16 ui32PrevViewID = psInterPictCtx->ui16CurrentViewId;
            if (bspp_H264ParseMvcSlieceExtension(hSwSrContext, psInterPictCtx))
            {
                //signal new view (never for base view)
                psUnitData->psParseState->bNewView = (ui32PrevViewID != psInterPictCtx->ui16CurrentViewId);
                psUnitData->psParseState->bIsPrefix = IMG_TRUE;
            }
        }
        else if (eNalUnitType == H264_NALTYPE_SLICE_SCALABLE ||
                 eNalUnitType == H264_NALTYPE_SLICE_IDR_SCALABLE ||
                 eNalUnitType == H264_NALTYPE_SLICE ||
                 eNalUnitType == H264_NALTYPE_IDR_SLICE)
        {
            BSPP_sH264SliceHdrInfo sH264SliceHdrInfo;
            BSPP_sH264PPSInfo *      psH264PPSInfo;        // Needed for parsing: Parse PPS structure
            BSPP_sPPSInfo * psPPSInfo;                                                                          // Returned: Parent PPS Structure
            H264FW_sPicturePS * psH264FWPPSInfo;                                                                // Need to transfer scaling lists into the returned: FW PPS Data structure
            H264FW_sSequencePS * psH264FWSequHdrInfo;                                                           // Needed for updating with SEI data.
            BSPP_sH264SequHdrInfo *  psH264SequHdrInfo;    // Needed for parsing: Parse SPS structure
            BSPP_sSequenceHdrInfo * psSequHdrInfo;                                                              // Returned: Parent SPS Structure
            IMG_BOOL bCurrentPicIsNew = IMG_FALSE;
            IMG_BOOL bDetermined = IMG_FALSE;

            DEBUG_REPORT(REPORT_MODULE_BSPP,"Unit Parser:Found PICTURE DATA unit");

            psUnitData->bSlice = IMG_TRUE;
            psUnitData->bExtSlice = IMG_FALSE;

            if (eNalUnitType == H264_NALTYPE_SLICE_SCALABLE || eNalUnitType == H264_NALTYPE_SLICE_IDR_SCALABLE)
            {
                IMG_UINT16 ui32PrevViewID = psInterPictCtx->ui16CurrentViewId;

                if(bspp_H264ParseMvcSlieceExtension(hSwSrContext, psInterPictCtx))
                {
                    //skip mvc extension, will be parsed in FW only
                    psUnitData->bExtSlice = IMG_TRUE;
                }
                else
                {
                    //SVC: do not parse
                    break;
                }

                //signal new view (never for base view)
                psUnitData->psParseState->bNewView = (ui32PrevViewID != psInterPictCtx->ui16CurrentViewId);

                if(psUnitData->psParseState->bNewView)
                {
                    if(psUnitData->psParseState->psNextPictHdrInfo == IMG_NULL)
                    {
                        psUnitData->eParseError |= BSPP_ERROR_UNSUPPORTED;
                        return IMG_ERROR_CANCELLED;
                    }
                    //new view needs for new picture header
                    psUnitData->out.psPictHdrInfo = psUnitData->psParseState->psNextPictHdrInfo;

                    //reset top/bottom flags
                    psUnitData->psParseState->ui8PrevBottomPicFlag = (IMG_UINT8)BSPP_INVALID;
                    psUnitData->psParseState->ui8SecondFieldFlag = IMG_FALSE;
                }
            }

            IMG_ASSERT(psUnitData->out.psPictHdrInfo);
            if (IMG_NULL == psUnitData->out.psPictHdrInfo)
            {
                return IMG_ERROR_CANCELLED;
            }
            psUnitData->out.psPictHdrInfo->bDiscontinuousMbs = IMG_FALSE;                                                       // Default
            do
            {
                // Parse the Pic Header, return Parse SPS/PPS Structures
                eParseError = bspp_H264PictHdrParser(
                    hSwSrContext,
                    psUnitData->hStrRes,
                    &sH264SliceHdrInfo,
                    &psPPSInfo,
                    &psSequHdrInfo,
                    eNalUnitType,
                    nal_ref_idc);

                if (eParseError)
                {
                    psUnitData->eParseError |= eParseError;
                    return IMG_ERROR_CANCELLED;
                }

#ifdef I_FRAME_SIGNALS_CLOSED_GOP
                // We are signalling closed gop at every I frame. This does not conform 100% with the specification but insures that seeking always
                // works.
                psUnitData->bNewClosedGOP = sH264SliceHdrInfo.slice_type == I_SLICE? IMG_TRUE : IMG_FALSE;
#endif
                // Now psPPSInfo and psSequHdrInfo contain the PPS/SPS info related to this picture
                psH264PPSInfo = (BSPP_sH264PPSInfo *)psPPSInfo->hSecurePPSInfo;
                psH264SequHdrInfo = (BSPP_sH264SequHdrInfo *)psSequHdrInfo->hSecureSequenceInfo;
                psH264FWPPSInfo = (H264FW_sPicturePS *)GET_ELEMENT_FROM_PACKED_DEVICE_BUFFER(&psPPSInfo->sFWPPS);
                psH264FWSequHdrInfo = (H264FW_sSequencePS *)GET_ELEMENT_FROM_PACKED_DEVICE_BUFFER(&psSequHdrInfo->sFWSequence);
                IMG_ASSERT(sH264SliceHdrInfo.pic_parameter_set_id == psH264PPSInfo->pic_parameter_set_id);
                IMG_ASSERT(psH264PPSInfo->seq_parameter_set_id == (IMG_UINT32)psH264SequHdrInfo->sSPSInfo.seq_parameter_set_id);

                // Update the decoding-related FW SPS info related to the current picture with the SEI data that were
                // potentially received and also relate to the current info. Until we receive the picture we do not
                // know which sequence to update with the SEI data.
                // Set from last SEI, needed for decoding
                psH264FWSequHdrInfo->bDisableVDMCFilt            = psInterPictCtx->bDisableVDMCFilt;
                psH264FWSequHdrInfo->b4x4TransformMBNotAvailable = psInterPictCtx->b4x4TransformMBNotAvailable;

                // Determine if current slice is a new picture, and update the related params for future reference
                // Order of checks is important
                {
                    SET_IF_NOT_DETERMINED_YET(
                        bDetermined,
                        psUnitData->psParseState->bNewView,
                        bCurrentPicIsNew,
                        IMG_TRUE);
                    SET_IF_NOT_DETERMINED_YET(
                        bDetermined,
                        psUnitData->psParseState->bNextPicIsNew,
                        bCurrentPicIsNew,
                        IMG_TRUE);
                    SET_IF_NOT_DETERMINED_YET(
                        bDetermined,
                        (sH264SliceHdrInfo.redundant_pic_cnt>0),
                        bCurrentPicIsNew,
                        IMG_FALSE);
                    SET_IF_NOT_DETERMINED_YET(
                        bDetermined,
                        (psUnitData->psParseState->ui32PrevFrameNum != sH264SliceHdrInfo.frame_num),
                        bCurrentPicIsNew,
                        IMG_TRUE);
                    SET_IF_NOT_DETERMINED_YET(
                        bDetermined,
                        (psUnitData->psParseState->ui32PrevPPSId != sH264SliceHdrInfo.pic_parameter_set_id),
                        bCurrentPicIsNew,
                        IMG_TRUE);
                    SET_IF_NOT_DETERMINED_YET(
                        bDetermined,
                        (psUnitData->psParseState->ui32PrevFieldPicFlag != sH264SliceHdrInfo.field_pic_flag),
                        bCurrentPicIsNew,
                        IMG_TRUE);
                    SET_IF_NOT_DETERMINED_YET(
                        bDetermined,
                        ( (sH264SliceHdrInfo.field_pic_flag) &&
                          (psUnitData->psParseState->ui8PrevBottomPicFlag != sH264SliceHdrInfo.bottom_field_flag)
                        ),
                        bCurrentPicIsNew,
                        IMG_TRUE);
                    SET_IF_NOT_DETERMINED_YET(
                        bDetermined,
                        ( (psUnitData->psParseState->ui32PrevNalRefIdc == 0 || nal_ref_idc == 0) &&
                          (psUnitData->psParseState->ui32PrevNalRefIdc != nal_ref_idc)
                        ),
                        bCurrentPicIsNew,
                        IMG_TRUE);
                    SET_IF_NOT_DETERMINED_YET(
                        bDetermined,
                        ( (psH264SequHdrInfo->sSPSInfo.pic_order_cnt_type == 0) &&
                          ( (psUnitData->psParseState->ui32PrevPicOrderCntLsb != sH264SliceHdrInfo.pic_order_cnt_lsb) ||
                            (psUnitData->psParseState->i32PrevDeltaPicOrderCntBottom != sH264SliceHdrInfo.delta_pic_order_cnt_bottom)
                          )
                        ),
                        bCurrentPicIsNew,
                        IMG_TRUE);
                    SET_IF_NOT_DETERMINED_YET(
                        bDetermined,
                        ( (psH264SequHdrInfo->sSPSInfo.pic_order_cnt_type == 1) &&
                          ( (psUnitData->psParseState->ai32PrevDeltaPicOrderCnt[0] != sH264SliceHdrInfo.delta_pic_order_cnt[0]) ||
                            (psUnitData->psParseState->ai32PrevDeltaPicOrderCnt[1] != sH264SliceHdrInfo.delta_pic_order_cnt[1])
                          )
                        ),
                        bCurrentPicIsNew,
                        IMG_TRUE);
                    SET_IF_NOT_DETERMINED_YET(
                        bDetermined,
                        ( (psUnitData->psParseState->iPrevNalUnitType == (IMG_INT32)H264_NALTYPE_IDR_SLICE || eNalUnitType == (IMG_INT32)H264_NALTYPE_IDR_SLICE) &&
                          (psUnitData->psParseState->iPrevNalUnitType != (IMG_INT32)eNalUnitType)
                        ),
                        bCurrentPicIsNew,
                        IMG_TRUE);
                    SET_IF_NOT_DETERMINED_YET(
                        bDetermined,
                        ( (psUnitData->psParseState->iPrevNalUnitType == (IMG_INT32)H264_NALTYPE_IDR_SLICE) &&
                          (psUnitData->psParseState->ui32PrevIdrPicId != sH264SliceHdrInfo.idr_pic_id)
                        ),
                        bCurrentPicIsNew,
                        IMG_TRUE);

                    // Update whatever is not updated already in different places of the code or just needs to be updated here
                    psUnitData->psParseState->ui32PrevFrameNum = sH264SliceHdrInfo.frame_num;
                    psUnitData->psParseState->ui32PrevPPSId = sH264SliceHdrInfo.pic_parameter_set_id;
                    psUnitData->psParseState->ui32PrevFieldPicFlag = sH264SliceHdrInfo.field_pic_flag;
                    psUnitData->psParseState->ui32PrevNalRefIdc = nal_ref_idc;
                    psUnitData->psParseState->ui32PrevPicOrderCntLsb = sH264SliceHdrInfo.pic_order_cnt_lsb;
                    psUnitData->psParseState->i32PrevDeltaPicOrderCntBottom = sH264SliceHdrInfo.delta_pic_order_cnt_bottom;
                    psUnitData->psParseState->ai32PrevDeltaPicOrderCnt[0] = sH264SliceHdrInfo.delta_pic_order_cnt[0];
                    psUnitData->psParseState->ai32PrevDeltaPicOrderCnt[1] = sH264SliceHdrInfo.delta_pic_order_cnt[1];
                    psUnitData->psParseState->iPrevNalUnitType = (IMG_INT32)eNalUnitType;
                    psUnitData->psParseState->ui32PrevIdrPicId = sH264SliceHdrInfo.idr_pic_id;
                }

                // Detect second field and manage the ui8PrevBottomPicFlag flag
                if (sH264SliceHdrInfo.field_pic_flag && bCurrentPicIsNew)
                {
                    // Do not confuse 1st slice as new (second) field
                    if (psUnitData->psParseState->ui8PrevBottomPicFlag != (IMG_UINT8)BSPP_INVALID)
                    {
                        // We should have not flagged 2nd field already
                        IMG_ASSERT(psUnitData->psParseState->ui8SecondFieldFlag == IMG_FALSE);
                        // If 2 fields in same buffer, they should be a pair
                        IMG_ASSERT(psUnitData->psParseState->ui8PrevBottomPicFlag != sH264SliceHdrInfo.bottom_field_flag);
                        psUnitData->psParseState->ui8SecondFieldFlag = IMG_TRUE;
                    }
                    psUnitData->psParseState->ui8PrevBottomPicFlag = sH264SliceHdrInfo.bottom_field_flag;
                }

                // Detect ASO
                if (bCurrentPicIsNew)  // Just met new pic
                {
                    IMG_UINT32 i;
                    for (i=0; i<MAX_COMPONENTS; i++)
                    {
                        // Reset the Previous MBs In Slice for ASO detection
                        psUnitData->psParseState->aui32PrevFirstMBInSlice[i] = 0;
                    }
                }
                else if (psUnitData->psParseState->aui32PrevFirstMBInSlice[sH264SliceHdrInfo.colour_plane_id] > sH264SliceHdrInfo.first_mb_in_slice)
                {
                    // We just found ASO
                    psUnitData->psParseState->bDiscontineousMB = IMG_TRUE;
                }
                psUnitData->psParseState->aui32PrevFirstMBInSlice[sH264SliceHdrInfo.colour_plane_id] = sH264SliceHdrInfo.first_mb_in_slice;

                // We may already knew we were DiscontineousMB
                if(psUnitData->psParseState->bDiscontineousMB)
                {
                    psUnitData->out.psPictHdrInfo->bDiscontinuousMbs = psUnitData->psParseState->bDiscontineousMB;
                }

                // We want to calculate the scaling lists only once per picture/field, not every slice
                // We want to populate the VDEC Picture Header Info only once per picture/field, not every slice
                if (bCurrentPicIsNew)
                {
                    // Common Sequence Header Info fetched
                    VDEC_sComSequHdrInfo *  psComSequHdrInfo = &psSequHdrInfo->sSequHdrInfo.sComSequHdrInfo;
                    BSPP_sPictData * psTypePictAuxData;

                    psUnitData->psParseState->bNextPicIsNew = IMG_FALSE;

                    // Generate SGM for this picture
                    if( (psH264PPSInfo->num_slice_groups_minus1 > 0) && (psH264PPSInfo->slice_group_map_type <= 6) )
                    {
                        bspp_H264UnitParser_CompileSGMData(&sH264SliceHdrInfo,psH264SequHdrInfo,psH264PPSInfo,psUnitData->out.psPictHdrInfo);
                    }
                    else
                    {
                        psUnitData->out.psPictHdrInfo->sPictSgmData.pvData = IMG_NULL;
                        psUnitData->out.psPictHdrInfo->sPictSgmData.ui32BufMapId = 0;
                        psUnitData->out.psPictHdrInfo->sPictSgmData.ui32BufOffset = 0;
                        psUnitData->out.psPictHdrInfo->sPictSgmData.ui32Id = BSPP_INVALID;
                        psUnitData->out.psPictHdrInfo->sPictSgmData.ui32Size = 0;
                    }

                    psUnitData->psParseState->bDiscontineousMB = psUnitData->out.psPictHdrInfo->bDiscontinuousMbs;

                    // Select the scaling lists based on psH264PPSInfo and psH264SequHdrInfo and pass them to psH264FWPPSInfo
                    bspp_H264SelectScalingLists(psH264FWPPSInfo, psH264PPSInfo, psH264SequHdrInfo);

                    // Uses the common sequence/SINGLE-slice info to populate the VDEC Picture Header Info
                    bspp_H264PictHdrPopulate(eNalUnitType, &sH264SliceHdrInfo, psComSequHdrInfo, psUnitData->out.psPictHdrInfo);

                    // Update the display-related picture header information with the related SEI parsed data
                    if (!psInterPictCtx->bSEIInfoAttachedToPic)   // The display-related SEI is used only for the first picture after the SEI
                    {
                        psInterPictCtx->bSEIInfoAttachedToPic = IMG_TRUE;
                        if (psInterPictCtx->ui32ActiveSpsForSeiParsing != psH264SequHdrInfo->sSPSInfo.seq_parameter_set_id)
                        {
                            // We tried to guess the SPS ID that we should use to parse the SEI, but we guessed wrong
                            REPORT(REPORT_MODULE_BSPP, REPORT_WARNING, "Parsed SEI with wrong SPS, data may have been parsed wrong");
                        }
                        psUnitData->out.psPictHdrInfo->sDispInfo.bRepeatFirstField = psInterPictCtx->bRepeatFirstField;
                        psUnitData->out.psPictHdrInfo->sDispInfo.ui32MaxFrmRepeat = psInterPictCtx->ui32MaxFrmRepeat;
                    }
                    // For Idr slices update the Active Sequence ID for SEI parsing, error resilient
                    if (eNalUnitType == H264_NALTYPE_IDR_SLICE)
                    {
                        psInterPictCtx->ui32ActiveSpsForSeiParsing = psH264SequHdrInfo->sSPSInfo.seq_parameter_set_id;
                    }

                    if (psUnitData->psParseState->ui8SecondFieldFlag)
                    {
                        psTypePictAuxData = &psUnitData->out.psPictHdrInfo->sSecondPictAuxData;
                    }
                    else
                    {
                        psTypePictAuxData = &psUnitData->out.psPictHdrInfo->sPictAuxData;
                    }

                    /* We have no container for the PPS that passes down to the kernel, for this reason the h264 secure parser
                       needs to populate that info into the picture header (Second)PictAuxData. */
                    psTypePictAuxData->ui32BufMapId = psPPSInfo->ui32BufMapId;
                    psTypePictAuxData->ui32BufOffset = psPPSInfo->ui32BufOffset;
                    psTypePictAuxData->pvData = (IMG_VOID *)psH264FWPPSInfo;
                    psTypePictAuxData->ui32Id = psH264PPSInfo->pic_parameter_set_id;
                    psTypePictAuxData->ui32Size = sizeof(H264FW_sPicturePS);

                    psPPSInfo->ui32RefCount++;

                    // This info comes from NAL directly
                    psUnitData->out.psPictHdrInfo->bReference = (nal_ref_idc==0) ? IMG_FALSE : IMG_TRUE;
                }
                if (eNalUnitType == H264_NALTYPE_IDR_SLICE)
                {
                    psUnitData->bNewClosedGOP = IMG_TRUE;
                }
                // Return the SPS ID
                psUnitData->ui32PictSequHdrId = GET_SUBSET_ID(eNalUnitType, psH264PPSInfo->seq_parameter_set_id);

            } while(0);
        }
        else if (eNalUnitType == H264_NALTYPE_SLICE_PARTITION_A ||
                 eNalUnitType == H264_NALTYPE_SLICE_PARTITION_B ||
                 eNalUnitType == H264_NALTYPE_SLICE_PARTITION_C)
        {
            psUnitData->bSlice = IMG_TRUE;

            REPORT(REPORT_MODULE_BSPP, REPORT_WARNING,
                "Unsupported Slice NAL type: %d",
                 eNalUnitType);
            psUnitData->eParseError = BSPP_ERROR_UNSUPPORTED;
        }
        break;

    case BSPP_UNIT_UNCLASSIFIED:
        if (eNalUnitType == H264_NALTYPE_ACCESS_UNIT_DELIMITER)
        {
            psUnitData->psParseState->bNextPicIsNew = IMG_TRUE;
        }
        else if (eNalUnitType == H264_NALTYPE_SLICE_PREFIX ||
                 eNalUnitType == H264_NALTYPE_SUBSET_SPS)
        {
            //if mvc disabled do nothing
        }
        else if (eNalUnitType == H264_NALTYPE_SUPPLEMENTAL_ENHANCEMENT_INFO)
        {
            /* We parse the SEI into a local variable. Everything we need to keep to attach to
               the Sequence or Next Picture needs to be copied into the inter-picture context*/
            BSPP_sH264SEIInfo  * psH264SEIInfo = BSPP_GetSEIDataInfo(psUnitData->hStrRes);

            IMG_ASSERT(psH264SEIInfo != IMG_NULL);
            if(psH264SEIInfo == IMG_NULL)
            {
                return IMG_ERROR_MALLOC_FAILED;
            }
            IMG_MEMSET(psH264SEIInfo, 0, sizeof(BSPP_sH264SEIInfo));

            // Parse the SEI payloads we are interested in
            psUnitData->eParseError |= BSPP_H264SEIParser(hSwSrContext,
                                                          psUnitData->hStrRes,
                                                          psH264SEIInfo,
                                                          &psInterPictCtx->ui32ActiveSpsForSeiParsing);

            // Set that the SEI Info has not been attached to a picture yet
            psInterPictCtx->bSEIInfoAttachedToPic = IMG_FALSE;

            // Extract the decoding-related information coming from the SEI message: They affect the whole bitstream until updated
#ifdef PARSE_SEI_USER_DATA_UNREGISTERED
            if (psH264SEIInfo->sSeiUserDataUnregistered.bInUseFlag)   // Update only if a new SEI message of this type has been parsed
            {
                BSPP_sH264SeiUserDataUnregistered * psSeiUserDataUnregistered = &psH264SEIInfo->sSeiUserDataUnregistered;
                psInterPictCtx->bDisableVDMCFilt = bspp_IsH264StreamX264(psSeiUserDataUnregistered->user_data_payload_byte);
                psInterPictCtx->b4x4TransformMBNotAvailable = bspp_IsH264StreamJM182(psSeiUserDataUnregistered->user_data_payload_byte);
            }
#endif

            // Extract the display-related information coming from the SEI message: Affect only the next picture
#ifdef PARSE_SEI_PIC_TIMING
            {
                BSPP_sH264SeiPicTiming * psSeiPicTiming = &psH264SEIInfo->sSeiPicTiming;
                psInterPictCtx->bRepeatFirstField = (psSeiPicTiming->bInUseFlag && (psSeiPicTiming->pic_struct == 5 || psSeiPicTiming->pic_struct == 6)) ? IMG_TRUE : IMG_FALSE;
                psInterPictCtx->ui32MaxFrmRepeat = (psSeiPicTiming->bInUseFlag && psSeiPicTiming->pic_struct == 8) ? 2 :
                                                   (psSeiPicTiming->bInUseFlag && psSeiPicTiming->pic_struct == 7) ? 1 :
                                                   0;
            }
#endif
        }
        else
        {
            // Should not have any other type of unclassified data.
            IMG_ASSERT(IMG_FALSE);
        }
        break;

    case BSPP_UNIT_NON_PICTURE:
        if (eNalUnitType == H264_NALTYPE_END_OF_SEQUENCE ||
            eNalUnitType == H264_NALTYPE_END_OF_STREAM)
        {
            psUnitData->psParseState->bNextPicIsNew = IMG_TRUE;
        }
        else if (eNalUnitType == H264_NALTYPE_FILLER_DATA ||
                 eNalUnitType == H264_NALTYPE_SEQUENCE_PARAMETER_SET_EXTENTION ||
                 eNalUnitType == H264_NALTYPE_AUXILIARY_SLICE)
        {

        }
        else if (eNalUnitType == H264_NALTYPE_SLICE_SCALABLE ||
                 eNalUnitType == H264_NALTYPE_SLICE_IDR_SCALABLE)
        {
            //if mvc disabled do nothing
        }
        else
        {
            // Should not have any other type of non-picture data.
            IMG_ASSERT(IMG_FALSE);
        }
        break;

    case BSPP_UNIT_UNSUPPORTED:
        REPORT(REPORT_MODULE_BSPP, REPORT_WARNING,
                "Unsupported NAL type: %d",
                 eNalUnitType);
        psUnitData->eParseError = BSPP_ERROR_UNKNOWN_DATAUNIT_DETECTED;
        break;

    default:
        IMG_ASSERT(IMG_FALSE);
        break;

    }

    return ui32Result;
}

