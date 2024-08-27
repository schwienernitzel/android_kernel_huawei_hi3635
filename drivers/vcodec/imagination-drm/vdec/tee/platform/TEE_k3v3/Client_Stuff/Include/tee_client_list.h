﻿/*!
 *****************************************************************************
 *
 * @File       tee_client_list.h
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

#ifndef _TEE_CLIENT_LIST_H_
#define _TEE_CLIENT_LIST_H_

/**
 * @ingroup TEEC_List
 * �������Ͷ���
 */
struct list_node {
    struct list_node *next;  /**< ָ��next�ڵ�  */
    struct list_node *prev;  /**< ָ��prev�ڵ�  */
};

/**
 * @ingroup TEEC_List
 * @brief ����һ������ڵ�
 *
 * @par ����:
 * �궨�壬����һ������ڵ㣬�������ʼ��
 * @param name [IN] �������������
 */
#define LIST_DECLARE(name) \
    struct list_node name = { \
        .next = &name, \
        .prev = &name, \
    }

/**
 * @ingroup  TEEC_List
 * @brief ��ʼ������
 *
 * @par ����:
 * ������ͷ���г�ʼ������
 *
 * @attention ��
 * @param list [IN/OUT]����ָ�룬ȡֵ����Ϊ��
 *
 * @retval ��
 *
 * @par ����:
 * @li libteec���ýӿ������Ĺ����
 * @li tee_client_list.h���ýӿ���������ͷ�ļ�
 * @since V100R002C00B301
 */
static /*DAB inline*/ void INIT_LIST_HEAD(struct list_node *list)
{
    list->next = list;
    list->prev = list;
}

/**
 * @ingroup TEEC_List
 * ��ȡ����list��next�ڵ�
 */
#define LIST_HEAD(list) ((list)->next)

/**
 * @ingroup TEEC_List
 * ��ȡ����list��prev�ڵ�
 */
#define LIST_TAIL(list) ((list)->prev)


/**
 * @ingroup TEEC_List
 * �ж�����list�Ƿ�Ϊ��
 */
#define LIST_EMPTY(list) ((list) == (list)->next)

/**
 * @ingroup  TEEC_List
 * @brief ������ͷ�������½ڵ�
 *
 * @par ����:
 * ������ͷ�������½ڵ�
 *
 * @attention ��
 * @param list [IN/OUT]����ͷָ�룬ȡֵ����Ϊ��
 * @param entry [IN/OUT]��������ڵ�ָ�룬ȡֵ����Ϊ��
 *
 * @retval ��
 *
 * @par ����:
 * @li libteec���ýӿ������Ĺ����
 * @li tee_client_list.h���ýӿ���������ͷ�ļ�
 * @see list_insert_tail
 * @since V100R002C00B301
 */
static /*DAB inline*/ void list_insert_head(struct list_node *list, struct list_node *entry)
{
    list->next->prev = entry;
    entry->next = list->next;
    entry->prev = list;
    list->next = entry;
}

/**
 * @ingroup  TEEC_List
 * @brief ������β�������½ڵ�
 *
 * @par ����:
 * ������β�������½ڵ�
 *
 * @attention ��
 * @param list [IN/OUT]����ͷָ�룬ȡֵ����Ϊ��
 * @param entry [IN/OUT]��������ڵ�ָ�룬ȡֵ����Ϊ��
 *
 * @retval ��
 *
 * @par ����:
 * @li libteec���ýӿ������Ĺ����
 * @li tee_client_list.h���ýӿ���������ͷ�ļ�
 * @see list_insert_head
 * @since V100R002C00B301
 */
static /*DAB inline*/ void list_insert_tail(struct list_node *list, struct list_node *entry)
{
    entry->next = list;
    entry->prev = list->prev;
    list->prev->next = entry;
    list->prev = entry;
}

/**
 * @ingroup  TEEC_List
 * @brief ɾ���ڵ�
 *
 * @par ����:
 * ɾ��ָ���Ľڵ�
 *
 * @attention ���غ��û���Ҫ�ͷ�ɾ���ڵ���ڴ�
 * @param entry [IN]��ɾ��������ڵ�ָ�룬ȡֵ����Ϊ��
 *
 * @retval ��
 *
 * @par ����:
 * @li libteec���ýӿ������Ĺ����
 * @li tee_client_list.h���ýӿ���������ͷ�ļ�
 * @see ��
 * @since V100R002C00B301
 */
static /*DAB inline*/ void list_remove(struct list_node *entry)
{
    entry->prev->next = entry->next;
    entry->next->prev = entry->prev;
}

