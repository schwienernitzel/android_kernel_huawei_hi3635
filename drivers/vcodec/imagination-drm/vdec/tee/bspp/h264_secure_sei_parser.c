/*!
 *****************************************************************************
 *
 * @File       h264_secure_sei_parser.c
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

#include "h264_secure_sei_parser.h"
#include "h264_secure_parser.h"
#include "swsr.h"
#include "bspp_int.h"


const int aui8PicStructToNumClockTSMapExtended[] = {1, 1, 1, 2, 2, 3, 3, 2, 3, 0, 0, 0, 0, 0, 0, 0};


/*!
******************************************************************************

 @Function          bspp_H264ParseSeiBufferingPeriod

 @Description

******************************************************************************/
static IMG_VOID bspp_H264ParseSeiBufferingPeriod(
    IMG_HANDLE                      hSwSrContext,
    BSPP_sH264SeiBufferingPeriod  * psSeiBufferingPeriod,
    IMG_HANDLE                      hStrRes,
    IMG_UINT32                    * pui32ActiveSpsForSeiParsing
)
{
    IMG_UINT8	SchedSelIdx = 0;
    IMG_UINT32	ui32Width = 0;
    BSPP_sH264VUIInfo * psVUIInfo = IMG_NULL;
    BSPP_sSequenceHdrInfo * psSequHdrInfo = IMG_NULL;
    BSPP_sH264SequHdrInfo * psH264SequHdrInfo = IMG_NULL;

    IMG_ASSERT( hSwSrContext != IMG_NULL );
    IMG_ASSERT( psSeiBufferingPeriod != IMG_NULL );
    IMG_ASSERT( hStrRes != IMG_NULL );
    IMG_ASSERT( pui32ActiveSpsForSeiParsing != IMG_NULL );

    /* set in-use flag */
    psSeiBufferingPeriod->bInUseFlag = IMG_TRUE;

    psSeiBufferingPeriod->seq_parameter_set_id = SWSR_ReadUnsignedExpGoulomb( hSwSrContext );

    *pui32ActiveSpsForSeiParsing = psSeiBufferingPeriod->seq_parameter_set_id;
    
    psSequHdrInfo = BSPP_GetSequHdr( hStrRes, *pui32ActiveSpsForSeiParsing );
    if ( psSequHdrInfo == IMG_NULL )
    {
        // Error case, return early so at least we do not parse out of current payload bounds
        return;
    }
    psH264SequHdrInfo = ( BSPP_sH264SequHdrInfo * )psSequHdrInfo->hSecureSequenceInfo;
    if ( psH264SequHdrInfo == IMG_NULL )
    {
        // Error case, return early so at least we do not parse out of current payload bounds
        return;
    }
    psVUIInfo = &psH264SequHdrInfo->sVUIInfo;

    if ( psVUIInfo->nal_hrd_parameters_present_flag && ( psVUIInfo->sNALHRDParameters.ui32CPBCntMinus1 < 32 ) ) 
    {
        ui32Width = psVUIInfo->sNALHRDParameters.ui32InitialCPBRemovalDelayLengthMinus1 + 1;
        IMG_ASSERT( ui32Width <= 32 );
        if ( ui32Width == 32 )
        {
            for ( SchedSelIdx = 0; SchedSelIdx <= psVUIInfo->sNALHRDParameters.ui32CPBCntMinus1; SchedSelIdx++ ) 
            {
                psSeiBufferingPeriod->initial_cpb_removal_delay[ SchedSelIdx ] = ( SWSR_ReadBits( hSwSrContext, 16 ) << 16 );
                psSeiBufferingPeriod->initial_cpb_removal_delay[ SchedSelIdx ] |= SWSR_ReadBits( hSwSrContext, 16 );
                psSeiBufferingPeriod->initial_cpb_removal_delay_offset[ SchedSelIdx ] = ( SWSR_ReadBits( hSwSrContext, 16 ) << 16 );
                psSeiBufferingPeriod->initial_cpb_removal_delay_offset[ SchedSelIdx ] |= SWSR_ReadBits( hSwSrContext, 16 );
            }
        }
        else
        {
            for (SchedSelIdx = 0; SchedSelIdx <= psVUIInfo->sNALHRDParameters.ui32CPBCntMinus1; SchedSelIdx++) 
            {
                psSeiBufferingPeriod->initial_cpb_removal_delay[ SchedSelIdx ] = SWSR_ReadBits( hSwSrContext, ui32Width );
                psSeiBufferingPeriod->initial_cpb_removal_delay_offset[ SchedSelIdx ] = SWSR_ReadBits( hSwSrContext, ui32Width );
            }
        }
    }
    if ( psVUIInfo->vcl_hrd_parameters_present_flag && ( psVUIInfo->sVCLHRDParameters.ui32CPBCntMinus1 < 32 ) ) 
    {
        ui32Width = psVUIInfo->sVCLHRDParameters.ui32InitialCPBRemovalDelayLengthMinus1 + 1;
        IMG_ASSERT( ui32Width <= 32 );
        if ( ui32Width == 32 )
        {
            for ( SchedSelIdx = 0; SchedSelIdx <= psVUIInfo->sVCLHRDParameters.ui32CPBCntMinus1; SchedSelIdx++ ) 
            {
                psSeiBufferingPeriod->initial_cpb_removal_delay[ SchedSelIdx ] = ( SWSR_ReadBits( hSwSrContext, 16 ) << 16 );
                psSeiBufferingPeriod->initial_cpb_removal_delay[ SchedSelIdx ] |= SWSR_ReadBits( hSwSrContext, 16 );
                psSeiBufferingPeriod->initial_cpb_removal_delay_offset[ SchedSelIdx ] = ( SWSR_ReadBits( hSwSrContext, 16 ) << 16 );
                psSeiBufferingPeriod->initial_cpb_removal_delay_offset[ SchedSelIdx ] |= SWSR_ReadBits( hSwSrContext, 16 );
            }
        }
        else
        {
            for ( SchedSelIdx = 0; SchedSelIdx <= psVUIInfo->sVCLHRDParameters.ui32CPBCntMinus1; SchedSelIdx++ ) 
            {
                psSeiBufferingPeriod->initial_cpb_removal_delay[ SchedSelIdx ] = SWSR_ReadBits( hSwSrContext, ui32Width );
                psSeiBufferingPeriod->initial_cpb_removal_delay_offset[ SchedSelIdx ] = SWSR_ReadBits( hSwSrContext, ui32Width );
            }
        }
    }
}


