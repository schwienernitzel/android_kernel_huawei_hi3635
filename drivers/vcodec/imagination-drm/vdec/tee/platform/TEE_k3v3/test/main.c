/*!
 *****************************************************************************
 *
 * @File       main.c
 * @Title      Test of the TEE reference code implementation.
 * @Description    This file contains simple testcase for the TEE reference code
 *  based on Global Platform TEE framework.
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


#include <stdlib.h>
#include "img_types.h"
#include "secure_defs.h"
#include "secureapi_ree.h"

#define CHECK_RESULT(_res_, _status_) \
    do { \
        if (_res_ != IMG_SUCCESS) { \
            fprintf(stderr, "%s failed: %d\n", _status_, _res_); \
            exit(_res_ % 255); \
        } \
    } while (0)

#define MSG_SIZE 100
#define OPT_BUF_SIZE 20

/*!
******************************************************************************

 @Function              main

******************************************************************************/
int main()
{
    IMG_RESULT   ui32Res;
    IMG_UINT32   ui32CtxId;
    IMG_BYTE   * pbyMsg;
    IMG_BYTE     abyOptBuf[OPT_BUF_SIZE];
    IMG_UINT32   ui32i;

    /* Initialize optional buffer with dummy data */
    for (ui32i = 0; ui32i < OPT_BUF_SIZE; ui32i++)
    {
        abyOptBuf[ui32i] = ui32i;
    }

    /* Initialize context */
    ui32Res = SECURE_REE_GetId(0, &ui32CtxId);
    CHECK_RESULT(ui32Res, "SECURE_REE_GetId");

    /* Get msg buffer and fill it with dummy data */
    pbyMsg = SECURE_REE_GetMsgBuffer(ui32CtxId, MSG_SIZE);
    if (pbyMsg == IMG_NULL)
    {
        fprintf(stderr, "SECURE_REE_GetMsgBuffer failed"); \
        exit(1);
    }
    memset(pbyMsg, 0xda, MSG_SIZE);

    /* Send the message */
    ui32Res = SECURE_REE_SendMessage(ui32CtxId, pbyMsg, MSG_SIZE,
                                     ENDPOINT_BSPP_STREAM_CREATE);
    CHECK_RESULT(ui32Res, "SECURE_REE_SendMessage");

    /* Send the message with an optional buffer */
    memset(pbyMsg, 0x19, MSG_SIZE);
    ui32Res = SECURE_REE_SendMessageWithBuf(ui32CtxId, pbyMsg, MSG_SIZE,
                                            ENDPOINT_VXD_PREPARE_FIRMWARE,
                                            abyOptBuf, OPT_BUF_SIZE);
    CHECK_RESULT(ui32Res, "SECURE_REE_SendMessageWithBuf");

    /* Release message buffer */
    ui32Res = SECURE_REE_ReleaseMsgBuffer(ui32CtxId, pbyMsg);
    CHECK_RESULT(ui32Res, "SECURE_REE_ReleaseMsgBuffer");
    pbyMsg = IMG_NULL;

    /* Destroy context */
    ui32Res = SECURE_REE_ReleaseId(ui32CtxId);
    CHECK_RESULT(ui32Res, "SECURE_REE_ReleaseId");

    return ui32Res;
}

/*!
******************************************************************************

 Functions that are required by libtee_k3v3.so follow

******************************************************************************/

/*!
******************************************************************************

 @Function              SECURE_ReleaseId

******************************************************************************/
IMG_RESULT SECURE_ReleaseId(
    IMG_UINT32 ui32SecureId
)
{
    printf("%s:%d releasing id 0x%x\n", __func__, __LINE__, ui32SecureId);
    return IMG_SUCCESS;
}

/*!
******************************************************************************

 @Function              SECURE_GetId

******************************************************************************/
IMG_RESULT SECURE_GetId(
    IMG_VIDEO_CORE nCore,
    IMG_UINT32    *pui32SecureId
)
{
    static IMG_UINT32 ui32Id = 0x1d1d0000;
    *pui32SecureId = ui32Id++;
    printf("%s:%d returning id: 0x%x\n", __func__, __LINE__, *pui32SecureId);
    return IMG_SUCCESS;
}

