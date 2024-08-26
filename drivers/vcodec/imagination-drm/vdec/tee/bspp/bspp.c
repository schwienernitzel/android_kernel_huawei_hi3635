/*!
 *****************************************************************************
 *
 * @File       bspp.c
 * @Title      VXD Bitstream Buffer Pre-Parser
 * @Description    This file contains the implementation of VXD Bitstream Buffer Pre-Parser.
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
#include "bspp.h"
#include "swsr.h"
#include "lst.h"
#include "securemem.h"
#include "h264_secure_parser.h"
#include "h264_secure_sei_parser.h"


// code for logging.
#if defined(BSPP_LOG) && !defined (BSPP_KM) && defined (BSPP_UM)
#include <stdio.h>
FILE * fpLog = IMG_NULL;
#endif

#define BSPP_ERR_MSG_LENGTH     1024


/*!
******************************************************************************
  This type defines the exception flag to catch the error if more catch block
  is required to catch different kind of errror then more enum can be added
******************************************************************************/
typedef enum
{
    BSPP_EXCEPTION_HANDLER_NONE = 0x00,         //!< BSPP parse exception handler
    BSPP_EXCEPTION_HANDLER_JUMP,                //!< Jump at exception (external use)
} BSPP_EXCEPTION_HANDLER;


/*!
******************************************************************************
    Macros to define try ,catch and throw concepts of c++
    These Macros can be used to handle any parsing error
    Error can be traped by using SWSR_TRY ,SWSR_THROW,SWSR_CATCH macros something lik this

    SWSR_TRY(SWSR_GetExceptionHandler(&psContext->pSWSRContext))  <block start>
    {
        do the parsing
    }
    SWSR_CATCH( BSPP_EXCEPTION_HANDLER_JUMP )
    {
    }
    SWSR_ETRY;  <block end>
******************************************************************************/

/* Disabled for secure implementation */
#define BSPP_TRY(ex_buf_) do{  switch( 0 ){ case 0:     //<! "try" block to start exception handling
#define BSPP_CATCH(x) break; case x:                    //<! "catch" block to catch exception after throw
#define BSPP_ETRY } }while(0)                           //<! end of exception handler block
#define BSPP_THROW(ex_buf_ ,x)                          //<! "catch" block to show exception this is basically used by  swsr
#define BSPP_EXIT(psContext)

typedef struct
{
    IMG_HANDLE hMutex;

} BSPP_sMutex;


#include "sysos_api.h"

#define SEC_CreateMutex(phMutex)        SYSOS_SEC_CreateMutex(phMutex)
#define SEC_DestroyMutex(hMutex)        SYSOS_SEC_DestroyMutex(hMutex)

// Replace the locks with OS mutex.
#define LOCK_HP(sMutex) SYSOS_SEC_LockMutex(sMutex.hMutex)
#define UNLOCK_HP(sMutex) SYSOS_SEC_UnlockMutex(sMutex.hMutex)

#define LOCK_LP(sMutex) SYSOS_SEC_LockMutex(sMutex.hMutex)
#define UNLOCK_LP(sMutex) SYSOS_SEC_UnlockMutex(sMutex.hMutex)


/*!
******************************************************************************
 ALWAYS KEEP IN SYNC TO VDEC_eVidStd STRUCTURE!!!
******************************************************************************/
IMG_UINT32 BSPP_ParseSequSize[] =
{
    0,                              /*!< Video standard not defined.              */
    0,                              /*!< MPEG2 (includes MPEG1).                  */
    0,                              /*!< MPEG4 (includes H263 and Sorenson Spark) */
    0,                              /*!< H263 (see MPEG4).                        */
    sizeof(BSPP_sH264SequHdrInfo),  /*!< H264.                                    */
    0,                              /*!< VC1 (includes WMV9).                     */
    0,                              /*!< AVS.                                     */
    0,                              /*!< RealVideo (RV30 and RV40).               */
    0,                              /*!< JPEG.                                    */
    0,                              /*!< On2 VP6.                                 */
    0,                              /*!< On2 VP8.                                 */
    0,                              /*!< Sorenson Spark (see MPEG4 above)         */
    0,                              /*!< HEVC.                                    */
    BSPP_INVALID                    /*!< Max. video standard.                     */
};

/*!
******************************************************************************
 ALWAYS KEEP IN SYNC WITH VDEC_eVidStd STRUCTURE!
******************************************************************************/
IMG_UINT32 BSPP_ParsePpsSize[] =
{
    0,                         /*!< Video standard not defined.         */
    0,                         /*!< MPEG2 (includes MPEG1).             */
    0,                         /*!< MPEG4 (includes H263 and Sorenson). */
    0,                         /*!< H263 (see MPEG4).                   */
    sizeof(BSPP_sH264PPSInfo), /*!< H264.                               */
    0,                         /*!< VC1 (includes WMV9).                */
    0,                         /*!< AVS.                                */
    0,                         /*!< RealVideo (RV30 and RV40).          */
    0,                         /*!< JPEG.                               */
    0,                         /*!< On2 VP6.                            */
    0,                         /*!< On2 VP8.                            */
    0,                         /*!< Sorenson Spark (see MPEG4 above)    */
    0,                          /*!< HEVC.                               */
    BSPP_INVALID               /*!< Max. video standard.                */
};

/*!
******************************************************************************

 @Function                BSPP_pfnParseUnit

 @Description

 This is a function prototype for the free item callback functions.

 @Input     hSwSrContext : A handle to software shift-register context.

 @InOut     psUnitData : A pointer to unit data which includes input and output
                         parameters as defined by structure.

 @Return   IMG_RESULT : This function returns either IMG_SUCCESS or an error code.

******************************************************************************/
typedef IMG_RESULT (* BSPP_pfnParseUnit) (
    IMG_HANDLE          hSwSrContext,
    BSPP_sUnitData    * psUnitData
);


/*!
******************************************************************************
 This structure contains bitstream buffer information.
 @brief  BSPP Bitstream Buffer Information
 ******************************************************************************/
typedef struct
{
    LST_LINK;

    BSPP_sDdBufInfo         sDdBufInfo;        /*!< Bitstream buffer.                                  */
    IMG_UINT32              ui32DataSize;      /*!< Size of bitstream data contained within buffer.    */
    IMG_UINT32              ui32BufMapId;      /*!< Buffer Map Id                                      */
    VDEC_eBstrElementType   eBstrElementType;  /*!< Type of bitstream contained within buffer.         */
    IMG_UINT64              ui64BytesRead;     /*!< Number of bytes read from current bitstream buffer.
                                                    Updated at the end of every parsed unit (after
                                                    seek for next unit) or when buffer is exhausted.   */
    IMG_UINT64              ui64PictTagParam;     /*!< Associated timestamp if it exists.                */

} BSPP_sBitstreamBuffer;


/*!
******************************************************************************
 This structure contains shift-register state.
 @brief  BSPP Shift-register State
 ******************************************************************************/
typedef struct
{
    IMG_HANDLE          hSwSrContext;               /*!< Software shift-register context                            */
    IMG_UINT32          jump_buffer;
    SWSR_eException     eException;                 /*!< Exception set from shift-register handler.                 */

} BSPP_sParseCtx;


/*!
******************************************************************************
 This structure contains context for the current picture.
 @brief  BSPP Picture Context
 ******************************************************************************/
typedef struct
{
    BSPP_sSequenceHdrInfo * psSequHdrInfo;
    IMG_BOOL                bClosedGOP;
    BSPP_sPictHdrInfo       sPictHdrInfo[VDEC_H264_MVC_MAX_VIEWS];

    BSPP_sSequenceHdrInfo * psExtSequHdrInfo;                             //sequence header info for additional views

    IMG_BOOL                bPresent;
    IMG_BOOL                bInvalid;
    IMG_BOOL                bUnsupported; // set when parser returns BSPP_ERROR_UNSUPPORTED flag set in eParseError
    IMG_BOOL                bFinished;

} BSPP_sPictCtx;


/*!
******************************************************************************
 This structure contains resources allocated for the stream.
 @brief  BSPP Stream Resource Allocations
 ******************************************************************************/
typedef struct
{
    LST_T               asSequenceDataList[SEQUENCE_SLOTS_SECURE];
    LST_T               asPPSDataList[PPS_SLOTS_SECURE];

	LST_T               sAvailableSequencesList;
    LST_T               sAvailablePPSsList;

    BSPP_sH264SEIInfo   *psSEIDataInfo;
} BSPP_sStreamAllocData;


/*!
******************************************************************************
 This structure contains bitstream parsing state information for the current
 group of buffers.
 @brief  BSPP Bitstream Parsing State Information
 ******************************************************************************/
typedef struct
{
    VDEC_eVidStd            eVidStd;                    /*!< Video standard of stream required in shift-register
                                                             callback to determine the type of the next unit.           */
    IMG_BOOL                bDisableMvc;                /*!< If set mvc extension unit will not be parsed               */

    IMG_BOOL                bDelimPresent;              /*!< Indicates when a delimiter is present for the current unit.*/

    IMG_HANDLE              hSwSrContext;               /*!< Software shift-register context                            */

    BSPP_eUnitType          eUnitType;                  /*!< Type of the current unit being processed in this group
                                                             of buffers.                                                */
    BSPP_eUnitType          eLastUnitType;              /*!< Type of the last unit processed in this group of buffers.  */
    IMG_BOOL                bNotPicUnitYet;             /*!< True if we have not processed a picture unit yet           */
    IMG_BOOL                bNotExtPicUnitYet;          /*!< True if we have not processed a extension picture unit yet */
    IMG_UINT32              ui32TotalDataSize;          /*!< Total amount of data in group of buffers (in bytes).       */

    IMG_UINT32              ui32TotalBytesRead;         /*!< Cumulative number of bytes read from whole, and exhausted,
                                                             bitstream buffers.                                         */

    LST_T                   sBufferChain;               /*!< List containing all bitstream buffers in group.            */

    LST_T                   sInFlightBufs;              /*!< List containing all bitstream buffers currently being
                                                             processed by the software shift-register.                  */

    LST_T                 * apsPrePictSegList[3];       /*!< Bitstream segment list containing all data in the group
                                                             *before* each picture.                                      */
    LST_T                 * apsPictSegList[3];          /*!< Bitstream segment list containing all data in the group
                                                             for each picture. This includes any spurious units/data
                                                             that is sandwiched between any slices/picture headers/data.
                                                             It is necessary to include these items since we can only
                                                             have a finite pre-calculable number of segments.           */
    IMG_UINT64            * apui64PictTagParam[3];

    LST_T                 * psSegmentList;              /*!< Bitstream segment list to use for data from the
                                                             current unit.                                              */
    IMG_UINT64            * pui64PictTagParam;

    LST_T                 * psFreeSegments;             /*!< List of user-supplied (available) segments for this
                                                             pre-parse operation (group of buffers).                    */

    IMG_UINT32              ui32SegmentOffset;          /*!< Offset (in bytes) in buffer to start of next segment.      */

    IMG_BOOL                bInsertStartCode;           /*!< Insert start code (24-bit) in the next bitstream segment.  */
    IMG_UINT8               ui8StartCodeSuffix;         /*!< Start code suffix to use when bInsertStartCode.            */

    IMG_UINT8               ui8CurrentViewIdx;          /*!< Current view index                                         */

} BSPP_sGrpBstrCtx;


/*!
******************************************************************************
 This structure contains the stream context information.
 @brief  BSPP Stream Context Information
 ******************************************************************************/
typedef struct
{
    VDEC_eVidStd            eVidStd;                    /*!< Video standard corresponding to the buffers provided for
                                                             pre-parsing.                                               */
    IMG_BOOL                bDisableMvc;                /*!< If set mvc extension unit will not be parsed               */

    IMG_BOOL                bFullScan;                  /*!< Search for bitstream data units (e.g. NALs) beyond the
                                                             first video slice in bitstream buffer group for picture.
                                                             This is necessary when non-picture data (PPS/SEI) might
                                                             follow slice. NOTE: CPU utilisation might significantly
                                                             increase and impact performance.                           */
    IMG_BOOL                bImmediateDecode;           /*!< Signaling to start Decode immediately from next picture.   */

    VDEC_eBstrFormat        eBstrFormat;                /*!< The bitstream format describes how the data will be
                                                             presented within each buffer. This shall be combined with
                                                             eVidStd, BSPP_sBitstreamBuffer.eBstrElementType to
                                                             determine how to process the data.                         */

    VDEC_sCodecConfig       sCodecConfig;               /*!< Out-of-band (outside bitstream buffers) standard-specific
                                                             data necesary for pre-parsing and/or decoding.             */

    IMG_UINT32              ui32UserStrId;              /*!< Stream ID passed by VDEC_StreameCreate.          */

    SWSR_sConfig            sSrConfig;                  /*!< Default configuration for the shift-register for this
                                                             stream. The delimiter type may be adjusted for each unit
                                                             where the buffer requires it. Information about how to
                                                             process each unit will be passed down with the picture
                                                             header information.                                        */

    SWSR_eEmPrevent         eEmulationPrevention;       /*!< Emulation prevention scheme present in bitstream. This is
                                                             sometimes not ascertained (e.g. VC-1) until the first
                                                             bitstream buffer (often codec configuration) has been
                                                             received.                                                  */

    IMG_HANDLE              hSwSrContext;               /*!< Software shift-register context.                           */

    BSPP_pfnParseUnit       pfnParseUnit;               /*!< Pointer to standard-specific unit parsing function.        */

    BSPP_sStreamAllocData   sStrAlloc;                  /*!< Stream allocation data.                                    */

    IMG_UINT32              ui32SequHdrId;              /*!< Last sequence header ID used.                              */
    IMG_UINT8 *             pui8SequHdrInfo;            /*!< Memory cointainer for sSequHdrInfo sequence structures     */

    IMG_UINT8 *             pui8SecureSequenceInfo;     /*!< Memory cointainer for hSecureSequenceInfo handles          */
    IMG_UINT8 *             pui8PPSInfo;                /*!< Memory cointainer for sPPSInfo sequence structures         */
    IMG_UINT8 *             pui8SecurePPSInfo;          /*!< Memory cointainer for hSecurePPSInfo handles               */
    BSPP_sGrpBstrCtx        sGrpBstrCtx;                /*!< Context for current group of buffers.                      */

    BSPP_sParseCtx          sParseCtx;                  /*!< Parsing context used for exception handling.               */

    inter_pict_data         sInterPictData;             /*!< Storage for context that needs to be retained between
                                                             pictures. A portion of this structure is reserved for
                                                             standard-specific data.                                    */

    LST_T                   sDecodedPicturesList;       /*!< List of decoded pictures that have not been processed.     */

    BSPP_sMutex             sMutex;                     /*!< Mutex per stream needed for access to the decoded
                                                             pictures list (see above).                                 */

    IMG_BOOL                bIntraFrmAsClosedGop;       /*!< To turn on/off considering I-Frames as ClosedGop boundaries. */

#if defined(BSPP_LOG) && !defined (BSPP_KM) && defined (BSPP_UM)
    FILE                    *fpLog;
#endif

} BSPP_sStrContext;


/*!
******************************************************************************

 @Function              BSPP_GetPpsHdr

 @Description

 Obtains the most recent Picture Parameter Set header of a given Id.

******************************************************************************/
BSPP_sPPSInfo *
BSPP_GetPpsHdr(
    IMG_HANDLE  hStrRes,
    IMG_UINT32  ui32PpsId
)
{
    BSPP_sStreamAllocData * psAllocData = (BSPP_sStreamAllocData*)hStrRes;

    IMG_ASSERT(psAllocData);
    if (ui32PpsId >= PPS_SLOTS_SECURE || psAllocData == IMG_NULL)
    {
        return IMG_NULL;
    }

    return LST_last(&psAllocData->asPPSDataList[ui32PpsId]);
}


/*!
******************************************************************************

 @Function              BSPP_GetSequHdr

 @Description

 Obtains the most recent sequence header of a given Id.

******************************************************************************/
BSPP_sSequenceHdrInfo *
BSPP_GetSequHdr(
    IMG_HANDLE  hStrRes,
    IMG_UINT32  ui32SequId
)
{
    BSPP_sStreamAllocData * psAllocData = (BSPP_sStreamAllocData*)hStrRes;

    IMG_ASSERT(psAllocData);
    if (ui32SequId >= SEQUENCE_SLOTS_SECURE || psAllocData == IMG_NULL)
    {
        return IMG_NULL;
    }

    return LST_last(&psAllocData->asSequenceDataList[ui32SequId]);
}


/*!
******************************************************************************

 @Function              BSPP_GetSEIDataInfo

 @Description

 Obtains the SEI info Data structure. (applicable only for H264)

******************************************************************************/
BSPP_sH264SEIInfo *
BSPP_GetSEIDataInfo(
    IMG_HANDLE hStrRes
)
{
    BSPP_sStreamAllocData * psAllocData = (BSPP_sStreamAllocData *) hStrRes;

    IMG_ASSERT(psAllocData != IMG_NULL);
    if (psAllocData == IMG_NULL)
    {
        return IMG_NULL;
    }

    return psAllocData->psSEIDataInfo;
}

/*!
******************************************************************************

 @Function              bspp_FreeBitstreamElem

 @Description

 Frees a bitstream chain element.

******************************************************************************/
static IMG_RESULT
bspp_FreeBitstreamElem(
    BSPP_sBitstreamBuffer * psBstrBuf
)
{
    IMG_ASSERT(psBstrBuf);

    // Unmapping the buffer once done with it.
    if (psBstrBuf->sDdBufInfo.pvCpuVirt)
        SECMEM_UnmapSecureMemory(psBstrBuf->sDdBufInfo.pvCpuVirt);

    IMG_MEMSET(psBstrBuf, 0, sizeof(BSPP_sBitstreamBuffer));	

    IMG_FREE(psBstrBuf);

    return IMG_SUCCESS;
}


