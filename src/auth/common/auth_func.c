/*
 * Copyright (c) 2020 YuQing <384681@qq.com>
 *
 * This program is free software: you can use, redistribute, and/or modify
 * it under the terms of the GNU Affero General Public License, version 3
 * or later ("AGPL"), as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 */

#include "fastcommon/system_info.h"
#include "fastcommon/sched_thread.h"
#include "fastcommon/md5.h"
#include "fastcommon/http_func.h"
#include "sf/sf_global.h"
#include "auth_func.h"

void fcfs_auth_generate_passwd(unsigned char passwd[16])
{
    struct {
        time_t current_time;
        pid_t pid;
        int random;
#if defined(OS_LINUX) || defined(OS_FREEBSD)
        struct fast_sysinfo si;
#endif
        int hash_codes[4];
    } input;

    input.current_time = get_current_time();
    input.pid = getpid();
    input.random = rand();

#if defined(OS_LINUX) || defined(OS_FREEBSD)
    get_sysinfo(&input.si);
#endif

    /*
       logInfo("procs: %d, pid: %d, random: %d",
        input.si.procs, input.pid, input.random);
       */

    INIT_HASH_CODES4(input.hash_codes);
    CALC_HASH_CODES4(&input, sizeof(input), input.hash_codes);
    FINISH_HASH_CODES4(input.hash_codes);

    my_md5_buffer((char *)&input, sizeof(input), passwd);
}

int fcfs_auth_save_passwd(const char *filename, unsigned char passwd[16])
{
    char hex_buff[2 * FCFS_AUTH_PASSWD_LEN + 1];

    bin2hex((char *)passwd, FCFS_AUTH_PASSWD_LEN, hex_buff);
    return safeWriteToFile(filename, hex_buff,
            FCFS_AUTH_PASSWD_LEN * 2);
}

int fcfs_auth_load_passwd_ex(const char *filename,
        unsigned char passwd[16], const bool ignore_enoent)
{
    int result;
    int len;
    int64_t file_size;
    char hex_buff[2 * FCFS_AUTH_PASSWD_LEN + 4];

    file_size = sizeof(hex_buff);
    if (IS_URL_RESOURCE(filename)) {
        int http_status;
        int content_len;
        char buff[8 * 1024];
        char *content;
        char error_info[512];

        content = buff;
        content_len = sizeof(buff);
        if ((result=get_url_content_ex(filename, strlen(filename),
                        SF_G_CONNECT_TIMEOUT, SF_G_NETWORK_TIMEOUT,
                        &http_status, &content, &content_len,
                        error_info)) != 0)
        {
            if (*error_info != '\0') {
                logError("file: "__FILE__", line: %d, "
                        "%s fetch fail, %s", __LINE__,
                        filename, error_info);
            } else {
                logError("file: "__FILE__", line: %d, "
                        "%s fetch fail, errno: %d, error info: %s",
                        __LINE__, filename, result, STRERROR(result));
            }
            return result;
        }

        if (http_status != 200) {
            if (http_status == 404 && ignore_enoent) {
                memset(passwd, 0, FCFS_AUTH_PASSWD_LEN);
                return 0;
            }

            logError("file: "__FILE__", line: %d, "
                    "HTTP status code: %d != 200, url: %s",
                    __LINE__, http_status, filename);
            return http_status == 404 ? ENOENT : EACCES;
        }

        if (content_len >= sizeof(hex_buff)) {
            logError("file: "__FILE__", line: %d, "
                    "%s is not a valid secret file because the content "
                    "length: %d >= %d", __LINE__, filename, content_len,
                    (int)sizeof(hex_buff));
            return EOVERFLOW;
        }
        memcpy(hex_buff, content, content_len + 1);
        file_size = content_len;
    } else {
        if (access(filename, F_OK) != 0) {
            if (errno == ENOENT && ignore_enoent) {
                memset(passwd, 0, FCFS_AUTH_PASSWD_LEN);
                return 0;
            }
        }

        if ((result=getFileContentEx(filename,
                        hex_buff, 0, &file_size)) != 0)
        {
            return result;
        }
    }

    if (file_size > 2 * FCFS_AUTH_PASSWD_LEN) {
        trim_right(hex_buff);
        file_size = strlen(hex_buff);
    }
    if (file_size != 2 * FCFS_AUTH_PASSWD_LEN) {
        logError("file: "__FILE__", line: %d, "
                "invalid secret filename: %s whose file size MUST be: %d",
                __LINE__, filename, 2 * FCFS_AUTH_PASSWD_LEN);
        return EINVAL;
    }

    hex2bin(hex_buff, (char *)passwd, &len);
    return 0;
}

