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


#ifndef _FCFS_API_H
#define _FCFS_API_H

#include "fastcommon/shared_func.h"
#include "fcfs_api_types.h"
#include "fcfs_api_file.h"
#include "fcfs_api_util.h"

#define FCFS_API_DEFAULT_FASTDIR_SECTION_NAME    "FastDIR"
#define FCFS_API_DEFAULT_FASTSTORE_SECTION_NAME  "FastStore"

#define FCFS_API_OWNER_TYPE_CALLER_STR  "caller"
#define FCFS_API_OWNER_TYPE_FIXED_STR   "fixed"

#define fcfs_api_set_contexts(ns)  fcfs_api_set_contexts_ex(&g_fcfs_api_ctx, ns)

#define fcfs_api_init(ns, config_filename)   \
    fcfs_api_init_ex(&g_fcfs_api_ctx, ns, config_filename,  \
            FCFS_API_DEFAULT_FASTDIR_SECTION_NAME,   \
            FCFS_API_DEFAULT_FASTSTORE_SECTION_NAME)

#define fcfs_api_pooled_init(ns, config_filename)   \
    fcfs_api_pooled_init_ex(&g_fcfs_api_ctx, ns, config_filename, \
            FCFS_API_DEFAULT_FASTDIR_SECTION_NAME,   \
            FCFS_API_DEFAULT_FASTSTORE_SECTION_NAME)

#define fcfs_api_pooled_init1(ns, ini_ctx) \
    fcfs_api_pooled_init_ex1(&g_fcfs_api_ctx, ns, ini_ctx, \
            FCFS_API_DEFAULT_FASTDIR_SECTION_NAME,   \
            FCFS_API_DEFAULT_FASTSTORE_SECTION_NAME)

#define fcfs_api_destroy()  fcfs_api_destroy_ex(&g_fcfs_api_ctx)

#define fcfs_api_start() fcfs_api_start_ex(&g_fcfs_api_ctx, NULL)
#define fcfs_api_terminate() fcfs_api_terminate_ex(&g_fcfs_api_ctx)

#define fcfs_api_async_report_config_to_string(output, size) \
    fcfs_api_async_report_config_to_string_ex(&g_fcfs_api_ctx, output, size)