/*!
******************************************************************************

 @Function              bspp_CreateSegment

 @Description

 Constructs a bitstream segment for the current unit and adds it to the list.

******************************************************************************/
static IMG_RESULT
bspp_CreateSegment(
    BSPP_sGrpBstrCtx      * psGrpBstrCtx,
    BSPP_sBitstreamBuffer * psCurBuf
)
{
    BSPP_sBitStrSeg   * psSegment;
    IMG_UINT32          ui32Result;

    IMG_ASSERT(psGrpBstrCtx);

    // Only create a segment when data (not in a previous segment) has been parsed from the buffer.
    if (psCurBuf->ui64BytesRead != psGrpBstrCtx->ui32SegmentOffset)
    {
        /* Allocate a software shift-register context structure...*/
        psSegment = LST_removeHead(psGrpBstrCtx->psFreeSegments);
        IMG_ASSERT(psSegment != IMG_NULL);
        if (psSegment == IMG_NULL)
        {
            REPORT(REPORT_MODULE_BSPP, REPORT_ERR,
                "Exhausted bitstream segments during pre-parsing");

            ui32Result = IMG_ERROR_COULD_NOT_OBTAIN_RESOURCE;
            goto error;
        }
        IMG_MEMSET(psSegment, 0, sizeof(BSPP_sBitStrSeg));

        //psSegment->hBufMapInfo = psCurBuf->psBufInfo->hBufMapHandle;
        psSegment->ui32BufMapId = psCurBuf->ui32BufMapId;// psSegment->psBufMapInfo->ui32BufMapId;
        psSegment->ui32DataSize = (IMG_UINT32)psCurBuf->ui64BytesRead - psGrpBstrCtx->ui32SegmentOffset;
        psSegment->ui32DataByteOffset = psGrpBstrCtx->ui32SegmentOffset;

        if (psCurBuf->ui64BytesRead == psCurBuf->ui32DataSize)
        {
            // This is the last segment in the buffer.
            psSegment->ui32BitStrSegFlag |= VDECDD_BSSEG_LASTINBUFF;
        }

        // Next segment will start part way through the buffer (current read position).
        psGrpBstrCtx->ui32SegmentOffset = (IMG_UINT32)psCurBuf->ui64BytesRead;


        if (psGrpBstrCtx->bInsertStartCode)
        {
            psSegment->ui32BitStrSegFlag |= VDECDD_BSSEG_INSERT_STARTCODE;
            psSegment->ui8StartCodeSuffix = psGrpBstrCtx->ui8StartCodeSuffix;
            psGrpBstrCtx->bInsertStartCode = IMG_FALSE;
        }

        LST_add(psGrpBstrCtx->psSegmentList, psSegment);

        // If multiple segments correspond to the same (picture) stream-unit, update it only the first time
        if (psCurBuf->ui64PictTagParam &&
            psGrpBstrCtx->pui64PictTagParam &&
            (psGrpBstrCtx->psSegmentList == psGrpBstrCtx->apsPictSegList[0] ||
             psGrpBstrCtx->psSegmentList == psGrpBstrCtx->apsPictSegList[1] ||
             psGrpBstrCtx->psSegmentList == psGrpBstrCtx->apsPictSegList[2]
            )
           )
        {
            *psGrpBstrCtx->pui64PictTagParam = psCurBuf->ui64PictTagParam;
        }
    }

    return IMG_SUCCESS;

error:
    IMG_ASSERT(ui32Result == IMG_ERROR_COULD_NOT_OBTAIN_RESOURCE);
    return ui32Result;
}


/*!
******************************************************************************

 @Function              bspp_DetermineUnitType

******************************************************************************/
static IMG_RESULT
bspp_DetermineUnitType(
    VDEC_eVidStd        eVidStd,
    IMG_UINT8           ui8UnitType,
    IMG_BOOL            bDisableMvc,
    BSPP_eUnitType    * peUnitType
)
{
    IMG_UINT32      ui32Result;

    // Determine the unit type from the NAL type.
    switch (eVidStd)
    {
     case VDEC_STD_H264:
        if ( (ui8UnitType & 0x1f) == H264_NALTYPE_SLICE_PREFIX )
        {
            *peUnitType = bDisableMvc ? BSPP_UNIT_UNCLASSIFIED : BSPP_UNIT_PICTURE;
        }
        else if ( (ui8UnitType & 0x1f) == H264_NALTYPE_SUBSET_SPS )
        {
            *peUnitType = bDisableMvc ? BSPP_UNIT_UNCLASSIFIED : BSPP_UNIT_SEQUENCE;
        }
        else if ((ui8UnitType & 0x1f) == H264_NALTYPE_SLICE_SCALABLE ||
                 (ui8UnitType & 0x1f) == H264_NALTYPE_SLICE_IDR_SCALABLE)
        {
            *peUnitType = bDisableMvc ? BSPP_UNIT_NON_PICTURE : BSPP_UNIT_PICTURE;
        }
        else if ((ui8UnitType & 0x1f) == H264_NALTYPE_SEQUENCE_PARAMETER_SET )
        {
            *peUnitType = BSPP_UNIT_SEQUENCE;
        }
        else if ((ui8UnitType & 0x1f) == H264_NALTYPE_PICTURE_PARAMETER_SET)
        {
            *peUnitType = BSPP_UNIT_PPS_H264;
        }
        else if ((ui8UnitType & 0x1f) == H264_NALTYPE_SLICE ||
                 (ui8UnitType & 0x1f) == H264_NALTYPE_SLICE_PARTITION_A ||
                 (ui8UnitType & 0x1f) == H264_NALTYPE_SLICE_PARTITION_B ||
                 (ui8UnitType & 0x1f) == H264_NALTYPE_SLICE_PARTITION_C ||
                 (ui8UnitType & 0x1f) == H264_NALTYPE_IDR_SLICE)
        {
            *peUnitType = BSPP_UNIT_PICTURE;
        }
        else if ((ui8UnitType & 0x1f) == H264_NALTYPE_ACCESS_UNIT_DELIMITER ||
                 (ui8UnitType & 0x1f) == H264_NALTYPE_SUPPLEMENTAL_ENHANCEMENT_INFO)

        {
            // Each of these Nal units should not change unit type if current
            // is picture since they can occur anywhere, any number of times.
            *peUnitType = BSPP_UNIT_UNCLASSIFIED;
        }
        else if ((ui8UnitType & 0x1f) == H264_NALTYPE_END_OF_SEQUENCE ||
                 (ui8UnitType & 0x1f) == H264_NALTYPE_END_OF_STREAM ||
                 (ui8UnitType & 0x1f) == H264_NALTYPE_FILLER_DATA ||
                 (ui8UnitType & 0x1f) == H264_NALTYPE_SEQUENCE_PARAMETER_SET_EXTENTION ||
                 (ui8UnitType & 0x1f) == H264_NALTYPE_AUXILIARY_SLICE)
        {
            *peUnitType = BSPP_UNIT_NON_PICTURE;
        }
        else
        {
            *peUnitType = BSPP_UNIT_UNSUPPORTED;
        }
        break;

     default:
        IMG_ASSERT(IMG_FALSE);
        ui32Result = IMG_ERROR_INVALID_PARAMETERS;
        goto error;
        break;
    }

    return IMG_SUCCESS;

error:
    return ui32Result;
}


/*!
******************************************************************************

 @Function              bspp_ShiftRegCb

******************************************************************************/
static IMG_VOID
bspp_ShiftRegCb(
    SWSR_eCbEvent       eEvent,
    BSPP_sGrpBstrCtx  * psGrpBstrCtx,
    IMG_UINT8           ui8NalType,
    IMG_UINT8        ** ppsDataBuffer,
    IMG_UINT64        * pui64Datasize
)
{
    IMG_UINT32  ui32Result;

    switch (eEvent)
    {
    case SWSR_EVENT_INPUT_BUFFER_START:
        {
            BSPP_sBitstreamBuffer * psNextBuf;

            // Take the next bitstream buffer for use in shift-register.
            psNextBuf = LST_removeHead(&psGrpBstrCtx->sBufferChain);

            IMG_ASSERT(psNextBuf);
            IMG_ASSERT(ppsDataBuffer);
            IMG_ASSERT(pui64Datasize);

            if(psNextBuf && ppsDataBuffer && pui64Datasize)
            {
                LST_add(&psGrpBstrCtx->sInFlightBufs, psNextBuf);

                *ppsDataBuffer = psNextBuf->sDdBufInfo.pvCpuVirt;
                *pui64Datasize = psNextBuf->ui32DataSize;

                psNextBuf->ui64BytesRead = 0;

            }
            else
            {
                goto error;
            }
        }
        break;

    case SWSR_EVENT_OUTPUT_BUFFER_END:
        {
            BSPP_sBitstreamBuffer * psCurBuf;

            psCurBuf = LST_removeHead(&psGrpBstrCtx->sInFlightBufs);
            IMG_ASSERT(psCurBuf);

            if(psCurBuf)
            {
                // Indicate that the whole buffer content has been used.
                psCurBuf->ui64BytesRead = psCurBuf->ui32DataSize;
                psGrpBstrCtx->ui32TotalBytesRead += (IMG_UINT32)psCurBuf->ui64BytesRead;

                // Construct segment for current buffer and add to active list.
                ui32Result = bspp_CreateSegment(psGrpBstrCtx, psCurBuf);
                IMG_ASSERT(ui32Result == IMG_SUCCESS);
                if(ui32Result != IMG_SUCCESS)
                {
                    goto error;
                }

                // Next segment will start at the beginning of the next buffer.
                psGrpBstrCtx->ui32SegmentOffset = 0;

                // Destroy the bitstream element.
                ui32Result = bspp_FreeBitstreamElem(psCurBuf);
                IMG_ASSERT(ui32Result == IMG_SUCCESS);
                if(ui32Result != IMG_SUCCESS)
                {
                    goto error;
                }
            }
            else
            {
                goto error;
            }
        }
        break;

    case SWSR_EVENT_DELIMITER_NAL_TYPE:

        // Initialise the unit type with the last (unclassified or
        // unsupported types are not retained since they.
        psGrpBstrCtx->eUnitType = psGrpBstrCtx->eLastUnitType;

        // Determine the unit type without consuming any data (start code) from shift-register.
        // Segments are created automatically when a new buffer is requested by the shift-register
        // so the unit type must be known in order to switch over the segment list.
        ui32Result = bspp_DetermineUnitType(psGrpBstrCtx->eVidStd,
                                            ui8NalType,
                                            psGrpBstrCtx->bDisableMvc,
                                            &psGrpBstrCtx->eUnitType);
        IMG_ASSERT(ui32Result == IMG_SUCCESS);
        IMG_ASSERT(psGrpBstrCtx->eUnitType != BSPP_UNIT_NONE);

        // Only look to change bitstream segment list when the unit type is different and
        // the current unit contains data that could be placed in a new list.
        if (psGrpBstrCtx->eLastUnitType != psGrpBstrCtx->eUnitType &&
            psGrpBstrCtx->eUnitType != BSPP_UNIT_UNSUPPORTED &&
            psGrpBstrCtx->eUnitType != BSPP_UNIT_UNCLASSIFIED)
        {
            IMG_BOOL    bPrevPictData;
            IMG_BOOL    bCurrPictData;

            bPrevPictData = (psGrpBstrCtx->eLastUnitType == BSPP_UNIT_PICTURE ||
                             psGrpBstrCtx->eLastUnitType == BSPP_UNIT_SKIP_PICTURE) ? IMG_TRUE : IMG_FALSE;

            bCurrPictData = (psGrpBstrCtx->eUnitType == BSPP_UNIT_PICTURE ||
                             psGrpBstrCtx->eUnitType == BSPP_UNIT_SKIP_PICTURE) ? IMG_TRUE : IMG_FALSE;

            // When switching between picture and non-picture units.
            if ((bPrevPictData && !bCurrPictData) ||
                (!bPrevPictData && bCurrPictData))
            {
                // Only delimit unit change when we're not the first unit and
                // we're not already in the last segment list.
                if (psGrpBstrCtx->eLastUnitType != BSPP_UNIT_NONE &&
                    psGrpBstrCtx->psSegmentList != psGrpBstrCtx->apsPictSegList[2])
                {
                    BSPP_sBitstreamBuffer * psCurBuf = LST_first(&psGrpBstrCtx->sInFlightBufs);
                    IMG_ASSERT(IMG_NULL != psCurBuf);
                    if (IMG_NULL == psCurBuf)
                    {
                        goto error;
                    }

                    // Update the offset within current buffer.
                    SWSR_GetByteOffsetCurBuf(psGrpBstrCtx->hSwSrContext, &psCurBuf->ui64BytesRead);

                    // Create the last segment of the previous type (which may split a buffer into two).
                    // If the unit is exactly at the start of a buffer this will not create a zero-byte segment.
                    ui32Result = bspp_CreateSegment(psGrpBstrCtx, psCurBuf);
                    IMG_ASSERT(ui32Result == IMG_SUCCESS);
                    if (ui32Result != IMG_SUCCESS)
                    {
                        goto error;
                    }
                }

                // Point at the next segment list.
                if (psGrpBstrCtx->psSegmentList == psGrpBstrCtx->apsPrePictSegList[0])
                {
                    IMG_ASSERT(!bPrevPictData && bCurrPictData);
                    psGrpBstrCtx->psSegmentList = psGrpBstrCtx->apsPictSegList[0];
                    psGrpBstrCtx->pui64PictTagParam = psGrpBstrCtx->apui64PictTagParam[0];
                }
                else if (psGrpBstrCtx->psSegmentList == psGrpBstrCtx->apsPictSegList[0])
                {
                    IMG_ASSERT(bPrevPictData && !bCurrPictData);
                    psGrpBstrCtx->psSegmentList = psGrpBstrCtx->apsPrePictSegList[1];
                }
                else if (psGrpBstrCtx->psSegmentList == psGrpBstrCtx->apsPrePictSegList[1])
                {
                    IMG_ASSERT(!bPrevPictData && bCurrPictData);
                    psGrpBstrCtx->psSegmentList = psGrpBstrCtx->apsPictSegList[1];
                    psGrpBstrCtx->pui64PictTagParam = psGrpBstrCtx->apui64PictTagParam[1];
                }
                else if (psGrpBstrCtx->psSegmentList == psGrpBstrCtx->apsPictSegList[1])
                {
                    IMG_ASSERT(bPrevPictData && !bCurrPictData);
                    psGrpBstrCtx->psSegmentList = psGrpBstrCtx->apsPrePictSegList[2];
                }
                else if (psGrpBstrCtx->psSegmentList == psGrpBstrCtx->apsPrePictSegList[2])
                {
                    IMG_ASSERT(!bPrevPictData && bCurrPictData);
                    psGrpBstrCtx->psSegmentList = psGrpBstrCtx->apsPictSegList[2];
                    psGrpBstrCtx->pui64PictTagParam = psGrpBstrCtx->apui64PictTagParam[2];
                }
                else
                {
                    // Must place all remaining bitstream segments in the last picture segment list.
                    // This will only be used in the case of errors.
                    IMG_ASSERT(psGrpBstrCtx->psSegmentList == psGrpBstrCtx->apsPictSegList[2]);

                    REPORT(REPORT_MODULE_BSPP, REPORT_WARNING,
                           "Run out of bitstream segment lists, adding all subsequent segments to the last picture");
                }
            }

            psGrpBstrCtx->eLastUnitType = psGrpBstrCtx->eUnitType;
        }
        break;

    default:
        IMG_ASSERT(IMG_FALSE);
        break;
    }

error:
    return;
}


/*!
******************************************************************************

 @Function              bspp_ExceptionHandler

******************************************************************************/
static IMG_VOID
bspp_ExceptionHandler(
    SWSR_eException     eException,
    IMG_HANDLE          hParseCtx
)
{
    BSPP_sParseCtx * psParseCtx = (BSPP_sParseCtx *)hParseCtx;

    IMG_ASSERT(psParseCtx);

    // Store the exception.
    psParseCtx->eException = eException;

    switch (psParseCtx->eException)
    {
        case SWSR_EXCEPT_NO_EXCEPTION:
            REPORT(REPORT_MODULE_BSPP, REPORT_ERR,
                   "SWSR_EXCEPT_NO_EXCEPTION");
            break;
        case SWSR_EXCEPT_ENCAPULATION_ERROR1:
            REPORT(REPORT_MODULE_BSPP, REPORT_ERR,
                   "SWSR_EXCEPT_ENCAPULATION_ERROR1");
            break;
        case SWSR_EXCEPT_ENCAPULATION_ERROR2:
            REPORT(REPORT_MODULE_BSPP, REPORT_ERR,
                   "SWSR_EXCEPT_ENCAPULATION_ERROR2");
            break;
        case SWSR_EXCEPT_ACCESS_INTO_SCP:
            REPORT(REPORT_MODULE_BSPP, REPORT_ERR,
                   "SWSR_EXCEPT_ACCESS_INTO_SCP");
            break;
        case SWSR_EXCEPT_ACCESS_BEYOND_EOD:
            REPORT(REPORT_MODULE_BSPP, REPORT_ERR,
                   "SWSR_EXCEPT_ACCESS_BEYOND_EOD");
            break;
        case SWSR_EXCEPT_EXPGOULOMB_ERROR:
            REPORT(REPORT_MODULE_BSPP, REPORT_ERR,
                   "SWSR_EXCEPT_EXPGOULOMB_ERROR");
            break;
        case SWSR_EXCEPT_WRONG_CODEWORD_ERROR:
            REPORT(REPORT_MODULE_BSPP, REPORT_ERR,
                   "SWSR_EXCEPT_WRONG_CODEWORD_ERROR");
            break;
        case SWSR_EXCEPT_NO_SCP:
            REPORT(REPORT_MODULE_BSPP, REPORT_ERR,
                   "SWSR_EXCEPT_NO_SCP");
            break;
        case SWSR_EXCEPT_INVALID_CONTEXT:
            REPORT(REPORT_MODULE_BSPP, REPORT_ERR,
                   "SWSR_EXCEPT_INVALID_CONTEXT");
            break;

        default:
            break;
    }

    // Clear the exception.
    SWSR_CheckException(psParseCtx->hSwSrContext);

    // this throw will be hnalded in parsing function
    BSPP_THROW(psParseCtx->jump_buffer, BSPP_EXCEPTION_HANDLER_JUMP);
}


/*!
******************************************************************************

 @Function              bspp_ResetSequence

******************************************************************************/
IMG_VOID bspp_ResetSequence(
    BSPP_sSequenceHdrInfo *  psSequHdrInfo,
    VDEC_eVidStd             eVidStd
)
{
    /* Temporarily store relevant sequence fields. */
    BSPP_sDdBufArrayInfo  sAuxFWSequence = psSequHdrInfo->sFWSequence;
    IMG_HANDLE       hAuxSecureSequenceInfo = psSequHdrInfo->hSecureSequenceInfo;

    /* Reset all related structures. */
    IMG_MEMSET(GET_ELEMENT_FROM_PACKED_DEVICE_BUFFER(&psSequHdrInfo->sFWSequence), 0x00, psSequHdrInfo->sFWSequence.ui32BufElementSize);

    if(eVidStd == VDEC_STD_H264)
    {
        BSPP_H264ResetSequHdrInfo(psSequHdrInfo->hSecureSequenceInfo);
    }
    else
    {
        IMG_MEMSET(hAuxSecureSequenceInfo, 0, BSPP_ParseSequSize[eVidStd]);
    }

    IMG_MEMSET(psSequHdrInfo, 0 , sizeof(BSPP_sSequenceHdrInfo));
    /* Restore relevant sequence fields. */
    psSequHdrInfo->sFWSequence = sAuxFWSequence;
    psSequHdrInfo->sSequHdrInfo.ui32BufMapId = sAuxFWSequence.sDdBufInfo.ui32BufMapId;
    psSequHdrInfo->sSequHdrInfo.ui32BufOffset = sAuxFWSequence.ui32BufOffset;
    psSequHdrInfo->hSecureSequenceInfo = hAuxSecureSequenceInfo;
}


