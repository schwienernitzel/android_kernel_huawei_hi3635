/*!
 *****************************************************************************
 *
 * @File       sysos_api.c
 * @Description    This file contains the OS Kernel Mode API.
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



#include <report_api.h>
#include <sysos_api.h>

/*!
******************************************************************************
 Mutex structure
******************************************************************************/
typedef struct
{
	IMG_UINT32 dummy;
  
} SYSOS_SEC_sMutex;

//!< Indicates where the API has been initialised
static IMG_BOOL    gInitialised = IMG_FALSE;

//!< Count of call to SYSOS_SEC_Initialise() - decremented on call to SYSOS_SEC_Deinitialise()
static IMG_UINT32  gui32InitCnt = 0;


/*!
******************************************************************************

 @Function                SYSOS_SEC_Initialise

******************************************************************************/
IMG_RESULT SYSOS_SEC_Initialise(IMG_VOID)
{
    /*** NOTE: Some of this code is NOT thread safe as we have no OS to rely on...*/
    /* If not initialised...*/
    if (!gInitialised)
    {
        /* Now we are initialised..*/
        gInitialised = IMG_TRUE;
    }

    /* Update count...*/
    gui32InitCnt++;

    return IMG_SUCCESS;
}

/*!
******************************************************************************

 @Function                SYSOS_SEC_Deinitialise


******************************************************************************/
IMG_VOID SYSOS_SEC_Deinitialise(IMG_VOID)
{
    /* Update count...*/
    IMG_ASSERT(gui32InitCnt != 0);
    gui32InitCnt--;

    /* If we are initialised and this is the last de-initialise...*/
    if ( gInitialised && gui32InitCnt == 0 )
    {
        /* Now we are de-initialised..*/
        gInitialised = IMG_FALSE;
    }
}


/*!
******************************************************************************

 @Function                SYSOS_SEC_CreateMutex

******************************************************************************/
IMG_RESULT SYSOS_SEC_CreateMutex(
    IMG_HANDLE *  phMutexHandle
)
{
	return IMG_ERROR_FATAL;
}


/*!
******************************************************************************

 @Function                SYSOS_SEC_DestroyMutex

******************************************************************************/
IMG_VOID SYSOS_SEC_DestroyMutex(
    IMG_HANDLE  hMutexHandle
)
{
  
}


/*!
******************************************************************************

 @Function                SYSOS_SEC_LockMutex

******************************************************************************/
IMG_VOID SYSOS_SEC_LockMutex(
    IMG_HANDLE  hMutexHandle
)
{
  
}


/*!
******************************************************************************

 @Function                SYSOS_SEC_UnlockMutex

******************************************************************************/
IMG_VOID SYSOS_SEC_UnlockMutex(
    IMG_HANDLE  hMutexHandle
)
{
  
}

/*!
******************************************************************************

 @Function				SYSOS_SEC_IORead32
 
******************************************************************************/
IMG_UINT32 SYSOS_SEC_IORead32(
    IMG_UINT32 * pui32Address
)
{
    return 0;
    
}

/*!
******************************************************************************

 @Function				SYSOS_SEC_IOWrite32

******************************************************************************/
IMG_VOID SYSOS_SEC_IOWrite32(
	IMG_UINT32 ui32Value,
	IMG_UINT32 * pui32Address
	)
{

}

/*!
******************************************************************************

 @Function				SYSOS_SEC_uSleep
 
******************************************************************************/
IMG_VOID SYSOS_SEC_uSleep(IMG_UINT32 ui32Interval)
{

}


