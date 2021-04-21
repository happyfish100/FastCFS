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


#ifndef _FCFS_AUTH_CLIENT_FUNC_H
#define _FCFS_AUTH_CLIENT_FUNC_H

#include "auth_global.h"
#include "client_types.h"

#ifdef __cplusplus
extern "C" {
#endif

#define fcfs_auth_client_load_from_file(filename) \
    fcfs_auth_client_load_from_file_ex((&g_fcfs_auth_client_vars.client_ctx), \
            filename, NULL)

#define fcfs_auth_client_init_ex(filename, index_type)  \
    fcfs_auth_client_init_ex1((&g_fcfs_auth_client_vars.client_ctx),  \
            filename, NULL, index_type)

#define fcfs_auth_client_init(filename)  \
    fcfs_auth_client_init_ex(filename,   \
            sf_server_group_index_type_service)

#define fcfs_auth_client_destroy() \
    fcfs_auth_client_destroy_ex((&g_fcfs_auth_client_vars.client_ctx))

#define fcfs_auth_client_log_config(client_ctx) \
    fcfs_auth_client_log_config_ex(client_ctx, NULL)

int fcfs_auth_client_load_from_file_ex1(FCFSAuthClientContext *client_ctx,
        IniFullContext *ini_ctx);

/**
* client initial from config file
* params:
*       client_ctx: the client context
*       config_filename: the client config filename
*       section_name: the section name, NULL or empty for global section
* return: 0 success, != 0 fail, return the error code
**/
static inline int fcfs_auth_client_load_from_file_ex(FCFSAuthClientContext
        *client_ctx, const char *config_filename, const char *section_name)
{
    IniFullContext ini_ctx;

    FAST_INI_SET_FULL_CTX(ini_ctx, config_filename, section_name);
    return fcfs_auth_client_load_from_file_ex1(client_ctx, &ini_ctx);
}

int fcfs_auth_client_init_ex2(FCFSAuthClientContext *client_ctx,
        IniFullContext *ini_ctx, const SFServerGroupIndexType index_type);

static inline int fcfs_auth_client_init_ex1(FCFSAuthClientContext *client_ctx,
        const char *config_filename, const char *section_name,
        const SFServerGroupIndexType index_type)
{
    IniFullContext ini_ctx;

    FAST_INI_SET_FULL_CTX(ini_ctx, config_filename, section_name);
    return fcfs_auth_client_init_ex2(client_ctx, &ini_ctx, index_type);
}

/**
* client destroy function
* params:
*       client_ctx: tracker group
* return: none
**/
void fcfs_auth_client_destroy_ex(FCFSAuthClientContext *client_ctx);

int fcfs_auth_alloc_group_servers(FCFSAuthServerGroup *server_group,
        const int alloc_size);

void fcfs_auth_client_log_config_ex(FCFSAuthClientContext *client_ctx,
        const char *extra_config);

#ifdef __cplusplus
}
#endif

#endif
