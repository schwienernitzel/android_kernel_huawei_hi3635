﻿/*!
 *****************************************************************************
 *
 * @File       tee_common.h
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

/** @defgroup TEE_API ��ȫ��ӿ� */
/** @defgroup TEE_COMMON_DATA �������ݶ���
* @ingroup TEE_API
*/

#ifndef __TEE_COMMON_H_
#define __TEE_COMMON_H_

#define CIPHER_ENCRYPT 0
#define CIPHER_DECRYPT 1

#define MD5_OUTPUT_LEN 16
#define SHA1_OUTPUT_LEN 20
#define SHA224_OUTPUT_LEN 28
#define SHA256_OUTPUT_LEN 32
#define SHA384_OUTPUT_LEN 48
#define SHA512_OUTPUT_LEN 64
#define HMAC_KEY_LEN 16
#define HMAC_DATA_LEN 50
#define HMAC_OUTPUT_LEN 16
#define AES_128_CBC_LEN 16
#define AES_128_ECB_LEN 16
#define AES_128_CTR_LEN 16
#define AES_128_XTS_LEN 16
#define DES_ECB_LEN 8
#define DES_CBC_LEN 8
#define DES3_CBC_LEN 8
#define DES3_ECB_LEN 8
#define IGNORE_PARAM  0xff
#if 1
/**
 * @ingroup  TEE_COMMON_DATA
 * ��Կ����ʱ����ṹ������ֽ�
 *
*/
#define TEE_DK_MAX_SIZE_OF_OTHER_INFO  64 /*bytes*/
#endif

/**
 * @ingroup  TEE_COMMON_DATA
 *
 * ��ȫ����Global
 */
#define TEE_SERVICE_GLOBAL \
{ \
    0x00000000, \
    0x0000, \
    0x0000, \
    { \
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 \
    } \
}

/**
 * @ingroup  TEE_COMMON_DATA
 *
 * ��ȫ����Echo
 */
#define TEE_SERVICE_ECHO \
{ \
    0x01010101, \
    0x0101, \
    0x0101, \
    { \
        0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01 \
    } \
}

/**
 * @ingroup  TEE_COMMON_DATA
 *
 * ��ȫ�洢����
 */
#define TEE_SERVICE_STORAGE \
{ \
    0x02020202, \
    0x0202, \
    0x0202, \
    { \
        0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02 \
    } \
}

/**
 * @ingroup  TEE_COMMON_DATA
 *
 * ��ȫ����UT����
 */
#define TEE_SERVICE_UT \
{ \
    0x03030303, \
    0x0303, \
    0x0303, \
    { \
        0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03 \
    } \
}

/**
 * @ingroup  TEE_COMMON_DATA
 *
 * �ӽ��ܷ���
 */
#define TEE_SERVICE_CRYPT \
{ \
    0x04040404, \
    0x0404, \
    0x0404, \
    { \
        0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04 \
    } \
}

/**
 * @ingroup  TEE_COMMON_DATA
 *
 * ��ȫ����global֧�ֵ�����ID
 */
