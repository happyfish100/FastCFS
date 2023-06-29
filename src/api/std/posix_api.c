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
#include "sf/idempotency/client/client_channel.h"
#include "posix_api.h"

#define DUMMY_MOUNTPOINT_STR  "/fastcfs/dummy/"
#define DUMMY_MOUNTPOINT_LEN  (sizeof(DUMMY_MOUNTPOINT_STR) - 1)

FCFSPosixAPIGlobalVars g_fcfs_papi_global_vars = {
    {{NULL, NULL}, {DUMMY_MOUNTPOINT_STR, DUMMY_MOUNTPOINT_LEN}}
};

static int load_posix_api_config(FCFSPosixAPIContext *ctx,
        const char *ns, IniFullContext *ini_ctx,
        const char *fdir_section_name)
{
    int result;

    ctx->nsmp.ns = (char *)ns;
    FC_SET_STRING_NULL(ctx->mountpoint);
    if ((result=fcfs_api_load_ns_mountpoint(ini_ctx,
                    fdir_section_name, &ctx->nsmp,
                    &ctx->mountpoint, false)) != 0)
    {
        return result;
    }

    return 0;
}

int fcfs_posix_api_init_ex1(FCFSPosixAPIContext *ctx, const char
        *log_prefix_name, const char *ns, const char *config_filename,
        const char *fdir_section_name, const char *fs_section_name,
        const bool publish)
{
    const bool need_lock = true;
    const bool persist_additional_gids = false;
    int result;
    bool fdir_idempotency_enabled;
    bool fs_idempotency_enabled;
    IniContext iniContext;
    IniFullContext ini_ctx;

    log_try_init();
    if ((result=iniLoadFromFile(config_filename, &iniContext)) != 0) {
        logError("file: "__FILE__", line: %d, "
                "load conf file \"%s\" fail, ret code: %d",
                __LINE__, config_filename, result);
        return result;
    }

    FAST_INI_SET_FULL_CTX_EX(ini_ctx, config_filename,
            NULL, &iniContext);
    do {
        if ((result=fcfs_api_load_idempotency_config_ex(log_prefix_name,
                        &ini_ctx, fdir_section_name, fs_section_name)) != 0)
        {
            break;
        }

        //save
        fdir_idempotency_enabled = g_fdir_client_vars.
            client_ctx.idempotency_enabled;
        fs_idempotency_enabled = g_fs_client_vars.
            client_ctx.idempotency_enabled;

        if ((result=load_posix_api_config(ctx, ns, &ini_ctx,
                        fdir_section_name)) != 0)
        {
            break;
        }

        if ((result=fcfs_api_pooled_init_ex1(&ctx->api_ctx,
                        ns, &ini_ctx, fdir_section_name,
                        fs_section_name, need_lock,
                        persist_additional_gids)) != 0)
        {
            break;
        }

        if ((result=fcfs_api_check_mountpoint(config_filename,
                        &ctx->mountpoint)) != 0)
        {
            break;
        }

        //restore
        g_fdir_client_vars.client_ctx.idempotency_enabled =
            fdir_idempotency_enabled;
        g_fs_client_vars.client_ctx.idempotency_enabled =
            fs_idempotency_enabled;
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

void fcfs_posix_api_log_configs_ex(FCFSPosixAPIContext *ctx,
        const char *fdir_section_name, const char *fs_section_name)
{
    BufferInfo sf_idempotency_config;
    char buff[256];
    char owner_config[2 * NAME_MAX + 64];

    sf_idempotency_config.buff = buff;
    sf_idempotency_config.alloc_size = sizeof(buff);
    fcfs_api_log_client_common_configs(&ctx->api_ctx,
            fdir_section_name, fs_section_name,
            &sf_idempotency_config, owner_config);

    logInfo("FastDIR namespace: %s, %smountpoint: %s, "
            "%s", ctx->nsmp.ns, sf_idempotency_config.buff,
            ctx->nsmp.mountpoint, owner_config);
}

void fcfs_posix_api_destroy_ex(FCFSPosixAPIContext *ctx)
{
    if (ctx->mountpoint.str != NULL) {
        fcfs_api_free_ns_mountpoint(&ctx->nsmp);
        ctx->mountpoint.str = NULL;
        fcfs_api_destroy_ex(&ctx->api_ctx);
    }
}
