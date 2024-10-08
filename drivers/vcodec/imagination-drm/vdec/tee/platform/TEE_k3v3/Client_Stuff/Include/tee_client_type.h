/*!
 *****************************************************************************
 *
 * @File       tee_client_type.h
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

#ifndef _TEE_CLIENT_TYPE_H_
#define _TEE_CLIENT_TYPE_H_

#include "tee_client_constants.h"


#ifndef ENABLE_LIN_SO_BUILD
//DAB Added
#ifndef size_t 
#define size_t     unsigned int
#endif

/**
 * @ingroup TEEC_COMMON_DATA
 * �޷������Ͷ���
 */
typedef unsigned int      uint32_t;
/**
 * @ingroup TEEC_COMMON_DATA
 * �з������Ͷ���
 */
typedef signed int        int32_t;
/**
 * @ingroup TEEC_COMMON_DATA
 * �޷��Ŷ����Ͷ���
 */
typedef unsigned short    uint16_t;
/**
 * @ingroup TEEC_COMMON_DATA
 * �з��Ŷ����Ͷ���
 */
typedef signed short      int16_t;
/**
 * @ingroup TEEC_COMMON_DATA
 * �޷����ַ��Ͷ���
 */
typedef unsigned char     uint8_t;
/**
 * @ingroup TEEC_COMMON_DATA
 * �з����ַ��Ͷ���
 */
typedef signed char       int8_t;
/**
 * @ingroup TEEC_COMMON_DATA
 * ���ݳ������Ͷ���
 */


/**
 * @ingroup TEEC_COMMON_DATA
 * �������Ͷ���
 */
#ifndef bool
#define bool    uint8_t
#endif

/**
 * @ingroup TEEC_COMMON_DATA
 * trueֵ�Ķ���
 */
#ifndef true
#define true    1
#endif

/**
 * @ingroup TEEC_COMMON_DATA
 * falseֵ�Ķ���
 */
#ifndef false
#define false   0
#endif

/**
 * @ingroup TEEC_COMMON_DATA
 * NULLֵ�Ķ���
 */
#ifndef NULL
#define NULL 0
#endif

#endif /* not ENABLE_LIN_SO_BUILD */
#include "tee_client_list.h"
#ifndef ENABLE_LIN_SO_BUILD

/**
 * @ingroup TEEC_COMMON_DATA
 * ��������ֵ���Ͷ���
 *
 * ���ڱ�ʾ�������ؽ��
 */
typedef uint32_t TEEC_Result;

#endif /* not ENABLE_LIN_SO_BUILD */

/**
 * @ingroup TEEC_COMMON_DATA
 * UUID���Ͷ���
 *
 * UUID������ѭRFC4122 [2]�����ڱ�ʶ��ȫ����
 */
typedef struct {
    uint32_t timeLow;  /**< ʱ����ĵ�4�ֽ�  */
    uint16_t timeMid;  /**< ʱ�������2�ֽ�  */
    uint16_t timeHiAndVersion;  /**< ʱ����ĸ�2�ֽ���汾�ŵ����  */
    uint8_t clockSeqAndNode[8];  /**< ʱ��������ڵ��ʶ�������  */
} TEEC_UUID;

/**
 * @ingroup TEEC_COMMON_DATA
 * TEEC_Context�ṹ�����Ͷ���
 *
 * �����ͻ���Ӧ���밲ȫ����֮�佨�������ӻ���
 */
typedef struct {
    uint32_t fd;  /**< �ļ�������  */
    uint8_t *ta_path;
    struct list_node session_list;  /**< �Ự����  */
    struct list_node shrd_mem_list;  /**< �����ڴ�����  */
} TEEC_Context;

/**
 * @ingroup TEEC_COMMON_DATA
 * TEEC_Session�ṹ�����Ͷ���
 *
 * �����ͻ���Ӧ���밲ȫ����֮�佨���ĻỰ
 */
typedef struct {
    uint32_t session_id;  /**< �ỰID���ɰ�ȫ���緵��  */
    TEEC_UUID service_id;  /**< ��ȫ�����UUID��ÿ����ȫ����ӵ��Ψһ��UUID  */
    uint32_t ops_cnt;  /**< �ڻỰ�ڵĲ�����  */
    struct list_node head;  /**< �Ự����ͷ  */
    TEEC_Context* context;  /**< ָ��Ự������TEE����  */
} TEEC_Session;

/**
 * @ingroup TEEC_COMMON_DATA
 * TEEC_SharedMemory�ṹ�����Ͷ���
 *
 * ����һ�鹲���ڴ棬����ע�ᣬҲ���Է���
 */
