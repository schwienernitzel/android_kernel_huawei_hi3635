/*!
 *****************************************************************************
 *
 * @File       tee_client_api.h
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

#ifndef _TEE_CLIENT_API_H_
#define _TEE_CLIENT_API_H_

#ifdef __cplusplus
extern "C" {
#endif
#define LOG_NDEBUG 0
#define LOG_TAG "libteec"
//DAB #include <android/log.h>
#include "tee_client_type.h"

/**
 * @ingroup TEEC_BASIC_FUNC
 * 用于计算非安全世界与安全世界传递参数的数值
 */
#define TEEC_PARAM_TYPES( param0Type, param1Type, param2Type, param3Type) \
    ((param3Type) << 12 | (param2Type) << 8 | (param1Type) << 4 | (param0Type))

/**
 * @ingroup TEEC_BASIC_FUNC
 * 用于计算paramTypes中字段index的数值
 */
#define TEEC_PARAM_TYPE_GET( paramTypes, index) \
    (((paramTypes) >> (4*(index))) & 0x0F)

/**
 * @ingroup TEEC_BASIC_FUNC
 * 当参数类型为#TEEC_Value时，如果成员变量a或b没有给定值，需赋予此值，
 * 表示没有用到此成员变量
 */
#define TEEC_VALUE_UNDEF 0xFFFFFFFF

/**
 * @ingroup TEEC_VERSION
 * TEEC版本号:1.0对应TrustedCore1.xx版本
 */
#define TEEC_VERSION (100)


/**
 * @ingroup TEEC_BASIC_FUNC
 * 调试接口，在定义宏TEEC_DEBUG情况下有效，宏TEEC_DEBUG是调试打印开关
 */
#ifdef TEEC_DEBUG
#define TEEC_Debug(...) __android_log_print(ANDROID_LOG_VERBOSE, LOG_TAG, __VA_ARGS__)
#else
#define TEEC_Debug(...)
#endif

#ifndef ENABLE_LIN_SO_BUILD
/**
 * @ingroup TEEC_BASIC_FUNC
 * 调试接口，API函数内部打印错误信息
 */
#define TEEC_Error(...) __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,__VA_ARGS__)
#else /* ENABLE_LIN_SO_BUILD */
#define TEEC_Error(...) printf(__VA_ARGS__)
#endif /* ENABLE_LIN_SO_BUILD */



/**
 * @ingroup  TEEC_BASIC_FUNC
 * @brief 初始化TEE环境
 *
 * @par 描述:
 * 初始化路径为name的TEE环境，参数name可以为空，
 * 初始化TEE环境是打开会话、发送命令的基础，
 * 初始化成功后，客户端应用与TEE建立一条链接。
 *
 * @attention 无
 * @param name [IN] TEE环境路径
 * @param context [IN/OUT] context指针，安全世界环境句柄
 *
 * @retval #TEEC_SUCCESS 初始化TEE环境成功
 * @retval #TEEC_ERROR_BAD_PARAMETERS 参数不正确，name不正确或context为空
 * @retval #TEEC_ERROR_GENERIC 系统可用资源不足等原因
 *
 * @par 依赖:
 * @li libteec：该接口所属的共享库
 * @li tee_client_api.h：该接口声明所在头文件
 * @see TEEC_FinalizeContext
 * @since V100R002C00B301
 */
TEEC_Result TEEC_InitializeContext (
    const char* name,
    TEEC_Context* context);

/**
 * @ingroup  TEEC_BASIC_FUNC
 * @brief 关闭TEE环境
 *
 * @par 描述:
 * 关闭context指向的TEE环境，断开客户端应用与TEE环境的链接
 *
 * @attention 无
 * @param context [IN/OUT] 指向已初始化成功的TEE环境
 *
 * @retval 无
 *
 * @par 依赖:
 * @li libteec：该接口所属的共享库
 * @li tee_client_api.h：该接口声明所在头文件
 * @see TEEC_InitializeContext
 * @since V100R002C00B301
 */
void TEEC_FinalizeContext (
    TEEC_Context* context);

