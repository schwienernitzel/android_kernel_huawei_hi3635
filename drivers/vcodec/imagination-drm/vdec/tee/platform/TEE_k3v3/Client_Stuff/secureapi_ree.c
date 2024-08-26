/*!
 *****************************************************************************
 *
 * @File       secureapi_ree.c
 * @Title      Secure Decode API REE.
 * @Description    This file contains sample code for communication with a Global
 *  Platform compatible TEE.
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
#include "secureapi_tee.h"
#ifdef ENABLE_LIN_SO_BUILD
#include "tee_internal_api.h"
#include "tee_client_type.h"
#include "tee_client_api.h"
#endif /* ENABLE_LIN_SO_BUILD */

typedef struct 
{
	TEEC_Context		sTEEContext;
	TEEC_Session		sTEESession;
	TEEC_SharedMemory	sMsgSharedMem;

	IMG_UINT32			ui32SecureId;

} sTEECommsContext;

static sTEECommsContext g_TEECommsContext;

#define IMG_MAX_MESSAGE_SIZE (20*1024)		//Needs to be set correctly

static TEEC_UUID uuidIMG_TA;			//UUID that identifies our component, needs to be initialised

/*!
******************************************************************************

 @Function              SECURE_REE_GetId

 @Description

 This function establishes communications with a secure driver and returns a handle to allow commands to be sent to the driver

 @Input     nCore				: The core to establish secure communications with

 @Output    pui32SecureId          : This function returns a Secure ID if successful, or null otherwise.

 @Return    This function returns either IMG_SUCCESS or an
                              error code.

******************************************************************************/

IMG_RESULT SECURE_REE_GetId(IMG_VIDEO_CORE nCore, IMG_UINT32 *pui32SecureId)
{
	IMG_RESULT			rtn = IMG_SUCCESS;

	TEEC_Result			result;
	TEEC_Operation		operation;

    //InitializeContext
    result = TEEC_InitializeContext(NULL, &g_TEECommsContext.sTEEContext);

    if(result != TEEC_SUCCESS) 
	{
        TEEC_Error("teec initial failed\n");

		return IMG_ERROR_FATAL;
    }

    //Allocate a shared memory
    g_TEECommsContext.sMsgSharedMem.buffer		= NULL;
    g_TEECommsContext.sMsgSharedMem.size		= IMG_MAX_MESSAGE_SIZE;
    g_TEECommsContext.sMsgSharedMem.flags		= TEEC_MEM_INOUT;

    result = TEEC_AllocateSharedMemory(&g_TEECommsContext.sTEEContext, &g_TEECommsContext.sMsgSharedMem);

    if(result != TEEC_SUCCESS) 
	{
        TEEC_Error("allocate failed\n");

		rtn = IMG_ERROR_OUT_OF_MEMORY;
        goto error_badSharedMem;
    }

    //OpenSession
	//ParamType 1,   TEE_PARAM_TYPE_VALUE_INPUT,       BSPP = BITSTREAM_SCAN, VXD_IO = VXD_CORE0 | VXD_CORE1
	//ParamType 2,   TEE_PARAM_TYPE_VALUE_OUTPUT,      Secure ID
	//ParamType 3,   TEE_PARAM_TYPE_VALUE_OUTPUT,      IMG_RESULT, return code from SECURE_GetID()
	//ParamType 4,   TEE_PARAM_TYPE_NONE,              not used

	operation.started    = 1;
    operation.paramTypes = TEEC_PARAM_TYPES(TEE_PARAM_TYPE_VALUE_INPUT,
                                            TEE_PARAM_TYPE_VALUE_OUTPUT,
                                            TEE_PARAM_TYPE_VALUE_OUTPUT,
                                            TEE_PARAM_TYPE_NONE);

    operation.params[0].value.a = nCore;
   
	result = TEEC_OpenSession(&g_TEECommsContext.sTEEContext, &g_TEECommsContext.sTEESession, &uuidIMG_TA, TEEC_LOGIN_PUBLIC, NULL, &operation, NULL);

    if(result != TEEC_SUCCESS) 
	{
        TEEC_Error("teec open session failed\n");

		rtn = IMG_ERROR_FATAL;
        goto error_badSession;
    }

	g_TEECommsContext.ui32SecureId = operation.params[1].value.a;

	*pui32SecureId = g_TEECommsContext.ui32SecureId;

	return(rtn);

error_badSession:
	 TEEC_ReleaseSharedMemory(&g_TEECommsContext.sMsgSharedMem);

error_badSharedMem:
	TEEC_FinalizeContext(&g_TEECommsContext.sTEEContext);

	return(rtn);
}

/*!
******************************************************************************

 @Function              SECURE_REE_ReleaseId

 @Description

 Releases a reference to a secure driver communications channel

******************************************************************************/

