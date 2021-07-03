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
#include "auth_types.h"

#define FCFS_AUTH_SESSION_OP_TYPE_CREATE   'C'
#define FCFS_AUTH_SESSION_OP_TYPE_REMOVE   'R'

typedef struct server_session_config {
    int shared_allocator_count;
    int shared_lock_count;
    int hashtable_capacity;
    int validate_within_fresh_seconds;
    string_t validate_key;
    string_t validate_key_filename;
    unsigned char validate_key_buff[FCFS_AUTH_PASSWD_LEN];
} ServerSessionConfig;

typedef union server_session_id_info {
    uint64_t id;
    struct {
        unsigned int ts;
        bool publish : 1;
        bool persistent : 1;
        unsigned int rn : 14;
        unsigned int sn : 16;
    } fields;
} ServerSessionIdInfo;

typedef struct session_synced_fields {
    struct {
        int64_t id;
        int64_t priv;
    } user;

    struct {
        int64_t id;
        bool available;
        FCFSAuthSPoolPriviledges privs;
    } pool;
} SessionSyncedFields;

typedef struct server_session_entry {
    ServerSessionIdInfo id_info;
    void *fields;
} ServerSessionEntry;

typedef struct server_session_hash_entry {
    ServerSessionEntry entry;
    struct fast_mblock_man *allocator;  //for free
    struct server_session_hash_entry *next;  //for hashtable
} ServerSessionHashEntry;

typedef void (*server_session_callback)(ServerSessionEntry *session);
typedef struct server_session_callbacks {
    server_session_callback add_func;
    server_session_callback del_func;
} ServerSessionCallbacks;

#define FCFS_AUTH_SERVER_SESSION_BY_FIELDS(fields)  \
    &((ServerSessionHashEntry *)fields - 1)->entry

#ifdef __cplusplus
extern "C" {
#endif

extern ServerSessionConfig g_server_session_cfg;

#define server_session_init(ini_ctx) \
    server_session_init_ex(ini_ctx, sizeof(SessionSyncedFields), NULL)

#define server_session_cfg_to_string(buff, size) \
    server_session_cfg_to_string_ex(buff, size, false)

#define server_session_add(entry, publish) \
    server_session_add_ex(entry, publish, false)

int server_session_init_ex(IniFullContext *ini_ctx, const int fields_size,
        ServerSessionCallbacks *callbacks);

void server_session_cfg_to_string_ex(char *buff,
        const int size, const bool output_all);

ServerSessionEntry *server_session_add_ex(const ServerSessionEntry *entry,
        const bool publish, const bool persistent);

int server_session_get_fields(const uint64_t session_id, void *fields);

int server_session_user_priv_granted(const uint64_t session_id,
        const int64_t the_priv);

int server_session_fstore_priv_granted(const uint64_t session_id,
        const int the_priv);

int server_session_fdir_priv_granted(const uint64_t session_id,
        const int the_priv);

int server_session_delete(const uint64_t session_id);

void server_session_clear();

#ifdef __cplusplus
}
#endif

#endif
