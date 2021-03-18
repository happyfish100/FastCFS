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
            free(array->users);
            array->users = array->fixed;
            array->alloc = FCFS_AUTH_FIXED_USER_COUNT;
        }
    }

    void fcfs_auth_generate_passwd(unsigned char passwd[16]);

    int fcfs_auth_save_passwd(const char *filename, unsigned char passwd[16]);

    int fcfs_auth_load_passwd(const char *filename, unsigned char passwd[16]);

    int fcfs_auth_replace_filename_with_username(const string_t *src,
            const string_t *username, FilenameString *new_filename);

#ifdef __cplusplus
}
#endif

#endif