typedef struct {
    void* buffer;  /**< �ڴ�ָ��  */
    size_t size;  /**< �ڴ��С  */
    uint32_t flags;  /**< �ڴ��ʶ����������������������ȡֵ��ΧΪ#TEEC_SharedMemCtl  */
    uint32_t ops_cnt;  /**< �ڴ������  */
    bool is_allocated;  /**< �ڴ�����ʾ��������������ע��ģ����Ƿ����  */
    struct list_node head;  /**< �����ڴ�����ͷ  */
    TEEC_Context* context;  /**< ָ��������TEE���� */
} TEEC_SharedMemory;

/**
 * @ingroup TEEC_COMMON_DATA
 * TEEC_TempMemoryReference�ṹ�����Ͷ���
 *
 * ����һ����ʱ������ָ��\n
 * ��������#TEEC_Parameter�����ͣ��������Ӧ�����Ϳ�����
 * #TEEC_MEMREF_TEMP_INPUT�� #TEEC_MEMREF_TEMP_OUTPUT����#TEEC_MEMREF_TEMP_INOUT
 */
typedef struct {
    void* buffer;  /**< ��ʱ������ָ��  */
    size_t size;  /**< ��ʱ��������С  */
} TEEC_TempMemoryReference;

/**
 * @ingroup TEEC_COMMON_DATA
 * TEEC_RegisteredMemoryReference�ṹ�����Ͷ���
 *
 * ���������ڴ�ָ�룬ָ������ע������õĹ����ڴ�\n
 * ��������#TEEC_Parameter�����ͣ��������Ӧ�����Ϳ�����
 * #TEEC_MEMREF_WHOLE�� #TEEC_MEMREF_PARTIAL_INPUT��
 * #TEEC_MEMREF_PARTIAL_OUTPUT���� #TEEC_MEMREF_PARTIAL_INOUT
 */
typedef struct {
    TEEC_SharedMemory* parent;  /**< �����ڴ�ָ��  */
    size_t size;  /**< �����ڴ��ʹ�ô�С  */
    size_t offset;  /**< �����ڴ��ʹ��ƫ��  */
} TEEC_RegisteredMemoryReference;

/**
 * @ingroup TEEC_COMMON_DATA
 * TEEC_Value�ṹ�����Ͷ���
 *
 * ��������������\n
 * ��������#TEEC_Parameter�����ͣ��������Ӧ�����Ϳ�����
 * #TEEC_VALUE_INPUT�� #TEEC_VALUE_OUTPUT����#TEEC_VALUE_INOUT
 */
typedef struct {
    uint32_t a;  /**< ��������a  */
    uint32_t b;  /**< ��������b  */
} TEEC_Value;

/**
 * @ingroup TEEC_COMMON_DATA
 * TEEC_Parameter���������Ͷ���
 *
 * ����#TEEC_Operation����Ӧ�Ĳ�������
 */
typedef union {
    TEEC_TempMemoryReference tmpref;  /**< ����#TEEC_TempMemoryReference����  */
    TEEC_RegisteredMemoryReference memref;  /**< ����#TEEC_RegisteredMemoryReference����  */
    TEEC_Value value;  /**< ����#TEEC_Value����  */
} TEEC_Parameter;

/**
 * @ingroup TEEC_COMMON_DATA
 * TEEC_Operation�ṹ�����Ͷ���
 *
 * �򿪻Ự��������ʱ�Ĳ�������ͨ��������������
 * ȡ������Ҳ����ʹ�ô˲���
 */
typedef struct {
    uint32_t started;  /**< ��ʶ�Ƿ���ȡ��������0��ʾȡ������  */
    uint32_t paramTypes;  /**����params�Ĳ�������#TEEC_ParamType����Ҫʹ�ú�#TEEC_PARAM_TYPES��ϲ������� */
    TEEC_Parameter params[4];  /**< �������ݣ�����Ϊ#TEEC_Parameter  */
    TEEC_Session *session;
    bool cancel_flag;
} TEEC_Operation;

/**
 * @ingroup TEEC_COMMON_DATA
 * TEEC_EXT_TEEInfo�ṹ�����Ͷ���
 *
 * ��ȡTEE�İ汾�ź�������Ϣ
 */
typedef struct {
    uint32_t version;
    uint32_t reserve1;
    uint32_t reserve2;
    uint32_t reserve3;
} TEEC_EXT_TEEInfo;
#endif

