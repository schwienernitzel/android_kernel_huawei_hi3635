/*!
 *****************************************************************************
 *
 * @File       h264_secure_sei_parser.h
 * @Title      h.264 SEI parsing
 * @Description    This file contains h.264 SEI structure and parsing
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

#if !defined(__H264_SECURE_SEI_PARSER__H__)
#define __H264_SECURE_SEI_PARSER__H__

#include "img_defs.h"
#include "img_types.h"
#include "bspp.h"

// This macro takes a (signed) value and its width in bits and returns the 32-bit signed extended value
#define SIGN_EXTEND32(value,width)  (                               \
        (((IMG_UINT32)value) & (1 << (width-1))) ?                  \
        (((IMG_INT32)value) | (0xFFFFFFFF ^ ((1<<width)-1))) :      \
        ((IMG_INT32)value)                                          \
    )

#define ROUND_UP_TO_8(value)    (((value+7)>>3)<<3)

/* constants should not be changed */
#define	H264_CPB_CNT_MAX                    32
#define H264_MAX_NUM_FIELD_PARAMS           3
#define	H264_BYTES_IN_128_BITS              16
#define	H264_MAX_DEC_REF_PIC_MARK           68
#define H264_MAX_NUM_SPARE_PICS             16
#define H264_MAX_PICSIZEINMAPUNITS          8196
#define	H264_MAX_NUM_SUB_SEQ                256
#define H264_MAX_NUM_SLICE_GROUPS           8
#define H264_MAX_NUM_FGS_COMP_MODELS        3
#define H264_MAX_NUM_FGS_INTEN_INTS         256
#define H264_MAX_NUM_FGS_MODEL_VALS         6

/* User data maximum sizes - could be altered */
#define	H264_MAX_ITUTT35_BYTES			    32
#define	H264_MAX_UNREGISTERED_BYTES		    128


// --- ENABLE/DISABLE Sei Payload Parsing per Type ---
#define PARSE_SEI_BUFFERING_PERIOD
#define PARSE_SEI_PIC_TIMING
#define PARSE_SEI_PAN_SCAN_RECT
//#define PARSE_SEI_FILLER_PAYLOAD
#define PARSE_SEI_USER_DATA_REGISTERED_ITU_T_T35
#define PARSE_SEI_USER_DATA_UNREGISTERED
#define PARSE_SEI_RECOVERY_POINT
#define PARSE_SEI_DEC_REF_PIC_MARKING_REPETITION
//#define PARSE_SEI_SPARE_PIC
#define PARSE_SEI_SCENE_INFO
#define PARSE_SEI_SUB_SEQ_INFO
#define PARSE_SEI_SUB_SEQ_LAYER_CHARACTERISTICS
#define PARSE_SEI_SUB_SEQ_CHARACTERISTICS
#define PARSE_SEI_FULL_FRAME_FREEZE
#define PARSE_SEI_FULL_FRAME_FREEZE_RELEASE
#define PARSE_SEI_FULL_FRAME_SNAPSHOT
#define PARSE_SEI_PROGRESSIVE_REFINEMENT_SEGMENT_START
#define PARSE_SEI_PROGRESSIVE_REFINEMENT_SEGMENT_END
//#define PARSE_SEI_MOTION_CONSTRAINED_SLICE_GROUP_SET      // Cannot be parsed properly due to out-of-order parsing requirements
//#define PARSE_SEI_FILM_GRAIN_CHARACTERISTICS
#define PARSE_SEI_DEBLOCKING_FILTER_DISPLAY_PREFERENCE
#define PARSE_SEI_STEREO_VIDEO_INFO
//#define PARSE_SEI_POST_FILTER_HINT
//#define PARSE_SEI_TONE_MAPPING_INFO                       // Huge size, activate at your own risk (384KBytes+)
// ---------------------------------------------------


