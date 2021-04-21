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
#include "fastcommon/shared_func.h"
#include "fastcommon/logger.h"
#include "async_reporter.h"
#include "fcfs_api.h"

#define FCFS_API_MIN_SHARED_ALLOCATOR_COUNT           1
#define FCFS_API_MAX_SHARED_ALLOCATOR_COUNT        1000
#define FCFS_API_DEFAULT_SHARED_ALLOCATOR_COUNT      11

#define FCFS_API_MIN_HASHTABLE_SHARDING_COUNT           1
#define FCFS_API_MAX_HASHTABLE_SHARDING_COUNT       10000
#define FCFS_API_DEFAULT_HASHTABLE_SHARDING_COUNT      17

#define FCFS_API_MIN_HASHTABLE_TOTAL_CAPACITY          10949
#define FCFS_API_MAX_HASHTABLE_TOTAL_CAPACITY      100000000
#define FCFS_API_DEFAULT_HASHTABLE_TOTAL_CAPACITY    1403641

FCFSAPIContext g_fcfs_api_ctx;

static int opendir_session_alloc_init(void *element, void *args)
{
    int result;
    FCFSAPIOpendirSession *session;

    session = (FCFSAPIOpendirSession *)element;
    if ((result=fdir_client_dentry_array_init(&session->array)) != 0) {
        return result;
    }

    if ((result=fast_buffer_init_ex(&session->buffer, 64 * 1024)) != 0) {
        return result;
    }
    return 0;
}

int fcfs_api_client_session_create(FCFSAPIContext *ctx, const bool publish)
{
    FCFSAuthClientFullContext *auth;

    if (ctx->contexts.fdir->auth.enabled) {
        auth = &ctx->contexts.fdir->auth;
    } else if (ctx->contexts.fsapi->fs->auth.enabled) {
        auth = &ctx->contexts.fsapi->fs->auth;
    } else {
        return 0;
    }

    return fcfs_auth_client_session_create_ex(auth, &ctx->ns, publish);
}

static int fcfs_api_common_init(FCFSAPIContext *ctx, FDIRClientContext *fdir,
        FSAPIContext *fsapi, const char *ns, IniFullContext *ini_ctx,
        const char *fdir_section_name, const char *fsapi_section_name,
        const bool need_lock)
{
    int64_t element_limit = 1000 * 1000;
    const int64_t min_ttl_sec = 600;
    const int64_t max_ttl_sec = 86400;
    int result;

    ctx->use_sys_lock_for_append = iniGetBoolValue(fdir_section_name,
            "use_sys_lock_for_append", ini_ctx->context, false);
    ctx->async_report.enabled = iniGetBoolValue(fdir_section_name,
            "async_report_enabled", ini_ctx->context, true);
    ctx->async_report.interval_ms = iniGetIntValue(fdir_section_name,
            "async_report_interval_ms", ini_ctx->context, 10);

    ini_ctx->section_name = fdir_section_name;
    ctx->async_report.shared_allocator_count = iniGetIntCorrectValueEx(
            ini_ctx, "shared_allocator_count",
            FCFS_API_DEFAULT_SHARED_ALLOCATOR_COUNT,
            FCFS_API_MIN_SHARED_ALLOCATOR_COUNT,
            FCFS_API_MAX_SHARED_ALLOCATOR_COUNT, true);

    ctx->async_report.hashtable_sharding_count = iniGetIntCorrectValue(
            ini_ctx, "hashtable_sharding_count",
            FCFS_API_DEFAULT_HASHTABLE_SHARDING_COUNT,
            FCFS_API_MIN_HASHTABLE_SHARDING_COUNT,
            FCFS_API_MAX_HASHTABLE_SHARDING_COUNT);

    ctx->async_report.hashtable_total_capacity = iniGetInt64CorrectValue(
            ini_ctx, "hashtable_total_capacity",
            FCFS_API_DEFAULT_HASHTABLE_TOTAL_CAPACITY,
            FCFS_API_MIN_HASHTABLE_TOTAL_CAPACITY,
            FCFS_API_MAX_HASHTABLE_TOTAL_CAPACITY);

    if (ctx->async_report.enabled) {
        if ((result=fcfs_api_allocator_init(ctx)) != 0) {
            return result;
        }

        if ((result=inode_htable_init(ctx->async_report.
                        hashtable_sharding_count,
                        ctx->async_report.hashtable_total_capacity,
                        ctx->async_report.shared_allocator_count,
                        element_limit, min_ttl_sec, max_ttl_sec)) != 0)
        {
            return result;
        }
    }

    ini_ctx->section_name = fsapi_section_name;
    if ((result=fs_api_init_ex(fsapi, ini_ctx,
                    fcfs_api_file_write_done_callback,
                    sizeof(FCFSAPIWriteDoneCallbackExtraData))) != 0)
    {
        return result;
    }

    if ((result=fast_mblock_init_ex1(&ctx->opendir_session_pool,
                    "opendir_session", sizeof(FCFSAPIOpendirSession), 64,
                    0, opendir_session_alloc_init, NULL, need_lock)) != 0)
    {
        return result;
    }

    fcfs_api_set_contexts_ex1(ctx, fdir, fsapi, ns);
    return 0;
}