/*!
******************************************************************************

 @Function              bspp_H264ParseSeiPicTiming

 @Description

******************************************************************************/
static IMG_VOID bspp_H264ParseSeiPicTiming(
    IMG_HANDLE                  hSwSrContext,
    BSPP_sH264SeiPicTiming    * psSeiPicTiming,
    IMG_HANDLE                  hStrRes,
    IMG_UINT32                  ui32ActiveSpsForSeiParsing
)
{
    IMG_UINT8	i, time_offset_length;
    IMG_UINT32	ui32Width;
    BSPP_sH264VUIInfo * psVUIInfo = IMG_NULL;
    BSPP_sSequenceHdrInfo * psSequHdrInfo = IMG_NULL;
    BSPP_sH264SequHdrInfo * psH264SequHdrInfo = IMG_NULL;

    IMG_ASSERT( hSwSrContext != IMG_NULL );
    IMG_ASSERT( psSeiPicTiming != IMG_NULL );
    IMG_ASSERT( hStrRes != IMG_NULL );

    /* set in-use flag */
    psSeiPicTiming->bInUseFlag = IMG_TRUE;

    psSequHdrInfo = BSPP_GetSequHdr( hStrRes, ui32ActiveSpsForSeiParsing );
    if ( psSequHdrInfo == IMG_NULL )
    {
        // Error case, return early so at least we do not parse out of current payload bounds
        return;
    }
    psH264SequHdrInfo = ( BSPP_sH264SequHdrInfo * )psSequHdrInfo->hSecureSequenceInfo;
    if ( psH264SequHdrInfo == IMG_NULL )
    {
        // Error case, return early so at least we do not parse out of current payload bounds
        return;
    }
    psVUIInfo = &psH264SequHdrInfo->sVUIInfo;

    /* if CpbDpbDelaysPresentFlag */
    if ( psVUIInfo->nal_hrd_parameters_present_flag ) 
    {
        ui32Width = psVUIInfo->sNALHRDParameters.ui32CPBRemovalDelayLenghtMinus1 + 1;
        IMG_ASSERT( ui32Width <= 32 );
        if ( ui32Width == 32 )
        {
            psSeiPicTiming->cpb_removal_delay = ( SWSR_ReadBits( hSwSrContext, 16 ) << 16 );
            psSeiPicTiming->cpb_removal_delay |= SWSR_ReadBits( hSwSrContext, 16 );
        }
        else
        {
            psSeiPicTiming->cpb_removal_delay = SWSR_ReadBits( hSwSrContext, ui32Width );
        }
        ui32Width = psVUIInfo->sNALHRDParameters.ui32DPBOutputDelayLengthMinus1 + 1;
        IMG_ASSERT( ui32Width <= 32 );
        if ( ui32Width == 32 )
        {
            psSeiPicTiming->dpb_output_delay = ( SWSR_ReadBits( hSwSrContext, 16 ) << 16 );
            psSeiPicTiming->dpb_output_delay |= SWSR_ReadBits( hSwSrContext, 16 );
        }
        else
        {
            psSeiPicTiming->dpb_output_delay = SWSR_ReadBits( hSwSrContext, ui32Width );
        }
        time_offset_length = psVUIInfo->sNALHRDParameters.ui32TimeOffsetLength;
    }
    else if ( psVUIInfo->vcl_hrd_parameters_present_flag )
    {
        ui32Width = psVUIInfo->sVCLHRDParameters.ui32CPBRemovalDelayLenghtMinus1 + 1;
        IMG_ASSERT( ui32Width <= 32 );
        if ( ui32Width == 32 )
        {
            psSeiPicTiming->cpb_removal_delay = ( SWSR_ReadBits( hSwSrContext, 16 ) << 16 );
            psSeiPicTiming->cpb_removal_delay |= SWSR_ReadBits( hSwSrContext, 16 );
        }
        else
        {
            psSeiPicTiming->cpb_removal_delay = SWSR_ReadBits( hSwSrContext, ui32Width );
        }
        ui32Width = psVUIInfo->sVCLHRDParameters.ui32DPBOutputDelayLengthMinus1 + 1;
        IMG_ASSERT( ui32Width <= 32 );
        if ( ui32Width == 32 )
        {
            psSeiPicTiming->dpb_output_delay = ( SWSR_ReadBits( hSwSrContext, 16 ) << 16 );
            psSeiPicTiming->dpb_output_delay |= SWSR_ReadBits( hSwSrContext, 16 );
        }
        else
        {
            psSeiPicTiming->dpb_output_delay = SWSR_ReadBits( hSwSrContext, ui32Width );
        }
        time_offset_length = psVUIInfo->sVCLHRDParameters.ui32TimeOffsetLength;
    }
    else
    {
        /* default value */
        time_offset_length = 24;
    }

    /* pic_struct_present_flag */
    if ( psVUIInfo->pic_struct_present_flag ) 
    {
        psSeiPicTiming->pic_struct = SWSR_ReadBits( hSwSrContext, 4 );
        for ( i = 0; i < aui8PicStructToNumClockTSMapExtended[ psSeiPicTiming->pic_struct ]; i++ ) 
        {
            psSeiPicTiming->clock_timestamp_flag[ i ] = SWSR_ReadBits( hSwSrContext, 1 );
            if ( psSeiPicTiming->clock_timestamp_flag[ i ] ) 
            {
                psSeiPicTiming->ct_type[ i ] = SWSR_ReadBits( hSwSrContext, 2 );
                psSeiPicTiming->nuit_field_based_flag[ i ] = SWSR_ReadBits( hSwSrContext, 1 );
                psSeiPicTiming->counting_type[ i ] = SWSR_ReadBits( hSwSrContext, 5 );
                psSeiPicTiming->full_timestamp_flag[ i ] = SWSR_ReadBits( hSwSrContext, 1 );
                psSeiPicTiming->discontinuity_flag[ i ] = SWSR_ReadBits( hSwSrContext, 1 );
                psSeiPicTiming->cnt_dropped_flag[ i ] = SWSR_ReadBits( hSwSrContext, 1 );
                psSeiPicTiming->n_frames[ i ] = SWSR_ReadBits( hSwSrContext, 8 );
                if ( psSeiPicTiming->full_timestamp_flag[ i ] ) 
                {
                    psSeiPicTiming->seconds_value[ i ] = SWSR_ReadBits( hSwSrContext, 6 ); /* 0..59 */
                    psSeiPicTiming->minutes_value[ i ] = SWSR_ReadBits( hSwSrContext, 6 ); /* 0..59 */
                    psSeiPicTiming->hours_value[ i ] = SWSR_ReadBits( hSwSrContext, 5 ); /* 0..23 */
                } 
                else 
                {
                    psSeiPicTiming->seconds_flag[ i ] = SWSR_ReadBits( hSwSrContext, 1 );
                    if ( psSeiPicTiming->seconds_flag[ i ] ) 
                    {
                        psSeiPicTiming->seconds_value[ i ] = SWSR_ReadBits( hSwSrContext, 6 );/* range 0..59 */ 
                        psSeiPicTiming->minutes_flag[ i ] = SWSR_ReadBits( hSwSrContext, 1 );
                        if ( psSeiPicTiming->minutes_flag[ i ] ) 
                        {
                            psSeiPicTiming->minutes_value[ i ] = SWSR_ReadBits( hSwSrContext, 6 );/* 0..59 */
                            psSeiPicTiming->hours_flag[ i ] = SWSR_ReadBits( hSwSrContext, 1 );
                            if ( psSeiPicTiming->hours_flag[ i ] )
                            {
                                psSeiPicTiming->hours_value[i] = SWSR_ReadBits( hSwSrContext, 5 );/* 0..23 */ 
                            }
                        }
                    }
                }
                if ( time_offset_length > 0 )
                {
                    IMG_ASSERT( time_offset_length < 32 );
                    psSeiPicTiming->time_offset[ i ] = SWSR_ReadBits( hSwSrContext, time_offset_length );
                    psSeiPicTiming->time_offset[ i ] = SIGN_EXTEND32( psSeiPicTiming->time_offset[ i ], time_offset_length );
                }
            }
        }
    }
}


/*!
******************************************************************************

 @Function              bspp_H264ParseSeiPanScanRect

 @Description

******************************************************************************/
static IMG_VOID bspp_H264ParseSeiPanScanRect(
    IMG_HANDLE                  hSwSrContext,
    BSPP_sH264SeiPanScanRect  * psSeiPanScanRect
)
{
    IMG_UINT8	i;

    IMG_ASSERT(hSwSrContext != IMG_NULL);
    IMG_ASSERT(psSeiPanScanRect != IMG_NULL);

    /* set in-use flag */
    psSeiPanScanRect->bInUseFlag = IMG_TRUE;

    psSeiPanScanRect->pan_scan_rect_id = SWSR_ReadUnsignedExpGoulomb( hSwSrContext );
    psSeiPanScanRect->pan_scan_rect_cancel_flag = SWSR_ReadBits( hSwSrContext, 1 );
    if ( !psSeiPanScanRect->pan_scan_rect_cancel_flag ) 
    {
        psSeiPanScanRect->pan_scan_cnt_minus1 = SWSR_ReadUnsignedExpGoulomb( hSwSrContext );
        if ( psSeiPanScanRect->pan_scan_cnt_minus1 >= H264_MAX_NUM_FIELD_PARAMS )
        {
            // Error case, return early so at least we do not parse out of current payload bounds
            return;
        }

        for ( i = 0; i <= psSeiPanScanRect->pan_scan_cnt_minus1; i++ ) 
        {
            psSeiPanScanRect->pan_scan_rect_left_offset[ i ] = SWSR_ReadSignedExpGoulomb( hSwSrContext );
            psSeiPanScanRect->pan_scan_rect_right_offset[ i ] = SWSR_ReadSignedExpGoulomb( hSwSrContext );
            psSeiPanScanRect->pan_scan_rect_top_offset[ i ] = SWSR_ReadSignedExpGoulomb( hSwSrContext );
            psSeiPanScanRect->pan_scan_rect_bottom_offset[ i ] = SWSR_ReadSignedExpGoulomb( hSwSrContext );
        }
        psSeiPanScanRect->pan_scan_rect_repetition_period = SWSR_ReadUnsignedExpGoulomb( hSwSrContext );
    }
}


#ifdef PARSE_SEI_FILLER_PAYLOAD
/*!
******************************************************************************

 @Function              bspp_H264ParseSeiFillerPayload

 @Description

******************************************************************************/
static IMG_VOID bspp_H264ParseSeiFillerPayload(
    IMG_HANDLE                      hSwSrContext,
    BSPP_sH264SeiFillerPayload    * psSeiFillerPayload,
    IMG_UINT32                      payloadSize 
)
{
    IMG_UINT8	i;

    IMG_ASSERT( hSwSrContext != IMG_NULL );
    IMG_ASSERT( psSeiFillerPayload != IMG_NULL );
    
    /* set in-use flag */
    psSeiFillerPayload->bInUseFlag = IMG_TRUE;

    for ( i = 0; i < payloadSize; i++ )
    {
        SWSR_ReadBits( hSwSrContext, 8 );
    }
}
#endif


