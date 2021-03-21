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

#ifndef _FCFS_AUTH_CLIENT_TYPES_H
#define _FCFS_AUTH_CLIENT_TYPES_H

#include "fastcommon/common_define.h"
#include "sf/sf_configs.h"
#include "sf/sf_connection_manager.h"
#include "auth_types.h"

typedef struct fcfs_auth_client_user_key_pair {
    string_t username;
    string_t key_filename;
} FCFSAuthClientUserKeyPair;

typedef struct fcfs_auth_client_server_entry {
    int server_id;
    ConnectionInfo conn;
    char status;
} FCFSAuthClientServerEntry;

typedef struct fcfs_auth_server_group {
    int alloc_size;
    int count;
    ConnectionInfo *servers;
} FCFSAuthServerGroup;

typedef struct fcfs_auth_client_context {
    FCServerConfig server_cfg;
    int service_group_index;
    SFConnectionManager cm;
    SFClientCommonConfig common_cfg;
    char session_id[FCFS_AUTH_SESSION_ID_LEN];
} FCFSAuthClientContext;

#endif