/*!
******************************************************************************
 All SEI parameters
******************************************************************************/
typedef enum {
    SEI_BUFFERING_PERIOD                        =   0,
    SEI_PIC_TIMING                              =   1,
    SEI_PAN_SCAN_RECT                           =   2,
    SEI_FILLER_PAYLOAD                          =   3,
    SEI_USER_DATA_REGISTERED_ITU_T_T35          =   4,
    SEI_USER_DATA_UNREGISTERED                  =   5,
    SEI_RECOVERY_POINT                          =   6,
    SEI_DEC_REF_PIC_MARKING_REPETITION          =   7,
    SEI_SPARE_PIC                               =   8,
    SEI_SCENE_INFO                              =   9,
    SEI_SUB_SEQ_INFO                            =   10,
    SEI_SUB_SEQ_LAYER_CHARACTERISTICS           =   11,
    SEI_SUB_SEQ_CHARACTERISTICS                 =   12,
    SEI_FULL_FRAME_FREEZE                       =   13,
    SEI_FULL_FRAME_FREEZE_RELEASE               =   14,
    SEI_FULL_FRAME_SNAPSHOT                     =   15,
    SEI_PROGRESSIVE_REFINEMENT_SEGMENT_START    =   16,
    SEI_PROGRESSIVE_REFINEMENT_SEGMENT_END      =   17,
    SEI_MOTION_CONSTRAINED_SLICE_GROUP_SET      =   18,
    SEI_FILM_GRAIN_CHARACTERISTICS              =   19,
    SEI_DEBLOCKING_FILTER_DISPLAY_PREFERENCE    =   20,
    SEI_STEREO_VIDEO_INFO                       =   21,
    SEI_POST_FILTER_HINT                        =   22,
    SEI_TONE_MAPPING_INFO                       =   23,
    /* MVC SEI*/
    SEI_SCALABLE                                =   24,
    SEI_SUB_PIC                                 =   25,
    SEI_NON_REQUIRED                            =   26,
    SEI_SCALABLE_LAYERS_NOT_PRESENT             =   28,
    SEI_SCALABLE_DEPENDENCY_CHANGE              =   29,
    SEI_SCALABLE_NESTING                        =   30,
    SEI_PARALLEL_DEC                            =   36,
    SEI_VIEW_SCALABILITY_INFO                   =   38, 
    SEI_MULTIVIEW_SCENE_INFO                    =   39,
    SEI_MULTIVIEW_ACQUISITION_INFO              =   40, 
    SEI_NON_REQ_VIEW_INFO                       =   41, 
    SEI_VIEW_DEPENDENCY_STRUCTURE               =   42,  
    SEI_OP_NOT_PRESENT                          =   43, 
    SEI_QUALITYLEVEL                            =   45,
    SEI_RESERVED                                =   46,
    SEI_MAX_ELEMENTS  //!< number of maximum syntax elements
} BSPP_eSEIType;


/*!
******************************************************************************

 BSPP_sH264SeiBufferingPeriod data structure

******************************************************************************/
typedef struct
{
    IMG_BOOL    bInUseFlag;

    IMG_UINT32  seq_parameter_set_id;
    IMG_UINT32  initial_cpb_removal_delay[ H264_CPB_CNT_MAX ];
    IMG_UINT32  initial_cpb_removal_delay_offset[ H264_CPB_CNT_MAX ];

} BSPP_sH264SeiBufferingPeriod;


/*!
******************************************************************************

 BSPP_sH264SeiPicTiming data structure

******************************************************************************/
typedef struct
{
    IMG_BOOL    bInUseFlag;

    IMG_UINT32  cpb_removal_delay;
    IMG_UINT32  dpb_output_delay;
    IMG_UINT32  pic_struct;
    IMG_UINT32  clock_timestamp_flag[H264_MAX_NUM_FIELD_PARAMS];
    IMG_UINT32  ct_type[H264_MAX_NUM_FIELD_PARAMS];
    IMG_BOOL    nuit_field_based_flag[H264_MAX_NUM_FIELD_PARAMS];
    IMG_UINT32  counting_type[H264_MAX_NUM_FIELD_PARAMS];
    IMG_BOOL    full_timestamp_flag[H264_MAX_NUM_FIELD_PARAMS];
    IMG_BOOL    discontinuity_flag[H264_MAX_NUM_FIELD_PARAMS];
    IMG_BOOL    cnt_dropped_flag[H264_MAX_NUM_FIELD_PARAMS];
    IMG_UINT32  n_frames[H264_MAX_NUM_FIELD_PARAMS];
    IMG_UINT32  seconds_value[H264_MAX_NUM_FIELD_PARAMS];
    IMG_UINT32  minutes_value[H264_MAX_NUM_FIELD_PARAMS];
    IMG_UINT32  hours_value[H264_MAX_NUM_FIELD_PARAMS];
    IMG_BOOL    seconds_flag[H264_MAX_NUM_FIELD_PARAMS];
    IMG_BOOL    minutes_flag[H264_MAX_NUM_FIELD_PARAMS];
    IMG_BOOL    hours_flag[H264_MAX_NUM_FIELD_PARAMS];
    IMG_INT32   time_offset[H264_MAX_NUM_FIELD_PARAMS];

} BSPP_sH264SeiPicTiming;

