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

#include <sys/param.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <limits.h>
#include <grp.h>
#include <pwd.h>
#include "fastcommon/common_define.h"

#ifdef OS_LINUX
#include <sys/vfs.h>
#include <sys/statfs.h>
#include <linux/magic.h>
#endif

#include "fastcommon/shared_func.h"
#include "fastcommon/logger.h"
#include "sf/idempotency/client/client_channel.h"
#include "sf/idempotency/client/receipt_handler.h"
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

#define FCFS_API_INI_IDEMPOTENCY_SECTION_NAME      "idempotency"
#define FCFS_API_IDEMPOTENCY_DEFAULT_WORK_THREADS  1

#ifndef FUSE_SUPER_MAGIC
#define FUSE_SUPER_MAGIC 0x65735546
#endif

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
        const char *fdir_section_name, const char *fs_section_name,
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

    ini_ctx->section_name = fs_section_name;
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
        const SFConnectionManager *fdir_conn_manager,
        const SFConnectionManager *fs_conn_manager,
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
            fdir_section_name, fs_section_name, need_lock);
}

int fcfs_api_init_ex(FCFSAPIContext *ctx, const char *ns,
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

    g_fs_api_ctx.fs = &g_fs_client_vars.client_ctx;
    FAST_INI_SET_FULL_CTX_EX(ini_ctx, config_filename,
            fdir_section_name, &iniContext);
    result = fcfs_api_init_ex1(ctx, &g_fdir_client_vars.client_ctx,
            &g_fs_api_ctx, ns, &ini_ctx, fdir_section_name,
            fs_section_name, NULL, NULL, false);
    iniFreeContext(&iniContext);
    return result;
}

int fcfs_api_pooled_init_ex(FCFSAPIContext *ctx, const char *ns,
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

int fcfs_api_init_ex2(FCFSAPIContext *ctx, FDIRClientContext *fdir,
        FSAPIContext *fsapi, const char *ns, IniFullContext *ini_ctx,
        const char *fdir_section_name, const char *fs_section_name,
        const FDIRClientConnManagerType conn_manager_type,
        const SFConnectionManager *fs_conn_manager,
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
            fdir_section_name, fs_section_name, need_lock);
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

static int check_create_root_path(FCFSAPIContext *ctx,
        const FCFSAPIOwnerInfo *owner)
{
    int result;
    int64_t inode;
    FDIRClientOwnerModePair omp;

    if ((result=fcfs_api_lookup_inode_by_path_ex(ctx,
                    "/", LOG_DEBUG, &inode)) != 0)
    {
        if (result == ENOENT) {
            FDIRDEntryFullName fullname;
            FDIRDEntryInfo dentry;

            fullname.ns = ctx->ns;
            FC_SET_STRING(fullname.path, "/");
            FCFS_API_SET_OMP(omp, *owner, (ACCESSPERMS | S_IFDIR),
                    geteuid(), getegid());
            result = fdir_client_create_dentry(ctx->contexts.fdir,
                    &fullname, &omp, &dentry);
        }
    }

    return result;
}

int fcfs_api_start_ex(FCFSAPIContext *ctx, const FCFSAPIOwnerInfo *owner)
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

    if (ctx->contexts.fdir->idempotency_enabled ||
            ctx->contexts.fsapi->fs->idempotency_enabled)
    {
        if ((result=receipt_handler_init()) != 0) {
            return result;
        }
    }

    if (owner != NULL) {
        if ((result=check_create_root_path(ctx, owner)) != 0) {
            return result;
        }
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
    if (ctx->async_report.enabled) {
        async_reporter_terminate();
    }
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

int fcfs_api_load_owner_config(IniFullContext *ini_ctx,
        FCFSAPIOwnerInfo *owner_info)
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
        owner_info->type = fcfs_api_owner_type_caller;
    } else if (strcasecmp(owner_type, FCFS_API_OWNER_TYPE_CALLER_STR) == 0) {
        owner_info->type = fcfs_api_owner_type_caller;
    } else if (strcasecmp(owner_type, FCFS_API_OWNER_TYPE_FIXED_STR) == 0) {
        owner_info->type = fcfs_api_owner_type_fixed;
    } else {
        owner_info->type = fcfs_api_owner_type_caller;
    }

    if (owner_info->type == fcfs_api_owner_type_caller) {
        owner_info->uid = geteuid();
        owner_info->gid = getegid();
        return 0;
    }

    owner_user = iniGetStrValue(ini_ctx->section_name,
            "owner_user", ini_ctx->context);
    if (owner_user == NULL || *owner_user == '\0') {
        owner_info->uid = geteuid();
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
        owner_info->uid = user->pw_uid;
    }

    owner_group = iniGetStrValue(ini_ctx->section_name,
            "owner_group", ini_ctx->context);
    if (owner_group == NULL || *owner_group == '\0') {
        owner_info->gid = getegid();
    } else {
        group = getgrnam(owner_group);
        if (group == NULL) {
            result = errno != 0 ? errno : ENOENT;
            logError("file: "__FILE__", line: %d, "
                    "getgrnam %s fail, errno: %d, error info: %s",
                    __LINE__, owner_group, result, STRERROR(result));
            return result;
        }

        owner_info->gid = group->gr_gid;
    }

    return 0;
}

