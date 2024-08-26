/*!
 *****************************************************************************
 *
 * @File       h264_secure_parser.h
 * @Title      h.264 bitstream pre-parsing
 * @Description    This file contains h.264 header pre-parse
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

#if !defined(__H264_SECURE_PARSER__H__)
#define __H264_SECURE_PARSER__H__

#include "img_defs.h"
#include "bspp_int.h"

#ifdef __cplusplus
extern "C" {
#endif

/*!
******************************************************************************

                NAL type emumeration

******************************************************************************/
typedef enum
{
    H264_NALTYPE_UNSPECIFIED                        = 0,
    H264_NALTYPE_SLICE                              = 1,
    H264_NALTYPE_SLICE_PARTITION_A                  = 2,
    H264_NALTYPE_SLICE_PARTITION_B                  = 3,
    H264_NALTYPE_SLICE_PARTITION_C                  = 4,
    H264_NALTYPE_IDR_SLICE                          = 5,
    H264_NALTYPE_SUPPLEMENTAL_ENHANCEMENT_INFO      = 6,
    H264_NALTYPE_SEQUENCE_PARAMETER_SET             = 7,
    H264_NALTYPE_PICTURE_PARAMETER_SET              = 8,
    H264_NALTYPE_ACCESS_UNIT_DELIMITER              = 9,
    H264_NALTYPE_END_OF_SEQUENCE                    = 10,
    H264_NALTYPE_END_OF_STREAM                      = 11,
    H264_NALTYPE_FILLER_DATA                        = 12,
    H264_NALTYPE_SEQUENCE_PARAMETER_SET_EXTENTION   = 13,
    H264_NALTYPE_SLICE_PREFIX                       = 14,
    H264_NALTYPE_SUBSET_SPS                         = 15,
    H264_NALTYPE_AUXILIARY_SLICE                    = 19,
    H264_NALTYPE_SLICE_SCALABLE                     = 20,
    H264_NALTYPE_SLICE_IDR_SCALABLE                 = 21,

    H264_NALTYPE_MAX                                = 31,

} H264_eNalUnitType;



typedef struct{
    IMG_UINT32      profile_idc;  
    IMG_UINT32      constraint_set_flags;             
    IMG_UINT32      level_idc;                       
    IMG_UINT8       seq_parameter_set_id;               
    IMG_UINT8       chroma_format_idc;   
    IMG_BOOL        separate_colour_plane_flag;            
    IMG_UINT32      bit_depth_luma_minus8; 
    IMG_UINT32      bit_depth_chroma_minus8; 
    IMG_UINT8       qpprime_y_zero_transform_bypass_flag;
    IMG_BOOL        seq_scaling_matrix_present_flag;    
    IMG_UINT8       seq_scaling_list_present_flag[12];
    IMG_UINT32      log2_max_frame_num_minus4 ;         
    IMG_UINT32      pic_order_cnt_type;          
    IMG_UINT32      log2_max_pic_order_cnt_lsb_minus4;
    IMG_BOOL        delta_pic_order_always_zero_flag; 
    IMG_INT32       offset_for_non_ref_pic;              
    IMG_INT32       offset_for_top_to_bottom_field ;
    IMG_UINT32      num_ref_frames_in_pic_order_cnt_cycle;
    IMG_UINT32      * pui32offset_for_ref_frame;   
    IMG_UINT32      max_num_ref_frames;               
    IMG_BOOL        gaps_in_frame_num_value_allowed_flag;
    IMG_UINT32      pic_width_in_mbs_minus1; 
    IMG_UINT32      pic_height_in_map_units_minus1; 
    IMG_BOOL        frame_mbs_only_flag;
    IMG_BOOL        mb_adaptive_frame_field_flag; 
    IMG_BOOL        direct_8x8_inference_flag;
    IMG_BOOL        frame_cropping_flag;
    IMG_UINT32      frame_crop_left_offset;
    IMG_UINT32      frame_crop_right_offset;
    IMG_UINT32      frame_crop_top_offset;
    IMG_UINT32      frame_crop_bottom_offset;
    IMG_BOOL        vui_parameters_present_flag;

    IMG_BOOL        bMVCVUIParameterPresentFlag;        /*!< mvc_vui_parameters_present_flag;   UNUSED        */

    /* scaling lists are derived from both SPS and PPS information */
    /* but will change whenever the PPS changes */
    /* The derived set of tables are associated here with the PPS */
    /* NB: These are in H.264 order */
    IMG_UINT8 *     pui8ScalingList4x4_seq;  //!< derived from SPS and PPS - 8 bit each
    IMG_UINT8 *     pui8ScalingList8x8_seq;  //!< derived from SPS and PPS - 8 bit each
    // This is not direct parsed data, though it is extracted
    IMG_UINT8       UseDefaultScalingMatrixFlag_seq[12];
} BSPP_sH264SPSInfo;