/*!
******************************************************************************

 BSPP_sH264SeiPanScanRect data structure

******************************************************************************/
typedef struct
{
    IMG_BOOL	bInUseFlag;

    IMG_UINT32  pan_scan_rect_id;
    IMG_BOOL    pan_scan_rect_cancel_flag;
    IMG_UINT32  pan_scan_cnt_minus1;
    IMG_INT32   pan_scan_rect_left_offset[H264_MAX_NUM_FIELD_PARAMS];
    IMG_INT32   pan_scan_rect_right_offset[H264_MAX_NUM_FIELD_PARAMS];
    IMG_INT32   pan_scan_rect_top_offset[H264_MAX_NUM_FIELD_PARAMS];
    IMG_INT32   pan_scan_rect_bottom_offset[H264_MAX_NUM_FIELD_PARAMS];
    IMG_UINT32  pan_scan_rect_repetition_period;

} BSPP_sH264SeiPanScanRect;

/*!
******************************************************************************

 BSPP_sH264SeiFillerPayload data structure

******************************************************************************/
typedef struct
{
    IMG_BOOL    bInUseFlag;

} BSPP_sH264SeiFillerPayload;


/*!
******************************************************************************

 data structure

******************************************************************************/
typedef struct
{
    IMG_BOOL    bInUseFlag;

    IMG_UINT8   itu_t_t35_country_code;
    IMG_UINT8   itu_t_t35_country_code_extension_byte;
    IMG_CHAR   itu_t_t35_payload_byte[H264_MAX_ITUTT35_BYTES];

} BSPP_sH264SeiUserDataRegisteredItuTT35;


/*!
******************************************************************************

 BSPP_sH264SeiUserDataUnregistered data structure

******************************************************************************/
typedef struct
{
    IMG_BOOL    bInUseFlag;

    IMG_CHAR   uuid_iso_iec_11578[H264_BYTES_IN_128_BITS];
    IMG_CHAR   user_data_payload_byte[H264_MAX_UNREGISTERED_BYTES];

} BSPP_sH264SeiUserDataUnregistered;


/*!
******************************************************************************

 BSPP_sH264SeiRecoveryPoint data structure

******************************************************************************/
typedef struct
{
    IMG_BOOL    bInUseFlag;

    IMG_INT32   recovery_frame_cnt;
    IMG_BOOL    exact_match_flag;
    IMG_BOOL    broken_link_flag;
    IMG_INT32   changing_slice_group_idc;

} BSPP_sH264SeiRecoveryPoint;


/*!
******************************************************************************

 BSPP_sH264SeiDecRefPicMarkingRepetition data structure

******************************************************************************/
typedef struct
{
    IMG_BOOL    bInUseFlag;

    IMG_BOOL    original_idr_flag;
    IMG_UINT32  original_frame_num;
    IMG_BOOL    original_field_pic_flag;
    IMG_BOOL    original_bottom_field_flag;

    /* dec_ref_picture_marking */
    IMG_BOOL    no_output_of_prior_pics_flag;
    IMG_BOOL    long_term_reference_flag;
    IMG_BOOL    adaptive_ref_pic_marking_mode_flag;

    IMG_UINT32  memory_management_control_operation[H264_MAX_DEC_REF_PIC_MARK];
    IMG_UINT32  difference_of_pic_nums_minus1[H264_MAX_DEC_REF_PIC_MARK];
    IMG_UINT32  long_term_pic_num[H264_MAX_DEC_REF_PIC_MARK];
    IMG_UINT32  long_term_frame_idx[H264_MAX_DEC_REF_PIC_MARK];
    IMG_UINT32  max_long_term_frame_idx_plus1[H264_MAX_DEC_REF_PIC_MARK];

} BSPP_sH264SeiDecRefPicMarkingRepetition;


