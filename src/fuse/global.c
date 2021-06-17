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
#include "fastcommon/common_define.h"

#ifdef OS_LINUX
#include <sys/vfs.h>
#include <sys/statfs.h>
#include <linux/magic.h>

#define unmount umount2

#elif defined(OS_FREEBSD)
#endif

#include <sys/param.h>
#include <sys/mount.h>
#include "fastcommon/sched_thread.h"
#include "fastcommon/system_info.h"
#include "sf/sf_global.h"
#include "sf/idempotency/client/client_channel.h"
#include "fastcfs/api/fcfs_api.h"
#include "fuse_wrapper.h"
#include "global.h"

#ifndef FUSE_SUPER_MAGIC
#define FUSE_SUPER_MAGIC 0x65735546
#endif

#define INI_FUSE_SECTION_NAME             "FUSE"
#define INI_IDEMPOTENCY_SECTION_NAME      "idempotency"
#define IDEMPOTENCY_DEFAULT_WORK_THREADS  1

#define FUSE_ALLOW_ALL_STR   "all"
#define FUSE_ALLOW_ROOT_STR  "root"

#define FUSE_OWNER_TYPE_CALLER_STR  "caller"
#define FUSE_OWNER_TYPE_FIXED_STR   "fixed"

FUSEGlobalVars g_fuse_global_vars = {NULL, NULL};

static int load_fuse_mountpoint(IniFullContext *ini_ctx, string_t *mountpoint)
{
    struct statfs buf;
    int result;

    if (g_fuse_global_vars.mountpoint == NULL) {
        mountpoint->str = iniGetStrValue(ini_ctx->section_name,
                "mountpoint", ini_ctx->context);
        if (mountpoint->str == NULL || *mountpoint->str == '\0') {
            logError("file: "__FILE__", line: %d, "
                    "config file: %s, section: %s, item: mountpoint "
                    "not exist or is empty", __LINE__, ini_ctx->filename,
                    ini_ctx->section_name);
            return ENOENT;
        }
    } else {
        mountpoint->str = g_fuse_global_vars.mountpoint;
    }

    if (!fileExists(mountpoint->str)) {
        result = errno != 0 ? errno : ENOENT;
        if (result == ENOTCONN) {
            if (unmount(mountpoint->str, 0) == 0) {
                result = 0;
            } else if (errno == EPERM) {
                logError("file: "__FILE__", line: %d, "
                        "unmount %s fail, you should run "
                        "\"sudo umount %s\" manually", __LINE__,
                        mountpoint->str, mountpoint->str);
            }
        }

        if (result != 0) {
            logError("file: "__FILE__", line: %d, "
                    "mountpoint: %s can't be accessed, "
                    "errno: %d, error info: %s",
                    __LINE__, mountpoint->str,
                    result, STRERROR(result));
            return result;
        }
    }
    if (!isDir(mountpoint->str)) {
        logError("file: "__FILE__", line: %d, "
                "mountpoint: %s is not a directory!",
                __LINE__, mountpoint->str);
        return ENOTDIR;
    }

    if (statfs(mountpoint->str, &buf) != 0) {
        logError("file: "__FILE__", line: %d, "
                "statfs mountpoint: %s fail, error info: %s",
                __LINE__, mountpoint->str, STRERROR(errno));
        return errno != 0 ? errno : ENOENT;
    }

    if ((buf.f_type & FUSE_SUPER_MAGIC) == FUSE_SUPER_MAGIC) {
        logError("file: "__FILE__", line: %d, "
                "mountpoint: %s already mounted by FUSE",
                __LINE__, mountpoint->str);
        return EEXIST;
    }

    mountpoint->len = strlen(mountpoint->str);
    return 0;
}

