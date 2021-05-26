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

#ifndef _FCFS_AUTH_FUNC_H
#define _FCFS_AUTH_FUNC_H

#include "sf/sf_proto.h"
#include "auth_types.h"

#define fcfs_auth_load_passwd(filename, passwd) \
    fcfs_auth_load_passwd_ex(filename, passwd, false)

#ifdef __cplusplus
extern "C" {
#endif

    static inline void fcfs_auth_user_init_array(FCFSAuthUserArray *array)
    {
        array->users = array->fixed;
        array->alloc = FCFS_AUTH_FIXED_USER_COUNT;
        array->count = 0;
    }

    static inline void fcfs_auth_user_free_array(FCFSAuthUserArray *array)
    {
        if (array->users != array->fixed) {
            if (array->users != NULL) {
                free(array->users);
            }
            array->users = array->fixed;
            array->alloc = FCFS_AUTH_FIXED_USER_COUNT;
        }
    }

    static inline void fcfs_auth_spool_init_array(
            FCFSAuthStoragePoolArray *array)
    {
        array->spools = array->fixed;
        array->alloc = FCFS_AUTH_FIXED_USER_COUNT;
        array->count = 0;
    }

    static inline void fcfs_auth_spool_free_array(
            FCFSAuthStoragePoolArray *array)
    {
        if (array->spools != array->fixed) {
            if (array->spools != NULL) {
                free(array->spools);
            }
            array->spools = array->fixed;
            array->alloc = FCFS_AUTH_FIXED_USER_COUNT;
        }
    }

    static inline void fcfs_auth_granted_init_array(
            FCFSAuthGrantedPoolArray *array)
    {
        array->gpools = array->fixed;
        array->alloc = FCFS_AUTH_FIXED_USER_COUNT;
        array->count = 0;
    }

    static inline void fcfs_auth_granted_free_array(
            FCFSAuthGrantedPoolArray *array)
    {
        if (array->gpools != array->fixed) {
            if (array->gpools != NULL) {
                free(array->gpools);
            }
            array->gpools = array->fixed;
            array->alloc = FCFS_AUTH_FIXED_USER_COUNT;
        }
    }

    int fcfs_auth_user_check_realloc_array(FCFSAuthUserArray *array,
            const int target_count);

    int fcfs_auth_spool_check_realloc_array(FCFSAuthStoragePoolArray *array,
            const int target_count);

    int fcfs_auth_gpool_check_realloc_array(FCFSAuthGrantedPoolArray *array,
            const int target_count);

    void fcfs_auth_generate_passwd(unsigned char passwd[16]);

    int fcfs_auth_save_passwd(const char *filename, unsigned char passwd[16]);

    int fcfs_auth_load_passwd_ex(const char *filename,
            unsigned char passwd[16], const bool ignore_enoent);

    int fcfs_auth_replace_filename_with_username(const string_t *src,
            const string_t *username, FilenameString *new_filename);

#ifdef __cplusplus
}
#endif

#endif
