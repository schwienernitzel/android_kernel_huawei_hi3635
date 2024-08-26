/*!
 *****************************************************************************
 *
 * @File       bspp_int.h
 * @Title      VXD Bitstream Buffer Pre-Parser Internal
 * @Description    This file contains the internal structures and function prototypes
 *  for the VXD Bitstream Buffer Pre-Parser.
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

#ifndef __BSPP_INT_H__
#define __BSPP_INT_H__

#include "img_defs.h"

#include "vdec_api.h"
#include "vdec_params.h"
#include "img_profiles_levels.h"
#include "bspp.h"
#include "swsr.h"

#ifdef SECURE_MEDIA_REPORTING
#include "report_api.h"
#define SECURE_REPORT_VAR(decl) decl
#else
#ifdef REPORT
#undef REPORT
#endif /* REPORT */
#define REPORT(MODULE, LEVEL, fmt, ...)
#ifdef DEBUG_REPORT
#undef DEBUG_REPORT
#endif /* DEBUG_REPORT */
#define DEBUG_REPORT(MODULE, fmt, ...)
#define SECURE_REPORT_VAR(decl)
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define VDEC_MB_DIMENSION  (16)
#define MAX_COMPONENTS  (4)

#define print_value(a, ...)

/*!
******************************************************************************
 This macro aligns the given size to the provided alignment.
******************************************************************************/
#define VDEC_ALIGN_SIZE(size, alignment) (((size) + (alignment) - 1) & ~((alignment)-1))

//#define BSPP_LOG

#if defined(BSPP_LOG) && !defined (BSPP_KM) && defined (BSPP_UM)
extern FILE * fpLog;

#define BSPP_LOG_PRINT(format, field, value)          fprintf(fpLog, format, field, value); fflush(fpLog);
//#define BSPP_LOG(format, field, value)          DEBUG_REPORT(REPORT_MODULE_BSPP, format, field, value)

#define BSPP_LOG_INT(field, value)              BSPP_LOG_PRINT("%s: %d\n", field, value)
#define BSPP_LOG_FLOAT(field, value)            BSPP_LOG_PRINT("%s: %f\n", field, value)
#define BSPP_LOG_DECLARE(decl)                  decl

#else

#define BSPP_LOG_PRINT(format, field, value)
//#define BSPP_LOG(format, field, value)

#define BSPP_LOG_INT(field, value)
#define BSPP_LOG_FLOAT(field, value)
#define BSPP_LOG_DECLARE(decl)

#endif



#define BSPP_DEFAULT_SEQUENCE_ID   (0)

typedef enum
{
    BSPP_UNIT_NONE = 0,

    BSPP_UNIT_VPS_HEVC,
    BSPP_UNIT_SEQUENCE,
    BSPP_UNIT_PPS_H264,
    BSPP_UNIT_PPS_HEVC,
    BSPP_UNIT_PICTURE,
    BSPP_UNIT_SKIP_PICTURE,
    BSPP_UNIT_NON_PICTURE,      /*!< Data from these units should be placed in non-picture bitstream segment lists. In conformant
                                     streams these units should not occur in-between the picture data.                              */
    BSPP_UNIT_UNCLASSIFIED,     /*!< Classification of unit is based on previous unit type. Since these units can occur anywhere
                                     in the bitstream their data must be added to the current bitstream segment list. Consequently
                                     they may be passed downstream as picture data (when strictly they are not required for
                                     decoding but this reduces/constrains the number of bitstream segments.                         */
    BSPP_UNIT_UNSUPPORTED,      /*!< Unit is unsupported, don't change segment list.                                                */

    BSPP_UNIT_MAX

} BSPP_eUnitType;


/*!
******************************************************************************
 This enumeration defines the stream codec type
******************************************************************************/
typedef enum
{
    WMV1_CODEC      = 0,    //!< unsupported
    MP43_CODEC      = 1,    //!< unsupported
    WMV2_CODEC      = 2,    //!< unsupported
    MP42_CODEC      = 3,    //!< unsupported
    MP4S_CODEC      = 4,    //!< unsupported
    WMV9_CODEC      = 5,    //!< WMV9 codec
    WMVA_CODEC      = 6,    //!< Advanced WMV9 Codec ???
    WVP2_CODEC      = 7,    //!< ???
    VC1_CODEC       = 8,    //!< VC1 Codec
    CODEC_UNDEFINED = 9,    //!< Undefined Codec

} WMF_eCodecType;