static int load_owner_config(IniFullContext *ini_ctx)
{
    int result;
    char *owner_type;
    char *owner_user;
    char *owner_group;
    struct group *group;
    struct passwd *user;

    owner_type = iniGetStrValue(ini_ctx->section_name,
            "owner_type", ini_ctx->context);
    if (owner_type == NULL || *owner_type == '\0') {
        g_fuse_global_vars.owner.type = owner_type_caller;
    } else if (strcasecmp(owner_type, FUSE_OWNER_TYPE_CALLER_STR) == 0) {
        g_fuse_global_vars.owner.type = owner_type_caller;
    } else if (strcasecmp(owner_type, FUSE_OWNER_TYPE_FIXED_STR) == 0) {
        g_fuse_global_vars.owner.type = owner_type_fixed;
    } else {
        g_fuse_global_vars.owner.type = owner_type_caller;
    }

    if (g_fuse_global_vars.owner.type == owner_type_caller) {
        g_fuse_global_vars.owner.uid = geteuid();
        g_fuse_global_vars.owner.gid = getegid();
        return 0;
    }

    owner_user = iniGetStrValue(ini_ctx->section_name,
            "owner_user", ini_ctx->context);
    if (owner_user == NULL || *owner_user == '\0') {
        g_fuse_global_vars.owner.uid = geteuid();
    } else {
        user = getpwnam(owner_user);
        if (user == NULL)
        {
            result = errno != 0 ? errno : ENOENT;
            logError("file: "__FILE__", line: %d, "
                    "getpwnam %s fail, errno: %d, error info: %s",
                    __LINE__, owner_user, result, STRERROR(result));
            return result;
        }
        g_fuse_global_vars.owner.uid = user->pw_uid;
    }

    owner_group = iniGetStrValue(ini_ctx->section_name,
            "owner_group", ini_ctx->context);
    if (owner_group == NULL || *owner_group == '\0') {
        g_fuse_global_vars.owner.gid = getegid();
    } else {
        group = getgrnam(owner_group);
        if (group == NULL) {
            result = errno != 0 ? errno : ENOENT;
            logError("file: "__FILE__", line: %d, "
                    "getgrnam %s fail, errno: %d, error info: %s",
                    __LINE__, owner_group, result, STRERROR(result));
            return result;
        }

        g_fuse_global_vars.owner.gid = group->gr_gid;
    }

    return 0;
}