/**
 * @ingroup  TEEC_BASIC_FUNC
 * @brief 打开会话
 *
 * @par 描述:
 * 在指定的TEE环境context下，为客户端应用与UUID为destination的安全服务建立一条链接，
 * 链接方式是connectionMethod，链接数据是connectionData，传递的数据包含在opetation里。
 * 打开会话成功后，输出参数session是对该链接的一个描述；
 * 如果打开会话失败，输出参数returnOrigin为错误来源。
 *
 * @attention 无
 * @param context [IN/OUT] 指向已初始化成功的TEE环境
 * @param session [OUT] 指向会话，取值不能为空
 * @param destination [IN] 安全服务的UUID，一个安全服务拥有唯一的UUID
 * @param connectionMethod [IN] 链接方式，取值范围为#TEEC_LoginMethod
 * @param connectionData [IN] 与链接方式相对应的链接数据，
 * 如果链接方式为#TEEC_LOGIN_PUBLIC、#TEEC_LOGIN_USER、
 * #TEEC_LOGIN_USER_APPLICATION、#TEEC_LOGIN_GROUP_APPLICATION，链接数据取值必须为空，
 * 如果链接方式为#TEEC_LOGIN_GROUP、#TEEC_LOGIN_GROUP_APPLICATION，
 * 链接数据必须指向类型为uint32_t的数据，此数据表示客户端应用期望链接的组用户
 * @param operation [IN/OUT] 客户端应用与安全服务传递的数据
 * @param returnOrigin [IN/OUT] 错误来源，取值范围为#TEEC_ReturnCodeOrigin
 *
 * @retval #TEEC_SUCCESS 打开会话成功
 * @retval #TEEC_ERROR_BAD_PARAMETERS 参数不正确，参数context为空或session为空或destination为空
 * @retval #TEEC_ERROR_ACCESS_DENIED 系统调用权限访问失败
 * @retval #TEEC_ERROR_OUT_OF_MEMORY 系统可用资源不足
 * @retval #TEEC_ERROR_TRUSTED_APP_LOAD_ERROR 加载安全服务失败
 * @retval 其它返回值参考 #TEEC_ReturnCode
 *
 * @par 依赖:
 * @li libteec：该接口所属的共享库
 * @li tee_client_api.h：该接口声明所在头文件
 * @see TEEC_CloseSession
 * @since V100R002C00B301
 */
TEEC_Result TEEC_OpenSession (
    TEEC_Context* context,
    TEEC_Session* session,
    const TEEC_UUID* destination,
    uint32_t connectionMethod,
    const void* connectionData,
    TEEC_Operation* operation,
    uint32_t* returnOrigin);

/**
 * @ingroup  TEEC_BASIC_FUNC
 * @brief 关闭会话
 *
 * @par 描述:
 * 关闭session指向的会话，断开客户端应用与安全服务的链接
 *
 * @attention 无
 * @param session [IN/OUT] 指向已成功打开的会话
 *
 * @retval 无
 *
 * @par 依赖:
 * @li libteec：该接口所属的共享库
 * @li tee_client_api.h：该接口声明所在头文件
 * @see TEEC_OpenSession
 * @since V100R002C00B301
 */
void TEEC_CloseSession(
    TEEC_Session* session);

/**
 * @ingroup  TEEC_BASIC_FUNC
 * @brief 发送命令
 *
 * @par 描述:
 * 在指定的会话session里，由客户端应用向安全服务发送命令commandID，
 * 发送的数据为operation，如果发送命令失败，输出参数returnOrigin为错误来源
 *
 * @attention 无
 * @param session [IN/OUT] 指向已打开成功的会话
 * @param commandID [IN] 安全服务支持的命令ID，由安全服务定义
 * @param operation [IN/OUT] 包含了客户端应用向安全服务发送的数据内容
 * @param returnOrigin [IN/OUT] 错误来源，取值范围为#TEEC_ReturnCodeOrigin
 *
 * @retval #TEEC_SUCCESS 发送命令成功
 * @retval #TEEC_ERROR_BAD_PARAMETERS 参数不正确，参数session为空或参数operation格式不正确
 * @retval #TEEC_ERROR_ACCESS_DENIED 系统调用权限访问失败
 * @retval #TEEC_ERROR_OUT_OF_MEMORY 系统可用资源不足
 * @retval 其它返回值参考 #TEEC_ReturnCode
 *
 * @par 依赖:
 * @li libteec：该接口所属的共享库
 * @li tee_client_api.h：该接口声明所在头文件
 * @see 无
 * @since V100R002C00B301
 */