/*!
******************************************************************************

 @Function              bspp_H264ParseSeiUserDataRegisteredItuTT35

 @Description

******************************************************************************/
static IMG_VOID bspp_H264ParseSeiUserDataRegisteredItuTT35(
    IMG_HANDLE                                  hSwSrContext,
    BSPP_sH264SeiUserDataRegisteredItuTT35    * psSeiUserDataRegisteredItuTT35,
    IMG_UINT32                                  payloadSize
)
{
    IMG_UINT32  i = 1;  // First read 8 happens always

    IMG_ASSERT( hSwSrContext != IMG_NULL );
    IMG_ASSERT( psSeiUserDataRegisteredItuTT35 != IMG_NULL );

    /* set in-use flag */
    psSeiUserDataRegisteredItuTT35->bInUseFlag = IMG_TRUE;

    psSeiUserDataRegisteredItuTT35->itu_t_t35_country_code = SWSR_ReadBits( hSwSrContext, 8 );
    if ( psSeiUserDataRegisteredItuTT35->itu_t_t35_country_code == 0xFF )	
    {
        i = 2;          // Second read will happen before the actual payload
        psSeiUserDataRegisteredItuTT35->itu_t_t35_country_code_extension_byte = SWSR_ReadBits( hSwSrContext, 8 );
    }

    while ( ( i < H264_MAX_ITUTT35_BYTES ) && ( i < payloadSize ) )
    {
        psSeiUserDataRegisteredItuTT35->itu_t_t35_payload_byte[ i++ ] = SWSR_ReadBits( hSwSrContext, 8 );
    } 
}

        
/*!
******************************************************************************

 @Function				bspp_H264ParseSeiUserDataUnregistered

 @Description

******************************************************************************/
static IMG_VOID bspp_H264ParseSeiUserDataUnregistered(
    IMG_HANDLE                          hSwSrContext,
    BSPP_sH264SeiUserDataUnregistered * psSeiUserDataUnregistered,
    IMG_UINT32                          payloadSize
)
{
    IMG_UINT32 i;

    IMG_ASSERT( hSwSrContext != IMG_NULL );
    IMG_ASSERT( psSeiUserDataUnregistered != IMG_NULL );

    /* set in-use flag */
    psSeiUserDataUnregistered->bInUseFlag = IMG_TRUE;

    for ( i = 0; ( i < H264_BYTES_IN_128_BITS ) && ( i < payloadSize ); i++ )
    {
        psSeiUserDataUnregistered->uuid_iso_iec_11578[ i ] = SWSR_ReadBits( hSwSrContext, 8 );
    }

    for ( ; ( i < payloadSize ) && ( i < H264_MAX_UNREGISTERED_BYTES-H264_BYTES_IN_128_BITS ); i++ )
    {
        IMG_ASSERT( i >= H264_BYTES_IN_128_BITS );  // Sanity check, the code above ensures that i is the MIN(H264_BYTES_IN_128_BITS,payloadSize)
                                                    // so either the condition in the for loop will be FALSE or the ASSERTION will be TRUE
        psSeiUserDataUnregistered->user_data_payload_byte[ i-H264_BYTES_IN_128_BITS ] = SWSR_ReadBits( hSwSrContext, 8 );
    }
}


/*!
******************************************************************************

 @Function				bspp_H264ParseSeiRecoveryPoint

 @Description

******************************************************************************/
static IMG_VOID bspp_H264ParseSeiRecoveryPoint(
    IMG_HANDLE                      hSwSrContext,
    BSPP_sH264SeiRecoveryPoint    * psSeiRecoveryPoint
)
{
    IMG_ASSERT( hSwSrContext != IMG_NULL );
    IMG_ASSERT( psSeiRecoveryPoint != IMG_NULL );

    /* set in-use flag */
    psSeiRecoveryPoint->bInUseFlag = IMG_TRUE;

    psSeiRecoveryPoint->recovery_frame_cnt = SWSR_ReadUnsignedExpGoulomb( hSwSrContext );
    psSeiRecoveryPoint->exact_match_flag = SWSR_ReadBits( hSwSrContext, 1 );
    psSeiRecoveryPoint->broken_link_flag = SWSR_ReadBits( hSwSrContext, 1 );
    psSeiRecoveryPoint->changing_slice_group_idc = SWSR_ReadBits( hSwSrContext, 2 );
}


/*!
******************************************************************************

 @Function				bspp_H264ParseSeiDecRefPicMarkingRepetition

 @Description

******************************************************************************/
static IMG_VOID bspp_H264ParseSeiDecRefPicMarkingRepetition(
    IMG_HANDLE                                  hSwSrContext,
    BSPP_sH264SeiDecRefPicMarkingRepetition   * psSeiDecRefPicMarkingRepetition,
    IMG_HANDLE                                  hStrRes,
    IMG_UINT32                                  ui32ActiveSpsForSeiParsing
)
{
    IMG_UINT32 i;
    BSPP_sH264SPSInfo * psSPSInfo = IMG_NULL;
    BSPP_sSequenceHdrInfo * psSequHdrInfo = IMG_NULL;
    BSPP_sH264SequHdrInfo * psH264SequHdrInfo = IMG_NULL;

    IMG_ASSERT( hSwSrContext != IMG_NULL );
    IMG_ASSERT( psSeiDecRefPicMarkingRepetition != IMG_NULL );

    /* set in-use flag */
    psSeiDecRefPicMarkingRepetition->bInUseFlag = IMG_TRUE;

    psSeiDecRefPicMarkingRepetition->original_idr_flag = SWSR_ReadBits( hSwSrContext, 1 );
    psSeiDecRefPicMarkingRepetition->original_frame_num = SWSR_ReadUnsignedExpGoulomb( hSwSrContext );

    psSequHdrInfo = BSPP_GetSequHdr( hStrRes, ui32ActiveSpsForSeiParsing );
    if ( psSequHdrInfo == IMG_NULL )
    {
        // Error case, return early so at least we do not parse out of current payload bounds
        return;
    }
    psH264SequHdrInfo = ( BSPP_sH264SequHdrInfo * )psSequHdrInfo->hSecureSequenceInfo;
    if ( psH264SequHdrInfo == IMG_NULL )
    {
        // Error case, return early so at least we do not parse out of current payload bounds
        return;
    }
    psSPSInfo = &psH264SequHdrInfo->sSPSInfo;

    if ( !psSPSInfo->frame_mbs_only_flag ) 
    {
        psSeiDecRefPicMarkingRepetition->original_field_pic_flag = SWSR_ReadBits( hSwSrContext, 1 );
        if ( psSeiDecRefPicMarkingRepetition->original_field_pic_flag )	
        {
            psSeiDecRefPicMarkingRepetition->original_bottom_field_flag = SWSR_ReadBits( hSwSrContext, 1 );
        }
    }

    if( psSeiDecRefPicMarkingRepetition->original_idr_flag )
    {
        psSeiDecRefPicMarkingRepetition->no_output_of_prior_pics_flag = SWSR_ReadBits( hSwSrContext, 1 );
        psSeiDecRefPicMarkingRepetition->long_term_reference_flag = SWSR_ReadBits( hSwSrContext, 1 );
    } 
    else 
    {
        psSeiDecRefPicMarkingRepetition->adaptive_ref_pic_marking_mode_flag = SWSR_ReadBits( hSwSrContext, 1 );
        if ( psSeiDecRefPicMarkingRepetition->adaptive_ref_pic_marking_mode_flag )
        {
            i = 0;
            do {
                psSeiDecRefPicMarkingRepetition->memory_management_control_operation[ i ] = SWSR_ReadUnsignedExpGoulomb( hSwSrContext );
                if( (psSeiDecRefPicMarkingRepetition->memory_management_control_operation[ i ] == 1) ||
                    (psSeiDecRefPicMarkingRepetition->memory_management_control_operation[ i ] == 3) )
                {
                    psSeiDecRefPicMarkingRepetition->difference_of_pic_nums_minus1[ i ] = SWSR_ReadUnsignedExpGoulomb( hSwSrContext );
                }
                if ( psSeiDecRefPicMarkingRepetition->memory_management_control_operation[ i ] == 2 )	
                {
                    psSeiDecRefPicMarkingRepetition->long_term_pic_num[ i ] = SWSR_ReadUnsignedExpGoulomb( hSwSrContext );
                }
                if ( ( psSeiDecRefPicMarkingRepetition->memory_management_control_operation[ i ] == 3 ) ||
                     ( psSeiDecRefPicMarkingRepetition->memory_management_control_operation[ i ] == 6 ) )
                {
                    psSeiDecRefPicMarkingRepetition->long_term_frame_idx[ i ] = SWSR_ReadUnsignedExpGoulomb( hSwSrContext );
                }
                if ( psSeiDecRefPicMarkingRepetition->memory_management_control_operation[ i ] == 4 )
                {
                    psSeiDecRefPicMarkingRepetition->max_long_term_frame_idx_plus1[ i ] = SWSR_ReadUnsignedExpGoulomb( hSwSrContext );
                }
            } while ( ( psSeiDecRefPicMarkingRepetition->memory_management_control_operation[ i++ ] != 0 ) && ( i < H264_MAX_DEC_REF_PIC_MARK ) );
        }
    }
}


