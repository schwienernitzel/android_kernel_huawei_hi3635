﻿/*!
 *****************************************************************************
 *
 * @File       rpmb_fcntl.h
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

#ifndef _RPMB_FCNTL_H
#define _RPMB_FCNTL_H

/**
 * @ingroup  TEE_RPMB_API
 * @brief RPMB��ȫ�洢��ʼ��
 *
 * @par ����:
 * ������ʼ����ִ��дRPMB Key�͸�ʽ������
 *
 * @attention �ú���ֻ��ִ��һ��
 * @param ��
 *
 * @retval #TEE_SUCCESS ��ʾ�ú���ִ�гɹ�
 * @retval #TEE_ERROR_RPMB_GENERIC RPMB������ͨ�ô���
 * @retval #TEE_ERROR_RPMB_MAC_FAIL RPMB������MACУ�����
 * @retval #TEE_ERROR_RPMB_RESP_UNEXPECT_MAC RPMBӦ�����ݵ�MACУ�����
 *
 * @par ����:
 * @li rpmb_fcntl.h���ýӿ��������ڵ�ͷ�ļ���
 * @since TrustedCore V100R005C00
*/
TEE_Result TEE_RPMB_FS_Init(void);

/**
 * @ingroup  TEE_RPMB_API
 * @brief RPMB��ȫ�洢��ʽ��
 *
 * @par ����:
 * RPMB��ȫ�洢��ʽ������
 *
 * @attention ��
 * @param ��
 *
 * @retval #TEE_SUCCESS ��ʾ�ú���ִ�гɹ�
 * @retval #TEE_ERROR_RPMB_GENERIC RPMB������ͨ�ô���
 * @retval #TEE_ERROR_RPMB_MAC_FAIL RPMB������MACУ�����
 * @retval #TEE_ERROR_RPMB_RESP_UNEXPECT_MAC RPMBӦ�����ݵ�MACУ�����
 *
 * @par ����:
 * @li rpmb_fcntl.h���ýӿ��������ڵ�ͷ�ļ���
 * @since TrustedCore V100R005C00
*/
TEE_Result TEE_RPMB_FS_Format(void);

/**
 * @ingroup  TEE_RPMB_API
 * @brief RPMB��ȫ�洢�洢�ļ�
 *
 * @par ����:
 * RPMB��ȫ�洢�洢�ļ�
 *
 * @attention ��
 * @param filename [IN]  д�����ݵ��ļ���
 * @param buf [IN]  д�����ݵĻ�����
 * @param size [IN]  д�����ݵĴ�С
 *
 * @retval #TEE_SUCCESS ��ʾ�ú���ִ�гɹ�
 * @retval #TEE_ERROR_BAD_PARAMETERS ����������󣬻��ļ������ȳ���96�ֽ�
 * @retval #TEE_ERROR_RPMB_NOSPC RPMB�������̿ռ䲻��
 *
 * @par ����:
 * @li rpmb_fcntl.h���ýӿ��������ڵ�ͷ�ļ���
 * @since TrustedCore V100R005C00
*/
TEE_Result TEE_RPMB_FS_Write(const char *filename, uint8_t *buf, size_t size);

/**
 * @ingroup  TEE_RPMB_API
 * @brief RPMB��ȫ�洢��ȡ�ļ�
 *
 * @par ����:
 * RPMB��ȫ�洢��ȡ�ļ�
 *
 * @attention ��
 * @param filename [IN]  ��ȡ���ݵ��ļ���
 * @param buf [IN]  ��ȡ���ݵĻ�����
 * @param size [IN]  ��ȡ���ݵĴ�С
 * @param count [OUT]  ʵ�ʶ�ȡ���ݵĴ�С
 *
 * @retval #TEE_SUCCESS ��ʾ�ú���ִ�гɹ�
 * @retval #TEE_ERROR_BAD_PARAMETERS ����������󣬻��ļ������ȳ���96�ֽ�
 * @retval #TEE_ERROR_RPMB_FILE_NOT_FOUND �ļ�������
 *
 * @par ����:
 * @li rpmb_fcntl.h���ýӿ��������ڵ�ͷ�ļ���
 * @since TrustedCore V100R005C00
*/
TEE_Result TEE_RPMB_FS_Read(const char *filename, uint8_t *buf, size_t size, uint32_t *count);