/**
 * @ingroup  TEEC_List
 * @brief ɾ������ͷ���
 *
 * @par ����:
 * ɾ��ָ�������ͷ�ڵ�
 *
 * @attention ���غ��û������ͷ�ɾ���ڵ���ڴ�
 * @param list [IN]����ͷָ�룬ȡֵ����Ϊ��
 *
 * @retval #NULL ����Ϊ��
 * @retval ��NULL ����ͷ���
 *
 * @par ����:
 * @li libteec���ýӿ������Ĺ����
 * @li tee_client_list.h���ýӿ���������ͷ�ļ�
 * @see list_remove_tail
 * @since V100R002C00B301
 */
static /*DAB inline*/ struct list_node* list_remove_head(struct list_node *list)
{
    struct list_node* entry = NULL;
    if (!LIST_EMPTY(list)) {
        entry = list->next;
        list_remove(entry);
    }
    return entry;
}

/**
 * @ingroup  TEEC_List
 * @brief ɾ������β���
 *
 * @par ����:
 * ɾ��ָ�������β�ڵ�
 *
 * @attention ���غ��û������ͷ�ɾ���ڵ���ڴ�
 * @param list [IN]����ͷָ�룬ȡֵ����Ϊ��
 *
 * @retval NULL ����Ϊ��
 * @retval �ǿ� ����β���
 *
 * @par ����:
 * @li libteec���ýӿ������Ĺ����
 * @li tee_client_list.h���ýӿ���������ͷ�ļ�
 * @see list_remove_head
 * @since V100R002C00B301
 */
static /*DAB inline*/ struct list_node* list_remove_tail(struct list_node *list)
{
    struct list_node* entry = NULL;
    if (!LIST_EMPTY(list)) {
        entry = list->prev;
        list_remove(entry);
    }
    return entry;
}

/**
 * @ingroup  TEEC_List
 * @brief �ӳ�Աmember��ָ��ptr�ҵ����������ݽṹtype��ָ��
 *
 * @par ����:
 * �ӳ�Աmember��ָ��ptr�ҵ����������ݽṹtype��ָ��(��ʼ��ַ)
 *
 * @attention ��
 * @param ptr [IN]����ָ�룬ȡֵ����Ϊ��
 * @param type [IN]ָ��ptr���ڵĽṹ������
 * @param member [IN]ָ��ptrָ��ṹ��type�еĳ�Աmember
 *
 * @retval �ǿ� ����ptr���ڽṹ��type����ʼ��ַ����������Ϊtype*
 *
 * @since V100R002C00B301
 */
#define list_entry(ptr, type, member) \
    ((type *)((char *)(ptr)-(unsigned long)(&((type *)0)->member)))

/**
 * @ingroup  TEEC_List
 * @brief ��������ڵ�
 *
 * @par ����:
 * ������������ÿ������ڵ�
 *
 * @attention ��
 * @param pos [OUT]����ָ�룬ȡֵ����Ϊ��
 * @param list [IN]����ͷָ�룬ȡֵ����Ϊ��
 *
 * @retval �ǿ� ��������ڵ�
 *
 * @since V100R002C00B301
 */
#define list_for_each(pos, list) \
    for (pos = (list)->next; pos != (list); pos = pos->next)

/**
 * @ingroup  TEEC_List
 * @brief ��������ڵ��������ݽṹ����ʼ��ַ
 *
 * @par ����:
 * ������������ȡ����ڵ��������ݽṹ����ʼ��ַ
 *
 * @attention ��
 * @param pos [IN]����ָ�룬ȡֵ����Ϊ��
 * @param list [IN]����ͷָ�룬ȡֵ����Ϊ��
 * @param member [IN]����ڵ������ĳ�Աmember
 *
 * @retval �ǿ� ����ڵ��������ݽṹ����ʼ��ַ����������Ϊtypeof(*pos)
 *
 * @since V100R002C00B301
 */
#define list_for_each_entry(pos, list, member) \
    for (pos = list_entry((list)->next, typeof(*pos), member); \
            &pos->member != (list); \
            pos = list_entry(pos->member.next, typeof(*pos), member))

/**
 * @ingroup  TEEC_List
 * @brief ��������ڵ��������ݽṹ����ʼ��ַ
 *
 * @par ����:
 * ������������ȡ����ڵ��������ݽṹ����ʼ��ַ
 *
 * @attention ��
 * @param pos [IN]����ָ�룬ȡֵ����Ϊ��
 * @param n [IN]����ָ�룬ȡֵ����Ϊ��
 * @param list [IN]����ͷָ�룬ȡֵ����Ϊ��
 * @param member [IN]����ڵ������ĳ�Աmember
 *
 * @retval �ǿ� ����ڵ��������ݽṹ����ʼ��ַ����������Ϊtypeof(*pos)
 *
 * @since V100R002C00B301
 */
#define list_for_each_entry_safe(pos, n, list, member) \
    for (pos = list_entry((list)->next, typeof(*pos), member), \
            n = list_entry(pos->member.next, typeof(*pos), member); \
            &pos->member != (list); \
            pos = n, n = list_entry(n->member.next, typeof(*n), member))

#endif

