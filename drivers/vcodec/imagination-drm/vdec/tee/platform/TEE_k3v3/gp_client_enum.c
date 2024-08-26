/*!
 *****************************************************************************
 *
 * @File       gp_client_enum.c
 * @Title      Global Platfrom Client function emulation layer.
 * @Description    This file contains sample code for faking the Global
 *  Platform client API's.
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

#include "tee_internal_api.h"
#include "tee_client_api.h"
#ifdef ENABLE_LIN_SO_BUILD
#include "tee_client_list.h"
#endif /* ENABLE_LIN_SO_BUILD */


//Entry Points into the TA - cannot include header files as not compatible with client TEE header files
TEEC_Result TA_CreateEntryPoint(void);
void TA_DestroyEntryPoint(void);
TEEC_Result TA_InvokeCommandEntryPoint(void* session_context, uint32_t cmd_id, uint32_t paramTypes, TEE_Param params[4]);
TEEC_Result TA_OpenSessionEntryPoint(uint32_t paramTypes, TEE_Param params[4], void** sessionContext);
void TA_CloseSessionEntryPoint(void* session_context);

//Internal function prototypes
bool MapParamsFromClientToTA(uint32_t paramTypes, TEEC_Parameter *ClientParams, TEE_Param *TAParams);
bool MapParamsFromTAToClient(uint32_t paramTypes, TEE_Param	*TAParams, TEEC_Parameter *ClientParams);

/*!
******************************************************************************

 @Function              TEEC_InitializeContext / TEEC_FinalizeContext

******************************************************************************/

static void *g_pSessionContext = NULL;

TEEC_Result  TEEC_InitializeContext(const char *name, TEEC_Context *context)
{
	return(TA_CreateEntryPoint());
}

void  TEEC_FinalizeContext(TEEC_Context *context)
{
	TA_DestroyEntryPoint();
}

/*!
******************************************************************************

 @Function              TEEC_OpenSession

******************************************************************************/

TEEC_Result  TEEC_OpenSession(TEEC_Context *context, TEEC_Session *session, const TEEC_UUID *destination, uint32_t connectionMethod, 
							  const void *connectionData, TEEC_Operation *operation, uint32_t *returnOrigin) 
{
	TEEC_Result rtn;
	uint32_t	paramTypes = 0;
	TEE_Param	params[4];

	if(operation)
	{
		paramTypes = operation->paramTypes; 
		if(!MapParamsFromClientToTA(paramTypes, operation->params, params)) return(TEEC_ERROR_BAD_PARAMETERS);
	}
	
	rtn = TA_OpenSessionEntryPoint(paramTypes, params, &g_pSessionContext);

	if(operation)
	{
		MapParamsFromTAToClient(paramTypes, params, operation->params);
	}

	return(rtn);
}

/*!
******************************************************************************

 @Function              TEEC_CloseSession

******************************************************************************/

void TEEC_CloseSession(TEEC_Session* session)
{
	TA_CloseSessionEntryPoint(g_pSessionContext);
}

/*!
******************************************************************************

 @Function              TEEC_InvokeCommand

******************************************************************************/

TEEC_Result TEEC_InvokeCommand(TEEC_Session *session, uint32_t  commandID, TEEC_Operation *operation,  uint32_t *returnOrigin)
{
	TEEC_Result rtn;
	uint32_t	paramTypes = 0;
	TEE_Param	params[4];

	if(operation)
	{
		paramTypes = operation->paramTypes; 
		if(!MapParamsFromClientToTA(paramTypes, operation->params, params)) return(TEEC_ERROR_BAD_PARAMETERS);
	}
	
	rtn = TA_InvokeCommandEntryPoint(g_pSessionContext, commandID, paramTypes, params);

	if(operation)
	{
		MapParamsFromTAToClient(paramTypes, params, operation->params);
	}

	return(rtn);
}

/*!
******************************************************************************

 @Function              TEEC_AllocateSharedMemory

******************************************************************************/

