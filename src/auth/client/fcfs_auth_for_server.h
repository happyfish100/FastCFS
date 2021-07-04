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

#define fcfs_auth_for_server_init(auth, ini_ctx, cluster_filename) \
    fcfs_auth_for_server_init_ex(auth, ini_ctx, cluster_filename, NULL)

#ifdef __cplusplus
extern "C" {
#endif

int fcfs_auth_for_server_check_priv(FCFSAuthClientContext *client_ctx,
        SFRequestInfo *request, SFResponseInfo *response,
        const FCFSAuthValidatePriviledgeType priv_type,
        const int64_t the_priv);

static inline int fcfs_auth_for_server_init_ex(FCFSAuthClientFullContext
        *auth, IniFullContext *ini_ctx, const char *cluster_filename,
        const char *section_name)
{
    int result;

    g_fcfs_auth_client_vars.ignore_when_passwd_not_exist = true;
    if ((result=fcfs_auth_load_config_ex(auth, cluster_filename,
                    section_name, sf_server_group_index_type_cluster)) != 0)
    {
        return result;
    }

    if (auth->enabled) {
        if ((result=server_session_init(ini_ctx)) != 0) {
            return result;
        }
        if ((result=session_sync_init()) != 0) {
            return result;
        }
    }

    return result;
}

static inline void fcfs_auth_for_server_cfg_to_string(const
        FCFSAuthClientFullContext *auth, char *output, const int size)
{
    const char *caption = "";
    char sz_session_config[512];
    char sz_auth_config[1024];

    fcfs_auth_config_to_string_ex(auth, caption,
            sz_auth_config, sizeof(sz_auth_config));
    if (auth->enabled) {
        server_session_cfg_to_string_ex(sz_session_config,
                sizeof(sz_session_config), true);
        snprintf(output, size, "auth {%s, %s}",
                sz_auth_config, sz_session_config);
    } else {
        snprintf(output, size, "auth {%s}", sz_auth_config);
    }
}

static inline int fcfs_auth_for_server_start(const
        FCFSAuthClientFullContext *auth)
{
    if (auth->enabled) {
        return session_sync_start();
    } else {
        return 0;
    }
}

static inline void fcfs_auth_for_server_destroy(
         FCFSAuthClientFullContext *auth)
{
    fcfs_auth_client_destroy_ex(auth->ctx);
}

#ifdef __cplusplus
}
#endif

#endif