/*!
******************************************************************************

 BSPP_sH264SeiSparePic data structure

******************************************************************************/
typedef struct
{
    IMG_BOOL    bInUseFlag;

    IMG_UINT32  target_frame_num;
    IMG_BOOL    spare_field_flag;
    IMG_BOOL    target_bottom_field_flag;
    IMG_UINT32  num_spare_pics_minus1;
    IMG_UINT32  delta_spare_frame_num[H264_MAX_NUM_SPARE_PICS];
    IMG_BOOL    spare_bottom_field_flag[H264_MAX_NUM_SPARE_PICS];
    IMG_UINT32  spare_area_idc[H264_MAX_NUM_SPARE_PICS];
    union {
        /* spare_area_idc == 1 */
        IMG_BOOL    spare_unit_flag[H264_MAX_NUM_SPARE_PICS][H264_MAX_PICSIZEINMAPUNITS];
        /* spare_area_idc == 2 */
        IMG_UINT32  zero_run_length[H264_MAX_NUM_SPARE_PICS][H264_MAX_PICSIZEINMAPUNITS];
    } spare_area;

} BSPP_sH264SeiSparePic;


/*!
******************************************************************************

 BSPP_sH264SeiSceneInfo data structure

******************************************************************************/
typedef struct
{
    IMG_BOOL    bInUseFlag;

    IMG_BOOL    scene_info_present_flag;
    IMG_UINT32  scene_id;
    IMG_UINT32  scene_transition_type;
    IMG_UINT32  second_scene_id;

} BSPP_sH264SeiSceneInfo;


/*!
******************************************************************************

 BSPP_sH264SeiSubSeqInfo data structure

******************************************************************************/
typedef struct
{
    IMG_BOOL    bInUseFlag;

    IMG_UINT32  sub_seq_layer_num;
    IMG_UINT32  sub_seq_id;
    IMG_BOOL    first_ref_pic_flag;
    IMG_BOOL    leading_non_ref_pic_flag;
    IMG_BOOL    last_pic_flag;
    IMG_BOOL    sub_seq_frame_num_flag;
    IMG_UINT32  sub_seq_frame_num;

} BSPP_sH264SeiSubSeqInfo;


/*!
******************************************************************************

 BSPP_sH264SeiSubSeqLayerCharacteristics data structure

******************************************************************************/
typedef struct
{
    IMG_BOOL    bInUseFlag;

    IMG_UINT32  num_sub_seq_layers_minus1;
    IMG_BOOL    accurate_statistics_flag[H264_MAX_NUM_SUB_SEQ];
    IMG_UINT32  average_bit_rate[H264_MAX_NUM_SUB_SEQ];
    IMG_UINT32  average_frame_rate[H264_MAX_NUM_SUB_SEQ];

} BSPP_sH264SeiSubSeqLayerCharacteristics;


/*!
******************************************************************************

 BSPP_sH264SeiSubSeqCharacteristics data structure

******************************************************************************/
typedef struct
{
    IMG_BOOL    bInUseFlag;

    IMG_UINT32  sub_seq_layer_num;
    IMG_UINT32  sub_seq_id;
    IMG_BOOL    duration_flag;
    IMG_UINT32  sub_seq_duration;
    IMG_BOOL    average_rate_flag;
    IMG_BOOL    accurate_statistics_flag;
    IMG_UINT32  average_bit_rate;
    IMG_UINT32  average_frame_rate;
    IMG_UINT32  num_referenced_subseqs;
    IMG_UINT32  ref_sub_seq_layer_num[H264_MAX_NUM_SUB_SEQ];
    IMG_UINT32  ref_sub_seq_id[H264_MAX_NUM_SUB_SEQ];
    IMG_UINT32  ref_sub_seq_direction[H264_MAX_NUM_SUB_SEQ];

} BSPP_sH264SeiSubSeqCharacteristics;


