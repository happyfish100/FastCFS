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
#include "common/auth_proto.h"
#include "common/auth_func.h"
#include "common/server_session.h"
#include "server_global.h"
#include "session_subscribe.h"
#include "cluster_info.h"
#include "server_func.h"

#define SECTION_NAME_FASTDIR  "FastDIR"

static int server_load_fdir_client_config(IniContext *ini_context,
        const char *config_filename)
{
#define ITEM_NAME_FDIR_CLIENT_CFG_FILENAME  "client_config_filename"

    char full_filename[PATH_MAX];
    char *client_cfg_filename;
    int result;

    if ((client_cfg_filename=iniGetRequiredStrValue(SECTION_NAME_FASTDIR,
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

static int server_load_admin_generate_config(IniContext *ini_context,
        const char *config_filename)
{
#define SECTION_NAME_ADMIN_GENERATE  "admin-generate"
#define USERNAME_VARIABLE_STR  "${username}"
#define USERNAME_VARIABLE_LEN  (sizeof(USERNAME_VARIABLE_STR) - 1)

    char *mode;
    string_t username;
    string_t secret_key_filename;
    FilenameString new_filename;
    char full_filename[PATH_MAX];
    int result;

    mode = iniGetStrValue(SECTION_NAME_ADMIN_GENERATE, "mode", ini_context);
    if (mode != NULL && strcmp(mode, "always") == 0) {
        ADMIN_GENERATE_MODE = AUTH_ADMIN_GENERATE_MODE_ALWAYS;
    } else {
        ADMIN_GENERATE_MODE = AUTH_ADMIN_GENERATE_MODE_FIRST;
    }

    username.str = iniGetStrValue(SECTION_NAME_ADMIN_GENERATE,
            "username", ini_context);
    if (username.str == NULL || *username.str == '\0') {
        username.str = "admin";
    }
    username.len = strlen(username.str);
    if ((result=fc_check_filename(&username, "username")) != 0) {
        return result;
    }

    secret_key_filename.str = iniGetStrValue(SECTION_NAME_ADMIN_GENERATE,
            "secret_key_filename", ini_context);
    if (secret_key_filename.str == NULL || *secret_key_filename.str == '\0') {
        secret_key_filename.str = "keys/"USERNAME_VARIABLE_STR".key";
    }
    secret_key_filename.len = strlen(secret_key_filename.str);
    fcfs_auth_replace_filename_with_username(&secret_key_filename,
            &username, &new_filename);

    resolve_path(config_filename, FC_FILENAME_STRING_PTR(new_filename),
            full_filename, sizeof(full_filename));
    FC_SET_STRING(secret_key_filename, full_filename);

    if (!fileExists(full_filename)) {
        char abs_path[PATH_MAX];
        int create_count;

        getAbsolutePath(full_filename, abs_path, sizeof(abs_path));
        if ((result=fc_mkdirs_ex(abs_path, 0755, &create_count)) != 0) {
            return result;
        }
        SF_CHOWN_TO_RUNBY_RETURN_ON_ERROR(abs_path);
    }

    ADMIN_GENERATE_BUFF = (char *)fc_malloc(username.len +
            secret_key_filename.len + 2);
    if (ADMIN_GENERATE_BUFF == NULL) {
        return ENOMEM;
    }
    ADMIN_GENERATE_USERNAME.str = ADMIN_GENERATE_BUFF;
    memcpy(ADMIN_GENERATE_USERNAME.str,
            username.str, username.len + 1);
    ADMIN_GENERATE_USERNAME.len = username.len;

    ADMIN_GENERATE_KEY_FILENAME.str =
        ADMIN_GENERATE_BUFF + username.len + 1;
    memcpy(ADMIN_GENERATE_KEY_FILENAME.str,
            secret_key_filename.str,
            secret_key_filename.len + 1);
    ADMIN_GENERATE_KEY_FILENAME.len = secret_key_filename.len;

    return 0;
}

static int server_load_pool_generate_config(IniContext *ini_context,
        const char *config_filename)
{
#define SECTION_NAME_POOL_GENERATE  "pool-generate"

    char *name_template;
    int result;

    AUTO_ID_INITIAL = iniGetInt64Value(SECTION_NAME_POOL_GENERATE,
            "auto_id_initial", ini_context, 1);

    name_template = iniGetStrValue(SECTION_NAME_POOL_GENERATE,
            "pool_name_template", ini_context);
    if (name_template == NULL || *name_template == '\0') {
        name_template = FCFS_AUTH_AUTO_ID_TAG_STR;
    } else if (strstr(name_template, FCFS_AUTH_AUTO_ID_TAG_STR) == NULL) {
        logError("file: "__FILE__", line: %d, "
                "config file: %s, pool_name_template: %s is invalid, "
                "must contains %s", __LINE__, config_filename,
                name_template, FCFS_AUTH_AUTO_ID_TAG_STR);
        return EINVAL;
    }

    POOL_NAME_TEMPLATE.len = strlen(name_template);
    POOL_NAME_TEMPLATE.str = (char *)fc_malloc(POOL_NAME_TEMPLATE.len + 1);
    if (POOL_NAME_TEMPLATE.str == NULL) {
        return ENOMEM;
    }
    memcpy(POOL_NAME_TEMPLATE.str, name_template, POOL_NAME_TEMPLATE.len + 1);
    if ((result=fc_check_filename(&POOL_NAME_TEMPLATE,
                    "pool_name_template")) != 0)
    {
        return result;
    }

    return 0;
}

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
    char sz_session_config[512];

    sf_global_config_to_string(sz_global_config, sizeof(sz_global_config));
    sf_slow_log_config_to_string(&SLOW_LOG_CFG, "slow-log",
            sz_slowlog_config, sizeof(sz_slowlog_config));
    sf_context_config_to_string(&g_sf_context,
            sz_service_config, sizeof(sz_service_config));
    server_session_cfg_to_string(sz_session_config,
            sizeof(sz_session_config));

    snprintf(sz_server_config, sizeof(sz_server_config),
            "admin-generate {mode: %s, username: %s, "
            "secret_key_filename: %s}, pool-generate: "
            "{auto_id_initial: %"PRId64", pool_name_template: %s}, "
            "master-election {quorum: %s, master_lost_timeout: %ds, "
            "max_wait_time: %ds}",
            (ADMIN_GENERATE_MODE == AUTH_ADMIN_GENERATE_MODE_FIRST ?
             "first" : "always"), ADMIN_GENERATE_USERNAME.str,
            ADMIN_GENERATE_KEY_FILENAME.str, AUTO_ID_INITIAL,
            POOL_NAME_TEMPLATE.str, sf_get_quorum_caption(
                MASTER_ELECTION_QUORUM), ELECTION_MASTER_LOST_TIMEOUT,
            ELECTION_MAX_WAIT_TIME);

    logInfo("FCFSAuth V%d.%d.%d, %s, %s, service: {%s}, %s",
            g_fcfs_auth_global_vars.version.major,
            g_fcfs_auth_global_vars.version.minor,
            g_fcfs_auth_global_vars.version.patch,
            sz_global_config, sz_slowlog_config,
            sz_service_config, sz_server_config);

    logInfo("FastDIR {client_config_filename: %s, "
            "pool_usage_refresh_interval: %d}, %s",
            g_server_global_vars.fdir_client_cfg_filename,
            POOL_USAGE_REFRESH_INTERVAL, sz_session_config);

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
    if ((result=sf_load_quorum_config(&MASTER_ELECTION_QUORUM,
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
                    ini_ctx, FCFS_AUTH_DEFAULT_CLUSTER_PORT,
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

    if ((result=sf_load_config("fcfs_authd", filename, &ini_context,
                    "service", FCFS_AUTH_DEFAULT_SERVICE_PORT,
                    FCFS_AUTH_DEFAULT_SERVICE_PORT,
                    task_buffer_extra_size)) != 0)
    {
        return result;
    }
    if ((result=sf_load_context_from_config(&CLUSTER_SF_CTX,
                    filename, &ini_context, "cluster",
                    FCFS_AUTH_DEFAULT_CLUSTER_PORT,
                    FCFS_AUTH_DEFAULT_CLUSTER_PORT)) != 0)
    {
        return result;
    }

    FAST_INI_SET_FULL_CTX_EX(ini_ctx, filename, NULL, &ini_context);
    if ((result=load_cluster_config(&ini_ctx,
                    full_cluster_filename)) != 0)
    {
        return result;
    }

    ini_ctx.section_name = "session";
    if ((result=server_session_init_ex(&ini_ctx,
                    sizeof(ServerSessionFields),
                    &g_server_session_callbacks)) != 0)
    {
        return result;
    }

    if ((result=server_load_admin_generate_config(
                    &ini_context, filename)) != 0)
    {
        return result;
    }

    if ((result=server_load_pool_generate_config(
                    &ini_context, filename)) != 0)
    {
        return result;
    }

    if ((result=server_load_fdir_client_config(
                    &ini_context, filename)) != 0)
    {
        return result;
    }

    POOL_USAGE_REFRESH_INTERVAL = iniGetIntValue(SECTION_NAME_FASTDIR,
            "pool_usage_refresh_interval", &ini_context, 3);
    if (POOL_USAGE_REFRESH_INTERVAL <= 0) {
        POOL_USAGE_REFRESH_INTERVAL = 1;
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
