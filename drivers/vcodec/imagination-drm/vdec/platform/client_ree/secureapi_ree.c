/*!
 *****************************************************************************
 *
 * @File       secureapi_ree.c
 * @Title      *       Secure Decode API REE.
 * @Description    This file contains sample code for communication with a Global
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



#include "secureapi_ree.h"
#include "sysos_api_km.h"

#include "teek_client_api.h"
#include "teek_client_id.h"
#include "teek_client_type.h"
#include "teek_ns_client.h"
#include "teek_client_list.h"
#include "report_api.h"
#include <asm/memory.h>

#define SECURE_REE "SECURE_REE"

extern void configure_master_security(unsigned int is_security, int master_id);

//#define DEBUG_TEE_LOCK
#define USE_PHY_ADDR 1

typedef struct 
{
    TEEC_Context        sTEEContext;
    TEEC_Session        sTEESession;
    TEEC_SharedMemory   sTEESharedMemory;
    IMG_UINT32          ui32SecureId;
} sTEECommsContext;

static sTEECommsContext g_TEECommsContext = {0};

static IMG_HANDLE hMutex;

#define IMG_MAX_MESSAGE_SIZE (64*1024)

static TEEC_UUID uuidIMG_TA = {
            0x0A0A0A0A, 0x0A0A, 0x0A0A,
                        { 0x0A, 0x0A, 0x0A, 0x0A, 0x0A, 0x0A, 0x0A, 0x0A }
};

static IMG_CHAR * getEndPointName(MSG_ENDPOINT nEndpoint) {                                                                         
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
     // TODO defualt
    }

    return "UNKNOWN";
}


static IMG_RESULT send_message_with_buffer(
    IMG_UINT32          ui32SecureId,
    IMG_BYTE          * pbyMsg,
    IMG_UINT16          ui16Size,
    MSG_ENDPOINT        nEndpoint,
    IMG_BYTE          * pbyBuf,
    IMG_UINT32          ui32BufSize)
{    
    TEEC_Result         result    = 0;
    TEEC_Operation      operation = {0};
    uint32_t            origin    = 0;
    uint8_t            *pu8Buf    = NULL;
    uint8_t            *pu8Msg    = NULL;
    IMG_UINT64          phy       = 0;
    DEBUG_REPORT(SECURE_REE, "%s:%d ui16Size %d ui32BufSize %d", __func__, __LINE__, ui16Size, ui32BufSize);

    if (ui16Size + ui32BufSize > g_TEECommsContext.sTEESharedMemory.size)
    {
        TEEC_Error(SECURE_REE, REPORT_ERR, "teec shared buffer too small. buffer %d ui16Size %d ui32BufSize %d\n",
            g_TEECommsContext.sTEESharedMemory.size, ui16Size, ui32BufSize);
        return IMG_ERROR_FATAL;
    }

    if (NULL == pbyMsg)
    {
        TEEC_Error("pbyMsg is null point\n");
        return IMG_ERROR_FATAL;
    }

    if (NULL == g_TEECommsContext.sTEESharedMemory.buffer)
    {
        TEEC_Error("sTEESharedMemory.buffer is null point\n");
        return IMG_ERROR_FATAL;
    }

    pu8Msg = g_TEECommsContext.sTEESharedMemory.buffer;

    IMG_MEMCPY(pu8Msg, pbyMsg, ui16Size);
    if (pbyBuf)
    {
        pu8Buf = &pu8Msg[ui16Size];
        IMG_MEMCPY(pu8Buf, pbyBuf, ui32BufSize);
    }

#if USE_PHY_ADDR
    phy = virt_to_phys((void *)pu8Msg);
#endif
    //Send Message to the TEE
    //ParamType 1,   TEEC_PARAM_TYPE_VALUE_INPUT,   IMG_RESULT, return code from SECURE_GetId()
    //ParamType 2,   TEEC_PARAM_TYPE_MEMREF_INOUT,  Message buffer (input/output)
    //ParamType 3,   TEEC_PARAM_TYPE_VALUE_OUTPUT,  IMG_RESULT, return code from SECURE_SendMessage()
    //[ParamType 4], Optional parameter must be TEEC_MEMREF_TEMP_OUTPUT/INPUT/INOUT
    memset(&operation, 0, sizeof(TEEC_Operation));
    operation.started    = 1;
    operation.paramTypes = TEEC_PARAM_TYPES(TEEC_VALUE_INPUT,
                                            TEEC_VALUE_INOUT,
                                            TEEC_VALUE_OUTPUT,
                                            TEEC_NONE);
    operation.params[0].value.a = g_TEECommsContext.ui32SecureId;
#if USE_PHY_ADDR
    operation.params[1].value.a = phy;
#else
    operation.params[1].value.a = virt_to_phys((void *)pu8Msg);
#endif
    operation.params[1].value.b = ui16Size;
    if (pbyBuf)
    {
        DEBUG_REPORT(SECURE_REE, "%s:%d", __func__, __LINE__);
        //There is also a buffer to pass to the TEE along with the message,
        //it was already copied to a shared area
        operation.paramTypes = TEEC_PARAM_TYPES(TEEC_VALUE_INPUT,
                                                TEEC_VALUE_INOUT,
                                                TEEC_VALUE_OUTPUT,
                                                TEEC_VALUE_INOUT);
#if USE_PHY_ADDR
        operation.params[3].value.a = phy + ui16Size;
#else
        operation.params[3].value.a = virt_to_phys((void *)pu8Buf);
#endif
        operation.params[3].value.b = ui32BufSize;
    }

    DEBUG_REPORT(SECURE_REE, "%s:%d session_id : %d", __func__, __LINE__, g_TEECommsContext.sTEESession.session_id);
    DEBUG_REPORT(SECURE_REE, "%s:%d nEndpoint : %d, operation.paramTypes : %d", __func__, __LINE__, nEndpoint, operation.paramTypes);
    result = TEEK_InvokeCommand(&(g_TEECommsContext.sTEESession), nEndpoint, &operation, &origin);
    if (result != TEEC_SUCCESS) 
    {
        // TODO Something 
        TEEC_Error("HISI CA Invoke failed return : %d, origin : %d\n", result, origin);
        return IMG_ERROR_FATAL;
    }

    IMG_MEMCPY(pbyMsg, pu8Msg, ui16Size);
    if (pbyBuf)
    {
        IMG_MEMCPY(pbyBuf, pu8Buf, ui32BufSize);
    }

    return IMG_SUCCESS;
}


static IMG_RESULT call_send_message_with_buffer(
    IMG_UINT32          ui32SecureId,
    IMG_BYTE          * pbyMsg,
    IMG_UINT16          ui16Size,
    MSG_ENDPOINT        nEndpoint,
    IMG_BYTE          * pbyBuf,
    IMG_UINT32          ui32BufSize)
{ 
    IMG_RESULT rtn = IMG_SUCCESS;

#ifdef DEBUG_TEE_LOCK
    REPORT(SECURE_REE, REPORT_INFO, "%s pid %d nEndpoint %s TEELOCK WAIT", __func__, current->pid, getEndPointName(nEndpoint));
#endif
    SYSOSKM_LockMutex(hMutex);
#ifdef DEBUG_TEE_LOCK
    REPORT(SECURE_REE, REPORT_INFO, "%s pid %d nEndpoint %s TEELOCK ON", __func__, current->pid, getEndPointName(nEndpoint));
#endif

    rtn = send_message_with_buffer(ui32SecureId, pbyMsg, ui16Size, nEndpoint, pbyBuf, ui32BufSize);

#ifdef DEBUG_TEE_LOCK
    REPORT(SECURE_REE, REPORT_INFO, "%s pid %d nEndpoint %s TEELOCK RELEASE", __func__, current->pid, getEndPointName(nEndpoint));
#endif
    SYSOSKM_UnlockMutex(hMutex);
#ifdef DEBUG_TEE_LOCK
    REPORT(SECURE_REE, REPORT_INFO, "%s pid %d nEndpoint %s TEELOCK OFF", __func__, current->pid, getEndPointName(nEndpoint));
#endif

    return rtn;
}

#define PROBE_VALUE 1001
IMG_BOOL probe_session_is_exit()
{
    TEEC_Operation      operation = {0};
    TEEC_Result         result    = 0;
    uint32_t            origin    = 0;

    memset(&operation, 0, sizeof(TEEC_Operation));
    operation.started    = 1;
    operation.paramTypes = TEEC_PARAM_TYPES(TEEC_VALUE_INOUT,
                                            TEEC_NONE,
                                            TEEC_NONE,
                                            TEEC_NONE);

    operation.params[0].value.a = PROBE_VALUE;
    result = TEEK_InvokeCommand(&(g_TEECommsContext.sTEESession), 0, &operation, &origin);
    if (result != TEEC_SUCCESS) 
    {
        TEEC_Error("Probe session failed return : %d, origin : %d\n", result, origin);
        return IMG_FALSE;
    }

    return IMG_TRUE;
}

void close_ta_session()
{
    DEBUG_REPORT(SECURE_REE, "%s:%d", __func__, __LINE__);

    //The TA function to close session calls SECURE_TEE_ReleaseHandle() - it knows the handle
    TEEK_CloseSession(&g_TEECommsContext.sTEESession);

    g_TEECommsContext.ui32SecureId = 0;

    TEEK_FinalizeContext(&g_TEECommsContext.sTEEContext);
}

/*!
******************************************************************************

 @Function              SECURE_REE_GetId

 @Description

 This function establishes communications with a secure driver and returns a handle to allow commands to be sent to the driver

 @Input     nCore               : The core to establish secure communications with

 @Output    pui32SecureId          : This function returns a Secure ID if successful, or null otherwise.

 @Return    This function returns either IMG_SUCCESS or an
                              error code.

******************************************************************************/