/*!
******************************************************************************

 BSPP_sH264SeiFullFrameFreeze data structure

******************************************************************************/
typedef struct
{
    IMG_BOOL    bInUseFlag;

    IMG_UINT32  full_frame_freeze_repetition_period;

} BSPP_sH264SeiFullFrameFreeze;


/*!
******************************************************************************

 BSPP_sH264SeiFullFrameFreezeRelease data structure

******************************************************************************/
typedef struct
{
    IMG_BOOL    bInUseFlag;

    /* no data payload */

} BSPP_sH264SeiFullFrameFreezeRelease;


/*!
******************************************************************************

 BSPP_sH264SeiFullFrameSnapshot data structure

******************************************************************************/
typedef struct
{
    IMG_BOOL    bInUseFlag;

    IMG_UINT32  snapshot_id;

} BSPP_sH264SeiFullFrameSnapshot;


/*!
******************************************************************************

 BSPP_sH264SeiProgressiveRefinementSegmentStart data structure

******************************************************************************/
typedef struct
{
    IMG_BOOL    bInUseFlag;

    IMG_UINT32  progressive_refinement_id;
    IMG_UINT32  num_refinement_steps_minus1;

} BSPP_sH264SeiProgressiveRefinementSegmentStart;


/*!
******************************************************************************

 BSPP_sH264SeiProgressiveRefinementEnd data structure

******************************************************************************/
typedef struct
{
    IMG_BOOL    bInUseFlag;

    IMG_UINT32  progressive_refinement_id;

} BSPP_sH264SeiProgressiveRefinementSegmentEnd;


/*!
******************************************************************************

 BSPP_sH264SeiMotionConstrainedSliceGroupSet data structure

******************************************************************************/
typedef struct
{
    IMG_BOOL    bInUseFlag;

    IMG_UINT32  num_slice_groups_in_set_minus1;
    IMG_UINT32  slice_group_id[H264_MAX_NUM_SLICE_GROUPS];
    IMG_BOOL    exact_sample_value_match_flag;
    IMG_BOOL    pan_scan_rect_flag;
    IMG_UINT32  pan_scan_rect_id;

} BSPP_sH264SeiMotionConstrainedSliceGroupSet;


/*!
******************************************************************************

 BSPP_sH264SeiFilmGrainCharacteristics data structure

******************************************************************************/
typedef struct
{
    IMG_BOOL    bInUseFlag;

    IMG_BOOL    film_grain_characteristics_cancel_flag;
    IMG_UINT32  model_id;
    IMG_BOOL    separate_colour_description_present_flag;
    IMG_UINT32  film_grain_bit_depth_luma_minus8;
    IMG_UINT32  film_grain_bit_depth_chroma_minus8;
    IMG_BOOL    film_grain_full_range_flag;
    IMG_UINT32  film_grain_colour_primaries;
    IMG_UINT32  film_grain_transfer_characteristics;
    IMG_UINT32  film_grain_matrix_coefficients;
    IMG_UINT32  blending_mode_id;
    IMG_UINT32  log2_scale_factor;
    IMG_BOOL    comp_model_present_flag[H264_MAX_NUM_FGS_COMP_MODELS];
    IMG_UINT32  num_intensity_intervals_minus1[H264_MAX_NUM_FGS_COMP_MODELS];
    IMG_UINT32  num_model_values_minus1[H264_MAX_NUM_FGS_COMP_MODELS];
    IMG_UINT8   intensity_interval_lower_bound[H264_MAX_NUM_FGS_COMP_MODELS][H264_MAX_NUM_FGS_INTEN_INTS];
    IMG_UINT8   intensity_interval_upper_bound[H264_MAX_NUM_FGS_COMP_MODELS][H264_MAX_NUM_FGS_INTEN_INTS];
    IMG_INT16   comp_model_value[H264_MAX_NUM_FGS_COMP_MODELS][H264_MAX_NUM_FGS_INTEN_INTS][H264_MAX_NUM_FGS_MODEL_VALS];
    IMG_UINT32  film_grain_characteristics_repetition_period;

} BSPP_sH264SeiFilmGrainCharacteristics;