static int load_fuse_config(IniFullContext *ini_ctx)
{
    string_t mountpoint;
    string_t ns;
    char *allow_others;
    int result;

    if (g_fuse_global_vars.ns == NULL) {
        ns.str = iniGetStrValue(FCFS_API_DEFAULT_FASTDIR_SECTION_NAME,
                "namespace", ini_ctx->context);
        if (ns.str == NULL || *ns.str == '\0') {
            logError("file: "__FILE__", line: %d, "
                    "config file: %s, section: %s, item: namespace "
                    "not exist or is empty", __LINE__, ini_ctx->filename,
                    FCFS_API_DEFAULT_FASTDIR_SECTION_NAME);
            return ENOENT;
        }
    } else {
        ns.str = g_fuse_global_vars.ns;
    }
    ns.len = strlen(ns.str);

    ini_ctx->section_name = INI_FUSE_SECTION_NAME;
    if ((result=load_fuse_mountpoint(ini_ctx, &mountpoint)) != 0) {
        return result;
    }

    g_fuse_global_vars.ns = fc_malloc(ns.len + mountpoint.len + 2);
    if (g_fuse_global_vars.ns == NULL) {
        return ENOMEM;
    }
    memcpy(g_fuse_global_vars.ns, ns.str, ns.len + 1);
    g_fuse_global_vars.mountpoint = g_fuse_global_vars.ns + ns.len + 1;
    memcpy(g_fuse_global_vars.mountpoint, mountpoint.str, mountpoint.len + 1);

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

    return load_owner_config(ini_ctx);
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

static const char *get_owner_type_caption(
        const FUSEOwnerType owner_type)
{
    switch (owner_type) {
        case owner_type_caller:
            return FUSE_OWNER_TYPE_CALLER_STR;
        case owner_type_fixed:
            return FUSE_OWNER_TYPE_FIXED_STR;
        default:
            return "";
    }
}

int fcfs_fuse_global_init(const char *config_filename)
{
#define MIN_THREAD_STACK_SIZE  (320 * 1024)
    const bool publish = true;
    int result;
    string_t base_path;
    string_t mountpoint;
    IniContext iniContext;
    IniFullContext ini_ctx;
    SFContextIniConfig config;
    char sf_idempotency_config[256];
    char fsapi_config[1024];
    char async_report_config[512];
    char owner_config[256];

    if ((result=iniLoadFromFile(config_filename, &iniContext)) != 0) {
        logError("file: "__FILE__", line: %d, "
                "load conf file \"%s\" fail, ret code: %d",
                __LINE__, config_filename, result);
        return result;
    }

    FAST_INI_SET_FULL_CTX_EX(ini_ctx, config_filename,
            FCFS_API_DEFAULT_FASTDIR_SECTION_NAME, &iniContext);
    do {
        ini_ctx.section_name = INI_IDEMPOTENCY_SECTION_NAME;
        if ((result=client_channel_init(&ini_ctx)) != 0) {
            return result;
        }

        SF_SET_CONTEXT_INI_CONFIG(config, config_filename,
                &iniContext, INI_IDEMPOTENCY_SECTION_NAME,
                0, 0, IDEMPOTENCY_DEFAULT_WORK_THREADS);
        if ((result=sf_load_config_ex("fcfs_fused", &config, 0)) != 0) {
            break;
        }
        if (SF_G_THREAD_STACK_SIZE < MIN_THREAD_STACK_SIZE) {
            logWarning("file: "__FILE__", line: %d, "
                    "config file: %s, thread_stack_size: %d is too small, "
                    "set to %d", __LINE__, config_filename,
                    SF_G_THREAD_STACK_SIZE, MIN_THREAD_STACK_SIZE);
            SF_G_THREAD_STACK_SIZE = MIN_THREAD_STACK_SIZE;
        }

        if ((result=load_fuse_config(&ini_ctx)) != 0) {
            break;
        }

        FC_SET_STRING(base_path, SF_G_BASE_PATH_STR);
        FC_SET_STRING(mountpoint, g_fuse_global_vars.mountpoint);
        if (fc_path_contains(&base_path, &mountpoint, &result)) {
            logError("file: "__FILE__", line: %d, "
                    "config file: %s, base path: %s contains mountpoint: %s, "
                    "this case is not allowed", __LINE__, config_filename,
                    SF_G_BASE_PATH_STR, g_fuse_global_vars.mountpoint);
            result = EINVAL;
            break;
        } else if (result != 0) {
            logError("file: "__FILE__", line: %d, "
                    "config file: %s, base path: %s or mountpoint: %s "
                    "is invalid", __LINE__, config_filename,
                    SF_G_BASE_PATH_STR, g_fuse_global_vars.mountpoint);
            break;
        }

        if ((result=fcfs_api_pooled_init1_with_auth(
                        g_fuse_global_vars.ns,
                        &ini_ctx, publish)) != 0)
        {
            break;
        }

        g_fdir_client_vars.client_ctx.idempotency_enabled =
            iniGetBoolValue(FCFS_API_DEFAULT_FASTDIR_SECTION_NAME,
                    "idempotency_enabled", ini_ctx.context,
                    g_idempotency_client_cfg.enabled);

        g_fs_client_vars.client_ctx.idempotency_enabled =
            iniGetBoolValue(FCFS_API_DEFAULT_FASTSTORE_SECTION_NAME,
                    "idempotency_enabled", ini_ctx.context,
                    g_idempotency_client_cfg.enabled);

    } while (0);

    iniFreeContext(&iniContext);
    if (result != 0) {
        return result;
    }

    if (g_fdir_client_vars.client_ctx.idempotency_enabled ||
            g_fs_client_vars.client_ctx.idempotency_enabled)
    {
        int len;

        len = sprintf(sf_idempotency_config,
                "%s idempotency_enabled=%d, "
                "%s idempotency_enabled=%d, ",
                FCFS_API_DEFAULT_FASTDIR_SECTION_NAME,
                g_fdir_client_vars.client_ctx.idempotency_enabled,
                FCFS_API_DEFAULT_FASTSTORE_SECTION_NAME,
                g_fs_client_vars.client_ctx.idempotency_enabled);
        idempotency_client_channel_config_to_string_ex(
                sf_idempotency_config + len,
                sizeof(sf_idempotency_config) - len, true);

        sf_log_config();
    } else {
        *sf_idempotency_config = '\0';
    }

    if (g_fuse_global_vars.owner.type == owner_type_fixed) {
        struct passwd *user;
        struct group *group;

        user = getpwuid(g_fuse_global_vars.owner.uid);
        group = getgrgid(g_fuse_global_vars.owner.gid);
        snprintf(owner_config, sizeof(owner_config),
                ", owner_user: %s, owner_group: %s",
                user->pw_name, group->gr_name);
    } else {
        *owner_config = '\0';
    }

    fcfs_api_async_report_config_to_string_ex(&g_fcfs_api_ctx,
            async_report_config, sizeof(async_report_config));
    fdir_client_log_config_ex(g_fcfs_api_ctx.contexts.fdir,
            async_report_config, false);

    fs_api_config_to_string(fsapi_config, sizeof(fsapi_config));
    fs_client_log_config_ex(g_fcfs_api_ctx.contexts.fsapi->fs,
            fsapi_config, false);

    logInfo("FastCFS V%d.%d.%d, FUSE library version %s, "
            "FastDIR namespace: %s, %sFUSE mountpoint: %s, "
            "owner_type: %s%s, singlethread: %d, clone_fd: %d, "
            "max_idle_threads: %d, allow_others: %s, auto_unmount: %d, "
            "attribute_timeout: %.1fs, entry_timeout: %.1fs",
            g_fcfs_global_vars.version.major,
            g_fcfs_global_vars.version.minor,
            g_fcfs_global_vars.version.patch,
            fuse_pkgversion(), g_fuse_global_vars.ns,
            sf_idempotency_config, g_fuse_global_vars.mountpoint,
            get_owner_type_caption(g_fuse_global_vars.owner.type),
            owner_config, g_fuse_global_vars.singlethread,
            g_fuse_global_vars.clone_fd, g_fuse_global_vars.max_idle_threads,
            get_allow_others_caption(g_fuse_global_vars.allow_others),
            g_fuse_global_vars.auto_unmount,
            g_fuse_global_vars.attribute_timeout,
            g_fuse_global_vars.entry_timeout);

    return 0;
}