enum global_service_cmd_id {
    GLOBAL_CMD_ID_INVALID = 0x0,    /**< Global Task ��ЧID*/
    GLOBAL_CMD_ID_BOOT_ACK,         /**< Global Task ����Ӧ��*/
    GLOBAL_CMD_ID_OPEN_SESSION,     /**< Global Task ��Session*/
    GLOBAL_CMD_ID_CLOSE_SESSION,    /**< Global Task �ر�Session*/
    GLOBAL_CMD_ID_LOAD_SECURE_APP,  /**< Global Task ��̬���ذ�ȫӦ��*/
    GLOBAL_CMD_ID_NEED_LOAD_APP,    /**< Global Task �ж��Ƿ�����Ҫ���ذ�ȫӦ��*/
    GLOBAL_CMD_ID_REGISTER_AGENT,   /**< Global Task ע�����*/
    GLOBAL_CMD_ID_UNREGISTER_AGENT, /**< Global Task ע������*/
    GLOBAL_CMD_ID_REGISTER_NOTIFY_MEMORY,   /**< Global Task ע���첽���û�����*/
    GLOBAL_CMD_ID_UNREGISTER_NOTIFY_MEMORY, /**< Global Task ע���첽���û�����*/
    GLOBAL_CMD_ID_INIT_CONTENT_PATH,      /**< Global Task��ʼ��content path*/
    GLOBAL_CMD_ID_TERMINATE_CONTENT_PATH,   /**< Global Task�ͷ�content path*/
    GLOBAL_CMD_ID_ALLOC_EXCEPTION_MEM,  /**< Global Task �����쳣��Ϣ����ռ�*/
    GLOBAL_CMD_ID_TEE_TIME,         /**< Global Task ��ȡ��ȫOS��ʱ�� */
    GLOBAL_CMD_ID_UNKNOWN         = 0x7FFFFFFE, /**< Global Task δ֪ID*/
    GLOBAL_CMD_ID_MAX             = 0x7FFFFFFF  /**< Global Task ���ID*/
};

/**
 * @ingroup  TEE_COMMON_DATA
 *
 * ��ȫ����echo֧�ֵ�����ID
 */
enum echo_service_cmd_id {
    ECHO_CMD_ID_INVALID = 0x10,     /**< Echo Task ��ЧID*/
    ECHO_CMD_ID_SEND_CMD,           /**< Echo Task��������*/
    ECHO_CMD_ID_UNKNOWN         = 0x7FFFFFFE,   /**< Echo Task δ֪ID*/
    ECHO_CMD_ID_MAX             = 0x7FFFFFFF    /**< Echo Task ���ID*/
};

/**
 * @ingroup  TEE_COMMON_DATA
 *
 * ��ȫ����CRYPTO֧�ֵ�����ID, �����ԳƼӽ��ܡ��ǶԳƼӽ��ܡ�ժҪHMAC��\n
 * ע��:�Գ��㷨�е�nopadģʽ��ҪTA(Trusted Application������Ӧ��)�������
 */
