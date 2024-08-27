﻿/*!
 *****************************************************************************
 *
 * @File       tee_client_storage_api.h
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

#ifndef _TEE_CLIENT_STORAGE_API_H_
#define _TEE_CLIENT_STORAGE_API_H_

#include "tee_client_type.h"

#define SFS_PROPERTY "trustedcore_sfs_property"
#define SFS_PROPERTY_VALUE "1"
/**
 * @ingroup  TEEC_StorageAPI
 *
 * ���ļ���ģʽ
 */
enum Data_Flag_Constants{
    TEE_DATA_FLAG_ACCESS_READ = 0x00000001,  /**< ֻ��ģʽ */
    TEE_DATA_FLAG_ACCESS_WRITE = 0x00000002,  /**< ֻдģʽ */
    TEE_DATA_FLAG_ACCESS_WRITE_META = 0x00000004,  /**< ������ļ�����ɾ�������������
    ����ʹ�ô�ģʽ�ȴ��ļ� */
    TEE_DATA_FLAG_SHARE_READ = 0x00000010,  /**< �����ģʽ ��
    ֻ���ڴ�ģʽ�£��ſ����Զ���ʽͬʱ��ͬһ���ļ� */
    TEE_DATA_FLAG_SHARE_WRITE = 0x00000020,  /**< ����дģʽ��
    ֻ���ڴ�ģʽ�£��ſ�����д��ʽͬʱ��ͬһ���ļ� */
    TEE_DATA_FLAG_CREATE = 0x00000200,  /**< �����ļ��������
    ���������ļ����������򸲸�ԭ���ļ� */
    TEE_DATA_FLAG_EXCLUSIVE = 0x00000400,  /**< ����ļ������ڣ�
    �򴴽��ļ������򱨴� */
};

/**
 * @ingroup  TEEC_StorageAPI
 * @brief �򿪰�ȫ�洢����
 *
 * @par ����:
 * �򿪰�ȫ�洢����ɹ����Ƕ��ļ�������ǰ��
 *
 * @attention ��
 * @param ��
 *
 * @retval #TEEC_SUCCESS �򿪰�ȫ�洢����ɹ�
 * @retval ��������ֵ�ο� #TEEC_ReturnCode
 *
 * @par ����:
 * @li libteec���ýӿ������Ĺ����
 * @li tee_client_storage_api.h���ýӿ���������ͷ�ļ�
 * @see TEEC_UninitStorageService
 * @since V100R002C00B302
 */
TEEC_Result TEEC_InitStorageService();

/**
 * @ingroup  TEEC_StorageAPI
 * @brief �رհ�ȫ�洢����
 *
 * @par ����:
 * �رհ�ȫ�洢����
 *
 * @attention ��
 * @param ��
 *
 * @retval #TEEC_SUCCESS �رհ�ȫ�洢����ɹ�
 * @retval #TEEC_ERROR_BAD_STATE ��ȫ�洢�����δ��
 *
 * @par ����:
 * @li libteec���ýӿ������Ĺ����
 * @li tee_client_storage_api.h���ýӿ���������ͷ�ļ�
 * @see TEEC_InitStorageService
 * @since V100R002C00B302
 */
TEEC_Result TEEC_UninitStorageService();

/**
 * @ingroup  TEEC_StorageAPI
 * @brief ���ļ�
 *
 * @par ����:
 * ��·��Ϊpath���ļ�����ģʽ��mode��ȡֵ��ΧΪ#Data_Flag_Constants
 *
 * @attention ��
 * @param path [IN] �ļ�·��
 * @param mode [IN] ���ļ��ķ�ʽ��ȡֵ��ΧΪ#Data_Flag_Constants
 *
 * @retval -1 ���ļ�ʧ�ܣ�û�г�ʼ����ȫ�洢���񡢻��ļ������ڡ�
 * �򲢷������ļ�ʱȨ��У��ʧ�ܣ���ͨ��#TEEC_GetFErr��ȡ������
 * @retval >=0 ���ļ��ɹ��������ļ�������
 *
 * @par ����:
 * @li libteec���ýӿ������Ĺ����
 * @li tee_client_storage_api.h���ýӿ���������ͷ�ļ�
 * @see TEEC_FClose
 * @since V100R002C00B302
 */
int32_t TEEC_FOpen(char* path, uint32_t mode);

/**
 * @ingroup  TEEC_StorageAPI
 * @brief �ر��ļ�
 *
 * @par ����:
 * �ر�fd��ָ����ļ�
 *
 * @attention ��
 * @param fd [IN] �ļ�������
 *
 * @retval -1 �ر��ļ�ʧ�ܣ�û�г�ʼ����ȫ�洢���񡢻����fd��Ч��
 * ��ͨ��#TEEC_GetFErr��ȡ������
 * @retval 0 �ر��ļ��ɹ�
 *
 * @par ����:
 * @li libteec���ýӿ������Ĺ����
 * @li tee_client_storage_api.h���ýӿ���������ͷ�ļ�
 * @see TEEC_FOpen
 * @since V100R002C00B302
 */
int32_t TEEC_FClose(int32_t fd);