/*!
******************************************************************************

 @Function              bspp_ResetPPS

******************************************************************************/
IMG_VOID bspp_ResetPPS(
    VDEC_eVidStd     eVidStd,
    BSPP_sPPSInfo *  psPPSInfo
)
{
    /* Temporarily store relevant PPS fields. */
    BSPP_sDdBufArrayInfo  sAuxFWPPS = psPPSInfo->sFWPPS;
    IMG_HANDLE       hAuxSecurePPSInfo = psPPSInfo->hSecurePPSInfo;

    /* Reset all related structures. */
    IMG_MEMSET(GET_ELEMENT_FROM_PACKED_DEVICE_BUFFER(&psPPSInfo->sFWPPS), 0x00, psPPSInfo->sFWPPS.ui32BufElementSize);

    if(eVidStd == VDEC_STD_H264)
    {
        BSPP_H264ResetPPSInfo(psPPSInfo->hSecurePPSInfo);
    }
    else
    {
        //Add code for other standards
        IMG_ASSERT(0);
    }

    IMG_MEMSET(psPPSInfo, 0, sizeof(BSPP_sPPSInfo));
    /* Restore relevant PPS fields. */
    psPPSInfo->sFWPPS = sAuxFWPPS;
    psPPSInfo->ui32BufMapId = sAuxFWPPS.sDdBufInfo.ui32BufMapId;
    psPPSInfo->ui32BufOffset = sAuxFWPPS.ui32BufOffset;
    psPPSInfo->hSecurePPSInfo = hAuxSecurePPSInfo;
}


/*!
******************************************************************************

 @Function              BSPP_StreamSubmitBuffer

******************************************************************************/
IMG_RESULT BSPP_StreamSubmitBuffer(
    IMG_HANDLE                  hStrContext,
    const BSPP_sDdBufInfo     * psDdBufInfo,
    IMG_UINT32                  ui32BufMapId,
    IMG_UINT32                  ui32DataSize,
    IMG_UINT64                  ui64PictTagParam,
    VDEC_eBstrElementType       eBstrElementType
)
{
    BSPP_sStrContext      * psStrContext = (BSPP_sStrContext *)hStrContext;
    BSPP_sBitstreamBuffer * psBstrBuf;
    IMG_UINT32              ui32Result;

    if (hStrContext == IMG_NULL)
    {
        REPORT(REPORT_MODULE_BSPP, REPORT_ERR,
               "A BSPP context handle must be provided");
        ui32Result = IMG_ERROR_INVALID_PARAMETERS;
        goto error;
    }

    if (eBstrElementType == VDEC_BSTRELEMENT_UNDEFINED ||
        eBstrElementType >= VDEC_BSTRELEMENT_MAX)
    {
        REPORT(REPORT_MODULE_BSPP, REPORT_ERR,
               "Invalid buffer element type");
        ui32Result = IMG_ERROR_INVALID_PARAMETERS;
        goto error;
    }

    // Check that the new bitstream buffer is compatible with those before.
    psBstrBuf = LST_last(&psStrContext->sGrpBstrCtx.sBufferChain);
    if (psBstrBuf && psBstrBuf->eBstrElementType != eBstrElementType)
    {
        REPORT(REPORT_MODULE_BSPP, REPORT_ERR,
               "Buffers of different element types cannot be grouped");
        ui32Result = IMG_ERROR_INVALID_PARAMETERS;
        goto error;
    }

    /* Allocate a bitstream buffer chain element structure...*/
    psBstrBuf = IMG_MALLOC(sizeof(BSPP_sBitstreamBuffer));

    IMG_ASSERT(psBstrBuf != IMG_NULL);
    if (psBstrBuf == IMG_NULL)
    {
        REPORT(REPORT_MODULE_BSPP, REPORT_ERR,
            "Failed to allocate memory for bitstream chain element");

        ui32Result = IMG_ERROR_OUT_OF_MEMORY;
        goto error;
    }

    IMG_MEMSET(psBstrBuf, 0, sizeof(BSPP_sBitstreamBuffer));

    // Queue buffer in a chain since units might span buffers.


    if(psDdBufInfo != IMG_NULL)
    {
        psBstrBuf->sDdBufInfo = *psDdBufInfo;
    }
    psBstrBuf->ui32DataSize = ui32DataSize;
    psBstrBuf->eBstrElementType = eBstrElementType;
    psBstrBuf->ui64PictTagParam = ui64PictTagParam;
    psBstrBuf->ui32BufMapId = ui32BufMapId;
    LST_add(&psStrContext->sGrpBstrCtx.sBufferChain, psBstrBuf);

    psStrContext->sGrpBstrCtx.ui32TotalDataSize += ui32DataSize;

    return IMG_SUCCESS;

error:
    return ui32Result;
}


/*!
******************************************************************************

 @Function              bspp_ObtainSequenceHdr

 @Description

******************************************************************************/
static BSPP_sSequenceHdrInfo *
bspp_ObtainSequenceHdr(
    BSPP_sStreamAllocData * psStrAlloc,
    VDEC_eVidStd            eVidStd
)
{
    BSPP_sSequenceHdrInfo * psSequHdrInfo;

    // Obtain any partially filled sequence data
    // else provide a new one (always new for H.264 and HEVC)
    psSequHdrInfo = LST_last(&psStrAlloc->asSequenceDataList[BSPP_DEFAULT_SEQUENCE_ID]);
    if (psSequHdrInfo == IMG_NULL
        || psSequHdrInfo->ui32RefCount > 0
        || eVidStd == VDEC_STD_H264)
    {
        // Get Sequence resource.
        psSequHdrInfo = LST_removeHead(&psStrAlloc->sAvailableSequencesList);
        if (psSequHdrInfo == IMG_NULL)
        {
            // allocate and return extra resources
            IMG_ASSERT("Ran out of sequence resources" == IMG_NULL);
        }
        else
        {
            bspp_ResetSequence(psSequHdrInfo, eVidStd);
            psSequHdrInfo->sSequHdrInfo.ui32SequHdrId = BSPP_INVALID;
        }
    }
    else
    {
        IMG_ASSERT(psSequHdrInfo->ui32RefCount == 0);
    }

    return psSequHdrInfo;
}


/*!
******************************************************************************

 @Function              BSPP_SubmitPictureDecoded

 @Description

******************************************************************************/
IMG_RESULT
BSPP_SubmitPictureDecoded(
    IMG_HANDLE              hStrContext,
    IMG_UINT32              ui32SequHdrId,
    IMG_UINT32              ui32PPSId,
    IMG_UINT32              ui32SecondPPSId
)
{
    BSPP_sPictureDecoded * psPictureDecoded;
    BSPP_sStrContext     * psStrContext = (BSPP_sStrContext *)hStrContext;

    // Validate input arguments.
    if (hStrContext == IMG_NULL)
    {
        REPORT(REPORT_MODULE_BSPP, REPORT_ERR,
               "A BSPP context handle must be provided");
        return IMG_ERROR_INVALID_PARAMETERS;

    }
    psPictureDecoded = IMG_MALLOC(sizeof(BSPP_sPictureDecoded));
    IMG_ASSERT(psPictureDecoded);
    if(psPictureDecoded == IMG_NULL)
    {
        return IMG_ERROR_MALLOC_FAILED;
    }

    psPictureDecoded->ui32SequHdrId     = ui32SequHdrId;
    psPictureDecoded->ui32PPSId         = ui32PPSId;
    psPictureDecoded->ui32SecondPPSId   = ui32SecondPPSId;

    // Lock access to the list for adding a picture - HIGH PRIORITY
    LOCK_HP(psStrContext->sMutex);

    LST_add(&psStrContext->sDecodedPicturesList, psPictureDecoded);

    // Unlock access to the list for adding a picture - HIGH PRIORITY
    UNLOCK_HP(psStrContext->sMutex);

    return IMG_SUCCESS;
}


/*!
******************************************************************************

 @Function              bspp_CheckAndDetachPPSInfo

 @Description

******************************************************************************/
static IMG_VOID
bspp_CheckAndDetachPPSInfo(
        BSPP_sStreamAllocData * psStrAlloc,
        IMG_UINT32 ui32PPSId)
{
    if (ui32PPSId != BSPP_INVALID)
    {
        BSPP_sPPSInfo * psPPSInfo =
            LST_first(&psStrAlloc->asPPSDataList[ui32PPSId]);

        IMG_ASSERT(psPPSInfo);
        IMG_ASSERT(psPPSInfo->ui32RefCount > 0);

        psPPSInfo->ui32RefCount--;
        if (psPPSInfo->ui32RefCount == 0) // If nothing references it any more
        {
            BSPP_sPPSInfo * psNextPPSInfo = LST_next(psPPSInfo);

            // If it is not the last sequence in the slot list
            // remove it and return it to the pool-list
            if (psNextPPSInfo)
            {
                LST_remove(&psStrAlloc->asPPSDataList[ui32PPSId], psPPSInfo);
                LST_addHead(&psStrAlloc->sAvailablePPSsList, psPPSInfo);
            }
        }
    }
}


/*!
******************************************************************************

 @Function              bspp_PictureDecoded

 @Description

******************************************************************************/
static IMG_RESULT
bspp_PictureDecoded(
    BSPP_sStrContext      * psStrContext,
    IMG_UINT32              ui32SequHdrId,
    IMG_UINT32              ui32PPSId,
    IMG_UINT32              ui32SecondPPSId
)
{
    BSPP_sStreamAllocData * psStrAlloc = &psStrContext->sStrAlloc;

    // Manage Sequence
    if (ui32SequHdrId != BSPP_INVALID)
    {
        BSPP_sSequenceHdrInfo * psSequHdrInfo =
            LST_first(&psStrAlloc->asSequenceDataList[ui32SequHdrId]);

        IMG_ASSERT(psSequHdrInfo);
        IMG_ASSERT(psSequHdrInfo->ui32RefCount > 0);

        psSequHdrInfo->ui32RefCount--;
        if (psSequHdrInfo->ui32RefCount == 0) // If nothing references it any more
        {
            BSPP_sSequenceHdrInfo * psNextSequHdrInfo = LST_next(psSequHdrInfo);

             // If it is not the last sequence in the slot list
             // remove it and return it to the pool-list
            if (psNextSequHdrInfo)
            {
                LST_remove(&psStrAlloc->asSequenceDataList[ui32SequHdrId], psSequHdrInfo);
                LST_addHead(&psStrAlloc->sAvailableSequencesList, psSequHdrInfo);
            }
        }
    }

    // Expect at least one valid PPS for H.264, exactly one for HEVC
    // and always invalid for all others
    IMG_ASSERT((psStrContext->eVidStd == VDEC_STD_H264 && 
                        (ui32PPSId != BSPP_INVALID || ui32SecondPPSId != BSPP_INVALID)) ||
               (psStrContext->eVidStd != VDEC_STD_H264 && ui32PPSId == BSPP_INVALID));

    bspp_CheckAndDetachPPSInfo(psStrAlloc, ui32PPSId);
    bspp_CheckAndDetachPPSInfo(psStrAlloc, ui32SecondPPSId);

    return IMG_SUCCESS;
}


/*!
******************************************************************************

 @Function              bspp_ServicePicturesDecoded

 @Description

******************************************************************************/
static IMG_RESULT
bspp_ServicePicturesDecoded(
    BSPP_sStrContext      * psStrContext
)
{
    BSPP_sPictureDecoded * psPictureDecoded;

    while(IMG_TRUE)
    {
        // Lock access to the list for removing a picture - LOW PRIORITY
        LOCK_LP(psStrContext->sMutex);

        psPictureDecoded = LST_removeHead(&psStrContext->sDecodedPicturesList);

        // Unlock access to the list for removing a picture - LOW PRIORITY
        UNLOCK_LP(psStrContext->sMutex);

        if (psPictureDecoded == IMG_NULL)
        {
            // We are done
            break;
        }

        IMG_ASSERT(psPictureDecoded);
        bspp_PictureDecoded(
            psStrContext,
            psPictureDecoded->ui32SequHdrId,
            psPictureDecoded->ui32PPSId,
            psPictureDecoded->ui32SecondPPSId
            );
        IMG_FREE(psPictureDecoded);
    }

    return IMG_SUCCESS;
}


static IMG_VOID
bspp_RemoveUnusedPps(
    BSPP_sStreamAllocData * psStrAlloc,
    IMG_UINT32              ui32PpsId
)
{
    BSPP_sPPSInfo * psTempPPSInfo = IMG_NULL;
    BSPP_sPPSInfo * psNextTempPPSInfo = IMG_NULL;

    // Check the whole PPS slot list for any unused PPSs BEFORE ADDING THE NEW ONE, if found remove them
    psNextTempPPSInfo = LST_first(&psStrAlloc->asPPSDataList[ui32PpsId]);
    while(psNextTempPPSInfo)
    {
        // Set Temp, it is the one which we will potentially remove
        psTempPPSInfo = psNextTempPPSInfo;
        // Set Next Temp, it is the one for the next iteration (we cannot ask for next after removing it)
        psNextTempPPSInfo = LST_next(psTempPPSInfo);
        // If it is not used remove it
        if ((psTempPPSInfo->ui32RefCount == 0) && psNextTempPPSInfo)
        {
            // Return resource to the available pool
            LST_remove(&psStrAlloc->asPPSDataList[ui32PpsId], psTempPPSInfo);
            LST_addHead(&psStrAlloc->sAvailablePPSsList, psTempPPSInfo);
        }
    }
}


static IMG_VOID
bspp_RemoveUnusedSequence(
    BSPP_sStreamAllocData * psStrAlloc,
    IMG_UINT32              ui32SpsId
)
{
    BSPP_sSequenceHdrInfo * psTempSequHdrInfo = IMG_NULL;
    BSPP_sSequenceHdrInfo * psNextTempSequHdrInfo = IMG_NULL;


    // Check the whole sequence slot list for any unused sequences, if found remove them
    psNextTempSequHdrInfo = LST_first(&psStrAlloc->asSequenceDataList[ui32SpsId]);
    while (psNextTempSequHdrInfo)
    {
        // Set Temp, it is the one which we will potentially remove
        psTempSequHdrInfo = psNextTempSequHdrInfo;
        // Set Next Temp, it is the one for the next iteration (we cannot ask for next after removing it)
        psNextTempSequHdrInfo = LST_next(psTempSequHdrInfo);

        // If the head is no longer used and there is something after, remove it
        if ((psTempSequHdrInfo->ui32RefCount == 0) && psNextTempSequHdrInfo)
        {
            // Return resource to the pool-list
            LST_remove(&psStrAlloc->asSequenceDataList[ui32SpsId], psTempSequHdrInfo);
            LST_addHead(&psStrAlloc->sAvailableSequencesList, psTempSequHdrInfo);
        }
    }
}



/*!
******************************************************************************

 @Function              bspp_ReturnOrStoreSequenceHdr

 @Description

******************************************************************************/
static IMG_RESULT
bspp_ReturnOrStoreSequenceHdr(
    BSPP_sStreamAllocData * psStrAlloc,
    BSPP_eErrorType         eParseError,
    BSPP_sSequenceHdrInfo * psSequHdrInfo
)
{
    if (eParseError & BSPP_ERROR_UNRECOVERABLE)
    {
        IMG_ASSERT(IMG_FALSE);
        if (LST_last(&psStrAlloc->asSequenceDataList[psSequHdrInfo->sSequHdrInfo.ui32SequHdrId]))
        {
            // Throw away corrupted sequence header if a previous good one exists.
            psSequHdrInfo->sSequHdrInfo.ui32SequHdrId = BSPP_INVALID;
            //sUnitData.eParseError |= BSPP_ERROR_CORRECTION_VSH;
        }
    }

    // Store or return Sequence resource.
    if (psSequHdrInfo->sSequHdrInfo.ui32SequHdrId != BSPP_INVALID)
    {
        // Only add when not already in list.
        //if (psSequHdrInfo->ui32RefCount == 0)
        {
            if (psSequHdrInfo != LST_last(&psStrAlloc->asSequenceDataList[psSequHdrInfo->sSequHdrInfo.ui32SequHdrId]))
            {
                // Add new sequence header (not already in list) to end of the slot-list.
                LST_add(&psStrAlloc->asSequenceDataList[psSequHdrInfo->sSequHdrInfo.ui32SequHdrId],
                        psSequHdrInfo);
            }

            bspp_RemoveUnusedSequence(psStrAlloc, psSequHdrInfo->sSequHdrInfo.ui32SequHdrId);
        }
    }
    else
    {
        // if unit was not a sequence info, add resource to the pool-list
        LST_addHead(&psStrAlloc->sAvailableSequencesList, psSequHdrInfo);
    }

    return IMG_SUCCESS;
}


/*!
******************************************************************************

 @Function              bspp_GetResource

 @Description

******************************************************************************/
static IMG_RESULT
bspp_GetResource(
    BSPP_sStreamAllocData * psStrAlloc,
    BSPP_sPictHdrInfo     * psPictHdrInfo,
    BSPP_sUnitData        * psUnitData
)
{
    IMG_RESULT ui32Result = IMG_SUCCESS;

    switch (psUnitData->eUnitType)
    {
    case BSPP_UNIT_SEQUENCE:
        psUnitData->out.psSequHdrInfo = bspp_ObtainSequenceHdr(psStrAlloc, psUnitData->eVidStd);
        if (!psUnitData->out.psSequHdrInfo)
        {
            ui32Result = IMG_ERROR_COULD_NOT_OBTAIN_RESOURCE;
        }
        break;

    case BSPP_UNIT_PPS_H264:
        // Get PPS resource (HEVC/H.264 only).
        IMG_ASSERT(psUnitData->eVidStd == VDEC_STD_H264);

        psUnitData->out.psPPSInfo = LST_removeHead(&psStrAlloc->sAvailablePPSsList);
        if (psUnitData->out.psPPSInfo == IMG_NULL)
        {
            // allocate and return extra resources
            IMG_ASSERT("Ran out of PPS resources" == IMG_NULL);
            ui32Result = IMG_ERROR_COULD_NOT_OBTAIN_RESOURCE;
        }
        else
        {
            bspp_ResetPPS(psUnitData->eVidStd, psUnitData->out.psPPSInfo);
            psUnitData->out.psPPSInfo->ui32PPSId = BSPP_INVALID;
        }
        break;

    case BSPP_UNIT_PICTURE:
    case BSPP_UNIT_SKIP_PICTURE:
        psUnitData->out.psPictHdrInfo = psPictHdrInfo;
        break;

    default:
        break;
    }

    return ui32Result;
}