/*!
******************************************************************************

 @Function              bspp_H264ParseSeiSparePic

 @Description

******************************************************************************/
static IMG_VOID bspp_H264ParseSeiSparePic(
    IMG_HANDLE              hSwSrContext,
    BSPP_sH264SeiSparePic * psSeiSparePic,
    IMG_HANDLE              hStrRes,
    IMG_UINT32              ui32ActiveSpsForSeiParsing
)
{
    IMG_UINT32 i, j, PicHeightInMapUnits, PicSizeInMapUnits;
    BSPP_sH264SPSInfo * psSPSInfo = IMG_NULL;
    BSPP_sSequenceHdrInfo * psSequHdrInfo = IMG_NULL;
    BSPP_sH264SequHdrInfo * psH264SequHdrInfo = IMG_NULL;

    IMG_ASSERT( hSwSrContext != IMG_NULL );
    IMG_ASSERT( psSeiSparePic != IMG_NULL );

    /* set in-use flag */
    psSeiSparePic->bInUseFlag = IMG_TRUE;

    psSeiSparePic->target_frame_num = SWSR_ReadUnsignedExpGoulomb( hSwSrContext );
    psSeiSparePic->spare_field_flag = SWSR_ReadBits( hSwSrContext, 1 );
    if ( psSeiSparePic->spare_field_flag )	
    {
        psSeiSparePic->target_bottom_field_flag = SWSR_ReadBits( hSwSrContext, 1 );
    }

    psSeiSparePic->num_spare_pics_minus1 = SWSR_ReadUnsignedExpGoulomb( hSwSrContext );
    if ( psSeiSparePic->num_spare_pics_minus1 >= H264_MAX_NUM_SPARE_PICS )
    {
        // Error case, return early so at least we do not parse out of current payload bounds
        return;
    }

    psSequHdrInfo = BSPP_GetSequHdr( hStrRes, ui32ActiveSpsForSeiParsing );
    if ( psSequHdrInfo == IMG_NULL )
    {
        // Error case, return early so at least we do not parse out of current payload bounds
        return;
    }
    psH264SequHdrInfo = ( BSPP_sH264SequHdrInfo * )psSequHdrInfo->hSecureSequenceInfo;
    if ( psH264SequHdrInfo == IMG_NULL )
    {
        // Error case, return early so at least we do not parse out of current payload bounds
        return;
    }
    psSPSInfo = &psH264SequHdrInfo->sSPSInfo;

    PicHeightInMapUnits = ( psSPSInfo->pic_height_in_map_units_minus1 + 1 ) / ( 2 - psSPSInfo->frame_mbs_only_flag );
    PicSizeInMapUnits = PicHeightInMapUnits * ( psSPSInfo->pic_width_in_mbs_minus1 + 1 );

    for ( i = 0; i < ( psSeiSparePic->num_spare_pics_minus1 + 1 ); i++ ) 
    {
        psSeiSparePic->delta_spare_frame_num[ i ] = SWSR_ReadUnsignedExpGoulomb( hSwSrContext );
        if ( psSeiSparePic->spare_field_flag )	
        {
            psSeiSparePic->spare_bottom_field_flag[ i ] = SWSR_ReadBits( hSwSrContext, 1 );
        }
        psSeiSparePic->spare_area_idc[ i ] = SWSR_ReadUnsignedExpGoulomb( hSwSrContext );
        if ( psSeiSparePic->spare_area_idc[ i ] == 1 )	
        {
            for ( j = 0; j < PicSizeInMapUnits; j++ ) 
            {
                psSeiSparePic->spare_area.spare_unit_flag[ i ][ j ] = SWSR_ReadBits( hSwSrContext, 1 );
            }
        }
        else if ( psSeiSparePic->spare_area_idc[ i ] == 2 ) 
        {
            IMG_UINT32	mapUnitCnt = 0;
            for ( j = 0; mapUnitCnt < PicSizeInMapUnits; j++ ) 
            {
                psSeiSparePic->spare_area.zero_run_length[ i ][ j ] = SWSR_ReadUnsignedExpGoulomb( hSwSrContext );
                mapUnitCnt += ( psSeiSparePic->spare_area.zero_run_length[ i ][ j ] + 1 );
            }
        }
    }
}

        
/*!
******************************************************************************

 @Function              bspp_H264ParseSeiSceneInfo

 @Description

******************************************************************************/
static IMG_VOID bspp_H264ParseSeiSceneInfo(
    IMG_HANDLE                  hSwSrContext,
    BSPP_sH264SeiSceneInfo    * psSeiSceneInfo
)
{
    IMG_ASSERT( hSwSrContext != IMG_NULL );
    IMG_ASSERT( psSeiSceneInfo != IMG_NULL );

    /* set in-use flag */
    psSeiSceneInfo->bInUseFlag = IMG_TRUE;

    psSeiSceneInfo->scene_info_present_flag = SWSR_ReadBits( hSwSrContext, 1 );
    if ( psSeiSceneInfo->scene_info_present_flag ) 
    {
        psSeiSceneInfo->scene_id = SWSR_ReadUnsignedExpGoulomb( hSwSrContext );
        psSeiSceneInfo->scene_transition_type = SWSR_ReadUnsignedExpGoulomb( hSwSrContext );
        if ( psSeiSceneInfo->scene_transition_type > 3 )
        {
            psSeiSceneInfo->second_scene_id = SWSR_ReadUnsignedExpGoulomb( hSwSrContext );
        }
    }
}