int fcfs_api_init_ex1(FCFSAPIContext *ctx, FDIRClientContext *fdir,
        FSAPIContext *fsapi, const char *ns, IniFullContext *ini_ctx,
        const char *fdir_section_name, const char *fs_section_name,
        const char *fsapi_section_name, const SFConnectionManager *
        fdir_conn_manager, const SFConnectionManager *fs_conn_manager,
        const bool need_lock)
{
    const bool bg_thread_enabled = true;
    int result;

    ini_ctx->section_name = fdir_section_name;
    if ((result=fdir_client_init_ex1(fdir, &g_fcfs_auth_client_vars.
                    client_ctx, ini_ctx, fdir_conn_manager)) != 0)
    {
        return result;
    }

    ini_ctx->section_name = fs_section_name;
    if ((result=fs_client_init_ex1(fsapi->fs, &g_fcfs_auth_client_vars.
                    client_ctx, ini_ctx, fs_conn_manager,
                    bg_thread_enabled)) != 0)
    {
        return result;
    }

    return fcfs_api_common_init(ctx, fdir, fsapi, ns, ini_ctx,
            fdir_section_name, fsapi_section_name, need_lock);
}

int fcfs_api_init_ex(FCFSAPIContext *ctx, const char *ns,
        const char *config_filename, const char *fdir_section_name,
        const char *fs_section_name, const char *fsapi_section_name)
{
    int result;
    IniContext iniContext;
    IniFullContext ini_ctx;

    if ((result=iniLoadFromFile(config_filename, &iniContext)) != 0) {
        logError("file: "__FILE__", line: %d, "
                "load conf file \"%s\" fail, ret code: %d",
                __LINE__, config_filename, result);
        return result;
    }

    g_fs_api_ctx.fs = &g_fs_client_vars.client_ctx;
    FAST_INI_SET_FULL_CTX_EX(ini_ctx, config_filename,
            fdir_section_name, &iniContext);
    result = fcfs_api_init_ex1(ctx, &g_fdir_client_vars.client_ctx,
            &g_fs_api_ctx, ns, &ini_ctx, fdir_section_name, fs_section_name,
            fsapi_section_name, NULL, NULL, false);
    iniFreeContext(&iniContext);
    return result;
}

int fcfs_api_pooled_init_ex(FCFSAPIContext *ctx, const char *ns,
        const char *config_filename, const char *fdir_section_name,
        const char *fs_section_name, const char *fsapi_section_name)
{
    int result;
    IniContext iniContext;
    IniFullContext ini_ctx;

    if ((result=iniLoadFromFile(config_filename, &iniContext)) != 0) {
        logError("file: "__FILE__", line: %d, "
                "load conf file \"%s\" fail, ret code: %d",
                __LINE__, config_filename, result);
        return result;
    }

    FAST_INI_SET_FULL_CTX_EX(ini_ctx, config_filename,
            fdir_section_name, &iniContext);
    result = fcfs_api_pooled_init_ex1(ctx, ns, &ini_ctx,
            fdir_section_name, fs_section_name, fsapi_section_name);
    iniFreeContext(&iniContext);
    return result;
}