/*** 6.2.3 Picture header ***/
typedef struct
{
    IMG_UINT16 temporal_reference;
    IMG_UINT8 picture_coding_type;
    IMG_UINT16 vbv_delay;
    IMG_UINT8 full_pel_forward_vector;
    IMG_UINT8 forward_f_code;
    IMG_UINT8 full_pel_backward_vector;
    IMG_UINT8 backward_f_code;

} BSPP_sMpeg2PictHdr;


typedef enum
{
    AVS_CODING_TYPE_I = 0,
    AVS_CODING_TYPE_P,
    AVS_CODING_TYPE_B,
    AVS_CODING_TYPE_MAX

} BSPP_eAvsCodingType;


///****** 7.1.3.1-2 AVS  Picture header   *************/
typedef struct
{
    IMG_UINT16  ui16bbv_delay;                  // bbv_delay u(16)
    BSPP_eAvsCodingType epicture_coding_type;   // picture_coding_type u(2)
    IMG_UINT8   ui8picture_distance;            // picture_distance u(8)
    IMG_UINT32  ui32bbv_check_times;            // bbv_check_times ue(v)
    IMG_UINT8   ui8progressive_frame;           // progressive_frame u(1)
    IMG_UINT8   ui8picture_structure;           // picture_structure u(1)
    IMG_UINT8   ui8top_field_first;             // top_field_first u(1)
    IMG_UINT8   ui8repeat_first_field;          // repeat_first_field u(1)
    IMG_UINT8   ui8fixed_picture_qp;            // fixed_picture_qp u(1)
    IMG_UINT8   ui8picture_qp;                  // picture_qp u(6)
    IMG_UINT8   ui8skip_mode_flag;              // skip_mode_flag u(1)

    IMG_UINT8   ui8loop_filter_disable;         // loop_filter_disable u(1)
    IMG_UINT8   ui8loop_filter_parameter_flag;  // loop_filter_parameter_flag u(1)
    IMG_INT32   i32alpha_c_offset;              // alpha_c_offset se(v)
    IMG_INT32   i32beta_offset;                 // beta_offset se(v)
    /* I-frames only */
    IMG_UINT8   ui8time_code_flag;              // time_code_flag u(1)
    IMG_UINT32  ui32time_code;                  // time_code u(24)
    /* P/B-frames only */
    IMG_UINT8   ui8advanced_pred_mode_disable;  // advanced_pred_mode_disable u(1)
    IMG_UINT8   ui8picture_reference_flag;      // picture_reference_flag u(1)
    IMG_UINT8   ui8no_forward_reference_flag;   // no_forward_reference_flag u(1)

} BSPP_sAvsPictHdr;