/*!
******************************************************************************

 @Function          bspp_H264ParseSeiSubSeqInfo

 @Description

******************************************************************************/
static IMG_VOID bspp_H264ParseSeiSubSeqInfo(
    IMG_HANDLE                  hSwSrContext,
    BSPP_sH264SeiSubSeqInfo   * psSeiSubSeqInfo
)
{
    IMG_ASSERT( hSwSrContext != IMG_NULL );
    IMG_ASSERT( psSeiSubSeqInfo != IMG_NULL );

    /* set in-use flag */
    psSeiSubSeqInfo->bInUseFlag = IMG_TRUE;

    psSeiSubSeqInfo->sub_seq_layer_num = SWSR_ReadUnsignedExpGoulomb( hSwSrContext );
    psSeiSubSeqInfo->sub_seq_id = SWSR_ReadUnsignedExpGoulomb( hSwSrContext );
    psSeiSubSeqInfo->first_ref_pic_flag = SWSR_ReadBits( hSwSrContext, 1 );
    psSeiSubSeqInfo->leading_non_ref_pic_flag = SWSR_ReadBits( hSwSrContext, 1 );
    psSeiSubSeqInfo->last_pic_flag = SWSR_ReadBits( hSwSrContext, 1 );
    psSeiSubSeqInfo->sub_seq_frame_num_flag = SWSR_ReadBits( hSwSrContext, 1 );
    if ( psSeiSubSeqInfo->sub_seq_frame_num_flag )	
    {
        psSeiSubSeqInfo->sub_seq_frame_num = SWSR_ReadUnsignedExpGoulomb( hSwSrContext );
    }
}

        
/*!
******************************************************************************

 @Function              bspp_H264ParseSeiSubSeqLayerCharacteristics

 @Description

******************************************************************************/
static IMG_VOID bspp_H264ParseSeiSubSeqLayerCharacteristics(
    IMG_HANDLE                                  hSwSrContext,
    BSPP_sH264SeiSubSeqLayerCharacteristics   * psSeiSubSeqLayerCharacteristics
)
{
    IMG_UINT32 layer;

    IMG_ASSERT( hSwSrContext != IMG_NULL );
    IMG_ASSERT( psSeiSubSeqLayerCharacteristics != IMG_NULL );

    /* set in-use flag */
    psSeiSubSeqLayerCharacteristics->bInUseFlag = IMG_TRUE;

    psSeiSubSeqLayerCharacteristics->num_sub_seq_layers_minus1 = SWSR_ReadUnsignedExpGoulomb( hSwSrContext );
    if ( psSeiSubSeqLayerCharacteristics->num_sub_seq_layers_minus1 >= H264_MAX_NUM_SUB_SEQ )
    {
        // Error case, return early so at least we do not parse out of current payload bounds
        return;
    }
    for ( layer = 0; layer <= psSeiSubSeqLayerCharacteristics->num_sub_seq_layers_minus1; layer++ ) 
    {
        psSeiSubSeqLayerCharacteristics->accurate_statistics_flag[ layer ] = SWSR_ReadBits( hSwSrContext, 1 );
        psSeiSubSeqLayerCharacteristics->average_bit_rate[ layer ] = SWSR_ReadBits( hSwSrContext, 16 );
        psSeiSubSeqLayerCharacteristics->average_frame_rate[ layer ] = SWSR_ReadBits( hSwSrContext, 16 );
    }
}

        
/*!
******************************************************************************

 @Function          bspp_H264ParseSeiSubSeqCharacteristics

 @Description

******************************************************************************/
static IMG_VOID bspp_H264ParseSeiSubSeqCharacteristics(
    IMG_HANDLE                              hSwSrContext,
    BSPP_sH264SeiSubSeqCharacteristics    * psSeiSubSeqCharacteristics
)
{
    IMG_UINT32	n;

    IMG_ASSERT( hSwSrContext != IMG_NULL );
    IMG_ASSERT( psSeiSubSeqCharacteristics != IMG_NULL );

    /* set in-use flag */
    psSeiSubSeqCharacteristics->bInUseFlag = IMG_TRUE;

    psSeiSubSeqCharacteristics->sub_seq_layer_num = SWSR_ReadUnsignedExpGoulomb( hSwSrContext );
    psSeiSubSeqCharacteristics->sub_seq_id = SWSR_ReadUnsignedExpGoulomb( hSwSrContext );
    psSeiSubSeqCharacteristics->duration_flag = SWSR_ReadBits( hSwSrContext, 1 );
    if ( psSeiSubSeqCharacteristics->duration_flag )	
    {
        psSeiSubSeqCharacteristics->sub_seq_duration = ( SWSR_ReadBits( hSwSrContext, 16 ) << 16 );
        psSeiSubSeqCharacteristics->sub_seq_duration |= SWSR_ReadBits( hSwSrContext, 16 );
    }
    psSeiSubSeqCharacteristics->average_rate_flag = SWSR_ReadBits( hSwSrContext, 1 );
    if ( psSeiSubSeqCharacteristics->average_rate_flag ) 
    {
        psSeiSubSeqCharacteristics->accurate_statistics_flag = SWSR_ReadBits( hSwSrContext, 1 );
        psSeiSubSeqCharacteristics->average_bit_rate = SWSR_ReadBits( hSwSrContext, 16 );
        psSeiSubSeqCharacteristics->average_frame_rate = SWSR_ReadBits( hSwSrContext, 16 );
    }
    psSeiSubSeqCharacteristics->num_referenced_subseqs = SWSR_ReadUnsignedExpGoulomb( hSwSrContext );
    if ( psSeiSubSeqCharacteristics->num_referenced_subseqs >= H264_MAX_NUM_SUB_SEQ )
    {
        // Error case, return early so at least we do not parse out of current payload bounds
        return;
    }
    for ( n = 0; n < psSeiSubSeqCharacteristics->num_referenced_subseqs; n++ ) 
    {
        psSeiSubSeqCharacteristics->ref_sub_seq_layer_num[ n ] = SWSR_ReadUnsignedExpGoulomb( hSwSrContext );
        psSeiSubSeqCharacteristics->ref_sub_seq_id[ n ] = SWSR_ReadUnsignedExpGoulomb( hSwSrContext );
        psSeiSubSeqCharacteristics->ref_sub_seq_direction[ n ] = SWSR_ReadBits( hSwSrContext, 1 );
    }
}

        
/*!
******************************************************************************

 @Function              bspp_H264ParseSeiFullFrameFreeze

 @Description

******************************************************************************/
static IMG_VOID bspp_H264ParseSeiFullFrameFreeze(
    IMG_HANDLE                      hSwSrContext,
    BSPP_sH264SeiFullFrameFreeze  * psSeiFullFrameFreeze
)
{
    IMG_ASSERT( hSwSrContext != IMG_NULL );
    IMG_ASSERT( psSeiFullFrameFreeze != IMG_NULL );

    /* set in-use flag */
    psSeiFullFrameFreeze->bInUseFlag = IMG_TRUE;

    psSeiFullFrameFreeze->full_frame_freeze_repetition_period = SWSR_ReadUnsignedExpGoulomb( hSwSrContext );
}

        
/*!
******************************************************************************

 @Function              bspp_H264ParseSeiFullFrameFreezeRelease

 @Description

******************************************************************************/
static IMG_VOID bspp_H264ParseSeiFullFrameFreezeRelease(
    IMG_HANDLE                              hSwSrContext,
    BSPP_sH264SeiFullFrameFreezeRelease   * psSeiFullFrameFreezeRelease
)
{
    IMG_ASSERT( hSwSrContext != IMG_NULL );
    IMG_ASSERT( psSeiFullFrameFreezeRelease != IMG_NULL );

    /* set in-use flag */
    psSeiFullFrameFreezeRelease->bInUseFlag = IMG_TRUE;
}


/*!
******************************************************************************

 @Function              bspp_H264ParseSeiFullFrameSnapshot

 @Description

******************************************************************************/
static IMG_VOID bspp_H264ParseSeiFullFrameSnapshot(
    IMG_HANDLE                          hSwSrContext,
    BSPP_sH264SeiFullFrameSnapshot    * psSeiFullFrameSnapshot
)
{
    IMG_ASSERT( hSwSrContext != IMG_NULL );
    IMG_ASSERT( psSeiFullFrameSnapshot != IMG_NULL );

    /* set in-use flag */
    psSeiFullFrameSnapshot->bInUseFlag = IMG_TRUE;

    psSeiFullFrameSnapshot->snapshot_id = SWSR_ReadUnsignedExpGoulomb( hSwSrContext );
}


/*!
******************************************************************************

 @Function              bspp_H264ParseSeiProgressiveRefinementSegmentStart

 @Description

******************************************************************************/
static IMG_VOID bspp_H264ParseSeiProgressiveRefinementSegmentStart(
    IMG_HANDLE                                          hSwSrContext,
    BSPP_sH264SeiProgressiveRefinementSegmentStart    * psSeiProgressiveRefinementSegmentStart
)
{
    IMG_ASSERT( hSwSrContext != IMG_NULL );
    IMG_ASSERT( psSeiProgressiveRefinementSegmentStart != IMG_NULL );

    /* set in-use flag */
    psSeiProgressiveRefinementSegmentStart->bInUseFlag = IMG_TRUE;

    psSeiProgressiveRefinementSegmentStart->progressive_refinement_id = SWSR_ReadUnsignedExpGoulomb( hSwSrContext );
    psSeiProgressiveRefinementSegmentStart->num_refinement_steps_minus1 = SWSR_ReadUnsignedExpGoulomb( hSwSrContext );
}

        
/*!
******************************************************************************

 @Function              bspp_H264ParseSeiProgressiveRefinementSegmentEnd

 @Description

******************************************************************************/
static IMG_VOID bspp_H264ParseSeiProgressiveRefinementSegmentEnd(
    IMG_HANDLE                                      hSwSrContext,
    BSPP_sH264SeiProgressiveRefinementSegmentEnd  * psSeiProgressiveRefinementSegmentEnd
)
{
    IMG_ASSERT( hSwSrContext != IMG_NULL );
    IMG_ASSERT( psSeiProgressiveRefinementSegmentEnd != IMG_NULL );

    /* set in-use flag */
    psSeiProgressiveRefinementSegmentEnd->bInUseFlag = IMG_TRUE;

    psSeiProgressiveRefinementSegmentEnd->progressive_refinement_id = SWSR_ReadUnsignedExpGoulomb( hSwSrContext );
}


#ifdef PARSE_SEI_MOTION_CONSTRAINED_SLICE_GROUP_SET
/*!
******************************************************************************

 @Function              bspp_H264ParseSeiMotionConstrainedSliceGroupSet

 @Description

******************************************************************************/
static IMG_VOID bspp_H264ParseSeiMotionConstrainedSliceGroupSet(
    IMG_HANDLE                                      hSwSrContext,
    BSPP_sH264SeiMotionConstrainedSliceGroupSet   * psSeiMotionConstrainedSliceGroupSet
)
{
    // IMPORTANT: Motion-constrained slice group set SEI message syntax requires "Out Of Order" parsing, 
    // see D.1.19 and Note in D.2.19
    // UNSUPPORTED

/*
    IMG_UINT32	i = 0, ui32Width = 0, ui32Number = 0;

    IMG_ASSERT( hSwSrContext != IMG_NULL );
    IMG_ASSERT( psSeiMotionConstrainedSliceGroupSet != IMG_NULL );

    // set in-use flag
    psSeiMotionConstrainedSliceGroupSet->bInUseFlag = IMG_TRUE;

    // calculate width of slice_group_id[] elements
    //ui32Number = psPPSInfo.num_slice_groups_minus1;   // Somehow we need the correct PPS here, which signifies Out-Of-Order parsing
    while(ui32Number > 0)
    {
        ui32Number >>= 1;
        ui32Width++;
    }
    if(ui32Width > 3)
    {
        // Error case, return early so at least we do not parse out of current payload bounds
        return;
    }

    psSeiMotionConstrainedSliceGroupSet->num_slice_groups_in_set_minus1 = SWSR_ReadUnsignedExpGoulomb( hSwSrContext );
    if ( psSeiMotionConstrainedSliceGroupSet->num_slice_groups_in_set_minus1 >= H264_MAX_NUM_SLICE_GROUPS )
    {
        // Error case, return early so at least we do not parse out of current payload bounds
        return;
    }
    if ( ( psSeiMotionConstrainedSliceGroupSet->num_slice_groups_in_set_minus1 > 0 ) && ( ui32Width != 0 ) )
    {
        for ( i = 0; i <= psSeiMotionConstrainedSliceGroupSet->num_slice_groups_in_set_minus1; i++ )
        {
            psSeiMotionConstrainedSliceGroupSet->slice_group_id[ i ] = SWSR_ReadBits( hSwSrContext, ui32Width );
        }
    }
    psSeiMotionConstrainedSliceGroupSet->exact_sample_value_match_flag = SWSR_ReadBits( hSwSrContext, 1 );

    psSeiMotionConstrainedSliceGroupSet->pan_scan_rect_flag = SWSR_ReadBits( hSwSrContext, 1 );
    if( psSeiMotionConstrainedSliceGroupSet->pan_scan_rect_flag )	
    {
        psSeiMotionConstrainedSliceGroupSet->pan_scan_rect_id = SWSR_ReadUnsignedExpGoulomb( hSwSrContext );
    }
*/
}
#endif


