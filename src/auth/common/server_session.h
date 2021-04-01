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


#ifndef _FCFS_AUTH_SERVER_SESSION_H
#define _FCFS_AUTH_SERVER_SESSION_H

#include "fastcommon/ini_file_reader.h"

typedef struct server_session_config {
    int shared_allocator_count;
    int shared_lock_count;
    int hashtable_capacity;
} ServerSessionConfig;

typedef struct server_session_entry {
    uint64_t session_id;
    int64_t user_id;
    int64_t user_priv;
    int64_t pool_id;
    struct {
        int fdir;
        int fstore;
    } pool_priv;
} ServerSessionEntry;

#ifdef __cplusplus
extern "C" {
#endif

int server_session_init(IniFullContext *ini_ctx);

void server_session_get_cfg(ServerSessionConfig *cfg);

void server_session_cfg_to_string(char *buff, const int size);

ServerSessionEntry *server_session_add(ServerSessionEntry *entry);

int server_session_user_priv_granted(const uint64_t session_id,
        const int64_t the_priv);

int server_session_fstore_priv_granted(const uint64_t session_id,
        const int the_priv);

int server_session_fdir_priv_granted(const uint64_t session_id,
        const int64_t pool_id, const int the_priv);

int server_session_delete(const uint64_t session_id);

#ifdef __cplusplus
}
#endif

#endif