int fcfs_api_init_ex2(FCFSAPIContext *ctx, FDIRClientContext *fdir,
        FSAPIContext *fsapi, const char *ns, IniFullContext *ini_ctx,
        const char *fdir_section_name, const char *fs_section_name,
        const char *fsapi_section_name, const FDIRClientConnManagerType
        conn_manager_type, const SFConnectionManager *fs_conn_manager,
        const bool need_lock)
{
    const bool bg_thread_enabled = true;
    const int max_count_per_entry = 0;
    const int max_idle_time = 3600;
    int result;

    ini_ctx->section_name = fdir_section_name;
    if (conn_manager_type == conn_manager_type_simple) {
        result = fdir_client_simple_init_ex1(fdir,
                &g_fcfs_auth_client_vars.client_ctx, ini_ctx);
    } else if (conn_manager_type == conn_manager_type_pooled) {
        result = fdir_client_pooled_init_ex1(fdir,
                &g_fcfs_auth_client_vars.client_ctx, ini_ctx,
                max_count_per_entry, max_idle_time, bg_thread_enabled);
    } else {
        result = EINVAL;
    }
    if (result != 0) {
        return result;
    }

    ini_ctx->section_name = fs_section_name;
    if ((result=fs_client_init_ex1(fsapi->fs, &g_fcfs_auth_client_vars.
                    client_ctx, ini_ctx, fs_conn_manager,
                    bg_thread_enabled)) != 0)
    {
        return result;
    }

    return fcfs_api_common_init(ctx, fdir, fsapi, ns, ini_ctx,
            fdir_section_name, fsapi_section_name, need_lock);
}

void fcfs_api_destroy_ex(FCFSAPIContext *ctx)
{
    if (ctx->contexts.fdir != NULL) {
        fdir_client_destroy_ex(ctx->contexts.fdir);
        ctx->contexts.fdir = NULL;
    }

    if (ctx->contexts.fsapi != NULL) {
        if (ctx->contexts.fsapi->fs != NULL) {
            fs_client_destroy_ex(ctx->contexts.fsapi->fs);
            ctx->contexts.fsapi->fs = NULL;
        }

        fs_api_destroy_ex(ctx->contexts.fsapi);
        ctx->contexts.fsapi = NULL;
    }
}

int fcfs_api_start_ex(FCFSAPIContext *ctx)
{
    int result;

    if ((result=fs_api_start_ex(ctx->contexts.fsapi)) != 0) {
        return result;
    }

    if ((result=sf_connection_manager_start(&ctx->
                    contexts.fdir->cm)) != 0)
    {
        return result;
    }

    if ((result=sf_connection_manager_start(&ctx->
                    contexts.fsapi->fs->cm)) != 0)
    {
        return result;
    }

    if (ctx->async_report.enabled) {
        return async_reporter_init(ctx);
    } else {
        return 0;
    }
}

void fcfs_api_terminate_ex(FCFSAPIContext *ctx)
{
    fs_api_terminate_ex(ctx->contexts.fsapi);
    async_reporter_terminate();
}

void fcfs_api_async_report_config_to_string_ex(FCFSAPIContext *ctx,
        char *output, const int size)
{
    int len;

    len = snprintf(output, size, "use_sys_lock_for_append: %d, "
            "async_report { enabled: %d", ctx->use_sys_lock_for_append,
            ctx->async_report.enabled);
    if (ctx->async_report.enabled) {
        len += snprintf(output + len, size - len, ", "
                "async_report_interval_ms: %d, "
                "shared_allocator_count: %d, "
                "hashtable_sharding_count: %d, "
                "hashtable_total_capacity: %"PRId64,
                ctx->async_report.interval_ms,
                ctx->async_report.shared_allocator_count,
                ctx->async_report.hashtable_sharding_count,
                ctx->async_report.hashtable_total_capacity);
        if (len > size) {
            len = size;
        }
    }
    snprintf(output + len, size - len, " } ");
}