typedef struct{
    IMG_UINT8   ui32CPBCntMinus1;                                         /*!< cpb_cnt_minus1;                                 */
    IMG_UINT8   ui32BitRateScale;                                         /*!< bit_rate_scale;                                 */
    IMG_UINT8   ui32CPBSizeScale;                                         /*!< cpb_size_scale;                                 */
    IMG_UINT32 * pui32BitRateValueMinus1;  /*!< bit_rate_value_minus1 [MAXIMUMVALUEOFcpb_cnt];  */
    IMG_UINT32 * pui32CPBSizeValueMinus1;  /*!< cpb_size_value_minus1 [MAXIMUMVALUEOFcpb_cnt];  */
    IMG_UINT8 * pui8CBRFlag;             /*!< cbr_flag              [MAXIMUMVALUEOFcpb_cnt];  */
    IMG_UINT8   ui32InitialCPBRemovalDelayLengthMinus1;                   /*!< initial_cpb_removal_delay_length_minus1;        */
    IMG_UINT8   ui32CPBRemovalDelayLenghtMinus1;                          /*!< cpb_removal_delay_length_minus1;                */
    IMG_UINT8   ui32DPBOutputDelayLengthMinus1;                           /*!< dpb_output_delay_length_minus1;                 */
    IMG_UINT8   ui32TimeOffsetLength;                                     /*!< time_offset_length;                             */
} BSPP_sH264HRDParamInfo;

typedef struct{
    IMG_BOOL                aspect_ratio_info_present_flag;
    IMG_UINT32              aspect_ratio_idc; 
    IMG_UINT32              sar_width; 
    IMG_UINT32              sar_height;
    IMG_BOOL                overscan_info_present_flag;  
    IMG_BOOL                overscan_appropriate_flag; 
    IMG_BOOL                video_signal_type_present_flag;   
    IMG_UINT32              video_format;       
    IMG_BOOL                video_full_range_flag;         
    IMG_BOOL                colour_description_present_flag;  
    IMG_UINT32              colour_primaries;        
    IMG_UINT32              transfer_characteristics;  
    IMG_UINT32              matrix_coefficients;  
    IMG_BOOL                chroma_location_info_present_flag;  
    IMG_UINT32              chroma_sample_loc_type_top_field; 
    IMG_UINT32              chroma_sample_loc_type_bottom_field;  
    IMG_BOOL                timing_info_present_flag;    
    IMG_UINT32              num_units_in_tick;      
    IMG_UINT32              time_scale;                  
    IMG_BOOL                fixed_frame_rate_flag;  
    IMG_BOOL                nal_hrd_parameters_present_flag;
    BSPP_sH264HRDParamInfo  sNALHRDParameters;                      /*!< hrd_parameters                                 */
    IMG_BOOL                vcl_hrd_parameters_present_flag;  
    BSPP_sH264HRDParamInfo  sVCLHRDParameters;                      /*!< hrd_parameters                                 */
    IMG_BOOL                low_delay_hrd_flag;                     
    IMG_BOOL                pic_struct_present_flag;    
    IMG_BOOL                bitstream_restriction_flag;   
    IMG_BOOL                motion_vectors_over_pic_boundaries_flag;  
    IMG_UINT32              max_bytes_per_pic_denom;  
    IMG_UINT32              max_bits_per_mb_denom;   
    IMG_UINT32              log2_max_mv_length_vertical; 
    IMG_UINT32              log2_max_mv_length_horizontal; 
    IMG_UINT32              num_reorder_frames;     
    IMG_UINT32              max_dec_frame_buffering;  
} BSPP_sH264VUIInfo;