/**
 * @ingroup  TEE_RPMB_API
 * @brief RPMB��ȫ�洢�������ļ�
 *
 * @par ����:
 * RPMB��ȫ�洢�������ļ�
 *
 * @attention ��
 * @param old_name [IN]  ���ļ���
 * @param new_name [IN]  ���ļ���
 *
 * @retval #TEE_SUCCESS ��ʾ�ú���ִ�гɹ�
 * @retval #TEE_ERROR_BAD_PARAMETERS ����������󣬻��ļ������ȳ���96�ֽ�
 * @retval #TEE_ERROR_RPMB_FILE_NOT_FOUND ���ļ�������
 *
 * @par ����:
 * @li rpmb_fcntl.h���ýӿ��������ڵ�ͷ�ļ���
 * @since TrustedCore V100R005C00
*/
TEE_Result TEE_RPMB_FS_Rename(const char *old_name, const char *new_name);

/**
 * @ingroup  TEE_RPMB_API
 * @brief RPMB��ȫ�洢ɾ���ļ�
 *
 * @par ����:
 * RPMB��ȫ�洢ɾ���ļ�
 *
 * @attention ��
 * @param filename [IN]  ��ɾ�����ļ���
 *
 * @retval #TEE_SUCCESS ��ʾ�ú���ִ�гɹ�
 * @retval #TEE_ERROR_BAD_PARAMETERS ����������󣬻��ļ������ȳ���96�ֽ�
 * @retval #TEE_ERROR_RPMB_FILE_NOT_FOUND �ļ�������
 *
 * @par ����:
 * @li rpmb_fcntl.h���ýӿ��������ڵ�ͷ�ļ���
 * @since TrustedCore V100R005C00
*/
TEE_Result TEE_RPMB_FS_Rm(const char *filename);

/**
 * @ingroup  TEE_RPMB_API
 *
 * �洢��RPMB�������ļ�״̬������#TEE_RPMB_FS_Stat���� \n
*/
struct rpmb_fs_stat {
    uint32_t size;    /**< �ļ���С  */
    uint32_t reserved;    /**< Ԥ��  */
};
/**
 * @ingroup  TEE_RPMB_API
 * @brief RPMB��ȫ�洢��ȡ�ļ�״̬
 *
 * @par ����:
 * RPMB��ȫ�洢��ȡ�ļ�״̬
 *
 * @attention ��
 * @param filename [IN]  �ļ���
 * @param stat [OUT]  ��ȡ���ļ�״̬��Ϣ
 *
 * @retval #TEE_SUCCESS ��ʾ�ú���ִ�гɹ�
 * @retval #TEE_ERROR_BAD_PARAMETERS ����������󣬻��ļ������ȳ���96�ֽ�
 * @retval #TEE_ERROR_RPMB_FILE_NOT_FOUND �ļ�������
 *
 * @par ����:
 * @li rpmb_fcntl.h���ýӿ��������ڵ�ͷ�ļ���
 * @since TrustedCore V100R005C00
*/
TEE_Result TEE_RPMB_FS_Stat(const char *filename, struct rpmb_fs_stat *stat);

/**
 * @ingroup  TEE_RPMB_API
 *
 * RPMB Key��״̬������#TEE_RPMB_KEY_Status���� \n
*/
enum TEE_RPMB_KEY_STAT {
    TEE_RPMB_KEY_INVALID = 0x0,
    TEE_RPMB_KEY_SUCCESS = 0x1,    /**< RPMB Key��д����ƥ����ȷ  */
    TEE_RPMB_KEY_NOT_PROGRAM,    /**< RPMB Keyδд��  */
    TEE_RPMB_KEY_NOT_MATCH,        /**< RPMB Key��д�뵫ƥ��ʧ��  */
};
/**
 * @ingroup  TEE_RPMB_API
 * @brief RPMB��ȫ�洢��ȡRPMB Key״̬
 *
 * @par ����:
 * RPMB��ȫ�洢��ȡRPMB Key״̬
 *
 * @attention ��
 * @param ��
 *
 * @retval #TEE_RPMB_KEY_SUCCESS RPMB Key��д����ƥ����ȷ
 * @retval #TEE_RPMB_KEY_NOT_PROGRAM RPMB Keyδд��
 * @retval #TEE_RPMB_KEY_NOT_MATCH RPMB Key��д�뵫ƥ��ʧ��
 * @retval #TEE_RPMB_KEY_INVALID RPMB Key״̬��Ч
 *
 * @par ����:
 * @li rpmb_fcntl.h���ýӿ��������ڵ�ͷ�ļ���
 * @since TrustedCore V100R005C00
*/
uint32_t TEE_RPMB_KEY_Status(void);

#endif //_RPMB_FCNTL_H