enum crypt_service_cmd_id {
    CRYPT_CMD_ID_INVALID = 0x10,            /**< ��ЧID */
    //CRYPT_CMD_ID_LOAD_LIBS,               /**< ���ؼӽ��ܿ� */
    //CRYPT_CMD_ID_UNLOAD_LIBS,             /**< ж�ؼӽ��ܿ� */
    CRYPT_CMD_ID_ENCRYPT,                   /**< ���� */
    CRYPT_CMD_ID_DECRYPT,                   /**< ���� */
    CRYPT_CMD_ID_MD5,                       /**< ժҪ�㷨MD5 */
    CRYPT_CMD_ID_SHA1,                      /**< ժҪ�㷨SHA1 */
    CRYPT_CMD_ID_SHA224,                    /**< ժҪ�㷨SHA224 */
    CRYPT_CMD_ID_SHA256,                    /**< ժҪ�㷨SHA256 */
    CRYPT_CMD_ID_SHA384,                    /**< ժҪ�㷨SHA384 */
    CRYPT_CMD_ID_SHA512,                    /**< ժҪ�㷨SHA512 */
    CRYPT_CMD_ID_HMAC_MD5,                  /**< HMAC MD5 */
    CRYPT_CMD_ID_HMAC_SHA1,                 /**< HMAC SHA1 */
    CRYPT_CMD_ID_HMAC_SHA224,               /**< HMAC SHA224 */
    CRYPT_CMD_ID_HMAC_SHA256,               /**< HMAC SHA256 */
    CRYPT_CMD_ID_HMAC_SHA384,               /**< HMAC SHA384 */
    CRYPT_CMD_ID_HMAC_SHA512,               /**< HMAC SHA512 */
    CRYPT_CMD_ID_CIPHER_AES_CBC,        /**< �ԳƼӽ���AES 128bits��Կ CBCģʽ */
    CRYPT_CMD_ID_CIPHER_AES_CBC_CTS,    /**< �ԳƼӽ���AES 128bits��Կ CBC_CTSģʽ */
    CRYPT_CMD_ID_CIPHER_AES_ECB,        /**< �ԳƼӽ���AES 128bits��Կ ECBģʽ */
	CRYPT_CMD_ID_CIPHER_AES_ECB_PKCS5,
    CRYPT_CMD_ID_CIPHER_AES_CTR,        /**< �ԳƼӽ���AES 128bits��Կ CTRģʽ */
    CRYPT_CMD_ID_CIPHER_AES_CBC_MAC,    /**< �ԳƼӽ���AES 128bits��Կ CBC_MACģʽ */
    CRYPT_CMD_ID_CIPHER_AES_XCBC_MAC,   /**< �ԳƼӽ���AES 128bits��Կ XCBC_MACģʽ */
    CRYPT_CMD_ID_CIPHER_AES_CMAC,       /**< �ԳƼӽ���AES 128bits��Կ CMACģʽ */
    CRYPT_CMD_ID_CIPHER_AES_CCM,        /**< �ԳƼӽ���AES 128bits��Կ CCMģʽ */
    CRYPT_CMD_ID_CIPHER_AES_XTS,        /**< �ԳƼӽ���AES 128bits��Կ XTSģʽ */
    CRYPT_CMD_ID_CIPHER_DES_ECB,            /**< �ԳƼӽ���DES ECBģʽ */
    CRYPT_CMD_ID_CIPHER_DES_CBC,            /**< �ԳƼӽ���DES CBCģʽ */
    CRYPT_CMD_ID_CIPHER_DES3_ECB,           /**< �ԳƼӽ���DES3 ECBģʽ */
    CRYPT_CMD_ID_CIPHER_DES3_CBC,           /**< �ԳƼӽ���DES3 CBCģʽ */
    CRYPT_CMD_ID_CIPHER_RND,                /**< �����ģʽ */
    CRYPT_CMD_ID_CIPHER_DK,                 /**< ��Կ����ģʽ */
    CRYPT_CMD_ID_RSAES_PKCS1_V1_5,          /**< �ǶԳƼӽ���PKCS1_V1_5ģʽ */
    CRYPT_CMD_ID_RSAES_PKCS1_OAEP_MGF1_SHA1,/**< �ǶԳƼӽ���OAEP_SHA1ģʽ */
    CRYPT_CMD_ID_RSAES_PKCS1_OAEP_MGF1_SHA224,/**< �ǶԳƼӽ���OAEP_SHA224ģʽ */
    CRYPT_CMD_ID_RSAES_PKCS1_OAEP_MGF1_SHA256,/**< �ǶԳƼӽ���OAEP_SHA256ģʽ */
    CRYPT_CMD_ID_RSAES_PKCS1_OAEP_MGF1_SHA384,/**< �ǶԳƼӽ���OAEP_SHA384ģʽ */
    CRYPT_CMD_ID_RSAES_PKCS1_OAEP_MGF1_SHA512,/**< �ǶԳƼӽ���OAEP_SHA512ģʽ */
    CRYPT_CMD_ID_RSA_NOPAD,                 /**< �ǶԳƼӽ��������ģʽ */
    CRYPT_CMD_ID_RSASSA_PKCS1_V1_5_MD5,     /**< �ǶԳ�ǩ����֤PKCS1_V1_5_MD5ģʽ */
    CRYPT_CMD_ID_RSASSA_PKCS1_V1_5_SHA1,    /**< �ǶԳ�ǩ����֤PKCS1_V1_5_SHA1ģʽ */
    CRYPT_CMD_ID_RSASSA_PKCS1_V1_5_SHA224,  /**< �ǶԳ�ǩ����֤PKCS1_V1_5_SHA224ģʽ */
    CRYPT_CMD_ID_RSASSA_PKCS1_V1_5_SHA256,  /**< �ǶԳ�ǩ����֤PKCS1_V1_5_SHA256ģʽ */
    CRYPT_CMD_ID_RSASSA_PKCS1_V1_5_SHA384,  /**< �ǶԳ�ǩ����֤PKCS1_V1_5_SHA384ģʽ */
    CRYPT_CMD_ID_RSASSA_PKCS1_V1_5_SHA512,  /**< �ǶԳ�ǩ����֤PKCS1_V1_5_SHA512ģʽ */
    CRYPT_CMD_ID_RSASSA_PKCS1_PSS_MGF1_SHA1,/**< �ǶԳ�ǩ����֤PSS_MGF1_SHA1ģʽ */
    CRYPT_CMD_ID_RSASSA_PKCS1_PSS_MGF1_SHA224,/**< �ǶԳ�ǩ����֤PSS_MGF1_SHA224ģʽ */
    CRYPT_CMD_ID_RSASSA_PKCS1_PSS_MGF1_SHA256,/**< �ǶԳ�ǩ����֤PSS_MGF1_SHA256ģʽ */
    CRYPT_CMD_ID_RSASSA_PKCS1_PSS_MGF1_SHA384,/**< �ǶԳ�ǩ����֤PSS_MGF1_SHA384ģʽ */
    CRYPT_CMD_ID_RSASSA_PKCS1_PSS_MGF1_SHA512,/**< �ǶԳ�ǩ����֤PSS_MGF1_SHA512ģʽ */
    CRYPT_CMD_ID_DSA_SHA1,                  /**< �ǶԳ�ǩ����֤DSAģʽ */
    CRYPT_CMD_ID_UNKNOWN = 0x7FFFFFFE,      /**< δ֪ģʽ */
    CRYPT_CMD_ID_MAX = 0x7FFFFFFF           /**< ���CMD ID */
};