/*!
******************************************************************************

 @Function              bspp_FileResource

 @Description

 Stores or returns all resources provided to parse unit.

******************************************************************************/
static IMG_RESULT
bspp_FileResource(
    BSPP_sStreamAllocData * psStrAlloc,
    BSPP_sUnitData        * psUnitData
)
{
    IMG_UINT32 ui32Result;

    switch (psUnitData->eUnitType)
    {
    case BSPP_UNIT_SEQUENCE:
        ui32Result = bspp_ReturnOrStoreSequenceHdr(psStrAlloc,
                                                   psUnitData->eParseError,
                                                   psUnitData->out.psSequHdrInfo);
        IMG_ASSERT(ui32Result == IMG_SUCCESS);
        break;

    case BSPP_UNIT_PPS_H264:
        // Store or return PPS resource (H.264 and HEVC only).
        IMG_ASSERT(psUnitData->eVidStd == VDEC_STD_H264);

        if (psUnitData->out.psPPSInfo->ui32PPSId != BSPP_INVALID)
        {
            // if unit was a PPS info, add resource to the slot-list AFTER REMOVING THE UNUSED ONES
            // otherwise this will be removed along the rest unless special provision for last is made
            LST_add(&psStrAlloc->asPPSDataList[psUnitData->out.psPPSInfo->ui32PPSId], psUnitData->out.psPPSInfo);

            bspp_RemoveUnusedPps(psStrAlloc, psUnitData->out.psPPSInfo->ui32PPSId);
        }
        else
        {
            // if unit was not a PPS info, add resource to the pool-list
            LST_addHead(&psStrAlloc->sAvailablePPSsList, psUnitData->out.psPPSInfo);
        }
        break;

    case BSPP_UNIT_PICTURE:
    case BSPP_UNIT_SKIP_PICTURE:
        break;

    default:
        break;
    }

    return IMG_SUCCESS;
}


/*!
******************************************************************************

 @Function              bspp_PrintReason

 @Description

******************************************************************************/
static IMG_RESULT
bspp_PrintReason(
    const IMG_CHAR   * pszErrorPrefix,
    BSPP_eErrorType    eParseError
)
{
    IMG_CHAR  * pszErrorReason;
    IMG_UINT32  ui32Result;

    pszErrorReason = IMG_MALLOC(BSPP_ERR_MSG_LENGTH);
    IMG_ASSERT(pszErrorReason);
    if (pszErrorReason == IMG_NULL)
    {
        ui32Result = IMG_ERROR_MALLOC_FAILED;
        goto error;
    }

    *pszErrorReason = '\0';

    if (eParseError & BSPP_ERROR_CORRECTION_VSH)
    {
        strncat(pszErrorReason, " BSPP_ERROR_CORRECTION_VSH", BSPP_ERR_MSG_LENGTH - strlen(pszErrorReason) - 1);
    }
    if (eParseError & BSPP_ERROR_CORRECTION_VALIDVALUE)
    {
        strncat(pszErrorReason, " BSPP_ERROR_CORRECTION_VALIDVALUE", BSPP_ERR_MSG_LENGTH - strlen(pszErrorReason) - 1);
    }
    if (eParseError & BSPP_ERROR_AUXDATA)
    {
        strncat(pszErrorReason, " BSPP_ERROR_AUXDATA", BSPP_ERR_MSG_LENGTH - strlen(pszErrorReason) - 1);
    }
    if (eParseError & BSPP_ERROR_DATA_REMAINS)
    {
        strncat(pszErrorReason, " BSPP_ERROR_DATA_REMAINS", BSPP_ERR_MSG_LENGTH - strlen(pszErrorReason) - 1);
    }
    if (eParseError & BSPP_ERROR_INVALID_VALUE)
    {
        strncat(pszErrorReason, " BSPP_ERROR_INVALID_VALUE", BSPP_ERR_MSG_LENGTH - strlen(pszErrorReason) - 1);
    }
    if (eParseError & BSPP_ERROR_DECODE)
    {
        strncat(pszErrorReason, " BSPP_ERROR_DECODE", BSPP_ERR_MSG_LENGTH - strlen(pszErrorReason) - 1);
    }
    if (eParseError & BSPP_ERROR_NO_REF_FRAME)
    {
        strncat(pszErrorReason, " BSPP_ERROR_NO_REF_FRAME", BSPP_ERR_MSG_LENGTH - strlen(pszErrorReason) - 1);
    }
    if (eParseError & BSPP_ERROR_NONIDR_FRAME_LOSS)
    {
        strncat(pszErrorReason, " BSPP_ERROR_NONIDR_FRAME_LOSS", BSPP_ERR_MSG_LENGTH - strlen(pszErrorReason) - 1);
    }
    if (eParseError & BSPP_ERROR_IDR_FRAME_LOSS)
    {
        strncat(pszErrorReason, " BSPP_ERROR_IDR_FRAME_LOSS", BSPP_ERR_MSG_LENGTH - strlen(pszErrorReason) - 1);
    }
    if (eParseError & BSPP_ERROR_INSUFFICIENT_DATA)
    {
        strncat(pszErrorReason, " BSPP_ERROR_INSUFFICIENT_DATA", BSPP_ERR_MSG_LENGTH - strlen(pszErrorReason) - 1);
    }
    if (eParseError & BSPP_ERROR_UNSUPPORTED)
    {
        strncat(pszErrorReason, " BSPP_ERROR_UNSUPPORTED", BSPP_ERR_MSG_LENGTH - strlen(pszErrorReason) - 1);
    }
    if (eParseError & BSPP_ERROR_UNRECOVERABLE)
    {
        strncat(pszErrorReason, " BSPP_ERROR_UNRECOVERABLE", BSPP_ERR_MSG_LENGTH - strlen(pszErrorReason) - 1);
    }
    if (eParseError & BSPP_ERROR_NO_NALHEADER)
    {
        strncat(pszErrorReason, " BSPP_ERROR_NO_NALHEADER", BSPP_ERR_MSG_LENGTH - strlen(pszErrorReason) - 1);
    }
    if (eParseError & BSPP_ERROR_NO_SEQUENCE_HDR)
    {
        strncat(pszErrorReason, " BSPP_ERROR_NO_SEQUENCE_HDR", BSPP_ERR_MSG_LENGTH - strlen(pszErrorReason) - 1);
    }
    if (eParseError & BSPP_ERROR_SIGNALED_IN_STREAM)
    {
        strncat(pszErrorReason, " BSPP_ERROR_SIGNALED_IN_STREAM", BSPP_ERR_MSG_LENGTH - strlen(pszErrorReason) - 1);
    }
    if (eParseError & BSPP_ERROR_UNKNOWN_DATAUNIT_DETECTED)
    {
        strncat(pszErrorReason, " BSPP_ERROR_UNKNOWN_DATAUNIT_DETECTED", BSPP_ERR_MSG_LENGTH - strlen(pszErrorReason) - 1);
    }
    if (eParseError & BSPP_ERROR_NO_PPS)
    {
        strncat(pszErrorReason, " BSPP_ERROR_NO_PPS", BSPP_ERR_MSG_LENGTH - strlen(pszErrorReason) - 1);
    }

    REPORT(REPORT_MODULE_BSPP, REPORT_WARNING,
        "%s: %s", pszErrorPrefix, pszErrorReason);

    IMG_FREE(pszErrorReason);

    return IMG_SUCCESS;

error:
    return ui32Result;
}


/*!
******************************************************************************

 @Function              bspp_ProcessUnit

 @Description

******************************************************************************/
static IMG_RESULT
bspp_ProcessUnit(
    BSPP_sStrContext      * psStrContext,
    IMG_UINT32              ui32SizeDelimBits,
    BSPP_sPictCtx         * psPictCtx,
    BSPP_sParseState      * psParseState
)
{
    BSPP_sUnitData      sUnitData;
    IMG_UINT64          ui64UnitSize = 0;               /*!< Unit size (in bytes, size delimited only).                 */
    IMG_UINT32          ui32Result;
    IMG_UINT8           ui8VIdx = psStrContext->sGrpBstrCtx.ui8CurrentViewIdx;
    BSPP_sPictHdrInfo * psCurrPictHdrInfo;

    // during call to SWSR_ConsumeDelim(), above.
    // Setup default unit data.
	IMG_MEMSET(&sUnitData, 0, sizeof(BSPP_sUnitData));

    if (psStrContext->sGrpBstrCtx.bDelimPresent)
    {
        // Consume delimiter and catch any exceptions.
        BSPP_TRY(psStrContext->sParseCtx.jump_buffer)
        {
            // Consume the bitstream unit delimiter (size or start code prefix).
            // When size-delimited the unit size is also returned so that the next unit can be found.
            ui32Result = SWSR_ConsumeDelim(psStrContext->hSwSrContext,
                                           psStrContext->eEmulationPrevention,
                                           ui32SizeDelimBits,
                                           &ui64UnitSize);
            IMG_ASSERT(ui32Result == IMG_SUCCESS);
            if (ui32Result != IMG_SUCCESS)
            {
                goto error;
            }
        }
        BSPP_CATCH(BSPP_EXCEPTION_HANDLER_JUMP)
        {
            REPORT(REPORT_MODULE_BSPP, REPORT_WARNING, "Exception in consuming delimiter");
            ui32Result = IMG_ERROR_UNEXPECTED_STATE;
            goto error;
        }
        BSPP_ETRY; /* end of the try catch block */
    }

    sUnitData.eUnitType = psStrContext->sGrpBstrCtx.eUnitType;
    sUnitData.eVidStd = psStrContext->eVidStd;
    sUnitData.bDelimPresent = psStrContext->sGrpBstrCtx.bDelimPresent;
    sUnitData.psCodecConfig = &psStrContext->sCodecConfig;
    sUnitData.psParseState = psParseState;
    sUnitData.ui32PictSequHdrId = psStrContext->ui32SequHdrId;
    sUnitData.hStrRes = &psStrContext->sStrAlloc;
    sUnitData.ui32UnitDataSize = psStrContext->sGrpBstrCtx.ui32TotalDataSize;
    sUnitData.bIntraFrmAsClosedGop = psStrContext->bIntraFrmAsClosedGop;

    //ponit to picture headers, check boundaries
    psCurrPictHdrInfo = ui8VIdx < VDEC_H264_MVC_MAX_VIEWS ? &psPictCtx->sPictHdrInfo[ui8VIdx] : IMG_NULL;
    sUnitData.psParseState->psNextPictHdrInfo = ui8VIdx+1 < VDEC_H264_MVC_MAX_VIEWS ? &psPictCtx->sPictHdrInfo[ui8VIdx+1] : IMG_NULL;
    sUnitData.psParseState->bIsPrefix = IMG_FALSE;

    // Obtain output data containers.
    ui32Result = bspp_GetResource(&psStrContext->sStrAlloc,
                                  psCurrPictHdrInfo,
                                  &sUnitData);
    if (ui32Result == IMG_ERROR_COULD_NOT_OBTAIN_RESOURCE)
    {
        return SWSR_FOUND_EOD;  // We cannot continue processing, just try to exit cleanly
    }

    // Process Unit and catch any exceptions.
    BSPP_TRY(psStrContext->sParseCtx.jump_buffer)
    {
        // Call the standard-specific function to parse the bitstream unit.
        ui32Result = psStrContext->pfnParseUnit(psStrContext->hSwSrContext,
                                                &sUnitData);
        if (ui32Result != IMG_SUCCESS)
        {
            bspp_PrintReason("Failed to process unit",
                             sUnitData.eParseError);
            goto error;
        }
    }
    BSPP_CATCH(BSPP_EXCEPTION_HANDLER_JUMP)
    {
        REPORT(REPORT_MODULE_BSPP, REPORT_WARNING,
               "Exception in unit (type: %d) parsing",
               psStrContext->sGrpBstrCtx.eUnitType);
        ui32Result = IMG_ERROR_UNEXPECTED_STATE;
        goto error;
    }
    BSPP_ETRY; /* end of the try catch block */

    if (sUnitData.eParseError != BSPP_ERROR_NONE)
    {
        bspp_PrintReason("Issues found while processing unit",
                         sUnitData.eParseError);
    }

    // Ensure that parse unit didn't modify the input variables.
    IMG_ASSERT(sUnitData.eUnitType == psStrContext->sGrpBstrCtx.eUnitType);
    IMG_ASSERT(sUnitData.eVidStd == psStrContext->eVidStd);
    IMG_ASSERT(sUnitData.bDelimPresent == psStrContext->sGrpBstrCtx.bDelimPresent);
    IMG_ASSERT(sUnitData.hStrRes == &psStrContext->sStrAlloc);

    // Store or return resource used for parsing unit.
    ui32Result = bspp_FileResource(&psStrContext->sStrAlloc, &sUnitData);
    IMG_ASSERT(ui32Result == IMG_SUCCESS);

    // FIXME: DCP
    if (!psStrContext->sInterPictData.bSeenClosedGOP &&
        psStrContext->sGrpBstrCtx.eUnitType == BSPP_UNIT_PICTURE &&
        sUnitData.bSlice &&
        (((IMG_NULL != sUnitData.out.psPictHdrInfo) && sUnitData.out.psPictHdrInfo->bIntraCoded)/* || psStrContext->eVidStd == VDEC_STD_REAL*/) &&
        psStrContext->eVidStd != VDEC_STD_H264)
    {
        sUnitData.bNewClosedGOP = IMG_TRUE;
    }

    if (sUnitData.bNewClosedGOP)
    {
        psStrContext->sInterPictData.bSeenClosedGOP = IMG_TRUE;
        psStrContext->sInterPictData.bNewClosedGOP = IMG_TRUE;
    }

    // Post-process unit (use local context in case
    // parse function tried to change the unit type.
    if (psStrContext->sGrpBstrCtx.eUnitType == BSPP_UNIT_PICTURE ||
        psStrContext->sGrpBstrCtx.eUnitType == BSPP_UNIT_SKIP_PICTURE)
    {
        if (psStrContext->sInterPictData.bNewClosedGOP)
        {
            psPictCtx->bClosedGOP = IMG_TRUE;
            psStrContext->sInterPictData.bNewClosedGOP = IMG_FALSE;
        }

        if (sUnitData.bExtSlice &&
            psStrContext->sGrpBstrCtx.bNotExtPicUnitYet &&
            sUnitData.ui32PictSequHdrId != BSPP_INVALID)
        {
            psStrContext->sGrpBstrCtx.bNotExtPicUnitYet = IMG_FALSE;

            psPictCtx->psExtSequHdrInfo = LST_last(&psStrContext->sStrAlloc.asSequenceDataList[sUnitData.ui32PictSequHdrId]);
            IMG_ASSERT(psPictCtx->psExtSequHdrInfo);
        }

        if (sUnitData.bSlice)
        {
            if (psStrContext->sGrpBstrCtx.bNotPicUnitYet &&
                sUnitData.ui32PictSequHdrId != BSPP_INVALID)
            {
                psStrContext->sGrpBstrCtx.bNotPicUnitYet = IMG_FALSE;

                // depend upon the picture header being populated (in addition
                // to slice data).
                psPictCtx->bPresent = IMG_TRUE;

                // Update the picture context from the last unit parsed.
                // This context must be stored since a non-picture unit may follow.
                // Obtain current instance of sequence data for given ID.
                psPictCtx->psSequHdrInfo = LST_last(&psStrContext->sStrAlloc.asSequenceDataList[sUnitData.ui32PictSequHdrId]);
                IMG_ASSERT(psPictCtx->psSequHdrInfo);

                // Do the sequence flagging/reference-counting
                psPictCtx->psSequHdrInfo->ui32RefCount++;

                // Warn when a picture precedes the first closed GOP.
                if (!psStrContext->sInterPictData.bSeenClosedGOP)
                {
                    REPORT(REPORT_MODULE_BSPP, REPORT_WARNING,
                           "Closed GOP not seen before this picture");
                    //psPictCtx->bInvalid = IMG_TRUE;
                }

                // Override the field here.
                if (psStrContext->sSrConfig.eDelimType == SWSR_DELIM_NONE)
                {
                    if (psStrContext->sGrpBstrCtx.eUnitType == BSPP_UNIT_SKIP_PICTURE)
                    {
                        psCurrPictHdrInfo->eParserMode = VDECFW_SKIPPED_PICTURE;
                        psCurrPictHdrInfo->ui32PicDataSize = 0;
                    }
                    else
                    {
                        psCurrPictHdrInfo->eParserMode = VDECFW_SIZE_SIDEBAND;
                        psCurrPictHdrInfo->ui32PicDataSize = psStrContext->sGrpBstrCtx.ui32TotalDataSize;
                    }
                }
                else if (psStrContext->sSrConfig.eDelimType == SWSR_DELIM_SIZE)
                {
                    if (psStrContext->sSrConfig.ui32DelimLength <= 8)
                    {
                        psCurrPictHdrInfo->eParserMode = VDECFW_SIZE_DELIMITED_1_ONLY;
                    }
                    else if (psStrContext->sSrConfig.ui32DelimLength <= 16)
                    {
                        psCurrPictHdrInfo->eParserMode = VDECFW_SIZE_DELIMITED_2_ONLY;
                    }
                    else if (psStrContext->sSrConfig.ui32DelimLength <= 32)
                    {
                        psCurrPictHdrInfo->eParserMode = VDECFW_SIZE_DELIMITED_4_ONLY;
                    }
                    else
                    {
                        IMG_ASSERT(IMG_FALSE);
                    }

                    psCurrPictHdrInfo->ui32PicDataSize += ((IMG_UINT32)ui64UnitSize + (ui32SizeDelimBits / 8));
                }
                else if (psStrContext->sSrConfig.eDelimType == SWSR_DELIM_SCP)
                {
                    psCurrPictHdrInfo->eParserMode = VDECFW_SCP_ONLY;
                }
                else
                {
                    IMG_ASSERT(IMG_FALSE);
                }
            }

            // for MVC, the Slice Extension should also have the same ParserMode as the Base view.
            if(sUnitData.psParseState->psNextPictHdrInfo)
            {
                sUnitData.psParseState->psNextPictHdrInfo->eParserMode = psCurrPictHdrInfo->eParserMode;
            }

            if (sUnitData.eParseError & BSPP_ERROR_UNSUPPORTED)
            {
                psPictCtx->bInvalid = IMG_TRUE;
                psPictCtx->bUnsupported = IMG_TRUE;
            }
            else if (!psStrContext->bFullScan)
            {
                // Only parse up to and including the first valid video slice unless full scanning.
                psPictCtx->bFinished = IMG_TRUE;
            }
        }
    }

    if (sUnitData.bExtractedAllData)
    {
        SWSR_eFound eFound;

        SWSR_ByteAlign(psStrContext->hSwSrContext);

        eFound = SWSR_CheckDelimOrEOD(psStrContext->hSwSrContext);
        if (eFound != SWSR_FOUND_DELIM &&
            eFound != SWSR_FOUND_EOD)
        {
            // Should already be at the next delimiter or EOD.
            // Any bits left at the end of the unit could indicate
            // corrupted syntax or erroneous parsing.
            REPORT(REPORT_MODULE_BSPP, REPORT_WARNING,
                   "Unit parsing should have extracted all data up until the next unit or EOD");
        }
    }

    return IMG_SUCCESS;

error:
    if (sUnitData.eUnitType == BSPP_UNIT_PICTURE ||
        sUnitData.eUnitType == BSPP_UNIT_SKIP_PICTURE)
    {
        psPictCtx->bInvalid = IMG_TRUE;
    }

    {
        IMG_UINT32 ui32NewResult;

        // Tidy-up resources.
        // Store or return resource used for parsing unit.
        ui32NewResult = bspp_FileResource(&psStrContext->sStrAlloc, &sUnitData);
        IMG_ASSERT(ui32NewResult == IMG_SUCCESS);
    }

    return ui32Result;
}