IMG_RESULT SECURE_REE_ReleaseId(IMG_UINT32 ui32SecureId)
{
	//The TA function to close session calls SECURE_TEE_ReleaseHandle() - it knows the handle
	TEEC_CloseSession(&g_TEECommsContext.sTEESession);

	TEEC_ReleaseSharedMemory(&g_TEECommsContext.sMsgSharedMem);

	TEEC_FinalizeContext(&g_TEECommsContext.sTEEContext);
	
	return(IMG_SUCCESS);
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
	//Transfer message to shared comms buffer
	if(size > g_TEECommsContext.sMsgSharedMem.size)
	{
		TEEC_Error("teec shared buffer too small\n");

		return(NULL);
	}

	return(g_TEECommsContext.sMsgSharedMem.buffer);
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
	return(IMG_SUCCESS);
}


/*!
******************************************************************************

 @Function              SECURE_REE_SendMessage

 @Description

******************************************************************************/

IMG_RESULT SECURE_REE_SendMessage(
    IMG_UINT32 ui32SecureId,
    IMG_BYTE* pbyMsg,
    IMG_UINT16 ui16Size,
    MSG_ENDPOINT nEndpoint
)
{
	return SECURE_REE_SendMessageWithBuf(ui32SecureId, pbyMsg, ui16Size, nEndpoint, NULL, 0);
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
	TEEC_Result			result;
	TEEC_Operation		operation;
    TEEC_SharedMemory   optBufSharedMem = { 0 };
	uint32_t			origin;
	
	//Transfer message to shared comms buffer
	if(ui16Size > g_TEECommsContext.sMsgSharedMem.size)
	{
		TEEC_Error("teec shared buffer too small\n");

		return(IMG_ERROR_FATAL);
	}

	if(pbyBuf)
	{
		//There is also a buffer to pass to the TEE along with the message,
        //allocate a shared chunk
        optBufSharedMem.buffer		= NULL;
        optBufSharedMem.size		= ui32BufSize;
        optBufSharedMem.flags		= TEEC_MEM_INOUT;

        result = TEEC_AllocateSharedMemory(&g_TEECommsContext.sTEEContext, &optBufSharedMem);

        if(result != TEEC_SUCCESS) 
        {
            TEEC_Error("optional buffer allocate failed\n");

            return(IMG_ERROR_OUT_OF_MEMORY);
        }
        IMG_MEMCPY(optBufSharedMem.buffer, pbyBuf, ui32BufSize);
    }

	IMG_MEMCPY(g_TEECommsContext.sMsgSharedMem.buffer, pbyMsg, ui16Size);

	//Send Message to the TEE
	//ParamType 1,   TEE_PARAM_TYPE_VALUE_INPUT,   IMG_RESULT, return code from SECURE_GetId()
	//ParamType 2,   TEE_PARAM_TYPE_MEMREF_INOUT,  Message buffer (input/output)
	//ParamType 3,   TEE_PARAM_TYPE_VALUE_OUTPUT,  IMG_RESULT, return code from SECURE_SendMessage()
	//[ParamType 4], Optional parameter must be TEEC_MEMREF_TEMP_OUTPUT/INPUT/INOUT

	operation.started    = 1;
    operation.paramTypes = TEEC_PARAM_TYPES(TEE_PARAM_TYPE_VALUE_INPUT,
                                            TEE_PARAM_TYPE_MEMREF_INOUT,
                                            TEE_PARAM_TYPE_VALUE_OUTPUT,
                                            TEE_PARAM_TYPE_NONE);

	operation.params[0].value.a = g_TEECommsContext.ui32SecureId;

	operation.params[1].tmpref.buffer = g_TEECommsContext.sMsgSharedMem.buffer;
	operation.params[1].tmpref.size   = ui16Size;

	if(pbyBuf)
	{
		//There is also a buffer to pass to the TEE along with the message,
        //it was already copied to a shared area
		operation.paramTypes = TEEC_PARAM_TYPES(TEE_PARAM_TYPE_VALUE_INPUT,
                                                TEE_PARAM_TYPE_MEMREF_INOUT,
                                                TEE_PARAM_TYPE_VALUE_OUTPUT,
                                                TEE_PARAM_TYPE_MEMREF_INOUT);

		operation.params[3].tmpref.buffer	= optBufSharedMem.buffer;
		operation.params[3].tmpref.size		= ui32BufSize;
	}

	result = TEEC_InvokeCommand(&g_TEECommsContext.sTEESession, nEndpoint, &operation, &origin);

    if (optBufSharedMem.buffer)
    {
        IMG_MEMCPY(pbyBuf, optBufSharedMem.buffer, ui32BufSize);
        TEEC_ReleaseSharedMemory(&optBufSharedMem);
    }

	if(result != TEEC_SUCCESS) 
	{
		//TODO - display error messages, check origin also!!!
        TEEC_Error("teec command failed\n");

		return(IMG_ERROR_FATAL);
	}

	IMG_MEMCPY(pbyMsg, g_TEECommsContext.sMsgSharedMem.buffer, ui16Size);

 	return(IMG_SUCCESS);
}

/*!
******************************************************************************

 @Function              SECURE_REE_ReleaseGlobalMsgBuffers

 @Description

 Release global message buffers if any. This is just temporary and will be deprecated.

 @Return    This function returns either IMG_SUCCESS or an
                              error code.

******************************************************************************/
IMG_RESULT SECURE_REE_ReleaseGlobalMsgBuffers(IMG_VOID)
{
	return(IMG_SUCCESS);
}
