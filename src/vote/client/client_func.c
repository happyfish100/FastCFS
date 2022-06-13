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


#include "fastcommon/ini_file_reader.h"
#include "fastcommon/shared_func.h"
#include "fastcommon/logger.h"
#include "sf/sf_global.h"
#include "client_global.h"
#include "client_func.h"

static int fcfs_vote_client_do_init_ex(FCFSVoteClientContext *client_ctx,
        IniFullContext *ini_ctx)
{
    int result;

    client_ctx->connect_timeout = iniGetIntValueEx(
            ini_ctx->section_name, "connect_timeout",
            ini_ctx->context, SF_DEFAULT_CONNECT_TIMEOUT, true);
    if (client_ctx->connect_timeout <= 0) {
        client_ctx->connect_timeout = SF_DEFAULT_CONNECT_TIMEOUT;
    }

    client_ctx->network_timeout = iniGetIntValueEx(
            ini_ctx->section_name, "network_timeout",
            ini_ctx->context, SF_DEFAULT_NETWORK_TIMEOUT, true);
    if (client_ctx->network_timeout <= 0) {
        client_ctx->network_timeout = SF_DEFAULT_NETWORK_TIMEOUT;
    }

    if ((result=sf_load_cluster_config(&client_ctx->cluster, ini_ctx,
                    FCFS_VOTE_DEFAULT_CLUSTER_PORT)) != 0)
    {
        return result;
    }

    return 0;
}

void fcfs_vote_client_log_config_ex(FCFSVoteClientContext *client_ctx,
        const char *extra_config)
{
    logInfo("fvote v%d.%d.%d, "
            "connect_timeout=%d, "
            "network_timeout=%d, "
            "vote_server_count=%d%s%s",
            g_fcfs_vote_global_vars.version.major,
            g_fcfs_vote_global_vars.version.minor,
            g_fcfs_vote_global_vars.version.patch,
            client_ctx->connect_timeout, client_ctx->network_timeout,
            FC_SID_SERVER_COUNT(client_ctx->cluster.server_cfg),
            extra_config != NULL ? ", " : "",
            extra_config != NULL ? extra_config : "");
}

int fcfs_vote_client_load_from_file_ex1(FCFSVoteClientContext *client_ctx,
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

    result = fcfs_vote_client_do_init_ex(client_ctx, ini_ctx);
    if (ini_ctx->context == &iniContext) {
        iniFreeContext(&iniContext);
        ini_ctx->context = NULL;
    }

    return result;
}

int fcfs_vote_client_init_ex2(FCFSVoteClientContext *client_ctx,
        IniFullContext *ini_ctx)
{
    int result;

    if ((result=fcfs_vote_client_load_from_file_ex1(
                    client_ctx, ini_ctx)) != 0)
    {
        return result;
    }

    return 0;
}

int fcfs_vote_client_init_for_server(IniFullContext *ini_ctx,
        bool *vote_node_enabled)
{
    g_fcfs_vote_client_vars.client_ctx.connect_timeout = SF_G_CONNECT_TIMEOUT;
    g_fcfs_vote_client_vars.client_ctx.network_timeout = SF_G_NETWORK_TIMEOUT;
    *vote_node_enabled = iniGetBoolValue(ini_ctx->section_name,
            "vote_node_enabled", ini_ctx->context, false);
    if (*vote_node_enabled) {
        return sf_load_cluster_config1(&g_fcfs_vote_client_vars.client_ctx.
                cluster, ini_ctx, "vote_node_cluster_filename",
                FCFS_VOTE_DEFAULT_CLUSTER_PORT);
    } else {
        return 0;
    }
}

void fcfs_vote_client_destroy_ex(FCFSVoteClientContext *client_ctx)
{
    fc_server_destroy(&client_ctx->cluster.server_cfg);
    memset(client_ctx, 0, sizeof(FCFSVoteClientContext));
}
