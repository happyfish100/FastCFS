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


#include <sys/stat.h>
#include <limits.h>
#include "fastcommon/ini_file_reader.h"
#include "fastcommon/shared_func.h"
#include "fastcommon/logger.h"
#include "client_global.h"
#include "simple_connection_manager.h"
#include "client_func.h"

int fcfs_auth_alloc_group_servers(FCFSAuthServerGroup *server_group,
        const int alloc_size)
{
    int bytes;

    bytes = sizeof(ConnectionInfo) * alloc_size;
    server_group->servers = (ConnectionInfo *)fc_malloc(bytes);
    if (server_group->servers == NULL) {
        return errno != 0 ? errno : ENOMEM;
    }
    memset(server_group->servers, 0, bytes);

    server_group->alloc_size = alloc_size;
    server_group->count = 0;
    return 0;
}

static int fcfs_auth_load_server_config(FCFSAuthClientContext *client_ctx,
        IniFullContext *ini_ctx)
{
    int result;

    if ((result=sf_load_cluster_config(&client_ctx->cluster, ini_ctx,
                    FCFS_AUTH_DEFAULT_CLUSTER_PORT)) != 0)
    {
        return result;
    }

    return 0;
}

static int fcfs_auth_client_do_init_ex(FCFSAuthClientContext *client_ctx,
        IniFullContext *ini_ctx)
{
    char *pBasePath;
    int result;

    pBasePath = iniGetStrValue(NULL, "base_path", ini_ctx->context);
    if (pBasePath == NULL) {
        strcpy(g_fcfs_auth_client_vars.base_path, "/tmp");
    } else {
        snprintf(g_fcfs_auth_client_vars.base_path,
                sizeof(g_fcfs_auth_client_vars.base_path),
                "%s", pBasePath);
        chopPath(g_fcfs_auth_client_vars.base_path);
    }

    client_ctx->common_cfg.connect_timeout = iniGetIntValueEx(
            ini_ctx->section_name, "connect_timeout",
            ini_ctx->context, SF_DEFAULT_CONNECT_TIMEOUT, true);
    if (client_ctx->common_cfg.connect_timeout <= 0) {
        client_ctx->common_cfg.connect_timeout = SF_DEFAULT_CONNECT_TIMEOUT;
    }

    client_ctx->common_cfg.network_timeout = iniGetIntValueEx(
            ini_ctx->section_name, "network_timeout",
            ini_ctx->context, SF_DEFAULT_NETWORK_TIMEOUT, true);
    if (client_ctx->common_cfg.network_timeout <= 0) {
        client_ctx->common_cfg.network_timeout = SF_DEFAULT_NETWORK_TIMEOUT;
    }
    if ((result=fcfs_auth_load_server_config(client_ctx, ini_ctx)) != 0) {
        return result;
    }

    if ((result=sf_load_net_retry_config(&client_ctx->
                    common_cfg.net_retry_cfg, ini_ctx)) != 0)
    {
        return result;
    }

    return 0;
}

void fcfs_auth_client_log_config_ex(FCFSAuthClientContext *client_ctx,
        const char *extra_config)
{
    char net_retry_output[256];

    sf_net_retry_config_to_string(&client_ctx->common_cfg.net_retry_cfg,
            net_retry_output, sizeof(net_retry_output));
    logInfo("FastDIR v%d.%d.%d, "
            "base_path=%s, "
            "connect_timeout=%d, "
            "network_timeout=%d, "
            "%s, auth_server_count=%d%s%s",
            g_fcfs_auth_global_vars.version.major,
            g_fcfs_auth_global_vars.version.minor,
            g_fcfs_auth_global_vars.version.patch,
            g_fcfs_auth_client_vars.base_path,
            client_ctx->common_cfg.connect_timeout,
            client_ctx->common_cfg.network_timeout,
            net_retry_output, FC_SID_SERVER_COUNT(client_ctx->cluster.server_cfg),
            extra_config != NULL ? ", " : "",
            extra_config != NULL ? extra_config : "");
}

int fcfs_auth_client_load_from_file_ex1(FCFSAuthClientContext *client_ctx,
        IniFullContext *ini_ctx)
{
    IniContext iniContext;
    int result;

    if (ini_ctx->context == NULL) {
        if ((result=iniLoadFromFile(ini_ctx->filename, &iniContext)) != 0) {
            logError("file: "__FILE__", line: %d, "
                    "load conf file \"%s\" fail, ret code: %d",
                    __LINE__, ini_ctx->filename, result);
            return result;
        }
        ini_ctx->context = &iniContext;
    }

    if ((result=fcfs_auth_client_do_init_ex(client_ctx, ini_ctx)) == 0) {
        client_ctx->inited = true;
    }

    if (ini_ctx->context == &iniContext) {
        iniFreeContext(&iniContext);
        ini_ctx->context = NULL;
    }

    return result;
}

int fcfs_auth_client_init_ex2(FCFSAuthClientContext *client_ctx,
        IniFullContext *ini_ctx, const SFServerGroupIndexType index_type)
{
    int result;
    int server_group_index;

    if ((result=fcfs_auth_client_load_from_file_ex1(
                    client_ctx, ini_ctx)) != 0)
    {
        return result;
    }

    server_group_index = (index_type == sf_server_group_index_type_cluster ?
            client_ctx->cluster.cluster_group_index :
            client_ctx->cluster.service_group_index);
    if ((result=fcfs_auth_simple_connection_manager_init(client_ctx,
                    &client_ctx->cm, server_group_index)) != 0)
    {
        return result;
    }

    return 0;
}

void fcfs_auth_client_destroy_ex(FCFSAuthClientContext *client_ctx)
{
    fc_server_destroy(&client_ctx->cluster.server_cfg);
    fcfs_auth_simple_connection_manager_destroy(&client_ctx->cm);
    memset(client_ctx, 0, sizeof(FCFSAuthClientContext));
}