/*!
******************************************************************************
 This structure contains the VC1 picture header information.
 @brief  VC-1 Picture Header Information
******************************************************************************/
typedef struct
{
    IMG_UINT8       FCM;                                        /*!< 7.1.1.15 Frame Coding Mode (FCM)                           */
    IMG_UINT8       PTYPE;                                      /*!< 7.1.1.4 Picture Type (PTYPE)                               */
    IMG_UINT8       FPTYPE;                                     /*!< 9.1.1.42 Field Picture Type (FPTYPE)                       */

    IMG_BOOL        INTERPFRM;                                  /*!< 7.1.1.1 Frame Interpolation Hint (INTERPFRM)               */
    IMG_UINT8       FRMCNT;                                     /*!< 7.1.1.2 Frame Count (FRMCNT)                               */
    IMG_BOOL        RANGEREDFRM;                                /*!< 7.1.1.3 Range Reduction Frame (RANGEREDFRM)                */
    IMG_UINT8       BFRACTION;                                  /*!< 7.1.1.14 B Picture Fraction (BFRACTION)                    */
    IMG_UINT8       BF;                                         /*!< 7.1.1.5 Buffer Fullness (BF)                               */
    IMG_UINT8       PQINDEX;                                    /*!< 7.1.1.6 Picture Quantizer Index (PQINDEX)                  */
    IMG_BOOL        HALFQP;                                     /*!< 7.1.1.7 Half QP Step (HALFQP)                              */
    IMG_BOOL        PQUANTIZER;                                 /*!< 7.1.1.8 Picture Quantizer Type (PQUANTIZER)                */
    IMG_UINT8       MVRANGE;                                    /*!< 7.1.1.9 Extended MV Range Flag (MVRANGE)                   */
    IMG_UINT8		RESPIC;                                     /*!< 7.1.1.10 Picture Resolution Index (RESPIC)                 */
    IMG_UINT32      TFCNTR;                                     /*!< 7.1.1.16 Temporal Reference Frame Counter (TFCNTR)         */

    IMG_UINT8       RPTFRM;                                     /*!< 7.1.1.19 Repeat Frame Count (RPTFRM)                       */
    IMG_BOOL        TFF;                                        /*!< 7.1.1.17 Top Field First (TFF)                             */
    IMG_BOOL        RFF;                                        /*!< 7.1.1.18 Repeat First Field (RFF)                          */

    IMG_BOOL        PS_PRESENT;                                 /*!< 7.1.1.20 Pan Scan Present Flag (PS_PRESENT)                */
    IMG_UINT32      PS_HOFFSET[VDEC_MAX_PANSCAN_WINDOWS];       /*!< 7.1.1.21 Pan Scan Window Horizontal Offset (PS_HOFFSET)    */
    IMG_UINT32      PS_VOFFSET[VDEC_MAX_PANSCAN_WINDOWS];       /*!< 7.1.1.22 Pan Scan Window Vertical Offset (PS_VOFFSET)      */
    IMG_UINT32      PS_WIDTH[VDEC_MAX_PANSCAN_WINDOWS];         /*!< 7.1.1.23 Pan Scan Window Width (PS_WIDTH)                  */
    IMG_UINT32      PS_HEIGHT[VDEC_MAX_PANSCAN_WINDOWS];        /*!< 7.1.1.24 Pan Scan Window Height (PS_HEIGHT)                */

    IMG_UINT32      ui32NumPanScanWindows;                      /*!< 8.9.1 Number of Pan Scan Windows                           */

} BSPP_sVc1PictHdr;


/*!
******************************************************************************
 This structure contains H264 state to be retained between pictures.
******************************************************************************/
typedef struct
{
    // The following get applied to every picture until updated (bitstream properties)
    IMG_BOOL            bDisableVDMCFilt;
    IMG_BOOL            b4x4TransformMBNotAvailable;

    // The following get applied to the next picture only (picture properties)
    IMG_BOOL            bRepeatFirstField;
    IMG_UINT32          ui32MaxFrmRepeat;
    // Control variable to decide when to attach the SEI info (picture properties) to a picture
    IMG_BOOL            bSEIInfoAttachedToPic;

    // The following variable is an approximation because we cannot parse out-of-order, it takes value as described:
    // 1) Initially it is BSPP_INVALID
    // 2) The first SPS sets it to its SPSid
    // 3) The last BSPP_sH264SeiBufferingPeriod sets it, and it is used for every SEI parsing until updated by
    //    another BSPP_sH264SeiBufferingPeriod message
    IMG_UINT32          ui32ActiveSpsForSeiParsing;
    IMG_UINT16          ui16CurrentViewId;

} BSPP_sH264InterPictCtx;


/*!
******************************************************************************
 This structure contains AVS state to be retained between pictures.
******************************************************************************/
typedef struct
{
    BSPP_sAvsPictHdr    sAvsPictHdr;

} BSPP_sAVSInterPictCtx;


