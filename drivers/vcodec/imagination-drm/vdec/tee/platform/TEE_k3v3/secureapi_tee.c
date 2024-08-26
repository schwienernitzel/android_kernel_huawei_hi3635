/*!
 *****************************************************************************
 *
 * @File       secureapi_tee.c
 * @Title      Secure Decode API TEE.
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


#include "secureapi.h"

#ifdef WIN32
	#include <stdlib.h>
#endif

#include "tee_internal_api.h"

#define TA_DEBUG
#include "tee_log.h"

/* ----------------------------------------------------------------------------
 *   Trusted Application Entry Points
 * ---------------------------------------------------------------------------- */

#ifdef WIN32
	#define DllExport __declspec(dllexport)
#else
	#define DllExport
#endif


EXPORT_SYMBOL(TA_CreateEntryPoint);
EXPORT_SYMBOL(TA_OpenSessionEntryPoint);
EXPORT_SYMBOL(TA_InvokeCommandEntryPoint);
EXPORT_SYMBOL(TA_CloseSessionEntryPoint);
EXPORT_SYMBOL(TA_DestroyEntryPoint);

//#define TA_ERROR ta_error
#ifndef ENABLE_LIN_SO_BUILD
#define TA_ERROR
#else /* ENABLE_LIN_SO_BUILD */
#define TA_ERROR(...) fprintf(stderr, __VA_ARGS__);
#endif /* ENABLE_LIN_SO_BUILD */

/* 
Assumptions
	* Will be called by either a 64bit or 32bit envrionment
	* The TEE UM and KM is 32bit

*/

/*!
******************************************************************************

 @Function              TA_CreateEntryPoint

 @Description

 The function TA_CreateEntryPoint is the Trusted Application's constructor,
 *which the Framework calls when it creates a new instance of the Trusted Application.

******************************************************************************/

DllExport TEE_Result TA_CreateEntryPoint(void)
{
	return(TEE_SUCCESS);
}


/*!
******************************************************************************

 @Function              TA_OpenSessionEntryPoint

 @Description

 The Framework calls the function TA_OpenSessionEntryPoint when a client requests 
 to open a session with the Trusted Application.The open session request may result
 in a new Trusted Application instance being created.

******************************************************************************/

DllExport TEE_Result TA_OpenSessionEntryPoint(uint32_t paramTypes, TEE_Param params[4], void** sessionContext)
{
	//Create a IMG secure handle for this session
	//ParamType 1,   TEE_PARAM_TYPE_VALUE_INPUT,  BSPP = BITSTREAM_SCAN, VXD_IO = VXD_CORE0 | VXD_CORE1
	//ParamType 2,   TEE_PARAM_TYPE_VALUE_OUTPUT,  IMG_HANDLE, Secure session handle (TEE ptr so will be 32bits)
	//ParamType 3,   TEE_PARAM_TYPE_VALUE_OUTPUT,  IMG_RESULT, return code from SECURE_GetHandle()
	//Session Context will be the Secure Handle *sessionContext = SECURE_HANDLE

	TEE_Result result = TEE_SUCCESS;

	/* Validation parameters */
	if((TEE_PARAM_TYPE_GET(paramTypes, 0) != TEE_PARAM_TYPE_VALUE_INPUT) ||
       (TEE_PARAM_TYPE_GET(paramTypes, 1) != TEE_PARAM_TYPE_VALUE_OUTPUT) ||
	   (TEE_PARAM_TYPE_GET(paramTypes, 2) != TEE_PARAM_TYPE_VALUE_OUTPUT))
	{
		TA_ERROR("Unexpected parameter types (%d)", paramTypes);
        result = TEE_ERROR_BAD_PARAMETERS;
	}
	else
	{
		IMG_RESULT		imgResult;
		IMG_UINT32		ui32SecureID;
		IMG_VIDEO_CORE	nCore = params[0].value.a;

		imgResult = SECURE_GetId(nCore, &ui32SecureID);

		params[1].value.a = ui32SecureID;
		params[2].value.a = imgResult;

		if(imgResult != IMG_SUCCESS)	
			imgResult = TEE_FAIL;
		else
			*sessionContext = (void *) ui32SecureID;
	}

   return(result);
}

/*!
******************************************************************************

 @Function              TA_InvokeCommandEntryPoint

 @Description

 The Framework calls this function when the client invokes a command within 
 the given session.

******************************************************************************/

