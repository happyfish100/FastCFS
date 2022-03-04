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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <grp.h>
#include <pwd.h>
#include "fastcommon/sched_thread.h"
#include "fastcommon/system_info.h"
#include "sf/sf_global.h"
#include "sf/idempotency/client/client_channel.h"
#include "fuse_wrapper.h"
#include "global.h"

#define INI_FUSE_SECTION_NAME             "FUSE"

#define FUSE_ALLOW_ALL_STR   "all"
#define FUSE_ALLOW_ROOT_STR  "root"

FUSEGlobalVars g_fuse_global_vars = {{NULL, NULL}};

static int load_fuse_config(IniFullContext *ini_ctx)
{
    string_t mountpoint;
    char *allow_others;
    int result;

    ini_ctx->section_name = INI_FUSE_SECTION_NAME;
    if (g_fuse_global_vars.nsmp.mountpoint == NULL) {
        FC_SET_STRING_NULL(mountpoint);
    } else {
        FC_SET_STRING(mountpoint, g_fuse_global_vars.nsmp.mountpoint);
    }
    if ((result=fcfs_api_load_ns_mountpoint(ini_ctx,
                    FCFS_API_DEFAULT_FASTDIR_SECTION_NAME,
                    &g_fuse_global_vars.nsmp, &mountpoint, true)) != 0)
    {
        return result;
    }

    g_fuse_global_vars.max_idle_threads = iniGetIntValue(ini_ctx->
            section_name, "max_idle_threads", ini_ctx->context, 10);

    g_fuse_global_vars.singlethread = iniGetBoolValue(ini_ctx->
            section_name, "singlethread", ini_ctx->context, false);

    g_fuse_global_vars.clone_fd = iniGetBoolValue(ini_ctx->
            section_name, "clone_fd", ini_ctx->context, false);
    if (g_fuse_global_vars.clone_fd) {
        Version version;
        get_kernel_version(&version);
        if (version.major < 4 || (version.major == 4 && version.minor < 2)) {
            logWarning("file: "__FILE__", line: %d, "
                    "kernel version %d.%d < 4.2, do NOT support "
                    "FUSE feature clone_fd", __LINE__,
                    version.major, version.minor);
            g_fuse_global_vars.clone_fd = false;
        }
    }

    g_fuse_global_vars.auto_unmount = iniGetBoolValue(ini_ctx->
            section_name, "auto_unmount", ini_ctx->context, false);

    allow_others = iniGetStrValue(ini_ctx->section_name,
            "allow_others", ini_ctx->context);
    if (allow_others == NULL || *allow_others == '\0') {
        g_fuse_global_vars.allow_others = allow_none;
    } else if (strcasecmp(allow_others, FUSE_ALLOW_ALL_STR) == 0) {
        g_fuse_global_vars.allow_others = allow_all;
    } else if (strcasecmp(allow_others, FUSE_ALLOW_ROOT_STR) == 0) {
        g_fuse_global_vars.allow_others = allow_root;
    } else {
        g_fuse_global_vars.allow_others = allow_none;
    }

    g_fuse_global_vars.attribute_timeout = iniGetDoubleValue(ini_ctx->
            section_name, "attribute_timeout", ini_ctx->context,
            FCFS_FUSE_DEFAULT_ATTRIBUTE_TIMEOUT);

    g_fuse_global_vars.entry_timeout = iniGetDoubleValue(ini_ctx->
            section_name, "entry_timeout", ini_ctx->context,
            FCFS_FUSE_DEFAULT_ENTRY_TIMEOUT);

    g_fuse_global_vars.xattr_enabled = iniGetBoolValue(ini_ctx->
            section_name, "xattr_enabled", ini_ctx->context, false);

    return fcfs_api_load_owner_config(ini_ctx, &g_fuse_global_vars.owner);
}

static const char *get_allow_others_caption(
        const FUSEAllowOthersMode allow_others)
{
    switch (allow_others) {
        case allow_all:
            return FUSE_ALLOW_ALL_STR;
        case allow_root:
            return FUSE_ALLOW_ROOT_STR;
        default:
            return "";
    }
}

int fcfs_fuse_global_init(const char *config_filename)
{
    const bool publish = true;
    int result;
    char sf_idempotency_config[256];
    char owner_config[2 * NAME_MAX + 64];
    IniContext iniContext;
    IniFullContext ini_ctx;

    if ((result=iniLoadFromFile(config_filename, &iniContext)) != 0) {
        logError("file: "__FILE__", line: %d, "
                "load conf file \"%s\" fail, ret code: %d",
                __LINE__, config_filename, result);
        return result;
    }

    FAST_INI_SET_FULL_CTX_EX(ini_ctx, config_filename,
            FCFS_API_DEFAULT_FASTDIR_SECTION_NAME, &iniContext);
    do {
        if ((result=load_fuse_config(&ini_ctx)) != 0) {
            break;
        }

        if ((result=fcfs_api_pooled_init1_with_auth(
                        g_fuse_global_vars.nsmp.ns,
                        &ini_ctx, publish)) != 0)
        {
            break;
        }

        if ((result=fcfs_api_load_idempotency_config(
                        "fcfs_fused", &ini_ctx)) != 0)
        {
            break;
        }

        if ((result=fcfs_api_check_mountpoint1(config_filename,
                        g_fuse_global_vars.nsmp.mountpoint)) != 0)
        {
            break;
        }
    } while (0);

    iniFreeContext(&iniContext);
    if (result != 0) {
        return result;
    }

    fcfs_api_log_client_common_configs(&g_fcfs_api_ctx,
            &g_fuse_global_vars.owner,
            FCFS_API_DEFAULT_FASTDIR_SECTION_NAME,
            FCFS_API_DEFAULT_FASTSTORE_SECTION_NAME,
            sf_idempotency_config, owner_config);

    logInfo("FastCFS V%d.%d.%d, FUSE library version %s, "
            "FastDIR namespace: %s, %sFUSE mountpoint: %s, "
            "owner_type: %s%s, singlethread: %d, clone_fd: %d, "
            "max_idle_threads: %d, allow_others: %s, auto_unmount: %d, "
            "attribute_timeout: %.1fs, entry_timeout: %.1fs, "
            "xattr_enabled: %d",
            g_fcfs_global_vars.version.major,
            g_fcfs_global_vars.version.minor,
            g_fcfs_global_vars.version.patch,
            fuse_pkgversion(), g_fuse_global_vars.nsmp.ns,
            sf_idempotency_config, g_fuse_global_vars.nsmp.mountpoint,
            fcfs_api_get_owner_type_caption(g_fuse_global_vars.owner.type),
            owner_config, g_fuse_global_vars.singlethread,
            g_fuse_global_vars.clone_fd, g_fuse_global_vars.max_idle_threads,
            get_allow_others_caption(g_fuse_global_vars.allow_others),
            g_fuse_global_vars.auto_unmount,
            g_fuse_global_vars.attribute_timeout,
            g_fuse_global_vars.entry_timeout,
            g_fuse_global_vars.xattr_enabled);

    return 0;
}
