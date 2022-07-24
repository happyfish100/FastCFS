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
#include "fastcommon/sockopt.h"
#include "fastcommon/local_ip_func.h"
#include "sf/sf_global.h"
#include "sf/sf_configs.h"
#include "sf/sf_service.h"
#include "sf/sf_cluster_cfg.h"
#include "common/vote_proto.h"
#include "server_global.h"
#include "cluster_info.h"
#include "server_func.h"

static void log_cluster_server_config()
{
    FastBuffer buffer;

    if (fast_buffer_init_ex(&buffer, 1024) != 0) {
        return;
    }
    fc_server_to_config_string(&CLUSTER_SERVER_CONFIG, &buffer);
    log_it1(LOG_INFO, buffer.data, buffer.length);
    fast_buffer_destroy(&buffer);

    fc_server_to_log(&CLUSTER_SERVER_CONFIG);
}

static void server_log_configs()
{
    char sz_server_config[512];
    char sz_global_config[512];
    char sz_slowlog_config[256];
    char sz_service_config[256];

    sf_global_config_to_string(sz_global_config, sizeof(sz_global_config));
    sf_slow_log_config_to_string(&SLOW_LOG_CFG, "slow-log",
            sz_slowlog_config, sizeof(sz_slowlog_config));
    sf_context_config_to_string(&g_sf_context,
            sz_service_config, sizeof(sz_service_config));

    snprintf(sz_server_config, sizeof(sz_server_config),
            "master-election {quorum: %s, master_lost_timeout: %ds, "
            "max_wait_time: %ds}", sf_get_election_quorum_caption(
                MASTER_ELECTION_QUORUM), ELECTION_MASTER_LOST_TIMEOUT,
            ELECTION_MAX_WAIT_TIME);

    logInfo("FCFSVote V%d.%d.%d, %s, %s, service: {%s}, %s",
            g_fcfs_vote_global_vars.version.major,
            g_fcfs_vote_global_vars.version.minor,
            g_fcfs_vote_global_vars.version.patch,
            sz_global_config, sz_slowlog_config,
            sz_service_config, sz_server_config);

    log_local_host_ip_addrs();
    log_cluster_server_config();
}

static int load_master_election_config(const char *cluster_filename)
{
    IniContext ini_context;
    IniFullContext ini_ctx;
    int result;

    if ((result=iniLoadFromFile(cluster_filename, &ini_context)) != 0) {
        logError("file: "__FILE__", line: %d, "
                "load conf file \"%s\" fail, ret code: %d",
                __LINE__, cluster_filename, result);
        return result;
    }

    FAST_INI_SET_FULL_CTX_EX(ini_ctx, cluster_filename,
            "master-election", &ini_context);
    ELECTION_MASTER_LOST_TIMEOUT = iniGetIntCorrectValue(
            &ini_ctx, "master_lost_timeout", 3, 1, 30);
    ELECTION_MAX_WAIT_TIME = iniGetIntCorrectValue(
            &ini_ctx, "max_wait_time", 5, 1, 300);
    if ((result=sf_load_election_quorum_config(&MASTER_ELECTION_QUORUM,
                    &ini_ctx)) != 0)
    {
        return result;
    }

    iniFreeContext(&ini_context);
    return 0;
}

static int load_cluster_config(IniFullContext *ini_ctx,
        char *full_cluster_filename)
{
    int result;

    if ((result=sf_load_cluster_config_ex(&CLUSTER_CONFIG,
                    ini_ctx, FCFS_VOTE_DEFAULT_CLUSTER_PORT,
                    full_cluster_filename, PATH_MAX)) != 0)
    {
        return result;
    }

    if ((result=load_master_election_config(full_cluster_filename)) != 0) {
        return result;
    }

    return cluster_info_init(full_cluster_filename);
}

int server_load_config(const char *filename)
{
    const int task_buffer_extra_size = 0;
    IniContext ini_context;
    IniFullContext ini_ctx;
    char full_cluster_filename[PATH_MAX];
    int result;

    if ((result=iniLoadFromFile(filename, &ini_context)) != 0) {
        logError("file: "__FILE__", line: %d, "
                "load conf file \"%s\" fail, ret code: %d",
                __LINE__, filename, result);
        return result;
    }

    if ((result=sf_load_config("fcfs_voted", filename, &ini_context,
                    "service", FCFS_VOTE_DEFAULT_SERVICE_PORT,
                    FCFS_VOTE_DEFAULT_SERVICE_PORT,
                    task_buffer_extra_size)) != 0)
    {
        return result;
    }
    if ((result=sf_load_context_from_config(&CLUSTER_SF_CTX,
                    filename, &ini_context, "cluster",
                    FCFS_VOTE_DEFAULT_CLUSTER_PORT,
                    FCFS_VOTE_DEFAULT_CLUSTER_PORT)) != 0)
    {
        return result;
    }

    FAST_INI_SET_FULL_CTX_EX(ini_ctx, filename, NULL, &ini_context);
    if ((result=load_cluster_config(&ini_ctx,
                    full_cluster_filename)) != 0)
    {
        return result;
    }

    if ((result=sf_load_slow_log_config(filename, &ini_context,
                    &SLOW_LOG_CTX, &SLOW_LOG_CFG)) != 0)
    {
        return result;
    }

    iniFreeContext(&ini_context);

    load_local_host_ip_addrs();
    server_log_configs();

    return 0;
}