IMG_RESULT SECURE_REE_GetId(IMG_VIDEO_CORE nCore, IMG_UINT32 *pui32SecureId)
{
    IMG_RESULT     rtn       = IMG_SUCCESS;
    TEEC_Result    result    = 0;
    TEEC_Operation operation = {0};

    DEBUG_REPORT(SECURE_REE, "%s:%d", __func__, __LINE__);

    if (g_TEECommsContext.ui32SecureId != 0)
    {
        IMG_BOOL isConnected = probe_session_is_exit();
        if (isConnected)
        {
            DEBUG_REPORT(SECURE_REE, "%s:%d Session have been init", __func__, __LINE__);
            *pui32SecureId = g_TEECommsContext.ui32SecureId;
            return IMG_SUCCESS;
        }
        else
        {
            close_ta_session();
            DEBUG_REPORT(SECURE_REE, "Session is aborted, we will connect again~");
        }
    }

    //InitializeContext
    result = TEEK_InitializeContext(NULL, &(g_TEECommsContext.sTEEContext));
    if (result != TEEC_SUCCESS) 
    {
        TEEC_Error("teec InitializeContext failed");
        return IMG_ERROR_FATAL;
    }
    else
    {
        // TODO success, need print something
        DEBUG_REPORT(SECURE_REE, "%s:%d, Context init success !", __func__, __LINE__);
    }

    if (g_TEECommsContext.sTEESharedMemory.buffer != NULL) 
    {
        memset(g_TEECommsContext.sTEESharedMemory.buffer, 0, g_TEECommsContext.sTEESharedMemory.size);
    }
    else
    {
        rtn = IMG_ERROR_FATAL;
        goto error_badSharedMem;   
    }


    memset(&operation, 0, sizeof(TEEC_Operation));
    operation.started    = 1;
    operation.paramTypes = TEEC_PARAM_TYPES(TEEC_VALUE_INPUT,
                                            TEEC_VALUE_OUTPUT,
                                            TEEC_VALUE_OUTPUT,
                                            TEEC_NONE);

    operation.params[0].value.a = nCore;
   
    DEBUG_REPORT(SECURE_REE, "%s:%d, paramTypes : 0x%x", __func__, __LINE__, operation.paramTypes);
    result = TEEK_OpenSession(&g_TEECommsContext.sTEEContext,
                              &g_TEECommsContext.sTEESession, &uuidIMG_TA, TEEC_LOGIN_PUBLIC, NULL, &operation, NULL);

    if (result != TEEC_SUCCESS) 
    {
        TEEC_Error("teec open session failed");
        rtn = IMG_ERROR_FATAL;
        goto error_badSession;
    }
    else
    {
        DEBUG_REPORT(SECURE_REE, "%s:%d, session_id : %d", __func__, __LINE__, g_TEECommsContext.sTEESession.session_id);
    }

    g_TEECommsContext.ui32SecureId = operation.params[1].value.a;
    DEBUG_REPORT(SECURE_REE, "%s:%d ui32SecureId : %d", __func__, __LINE__, g_TEECommsContext.ui32SecureId);

    *pui32SecureId = g_TEECommsContext.ui32SecureId;

    return (rtn);

error_badSession:
    // TODO Something
error_badSharedMem:
    TEEK_FinalizeContext(&g_TEECommsContext.sTEEContext);

    return (rtn);
}