int fcfs_api_load_idempotency_config_ex(const char *log_prefix_name,
        IniFullContext *ini_ctx, const char *fdir_section_name,
        const char *fs_section_name)
{
#define MIN_THREAD_STACK_SIZE  (320 * 1024)
    int result;
    SFContextIniConfig config;

    ini_ctx->section_name = FCFS_API_INI_IDEMPOTENCY_SECTION_NAME;
    if ((result=client_channel_init(ini_ctx)) != 0) {
        return result;
    }

    SF_SET_CONTEXT_INI_CONFIG(config, ini_ctx->filename,
            ini_ctx->context, FCFS_API_INI_IDEMPOTENCY_SECTION_NAME,
            0, 0, FCFS_API_IDEMPOTENCY_DEFAULT_WORK_THREADS);
    if ((result=sf_load_config_ex(log_prefix_name, &config, 0)) != 0) {
        return result;
    }
    if (SF_G_THREAD_STACK_SIZE < MIN_THREAD_STACK_SIZE) {
        logWarning("file: "__FILE__", line: %d, "
                "config file: %s, thread_stack_size: %d is too small, "
                "set to %d", __LINE__, ini_ctx->filename,
                SF_G_THREAD_STACK_SIZE, MIN_THREAD_STACK_SIZE);
        SF_G_THREAD_STACK_SIZE = MIN_THREAD_STACK_SIZE;
    }

    g_fdir_client_vars.client_ctx.idempotency_enabled =
        iniGetBoolValue(fdir_section_name, "idempotency_enabled",
                ini_ctx->context, g_idempotency_client_cfg.enabled);
    g_fs_client_vars.client_ctx.idempotency_enabled =
        iniGetBoolValue(fs_section_name, "idempotency_enabled",
                ini_ctx->context, g_idempotency_client_cfg.enabled);

    return 0;
}