/**
 * @ingroup  TEEC_StorageAPI
 * @brief ��ȡ�ļ�
 *
 * @par ����:
 * �Ӳ���fd��ָ���ļ���ȡ���ݵ�buf�У�����ȡsize���ֽ�
 *
 * @attention ��
 * @param fd [IN] �ļ�������
 * @param buf [IN] �ļ���ȡ���ݻ�����
 * @param size [IN] ��Ҫ��ȡ�Ĵ�С�����ֽ�Ϊ��λ�����ܴ���bufָ��Ļ�����
 *
 * @retval С��size ʵ�ʶ�ȡ���Ĵ�С����ȡ�ļ�ʱ��������������ļ�β��
 * ��ͨ������#TEEC_GetFErr��ȡ������
 * @retval ����size ��ȡ�ɹ�
 *
 * @par ����:
 * @li libteec���ýӿ������Ĺ����
 * @li tee_client_storage_api.h���ýӿ���������ͷ�ļ�
 * @see TEEC_FOpen | TEEC_FClose | TEEC_GetFErr
 * @since V100R002C00B302
 */
size_t TEEC_FRead(int32_t fd, uint8_t* buf, size_t size);

/**
 * @ingroup  TEEC_StorageAPI
 * @brief д���ļ�
 *
 * @par ����:
 * �����fd��ָ���ļ�д������buf�����д��size���ֽ�
 *
 * @attention ��
 * @param fd [IN] �ļ�������
 * @param buf [IN] �ļ�д�����ݻ�����
 * @param size [IN] ��Ҫд��Ĵ�С�����ֽ�Ϊ��λ�����ܴ���bufָ��Ļ�����
 *
 * @retval С��size ʵ��д��Ĵ�С��д���ļ�ʱ��������������ļ�β��
 * ��ͨ������#TEEC_GetFErr��ȡ������
 * @retval ����size д��ɹ�
 *
 * @par ����:
 * @li libteec���ýӿ������Ĺ����
 * @li tee_client_storage_api.h���ýӿ���������ͷ�ļ�
 * @see TEEC_FOpen | TEEC_FClose | TEEC_GetFErr
 * @since V100R002C00B302
 */
size_t TEEC_FWrite(int32_t fd, uint8_t* buf, size_t size);
/**
 * @ingroup  TEEC_StorageAPI
 * @brief �ض�λ�ļ�
 *
 * @par ����:
 * �ض�λ�ļ�
 *
 * @attention ��
 * @param ��
 *
 * @retval #TEEC_SUCCESS ��ȡ�ɹ�
 * @retval
 * @retval ��������ֵ�ο� #TEEC_ReturnCode
 *
 * @par ����:
 * @li libteec���ýӿ������Ĺ����
 * @li tee_client_storage_api.h���ýӿ���������ͷ�ļ�
 * @see TEEC_FOpen
 * @since V100R003C00B061
 */
int32_t TEEC_FSeek(int32_t fd, int32_t offset, int32_t fromwhere);
/**
 * @ingroup  TEEC_StorageAPI
 * @brief ��ȡ�ļ���Ϣ
 *
 * @par ����:
 * ��ȡ��ǰ�ļ�λ���Լ��ļ�����
 *
 * @attention ��
 * @param ��
 *
 * @retval #TEEC_SUCCESS ��ȡ�ɹ�
 * @retval
 * @retval ��������ֵ�ο� #TEEC_ReturnCode
 *
 * @par ����:
 * @li libteec���ýӿ������Ĺ����
 * @li tee_client_storage_api.h���ýӿ���������ͷ�ļ�
 * @see TEEC_FOpen
 * @since V100R003C00B061
 */
size_t TEEC_FInfo(int32_t fd, uint32_t* pos, uint32_t* len);
/**
 * @ingroup  TEEC_StorageAPI
 * @brief �رղ�ɾ���ļ�
 *
 * @par ����:
 * �رղ�ɾ����Ӧ�ļ�
 *
 * @attention ��
 * @param ��
 *
 * @retval #TEEC_SUCCESS ɾ���ɹ�
 * @retval
 * @retval ��������ֵ�ο� #TEEC_ReturnCode
 *
 * @par ����:
 * @li libteec���ýӿ������Ĺ����
 * @li tee_client_storage_api.h���ýӿ���������ͷ�ļ�
 * @see TEEC_FOpen
 * @since V100R003C00B061
 */
int32_t TEEC_FCloseAndDelete(int32_t fd);

/**
 * @ingroup  TEEC_StorageAPI
 * @brief sync�ļ���flash
 *
 * @par ����:
 * sync�ļ���flash
 *
 * @attention ��
 * @param ��
 *
 * @retval #TEEC_SUCCESS ɾ���ɹ�
 * @retval
 * @retval ��������ֵ�ο� #TEEC_ReturnCode
 *
 * @par ����:
 * @li libteec���ýӿ������Ĺ����
 * @li tee_client_storage_api.h���ýӿ���������ͷ�ļ�
 * @see TEEC_FOpen
 * @since V100R003C00B061
 */
int32_t TEEC_FSync(int32_t fd);

/**
 * @ingroup  TEEC_StorageAPI
 * @brief ��ȡ������
 *
 * @par ����:
 * ��ȡ��ȫ�洢����Ĵ�����
 *
 * @attention ��
 * @param ��
 *
 * @retval #TEEC_SUCCESS û�д������
 * @retval #TEEC_ERROR_BAD_STATE ��ȫ�洢�����δ��
 * @retval ��������ֵ�ο� #TEEC_ReturnCode
 *
 * @par ����:
 * @li libteec���ýӿ������Ĺ����
 * @li tee_client_storage_api.h���ýӿ���������ͷ�ļ�
 * @see TEEC_FRead | TEEC_FWrite
 * @since V100R002C00B302
 */
TEEC_Result TEEC_GetFErr();

#endif