/*!
******************************************************************************

 @Function              bspp_TerminateBuffer

 @Description

******************************************************************************/
static IMG_RESULT
bspp_TerminateBuffer(
    BSPP_sGrpBstrCtx      * psGrpBstrCtx,
    BSPP_sBitstreamBuffer * psBuf
)
{
    IMG_UINT32          ui32Result;

    // Indicate that all the data in buffer should be added to segment.
    psBuf->ui64BytesRead = psBuf->ui32DataSize;

    ui32Result = bspp_CreateSegment(psGrpBstrCtx, psBuf);
    IMG_ASSERT(ui32Result == IMG_SUCCESS);
    if (ui32Result != IMG_SUCCESS)
    {
        goto error;
    }
    // Next segment will start at the beginning of the next buffer.
    psGrpBstrCtx->ui32SegmentOffset = 0;

    ui32Result = bspp_FreeBitstreamElem(psBuf);
    IMG_ASSERT(ui32Result == IMG_SUCCESS);
    if (ui32Result != IMG_SUCCESS)
    {
        goto error;
    }

    // Ensure that the next data starts at the beginning of new buffer.
    IMG_ASSERT(psGrpBstrCtx->ui32SegmentOffset == 0);

    return IMG_SUCCESS;

error:
    return ui32Result;
}


/*!
******************************************************************************

 @Function              bspp_JumpToNextView

 @Description

******************************************************************************/
static IMG_RESULT
bspp_JumpToNextView(
    BSPP_sGrpBstrCtx      * psGrpBstrCtx,
    BSPP_sPreParsedData   * psPreParsedData,
    BSPP_sParseState      * psParseState,
	LST_T				    pasPrePictSegList[][BSPP_MAX_PICTURES_PER_BUFFER],
	LST_T				    pasPictSegList[][BSPP_MAX_PICTURES_PER_BUFFER]
)
{
    BSPP_sBitstreamBuffer * psCurBuf;
    IMG_RESULT ui32Result;
    IMG_UINT32 i;

    IMG_ASSERT(psGrpBstrCtx != IMG_NULL && psParseState != IMG_NULL && psPreParsedData != IMG_NULL);
    if(psGrpBstrCtx == IMG_NULL || psParseState == IMG_NULL || psPreParsedData == IMG_NULL)
    {
        REPORT(REPORT_MODULE_BSPP, REPORT_ERR,
                "Invalid parameters while jumping to next view");
        ui32Result = IMG_ERROR_INVALID_PARAMETERS;
        goto error;
    }

    if(psGrpBstrCtx->ui8CurrentViewIdx >= VDEC_H264_MVC_MAX_VIEWS)
    {
        REPORT(REPORT_MODULE_BSPP, REPORT_ERR,
                "Number of views is greather than maximum supported (%d)", VDEC_H264_MVC_MAX_VIEWS);
        ui32Result = IMG_ERROR_NOT_SUPPORTED;
        goto error;
    }

    //get current buffer
    psCurBuf = (BSPP_sBitstreamBuffer *)LST_first(&psGrpBstrCtx->sInFlightBufs);
    if (IMG_NULL == psCurBuf)
    {
        ui32Result = IMG_ERROR_CANCELLED;
        goto error;
    }
    if(psCurBuf->ui32BufMapId != psParseState->ui32PrevBufMapId)
    {
        // If we moved to the next buffer while parsing the slice header of the new view
        // we have to reduce the size of the last segment up to the begining of the new view slice
        // and create a new segment from that point up to the end of the buffer. The new segment
        // should belong to the new view.
        // THIS ONLY WORKS IF THE SLICE HEADER DOES NOT SPAN MORE THAN TWO BUFFERS.
        // If we want to support the case that the slice header of the new view spans multiple buffer
        // we either have here remove all the segments up to the point were we find the buffer we are looking
        // for, then adjust the size of this segment and then add the segments we removed to the next view
        // list or we can implement a mechanism like the one that peeks for the NAL unit type and delemit
        // the next view segment before parsing the first slice of the view.
        BSPP_sBitStrSeg * psSegment;

        psSegment = LST_last(psGrpBstrCtx->psSegmentList);
        if(psSegment && psSegment->ui32BufMapId == psParseState->ui32PrevBufMapId)
        {
            BSPP_sBitstreamBuffer sPrevBuf;

            psSegment->ui32DataSize -= psParseState->ui32PrevBufDataSize - psParseState->ui64PrevByteOffsetBuf;
            psSegment->ui32BitStrSegFlag &= ~VDECDD_BSSEG_LASTINBUFF;

            // Change the segmenOffset value with the value it would have if we had delemited the segment correctly beforehand.
            psGrpBstrCtx->ui32SegmentOffset = psParseState->ui64PrevByteOffsetBuf;

            //set lists of segments to new view...
            for (i = 0; i < BSPP_MAX_PICTURES_PER_BUFFER; i++)
            {
                psGrpBstrCtx->apsPrePictSegList[i] = &pasPrePictSegList[psGrpBstrCtx->ui8CurrentViewIdx][i];
                psGrpBstrCtx->apsPictSegList[i] = &pasPictSegList[psGrpBstrCtx->ui8CurrentViewIdx][i];

                LST_init(psGrpBstrCtx->apsPrePictSegList[i]);
                LST_init(psGrpBstrCtx->apsPictSegList[i]);
            }
            //and current segment list
            psGrpBstrCtx->psSegmentList = psGrpBstrCtx->apsPictSegList[0];

            IMG_MEMSET(&sPrevBuf, 0, sizeof(BSPP_sBitstreamBuffer));
            sPrevBuf.ui32BufMapId = psSegment->ui32BufMapId;
            sPrevBuf.ui32DataSize = psParseState->ui32PrevBufDataSize;
            sPrevBuf.ui64BytesRead = sPrevBuf.ui32DataSize;

            // Create the segment the first part of the next view
            ui32Result = bspp_CreateSegment(psGrpBstrCtx, &sPrevBuf);
            IMG_ASSERT(ui32Result == IMG_SUCCESS);
            if (ui32Result != IMG_SUCCESS)
            {
                REPORT(REPORT_MODULE_BSPP, REPORT_ERR,
                        "Cannot create segment for next view");
                goto error;
            }

        }
        else
        {
            REPORT(REPORT_MODULE_BSPP, REPORT_ERR,
                    "Begining of view not found in previous segment/buffer");
            ui32Result = IMG_ERROR_NOT_SUPPORTED;
            goto error;
        }
    }
    else
    {
        //the data just parsed belongs to new view, so use prevoius byte offset
        psCurBuf->ui64BytesRead = psParseState->ui64PrevByteOffsetBuf;

        // Create the segment for previous view
        ui32Result = bspp_CreateSegment(psGrpBstrCtx, psCurBuf);
        IMG_ASSERT(ui32Result == IMG_SUCCESS);
        if (ui32Result != IMG_SUCCESS)
        {
            REPORT(REPORT_MODULE_BSPP, REPORT_ERR,
                    "Cannot create segment for next view");
            goto error;
        }

        //set lists of segments to new view...
        for (i = 0; i < BSPP_MAX_PICTURES_PER_BUFFER; i++)
        {
            psGrpBstrCtx->apsPrePictSegList[i] = &pasPrePictSegList[psGrpBstrCtx->ui8CurrentViewIdx][i];
            psGrpBstrCtx->apsPictSegList[i] = &pasPictSegList[psGrpBstrCtx->ui8CurrentViewIdx][i];

            LST_init(psGrpBstrCtx->apsPrePictSegList[i]);
            LST_init(psGrpBstrCtx->apsPictSegList[i]);
        }
        //and current segment list
        psGrpBstrCtx->psSegmentList = psGrpBstrCtx->apsPictSegList[0];

    }

    //update prefix flag...
    psPreParsedData->asExtPicturesData[psGrpBstrCtx->ui8CurrentViewIdx].bIsPrefix = psParseState->bIsPrefix;
    //and view index
    psGrpBstrCtx->ui8CurrentViewIdx++;

    //set number of extended pictures
    psPreParsedData->ui32NumExtPictures = psGrpBstrCtx->ui8CurrentViewIdx;

    return IMG_SUCCESS;

error:
    return ui32Result;
}

