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
#include "sf/sf_service.h"
#include "common/auth_proto.h"
#include "server_global.h"
#include "server_func.h"

static int server_load_fdir_client_config(IniContext *ini_context,
        const char *config_filename)
{
#define FASTDIR_SECTION_NAME  "FastDIR"
#define ITEM_NAME_FDIR_CLIENT_CFG_FILENAME  "client_config_filename"

    char full_filename[PATH_MAX];
    char *client_cfg_filename;
    int result;

    if ((client_cfg_filename=iniGetRequiredStrValue(FASTDIR_SECTION_NAME,
                    ITEM_NAME_FDIR_CLIENT_CFG_FILENAME, ini_context)) == NULL)
    {
        return ENOENT;
    }

    resolve_path(config_filename, client_cfg_filename,
            full_filename, sizeof(full_filename));
    if (!fileExists(full_filename)) {
        result = errno != 0 ? errno : ENOENT;
        logError("file: "__FILE__", line: %d, "
                "config file: %s, item: %s, value: %s, access file %s fail, "
                "errno: %d, error info: %s", __LINE__, config_filename,
                ITEM_NAME_FDIR_CLIENT_CFG_FILENAME, client_cfg_filename,
                full_filename, result, STRERROR(result));
        return result;
    }

    if ((g_server_global_vars.fdir_client_cfg_filename=
                fc_strdup(full_filename)) == NULL)
    {
        return ENOMEM;
    }
    return 0;
}

static void server_log_configs()
{
    char sz_server_config[512];
    char sz_global_config[512];
    char sz_slowlog_config[256];
    char sz_service_config[128];

    sf_global_config_to_string(sz_global_config, sizeof(sz_global_config));
    sf_slow_log_config_to_string(&SLOW_LOG_CFG, "slow_log",
            sz_slowlog_config, sizeof(sz_slowlog_config));
    sf_context_config_to_string(&g_sf_context,
            sz_service_config, sizeof(sz_service_config));

    snprintf(sz_server_config, sizeof(sz_server_config),
            "FastDIR client_config_filename: %s",
            g_server_global_vars.fdir_client_cfg_filename);

    logInfo("FCFSAuth V%d.%d.%d, %s, %s, service: {%s}, %s",
            g_fcfs_auth_global_vars.version.major,
            g_fcfs_auth_global_vars.version.minor,
            g_fcfs_auth_global_vars.version.patch,
            sz_global_config, sz_slowlog_config,
            sz_service_config, sz_server_config);
    log_local_host_ip_addrs();
}

int server_load_config(const char *filename)
{
    const int task_buffer_extra_size = 0;
    IniContext ini_context;
    int result;

    if ((result=iniLoadFromFile(filename, &ini_context)) != 0) {
        logError("file: "__FILE__", line: %d, "
                "load conf file \"%s\" fail, ret code: %d",
                __LINE__, filename, result);
        return result;
    }

    if ((result=sf_load_config("fcfs_authd", filename, &ini_context,
                    "service", FCFS_AUTH_DEFAULT_SERVICE_PORT,
                    FCFS_AUTH_DEFAULT_SERVICE_PORT,
                    task_buffer_extra_size)) != 0)
    {
        return result;
    }

    if ((result=server_load_fdir_client_config(
                    &ini_context, filename)) != 0)
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