/*!
******************************************************************************

 BSPP_sH264SeiDeblockingFilterDisplayPreference data structure

******************************************************************************/
typedef struct
{
    IMG_BOOL    bInUseFlag;

    IMG_BOOL    deblocking_display_preference_cancel_flag;
    IMG_BOOL    display_prior_to_deblocking_preferred_flag;
    IMG_BOOL    dec_frame_buffering_constraint_flag;
    IMG_UINT32  deblocking_display_preference_repetition_period;

} BSPP_sH264SeiDeblockingFilterDisplayPreference;


/*!
******************************************************************************

 BSPP_sH264SeiStereoVideoInfo data structure

******************************************************************************/
typedef struct
{
    IMG_BOOL    bInUseFlag;

    IMG_BOOL    field_views_flag;
    IMG_BOOL    top_field_is_left_view_flag;
    IMG_BOOL    current_frame_is_left_view_flag;
    IMG_BOOL    next_frame_is_second_view_flag;
    IMG_BOOL    left_view_self_contained_flag;
    IMG_BOOL    right_view_self_contained_flag;

} BSPP_sH264SeiStereoVideoInfo;


/*!
******************************************************************************

 BSPP_sH264SeiPostFilterHint data structure

******************************************************************************/
typedef struct
{
    IMG_BOOL    bInUseFlag;

    IMG_UINT32  filter_hint_size_y;
    IMG_UINT32  filter_hint_size_x;
    IMG_UINT8   filter_hint_type;
    IMG_INT32   filter_hint[3][15][15];
    IMG_BOOL    additional_extension_flag;

} BSPP_sH264SeiPostFilterHint;


/*!
******************************************************************************

 BSPP_sH264SeiToneMappingInfo data structure

******************************************************************************/
typedef struct
{
    IMG_BOOL    bInUseFlag;

    IMG_UINT32  tone_map_id;
    IMG_BOOL    tone_map_cancel_flag;
    IMG_UINT32  tone_map_repetition_period;
    IMG_UINT8   coded_data_bit_depth;
    IMG_UINT8   target_bit_depth;
    IMG_UINT32  model_id;
    IMG_UINT32  min_value;
    IMG_UINT32  max_value;
    IMG_UINT32  sigmoid_midpoint;
    IMG_UINT32  sigmoid_width;
    IMG_UINT16  start_of_coded_interval[64*1024];
    IMG_UINT16  num_pivots;
    IMG_UINT16  coded_pivot_value[64*1024];
    IMG_UINT16  target_pivot_value[64*1024];

} BSPP_sH264SeiToneMappingInfo;


