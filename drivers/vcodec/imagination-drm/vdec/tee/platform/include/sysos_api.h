/*!
 *****************************************************************************
 *
 * @File       sysos_api.h
 * @Title      The System OS kernel mode API.
 * @Description    This file contains the header file information for the
 *  OS Kernel Mode API.
 *  The implementation of the OS Kernel Mode API is
 *  specific to OS.
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

#if !defined (__SYSOS_SEC_API_H__)
#define __SYSOS_SEC_API_H__

#include <img_errors.h>
#include <img_types.h>
#include <img_defs.h>

#if defined(__cplusplus)
extern "C" {
#endif

/*!
******************************************************************************
 This type defines the power transition.
******************************************************************************/
typedef enum
{
	SYSOS_SEC_POWERTRANS_PRE,				//!< Pre state transition
	SYSOS_SEC_POWERTRANS_POST,			//!< Post state transition

} SYSOS_SEC_ePowerTrans;

/*!
******************************************************************************
 This type defines the power states - as defined in the ACPI spec.  However,
 only S0 and S5 are currently supported, more states may be added as/when they
 are required.

 NOTE: Not all OS's will support all state or state transitions and not all
 devices can support the subtle differences between the various modes of 
 "sleeping" - which is why only a limited set of states is currently supported.
******************************************************************************/
typedef enum
{
	SYSOS_SEC_POWERSTATE_S0,			//!< Working:  The normal working state of the computer 
//	SYSOS_SEC_POWERSTATE_S1,			//!< Sleeping: The most power-hungry of sleep-modes.
//	SYSOS_SEC_POWERSTATE_S2,			//!<		   A deeper sleep-state than S1
//	SYSOS_SEC_POWERSTATE_S3,			//!<           Known as suspend to RAM
//	SYSOS_SEC_POWERSTATE_S4,			//!<		   Hibernation in Windows, Safe Sleep in Mac OS X, also known as Suspend to disk
	SYSOS_SEC_POWERSTATE_S5,			//!<		   Soft Off

} SYSOS_SEC_ePowerState;

/*!
******************************************************************************

 @Function				SYSOS_SEC_Initialise
 
 @Description 
 
 This function is used to initialise the kernel OS component and is called at
 start-up.
 
 @Input		None. 

 @Return    IMG_RESULT :	This function returns either IMG_SUCCESS or an
							error code.

******************************************************************************/
extern IMG_RESULT SYSOS_SEC_Initialise(IMG_VOID);


/*!
******************************************************************************

 @Function				SYSOS_SEC_Deinitialise
 
 @Description 
 
 This function is used to deinitialises the kernel OS component and would 
 normally be called at shutdown.
 
 @Input		None.

 @Return	None.

******************************************************************************/
extern IMG_VOID SYSOS_SEC_Deinitialise(IMG_VOID);


/*!
******************************************************************************

 @Function				SYSOS_SEC_CreateMutex
 
 @Description 
 
 This function is used to create a mutex.
 
 @Output	phMutexHandle :	A pointer used to return the mutex handle.

 @Return    IMG_RESULT :	This function returns either IMG_SUCCESS or an
							error code.

******************************************************************************/
extern IMG_RESULT SYSOS_SEC_CreateMutex(
	IMG_HANDLE *			phMutexHandle
);


/*!
******************************************************************************

 @Function				SYSOS_SEC_DestroyMutex
 
 @Description 
 
 This function is used to destroy a mutex.
 
 @Input		hMutexHandle :	The mutex handle returned by SYSOS_SEC_CreateMutex().

 @Return	None.

******************************************************************************/
extern  IMG_VOID SYSOS_SEC_DestroyMutex(
    IMG_HANDLE			hMutexHandle
);


/*!
******************************************************************************

 @Function				SYSOS_SEC_LockMutex
 
 @Description 
 
 This function is used to lock a mutex.

 NOTE: Providing the lock requirements are met (correct process/thread etc) then
 then the mutex may be locked multiple times.
 
 NOTE: Each call to SYSOS_SEC_LockMutex() should be matched by a call to 
 SYSOS_SEC_UnlockMutex() - only when the last SYSOS_SEC_UnlockMutex() call is made
 is the mutex free to be locked by some other process and/or thread.

 @Input		hMutexHandle :	The mutex handle returned by SYSOS_SEC_CreateMutex().

 @Return	None.

******************************************************************************/
extern IMG_VOID SYSOS_SEC_LockMutex(
    IMG_HANDLE			hMutexHandle
);

/*!
******************************************************************************

 @Function				SYSOS_SEC_UnlockMutex
 
 @Description 
 
 This function is used to unlock a mutex.
 
 @Input		hMutexHandle:	The mutex handle returned by SYSOS_SEC_CreateMutex().

 @Return	None.

******************************************************************************/
extern IMG_VOID SYSOS_SEC_UnlockMutex(
    IMG_HANDLE			hMutexHandle
);


/*!
******************************************************************************

 @Function				SYSOS_SEC_uSleep
 
 @Description 
 
 This function is used to wait for a fixed amount of time.
 
 @Input		ui32Interval :	Timer period in usecs.

 @Return	None.

******************************************************************************/
extern IMG_VOID SYSOS_SEC_uSleep(
    IMG_UINT32                  ui32Interval
);

/*!
******************************************************************************

 @Function				SYSOS_SEC_IORead32
 
 @Description 
 
 This function is used read a 32 bit value from an I/O address
 
 @Input		pui32Address :	Address to read from

 @Return	Value read from memory

******************************************************************************/
extern IMG_UINT32 SYSOS_SEC_IORead32(
    IMG_UINT32 * pui32Address
);

/*!
******************************************************************************

 @Function				SYSOS_SEC_IOWrite32
 
 @Description 
 
 This function is used write a 32 bit value to an I/O address
 
 @Input		ui32Value    :	Value to write

 @Input		pui32Address :	Address to write to

 @Return	None.

******************************************************************************/
extern IMG_VOID SYSOS_SEC_IOWrite32(
    IMG_UINT32 ui32Value,
    IMG_UINT32 * pui32Address
);

#if defined(__cplusplus)
}
#endif
#endif /* __SYSOS_SEC_API_H__	*/
