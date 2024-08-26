/*!
 *****************************************************************************
 *
 * @File       tee_fs.h
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

#ifndef __TEE_FS_H
#define __TEE_FS_H

#include "tee_internal_api.h"

/* Macros for access() */
#define R_OK  4     // Read
#define W_OK  2     // Write
//#define X_OK  1   // Execute
#define F_OK  0     // Existence

#define CRYPT_KEY_SIZE 32   //file's crypt key size, now it's AES-256
#define IDENTIFY_SIZE   16  //TA info, now it's UUID

#define CRYPT_BLOCK_SIZE 64         //crypt block size
#define LAST_SIZE 4
#define TRANS_BUFF_SIZE AGENT_BUFF_SIZE    //transfer buffer size,should be equal to teecd's buffer size
#define HASH_FILE_MAGIC 'h'
#define HASH_LEN   32
#define HASH_VERIFY_LEN (2*HASH_LEN)
/*DIR_LEN is for mutiple sec storage partition and dir,e.g. sec_storage/dirA/file1.txt*/
#define DIR_LEN 64
#define HASH_NAME_BUFF_LEN  (2*HASH_LEN + 1 + DIR_LEN)
#define FD_MAX  1024    //max opend files per process at same time
#define BLOCK_SIZE  1024 //read or write block
#define SFS_PARTITION_PERSISTENT "sec_storage"
#define SFS_PARTITION_TRANSIENT "sec_storage_data"

int32_t file_name_transfer(const char *old_name, char *hash_name, bool is_file_hash);
TEE_Result tee_fs_init(void *control, const char *secret_buff);

void tee_fs_exit(void);

void dump_current_fd_map(void);

/*******************************************************************************
* Function    : pretend a filesystem opt "fopen"
* Description : null
* Input       : name - file name, include path
                flag - open mode
* Output      : open mode
* Return      : -1:opt failed, >0:file handler
*******************************************************************************/
int32_t fopen(const char *name, uint32_t flags);

/*******************************************************************************
* Function    : pretend a filesystem opt "fclose"
* Description : null
* Input       : file handler
* Output      : null
* Return      : -1:failed, =0:success
*******************************************************************************/
int32_t fclose(int32_t fd);

/*******************************************************************************
* Function    : pretend a filesystem opt "fread"
* Description : null
* Input       : size - read size
                fd - file handler
                error - 0:success, -1:error
* Output      : out_buf - buffer to store read file content
* Return      : number of element successfully read, less than count means error
                occurs
*******************************************************************************/
/*
 * Notice: total buffer is TRANS_BUFF_SIZE - sizeof(struct sec_storage_t),
 *         if write content is more than it, it need to write for N times.
 */
uint32_t fread(void *out_buf, uint32_t count, int32_t fd, int32_t *error);

/*******************************************************************************
* Function    : pretend a filesystem opt "fwrite"
* Description : null
* Input       : content - content buffer
                size - element size
                count - element count
                fd - file handler
* Output      : null
* Return      : number of element successfully write, less than count means error
                occurs
*******************************************************************************/
uint32_t fwrite(const void *content, uint32_t count, int32_t fd);

/*******************************************************************************
* Function    : pretend a filesystem opt "fseek"
* Description : null
* Input       : fd - file handler
                offset - bytes to the position specified by whence
                whence - SEEK_SET, SEEK_CUR, or SEEK_END, the offset is relative
                         to the start of the file, the current position indicator,
                         or end-of-file, respectively
* Output      : null
* Return      : -1:failed, =0:success
*******************************************************************************/
int32_t fseek(int32_t fd, int32_t offset, uint32_t whence);

/*******************************************************************************
* Function    : pretend a filesystem opt "fremove"
* Description : null
* Input       : file path
* Output      : null
* Return      : -1:failed, =0:success
*******************************************************************************/
int32_t fremove(const char *r_pth);

/*******************************************************************************
* Function    : pretend a filesystem opt "ftruncate"
* Description : null
* Input       : name - file name
              : len - new file length
* Output      : null
* Return      : -1:failed, =0:success
*******************************************************************************/
int32_t ftruncate(const char* name, uint32_t len);

/*******************************************************************************
* Function    : pretend a filesystem opt "frename"
* Description : null
* Input       : old_name - old file name
              : new_name - new file name
* Output      : null
* Return      : -1:failed, =0:success
*******************************************************************************/
int32_t frename(const char* old_name, const char* new_name);

/*******************************************************************************
* Function    : pretend a filesystem opt "fcreate"
* Description : null
* Input       : name - file name, include path
                flag - open mode
* Output      : null
* Return      : -1:opt failed, >0:file handler
*******************************************************************************/
int32_t fcreate(const char *name, uint32_t flag);

/*******************************************************************************
* Function    : pretend a filesystem opt "fcreate"
* Description : comfirm return value is success before use output value
* Input       : fd - file handler
* Output      : pos - current file pointer position
                len - file length
* Return      : -1:opt failed, >0:file handler
*******************************************************************************/
int32_t finfo(int32_t fd, uint32_t *pos, uint32_t *len);

/*******************************************************************************
* Function    : pretend a filesystem opt "access"
* Description : check if caller can access file with mode
* Input       : name - file name
* Output      : mode - access mode, R_OK:read, W_OK:write, F_OK:exist
* Return      : 0:access ok, <0:fail
*******************************************************************************/
int32_t faccess(const char* name, int mode);

/*******************************************************************************
* Function    : pretend a filesystem opt "access"
                difference from faccess: it can detect all path file
* Description : check if caller can access file with mode
* Input       : name - file path
* Output      : mode - access mode, R_OK:read, W_OK:write, F_OK:exist
* Return      : 0:access ok, <0:fail
*******************************************************************************/
int32_t faccess2(const char* name, int mode);
 int32_t s_fseek(int32_t fd, int32_t offset, uint32_t whence, const char* name);
//some secure fs opt is same with fs
#define s_fclose(fd) fclose(fd)
//#define s_fseek(fd, offset, whence)  fseek(fd, offset, whence)
//#define s_finfo(fd, pos, len)  finfo(fd, pos, len)
int32_t s_finfo(int32_t fd, uint32_t *pos, uint32_t *len, const char* name);
/*******************************************************************************
* Function    : safe file read,a serries call of fs to make a descrypto fs read,
                the input and output as same as fread func
* Description : null
* Input       : size - read size
                fd - file handler
                error - 0:success, -1:error
* Output      : out_buf - buffer to store read file content
* Return      : number of element successfully read, less than count means error
                occurs
*******************************************************************************/
uint32_t s_fread(void *out_buf, uint32_t count, int32_t fd, int32_t *error,const char* name);

/*******************************************************************************
* Function    : safe file write,a serries call of fs to make a encrypto fs write,
                the input and output as same as fwrite func
* Description : if error occurs, the crypto file is broken which can't be read
                again!
* Input       : content - content buffer
                count - content size
                fd - file handler
                name - file name,for save hash
* Output      : null
* Return      : number of element successfully write, less than count means error
                occurs
*******************************************************************************/
uint32_t s_fwrite(const void *content, uint32_t count, int32_t fd, const char* name);

int32_t s_fopen(const char *name, uint32_t flag);

int32_t s_fremove(const char *r_pth);

int32_t s_fsync(int32_t fd, const char *name);

int32_t s_ftruncate(const char* name, uint32_t len, int32_t fd);

int32_t s_frename(const char* old_name, const char* new_name);

int32_t s_fcreate(const char *name, uint32_t flag);

int32_t s_faccess(const char* name, int mode);

#endif
