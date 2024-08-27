﻿/*!
 *****************************************************************************
 *
 * @File       dx_infra_api.h
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

#ifndef _DX_INFRA_API_
#define _DX_INFRA_API_

#include "tee_internal_api.h"
#ifdef CC_DRIVER
#include "dx_pal_types.h"
#endif
/**
 * @ingroup dx_infra_api
 * �޷������Ͷ���
 */
typedef unsigned int              DxUint32;
/**
 * @ingroup dx_infra_api
 * �޷����ַ��Ͷ���
 */
typedef unsigned char             DxByte;

#ifndef CC_DRIVER
/**
 * @ingroup dx_infra_api
 * ���ؽ������
 */
typedef TEE_Result                 DxStatus;
/**
 * @ingroup dx_infra_api
 * ״̬ö�ٶ���
 */
typedef enum {
    DX_FALSE = 0,
    DX_TRUE = 1
} DxBool;
#else
/**
 * @ingroup dx_infra_api
 * ״̬��������
 */
typedef DxBool_t DxBool;
#endif
/**
 * @ingroup dx_infra_api
 * ����״̬ö�ٶ���
 */
typedef enum{
    CONNECTED = 0,              /**< 0 ������ */
    UNCONNECTED                 /**< 1 δ���� */
} EDxTzConnectionStatus_t;
/**
 * @ingroup dx_infra_api
 * ��ȫ�ڴ��־λ 0:����ȫ����;1:�ǰ�ȫ�Ͱ�ȫ���ܷ���
 */
typedef enum{
    INIT = 0,                   /**< 0 �����ڴ�Ϊ����ȫ���� */
    TERMINATE                   /**< 1 �����ڴ�Ϊ��ȫ�ͷǰ�ȫ���ܷ��� */
} CP_Flag;
/**
 * @ingroup dx_infra_api
 * ��ȫ�ڴ����ݽṹ����
 */
typedef struct{
    DxByte*     addr;           /**< �����ڴ��ַ */
    DxUint32    size;           /**< �����ڴ泤�� */
    CP_Flag     flag;           /**< �����ڴ��ʶ */
} ContentPath;
/**
 * @ingroup dx_infra_api
 * ��ȫ�ڴ�����ݺ͵�ַ����Ҫ32KB����
 */