/*!
******************************************************************************

 @Function              BSPP_StreamPreParseBuffers

 @Description

 Buffer list cannot be processed since units in this last buffer may not be complete.
 Must wait until a buffer is provided with end-of-picture signalled.
 When the buffer indicates that units won't span then we can process the bitstream buffer chain.

******************************************************************************/
IMG_RESULT BSPP_StreamPreParseBuffers(
    IMG_HANDLE              hStrContext,
    const BSPP_sDdBufInfo * psContiguousBufInfo,
    IMG_UINT32              ui32ContiguousBufMapId,
	LST_T                 * psSegments,
    BSPP_sPreParsedData   * psPreParsedData
)
{
    BSPP_sStrContext      * psStrContext = (BSPP_sStrContext *)hStrContext;
    BSPP_sPictCtx         * psPictCtx = IMG_NULL;
    BSPP_sParseState      * psParseState = IMG_NULL;
    IMG_UINT32              i,j;
    IMG_UINT32              ui32UnitCount = 0;
    IMG_UINT32              ui32SizeDelimBits = 0;
    SWSR_eFound             eFound = SWSR_FOUND_NONE;
    IMG_UINT32              ui32Result;

	LST_T					asPrePictSegList[BSPP_MAX_PICTURES_PER_BUFFER];
	LST_T				    asPictSegList[BSPP_MAX_PICTURES_PER_BUFFER];

	LST_T					asExtPrePictSegList[VDEC_H264_MVC_MAX_VIEWS][BSPP_MAX_PICTURES_PER_BUFFER];
	LST_T				    asExtPictSegList[VDEC_H264_MVC_MAX_VIEWS][BSPP_MAX_PICTURES_PER_BUFFER];

	BSPP_sBitStrSeg *     psSegment = IMG_NULL;


    IMG_ASSERT(IMG_NULL != hStrContext);
    IMG_ASSERT(IMG_NULL != psPreParsedData);

    if (hStrContext == IMG_NULL)
    {
        REPORT(REPORT_MODULE_BSPP, REPORT_ERR,
               "A BSPP context handle must be provided");
        return IMG_ERROR_INVALID_PARAMETERS;
    }

    if (psPreParsedData == IMG_NULL)
    {
        REPORT(REPORT_MODULE_BSPP, REPORT_ERR,
               "Both bitstream segments and a pre-parsed data container must be provided");
        return IMG_ERROR_INVALID_PARAMETERS;
    }

    // Check that bitstream buffers have been registered.
    if (LST_last(&psStrContext->sGrpBstrCtx.sBufferChain) == IMG_NULL)
    {
        REPORT(REPORT_MODULE_BSPP, REPORT_ERR,
               "No buffers registered for pre-parsing");
        return IMG_ERROR_OPERATION_PROHIBITED;
    }

    psPictCtx = IMG_MALLOC(sizeof(BSPP_sPictCtx));
    IMG_ASSERT(psPictCtx != IMG_NULL);
    if(psPictCtx == IMG_NULL)
    {
        REPORT(REPORT_MODULE_BSPP, REPORT_ERR,
               "Failed to allocate memory for internal structure");
        ui32Result = IMG_ERROR_OUT_OF_MEMORY;
        goto error_pic_ctx;
    }
    IMG_MEMSET(psPictCtx, 0, sizeof(BSPP_sPictCtx));

    psParseState = IMG_MALLOC(sizeof(BSPP_sParseState));
    IMG_ASSERT(psParseState != IMG_NULL);
    if(psParseState == IMG_NULL)
    {
        REPORT(REPORT_MODULE_BSPP, REPORT_ERR,
               "Failed to allocate memory for internal structure");
        ui32Result = IMG_ERROR_OUT_OF_MEMORY;
        goto error_psr_state;
    }
    IMG_MEMSET(psParseState, 0, sizeof(BSPP_sParseState));

    // Initialise the output data.
    IMG_MEMSET(psPreParsedData, 0 , sizeof(BSPP_sPreParsedData));

    for (i = 0; i < BSPP_MAX_PICTURES_PER_BUFFER; i++)
    {
        LST_init(&asPrePictSegList[i]);
        LST_init(&asPictSegList[i]);
    }
	for(i = 0; i < VDEC_H264_MVC_MAX_VIEWS; i++)
	{
		for(j = 0; j < BSPP_MAX_PICTURES_PER_BUFFER; j++)
		{
			LST_init(&asExtPrePictSegList[i][j]);
			LST_init(&asExtPictSegList[i][j]);
		}
	}

    // Setup group buffer processing state.
    psParseState->psInterPictCtx = &psStrContext->sInterPictData;
    psParseState->ui8PrevBottomPicFlag = (IMG_UINT8)BSPP_INVALID;
    psParseState->bNextPicIsNew = IMG_TRUE;
    psParseState->ui32PrevFrameNum = BSPP_INVALID;
    psParseState->ui8SecondFieldFlag = IMG_FALSE;

    for (i = 0; i < VDEC_H264_MVC_MAX_VIEWS; i++)
    {
        psPictCtx->sPictHdrInfo[i].sPictAuxData.ui32Id = BSPP_INVALID;
        psPictCtx->sPictHdrInfo[i].sSecondPictAuxData.ui32Id = BSPP_INVALID;
    }

    // Setup buffer group bitstream context.
    psStrContext->sGrpBstrCtx.eVidStd = psStrContext->eVidStd;
    psStrContext->sGrpBstrCtx.bDisableMvc = psStrContext->bDisableMvc;
    psStrContext->sGrpBstrCtx.bDelimPresent = IMG_TRUE;
    psStrContext->sGrpBstrCtx.hSwSrContext = psStrContext->hSwSrContext;
    psStrContext->sGrpBstrCtx.eUnitType = BSPP_UNIT_NONE;
    psStrContext->sGrpBstrCtx.eLastUnitType = BSPP_UNIT_NONE;
    psStrContext->sGrpBstrCtx.bNotPicUnitYet = IMG_TRUE;
    psStrContext->sGrpBstrCtx.bNotExtPicUnitYet = IMG_TRUE;
    psStrContext->sGrpBstrCtx.ui32TotalBytesRead = 0;
    psStrContext->sGrpBstrCtx.ui8CurrentViewIdx = 0;

    IMG_ASSERT(!LST_empty(&psStrContext->sGrpBstrCtx.sBufferChain));
    IMG_ASSERT(LST_empty(&psStrContext->sGrpBstrCtx.sInFlightBufs));


    for (i = 0; i < BSPP_MAX_PICTURES_PER_BUFFER; i++)
    {
        psStrContext->sGrpBstrCtx.apsPrePictSegList[i] = &asPrePictSegList[i];
        psStrContext->sGrpBstrCtx.apsPictSegList[i] = &asPictSegList[i];
        psStrContext->sGrpBstrCtx.apui64PictTagParam[i] = &psPreParsedData->sPictureData.data[i].aui64PictTagParam;
    }
    psStrContext->sGrpBstrCtx.psSegmentList = psStrContext->sGrpBstrCtx.apsPrePictSegList[0];
    psStrContext->sGrpBstrCtx.pui64PictTagParam = psStrContext->sGrpBstrCtx.apui64PictTagParam[0]; //Fix for BRN45111 ?
    psStrContext->sGrpBstrCtx.psFreeSegments = psSegments;
    psStrContext->sGrpBstrCtx.ui32SegmentOffset = 0;
    psStrContext->sGrpBstrCtx.bInsertStartCode = IMG_FALSE;

    // Before processing the units service all the picture decoded events to free the resources
    bspp_ServicePicturesDecoded(psStrContext);

    // For bitstreams without unit delimiters treat all the buffers as a single unit
    // whose type is defined by the first buffer element.
    if (psStrContext->sSrConfig.eDelimType == SWSR_DELIM_NONE)
    {
        BSPP_sBitstreamBuffer * psCurBuf = LST_first(&psStrContext->sGrpBstrCtx.sBufferChain);

        if (psCurBuf == IMG_NULL || psCurBuf->ui32DataSize == 0)
        {
            // if there is no picture data we must be skipped.
            IMG_ASSERT(psStrContext->sGrpBstrCtx.ui32TotalDataSize == 0);
            psStrContext->sGrpBstrCtx.eUnitType = BSPP_UNIT_SKIP_PICTURE;
        }
        else if (psCurBuf->eBstrElementType == VDEC_BSTRELEMENT_CODEC_CONFIG)
        {
            psStrContext->sGrpBstrCtx.eUnitType = BSPP_UNIT_SEQUENCE;
        }
        else if (psCurBuf->eBstrElementType == VDEC_BSTRELEMENT_PICTURE_DATA ||
                 psCurBuf->eBstrElementType == VDEC_BSTRELEMENT_UNSPECIFIED)
        {
            psStrContext->sGrpBstrCtx.eUnitType = BSPP_UNIT_PICTURE;
            psStrContext->sGrpBstrCtx.psSegmentList = psStrContext->sGrpBstrCtx.apsPictSegList[0];
        }
        else
        {
            IMG_ASSERT(IMG_FALSE);
        }

        psStrContext->sGrpBstrCtx.bDelimPresent = IMG_FALSE;
    }

    // Load the first section (buffer) of biststream into the software shift-register.
    // BSPP maps "buffer" to "section" and allows for contiguous parsing of all
    // buffers since unit boundaries are not known up-front.
    // Unit parsing and segment creation is happening in a single pass.
    ui32Result = SWSR_StartBitstream(psStrContext->hSwSrContext,
                                     &psStrContext->sSrConfig,
                                     psStrContext->sGrpBstrCtx.ui32TotalDataSize,
                                     psStrContext->eEmulationPrevention);
    IMG_ASSERT(ui32Result == IMG_SUCCESS);

    // Seek for next delimiter or end of data and catch any exceptions.
    if (psStrContext->sGrpBstrCtx.bDelimPresent)
    {
        BSPP_TRY(psStrContext->sParseCtx.jump_buffer)
        {
            // Locate the first bitstream unit.
            eFound = SWSR_SeekDelimOrEOD(psStrContext->hSwSrContext);
        }
        BSPP_CATCH(BSPP_EXCEPTION_HANDLER_JUMP)
        {
            REPORT(REPORT_MODULE_BSPP, REPORT_WARNING, "Exception in seeking to delimiter or end of data");
            ui32Result = IMG_ERROR_UNEXPECTED_STATE;
            goto error;
        }
        BSPP_ETRY; /* end of the try catch block */
    }

    if (psStrContext->sSrConfig.eDelimType == SWSR_DELIM_SIZE)
    {
        BSPP_sBitstreamBuffer * psCurBuf = LST_first(&psStrContext->sGrpBstrCtx.sInFlightBufs);

        if (psCurBuf->eBstrElementType == VDEC_BSTRELEMENT_CODEC_CONFIG &&
            psStrContext->eVidStd == VDEC_STD_H264)
        {
            IMG_UINT64  ui64Value = 6;

            // Parse codec config header and catch any exceptions.
            BSPP_TRY(psStrContext->sParseCtx.jump_buffer)
            {
                // Set the shift-register up to provide next 6 bytes
                // without emulation prevention detection.
                SWSR_ConsumeDelim(psStrContext->hSwSrContext, SWSR_EMPREVENT_NONE, 0, &ui64Value);

                // Codec config header must be read for size delimited data (H.264)
                // to get to the start of each unit.
                // This parsing follows section 5.2.4.1.1 of ISO/IEC 14496-15:2004(E).
                SWSR_ReadBits(psStrContext->hSwSrContext, 8); // Configuration version.
                SWSR_ReadBits(psStrContext->hSwSrContext, 8); // AVC Profile Indication.
                SWSR_ReadBits(psStrContext->hSwSrContext, 8); // Profile compatibility.
                SWSR_ReadBits(psStrContext->hSwSrContext, 8); // AVC Level Indication.
                psStrContext->sSrConfig.ui32DelimLength = (SWSR_ReadBits(psStrContext->hSwSrContext, 8) & 0x3) + 1;
                psStrContext->sSrConfig.ui32DelimLength *= 8;
                ui32UnitCount = SWSR_ReadBits(psStrContext->hSwSrContext, 8) & 0x1f;
            }
            BSPP_CATCH(BSPP_EXCEPTION_HANDLER_JUMP)
            {
                REPORT(REPORT_MODULE_BSPP, REPORT_WARNING, "Exception in consuming delimiter");
                ui32Result = IMG_ERROR_UNEXPECTED_STATE;
                goto error;
            }
            BSPP_ETRY; /* end of the try catch block */

            // Size delimiter is only 2 bytes for H.264 codec configuration.
            ui32SizeDelimBits = 2 * 8;
        }
        else
        {
            ui32SizeDelimBits = psStrContext->sSrConfig.ui32DelimLength;
        }
    }

    // Process all the bitstream units until the picture is located.
    while (eFound != SWSR_FOUND_EOD && !psPictCtx->bFinished)
    {
        BSPP_sBitstreamBuffer * psCurBuf = LST_first(&psStrContext->sGrpBstrCtx.sInFlightBufs);

        if (psStrContext->sSrConfig.eDelimType == SWSR_DELIM_SIZE &&
            psCurBuf->eBstrElementType == VDEC_BSTRELEMENT_CODEC_CONFIG)
        {
            if (psStrContext->eVidStd == VDEC_STD_H264)
            {
                if (ui32UnitCount == 0)
                {
                    IMG_UINT64 ui64Value = 1;

                    // Parse middle part of codec config header and catch any exceptions.
                    BSPP_TRY(psStrContext->sParseCtx.jump_buffer)
                    {
                        // Set the shift-register up to provide next 1 byte
                        // without emulation prevention detection.
                        SWSR_ConsumeDelim(psStrContext->hSwSrContext, SWSR_EMPREVENT_NONE, 0, &ui64Value);

                        ui32UnitCount = SWSR_ReadBits(psStrContext->hSwSrContext, 8);
                    }
                    BSPP_CATCH(BSPP_EXCEPTION_HANDLER_JUMP)
                    {
                        REPORT(REPORT_MODULE_BSPP, REPORT_WARNING, "Exception in consuming delimiter");
                        ui32Result = IMG_ERROR_UNEXPECTED_STATE;
                        goto error;
                    }
                    BSPP_ETRY; /* end of the try catch block */
                }

                ui32UnitCount--;
            }
        }

        // Process the next unit.
        ui32Result = bspp_ProcessUnit(psStrContext,
                                      ui32SizeDelimBits,
                                      psPictCtx,
                                      psParseState);
        if (ui32Result == IMG_ERROR_NOT_SUPPORTED)
        {
            REPORT(REPORT_MODULE_BSPP, REPORT_WARNING, "Parser explicitly asked to bail out by returning IMG_ERROR_NOT_SUPPORTED");
            goto error;
        }


        if (psStrContext->sSrConfig.eDelimType != SWSR_DELIM_NONE)
        {
            psStrContext->sGrpBstrCtx.bDelimPresent = IMG_TRUE;
        }

        //jump to the next view
        if(psParseState->bNewView)
        {
            ui32Result = bspp_JumpToNextView( &psStrContext->sGrpBstrCtx,
                                              psPreParsedData,
                                              psParseState,
											  asExtPrePictSegList,
											  asExtPictSegList);
            if(ui32Result != IMG_SUCCESS)
            {
                goto error;
            }

            psParseState->bNewView = IMG_FALSE;
        }

        if (!psPictCtx->bFinished)
        {
            // Seek for next delimiter or end of data and catch any exceptions.
            BSPP_TRY(psStrContext->sParseCtx.jump_buffer)
            {
                // Locate the next bitstream unit or end of data.
                eFound = SWSR_SeekDelimOrEOD(psStrContext->hSwSrContext);
            }
            BSPP_CATCH(BSPP_EXCEPTION_HANDLER_JUMP)
            {
                REPORT(REPORT_MODULE_BSPP, REPORT_WARNING, "Exception in seeking to delimiter or end of data");
                ui32Result = IMG_ERROR_UNEXPECTED_STATE;
                goto error;
            }
            BSPP_ETRY; /* end of the try catch block */

            {
                BSPP_sBitstreamBuffer * psBuf;
                // Update the offset within current buffer.
                SWSR_GetByteOffsetCurBuf(psStrContext->sGrpBstrCtx.hSwSrContext, &psParseState->ui64PrevByteOffsetBuf);
                psBuf = LST_first(&psStrContext->sGrpBstrCtx.sInFlightBufs);
                if(psBuf)
                {
                    psParseState->ui32PrevBufMapId = psBuf->ui32BufMapId;
                    psParseState->ui32PrevBufDataSize = psBuf->ui32DataSize;
                }
            }
        }
    }

    // Create segments for each buffer held by the software shift register (and not yet processed).
    while (LST_first(&psStrContext->sGrpBstrCtx.sInFlightBufs))
    {
        BSPP_sBitstreamBuffer * psBuf = LST_removeHead(&psStrContext->sGrpBstrCtx.sInFlightBufs);

        ui32Result = bspp_TerminateBuffer(&psStrContext->sGrpBstrCtx,
                                          psBuf);
        IMG_ASSERT(ui32Result == IMG_SUCCESS);
    }

    // Create segments for each buffer not yet requested by the shift register.
    while (LST_first(&psStrContext->sGrpBstrCtx.sBufferChain))
    {
        BSPP_sBitstreamBuffer * psBuf = LST_removeHead(&psStrContext->sGrpBstrCtx.sBufferChain);

        ui32Result = bspp_TerminateBuffer(&psStrContext->sGrpBstrCtx,
                                  psBuf);
        IMG_ASSERT(ui32Result == IMG_SUCCESS);
    }

    // Populate the parsed data information for picture only if one is present.
    // The anonymous data has already been added to the appropriate segment list.
    if (psPictCtx->bPresent && !psPictCtx->bInvalid)
    {
        IMG_ASSERT(!LST_empty(&asPictSegList[0]) ||
                   psStrContext->sGrpBstrCtx.eUnitType == BSPP_UNIT_SKIP_PICTURE);
        IMG_ASSERT(psPictCtx->psSequHdrInfo->ui32RefCount>0);

        // Provide data about sequence used by picture.
        // Signal "new sequence" if the sequence header is new or has changed.
        if ( (psPictCtx->psSequHdrInfo->sSequHdrInfo.ui32SequHdrId != psStrContext->ui32SequHdrId) ||
             (psPictCtx->psSequHdrInfo->ui32RefCount == 1) ||
             (psPictCtx->psExtSequHdrInfo)  || //always switch seq when changing base and additional views
             (psPictCtx->bClosedGOP )
           )       
        {
            psPreParsedData->bNewSequence = IMG_TRUE;
            psPreParsedData->sSequHdrInfo = psPictCtx->psSequHdrInfo->sSequHdrInfo;
        }

        // Signal "new subsequence" and its common header information.
        if(psPictCtx->psExtSequHdrInfo)
        {
            psPreParsedData->bNewSubSequence = IMG_TRUE;
            psPreParsedData->sExtSequHdrInfo = psPictCtx->psExtSequHdrInfo->sSequHdrInfo;

            for(i = 0; i < VDEC_H264_MVC_MAX_VIEWS - 1 ; i++)
            {
                //prefix is always the last one
                //do not attach any header info to it
                if(psPreParsedData->asExtPicturesData[i].bIsPrefix)
                {
                    break;
                }

                //attach headers
                psPreParsedData->asExtPicturesData[i].ui32SequHdrId = psPictCtx->psExtSequHdrInfo->sSequHdrInfo.ui32SequHdrId;
                psPictCtx->psExtSequHdrInfo->ui32RefCount++;
                psPreParsedData->asExtPicturesData[i].sPictHdrInfo = psPictCtx->sPictHdrInfo[i+1];
            }

            psPreParsedData->asExtPicturesData[0].sPictHdrInfo.bFirstPicOfSequence = psPreParsedData->bNewSubSequence;

            // Update the base view common sequence info with the number of views that the stream has. Otherwise the number of views is
            // incositant between base view sequence and dependatn view sequences. Also base view sequence appears with one view
            // and the driver calculates the wrong number of resources.
            psPreParsedData->sSequHdrInfo.sComSequHdrInfo.ui32NumViews = psPreParsedData->sExtSequHdrInfo.sComSequHdrInfo.ui32NumViews;
        }


        {
            LST_T             sTmpLst;
			IMG_UINT32 ui32SegCount = 0;

            LST_init(&sTmpLst);

            psSegment = LST_removeHead(&(asPictSegList[0]));
            while(psSegment)
            {
                LST_add(&sTmpLst, psSegment);
                psSegment = LST_removeHead(&(asPictSegList[0]));
            }

            psSegment = LST_removeHead(&(psStrContext->sInterPictData.sPicPrefixSeg));
            while(psSegment)
            {
				IMG_ASSERT(ui32SegCount < MAX_SEGMENTS_PER_PIC);
				if(ui32SegCount >= MAX_SEGMENTS_PER_PIC)
				{
					REPORT(REPORT_MODULE_BSPP, REPORT_ERR,
						"More segments per picture then valid!!");
					ui32Result = IMG_ERROR_INVALID_PARAMETERS;
					goto error;
				}
				IMG_MEMCPY(&psPreParsedData->sPictureData.data[0].asPictSegments[psPreParsedData->sPictureData.data[0].ui32PictSegments++],psSegment,sizeof(*psSegment));
				ui32SegCount = psPreParsedData->sPictureData.data[0].ui32PictSegments;
				LST_add(psSegments,psSegment);
                psSegment = LST_removeHead(&(psStrContext->sInterPictData.sPicPrefixSeg));
            }

            psSegment = LST_removeHead(&sTmpLst);
            while(psSegment)
            {
				IMG_ASSERT(ui32SegCount < MAX_SEGMENTS_PER_PIC);
				if(ui32SegCount >= MAX_SEGMENTS_PER_PIC)
				{
					REPORT(REPORT_MODULE_BSPP, REPORT_ERR,
						"More segments per picture then valid!!");
					ui32Result = IMG_ERROR_INVALID_PARAMETERS;
					goto error;
				}
				IMG_MEMCPY(&psPreParsedData->sPictureData.data[0].asPictSegments[psPreParsedData->sPictureData.data[0].ui32PictSegments++],psSegment,sizeof(*psSegment));
				ui32SegCount = psPreParsedData->sPictureData.data[0].ui32PictSegments;
				LST_add(psSegments,psSegment);
                psSegment = LST_removeHead(&sTmpLst);
            }

			psSegment = LST_removeHead(&(asPrePictSegList[0]));
            while(psSegment)
            {
                IMG_ASSERT(ui32SegCount < MAX_SEGMENTS_PER_PIC);
				if(ui32SegCount >= MAX_SEGMENTS_PER_PIC)
				{
					REPORT(REPORT_MODULE_BSPP, REPORT_ERR,
						"More segments per picture then valid!!");
					ui32Result = IMG_ERROR_INVALID_PARAMETERS;
					goto error;
				}
				IMG_MEMCPY(&psPreParsedData->sPictureData.data[0].asPrePictSegments[psPreParsedData->sPictureData.data[0].ui32PrePictSegments++],psSegment,sizeof(*psSegment));
				ui32SegCount = psPreParsedData->sPictureData.data[0].ui32PrePictSegments;
				LST_add(psSegments,psSegment);
                psSegment = LST_removeHead(&(asPrePictSegList[0]));
            }

			for(i = 1; i < BSPP_MAX_PICTURES_PER_BUFFER; i++)
			{
				ui32SegCount = 0;
				psSegment = LST_removeHead(&asPictSegList[i]);
				while(psSegment)
				{
					IMG_ASSERT(ui32SegCount < MAX_SEGMENTS_PER_PIC);
					if(ui32SegCount >= MAX_SEGMENTS_PER_PIC)
					{
						REPORT(REPORT_MODULE_BSPP, REPORT_ERR,
							"More segments per picture then valid!!");
						ui32Result = IMG_ERROR_INVALID_PARAMETERS;
						goto error;
					}
					IMG_MEMCPY(&psPreParsedData->sPictureData.data[i].asPictSegments[psPreParsedData->sPictureData.data[i].ui32PictSegments++],psSegment,sizeof(*psSegment));
					ui32SegCount = psPreParsedData->sPictureData.data[i].ui32PictSegments;
					LST_add(psSegments,psSegment);
					psSegment = LST_removeHead(&asPictSegList[i]);
				}

				psSegment = LST_removeHead(&(asPrePictSegList[i]));
				while(psSegment)
				{
					IMG_ASSERT(ui32SegCount < MAX_SEGMENTS_PER_PIC);
					if(ui32SegCount >= MAX_SEGMENTS_PER_PIC)
					{
						REPORT(REPORT_MODULE_BSPP, REPORT_ERR,
							"More segments per picture then valid!!");
						ui32Result = IMG_ERROR_INVALID_PARAMETERS;
						goto error;
					}
					IMG_MEMCPY(&psPreParsedData->sPictureData.data[i].asPrePictSegments[psPreParsedData->sPictureData.data[i].ui32PrePictSegments++],psSegment,sizeof(*psSegment));
					ui32SegCount = psPreParsedData->sPictureData.data[i].ui32PrePictSegments;

					//LST_add(&(psPreParsedData->sPictureData.data[0].asPictSegList),psSegment);
					LST_add(psSegments,psSegment);
					psSegment = LST_removeHead(&(asPrePictSegList[i]));
				}
			}

            for(i = 0; i < VDEC_H264_MVC_MAX_VIEWS; i++)
            {
                if(psPreParsedData->asExtPicturesData[i].bIsPrefix)
                {
                    for(j = 0; j < BSPP_MAX_PICTURES_PER_BUFFER; j++)
                    {
                        psSegment = LST_removeHead(&(asExtPictSegList[i][j]));
                        while(psSegment)
                        {
                            LST_add(&(psStrContext->sInterPictData.sPicPrefixSeg),psSegment);
                            psSegment = LST_removeHead(&(asExtPictSegList[i][j]));
                        }
                    }
                    psPreParsedData->ui32NumExtPictures--;
                    break;

                }
            }
			
			for(i = 0; i < VDEC_H264_MVC_MAX_VIEWS; i++)
            {
                for(j = 0; j < BSPP_MAX_PICTURES_PER_BUFFER; j++)
                {
                    psSegment = LST_removeHead(&(asExtPictSegList[i][j]));
					ui32SegCount = 0;
                    while(psSegment)
                    {
                        IMG_ASSERT(ui32SegCount < MAX_SEGMENTS_PER_PIC);
						if(ui32SegCount < MAX_SEGMENTS_PER_PIC)
						{
							REPORT(REPORT_MODULE_BSPP, REPORT_ERR,
								"More segments per picture then valid!!");
							ui32Result = IMG_ERROR_INVALID_PARAMETERS;
							goto error;
						}
						IMG_MEMCPY(&psPreParsedData->asExtPicturesData[i].data[j].asPictSegments[psPreParsedData->asExtPicturesData[i].data[j].ui32PictSegments++],psSegment,sizeof(*psSegment));
						ui32SegCount = psPreParsedData->asExtPicturesData[i].data[j].ui32PictSegments;
						
						LST_add(psSegments,psSegment);
                        psSegment = LST_removeHead(&(asExtPictSegList[i][j]));
                    }

					psSegment = LST_removeHead(&(asExtPrePictSegList[i][j]));
                    while(psSegment)
                    {
                        IMG_ASSERT(ui32SegCount < MAX_SEGMENTS_PER_PIC);
						if(ui32SegCount < MAX_SEGMENTS_PER_PIC)
						{
							REPORT(REPORT_MODULE_BSPP, REPORT_ERR,
								"More segments per picture then valid!!");
							ui32Result = IMG_ERROR_INVALID_PARAMETERS;
							goto error;
						}
						IMG_MEMCPY(&psPreParsedData->asExtPicturesData[i].data[j].asPrePictSegments[psPreParsedData->asExtPicturesData[i].data[j].ui32PrePictSegments++],psSegment,sizeof(*psSegment));
						ui32SegCount = psPreParsedData->asExtPicturesData[i].data[j].ui32PrePictSegments;
						
						LST_add(psSegments,psSegment);
                        psSegment = LST_removeHead(&(asExtPrePictSegList[i][j]));
                    }
                }
            }

        }

        // Signal if this picture is the first in a closed GOP.
        if (psPictCtx->bClosedGOP)
        {
            psPreParsedData->bClosedGOP = IMG_TRUE;
            psPreParsedData->sSequHdrInfo.sComSequHdrInfo.bNotDpbFlush =
                psStrContext->sInterPictData.bNotDpbFlush;
        }

        // Signal "new picture" and its common header information.
        psPreParsedData->bNewPicture = IMG_TRUE;
        psPreParsedData->sPictureData.ui32SequHdrId = psPictCtx->psSequHdrInfo->sSequHdrInfo.ui32SequHdrId;
        psPreParsedData->sPictureData.sPictHdrInfo =  psPictCtx->sPictHdrInfo[0];

        psPreParsedData->sPictureData.sPictHdrInfo.bFirstPicOfSequence = psPreParsedData->bNewSequence;
        psPreParsedData->sPictureData.sPictHdrInfo.bFragmentedData = (psContiguousBufInfo != IMG_NULL);

        psStrContext->ui32SequHdrId = psPreParsedData->sPictureData.ui32SequHdrId;

        if (psStrContext->sGrpBstrCtx.eVidStd == VDEC_STD_VP8 && psContiguousBufInfo != IMG_NULL)
        {
			IMG_UINT32 i = 0;

            //Reconstruct Buffer from the segments

			for(i=0;i<psPreParsedData->sPictureData.data[0].ui32PictSegments;i++)
			{
				if(psPreParsedData->sPictureData.data[0].asPictSegments[i].ui32BufMapId != ui32ContiguousBufMapId)
                {
                    psPreParsedData->sPictureData.data[0].asPictSegments[i].ui32BitStrSegFlag |= VDECDD_BSSEG_SKIP;
                }
                else
                {
                    psPreParsedData->sPictureData.data[0].asPictSegments[i].ui32BitStrSegFlag |= VDECDD_BSSEG_LASTINBUFF;
                }
			}
        }
    }
    else if (psPictCtx->bPresent)
    {
        REPORT(REPORT_MODULE_BSPP, REPORT_WARNING,
               "Ignoring picture since it cannot be decoded correctly");

        // Reduce the reference count since this picture will not be decoded.
        IMG_ASSERT(psPictCtx->psSequHdrInfo);
        psPictCtx->psSequHdrInfo->ui32RefCount--;

        // This may need to happen earlier because more PPS data may have
        // been received since the picture header was processed.
        if (psPictCtx->sPictHdrInfo[0].sPictAuxData.ui32Id != BSPP_INVALID)
        {
            REPORT(REPORT_MODULE_BSPP, REPORT_WARNING,
                   "Picture aux (+secondary) data should be freed and the PPS ref count decremented");
        }
    }

    // Reset the group bitstream context.
    IMG_ASSERT(LST_empty(&psStrContext->sGrpBstrCtx.sBufferChain));
    LST_init(&psStrContext->sGrpBstrCtx.sBufferChain);
    IMG_MEMSET(&psStrContext->sGrpBstrCtx, 0, sizeof(psStrContext->sGrpBstrCtx));

    // for now: return IMG_ERROR_NOT_SUPPORTED only if explicitly set by parser
    //ui32Result = psPictCtx->bUnsupported ? IMG_ERROR_NOT_SUPPORTED : IMG_SUCCESS;
    ui32Result = (ui32Result == IMG_ERROR_NOT_SUPPORTED) ? IMG_ERROR_NOT_SUPPORTED : IMG_SUCCESS;

    IMG_FREE(psPictCtx);
    IMG_FREE(psParseState);


	return ui32Result;

error:
    // Free the SWSR list of buffers
    while (LST_first(&psStrContext->sGrpBstrCtx.sInFlightBufs))
    {
        LST_removeHead(&psStrContext->sGrpBstrCtx.sInFlightBufs);
    }
    IMG_FREE(psParseState);
error_psr_state:
    IMG_FREE(psPictCtx);
error_pic_ctx:
    return ui32Result;
}