TEEC_Result TEEC_InvokeCommand(
    TEEC_Session*     session,
    uint32_t          commandID,
    TEEC_Operation*   operation,
    uint32_t*         returnOrigin);

/**
 * @ingroup  TEEC_BASIC_FUNC
 * @brief 注册共享内存
 *
 * @par 描述:
 * 在指定的TEE环境context内注册共享内存sharedMem，
 * 通过注册的方式获取共享内存来实现零拷贝需要操作系统的支持，
 * 目前的实现中，该方式不能实现零拷贝
 *
 * @attention 如果入参sharedMem的size域设置为0，函数会返回成功，但无法使用这块
 * 共享内存，因为这块内存没有大小
 * @param context [IN/OUT] 已初始化成功的TEE环境
 * @param sharedMem [IN/OUT] 共享内存指针，共享内存所指向的内存不能为空、大小不能为零
 *
 * @retval #TEEC_SUCCESS 发送命令成功
 * @retval #TEEC_ERROR_BAD_PARAMETERS 参数不正确，参数context为空或sharedMem为空，
 * 或共享内存所指向的内存为空
 *
 * @par 依赖:
 * @li libteec：该接口所属的共享库
 * @li tee_client_api.h：该接口声明所在头文件
 * @see TEEC_AllocateSharedMemory
 * @since V100R002C00B301
 */
TEEC_Result TEEC_RegisterSharedMemory (
    TEEC_Context* context,
    TEEC_SharedMemory* sharedMem);

/**
 * @ingroup  TEEC_BASIC_FUNC
 * @brief 申请共享内存
 *
 * @par 描述:
 * 在指定的TEE环境context内申请共享内存sharedMem，
 * 通过共享内存可以实现非安全世界与安全世界传递数据时的零拷贝
 *
 * @attention 如果入参sharedMem的size域设置为0，函数会返回成功，但无法使用这块
 * 共享内存，因为这块内存既没有地址也没有大小
 * @param context [IN/OUT] 已初始化成功的TEE环境
 * @param sharedMem [IN/OUT] 共享内存指针，共享内存的大小不能为零
 *
 * @retval #TEEC_SUCCESS 发送命令成功
 * @retval #TEEC_ERROR_BAD_PARAMETERS 参数不正确，参数context为空或sharedMem为空
 * @retval #TEEC_ERROR_OUT_OF_MEMORY 系统可用资源不足，分配失败
 *
 * @par 依赖:
 * @li libteec：该接口所属的共享库
 * @li tee_client_api.h：该接口声明所在头文件
 * @see TEEC_RegisterSharedMemory
 * @since V100R002C00B301
 */
TEEC_Result TEEC_AllocateSharedMemory (
    TEEC_Context* context,
    TEEC_SharedMemory* sharedMem);

/**
 * @ingroup  TEEC_BASIC_FUNC
 * @brief 释放共享内存
 *
 * @par 描述:
 * 释放已注册成功的的或已申请成功的共享内存sharedMem
 *
 * @attention 如果是通过#TEEC_AllocateSharedMemory方式获取的共享内存，
 * 释放时会回收这块内存；如果是通过#TEEC_RegisterSharedMemory方式
 * 获取的共享内存，释放时不会回收共享内存所指向的本地内存
 * @param sharedMem [IN/OUT] 指向已注册成功或申请成功的共享内存
 *
 * @retval 无
 *
 * @par 依赖:
 * @li libteec：该接口所属的共享库
 * @li tee_client_api.h：该接口声明所在头文件
 * @see TEEC_RegisterSharedMemory | TEEC_AllocateSharedMemory
 * @since V100R002C00B301
 */
void TEEC_ReleaseSharedMemory (
    TEEC_SharedMemory* sharedMem);

/**
 * @ingroup  TEEC_BASIC_FUNC
 * @brief cancel API
 *
 * @par 描述:
 * 取消掉一个正在运行的open Session或者是一个invoke command
 * 发送一个cancel的signal后立即返回
 *
 * @attention 此操作仅仅是发送一个cancel的消息，是否进行cancel操作由前面的TEE 或 TA决定
 * @param operation [IN/OUT] 包含了客户端应用向安全服务发送的数据内容
 *
 * @retval 无
 *
 * @par 依赖:
 * @li libteec：该接口所属的共享库
 * @li tee_client_api.h：该接口声明所在头文件
 * @see 无
 * @since V100R002C00B309
 */
 void TEEC_RequestCancellation(
 TEEC_Operation *operation);