typedef struct{
    IMG_UINT16  aui16ViewId[VDEC_H264_MVC_MAX_VIEWS];                                                                                   /*!< View Id                                */
    IMG_UINT16  aui16NumAnchorRefs1X[2][VDEC_H264_MVC_MAX_VIEWS];                                                                       /*!< number of anchor references;           */
    IMG_UINT16  * pui16AnchorRefIndicies1X;                                                                                             /*!< anchor reference id;                   */
    IMG_UINT16  aui16NumNonAnchorRefs1X[2][VDEC_H264_MVC_MAX_VIEWS];                                                                    /*!< number of non-anchor ref ;             */
    IMG_UINT16  * pui16NonAnchorRefIndicies1X;                                                                                          /*!< number of non anchor ref id            */
    //IMG_UINT16  ui16NumLevelValuesSignalledMinus1;                                                                                    /*!< num_level_values_signalled_minus1;     */
    //IMG_UINT8   ui8LevelIdc[VDEC_H264_MVC_MAX_LEVELS];                                                                                /*!< level_idc;                             */
    //IMG_UINT16  ui16NumApplicableOpsMinus1[VDEC_H264_MVC_MAX_LEVELS];                                                                 /*!< num_applicable_ops_minus1;             */
    //IMG_UINT32  ui32ApplicableOpTemporalId[VDEC_H264_MVC_MAX_LEVELS][VDEC_H264_MVC_MAX_APP_OP_TID];                                   /*!< applicable_op_temporal_id;             */
    //IMG_UINT16  ui16ApplicableOpNumTargetViewsMinus1[VDEC_H264_MVC_MAX_LEVELS][VDEC_H264_MVC_MAX_APP_OP_TID];                         /*!< applicable_op_num_target_views_minus1; */
    //IMG_UINT16  ui16ApplicableOpTargetViewId[VDEC_H264_MVC_MAX_LEVELS][VDEC_H264_MVC_MAX_APP_OP_TID][VDEC_H264_MVC_MAX_TARGET_VIEW];  /*!< applicable_op_target_view_id;          */
    //IMG_UINT16  ui16ApplicableOpNumViewsMinus1[VDEC_H264_MVC_MAX_LEVELS][VDEC_H264_MVC_MAX_APP_OP_TID];                               /*!< applicable_op_num_views_minus1;        */
    IMG_UINT16  ui16NumViewsMinus1;                                                                                                     /*!< num_views_minus1                       */
} BSPP_sH264SPSInfo_MVCExt;

/*!
******************************************************************************
 This structure contains H264 sequence header information (SPS, VUI, MVC).
 Contains evrything parsed from the Sequence Header.
******************************************************************************/
typedef struct{
    BSPP_sH264SPSInfo         sSPSInfo;        /*!< Video sequence header information.          */
    BSPP_sH264VUIInfo         sVUIInfo;        /*!< VUI sequence header information.            */
    BSPP_sH264SPSInfo_MVCExt  sSPSMVCExtInfo;  /*!< MVC sequence header information. Extension  */
} BSPP_sH264SequHdrInfo;

/*!
******************************************************************************
 This structure contains H264 PPS parse data.
******************************************************************************/
typedef struct{
    IMG_UINT8 * slice_group_id;
    IMG_UINT16 ui16SliceGroupIdNum;
} BSPP_sH264PPSSGMInfo;

/*!
******************************************************************************
 This structure contains H264 PPS parse data.
******************************************************************************/
typedef struct{
    IMG_INT32       pic_parameter_set_id;                   /*!< pic_parameter_set_id: defines the PPS ID of the current PPS      */
    IMG_INT32       seq_parameter_set_id;                   /*!< seq_parameter_set_id: defines the SPS that current PPS points to */
    IMG_BOOL        entropy_coding_mode_flag;           
    IMG_BOOL        pic_order_present_flag;
    IMG_UINT8       num_slice_groups_minus1;                
    IMG_UINT8       slice_group_map_type;
    IMG_UINT16      run_length_minus1[8];
    IMG_UINT16      top_left[8];
    IMG_UINT16      bottom_right[8];
    IMG_BOOL        slice_group_change_direction_flag;
    IMG_UINT16      slice_group_change_rate_minus1;         
    IMG_UINT16      pic_size_in_map_unit;
    BSPP_sH264PPSSGMInfo sH264PPSSGMInfo;
    IMG_UINT8       num_ref_idx_lX_active_minus1[H264FW_MAX_REFPIC_LISTS];        
    IMG_BOOL        weighted_pred_flag;                   
    IMG_UINT8       weighted_bipred_idc;                 
    IMG_INT32       pic_init_qp_minus26;   
    IMG_INT32       pic_init_qs_minus26;   
    IMG_INT32       chroma_qp_index_offset;                 
    IMG_BOOL        deblocking_filter_control_present_flag;
    IMG_BOOL        constrained_intra_pred_flag;      
    IMG_BOOL        redundant_pic_cnt_present_flag;      
    IMG_BOOL        transform_8x8_mode_flag;                  
    IMG_BOOL        pic_scaling_matrix_present_flag;
    IMG_UINT8       pic_scaling_list_present_flag[12];                
    IMG_INT32       second_chroma_qp_index_offset;

    /* scaling lists are derived from both SPS and PPS information */
    /* but will change whenever the PPS changes */
    /* The derived set of tables are associated here with the PPS */
    /* NB: These are in H.264 order */
    IMG_UINT8 *      pui8ScalingList4x4_pic;  //!< derived from SPS and PPS - 8 bit each
    IMG_UINT8 *      pui8ScalingList8x8_pic;  //!< derived from SPS and PPS - 8 bit each
    // This is not direct parsed data, though it is extracted
    IMG_UINT8       UseDefaultScalingMatrixFlag_pic[12];
} BSPP_sH264PPSInfo;