/*!
******************************************************************************

 @Function              bspp_LogPictHdr

******************************************************************************/
static IMG_RESULT
bspp_LogPictHdr(
    const BSPP_sPictHdrInfo * psPictHdrInfo
)
{
    IMG_UINT32 i;
    BSPP_LOG_PRINT("%s%s", ">>>>>>>>>>>>>> PICTURE HEADER\n", "");

    BSPP_LOG_INT("bIntraCoded", psPictHdrInfo->bIntraCoded);
    BSPP_LOG_INT("bReference", psPictHdrInfo->bReference);
    BSPP_LOG_INT("bField", psPictHdrInfo->bField);
    BSPP_LOG_INT("bEmulationPrevention", psPictHdrInfo->bEmulationPrevention);
    BSPP_LOG_INT("bPostProcessing", psPictHdrInfo->bPostProcessing);
    BSPP_LOG_INT("bDiscontinuousMbs", psPictHdrInfo->bDiscontinuousMbs);
    BSPP_LOG_INT("bFragmentedData", psPictHdrInfo->bFragmentedData);
    BSPP_LOG_INT("ui32PicDataSize", psPictHdrInfo->ui32PicDataSize);

    BSPP_LOG_INT("sCodedFrameSize.ui32Width", psPictHdrInfo->sCodedFrameSize.ui32Width);
    BSPP_LOG_INT("sCodedFrameSize.ui32Height", psPictHdrInfo->sCodedFrameSize.ui32Height);

    BSPP_LOG_INT("sDispInfo.sEncDispRegion.ui32TopOffset", psPictHdrInfo->sDispInfo.sEncDispRegion.ui32TopOffset);
    BSPP_LOG_INT("sDispInfo.sEncDispRegion.ui32LeftOffset", psPictHdrInfo->sDispInfo.sEncDispRegion.ui32LeftOffset);
    BSPP_LOG_INT("sDispInfo.sEncDispRegion.ui32Width", psPictHdrInfo->sDispInfo.sEncDispRegion.ui32Width);
    BSPP_LOG_INT("sDispInfo.sEncDispRegion.ui32Height", psPictHdrInfo->sDispInfo.sEncDispRegion.ui32Height);

    BSPP_LOG_INT("sDispInfo.sDispRegion.ui32TopOffset", psPictHdrInfo->sDispInfo.sDispRegion.ui32TopOffset);
    BSPP_LOG_INT("sDispInfo.sDispRegion.ui32LeftOffset", psPictHdrInfo->sDispInfo.sDispRegion.ui32LeftOffset);
    BSPP_LOG_INT("sDispInfo.sDispRegion.ui32Width", psPictHdrInfo->sDispInfo.sDispRegion.ui32Width);
    BSPP_LOG_INT("sDispInfo.sDispRegion.ui32Height", psPictHdrInfo->sDispInfo.sDispRegion.ui32Height);

    BSPP_LOG_INT("sDispInfo.bTopFieldFirst", psPictHdrInfo->sDispInfo.bTopFieldFirst);
    BSPP_LOG_INT("sDispInfo.ui32MaxFrmRepeat", psPictHdrInfo->sDispInfo.ui32MaxFrmRepeat);
    BSPP_LOG_INT("sDispInfo.bRepeatFirstField", psPictHdrInfo->sDispInfo.bRepeatFirstField);
    BSPP_LOG_INT("sDispInfo.ui32NumPanScanWindows", psPictHdrInfo->sDispInfo.ui32NumPanScanWindows);
    for (i = 0; i < VDEC_MAX_PANSCAN_WINDOWS; i++)
    {
        BSPP_LOG_INT("sDispInfo.asPanScanWindow ui32Width ", psPictHdrInfo->sDispInfo.asPanScanWindows[i].ui32Width);
        BSPP_LOG_INT("sDispInfo.asPanScanWindow ui32Height ", psPictHdrInfo->sDispInfo.asPanScanWindows[i].ui32Height);
        BSPP_LOG_INT("sDispInfo.asPanScanWindow ui32TopOffset ", psPictHdrInfo->sDispInfo.asPanScanWindows[i].ui32TopOffset);
        BSPP_LOG_INT("sDispInfo.asPanScanWindow ui32LeftOffset ", psPictHdrInfo->sDispInfo.asPanScanWindows[i].ui32LeftOffset);
    }


    return IMG_SUCCESS;
}


/*!
******************************************************************************

 @Function              bspp_LogComSequHdr

******************************************************************************/
static IMG_RESULT
bspp_LogComSequHdr(
    const VDEC_sComSequHdrInfo  * psComSequHdrInfo
)
{
    BSPP_LOG_PRINT("%s%s", ">>>>>>>>>>>>>> SEQUENCE HEADER\n", "");

    BSPP_LOG_INT("ui32CodecProfile", psComSequHdrInfo->ui32CodecProfile);
    BSPP_LOG_INT("ui32CodecLevel", psComSequHdrInfo->ui32CodecLevel);

    BSPP_LOG_INT("ui32Bitrate", psComSequHdrInfo->ui32Bitrate);
    BSPP_LOG_FLOAT("fFrameRate", psComSequHdrInfo->fFrameRate);
    BSPP_LOG_INT("ui32FrameRateNum", psComSequHdrInfo->ui32FrameRateNum);
    BSPP_LOG_INT("ui32FrameRateDen", psComSequHdrInfo->ui32FrameRateDen);

    BSPP_LOG_INT("ui32AspectRatioNum", psComSequHdrInfo->ui32AspectRatioNum);
    BSPP_LOG_INT("ui32AspectRatioDen", psComSequHdrInfo->ui32AspectRatioDen);

    BSPP_LOG_INT("bInterlacedFrames", psComSequHdrInfo->bInterlacedFrames);

    BSPP_LOG_INT("sPixelInfo.ePixelFormat", psComSequHdrInfo->sPixelInfo.ePixelFormat);
    BSPP_LOG_INT("sPixelInfo.eChromaInterleaved", psComSequHdrInfo->sPixelInfo.eChromaInterleaved);
    BSPP_LOG_INT("sPixelInfo.bChromaFormat", psComSequHdrInfo->sPixelInfo.bChromaFormat);
    BSPP_LOG_INT("sPixelInfo.eMemoryPacking", psComSequHdrInfo->sPixelInfo.eMemoryPacking);
    BSPP_LOG_INT("sPixelInfo.eChromaFormatIdc", psComSequHdrInfo->sPixelInfo.eChromaFormatIdc);
    BSPP_LOG_INT("sPixelInfo.ui32BitDepthY", psComSequHdrInfo->sPixelInfo.ui32BitDepthY);
    BSPP_LOG_INT("sPixelInfo.ui32BitDepthC", psComSequHdrInfo->sPixelInfo.ui32BitDepthC);
    BSPP_LOG_INT("sPixelInfo.ui32NoPlanes", psComSequHdrInfo->sPixelInfo.ui32NoPlanes);

    BSPP_LOG_INT("sMaxFrameSize.ui32Width", psComSequHdrInfo->sMaxFrameSize.ui32Width);
    BSPP_LOG_INT("sMaxFrameSize.ui32Height", psComSequHdrInfo->sMaxFrameSize.ui32Height);

    BSPP_LOG_INT("bFieldCodedMBlocks", psComSequHdrInfo->bFieldCodedMBlocks);

    BSPP_LOG_INT("ui32MinPicBufNum", psComSequHdrInfo->ui32MinPicBufNum);
    BSPP_LOG_INT("bPictureReordering", psComSequHdrInfo->bPictureReordering);

    BSPP_LOG_INT("bPostProcessing", psComSequHdrInfo->bPostProcessing);

    BSPP_LOG_INT("sOrigDisplayRegion.ui32TopOffset", psComSequHdrInfo->sOrigDisplayRegion.ui32TopOffset);
    BSPP_LOG_INT("sOrigDisplayRegion.ui32LeftOffset", psComSequHdrInfo->sOrigDisplayRegion.ui32LeftOffset);
    BSPP_LOG_INT("sOrigDisplayRegion.ui32Width", psComSequHdrInfo->sOrigDisplayRegion.ui32Width);
    BSPP_LOG_INT("sOrigDisplayRegion.ui32Height", psComSequHdrInfo->sOrigDisplayRegion.ui32Height);

    return IMG_SUCCESS;
}


/*!
******************************************************************************

 @Function              BSPP_StreamLogPicture

******************************************************************************/
IMG_RESULT BSPP_StreamLogPicture(
    const IMG_HANDLE          hStrContext,
    const BSPP_sPictHdrInfo * psPictHdrInfo
)
{
    BSPP_sStrContext  * psStrContext = (BSPP_sStrContext *)hStrContext;
    IMG_UINT32          ui32Result;

    if (hStrContext == IMG_NULL)
    {
        REPORT(REPORT_MODULE_BSPP, REPORT_ERR,
               "A BSPP context handle must be provided");
        ui32Result = IMG_ERROR_INVALID_PARAMETERS;
        goto error;
    }

#if defined(BSPP_LOG) && !defined (BSPP_KM) && defined (BSPP_UM)
    fpLog = psStrContext->fpLog;
#endif

    if (psPictHdrInfo == IMG_NULL)
    {
        REPORT(REPORT_MODULE_BSPP, REPORT_ERR,
               "A valid picture header must be provided");
        ui32Result = IMG_ERROR_INVALID_PARAMETERS;
        goto error;
    }

    // Log the common sequence header information.
    ui32Result = bspp_LogPictHdr(psPictHdrInfo);
    IMG_ASSERT(ui32Result == IMG_SUCCESS);
    if (ui32Result != IMG_SUCCESS)
    {
        goto error;
    }

    // Log standard specific sequence header information.
    switch(psStrContext->eVidStd)
    {
    case VDEC_STD_H264:
        {
#if 0

            BSPP_sPPSInfo * psBsppPpsInfo;
            IMG_HANDLE      hSecurePpsInfo = IMG_NULL;

#if 0
            // Obtain last instance of sequence data for given ID.
            psBsppPpsInfo = LST_last(&psStrContext->sStrAlloc.asPPSDataList[psPictHdrInfo->ui32AuxId]);
            IMG_ASSERT(psBsppPpsInfo);

            hSecurePpsInfo = psBsppPpsInfo->hSecurePPSInfo;

            IMG_ASSERT(psBsppPpsInfo->sFWPPS.pvCpuLinearAddr == psPictHdrInfo->sPictAuxData.pvData);

            BSPP_H264LogPpsInfo(psBsppPpsInfo->sFWPPS.pvCpuLinearAddr, hSecurePpsInfo);
#else
            BSPP_H264LogPpsInfo(psPictHdrInfo->sPictAuxData.pvData, IMG_NULL);
#endif

#endif // #if 0
        }
        break;

    default:
        /*REPORT(REPORT_MODULE_BSPP, REPORT_WARNING,
               "No standard-specific picture logging function registered");*/
        break;
    }

    return IMG_SUCCESS;

error:
    return ui32Result;
}


/*!
******************************************************************************

 @Function              BSPP_StreamLogSequence

******************************************************************************/
IMG_RESULT BSPP_StreamLogSequence(
    const IMG_HANDLE            hStrContext,
    IMG_UINT32                  ui32SequHdrId,
    const BSPP_sSequHdrInfo   * psSequHdrInfo
)
{
    BSPP_sStrContext *      psStrContext = (BSPP_sStrContext *)hStrContext;
    BSPP_sSequenceHdrInfo * psBsppSequHdrInfo;
    IMG_HANDLE              hSecureSequInfo = IMG_NULL;
    IMG_UINT32              ui32Result;

    if (hStrContext == IMG_NULL)
    {
        REPORT(REPORT_MODULE_BSPP, REPORT_ERR,
               "A BSPP context handle must be provided");
        ui32Result = IMG_ERROR_INVALID_PARAMETERS;
        goto error;
    }

#if defined(BSPP_LOG) && !defined (BSPP_KM) && defined (BSPP_UM)
    fpLog = psStrContext->fpLog;
#endif

    if (psSequHdrInfo == IMG_NULL &&
        ui32SequHdrId == BSPP_INVALID)
    {
        REPORT(REPORT_MODULE_BSPP, REPORT_ERR,
               "A valid sequence header or ID must be provided");
        ui32Result = IMG_ERROR_INVALID_PARAMETERS;
        goto error;
    }

    if (psSequHdrInfo == IMG_NULL)
    {
        // Obtain last instance of sequence data for given ID.
        psBsppSequHdrInfo = LST_last(&psStrContext->sStrAlloc.asSequenceDataList[ui32SequHdrId]);
        IMG_ASSERT(psBsppSequHdrInfo);
        IMG_ASSERT(psBsppSequHdrInfo->ui32RefCount>0);

        psSequHdrInfo = &psBsppSequHdrInfo->sSequHdrInfo;
        hSecureSequInfo = psBsppSequHdrInfo->hSecureSequenceInfo;
    }

    // Log the common sequence header information.
    ui32Result = bspp_LogComSequHdr(&psSequHdrInfo->sComSequHdrInfo);
    IMG_ASSERT(ui32Result == IMG_SUCCESS);
    if (ui32Result != IMG_SUCCESS)
    {
        goto error;
    }

    // Log standard specific sequence header information.
    switch(psStrContext->eVidStd)
    {
    case VDEC_STD_H264:
        BSPP_H264LogSequHdrInfo(psSequHdrInfo, hSecureSequInfo);
        break;

    default:
        REPORT(REPORT_MODULE_BSPP, REPORT_WARNING,
               "No standard-specific sequence logging function registered");
        break;
    }

    return IMG_SUCCESS;

error:
    return ui32Result;
}