DllExport TEE_Result TA_InvokeCommandEntryPoint(void* session_context, uint32_t cmd_id, uint32_t paramTypes, TEE_Param params[4])
{
	//ParamType 1,   TEE_PARAM_TYPE_VALUE_INPUT,  IMG_RESULT, return code from SECURE_GetHandle()
	//ParamType 2,   TEE_PARAM_TYPE_MEMREF_INOUT, IMG Message buffer (input/output)
	//ParamType 3,   TEE_PARAM_TYPE_VALUE_OUTPUT,  IMG_RESULT, return code from SECURE_SendMessage()
	//[ParamType 4], Optional parameter must be TEEC_MEMREF_TEMP_OUTPUT/INPUT/INOUT

	TEE_Result result = TEE_SUCCESS;

	/* Validation parameters 3 fixed paramters */
	if((TEE_PARAM_TYPE_GET(paramTypes, 0) != TEE_PARAM_TYPE_VALUE_INPUT) ||
       (TEE_PARAM_TYPE_GET(paramTypes, 1) != TEE_PARAM_TYPE_MEMREF_INOUT) ||
	   (TEE_PARAM_TYPE_GET(paramTypes, 2) != TEE_PARAM_TYPE_VALUE_OUTPUT))
	{
		TA_ERROR("Unexpected parameter types (%d)", paramTypes);
        result = TEE_ERROR_BAD_PARAMETERS;
	}
	else if(!((TEE_PARAM_TYPE_GET(paramTypes, 3) == TEE_PARAM_TYPE_NONE) ||
	          (TEE_PARAM_TYPE_GET(paramTypes, 3) == TEE_PARAM_TYPE_MEMREF_INPUT) ||
              (TEE_PARAM_TYPE_GET(paramTypes, 3) == TEE_PARAM_TYPE_MEMREF_INOUT) ||
	          (TEE_PARAM_TYPE_GET(paramTypes, 3) == TEE_PARAM_TYPE_MEMREF_OUTPUT)))
	{
		//Optional fourth parameter incorrect
		TA_ERROR("Unexpected parameter types (%d)", paramTypes);
        result = TEE_ERROR_BAD_PARAMETERS;
	}
	else if (params[0].value.a != (unsigned int)session_context)
	{
		//Client as passed back in the wrong secure handle
		TA_ERROR("Incorrect secure handle (%d)", params[0].value.a);
        result = TEE_ERROR_BAD_PARAMETERS;
	}
	else
	{
		IMG_UINT32		ui32SecureID = (IMG_UINT32) session_context;
		IMG_BYTE		*pbyMsg		=params[1].memref.buffer;
		IMG_UINT16		ui16Size	=params[1].memref.size;
		MSG_ENDPOINT    nEndpoint	=cmd_id;

		IMG_BYTE		*pOptionalBuffer     = NULL; 
		IMG_SIZE		szOptionalBufferSize = 0;

		IMG_RESULT		imgResult;

		if(TEE_PARAM_TYPE_GET(paramTypes, 3) != TEE_PARAM_TYPE_NONE)
		{
			pOptionalBuffer			= params[3].memref.buffer;
			szOptionalBufferSize	= params[3].memref.size;
		}

		imgResult = SECURE_SendMessage(ui32SecureID, pbyMsg, ui16Size, nEndpoint, pOptionalBuffer, szOptionalBufferSize);

		params[2].value.a = imgResult;

		if(imgResult != IMG_SUCCESS)	
			imgResult = TEE_FAIL;
	}

	return(result);
}

/*!
******************************************************************************

 @Function              TA_CloseSessionEntryPoint

 @Description

 The Framework calls this function to close a client session. During the call 
 to this function the implementation can use any session functions.

******************************************************************************/

DllExport void TA_CloseSessionEntryPoint(void* session_context)
{
	//Session Context is the SECURE_HANDLE for this session

	IMG_UINT32		ui32SecureID = (IMG_UINT32) session_context;
	IMG_RESULT		imgResult;

	imgResult = SECURE_ReleaseId(ui32SecureID);

	if(imgResult != IMG_SUCCESS)
		TA_ERROR("CloseSessionFailed (%d)", imgResult);
}

/*!
******************************************************************************

 @Function              TA_DestroyEntryPoint

 @Description

 The Framework calls this function to close a client session. During the call 
 to this function the implementation can use any session functions.

******************************************************************************/

DllExport void TA_DestroyEntryPoint(void)
{
}


/* Mapping for TEE functions to Windows ones */ 


#ifdef WIN32

void  TEE_MemFill(void* buffer, uint32_t x, uint32_t size)	{ memset(buffer,x,size); }
void  TEE_MemMove(void* dest, void* src, uint32_t size)		{ memmove(dest,src,size); }

void *TEE_Malloc(size_t size, uint32_t hint)				{ return malloc(size); }			
void  TEE_Free(void *buffer)								{ free(buffer); }

void SLog(const char *fmt, ...)								{ return; }

void TEE_Panic(TEE_Result panicCode)						{ exit(panicCode); } 

#endif
