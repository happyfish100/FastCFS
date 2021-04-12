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


#ifndef _FCFS_AUTH_FOR_SERVER_H
#define _FCFS_AUTH_FOR_SERVER_H

#include "fcfs_auth_client.h"
#include "server_session.h"
#include "session_sync.h"

#ifdef __cplusplus
extern "C" {
#endif

#define fcfs_auth_for_server_init(ini_ctx, cluster_filename, auth_enabled)  \
    fcfs_auth_for_server_init_ex((&g_fcfs_auth_client_vars.client_ctx), \
            ini_ctx, cluster_filename, NULL, auth_enabled)

#define fcfs_auth_for_server_destroy() \
    fcfs_auth_for_server_destroy_ex((&g_fcfs_auth_client_vars.client_ctx))

static inline int fcfs_auth_for_server_init_ex(FCFSAuthClientContext
        *client_ctx, IniFullContext *ini_ctx, const char *cluster_filename,
        const char *section_name, bool *auth_enabled)
{
    int result;

    if ((result=fcfs_auth_load_config(client_ctx, cluster_filename,
                    section_name, auth_enabled)) != 0)
    {
        return result;
    }

    if (*auth_enabled) {
        if ((result=server_session_init(ini_ctx)) != 0) {
            return result;
        }
        if ((result=session_sync_init()) != 0) {
            return result;
        }
    }

    return result;
}

#define fcfs_auth_for_server_cfg_to_string(auth_enabled, output, size) \
    fcfs_auth_for_server_cfg_to_string_ex(&g_fcfs_auth_client_vars. \
            client_ctx, auth_enabled, output, size)

static inline void fcfs_auth_for_server_cfg_to_string_ex(
        FCFSAuthClientContext *client_ctx, const bool auth_enabled,
        char *output, const int size)
{
    char sz_session_config[128];
    char sz_auth_config[1024];

    fcfs_auth_config_to_string(client_ctx, auth_enabled,
            sz_auth_config, sizeof(sz_auth_config));
    if (auth_enabled) {
        server_session_cfg_to_string(sz_session_config,
                sizeof(sz_session_config));
        snprintf(output, size, "auth {%s, %s}",
                sz_auth_config, sz_session_config);
    } else {
        snprintf(output, size, "auth {%s}", sz_auth_config);
    }
}

static inline int fcfs_auth_for_server_start(const bool auth_enabled)
{
    if (auth_enabled) {
        return session_sync_start();
    } else {
        return 0;
    }
}

static inline void fcfs_auth_for_server_destroy_ex(
         FCFSAuthClientContext *client_ctx)
{
    fcfs_auth_client_destroy_ex(client_ctx);
}

#ifdef __cplusplus
}
#endif

#endif