/*!
******************************************************************************
 This structure contains MPEG-2 state to be retained between pictures.
******************************************************************************/
typedef struct
{
    IMG_BOOL            bIsMpeg2;
    IMG_BOOL            bFirstField;
    IMG_BOOL            bSeenSequenceEnd;
    IMG_UINT8           ui8PrevValidProfileLevel;
    IMG_UINT8           ui8PrevValidChromaFormat;

    BSPP_sMpeg2PictHdr  sMpeg2PictHdr;

    BSPP_sPictHdrInfo   sPictHdrInfo;

} BSPP_sMPEG2InterPictCtx;


/*!
******************************************************************************
 This structure contains MPEG-4 state to be retained between pictures.
******************************************************************************/
typedef struct
{
    // Required state to get short headers building.
    IMG_UINT8           ui8PrevSourceFormat;
    IMG_UINT8           optional_custom_pcf;
    IMG_UINT16          width;
    IMG_UINT16          height;

} BSPP_sMPEG4InterPictCtx;


/*!
******************************************************************************
 This structure contains VC1 state to be retained between pictures.
******************************************************************************/
typedef struct
{
    IMG_UINT32          ui32Profile;
    IMG_BOOL            bFirstField;
    IMG_BOOL            bOldSequenceEnded;

    BSPP_sVc1PictHdr    sCurPictHdr;

} BSPP_sVC1InterPictCtx;


/*!
******************************************************************************
 This structure contains VP6 state to be retained between pictures.
******************************************************************************/
typedef struct
{
    // Key frame data.
    IMG_UINT8           ui8VP3VersionNo;
    IMG_UINT8           ui8VPProfile;
    IMG_UINT32          ui32VMacroblockCount;
    IMG_UINT32          ui32HMacroblockCount;
    IMG_UINT32          ui32OutputVMacroblockCount;
    IMG_UINT32          ui32OutputHMacroblockCount;

} BSPP_sVP6InterPictCtx;


/*!
******************************************************************************
 This structure contains VP8 state to be retained between pictures.
******************************************************************************/
typedef struct
{
    // Key frame data.
    IMG_UINT8               ui8Version;
    IMG_UINT32              ui32FrameWidth;
    IMG_UINT32              ui32FrameHeight;

} BSPP_sVP8InterPictCtx;


/*!
******************************************************************************
 This structure contains HEVC state to be retained between pictures.
******************************************************************************/
typedef struct
{
    /* Picture count in a sequence                       */
    IMG_UINT32 ui32SeqPicCount;
    struct
    {
        /* There was EOS NAL detected and no new picture yet */
        unsigned bEOSdetected:1;
        /* This is first picture after EOS NAL               */
        unsigned bFirstAfterEOS:1;
    };

} BSPP_sHEVCInterPictCtx;


/*!
******************************************************************************
 This structure contains JPEG state to be retained between pictures.
******************************************************************************/
typedef struct
{
	IMG_BOOL                bSeenClosedGOP;     // A closed GOP has occured in the bitstream.
    IMG_BOOL                bNewClosedGOP;      // Closed GOP has been signalled by a unit before the next picture.
    IMG_BOOL                bNotDpbFlush;       // Indicates whether or not DPB flush is needed
    LST_T                   sPicPrefixSeg;

    union
    {
        BSPP_sAVSInterPictCtx       sAvsCtx;
        BSPP_sH264InterPictCtx      sH264Ctx;
        BSPP_sHEVCInterPictCtx      sHevcCtx;
        BSPP_sMPEG2InterPictCtx     sMpeg2Ctx;
        BSPP_sMPEG4InterPictCtx     sMpeg4Ctx;
        BSPP_sVC1InterPictCtx       sVc1Ctx;
        BSPP_sVP6InterPictCtx       sVp6Ctx;
        BSPP_sVP8InterPictCtx       sVp8Ctx;
    };

} inter_pict_data;