TEEC_Result TEEC_AllocateSharedMemory(TEEC_Context* context, TEEC_SharedMemory* sharedMem)
{
	sharedMem->buffer = IMG_SYSMALLOC(sharedMem->size);
		
	return(sharedMem->buffer ? TEEC_SUCCESS : TEEC_ERROR_OUT_OF_MEMORY);
}

/*!
******************************************************************************

 @Function              TEEC_ReleaseSharedMemory

******************************************************************************/

void TEEC_ReleaseSharedMemory(TEEC_SharedMemory *sharedMem)
{
	if(sharedMem->buffer)
		IMG_SYSFREE(sharedMem->buffer);
		
	sharedMem->buffer	= NULL;
	sharedMem->size		= 0;
}

/*!
******************************************************************************

 @Function              MapParamsFromClientToTA

******************************************************************************/

bool MapParamsFromClientToTA(uint32_t paramTypes, TEEC_Parameter *ClientParams, TEE_Param *TAParams)
{
	int i;	

	for(i = 0; i < 4; i++)
	{
		switch(TEEC_PARAM_TYPE_GET(paramTypes, i))
		{
			case TEEC_NONE:
				break;

			case TEEC_VALUE_INPUT:
			case TEEC_VALUE_INOUT:
#ifdef ENABLE_LIN_SO_BUILD
			case TEEC_VALUE_OUTPUT:
#endif /* ENABLE_LIN_SO_BUILD */
				TAParams[i].value.a = ClientParams[i].value.a;
				TAParams[i].value.b = ClientParams[i].value.b;
				break;

			case TEEC_MEMREF_PARTIAL_INOUT:
			case TEEC_MEMREF_PARTIAL_OUTPUT:
			case TEEC_MEMREF_PARTIAL_INPUT:
				TAParams[i].memref.buffer = ClientParams[i].memref.parent->buffer;
				TAParams[i].memref.size	=	ClientParams[i].memref.parent->size;
				break;

			case TEEC_MEMREF_TEMP_INPUT:
			case TEEC_MEMREF_TEMP_OUTPUT:
			case TEEC_MEMREF_TEMP_INOUT:
				TAParams[i].memref.buffer = ClientParams[i].tmpref.buffer;
				TAParams[i].memref.size	=	ClientParams[i].tmpref.size;
				break;
				
			//Not Implemented, fail function
			default:
				return(false);      
		}
	}

	return(true);
}

/*!
******************************************************************************

 @Function              MapParamsFromTAToClient

******************************************************************************/

bool MapParamsFromTAToClient(uint32_t paramTypes, TEE_Param	*TAParams, TEEC_Parameter *ClientParams)
{
	int i;	

	for(i = 0; i < 4; i++)
	{
		switch(TEEC_PARAM_TYPE_GET(paramTypes, i))
		{
			case TEEC_VALUE_OUTPUT:
			case TEEC_VALUE_INOUT:
				ClientParams[i].value.a = TAParams[i].value.a;
				ClientParams[i].value.b = TAParams[i].value.b;
				break;

			case TEEC_MEMREF_TEMP_OUTPUT:
			case TEEC_MEMREF_TEMP_INOUT:
				ClientParams[i].tmpref.size = TAParams[i].memref.size;
				break;
		}
	}

	return(true);
}


#if defined(__KERNEL__)
// ------------------- Linux kernel mode section ----------------------------

static bool self_test = 0;

module_param(self_test, bool, S_IRUGO);

EXPORT_SYMBOL(TEEC_InitializeContext);
EXPORT_SYMBOL(TEEC_OpenSession);
EXPORT_SYMBOL(TEEC_CloseSession);
EXPORT_SYMBOL(TEEC_InvokeCommand);
EXPORT_SYMBOL(TEEC_AllocateSharedMemory);
EXPORT_SYMBOL(TEEC_ReleaseSharedMemory);

#define IMG_PCI_VENDOR_ID 0x1010

MODULE_AUTHOR("Imagination Technologies");
MODULE_DESCRIPTION("Trusted execution environment reference for MSVDX cores");
MODULE_SUPPORTED_DEVICE("");
MODULE_VERSION("0.1.0");
//MODULE_LICENSE("Proprietary");
MODULE_LICENSE("GPL");

#endif /* __KERNEL__ */