#define ALIGN_SIZE 0x8000 //32KB
/**
 * @ingroup  dx_infra_api
 * @brief g_dx_content_path_addr �޸�DX��ȫ�ڴ��ַȫ�ֱ���
 *
 * @par ����:
 * DX��ȫ�ڴ�ͨ��ȫ�ֱ��������ַ���ýӿڻ��ȫ�ֱ��������޸�
 *
 * @attention ȫ�ֱ����(�����ڴ�)ֻ��һ������֧�ֲ�������
 * @param contentpath [IN]  ��ȫ�ڴ����ݽṹ #ContentPath
 *
 * @retval #DX_FALSE ȫ�ֱ�����ֵʧ��
 * @retval #DX_TRUE ȫ�ֱ�����ֵ�ɹ�
 *
 * @par ����:
 * @li dx_infra_api DX����API
 * @li tee_internal_api.h TEE�������ݶ���
 * @see DxTzService_InitSecureContentPath | DxTzService_TerminateSecureContentPath | DxTzService_IsSecureContentMemory
 * @since V100R002C00B302
*/
DxBool g_dx_content_path_addr(ContentPath *contentpath);
/**
 * @ingroup  dx_infra_api
 * @brief DxTzService_IsDeviceRooted �Ӱ�ȫ�����豸�Ƿ�root
 *
 * @par ����:
 * �Ӱ�ȫ�����豸�Ƿ�root�������root��ӵ��rootȨ��
 *
 * @attention ��
 * @param ��
 *
 * @retval 0 û��root
 * @retval 1 �Ѿ�root
 *
 * @par ����:
 * @li dx_infra_api DX����API
 * @li tee_internal_api.h TEE�������ݶ���
 * @see ��
 * @since V100R002C00B302
*/
DxUint32  DxTzService_IsDeviceRooted(void);
/**
 * @ingroup  dx_infra_api
 * @brief DxTzService_IsBootLoaderUnlocked �Ӱ�ȫ�����豸bootloader�Ƿ񱻽���
 *
 * @par ����:
 * �Ӱ�ȫ�����豸bootloader�Ƿ񱻽������������bootloader����������¼������rom
 *
 * @attention ��
 * @param ��
 *
 * @retval 0 bootloaderδ����
 * @retval 1 bootloader�ѽ���
 *
 * @par ����:
 * @li dx_infra_api DX����API
 * @li tee_internal_api.h TEE�������ݶ���
 * @see ��
 * @since V100R002C00B302
*/
DxUint32  DxTzService_IsBootLoaderUnlocked(void);
/**
 * @ingroup  dx_infra_api
 * @brief DxTzService_InitSecureContentPath ���÷ǰ�ȫ�ഫ�ݵ��ڴ�Ϊ����ȫ����
 *
 * @par ����:
 * ���÷ǰ�ȫ�ഫ�ݵ��ڴ�Ϊ����ȫ���ʣ��ǰ�ȫ���ܷ��ʸõ�ַ
 *
 * @attention ��
 * @param ��
 *
 * @retval #TEE_SUCCESS ��ȫ��־λ���óɹ�
 * @retval #TEE_ERROR_BAD_PARAMETERS ��������
 *
 * @par ����:
 * @li dx_infra_api DX����API
 * @li tee_internal_api.h TEE�������ݶ���
 * @see DxTzService_TerminateSecureContentPath | DxTzService_IsSecureContentMemory
 * @since V100R002C00B302
*/
DxStatus DxTzService_InitSecureContentPath(void);
/**
 * @ingroup  dx_infra_api
 * @brief DxTzService_TerminateSecureContentPath ���÷ǰ�ȫ�ഫ�ݵ��ڴ�Ϊ��ȫ�ͷǰ�ȫ���ܷ���
 *
 * @par ����:
 * ���÷ǰ�ȫ�ഫ�ݵ��ڴ�Ϊ��ȫ�ͷǰ�ȫ���ܷ��ʣ���ȫ�໹�ذ�ȫ�ڴ棬ʹ�÷ǰ�ȫ������ͷ�
 *
 * @attention ��
 * @param ��
 *
 * @retval #TEE_SUCCESS ��ȫ��־λ���óɹ�
 * @retval #TEE_ERROR_BAD_PARAMETERS ��������
 *
 * @par ����:
 * @li dx_infra_api DX����API
 * @li tee_internal_api.h TEE�������ݶ���
 * @see DxTzService_InitSecureContentPath | DxTzService_IsSecureContentMemory
 * @since V100R002C00B302
*/
DxStatus DxTzService_TerminateSecureContentPath(void);
/**
 * @ingroup  dx_infra_api
 * @brief DxTzService_IsSecureContentMemory �ڰ�ȫ����һ���ڴ��Ƿ�Ϊ����ȫ����
 *
 * @par ����:
 * �ڰ�ȫ����һ���ڴ��Ƿ�Ϊ����ȫ����
 *
 * @attention ��
 * @param mem [IN] �ڴ��ַ #DxByte
 * @param len [IN] �ڴ泤�� #DxUint32
 *
 * @retval #TEE_SUCCESS ��ȫ��־λ���óɹ�
 * @retval #TEE_ERROR_BAD_PARAMETERS ��������
 *
 * @par ����:
 * @li dx_infra_api DX����API
 * @li tee_internal_api.h TEE�������ݶ���
 * @see DxTzService_InitSecureContentPath | DxTzService_IsSecureContentMemory
 * @since V100R002C00B302
*/
DxBool DxTzService_IsSecureContentMemory(DxByte* mem, DxUint32 len);
#endif