/*!
******************************************************************************

 @Function              SECURE_REE_ReleaseId

 @Description

 Releases a reference to a secure driver communications channel


******************************************************************************/

IMG_RESULT SECURE_REE_ReleaseId(IMG_UINT32 ui32SecureId)
{
    DEBUG_REPORT(SECURE_REE, "%s:%d", __func__, __LINE__);

    //The TA function to close session calls SECURE_TEE_ReleaseHandle() - it knows the handle
    //TEEK_CloseSession(&g_TEECommsContext.sTEESession);

    //g_TEECommsContext.ui32SecureId = 0;

    //TEEK_FinalizeContext(&g_TEECommsContext.sTEEContext);

    return (IMG_SUCCESS);
}

/*!
******************************************************************************

 @Function              SECURE_REE_GetMsgBuffer

 @Description

 Provides shared memory for buffer.

 @Input     ui32SecureId     : Secure ID

 @Input     ui32Size         : Size of requested buffer

 @Return    Returns pointer to start of the allocated block.

******************************************************************************/

IMG_VOID * SECURE_REE_GetMsgBuffer(
    IMG_UINT32 ui32SecureId,
    IMG_UINT32 size,
    MSG_ENDPOINT nEndpoint)
{
    DEBUG_REPORT(SECURE_REE, "%s:%d", __func__, __LINE__);
    //Transfer message to shared comms buffer
    if (size > g_TEECommsContext.sTEESharedMemory.size)
    {
        TEEC_Error("teec shared buffer too small\n");
        return (NULL);
    }

#ifdef DEBUG_TEE_LOCK
    REPORT(SECURE_REE, REPORT_INFO, "%s pid %d nEndpoint %s TEELOCK WAIT", __func__, current->pid, getEndPointName(nEndpoint));
#endif
    SYSOSKM_LockMutex(hMutex);
#ifdef DEBUG_TEE_LOCK
    REPORT(SECURE_REE, REPORT_INFO, "%s pid %d nEndpoint %s TEELOCK ON", __func__, current->pid, getEndPointName(nEndpoint));
#endif
    return g_TEECommsContext.sTEESharedMemory.buffer;
}