/**
 * @ingroup  TEE_COMMON_DATA
 *
 * ��ȫ����CRYPTO״̬����
 */
typedef enum crypt_cmd_status{
    CRYPT_INIT = 0,         /**< ��ʼ�� */
    CRYPT_UPDATE,           /**< ������ */
    CRYPT_UPDATEAAD,        /**< AE����֤���ݿ����� */
    CRYPT_DOFINAL,          /**< ���� */
    CRYPT_CMPFINAL,         /**< ��������֤ */
    CRYPT_SIGNDIGEST,       /**< ǩ�� */
    CRYPT_VERIFYDIGEST,     /**< ��֤ */
    CRYPT_STATUS_NUM        /**< ״̬���� */
} crypt_status;

/**
 * @ingroup  TEE_COMMON_DATA
 *
 * ��ȫ����STORAGE֧�ֵ�����ID
 */
enum storage_service_cmd_id {
    STORAGE_CMD_ID_INVALID = 0x10,          /**< Storage Task ��ЧID*/
    STORAGE_CMD_ID_OPEN,                    /**< Storage Task���ļ�*/
    STORAGE_CMD_ID_CLOSE,                   /**< Storage Task�ر��ļ�*/
    STORAGE_CMD_ID_CLOSEALL,                /**< Storage Task�ر������ļ�*/
    STORAGE_CMD_ID_READ,                    /**< Storage Task��ȡ�ļ�*/
    STORAGE_CMD_ID_WRITE,                   /**< Storage Taskд���ļ�*/
    STORAGE_CMD_ID_SEEK,                    /**< Storage Task��ȡ��ǰ�ļ�λ��*/
    STORAGE_CMD_ID_TELL,                    /**< Storage Task�ض�λ�ļ�*/
    STORAGE_CMD_ID_TRUNCATE,                /**< Storage Task�ı��ļ���С*/
    STORAGE_CMD_ID_REMOVE,                  /**< Storage Taskɾ���ļ�*/
    STORAGE_CMD_ID_FINFO,                   /**< Storage Task�����ļ�״̬*/
    STORAGE_CMD_ID_FSYNC,                   /**< Storage Taskͬ���ļ����洢�豸*/
    STORAGE_CMD_ID_UNKNOWN = 0x7FFFFFFE,    /**< Storage Task δ֪ID*/
    STORAGE_CMD_ID_MAX = 0x7FFFFFFF         /**< Storage Task ���ID*/
};
#if 1
/**
 * ingroup  TEE_COMMON_DATA
 *
 * tee DK other structure, containing the optional data for KDF,
 * if any data is not needed, then the pointer value and
 * the size must be set to NULL
 */