#ifdef __cplusplus
extern "C" {
#endif

    static inline void fcfs_api_set_contexts_ex1(FCFSAPIContext *ctx,
            FDIRClientContext *fdir, FSAPIContext *fsapi, const char *ns)
    {
        ctx->contexts.fdir = fdir;
        ctx->contexts.fsapi = fsapi;

        ctx->ns.str = ctx->ns_holder;
        ctx->ns.len = snprintf(ctx->ns_holder,
                sizeof(ctx->ns_holder), "%s", ns);
    }

    static inline void fcfs_api_set_contexts_ex(FCFSAPIContext *ctx,
            const char *ns)
    {
        g_fs_api_ctx.fs = &g_fs_client_vars.client_ctx;
        return fcfs_api_set_contexts_ex1(ctx, &g_fdir_client_vars.
                client_ctx, &g_fs_api_ctx, ns);
    }

    int fcfs_api_init_ex1(FCFSAPIContext *ctx, FDIRClientContext *fdir,
            FSAPIContext *fsapi, const char *ns, IniFullContext *ini_ctx,
            const char *fdir_section_name, const char *fs_section_name,
            const SFConnectionManager *fdir_conn_manager,
            const SFConnectionManager *fs_conn_manager,
            const bool need_lock);

    int fcfs_api_init_ex(FCFSAPIContext *ctx, const char *ns,
            const char *config_filename, const char *fdir_section_name,
            const char *fs_section_name);

    int fcfs_api_init_ex2(FCFSAPIContext *ctx, FDIRClientContext *fdir,
            FSAPIContext *fsapi, const char *ns, IniFullContext *ini_ctx,
            const char *fdir_section_name, const char *fs_section_name,
            const FDIRClientConnManagerType conn_manager_type,
            const SFConnectionManager *fs_conn_manager,
            const bool need_lock);

    static inline int fcfs_api_pooled_init_ex1(FCFSAPIContext *ctx,
            const char *ns, IniFullContext *ini_ctx, const char *
            fdir_section_name, const char *fs_section_name)
    {
        g_fs_api_ctx.fs = &g_fs_client_vars.client_ctx;
        return fcfs_api_init_ex2(ctx, &g_fdir_client_vars.client_ctx,
                &g_fs_api_ctx, ns, ini_ctx, fdir_section_name,
                fs_section_name, conn_manager_type_pooled, NULL, true);
    }

    int fcfs_api_pooled_init_ex(FCFSAPIContext *ctx, const char *ns,
            const char *config_filename, const char *fdir_section_name,
            const char *fs_section_name);

    void fcfs_api_destroy_ex(FCFSAPIContext *ctx);

    int fcfs_api_start_ex(FCFSAPIContext *ctx, const FCFSAPIOwnerInfo *owner);

    void fcfs_api_terminate_ex(FCFSAPIContext *ctx);

    int fcfs_api_client_session_create(FCFSAPIContext *ctx, const bool publish);

    static inline int fcfs_api_init_with_auth(const char *ns,
            const char *config_filename, const bool publish)
    {
        int result;
        if ((result=fcfs_api_init(ns, config_filename)) != 0) {
            return result;
        }

        return fcfs_api_client_session_create(&g_fcfs_api_ctx, publish);
    }

    static inline int fcfs_api_pooled_init_with_auth(const char *ns,
            const char *config_filename, const bool publish)
    {
        int result;
        if ((result=fcfs_api_pooled_init(ns, config_filename)) != 0) {
            return result;
        }

        return fcfs_api_client_session_create(&g_fcfs_api_ctx, publish);
    }

    static inline int fcfs_api_pooled_init1_with_auth(const char *ns,
            IniFullContext *ini_ctx, const bool publish)
    {
        int result;
        if ((result=fcfs_api_pooled_init1(ns, ini_ctx)) != 0) {
            return result;
        }

        return fcfs_api_client_session_create(&g_fcfs_api_ctx, publish);
    }

    int fcfs_api_load_idempotency_config_ex(const char *log_prefix_name,
            IniFullContext *ini_ctx, const char *fdir_section_name,
            const char *fs_section_name);

    static inline int fcfs_api_load_idempotency_config(const char
            *log_prefix_name, IniFullContext *ini_ctx)
    {
        return fcfs_api_load_idempotency_config_ex(log_prefix_name,
                ini_ctx, FCFS_API_DEFAULT_FASTDIR_SECTION_NAME,
                FCFS_API_DEFAULT_FASTSTORE_SECTION_NAME);
    }

    int fcfs_api_load_ns_mountpoint(IniFullContext *ini_ctx,
            const char *fdir_section_name, FCFSAPINSMountpointHolder *nsmp,
            string_t *mountpoint, const bool fuse_check);

    void fcfs_api_free_ns_mountpoint(FCFSAPINSMountpointHolder *nsmp);

    int fcfs_api_check_mountpoint(const char *config_filename,
            const string_t *mountpoint);

    static inline int fcfs_api_check_mountpoint1(const char
            *config_filename, const char *mpoint)
    {
        string_t mountpoint;
        FC_SET_STRING(mountpoint, (char *)mpoint);
        return fcfs_api_check_mountpoint(config_filename, &mountpoint);
    }

    void fcfs_api_async_report_config_to_string_ex(FCFSAPIContext *ctx,
            char *output, const int size);

    int fcfs_api_load_owner_config(IniFullContext *ini_ctx,
            FCFSAPIOwnerInfo *owner_info);

    void fcfs_api_log_client_common_configs(FCFSAPIContext *ctx,
            const FCFSAPIOwnerInfo *owner, const char *fdir_section_name,
            const char *fs_section_name, char *sf_idempotency_config,
            char *owner_config);

    static inline const char *fcfs_api_get_owner_type_caption(
            const FCFSAPIOwnerType owner_type)
    {
        switch (owner_type) {
            case fcfs_api_owner_type_caller:
                return FCFS_API_OWNER_TYPE_CALLER_STR;
            case fcfs_api_owner_type_fixed:
                return FCFS_API_OWNER_TYPE_FIXED_STR;
            default:
                return "";
        }
    }

#ifdef __cplusplus
}
#endif

#endif