/*!
******************************************************************************

 @Function              SECURE_REE_ReleaseMsgBuffer

 @Description

 Provides shared memoryu

 @Input     ui32SecureId     : Secure ID

 @Input     pMsgBuff         : Buffer to release.

 @Return    This function returns either IMG_SUCCESS or an
                              error code.

******************************************************************************/

IMG_RESULT SECURE_REE_ReleaseMsgBuffer(IMG_UINT32 ui32SecureId, IMG_VOID  *pMsgBuff)
{
#ifdef DEBUG_TEE_LOCK
    REPORT(SECURE_REE, REPORT_INFO, "%s pid %d TEELOCK RELEASE", __func__, current->pid);
#endif
    SYSOSKM_UnlockMutex(hMutex);
#ifdef DEBUG_TEE_LOCK
    REPORT(SECURE_REE, REPORT_INFO, "%s pid %d TEELOCK OFF", __func__, current->pid);
#endif
    return (IMG_SUCCESS);
}


/*!
******************************************************************************

 @Function              SECURE_REE_SendMessage

 @Description

******************************************************************************/

IMG_RESULT SECURE_REE_SendMessage(
    IMG_UINT32      ui32SecureId,
    IMG_BYTE*       pbyMsg,
    IMG_UINT16      ui16Size,
    MSG_ENDPOINT    nEndpoint)
{
    DEBUG_REPORT(SECURE_REE,
                 "%s:%d ui32SecureId : %d, ui16Size : %d, nEndpoint [%d] : [%s]", __func__, __LINE__,
                 ui32SecureId, ui16Size, nEndpoint, getEndPointName(nEndpoint));
    return call_send_message_with_buffer(ui32SecureId, pbyMsg, ui16Size, nEndpoint, NULL, 0);
}