typedef struct
{
    /* a unique object identifier (OID), indicating algorithm(s)
    for which the keying data will be used*/
    unsigned char    AlgorithmID[TEE_DK_MAX_SIZE_OF_OTHER_INFO];    /**< a unique object identifier (OID) */
    unsigned int   SizeOfAlgorithmID;                           /**< size of AlgorithmID */
    /* Public information contributed by the initiator */
    unsigned char    PartyUInfo[TEE_DK_MAX_SIZE_OF_OTHER_INFO]; /**< Public information contributed by the initiator */
    unsigned int   SizeOfPartyUInfo;                            /**< size of PartyUInfo */
    /* Public information contributed by the responder */
    unsigned char    PartyVInfo[TEE_DK_MAX_SIZE_OF_OTHER_INFO]; /**< Public information contributed by the responder */
    unsigned int   SizeOfPartyVInfo;                            /**< size of PartyVInfo */
    /* Mutually-known private information, e.g. shared information
    communicated throgh a separate channel */
    unsigned char    SuppPrivInfo[TEE_DK_MAX_SIZE_OF_OTHER_INFO];   /**< Mutually-known private information */
    unsigned int   SizeOfSuppPrivInfo;                          /**< size of SuppPrivInfo */
    /* Mutually-known public information, */
    unsigned char    SuppPubInfo[TEE_DK_MAX_SIZE_OF_OTHER_INFO];    /**< Mutually-known public information */
    unsigned int   SizeOfSuppPubInfo;                           /**< size of SuppPubInfo */
}tee_DK_OtherInfo;

/**
 * ingroup  TEE_COMMON_DATA
 *
 * TEE����Կ��������ģʽ����
*/
typedef enum
{
    TEE_DK_ASN1_DerivMode = 0,                          /**< ASN1_DerivMode */
    TEE_DK_ConcatDerivMode = 1,                         /**< ConcatDerivMode */
    TEE_DK_X963_DerivMode = TEE_DK_ConcatDerivMode,     /**< X963_DerivMode */
    TEE_DK_OMADRM_DerivMode = 2,                        /**< OMADRM_DerivMode */
    TEE_DK_ISO18033_KDF1_DerivMode = 3,                 /**< ISO18033_KDF1_DerivMode */
    TEE_DK_ISO18033_KDF2_DerivMode = 4,                 /**< ISO18033_KDF2_DerivMode */
    TEE_DK_DerivFunc_NumOfModes,                        /**< num of modes */
}tee_DK_DerivFuncMode;

/**
 * ingroup  TEE_COMMON_DATA
 *
 * TEE����Կ����HASHģʽ����
*/
typedef enum
{
    TEE_DK_HASH_SHA1_mode = 0,              /**< HASH_SHA1 */
    TEE_DK_HASH_SHA224_mode = 1,            /**< HASH_SHA224 */
    TEE_DK_HASH_SHA256_mode = 2,            /**< HASH_SHA256 */
    TEE_DK_HASH_SHA384_mode = 3,            /**< HASH_SHA384 */
    TEE_DK_HASH_SHA512_mode = 4,            /**< HASH_SHA512 */
    TEE_DK_HASH_NumOfModes,                 /**< num of modes */
}tee_DK_HASH_Mode;
#endif
#endif


