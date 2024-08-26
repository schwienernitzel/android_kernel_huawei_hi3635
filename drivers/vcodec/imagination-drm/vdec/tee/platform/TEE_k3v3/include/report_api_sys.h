/*!
 *****************************************************************************
 *
 * @File       report_api_sys.h
 * @Title      REPORT API system bindings
 * @Description    This file contains the header file information for the
 *  REPORT API
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

#if !defined (__REPORT_API_SYS_H__)
#define __REPORT_API_SYS_H__

#include <report_modules.h>

#if defined(__cplusplus)
extern "C" {
#endif


/*
  REPORT levels
*/

#define REPORT_EMERG    KERN_ERR          /* system is unusable */
#define REPORT_ALERT    KERN_ERR          /* action must be taken immediately */
#define REPORT_CRIT     KERN_ERR          /* critical conditions */
#define REPORT_ERR      KERN_ERR          /* error conditions */
#define REPORT_WARNING  KERN_WARNING      /* warning conditions */
#define REPORT_NOTICE   KERN_INFO         /* normal but significant condition */
#define REPORT_INFO     KERN_INFO         /* informational */


/*
  REPORT macros
*/

#define DEBUG_REPORT(MODULE, fmt, ...) \
    do {\
        ta_debug(REPORT_IMG_PREFIX ":" MODULE ": " fmt "\n", ##__VA_ARGS__); \
    } while(0)

#define REPORT(MODULE, LEVEL, fmt, ...) \
    do {\
        ta_debug(REPORT_IMG_PREFIX ":" MODULE ": " fmt "\n", ##__VA_ARGS__); \
    } while(0)

	
/*
  REPORT modules
*/

#define REPORT_MODULE_BSPP      REPORT_MODULE_TEXT_BSPP
#define REPORT_MODULE_VXDIO     REPORT_MODULE_TEXT_VXDIO
#define REPORT_MODULE_MTXIO     REPORT_MODULE_TEXT_MTXIO
#define REPORT_MODULE_SWSR      REPORT_MODULE_TEXT_SWSR

#if defined (__cplusplus)
}
#endif

#endif /* __REPORT_API_SYS_H__  */