/*!
******************************************************************************

 @Function              SECURE_REE_SendMessageWithBuf

 @Description

******************************************************************************/

IMG_RESULT SECURE_REE_SendMessageWithBuf(
    IMG_UINT32          ui32SecureId,
    IMG_BYTE          * pbyMsg,
    IMG_UINT16          ui16Size,
    MSG_ENDPOINT        nEndpoint,
    IMG_BYTE          * pbyBuf,
    IMG_UINT32          ui32BufSize)
{    
    DEBUG_REPORT(SECURE_REE,
                 "%s:%d ui32SecureId : %d, ui16Size : %d, nEndpoint [%d] : [%s], ui32BufSize[%d]", __func__, __LINE__,
                 ui32SecureId, ui16Size,  nEndpoint, getEndPointName(nEndpoint), ui32BufSize);
    return call_send_message_with_buffer(ui32SecureId, pbyMsg, ui16Size, nEndpoint, pbyBuf, ui32BufSize);
}

IMG_VOID SECURE_REE_Init(IMG_VOID)
{
    SYSOSKM_CreateMutex(&hMutex);

    g_TEECommsContext.sTEESharedMemory.buffer = kmalloc(IMG_MAX_MESSAGE_SIZE, GFP_KERNEL);
    g_TEECommsContext.sTEESharedMemory.size   = IMG_MAX_MESSAGE_SIZE;
    g_TEECommsContext.sTEESharedMemory.flags  = TEEC_MEM_INOUT;
    if (g_TEECommsContext.sTEESharedMemory.buffer == NULL)
    {
        REPORT(SECURE_REE, REPORT_ERR, "allocate buffer failed");
        TEEC_Error("allocate buffer failed\n");
        return;
    }

    memset(g_TEECommsContext.sTEESharedMemory.buffer, 0, g_TEECommsContext.sTEESharedMemory.size);

    REPORT(SECURE_REE, REPORT_ERR,"%s msg data %d bytes", __func__, g_TEECommsContext.sTEESharedMemory.size);

    g_TEECommsContext.ui32SecureId = 0;
}

IMG_VOID SECURE_REE_DeInit(IMG_VOID)
{
    if (g_TEECommsContext.sTEESharedMemory.buffer)
    {
        kfree(g_TEECommsContext.sTEESharedMemory.buffer);
        g_TEECommsContext.sTEESharedMemory.buffer = NULL;
        g_TEECommsContext.sTEESharedMemory.size   = 0;
    }

    SYSOSKM_DestroyMutex(hMutex); // TODO need to be protected
}

IMG_VOID SECURE_REE_StreamCreate(IMG_VOID)
{
    DEBUG_REPORT(SECURE_REE, "%s:%d", __func__, __LINE__);
    configure_master_security(1, 0);
}

IMG_VOID SECURE_REE_StreamDestroy(IMG_VOID)
{
    DEBUG_REPORT(SECURE_REE, "%s:%d", __func__, __LINE__);
    configure_master_security(0, 0);
}