typedef struct
{
    inter_pict_data * psInterPictCtx;

    // Input/Output (H264 etc. state).
    IMG_UINT32  aui32PrevFirstMBInSlice[MAX_COMPONENTS]; // For SCP ASO detection we need to log 3 components
    BSPP_sPictHdrInfo * psNextPictHdrInfo;
    IMG_UINT8   ui8PrevBottomPicFlag;
    IMG_UINT8   ui8SecondFieldFlag;
    IMG_BOOL8   bNextPicIsNew;
    IMG_UINT32  ui32PrevFrameNum;
    IMG_UINT32  ui32PrevPPSId;
    IMG_UINT32  ui32PrevFieldPicFlag;
    IMG_UINT32  ui32PrevNalRefIdc;
    IMG_UINT32  ui32PrevPicOrderCntLsb;
    IMG_INT32   i32PrevDeltaPicOrderCntBottom;
    IMG_INT32   ai32PrevDeltaPicOrderCnt[2];
    IMG_INT32   iPrevNalUnitType;
    IMG_INT32   ui32PrevIdrPicId;
    IMG_BOOL    bDiscontineousMB;

    // Position in bitstream before parsing a unit
    IMG_UINT64  ui64PrevByteOffsetBuf;
    IMG_UINT32  ui32PrevBufMapId;
    IMG_UINT32  ui32PrevBufDataSize;

    IMG_UINT32  ui32ErrorFlags;         /*!< Flags word to indicate error in parsing/decoding - see #VDEC_eErrorType.           */
    IMG_UINT32  ui32WarningFlags;       /*!< Flags word to indicate warning in parsing/decoding - see #VDEC_eWarningType.       */
    IMG_UINT32  ui32CorrectionFlags;    /*!< Flags word to indicate correction in parsing/decoding - see #VDEC_eCorrectionType. */// Error/warning/correction flags?

    // Outputs.
    IMG_BOOL    bNewClosedGOP;
    IMG_BOOL8   bNewView;
    IMG_BOOL8   bIsPrefix;

} BSPP_sParseState;


/*!
******************************************************************************
 Sequence Data Handling - Start
 ******************************************************************************/

typedef struct
{
    LST_LINK;

    IMG_UINT32              ui32PPSId;            /*!< PPS Id.                                INSECURE MEMORY HOST */
    IMG_UINT32              ui32RefCount;         /*!< Reference count for PPS.               INSECURE MEMORY HOST */

    BSPP_sDdBufArrayInfo    sFWPPS;               /*!< FW needed info.                        SECURE MEMORY DEVICE */

	IMG_UINT32              ui32BufMapId;         /*!< Buffer ID to be used in Kernel                              */
    IMG_HANDLE              hSecurePPSInfo;       /*!< Parsing Info.                          SECURE MEMORY HOST   */
    IMG_UINT32              ui32BufOffset;        /*!< Buffer Offeset to be used in kernel                         */

} BSPP_sPPSInfo;


typedef struct
{
    LST_LINK;

    IMG_UINT32              ui32RefCount;         /*!< Reference count for sequence header.   INSECURE MEMORY HOST */

    BSPP_sSequHdrInfo       sSequHdrInfo;

    BSPP_sDdBufArrayInfo    sFWSequence;          /*!< FW needed info.                        SECURE MEMORY DEVICE */

    IMG_HANDLE              hSecureSequenceInfo;  /*!< Parsing Info.                          SECURE MEMORY HOST */

} BSPP_sSequenceHdrInfo;


typedef struct
{
    LST_LINK;

    IMG_UINT32              ui32VpsId;            /*!< VPS Id.                                INSECURE MEMORY HOST */
    IMG_UINT32              ui32RefCount;         /*!< Reference count for video header.      INSECURE MEMORY HOST */

//    BSPP_sDdBufInfo           sFWVps;               /*!< FW needed info.                        SECURE MEMORY DEVICE */
    IMG_HANDLE              hSecureVpsInfo;       /*!< Parsing Info.                          SECURE MEMORY HOST */

} BSPP_sVpsInfo;


typedef enum
{
    BSPP_UNALLOCATED = 0,
    BSPP_AVAILABLE,
    BSPP_UNAVAILABLE,
    BSPP_STATUSMAX

} BSPP_eElementStatus;


/*!
******************************************************************************
 Sequence Data Handling - End
 ******************************************************************************/