typedef enum{
    P_SLICE = 0,
    B_SLICE,
    I_SLICE,
    SP_SLICE,
    SI_SLICE
} BSPP_eH264SliceType;

typedef struct{
    IMG_UINT16          first_mb_in_slice;
    BSPP_eH264SliceType slice_type;

    /* data to ID new picture */
    IMG_UINT32          pic_parameter_set_id;
    IMG_UINT32          frame_num;
    IMG_UINT8           colour_plane_id;
    IMG_UINT8           field_pic_flag;
    IMG_UINT8           bottom_field_flag;
    IMG_UINT32          idr_pic_id;
    IMG_UINT32          pic_order_cnt_lsb;
    IMG_INT32           delta_pic_order_cnt_bottom;
    IMG_INT32           delta_pic_order_cnt[2];
    IMG_UINT32          redundant_pic_cnt;

    /* Things we need to read out when doing In Secure */

    IMG_UINT8	num_ref_idx_active_override_flag;
	IMG_UINT8	num_ref_idx_lX_active_minus1[2];

    IMG_UINT16	slice_group_change_cycle;



} BSPP_sH264SliceHdrInfo;

/*!
******************************************************************************

 @Function              BSPP_H264DestroySequHdrInfo

******************************************************************************/
extern IMG_RESULT BSPP_H264DestroyPPSInfo(
    const IMG_HANDLE      hSecurePPSInfo
);

/*!
******************************************************************************

 @Function              BSPP_H264DestroySequHdrInfo

******************************************************************************/
extern IMG_RESULT BSPP_H264DestroySequHdrInfo(
    const IMG_HANDLE      hSecureSPSInfo
);

/*!
******************************************************************************

 @Function              BSPP_H264ResetSequHdrInfo

******************************************************************************/
extern IMG_RESULT BSPP_H264ResetSequHdrInfo(
    IMG_HANDLE       hSecureSPSInfo
);

/*!
******************************************************************************

 @Function              BSPP_H264ResetPPSInfo

******************************************************************************/
extern IMG_RESULT BSPP_H264ResetPPSInfo(
    IMG_HANDLE       hSecurePPSInfo
);

/*!
******************************************************************************

 @Function              BSPP_H264LogPpsInfo
 
 @Description
 
 Logs the H.264 PPS header information.

 @Input     psFwPps         : A pointer to the FW PPS device memory.

 @Input     hSecurePpsInfo : A handle to the BSPP internal copy of the
                             PPS header information.
 
 @Return    IMG_RESULT : This function returns either IMG_SUCCESS or an error code.

******************************************************************************/
extern IMG_RESULT
BSPP_H264LogPpsInfo(
    const H264FW_sPicturePS * psFwPps,
    const IMG_HANDLE          hSecurePpsInfo
);


/*!
******************************************************************************

 @Function              BSPP_H264LogSequHdrInfo

 @Description
 
 Logs the H.264 sequence header information.

 @Input     psSequHdrInfo : A pointer to the sequence header information containing
                            reference to the FW sequence device memory.

 @Input     hSecureSequenceInfo : A handle to the BSPP internal copy of the
                                  sequence header information.
 
 @Return    IMG_RESULT : This function returns either IMG_SUCCESS or an error code.

******************************************************************************/
extern IMG_RESULT  
BSPP_H264LogSequHdrInfo(
    const BSPP_sSequHdrInfo * psSequHdrInfo,
    const IMG_HANDLE          hSecureSequenceInfo
);



/*!
******************************************************************************

 @Function              BSPP_H264UnitParser

 @Description
 
 Parses the unit (not necessarily everything) and returns the populated 
 structure depending on the unit type.

 @Input     hSwSrContext : A handle to software shift-register context.

 @InOut     psUnitData : A pointer to unit data which includes input and output
                         parameters as defined by structure.
 
 @Return    IMG_RESULT : This function returns either IMG_SUCCESS or an error code.

******************************************************************************/
extern IMG_RESULT  BSPP_H264UnitParser( 
    const IMG_HANDLE    hSwSrContext,
    BSPP_sUnitData    * psUnitData
);


#ifdef __cplusplus
}
#endif

#endif
/*__H264_SECURE_PARSER__H__*/