/*!
******************************************************************************

 @Function              bspp_H264ParseSeiFilmGrainCharacteristics

 @Description

******************************************************************************/
static IMG_VOID bspp_H264ParseSeiFilmGrainCharacteristics(
    IMG_HANDLE                              hSwSrContext,
    BSPP_sH264SeiFilmGrainCharacteristics * psSeiFilmGrainCharacteristics
)
{
    IMG_UINT32	c = 0, i = 0, j = 0;

    IMG_ASSERT( hSwSrContext != IMG_NULL );
    IMG_ASSERT( psSeiFilmGrainCharacteristics != IMG_NULL );

    /* set in-use flag */
    psSeiFilmGrainCharacteristics->bInUseFlag = IMG_TRUE;

    psSeiFilmGrainCharacteristics->film_grain_characteristics_cancel_flag = SWSR_ReadBits( hSwSrContext, 1 );
    if ( !psSeiFilmGrainCharacteristics->film_grain_characteristics_cancel_flag )
    {
        psSeiFilmGrainCharacteristics->model_id = SWSR_ReadBits( hSwSrContext, 2 );
        psSeiFilmGrainCharacteristics->separate_colour_description_present_flag = SWSR_ReadBits( hSwSrContext, 1 );
        if ( psSeiFilmGrainCharacteristics->separate_colour_description_present_flag )
        {
            psSeiFilmGrainCharacteristics->film_grain_bit_depth_luma_minus8 = SWSR_ReadBits( hSwSrContext, 3 );
            psSeiFilmGrainCharacteristics->film_grain_bit_depth_chroma_minus8 = SWSR_ReadBits( hSwSrContext, 3 );
            psSeiFilmGrainCharacteristics->film_grain_full_range_flag = SWSR_ReadBits( hSwSrContext, 1 );
            psSeiFilmGrainCharacteristics->film_grain_colour_primaries = SWSR_ReadBits( hSwSrContext, 8 );
            psSeiFilmGrainCharacteristics->film_grain_transfer_characteristics = SWSR_ReadBits( hSwSrContext, 8 );
            psSeiFilmGrainCharacteristics->film_grain_matrix_coefficients = SWSR_ReadBits( hSwSrContext, 8 );
        }
        psSeiFilmGrainCharacteristics->blending_mode_id = SWSR_ReadBits( hSwSrContext, 2 );
        psSeiFilmGrainCharacteristics->log2_scale_factor = SWSR_ReadBits( hSwSrContext, 4 );
        for ( c = 0; c < 3; c++ )
        {
            psSeiFilmGrainCharacteristics->comp_model_present_flag[ c ] = SWSR_ReadBits( hSwSrContext, 1 );
        }
        for ( c = 0; c < 3; c++ )
        {
            if ( psSeiFilmGrainCharacteristics->comp_model_present_flag[ c ] )
            {
                psSeiFilmGrainCharacteristics->num_intensity_intervals_minus1[ c ] = SWSR_ReadBits( hSwSrContext, 8 );
                psSeiFilmGrainCharacteristics->num_model_values_minus1[ c ] = SWSR_ReadBits( hSwSrContext, 3 );
                for ( i = 0; i <= psSeiFilmGrainCharacteristics->num_intensity_intervals_minus1[ c ]; i++ )
                {
                    psSeiFilmGrainCharacteristics->intensity_interval_lower_bound[ c ][ i ] = SWSR_ReadBits( hSwSrContext, 8 );
                    psSeiFilmGrainCharacteristics->intensity_interval_upper_bound[ c ][ i ] = SWSR_ReadBits( hSwSrContext, 8 );
                    for ( j = 0; j <= psSeiFilmGrainCharacteristics->num_model_values_minus1[ c ]; j++ )
                    {
                        psSeiFilmGrainCharacteristics->comp_model_value[ c ][ i ][ j ] = SWSR_ReadSignedExpGoulomb( hSwSrContext );
                    }
                }
            }
        }
        psSeiFilmGrainCharacteristics->film_grain_characteristics_repetition_period = SWSR_ReadUnsignedExpGoulomb( hSwSrContext );
    }
}


/*!
******************************************************************************

 @Function              bspp_H264ParseSeiDeblockingFilterDisplayPreference

 @Description

******************************************************************************/
static IMG_VOID bspp_H264ParseSeiDeblockingFilterDisplayPreference(
    IMG_HANDLE                                          hSwSrContext,
    BSPP_sH264SeiDeblockingFilterDisplayPreference    * psSeiDeblockingFilterDisplayPreference
)
{
    IMG_ASSERT( hSwSrContext != IMG_NULL );
    IMG_ASSERT( psSeiDeblockingFilterDisplayPreference != IMG_NULL );

    /* set in-use flag */
    psSeiDeblockingFilterDisplayPreference->bInUseFlag = IMG_TRUE;

    psSeiDeblockingFilterDisplayPreference->deblocking_display_preference_cancel_flag = SWSR_ReadBits( hSwSrContext, 1 );
    if ( !psSeiDeblockingFilterDisplayPreference->deblocking_display_preference_cancel_flag )
    {
        psSeiDeblockingFilterDisplayPreference->display_prior_to_deblocking_preferred_flag = SWSR_ReadBits( hSwSrContext, 1 );
        psSeiDeblockingFilterDisplayPreference->dec_frame_buffering_constraint_flag = SWSR_ReadBits( hSwSrContext, 1 );
        psSeiDeblockingFilterDisplayPreference->deblocking_display_preference_repetition_period = SWSR_ReadUnsignedExpGoulomb( hSwSrContext );
    }
}


/*!
******************************************************************************

 @Function              bspp_H264ParseSeiStereoVideoInfo

 @Description

******************************************************************************/
static IMG_VOID bspp_H264ParseSeiStereoVideoInfo(
    IMG_HANDLE                      hSwSrContext,
    BSPP_sH264SeiStereoVideoInfo  * psSeiStereoVideoInfo
)
{
    IMG_ASSERT( hSwSrContext != IMG_NULL );
    IMG_ASSERT( psSeiStereoVideoInfo != IMG_NULL );

    /* set in-use flag */
    psSeiStereoVideoInfo->bInUseFlag = IMG_TRUE;

    psSeiStereoVideoInfo->field_views_flag = SWSR_ReadBits( hSwSrContext, 1 );
    if ( psSeiStereoVideoInfo->field_views_flag )
    {
        psSeiStereoVideoInfo->top_field_is_left_view_flag = SWSR_ReadBits( hSwSrContext, 1 );
    }
    else
    {
        psSeiStereoVideoInfo->current_frame_is_left_view_flag = SWSR_ReadBits( hSwSrContext, 1 );
        psSeiStereoVideoInfo->next_frame_is_second_view_flag = SWSR_ReadBits( hSwSrContext, 1 );
    }
    psSeiStereoVideoInfo->left_view_self_contained_flag = SWSR_ReadBits( hSwSrContext, 1 );
    psSeiStereoVideoInfo->right_view_self_contained_flag = SWSR_ReadBits( hSwSrContext, 1 );
}