/*!
******************************************************************************

 @Function              getEndPointName

******************************************************************************/
static IMG_CHAR * getEndPointName(MSG_ENDPOINT nEndpoint)
{
    switch (nEndpoint)
    {
        case ENDPOINT_VXD_INPUT:
            return "ENDPOINT_VXD_INPUT";
        case ENDPOINT_BSPP_STREAM_CREATE:
            return "ENDPOINT_BSPP_STREAM_CREATE";
        case ENDPOINT_BSPP_STREAM_DESTROY:
            return "ENDPOINT_BSPP_STREAM_DESTROY";
        case ENDPOINT_BSPP_SUBMIT_PICTURE_DECODED:
            return "ENDPOINT_BSPP_SUBMIT_PICTURE_DECODED";
        case ENDPOINT_BSPP_STREAM_SUBMIT_BUFFER:
            return "ENDPOINT_BSPP_STREAM_SUBMIT_BUFFER";
        case ENDPOINT_BSPP_STREAM_PREPARSE_BUFFERS:
            return "ENDPOINT_BSPP_STREAM_PREPARSE_BUFFERS";
        case ENDPOINT_VXD_INITIALISE:
            return "ENDPOINT_VXD_INITIALISE";
        case ENDPOINT_VXD_DEINITIALISE:
            return "ENDPOINT_VXD_DEINITIALISE";
        case ENDPOINT_VXD_RESET:
            return "ENDPOINT_VXD_RESET";
        case ENDPOINT_VXD_HANDLE_INTERRUPTS:
            return "ENDPOINT_VXD_HANDLE_INTERRUPTS";
        case ENDPOINT_VXD_GET_STATE:
            return "ENDPOINT_VXD_GET_STATE";
        case ENDPOINT_VXD_PREPARE_FIRMWARE:
            return "ENDPOINT_VXD_PREPARE_FIRMWARE";
        case ENDPOINT_VXD_LOAD_CORE_FW:
            return "ENDPOINT_VXD_LOAD_CORE_FW";
        case ENDPOINT_VXD_READN_REGS:
            return "ENDPOINT_VXD_READN_REGS";
    }

    return "UNKNOWN";
}

/*!
******************************************************************************

 @Function              dumpBuf

******************************************************************************/
static IMG_VOID dumpBuf(
    IMG_UINT8 *pui8Buf,
    IMG_UINT32 ui32BufSize
)
{
    if (pui8Buf != IMG_NULL && ui32BufSize != 0)
    {
        IMG_UINT32 ui32i;
        for (ui32i = 0; ui32i < ui32BufSize - 4; ui32i += 4)
        {
            printf("    0x%08x: 0x%02x%02x%02x%02x\n",
                   ui32i, pui8Buf[ui32i], pui8Buf[ui32i + 1],
                   pui8Buf[ui32i + 2], pui8Buf[ui32i + 3]);
        }
    }
}

/*!
******************************************************************************

 @Function              SECURE_SendMessage

******************************************************************************/
IMG_RESULT SECURE_SendMessage(
    IMG_UINT32 ui32SecureId,
    IMG_BYTE *pbyMsg,
    IMG_UINT16 ui16Size,
    MSG_ENDPOINT nEndpoint,
    IMG_BYTE *pbyBuf,
    IMG_UINT32 ui32BufSize
)
{
    printf("%s:%d got msg for CTX 0x%x, endpoint: %s\n",
           __func__, __LINE__, ui32SecureId, getEndPointName(nEndpoint));

    printf("dumping message:\n");
    dumpBuf(pbyMsg, ui16Size);

    if (pbyBuf != IMG_NULL && ui32BufSize != 0)
    {
        printf("dumping optional buffer:\n");
        dumpBuf(pbyBuf, ui32BufSize);
    }

    return IMG_SUCCESS;
}

