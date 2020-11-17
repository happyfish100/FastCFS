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
#include "fcfs_api.h"

FSAPIContext g_fcfs_api_ctx;

static int opendir_session_alloc_init(void *element, void *args)
{
    int result;
    FSAPIOpendirSession *session;

    session = (FSAPIOpendirSession *)element;
    if ((result=fdir_client_dentry_array_init(&session->array)) != 0) {
        return result;
    }

    if ((result=fast_buffer_init_ex(&session->buffer, 64 * 1024)) != 0) {
        return result;
    }
    return 0;
}

static int fcfs_api_common_init(FSAPIContext *ctx, FDIRClientContext *fdir,
        FSClientContext *fs, const char *ns, const bool need_lock)
{
    int result;

    if ((result=fast_mblock_init_ex1(&ctx->opendir_session_pool,
                    "opendir_session", sizeof(FSAPIOpendirSession), 64,
                    0, opendir_session_alloc_init, NULL, need_lock)) != 0)
    {
        return result;
    }

    fcfs_api_set_contexts_ex1(ctx, fdir, fs, ns);
    return 0;
}

int fcfs_api_init_ex1(FSAPIContext *ctx, FDIRClientContext *fdir,
        FSClientContext *fs, const char *ns, IniFullContext *ini_ctx,
        const char *fdir_section_name, const char *fs_section_name,
        const FDIRConnectionManager *fdir_conn_manager,
        const FSConnectionManager *fs_conn_manager, const bool need_lock)
{
    int result;

    ini_ctx->section_name = fdir_section_name;
    if ((result=fdir_client_init_ex1(fdir, ini_ctx, fdir_conn_manager)) != 0) {
        return result;
    }

    ini_ctx->section_name = fs_section_name;
    if ((result=fs_client_init_ex1(fs, ini_ctx, fs_conn_manager)) != 0) {
        return result;
    }

    return fcfs_api_common_init(ctx, fdir, fs, ns, need_lock);
}

int fcfs_api_init_ex(FSAPIContext *ctx, const char *ns,
        const char *config_filename, const char *fdir_section_name,
        const char *fs_section_name)
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
    result = fcfs_api_init_ex1(ctx, &g_fdir_client_vars.client_ctx,
            &g_fs_client_vars.client_ctx, ns, &ini_ctx,
            fdir_section_name, fs_section_name, NULL, NULL, false);
    iniFreeContext(&iniContext);
    return result;
}

int fcfs_api_pooled_init_ex(FSAPIContext *ctx, const char *ns,
        const char *config_filename, const char *fdir_section_name,
        const char *fs_section_name)
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
            fdir_section_name, fs_section_name);
    iniFreeContext(&iniContext);
    return result;
}

int fcfs_api_init_ex2(FSAPIContext *ctx, FDIRClientContext *fdir,
        FSClientContext *fs, const char *ns, IniFullContext *ini_ctx,
        const char *fdir_section_name, const char *fs_section_name,
        const FDIRClientConnManagerType conn_manager_type,
        const FSConnectionManager *fs_conn_manager, const bool need_lock)
{
    int result;
    const int max_count_per_entry = 0;
    const int max_idle_time = 3600;

    ini_ctx->section_name = fdir_section_name;
    if (conn_manager_type == conn_manager_type_simple) {
        result = fdir_client_simple_init_ex1(fdir, ini_ctx);
    } else if (conn_manager_type == conn_manager_type_pooled) {
        result = fdir_client_pooled_init_ex1(fdir, ini_ctx,
                max_count_per_entry, max_idle_time);
    } else {
        result = EINVAL;
    }
    if (result != 0) {
        return result;
    }

    ini_ctx->section_name = fs_section_name;
    if ((result=fs_client_init_ex1(fs, ini_ctx, fs_conn_manager)) != 0) {
        return result;
    }

    return fcfs_api_common_init(ctx, fdir, fs, ns, need_lock);
}

void fcfs_api_destroy_ex(FSAPIContext *ctx)
{
    if (ctx->contexts.fdir != NULL) {
        fdir_client_destroy_ex(ctx->contexts.fdir);
        ctx->contexts.fdir = NULL;
    }

    if (ctx->contexts.fs != NULL) {
        fs_client_destroy_ex(ctx->contexts.fs);
        ctx->contexts.fs = NULL;
    }
}