int fcfs_auth_replace_filename_with_username(const string_t *src,
        const string_t *username, FilenameString *new_filename)
{
#define USERNAME_VARIABLE_STR  "${username}"
#define USERNAME_VARIABLE_LEN  (sizeof(USERNAME_VARIABLE_STR) - 1)
    string_t tag;

    FC_INIT_FILENAME_STRING(*new_filename);
    FC_SET_STRING_EX(tag, USERNAME_VARIABLE_STR, USERNAME_VARIABLE_LEN);
    return str_replace(src, &tag, username, &new_filename->s,
            FC_FILENAME_BUFFER_SIZE(*new_filename));
}

int fcfs_auth_user_check_realloc_array(FCFSAuthUserArray *array,
        const int target_count)
{
    int new_alloc;
    FCFSAuthUserInfo *new_users;

    if (array->alloc >= target_count) {
        return 0;
    }

    new_alloc = array->alloc;
    while (new_alloc < target_count) {
        new_alloc *= 2;
    }

    new_users = (FCFSAuthUserInfo *)fc_malloc(
            sizeof(FCFSAuthUserInfo) * new_alloc);
    if (new_users == NULL) {
        return ENOMEM;
    }

    if (array->count > 0) {
        int bytes;
        bytes = sizeof(FCFSAuthUserInfo) * array->count;
        memcpy(new_users, array->users, bytes);
    }
    if (array->users != array->fixed) {
        free(array->users);
    }

    array->users = new_users;
    array->alloc = new_alloc;
    return 0;
}

int fcfs_auth_spool_check_realloc_array(FCFSAuthStoragePoolArray *array,
        const int target_count)
{
    int new_alloc;
    FCFSAuthStoragePoolInfo *new_spools;

    if (array->alloc >= target_count) {
        return 0;
    }

    new_alloc = array->alloc;
    while (new_alloc < target_count) {
        new_alloc *= 2;
    }

    new_spools = (FCFSAuthStoragePoolInfo *)fc_malloc(
            sizeof(FCFSAuthStoragePoolInfo) * new_alloc);
    if (new_spools == NULL) {
        return ENOMEM;
    }

    if (array->count > 0) {
        int bytes;
        bytes = sizeof(FCFSAuthStoragePoolInfo) * array->count;
        memcpy(new_spools, array->spools, bytes);
    }
    if (array->spools != array->fixed) {
        free(array->spools);
    }

    array->spools = new_spools;
    array->alloc = new_alloc;
    return 0;
}

int fcfs_auth_gpool_check_realloc_array(FCFSAuthGrantedPoolArray *array,
        const int target_count)
{
    int new_alloc;
    FCFSAuthGrantedPoolFullInfo *new_gpools;

    if (array->alloc >= target_count) {
        return 0;
    }

    new_alloc = array->alloc;
    while (new_alloc < target_count) {
        new_alloc *= 2;
    }

    new_gpools = (FCFSAuthGrantedPoolFullInfo *)fc_malloc(
            sizeof(FCFSAuthGrantedPoolFullInfo) * new_alloc);
    if (new_gpools == NULL) {
        return ENOMEM;
    }

    if (array->count > 0) {
        int bytes;
        bytes = sizeof(FCFSAuthGrantedPoolFullInfo) * array->count;
        memcpy(new_gpools, array->gpools, bytes);
    }
    if (array->gpools != array->fixed) {
        free(array->gpools);
    }

    array->gpools = new_gpools;
    array->alloc = new_alloc;
    return 0;
}