#ifdef PARSE_SEI_POST_FILTER_HINT
/*!
******************************************************************************

 @Function              bspp_H264ParseSeiPostFilterHint

 @Description

******************************************************************************/
static IMG_VOID bspp_H264ParseSeiPostFilterHint(
    IMG_HANDLE                      hSwSrContext,
    BSPP_sH264SeiPostFilterHint   * psSeiPostFilterHint
)
{
    IMG_UINT8   colour_component = 0, cy = 0, cx = 0;

    IMG_ASSERT( hSwSrContext != IMG_NULL );
    IMG_ASSERT( psSeiPostFilterHint != IMG_NULL );

    /* set in-use flag */
    psSeiPostFilterHint->bInUseFlag = IMG_TRUE;

    psSeiPostFilterHint->filter_hint_size_y = SWSR_ReadUnsignedExpGoulomb( hSwSrContext );
    psSeiPostFilterHint->filter_hint_size_x = SWSR_ReadUnsignedExpGoulomb( hSwSrContext );
    psSeiPostFilterHint->filter_hint_type = SWSR_ReadBits( hSwSrContext, 2 );

    if ( (psSeiPostFilterHint->filter_hint_size_y > 15) || (psSeiPostFilterHint->filter_hint_size_x > 15) )
    {
        // Error case, return early so at least we do not parse out of current payload bounds
        return;
    }
    for ( colour_component = 0; colour_component < 3; colour_component++ )
    {
        for ( cy = 0; cy < psSeiPostFilterHint->filter_hint_size_y; cy++ )
        {
            for ( cx = 0; cx < psSeiPostFilterHint->filter_hint_size_x; cx++ )
            {
                psSeiPostFilterHint->filter_hint[ colour_component ][ cy ][ cx ] = SWSR_ReadSignedExpGoulomb( hSwSrContext );
            }
        }
    }
    psSeiPostFilterHint->additional_extension_flag = SWSR_ReadBits( hSwSrContext, 1 );
}
#endif


