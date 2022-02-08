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

#include "fastcommon/logger.h"
#include "posix_api.h"

FCFSPosixAPIGlobalVars g_fcfs_papi_global_vars;

__attribute__ ((constructor)) static void posix_api_global_init(void)
{
    log_init();
    logInfo("file: "__FILE__", line: %d, "
            "constructor", __LINE__);
}

__attribute__ ((destructor)) static void posix_api_global_destroy(void)
{
    fprintf(stderr, "file: "__FILE__", line: %d, "
            "destructor\n", __LINE__);
}

static int load_posix_api_config(FCFSPosixAPIContext *ctx,
        IniFullContext *ini_ctx)
{
    char *mountpoint;
    int len;

    mountpoint = iniGetStrValue(ini_ctx->section_name,
            "mountpoint", ini_ctx->context);
    if (mountpoint == NULL || *mountpoint == '\0') {
        logError("file: "__FILE__", line: %d, "
                "config file: %s, item: mountpoint not exist "
                "or is empty", __LINE__, ini_ctx->filename);
        return ENOENT;
    }

    if (*mountpoint != '/') {
        logError("file: "__FILE__", line: %d, "
                "config file: %s, mountpoint: %s must start with \"/\"",
                __LINE__, ini_ctx->filename, mountpoint);
        return ENOENT;
    }

    len = strlen(mountpoint);
    while (len > 0 && mountpoint[len - 1] == '/') {
        len--;
    }
    ctx->mountpoint.str = fc_malloc(len + 1);
    if (ctx->mountpoint.str == NULL) {
        return ENOMEM;
    }
    memcpy(ctx->mountpoint.str, mountpoint, len);
    *(ctx->mountpoint.str + len) = '\0';
    ctx->mountpoint.len = len;

    return fcfs_api_load_owner_config(ini_ctx, &ctx->owner);
}

int fcfs_posix_api_init_ex1(FCFSPosixAPIContext *ctx, const char *ns,
        const char *config_filename, const char *fdir_section_name,
        const char *fs_section_name, const bool publish)
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

    do {
        FAST_INI_SET_FULL_CTX_EX(ini_ctx, config_filename, NULL, &iniContext);
        if ((result=load_posix_api_config(ctx, &ini_ctx)) != 0) {
            break;
        }

        if ((result=fcfs_api_pooled_init_ex1(&ctx->api_ctx, ns, &ini_ctx,
                        fdir_section_name, fs_section_name)) != 0)
        {
            break;
        }
    } while (0);

    iniFreeContext(&iniContext);
    if (result != 0) {
        return result;
    }

    if ((result=fcfs_api_client_session_create(
                    &ctx->api_ctx, publish)) != 0)
    {
        return result;
    }
    return fcfs_fd_manager_init();
}

void fcfs_posix_api_destroy_ex(FCFSPosixAPIContext *ctx)
{
    if (ctx->mountpoint.str != NULL) {
        free(ctx->mountpoint.str);
        ctx->mountpoint.str = NULL;

        fcfs_api_destroy_ex(&ctx->api_ctx);
    }
}