/*!
******************************************************************************

 @Function              BSPP_StreamDestroy

******************************************************************************/
IMG_RESULT BSPP_StreamDestroy(
    const IMG_HANDLE            hStrContext
)
{
    BSPP_sStrContext *      psStrContext = (BSPP_sStrContext *)hStrContext;
    IMG_UINT32              i;
    IMG_UINT32              ui32SpsId;
    IMG_UINT32              ui32PpsId;
    BSPP_sSequenceHdrInfo * psSequHdrInfo;
    BSPP_sPPSInfo         * psPPSInfo;
    IMG_UINT32              ui32Result;

    // Validate input arguments.
    if (hStrContext == IMG_NULL)
    {
        REPORT(REPORT_MODULE_BSPP, REPORT_ERR,
               "A BSPP context handle must be provided");
        ui32Result = IMG_ERROR_INVALID_PARAMETERS;
        goto error;
    }

    SWSR_DeInitialise(psStrContext->hSwSrContext);

    // Service all the picture decoded events and free any unused resources.
    bspp_ServicePicturesDecoded(psStrContext);
    for (ui32SpsId = 0; ui32SpsId < SEQUENCE_SLOTS_SECURE; ui32SpsId++)
    {
        bspp_RemoveUnusedSequence(&psStrContext->sStrAlloc, ui32SpsId);
    }
    for (ui32PpsId = 0; ui32PpsId < PPS_SLOTS_SECURE; ui32PpsId++)
    {
        bspp_RemoveUnusedPps(&psStrContext->sStrAlloc, ui32PpsId);
    }

    // Free the memory required for this stream.
    for (i=0; i<SEQUENCE_SLOTS_SECURE; i++)
    {
        psSequHdrInfo = LST_removeHead(&psStrContext->sStrAlloc.asSequenceDataList[i]);
        if (psSequHdrInfo)
        {
            LST_add(&psStrContext->sStrAlloc.sAvailableSequencesList, psSequHdrInfo);
        }

        // when we are done with the stream we should have MAXIMUM 1 sequence per slot, so after removing this one we should have none
        // In case of "decodenframes" this is not true because we send more pictures for decode than what we expect to receive back,
        // which means that potentially additional sequences/PPS are in the list
        psSequHdrInfo = LST_removeHead(&psStrContext->sStrAlloc.asSequenceDataList[i]);
        if (psSequHdrInfo)
        {
            IMG_UINT32 ui32CountExtraSequences = 0;
            do
            {
                ui32CountExtraSequences++;
                LST_add(&psStrContext->sStrAlloc.sAvailableSequencesList, psSequHdrInfo);
                psSequHdrInfo = LST_removeHead(&psStrContext->sStrAlloc.asSequenceDataList[i]);
            } while(psSequHdrInfo);
            REPORT(REPORT_MODULE_BSPP, REPORT_WARNING,
                "Sequence Slot \t[%d]: \tAdditional Sequences Detected during shutdown: %d", i, ui32CountExtraSequences); // implies not all pictures submitted have been decoded
        }
    }

    if (psStrContext->eVidStd == VDEC_STD_H264)
    {
        for (i=0; i<PPS_SLOTS_SECURE; i++)
        {
            psPPSInfo = LST_removeHead(&psStrContext->sStrAlloc.asPPSDataList[i]);
            if (psPPSInfo)
            {
                LST_add(&psStrContext->sStrAlloc.sAvailablePPSsList, psPPSInfo);
            }

            // when we are done with the stream we should have MAXIMUM 1 PPS per slot, so after removing this one we should have none
            // In case of "decodenframes" this is not true because we send more pictures for decode than what we expect to receive back,
            // which means that potentially additional sequences/PPS are in the list
            psPPSInfo = LST_removeHead(&psStrContext->sStrAlloc.asPPSDataList[i]);
            if (psPPSInfo)
            {
                IMG_UINT32 ui32CountExtraPPSs = 0;
                do
                {
                    ui32CountExtraPPSs++;
                    LST_add(&psStrContext->sStrAlloc.sAvailablePPSsList, psPPSInfo);
                    psPPSInfo = LST_removeHead(&psStrContext->sStrAlloc.asPPSDataList[i]);
                } while(psPPSInfo);
                REPORT(REPORT_MODULE_BSPP, REPORT_WARNING,
                    "PPS Slot \t[%d]: \tAdditional PPSs Detected during shutdown: %d", i, ui32CountExtraPPSs); // implies not all pictures submitted have been decoded
            }
            IMG_ASSERT(LST_empty(&psStrContext->sStrAlloc.asPPSDataList[i]));
        }
    }

    if(psStrContext->eVidStd == VDEC_STD_H264)
    {
       if(psStrContext->sStrAlloc.psSEIDataInfo)
       {
           IMG_BIGFREE(psStrContext->sStrAlloc.psSEIDataInfo);
           psStrContext->sStrAlloc.psSEIDataInfo = IMG_NULL;
       }
    }

	// Just unmap the first elements of sequence arrays
	IMG_ASSERT(!LST_empty(&psStrContext->sStrAlloc.sAvailableSequencesList));
	psSequHdrInfo = LST_first(&psStrContext->sStrAlloc.sAvailableSequencesList);
	if (psSequHdrInfo && psSequHdrInfo->sFWSequence.sDdBufInfo.pvCpuVirt)
	{
		// Just unmap the first Sequence element. All units were mapped toghether, and have same
		// pvCpuVirt. Just unmap one.
		SECMEM_UnmapSecureMemory(psSequHdrInfo->sFWSequence.sDdBufInfo.pvCpuVirt);
	}

    for (i = 0; i < MAX_SEQUENCES_SECURE; i++)
    {
        IMG_ASSERT(!LST_empty(&psStrContext->sStrAlloc.sAvailableSequencesList));
        psSequHdrInfo = LST_removeHead(&psStrContext->sStrAlloc.sAvailableSequencesList);

        if (psStrContext->eVidStd == VDEC_STD_H264 && psSequHdrInfo && psSequHdrInfo->hSecureSequenceInfo)
        {
            BSPP_H264DestroySequHdrInfo(psSequHdrInfo->hSecureSequenceInfo);
        }
    }
    IMG_ASSERT(LST_empty(&psStrContext->sStrAlloc.sAvailableSequencesList));

    if(psStrContext->pui8SecureSequenceInfo)
    {
        IMG_SECURE_FREE(psStrContext->pui8SecureSequenceInfo);
        psStrContext->pui8SecureSequenceInfo = IMG_NULL;
    }
    if(psStrContext->pui8SequHdrInfo)
    {
        IMG_SECURE_FREE(psStrContext->pui8SequHdrInfo);
        psStrContext->pui8SequHdrInfo = IMG_NULL;
    }

    if (psStrContext->eVidStd == VDEC_STD_H264)
	{
		// Just unmap the first PPS element (rest refer to the same buffer)
		IMG_ASSERT(!LST_empty(&psStrContext->sStrAlloc.sAvailablePPSsList));
		psPPSInfo = LST_first(&psStrContext->sStrAlloc.sAvailablePPSsList);		
		if(psPPSInfo && psPPSInfo->sFWPPS.sDdBufInfo.pvCpuVirt)
		{
			// Just unmap the first PPS element. All units were mapped toghether, and have same
			// pvCpuVirt. Just unmap one.
			SECMEM_UnmapSecureMemory(psPPSInfo->sFWPPS.sDdBufInfo.pvCpuVirt);
		}	

		for (i = 0; i < MAX_PPSS_SECURE; i++)
        {
            IMG_ASSERT(!LST_empty(&psStrContext->sStrAlloc.sAvailablePPSsList));
            psPPSInfo = LST_removeHead(&psStrContext->sStrAlloc.sAvailablePPSsList);

            if(psPPSInfo && psPPSInfo->hSecurePPSInfo && psStrContext->eVidStd == VDEC_STD_H264)
            {
                BSPP_H264DestroyPPSInfo(psPPSInfo->hSecurePPSInfo);
            }
        }
        IMG_ASSERT(LST_empty(&psStrContext->sStrAlloc.sAvailablePPSsList));

        if(psStrContext->pui8SecurePPSInfo)
        {
            IMG_SECURE_FREE(psStrContext->pui8SecurePPSInfo);
            psStrContext->pui8SecurePPSInfo = IMG_NULL;
        }
        if(psStrContext->pui8PPSInfo)
        {
            IMG_SECURE_FREE(psStrContext->pui8PPSInfo);
            psStrContext->pui8PPSInfo = IMG_NULL;
        }
    }

    SEC_DestroyMutex(psStrContext->sMutex.hMutex);

#if defined(BSPP_LOG) && !defined (BSPP_KM) && defined (BSPP_UM)
    if (psStrContext->fpLog)
    {
        fclose(psStrContext->fpLog);
    }
#endif

    IMG_FREE(psStrContext);

    return IMG_SUCCESS;

error:
    return ui32Result;
}


/*!
******************************************************************************

 @Function              BSPP_SetCodecConfig

******************************************************************************/
IMG_RESULT BSPP_SetCodecConfig(
    const IMG_HANDLE            hStrContext,
    const VDEC_sCodecConfig   * psCodecConfig
)
{
	return IMG_ERROR_NOT_SUPPORTED;
}


/*!
******************************************************************************

 @Function              BSPP_StreamCreate

******************************************************************************/
IMG_RESULT BSPP_StreamCreate(
    const VDEC_sStrConfigData * psStrConfigData,
    IMG_HANDLE            * phStrContext,
    BSPP_sDdBufArrayInfo    asFWSequence[],
    BSPP_sDdBufArrayInfo    asFWPPS[]
)
{
    BSPP_sStrContext      * psStrContext;
    IMG_UINT32              ui32Result = IMG_SUCCESS;
    IMG_UINT32              i;
    BSPP_sSequenceHdrInfo * psSequHdrInfo;
    BSPP_sPPSInfo         * psPPSInfo;

#if defined(BSPP_LOG) && !defined (BSPP_KM) && defined (BSPP_UM)
    //fpLog = fopen("log_new.txt", "wt");
#endif

    /* Allocate a stream structure...*/
    psStrContext = IMG_MALLOC(sizeof(BSPP_sStrContext));

    IMG_ASSERT(psStrContext != IMG_NULL);
    if (psStrContext == IMG_NULL)
    {
        REPORT(REPORT_MODULE_BSPP, REPORT_ERR,
            "Failed to allocate memory for stream context");

        ui32Result = IMG_ERROR_OUT_OF_MEMORY;
        goto error;
    }
    IMG_MEMSET(psStrContext, 0, sizeof(BSPP_sStrContext));

    // Initialise the stream context structure.
    psStrContext->ui32SequHdrId = BSPP_INVALID;
    psStrContext->eVidStd = psStrConfigData->eVidStd;
    psStrContext->eBstrFormat = psStrConfigData->eBstrFormat;
    psStrContext->bDisableMvc = psStrConfigData->bDisableMvc;
    psStrContext->bFullScan = psStrConfigData->bFullScan;
    psStrContext->bImmediateDecode = psStrConfigData->bImmediateDecode;
    psStrContext->bIntraFrmAsClosedGop = psStrConfigData->bIntraFrmAsClosedGop;

    //psStrContext->ui32UserStrId = ui32UserStrId;

#if defined(BSPP_LOG) && !defined (BSPP_KM) && defined (BSPP_UM)
    {
        IMG_CHAR  hFileName[50];
        // if more than 1 stream is running then concatenate stream id in the log file name
        psStrContext->ui32UserStrId  > 0 ?  sprintf(hFileName,"%s_%08x.txt","log_new",psStrContext->ui32UserStrId) :  sprintf(hFileName,"log_new.txt");
        psStrContext->fpLog         =   fopen( hFileName ,"w");
        IMG_ASSERT( psStrContext->fpLog != NULL);

        fpLog = psStrContext->fpLog;
    }
#endif

    LST_init(&psStrContext->sGrpBstrCtx.sBufferChain);

    switch (psStrContext->eVidStd)
    {
    case VDEC_STD_H264:
        psStrContext->pfnParseUnit = BSPP_H264UnitParser;
        psStrContext->eEmulationPrevention = SWSR_EMPREVENT_00000300;
        psStrContext->sInterPictData.sH264Ctx.ui32ActiveSpsForSeiParsing = BSPP_INVALID;

        if (psStrContext->eBstrFormat == VDEC_BSTRFORMAT_DEMUX_BYTESTREAM ||
            psStrContext->eBstrFormat == VDEC_BSTRFORMAT_ELEMENTARY)
        {
            psStrContext->sSrConfig.eDelimType = SWSR_DELIM_SCP;
            psStrContext->sSrConfig.ui32DelimLength = 3 * 8;
            psStrContext->sSrConfig.ui64ScpValue = 0x000001;
        }
        else if (psStrContext->eBstrFormat == VDEC_BSTRFORMAT_DEMUX_SIZEDELIMITED)
        {
            psStrContext->sSrConfig.eDelimType = SWSR_DELIM_SIZE;
            psStrContext->sSrConfig.ui32DelimLength = 4 * 8;   // Set the default size-delimiter number of bits.
        }
        else
        {
            IMG_ASSERT(IMG_FALSE);
        }
        break;

    default:
        IMG_ASSERT(IMG_FALSE);
        ui32Result = IMG_ERROR_NOT_SUPPORTED;
        goto error;
    }

    // Allocate the memory required for this stream for Sequence/PPS info ...
    LST_init(&psStrContext->sStrAlloc.sAvailableSequencesList);

    psStrContext->pui8SequHdrInfo = (IMG_UINT8 *)IMG_SECURE_MALLOC(MAX_SEQUENCES_SECURE * sizeof(BSPP_sSequenceHdrInfo));
    IMG_ASSERT(psStrContext->pui8SequHdrInfo);
    if(psStrContext->pui8SequHdrInfo == IMG_NULL)
    {
        REPORT(REPORT_MODULE_BSPP, REPORT_ERR,
            "Failed to allocate memory for internal Sequence structure");

        ui32Result = IMG_ERROR_OUT_OF_MEMORY;
        goto error;
    }
    IMG_MEMSET(psStrContext->pui8SequHdrInfo, 0x00 , (MAX_SEQUENCES_SECURE * sizeof(BSPP_sSequenceHdrInfo)));

    psStrContext->pui8SecureSequenceInfo = (IMG_UINT8 *)IMG_SECURE_MALLOC(MAX_SEQUENCES_SECURE * BSPP_ParseSequSize[psStrContext->eVidStd]);

    IMG_ASSERT(psStrContext->pui8SecureSequenceInfo != IMG_NULL);
    if(psStrContext->pui8SecureSequenceInfo == IMG_NULL)
    {
        REPORT(REPORT_MODULE_BSPP, REPORT_ERR,
            "Failed to allocate memory for internal Sequence structure");

        ui32Result = IMG_ERROR_OUT_OF_MEMORY;
        goto error;
    }
    IMG_MEMSET(psStrContext->pui8SecureSequenceInfo, 0x00, (MAX_SEQUENCES_SECURE * BSPP_ParseSequSize[psStrContext->eVidStd]));

    psSequHdrInfo = (BSPP_sSequenceHdrInfo *)(psStrContext->pui8SequHdrInfo);
    // Only the first element is populated, all asFWSequence[i] use the same buffer
    IMG_ASSERT(asFWSequence[0].sDdBufInfo.pvCpuVirt != IMG_NULL);

    for (i = 0; i < MAX_SEQUENCES_SECURE; i++)
    {
        // Deal with the device memory for FW SPS data.
        psSequHdrInfo->sFWSequence = asFWSequence[i];
        psSequHdrInfo->sSequHdrInfo.ui32BufMapId = asFWSequence[i].sDdBufInfo.ui32BufMapId;
        psSequHdrInfo->sSequHdrInfo.ui32BufOffset = asFWSequence[i].ui32BufOffset;
		
		psSequHdrInfo->sFWSequence.sDdBufInfo.pvCpuVirt = asFWSequence[0].sDdBufInfo.pvCpuVirt;

        //No need to memset, it will be done before using it
        //IMG_MEMSET(psSequHdrInfo->sFWSequence.sDdBufInfo.pvCpuVirt, 0, psSequHdrInfo->sFWSequence.sDdBufInfo.ui32BufSize);

        psSequHdrInfo->hSecureSequenceInfo = (IMG_HANDLE)(psStrContext->pui8SecureSequenceInfo + (i * BSPP_ParseSequSize[psStrContext->eVidStd]));

        LST_add(&psStrContext->sStrAlloc.sAvailableSequencesList, psSequHdrInfo);
        psSequHdrInfo++;
    }

    if (psStrContext->eVidStd == VDEC_STD_H264)
    {
        IMG_UINT32 ui32ElementSize = 0;
        if(psStrContext->eVidStd == VDEC_STD_H264)
        {
            ui32ElementSize = sizeof(BSPP_sH264PPSInfo);
        }
        IMG_ASSERT(0 != ui32ElementSize);
        IMG_ASSERT(IMG_NULL != asFWPPS);
        LST_init(&psStrContext->sStrAlloc.sAvailablePPSsList);
        psStrContext->pui8PPSInfo = (IMG_UINT8 *)IMG_SECURE_MALLOC(MAX_PPSS_SECURE * sizeof(BSPP_sPPSInfo));
		
        IMG_ASSERT(psStrContext->pui8PPSInfo);
        if(psStrContext->pui8PPSInfo == IMG_NULL)
        {
            REPORT(REPORT_MODULE_BSPP, REPORT_ERR,
                "Failed to allocate memory for internal PPS structure");

            ui32Result = IMG_ERROR_OUT_OF_MEMORY;
            goto error;
        }
        IMG_MEMSET(psStrContext->pui8PPSInfo, 0x00, (MAX_PPSS_SECURE * sizeof(BSPP_sPPSInfo)));
        psStrContext->pui8SecurePPSInfo = (IMG_UINT8 *)IMG_SECURE_MALLOC(MAX_PPSS_SECURE * ui32ElementSize);

        IMG_ASSERT(psStrContext->pui8SecurePPSInfo != IMG_NULL);
        if(psStrContext->pui8SecurePPSInfo == IMG_NULL)
        {
            REPORT(REPORT_MODULE_BSPP, REPORT_ERR,
                "Failed to allocate memory for internal PPS structure");

            ui32Result = IMG_ERROR_OUT_OF_MEMORY;
            goto error;
        }
        IMG_MEMSET(psStrContext->pui8SecurePPSInfo, 0x00, (MAX_PPSS_SECURE * ui32ElementSize));

        psPPSInfo = (BSPP_sPPSInfo *)(psStrContext->pui8PPSInfo);
        // Only the first element is populated, all asFWPPS[i] use the same buffer
        IMG_ASSERT(asFWPPS[0].sDdBufInfo.pvCpuVirt != IMG_NULL);
        for (i = 0; i < MAX_PPSS_SECURE; i++)
        {
            // Deal with the device memory for FW PPS data.
            psPPSInfo->sFWPPS = asFWPPS[i];			
            psPPSInfo->ui32BufMapId = asFWPPS[i].sDdBufInfo.ui32BufMapId;
            psPPSInfo->ui32BufOffset = asFWPPS[i].ui32BufOffset;
            //No need to memset, it will be done before using it
            //IMG_MEMSET(psPPSInfo->sFWPPS.sDdBufInfo.pvCpuVirt, 0, psPPSInfo->sFWPPS.sDdBufInfo.ui32BufSize);

            /* We have no container for the PPS that passes down to the kernel, for this reason the h264 secure parser
               needs to populate that info into the picture header (Second)PictAuxData. */
            //ui32FWBufMapId = ((VDEC_sBufMapInfo *)(psPPSInfo->sFWPPS.hBufMapHandle))->ui32BufMapId;
            psPPSInfo->hSecurePPSInfo = (IMG_HANDLE)(psStrContext->pui8SecurePPSInfo + (i * ui32ElementSize));

			psPPSInfo->sFWPPS.sDdBufInfo.pvCpuVirt = asFWPPS[0].sDdBufInfo.pvCpuVirt;

            LST_add(&psStrContext->sStrAlloc.sAvailablePPSsList, psPPSInfo);
            psPPSInfo++;
        }
    }

    if(psStrContext->eVidStd == VDEC_STD_H264)
    {
       psStrContext->sStrAlloc.psSEIDataInfo = (BSPP_sH264SEIInfo *)IMG_BIGALLOC(sizeof(BSPP_sH264SEIInfo));

       IMG_ASSERT(psStrContext->sStrAlloc.psSEIDataInfo);
       if(psStrContext->sStrAlloc.psSEIDataInfo == IMG_NULL)
       {
           REPORT(REPORT_MODULE_BSPP, REPORT_ERR,
                  "Failed to allocate memory for internal SEI info structure");
           ui32Result = IMG_ERROR_OUT_OF_MEMORY;
           goto error;
       }
       IMG_MEMSET(psStrContext->sStrAlloc.psSEIDataInfo, 0, sizeof(BSPP_sH264SEIInfo));
    }

    // ... and initialise the lists that will use this data
    for (i=0; i<SEQUENCE_SLOTS_SECURE; i++)
    {
        LST_init(&psStrContext->sStrAlloc.asSequenceDataList[i]);
    }
    if (psStrContext->eVidStd == VDEC_STD_H264)
    {
        for (i=0; i<PPS_SLOTS_SECURE; i++)
        {
            LST_init(&psStrContext->sStrAlloc.asPPSDataList[i]);
        }
    }

    ui32Result = SEC_CreateMutex(&psStrContext->sMutex.hMutex);
    IMG_ASSERT(ui32Result == IMG_SUCCESS);

    // Initialise the software shift-register.
    SWSR_Initialise(bspp_ExceptionHandler,
                    &psStrContext->sParseCtx,
                    (SWSR_pfnCallback)bspp_ShiftRegCb,
                    &psStrContext->sGrpBstrCtx,
                    &psStrContext->hSwSrContext);

    // Setup the parse context.
    psStrContext->sParseCtx.hSwSrContext = psStrContext->hSwSrContext;

    *phStrContext = psStrContext;

    return IMG_SUCCESS;

error:
    if(psStrContext)
    {
        if(psStrContext->pui8SequHdrInfo)
        {
            IMG_SECURE_FREE(psStrContext->pui8SequHdrInfo);
        }

        if(psStrContext->pui8SecureSequenceInfo)
        {
            IMG_SECURE_FREE(psStrContext->pui8SecureSequenceInfo);
        }

        if(psStrContext->pui8PPSInfo)
        {
            IMG_SECURE_FREE(psStrContext->pui8PPSInfo);
        }

        if(psStrContext->pui8SecurePPSInfo)
        {
            IMG_SECURE_FREE(psStrContext->pui8SecurePPSInfo);
        }

        IMG_FREE(psStrContext);
    }
    return ui32Result;
}



