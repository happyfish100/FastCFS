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


#ifndef _FCFS_AUTH_CLIENT_GLOBAL_H
#define _FCFS_AUTH_CLIENT_GLOBAL_H

#include "auth_global.h"
#include "client_types.h"

typedef struct fcfs_auth_client_global_vars {
    char base_path[MAX_PATH_SIZE];
    bool need_load_passwd;   //false for auth server
    bool ignore_when_passwd_not_exist; //true for fdir and fstore servers
    FCFSAuthClientContext client_ctx;
} FCFSAuthClientGlobalVars;

#define fcfs_auth_client_init_full_ctx_ex(auth, client_ctx) \
    (auth)->ctx = client_ctx

#define fcfs_auth_client_init_full_ctx(auth) \
    fcfs_auth_client_init_full_ctx_ex(auth, &g_fcfs_auth_client_vars.client_ctx)

#ifdef __cplusplus
extern "C" {
#endif

    extern FCFSAuthClientGlobalVars g_fcfs_auth_client_vars;

#ifdef __cplusplus
}
#endif

#endif