typedef struct
{
    // Input.
    BSPP_eUnitType              eUnitType;          /*!< Indicates which output data to populate.                           */

    VDEC_eVidStd                eVidStd;            /*!< Video Standard of unit to parse.                                   */

    IMG_BOOL                    bDelimPresent;      /*!< Indicates whether delimiter is present for unit.                   */

    const VDEC_sCodecConfig   * psCodecConfig;      /*!< Codec configuration used by this stream.                           */

    IMG_HANDLE                  hStrRes;            /*!< Use to access last instance of sequence/PPS data of a given ID
                                                         through call to BSPP_GetSequHdr()/BSPP_GetPpsHdr().                */

    IMG_UINT32                  ui32UnitDataSize;   /*<! Needed for calculating the size of the last fragment               */
    // Input/Output.
    BSPP_sParseState          * psParseState;       /*!< Inter-unit parsing state for current group of buffers required
                                                         by unit parser.                                                    */

	// Output
	// eVidStd == VDEC_STD_H263 && BSPP_UNIT_PICTURE.
    BSPP_sSequenceHdrInfo     * psImplSequHdrInfo;  /*!< For Sequence Unit, it should have the extracted Sequence Info,
                                                         for all other cases it should be left unmodified. It may contain
                                                         partially or completely populated fields from previous sequence
                                                         information since some standards repeat or provided sequence data
                                                         in multiple units.                                                 */

    // Union of output data for each of the unit types.
    union
    {
        // BSPP_UNIT_SEQUENCE.
        BSPP_sSequenceHdrInfo * psSequHdrInfo;      /*!< For Sequence Unit, it should have the extracted Sequence Info,
                                                         for all other cases it should be left unmodified. It may contain
                                                         partially or completely populated fields from previous sequence
                                                         information since some standards repeat or provided sequence data
                                                         in multiple units.                                                 */

        // BSPP_UNIT_PPS_H264.
        BSPP_sPPSInfo         * psPPSInfo;          /*!< Same as above, but for PPS unit.                                   */

        // BSPP_UNIT_PICTURE.
        BSPP_sPictHdrInfo     * psPictHdrInfo;      /*!< For picture it should have the extracted picture header info
                                                         (see struct definition).                                           */
        // BSPP_UNIT_VPS_HEVC.
        BSPP_sVpsInfo         * psVpsInfo;          /*!< For Video Header (HEVC)                                            */

    } out;

    IMG_UINT32                  ui32PictSequHdrId;  /*!< For picture it should give the SequenceHdrId, for anything
                                                         else it should contain BSPP_INVALID. This value is pre-loaded
                                                         with the sequence ID of the last picture.                          */

    // State: output.
    IMG_BOOL                    bSlice;             /*!< Picture unit (BSPP_UNIT_PICTURE) contains slice data. Picture
                                                         header information must be populated once this unit has been
                                                         parsed.                                                            */

    IMG_BOOL                    bExtSlice;          /*!< Current slice belongs to non-base view (MVC only)                  */

    IMG_BOOL                    bNewClosedGOP;      /*!< True if we meet a unit that signifies closed gop, different
                                                         for each standard.                                                 */
    IMG_BOOL                    bSequenceEnd;       /*!< True if the end of a sequence of pictures has been reached.        */

    IMG_BOOL                    bExtractedAllData;  /*!< Extracted all data from unit whereby shift-register should now
                                                         be at the next delimiter or end of data (when byte-aligned).       */

    BSPP_eErrorType             eParseError;        /*!< Indicates the presence of any errors while processing this unit.   */

    IMG_BOOL                    bIntraFrmAsClosedGop; /*!< To turn on/off considering I-Frames as ClosedGop boundaries. */

} BSPP_sUnitData;


extern BSPP_sPPSInfo *
BSPP_GetPpsHdr(
    IMG_HANDLE  hStrRes,
    IMG_UINT32  ui32PpsId
);


extern BSPP_sSequenceHdrInfo *
BSPP_GetSequHdr(
    IMG_HANDLE  hStrRes,
    IMG_UINT32  ui32SequId
);

extern BSPP_sVpsInfo *
BSPP_GetVpsHdr(
    IMG_HANDLE  hStrRes,
    IMG_UINT32  ui32VpsId
);


#ifdef __cplusplus
}
#endif

#endif /* __BSPP_INT_H__   */