TEEC_Result TEEC_EXT_GetTEEInfo (TEEC_EXT_TEEInfo *tee_info_data, uint32_t *length);

/**
 * @ingroup  TEEC_EXT_FUNC
 * @brief register agent API
 *
 * @par 描述:
 * 非安全侧注册agent（listener）的接口
 *
 * @attention 此操作首先会映射并注册共享内存（目前只支持4K大小的共享内存），然后再注册agent
 * @param agent_id [IN] 用户传入一个agent_id，与TA进行通信的唯一标识，
 * 因此，TA发送消息给CA时，会根据该agent_id来发起通信
 * @param dev_fd [OUT] 用户获取访问TEE驱动设备的描述符
 * @param buffer [OUT] 用户获取指向共享内存的用户态地址
 * @retval #TEEC_SUCCESS agent注册成功
 * @retval #TEEC_ERROR_GENERIC 其他的一般错误
 *
 * @par 依赖:
 * @li libteec：该接口所属的共享库
 * @li tee_client_api.h：该接口声明所在头文件
 * @see 无
 * @since V100R005C00
 */
TEEC_Result TEEC_EXT_RegisterAgent (uint32_t agent_id, int* dev_fd, void** buffer);

/**
 * @ingroup  TEEC_EXT_FUNC
 * @brief wait for event from TA
 *
 * @par 描述:
 * 非安全侧agent等待安全侧TA事件
 *
 * @attention 此接口会阻塞等待，因此建议在新创建的线程中来调用此接口
 * @param agent_id [IN] 用户传入一个agent_id，与TA进行通信的唯一标识，
 * 因此，TA发送消息给CA时，会根据该agent_id来发起通信
 * @param dev_fd [IN] 访问TEE驱动设备的描述符
 * @retval #TEEC_SUCCESS agent等待TA消息事件成功
 * @retval #TEEC_ERROR_GENERIC 其他的一般错误
 *
 * @par 依赖:
 * @li libteec：该接口所属的共享库
 * @li tee_client_api.h：该接口声明所在头文件
 * @see 无
 * @since V100R005C00
 */
TEEC_Result TEEC_EXT_WaitEvent(uint32_t agent_id, int dev_fd);

/**
 * @ingroup  TEEC_EXT_FUNC
 * @brief send response to TA
 *
 * @par 描述:
 * 非安全侧agent响应安全侧TA的事件
 *
 * @attention 此接口会唤醒主进程继续执行
 * @param agent_id [IN] 用户传入一个agent_id，与TA进行通信的唯一标识，
 * 因此，TA发送消息给CA时，会根据该agent_id来发起通信
 * @param dev_fd [IN] 用户访问TEE驱动设备的描述符
 * @retval #TEEC_SUCCESS agent响应事件发送成功
 * @retval #TEEC_ERROR_GENERIC 其他的一般错误
 *
 * @par 依赖:
 * @li libteec：该接口所属的共享库
 * @li tee_client_api.h：该接口声明所在头文件
 * @see 无
 * @since V100R005C00
 */
TEEC_Result TEEC_EXT_SendEventResponse(uint32_t agent_id, int dev_fd);


/**
 * @ingroup  TEEC_EXT_FUNC
 * @brief unregister agent API
 *
 * @par 描述:
 * 非安全侧去注册agent（listener）的接口
 *
 * @attention 此接口会通知内核去注册agent，同时释放共享内存，因此指向共享内存的
 * 用户态指针不能再继续使用了
 * @param agent_id [IN] 用户传入一个agent_id，与TA进行通信的唯一标识，
 * 因此，TA发送消息给CA时，会根据该agent_id来发起通信
 * @param dev_fd [IN] 用户访问TEE驱动设备的描述符
 * @param buffer [IN] 用户指向共享内存的用户态地址，函数返回时会将指针置为NULL
 * @retval #TEEC_SUCCESS agent去注册成功
 * @retval #TEEC_ERROR_GENERIC 其他的一般错误
 *
 * @par 依赖:
 * @li libteec：该接口所属的共享库
 * @li tee_client_api.h：该接口声明所在头文件
 * @see 无
 * @since V100R005C00
 */
TEEC_Result TEEC_EXT_UnregisterAgent (uint32_t agent_id, int dev_fd, void** buffer);

#ifdef __cplusplus
}
#endif

#endif