/*!
******************************************************************************

 BSPP_sH264SEIInfo is a structure containing all selected SEI messages and can be
 associated with a picture. No info from this structure should be used directly
 for decoding outside the BSPP, if needed copy them externally and use them from 
 there.

******************************************************************************/
typedef struct
{
#ifdef PARSE_SEI_BUFFERING_PERIOD
    BSPP_sH264SeiBufferingPeriod                    sSeiBufferingPeriod;
#endif
#ifdef PARSE_SEI_PIC_TIMING
    BSPP_sH264SeiPicTiming                          sSeiPicTiming;
#endif
#ifdef PARSE_SEI_PAN_SCAN_RECT
    BSPP_sH264SeiPanScanRect                        sSeiPanScanRect;
#endif
#ifdef PARSE_SEI_FILLER_PAYLOAD
    BSPP_sH264SeiFillerPayload                      sSeiFillerPayload;
#endif
#ifdef PARSE_SEI_USER_DATA_REGISTERED_ITU_T_T35
    BSPP_sH264SeiUserDataRegisteredItuTT35          sSeiUserDataRegisteredItuTT35;
#endif
#ifdef PARSE_SEI_USER_DATA_UNREGISTERED
    BSPP_sH264SeiUserDataUnregistered               sSeiUserDataUnregistered;
#endif
#ifdef PARSE_SEI_RECOVERY_POINT
    BSPP_sH264SeiRecoveryPoint                      sSeiRecoveryPoint;
#endif
#ifdef PARSE_SEI_DEC_REF_PIC_MARKING_REPETITION
    BSPP_sH264SeiDecRefPicMarkingRepetition         sSeiDecRefPicMarkingRepetition;
#endif
#ifdef PARSE_SEI_SPARE_PIC
    BSPP_sH264SeiSparePic                           sSeiSparePic;
#endif
#ifdef PARSE_SEI_SCENE_INFO
    BSPP_sH264SeiSceneInfo                          sSeiSceneInfo;
#endif
#ifdef PARSE_SEI_SUB_SEQ_INFO
    BSPP_sH264SeiSubSeqInfo                         sSeiSubSeqInfo;
#endif
#ifdef PARSE_SEI_SUB_SEQ_LAYER_CHARACTERISTICS
    BSPP_sH264SeiSubSeqLayerCharacteristics         sSeiSubSeqLayerCharacteristics;
#endif
#ifdef PARSE_SEI_SUB_SEQ_CHARACTERISTICS
    BSPP_sH264SeiSubSeqCharacteristics              sSeiSubSeqCharacteristics;
#endif
#ifdef PARSE_SEI_FULL_FRAME_FREEZE
    BSPP_sH264SeiFullFrameFreeze                    sSeiFullFrameFreeze;
#endif
#ifdef PARSE_SEI_FULL_FRAME_FREEZE_RELEASE
    BSPP_sH264SeiFullFrameFreezeRelease             sSeiFullFrameFreezeRelease;
#endif
#ifdef PARSE_SEI_FULL_FRAME_SNAPSHOT
    BSPP_sH264SeiFullFrameSnapshot                  sSeiFullFrameSnapshot;
#endif
#ifdef PARSE_SEI_PROGRESSIVE_REFINEMENT_SEGMENT_START
    BSPP_sH264SeiProgressiveRefinementSegmentStart  sSeiProgressiveRefinementSegmentStart;
#endif
#ifdef PARSE_SEI_PROGRESSIVE_REFINEMENT_SEGMENT_END
    BSPP_sH264SeiProgressiveRefinementSegmentEnd    sSeiProgressiveRefinementSegmentEnd;
#endif
#ifdef PARSE_SEI_MOTION_CONSTRAINED_SLICE_GROUP_SET
    BSPP_sH264SeiMotionConstrainedSliceGroupSet     sSeiMotionConstrainedSliceGroupSet;
#endif
#ifdef PARSE_SEI_FILM_GRAIN_CHARACTERISTICS
    BSPP_sH264SeiFilmGrainCharacteristics           sSeiFilmGrainCharacteristics;
#endif
#ifdef PARSE_SEI_DEBLOCKING_FILTER_DISPLAY_PREFERENCE
    BSPP_sH264SeiDeblockingFilterDisplayPreference  sSeiDeblockingFilterDisplayPreference;
#endif
#ifdef PARSE_SEI_STEREO_VIDEO_INFO
    BSPP_sH264SeiStereoVideoInfo                    sSeiStereoVideoInfo;
#endif
#ifdef PARSE_SEI_POST_FILTER_HINT
    BSPP_sH264SeiPostFilterHint                     sSeiPostFilterHint;
#endif
#ifdef PARSE_SEI_TONE_MAPPING_INFO
    BSPP_sH264SeiToneMappingInfo                    sSeiToneMappingInfo;
#endif

} BSPP_sH264SEIInfo;


/*!
******************************************************************************

 @Function              BSPP_H264SEIParser
 
 @Descripton            Parse SEI messages

******************************************************************************/
BSPP_eErrorType BSPP_H264SEIParser(
    IMG_HANDLE                hSwSrContext,
    IMG_HANDLE                hStrRes,
    BSPP_sH264SEIInfo       * psH264SEIInfo,
    IMG_UINT32              * pui32ActiveSpsForSeiParsing
);


#endif
/*__SEI_PARSER__H__*/