#ifdef PARSE_SEI_TONE_MAPPING_INFO
/*!
******************************************************************************

 @Function              bspp_H264ParseSeiToneMappingInfo

 @Description

******************************************************************************/
static IMG_VOID bspp_H264ParseSeiToneMappingInfo(
    IMG_HANDLE                      hSwSrContext,
    BSPP_sH264SeiToneMappingInfo  * psSeiToneMappingInfo
)
{
    IMG_UINT32  i = 0;
    IMG_ASSERT( hSwSrContext != IMG_NULL );
    IMG_ASSERT( psSeiToneMappingInfo != IMG_NULL );

    /* set in-use flag */
    psSeiToneMappingInfo->bInUseFlag = IMG_TRUE;

    psSeiToneMappingInfo->tone_map_id = SWSR_ReadUnsignedExpGoulomb( hSwSrContext );
    psSeiToneMappingInfo->tone_map_cancel_flag = SWSR_ReadBits( hSwSrContext, 1 );
    if ( !psSeiToneMappingInfo->tone_map_cancel_flag )
    {
        psSeiToneMappingInfo->tone_map_repetition_period = SWSR_ReadUnsignedExpGoulomb( hSwSrContext );
        psSeiToneMappingInfo->coded_data_bit_depth = SWSR_ReadBits( hSwSrContext, 8 );
        psSeiToneMappingInfo->target_bit_depth = SWSR_ReadBits( hSwSrContext, 8 );
        psSeiToneMappingInfo->model_id = SWSR_ReadUnsignedExpGoulomb( hSwSrContext );
        if ( psSeiToneMappingInfo->model_id == 0 )
        {
            psSeiToneMappingInfo->min_value = ( SWSR_ReadBits( hSwSrContext, 16 ) << 16 );
            psSeiToneMappingInfo->min_value |= SWSR_ReadBits( hSwSrContext, 16 );
            psSeiToneMappingInfo->max_value = ( SWSR_ReadBits( hSwSrContext, 16 ) << 16 );
            psSeiToneMappingInfo->max_value |= SWSR_ReadBits( hSwSrContext, 16 );
        }
        else if ( psSeiToneMappingInfo->model_id == 1 )
        {
            psSeiToneMappingInfo->sigmoid_midpoint = ( SWSR_ReadBits( hSwSrContext, 16 ) << 16 );
            psSeiToneMappingInfo->sigmoid_midpoint |= SWSR_ReadBits( hSwSrContext, 16 );
            psSeiToneMappingInfo->sigmoid_width = ( SWSR_ReadBits( hSwSrContext, 16 ) << 16 );
            psSeiToneMappingInfo->sigmoid_width |= SWSR_ReadBits( hSwSrContext, 16 );
        }
        else if ( psSeiToneMappingInfo->model_id == 2 )
        {
            if ( ( psSeiToneMappingInfo->coded_data_bit_depth > 14 ) || ( psSeiToneMappingInfo->coded_data_bit_depth < 8 ) )
            {
                // Error case, return early so at least we do not parse out of current payload bounds
                return;
            }
            if ( psSeiToneMappingInfo->target_bit_depth > 16 )
            {
                // Error case, return early so at least we do not parse out of current payload bounds
                return;
            }
            for ( i = 0; i < ( 1UL << psSeiToneMappingInfo->target_bit_depth ); i++ )
            {
                psSeiToneMappingInfo->start_of_coded_interval[ i ] = SWSR_ReadBits( hSwSrContext, ROUND_UP_TO_8(psSeiToneMappingInfo->coded_data_bit_depth) );
            }
        }
        else if ( psSeiToneMappingInfo->model_id == 3 )
        {
            if ( ( psSeiToneMappingInfo->coded_data_bit_depth > 14 ) || ( psSeiToneMappingInfo->coded_data_bit_depth < 8 ) )
            {
                // Error case, return early so at least we do not parse out of current payload bounds
                return;
            }
            psSeiToneMappingInfo->num_pivots = SWSR_ReadBits( hSwSrContext, 16 );
            for ( i = 0; i < psSeiToneMappingInfo->num_pivots; i++ )
            {
                psSeiToneMappingInfo->target_pivot_value[ i ] = SWSR_ReadBits( hSwSrContext, ROUND_UP_TO_8(psSeiToneMappingInfo->coded_data_bit_depth) );
            }
        }
    }
}
#endif


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
)
{
    IMG_BOOL  bMoreData = IMG_TRUE;
    IMG_INT32 payloadType = 0;
    IMG_INT32 payloadSize = 0;
    IMG_UINT64 ui64BitOffsetBeforePayloadParse = 0;
    IMG_UINT64 ui64BitOffsetAfterPayloadParse = 0;
    IMG_INT32 i32NonConsumedPayloadBits = 0;
    IMG_UINT8 ui8TempByte = 0;      // We use that to deviate slightly from the standard definition for efficiency, the result is the same
    BSPP_eErrorType eParseError = BSPP_ERROR_NONE;

    IMG_ASSERT(hSwSrContext != IMG_NULL);
    {
        do
        {
            payloadType = 0;
            // Until the "payloadType" is finalised, ui8TempByte plays the role of "ff_byte" except for 
            // the last read (when running) where it plays the role of "last_payload_type_byte"
            ui8TempByte = SWSR_ReadBits(hSwSrContext, 8);
            while (ui8TempByte == 0xFF)     // In non-errored cases this condition should fail
            {
                payloadType += 255;
                ui8TempByte = SWSR_ReadBits(hSwSrContext, 8);
            }
            payloadType += ui8TempByte;     // this is the last byte read, corresponds to the "last_payload_type_byte" in the standard

            payloadSize = 0;
            // Until the "payloadSize" is finalised, ui8TempByte plays the role of "ff_byte" except for 
            // the last read (when running) where it plays the role of "last_payload_size_byte"
            ui8TempByte = SWSR_ReadBits(hSwSrContext, 8);
            while (ui8TempByte == 0xFF)     // This is valid to loop multiple times
            {
                payloadSize += 255;
                ui8TempByte = SWSR_ReadBits(hSwSrContext, 8);
            }
            payloadSize += ui8TempByte;     // this is the last byte read, corresponds to the "last_payload_type_byte" in the standard

            if (SWSR_GetTotalBitsConsumed(hSwSrContext, &ui64BitOffsetBeforePayloadParse) != IMG_SUCCESS)
            {
                eParseError = BSPP_ERROR_DECODE;
                break;
            }

            switch ( payloadType )     // sei_payload( type, size );
            {
            case  SEI_BUFFERING_PERIOD:
#ifdef PARSE_SEI_BUFFERING_PERIOD
                bspp_H264ParseSeiBufferingPeriod(hSwSrContext, &psH264SEIInfo->sSeiBufferingPeriod, hStrRes, pui32ActiveSpsForSeiParsing);   // This will set and use the Active SPS
#endif
                break;
            case  SEI_PIC_TIMING:
#ifdef PARSE_SEI_PIC_TIMING
                bspp_H264ParseSeiPicTiming(hSwSrContext, &psH264SEIInfo->sSeiPicTiming, hStrRes, *pui32ActiveSpsForSeiParsing); // This will use ONLY the Active SPS
#endif
                break;
            case  SEI_PAN_SCAN_RECT:
#ifdef PARSE_SEI_PAN_SCAN_RECT
                bspp_H264ParseSeiPanScanRect(hSwSrContext, &psH264SEIInfo->sSeiPanScanRect);
#endif
                break;
            case  SEI_FILLER_PAYLOAD:
#ifdef PARSE_SEI_FILLER_PAYLOAD
                bspp_H264ParseSeiFillerPayload(hSwSrContext, &psH264SEIInfo->sSeiFillerPayload, payloadSize);
#endif
                break;
            case  SEI_USER_DATA_REGISTERED_ITU_T_T35:
#ifdef PARSE_SEI_USER_DATA_REGISTERED_ITU_T_T35
                bspp_H264ParseSeiUserDataRegisteredItuTT35(hSwSrContext, &psH264SEIInfo->sSeiUserDataRegisteredItuTT35, payloadSize);
#endif
                break;
            case  SEI_USER_DATA_UNREGISTERED:
#ifdef PARSE_SEI_USER_DATA_UNREGISTERED
                bspp_H264ParseSeiUserDataUnregistered(hSwSrContext, &psH264SEIInfo->sSeiUserDataUnregistered, payloadSize);
#endif
                break;
            case  SEI_RECOVERY_POINT:
#ifdef PARSE_SEI_RECOVERY_POINT
                bspp_H264ParseSeiRecoveryPoint(hSwSrContext, &psH264SEIInfo->sSeiRecoveryPoint);
#endif
                break;
            case  SEI_DEC_REF_PIC_MARKING_REPETITION:
#ifdef PARSE_SEI_DEC_REF_PIC_MARKING_REPETITION
                bspp_H264ParseSeiDecRefPicMarkingRepetition(hSwSrContext, &psH264SEIInfo->sSeiDecRefPicMarkingRepetition, hStrRes, *pui32ActiveSpsForSeiParsing);   // This will use ONLY the Active SPS
#endif
                break;
            case  SEI_SPARE_PIC:
#ifdef PARSE_SEI_SPARE_PIC
                bspp_H264ParseSeiSparePic(hSwSrContext, &psH264SEIInfo->sSeiSparePic, hStrRes, *pui32ActiveSpsForSeiParsing);   // This will use ONLY the Active SPS
#endif
                break;
            case  SEI_SCENE_INFO:
#ifdef PARSE_SEI_SCENE_INFO
                bspp_H264ParseSeiSceneInfo(hSwSrContext, &psH264SEIInfo->sSeiSceneInfo);
#endif
                break;
            case  SEI_SUB_SEQ_INFO:
#ifdef PARSE_SEI_SUB_SEQ_INFO
                bspp_H264ParseSeiSubSeqInfo(hSwSrContext, &psH264SEIInfo->sSeiSubSeqInfo);
#endif
                break;
            case  SEI_SUB_SEQ_LAYER_CHARACTERISTICS:
#ifdef PARSE_SEI_SUB_SEQ_LAYER_CHARACTERISTICS
                bspp_H264ParseSeiSubSeqLayerCharacteristics(hSwSrContext, &psH264SEIInfo->sSeiSubSeqLayerCharacteristics);
#endif
                break;
            case  SEI_SUB_SEQ_CHARACTERISTICS:
#ifdef PARSE_SEI_SUB_SEQ_CHARACTERISTICS
                bspp_H264ParseSeiSubSeqCharacteristics(hSwSrContext, &psH264SEIInfo->sSeiSubSeqCharacteristics);
#endif
                break;
            case  SEI_FULL_FRAME_FREEZE:
#ifdef PARSE_SEI_FULL_FRAME_FREEZE
                bspp_H264ParseSeiFullFrameFreeze(hSwSrContext, &psH264SEIInfo->sSeiFullFrameFreeze);
#endif
                break;
            case  SEI_FULL_FRAME_FREEZE_RELEASE:
#ifdef PARSE_SEI_FULL_FRAME_FREEZE_RELEASE
                bspp_H264ParseSeiFullFrameFreezeRelease(hSwSrContext, &psH264SEIInfo->sSeiFullFrameFreezeRelease);
#endif
                break;
            case  SEI_FULL_FRAME_SNAPSHOT:
#ifdef PARSE_SEI_FULL_FRAME_SNAPSHOT
                bspp_H264ParseSeiFullFrameSnapshot(hSwSrContext, &psH264SEIInfo->sSeiFullFrameSnapshot);
#endif
                break;
            case  SEI_PROGRESSIVE_REFINEMENT_SEGMENT_START:
#ifdef PARSE_SEI_PROGRESSIVE_REFINEMENT_SEGMENT_START
                bspp_H264ParseSeiProgressiveRefinementSegmentStart(hSwSrContext, &psH264SEIInfo->sSeiProgressiveRefinementSegmentStart);
#endif
                break;
            case  SEI_PROGRESSIVE_REFINEMENT_SEGMENT_END:
#ifdef PARSE_SEI_PROGRESSIVE_REFINEMENT_SEGMENT_END
                bspp_H264ParseSeiProgressiveRefinementSegmentEnd(hSwSrContext, &psH264SEIInfo->sSeiProgressiveRefinementSegmentEnd);
#endif
                break;
            case  SEI_MOTION_CONSTRAINED_SLICE_GROUP_SET:
#ifdef PARSE_SEI_MOTION_CONSTRAINED_SLICE_GROUP_SET
                bspp_H264ParseSeiMotionConstrainedSliceGroupSet(hSwSrContext, &psH264SEIInfo->sSeiMotionConstrainedSliceGroupSet);
#endif
                break;
            case  SEI_FILM_GRAIN_CHARACTERISTICS:
#ifdef PARSE_SEI_FILM_GRAIN_CHARACTERISTICS
                bspp_H264ParseSeiFilmGrainCharacteristics(hSwSrContext, &psH264SEIInfo->sSeiFilmGrainCharacteristics);
#endif
                break;
            case  SEI_DEBLOCKING_FILTER_DISPLAY_PREFERENCE:
#ifdef PARSE_SEI_DEBLOCKING_FILTER_DISPLAY_PREFERENCE
                bspp_H264ParseSeiDeblockingFilterDisplayPreference(hSwSrContext, &psH264SEIInfo->sSeiDeblockingFilterDisplayPreference);
#endif
                break;
            case  SEI_STEREO_VIDEO_INFO:
#ifdef PARSE_SEI_STEREO_VIDEO_INFO
                bspp_H264ParseSeiStereoVideoInfo(hSwSrContext, &psH264SEIInfo->sSeiStereoVideoInfo);
#endif
                break;
            case  SEI_POST_FILTER_HINT:
#ifdef PARSE_SEI_POST_FILTER_HINT
                bspp_H264ParseSeiPostFilterHint(hSwSrContext, &psH264SEIInfo->sSeiPostFilterHint);
#endif
                break;
            case  SEI_TONE_MAPPING_INFO:
#ifdef PARSE_SEI_TONE_MAPPING_INFO
                bspp_H264ParseSeiToneMappingInfo(hSwSrContext, &psH264SEIInfo->sSeiToneMappingInfo);
#endif
                break;
            /* MVC / SVC from here on, UNSUPPORTED */
            case SEI_SCALABLE:
            case SEI_SUB_PIC:
            case SEI_NON_REQUIRED:
            case SEI_SCALABLE_LAYERS_NOT_PRESENT:
            case SEI_SCALABLE_DEPENDENCY_CHANGE:
            case SEI_SCALABLE_NESTING:
            case SEI_PARALLEL_DEC:
            case SEI_VIEW_SCALABILITY_INFO:
            case SEI_MULTIVIEW_SCENE_INFO:
            case SEI_MULTIVIEW_ACQUISITION_INFO:
            case SEI_NON_REQ_VIEW_INFO:
            case SEI_VIEW_DEPENDENCY_STRUCTURE:
            case SEI_OP_NOT_PRESENT:
            case SEI_QUALITYLEVEL:
            case SEI_RESERVED:
            default:
                break;
            }

            if (SWSR_GetTotalBitsConsumed(hSwSrContext, &ui64BitOffsetAfterPayloadParse) != IMG_SUCCESS)
            {
                eParseError = BSPP_ERROR_DECODE;
                break;
            }

            i32NonConsumedPayloadBits = (payloadSize << 3) - (IMG_INT32)(ui64BitOffsetAfterPayloadParse - ui64BitOffsetBeforePayloadParse);
            if (i32NonConsumedPayloadBits < 0)
            {
                // Parsing overstepped the defined size for the payload
                eParseError = BSPP_ERROR_DECODE;
                break;
            }

            // Consume whatever is left from the parsing (if parsing even happened)
            while(i32NonConsumedPayloadBits >= 16)
            {
                SWSR_ReadBits(hSwSrContext, 16);
                i32NonConsumedPayloadBits -= 16;
            }
            if (i32NonConsumedPayloadBits > 0)
            {
                SWSR_ReadBits(hSwSrContext, i32NonConsumedPayloadBits);
            }

            SWSR_CheckMoreRbspData( hSwSrContext, &bMoreData );

        } while( bMoreData ); // Break when SEI message has finished   
    }

    return eParseError;
}



