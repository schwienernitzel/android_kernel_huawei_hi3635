/*!
 *****************************************************************************
 *
 * @File       tee_log.h
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

#ifndef __TEE_LOG_H
#define __TEE_LOG_H

#include "tee_internal_api.h"

extern void uart_printf_func(const char *fmt, ...);

//#define TA_DEBUG
#ifdef TA_DEBUG
#define DEBUG_TAG "[debug]"
#define ta_debug(fmt, args,...) uart_printf_func("%s %s: " fmt "", DEBUG_TAG, __FUNCTION__, ## args)
#else
#define ta_debug(fmt, args...)
#endif

#define ERROR_TAG "[error]"
#define ta_error(fmt, args,...) uart_printf_func("%s %s: " fmt "", ERROR_TAG, __FUNCTION__, ## args)

#define TA_LOG
#ifdef TA_LOG
/**
 * @ingroup grp_log_api
 * 日志的级别Trace
 */
#define TRACE "[Trace]"
/**
 * @ingroup grp_log_api
 * 日志的级别Warning
 */
#define WARNING "[Warning]"
/**
 * @ingroup grp_log_api
 * 日志的级别Error
 */
#define ERROR "[Error]"

/**
 * @ingroup grp_log_api
 * 打印Trace日志的接口
 */
#define SLogTrace(fmt, args,...) SLog("%s %s: " fmt "\n", TRACE, __FUNCTION__, ## args)
/**
 * @ingroup grp_log_api
 * 打印Warning日志的接口
 */
#define SLogWarning(fmt, args,...) SLog("%s %s: " fmt "\n", WARNING, __FUNCTION__, ## args)
/**
 * @ingroup grp_log_api
 * 打印Error日志的接口
 */
#define SLogError(fmt, args,...) SLog("%s %s: " fmt "\n", ERROR, __FUNCTION__, ## args)

//TODO: SHOULD call Panic to deal, here just return
/**
 * @ingroup grp_log_api
 * 断言接口
 */
#define SAssert(exp) \
    do {    \
        if (!(exp)) {   \
            SLog("Assertion [ %s ] Failed: File %s, Line %d, Function %s\n",   \
            #exp, __FILE__, __LINE__, __FUNCTION__);    \
            return 0xFFFF0001;    \
        }   \
    } while(0);
#else
#define SLogTrace(fmt, args...) ((void)0)
#define SLogWarning(fmt, args...) ((void)0)
#define SLogError(fmt, args...) ((void)0)
#define SAssert(exp)  ((void)0)
#endif

/**
* @ingroup  grp_log_api
* @brief 将日志输出至文件
*
* @par 描述:
* 无
*
* @attention 无
* @param fmt [IN] 日志的格式化内容
*
* @retval #无
*
* @par 依赖:
* @li tee_internal_api.h：内部数据类型定义
**/
void SLog(const char *fmt, ...);

#endif