static int load_mountpoint(IniFullContext *ini_ctx,
        string_t *mountpoint, const bool fuse_check)
{
    struct statfs buf;
    int result;

    if (mountpoint->str == NULL) {
        mountpoint->str = iniGetStrValueEx(ini_ctx->section_name,
                "mountpoint", ini_ctx->context, true);
        if (mountpoint->str == NULL || *mountpoint->str == '\0') {
            logError("file: "__FILE__", line: %d, "
                    "config file: %s, section: %s, item: mountpoint "
                    "not exist or is empty", __LINE__, ini_ctx->filename,
                    ini_ctx->section_name);
            return ENOENT;
        }
        mountpoint->len = strlen(mountpoint->str);
    }

    if (*mountpoint->str != '/') {
        logError("file: "__FILE__", line: %d, "
                "config file: %s, mountpoint: %s must start with \"/\"",
                __LINE__, ini_ctx->filename, mountpoint->str);
        return ENOENT;
    }

    while (mountpoint->len > 0 && mountpoint->
            str[mountpoint->len - 1] == '/')
    {
        mountpoint->len--;
    }

    if (fuse_check && !fileExists(mountpoint->str)) {
        result = errno != 0 ? errno : ENOENT;
        if (result == ENOTCONN) {
#ifdef OS_LINUX
            result = umount2(mountpoint->str, MNT_FORCE);
#else
            result = unmount(mountpoint->str, 0);
#endif
            if (result != 0 && errno == EPERM) {
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

    if (fuse_check) {
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
    }

    return 0;
}

int fcfs_api_check_mountpoint(const char *config_filename,
        const string_t *mountpoint)
{
    int result;
    string_t base_path;

    FC_SET_STRING(base_path, SF_G_BASE_PATH_STR);
    if (fc_path_contains(&base_path, mountpoint, &result)) {
        logError("file: "__FILE__", line: %d, "
                "config file: %s, base path: %s contains mountpoint: %.*s, "
                "this case is not allowed", __LINE__, config_filename,
                SF_G_BASE_PATH_STR, mountpoint->len, mountpoint->str);
        return EINVAL;
    } else if (result != 0) {
        logError("file: "__FILE__", line: %d, "
                "config file: %s, base path: %s or mountpoint: %.*s "
                "is invalid", __LINE__, config_filename,
                SF_G_BASE_PATH_STR, mountpoint->len, mountpoint->str);
    }

    return result;
}

int fcfs_api_load_ns_mountpoint(IniFullContext *ini_ctx,
        const char *fdir_section_name, FCFSAPINSMountpointHolder *nsmp,
        string_t *mountpoint, const bool fuse_check)
{
    string_t ns;
    int result;

    if ((result=load_mountpoint(ini_ctx, mountpoint, fuse_check)) != 0) {
        return result;
    }

    if (nsmp->ns == NULL) {
        ns.str = iniGetStrValue(fdir_section_name,
                "namespace", ini_ctx->context);
        if (ns.str == NULL || *ns.str == '\0') {
            logError("file: "__FILE__", line: %d, "
                    "config file: %s, section: %s, item: namespace "
                    "not exist or is empty", __LINE__, ini_ctx->
                    filename, fdir_section_name);
            return ENOENT;
        }
    } else {
        ns.str = nsmp->ns;
    }

    ns.len = strlen(ns.str);
    nsmp->ns = fc_malloc(ns.len + mountpoint->len + 2);
    if (nsmp->ns == NULL) {
        return ENOMEM;
    }
    memcpy(nsmp->ns, ns.str, ns.len + 1);
    nsmp->mountpoint = nsmp->ns + ns.len + 1;
    memcpy(nsmp->mountpoint, mountpoint->str, mountpoint->len);
    *(nsmp->mountpoint + mountpoint->len) = '\0';
    mountpoint->str = nsmp->mountpoint;
    return 0;
}

void fcfs_api_free_ns_mountpoint(FCFSAPINSMountpointHolder *nsmp)
{
    if (nsmp->ns != NULL) {
        free(nsmp->ns);
        nsmp->ns = NULL;
    }
}

void fcfs_api_log_client_common_configs(FCFSAPIContext *ctx,
        const FCFSAPIOwnerInfo *owner, const char *fdir_section_name,
        const char *fs_section_name, char *sf_idempotency_config,
        char *owner_config)
{
    char auth_config[512];
    char fsapi_config[1024];
    char async_report_config[512];

    if (ctx->contexts.fdir->idempotency_enabled ||
            ctx->contexts.fsapi->fs->idempotency_enabled)
    {
        int len;

        len = sprintf(sf_idempotency_config,
                "%s idempotency_enabled=%d, "
                "%s idempotency_enabled=%d, ",
                fdir_section_name, ctx->contexts.
                fdir->idempotency_enabled,
                fs_section_name, ctx->contexts.
                fsapi->fs->idempotency_enabled);

        idempotency_client_channel_config_to_string_ex(
                sf_idempotency_config + len,
                sizeof(sf_idempotency_config) - len, true);
        sf_log_config();
    } else {
        *sf_idempotency_config = '\0';
    }

    if (owner->type == fcfs_api_owner_type_fixed) {
        struct passwd *user;
        struct group *group;

        user = getpwuid(owner->uid);
        group = getgrgid(owner->gid);
        sprintf(owner_config, ", owner_user: %s, owner_group: %s",
                user->pw_name, group->gr_name);
    } else {
        *owner_config = '\0';
    }

    fcfs_api_async_report_config_to_string_ex(ctx, async_report_config,
            sizeof(async_report_config));
    fcfs_auth_config_to_string(&ctx->contexts.fdir->auth,
            auth_config, sizeof(auth_config));
    snprintf(async_report_config + strlen(async_report_config),
            sizeof(async_report_config) - strlen(async_report_config),
            ", %s", auth_config);
    fdir_client_log_config_ex(ctx->contexts.fdir,
            async_report_config, false);

    fs_api_config_to_string(fsapi_config, sizeof(fsapi_config));
    fcfs_auth_config_to_string(&ctx->contexts.fsapi->fs->auth,
            auth_config, sizeof(auth_config));
    snprintf(fsapi_config + strlen(fsapi_config),
            sizeof(fsapi_config) - strlen(fsapi_config),
            ", %s", auth_config);
    fs_client_log_config_ex(ctx->contexts.fsapi->fs,
            fsapi_config, false);
}
