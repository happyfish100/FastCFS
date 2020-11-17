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


#ifndef _FS_API_H
#define _FS_API_H

#include "fcfs_api_types.h"
#include "fcfs_api_file.h"
#include "fastcommon/shared_func.h"

#define FS_API_DEFAULT_FASTDIR_SECTION_NAME    "FastDIR"
#define FS_API_DEFAULT_FASTSTORE_SECTION_NAME  "FastStore"

#define fcfs_api_set_contexts(ns)  fcfs_api_set_contexts_ex(&g_fcfs_api_ctx, ns)

#define fcfs_api_init(ns, config_filename)   \
    fcfs_api_init_ex(&g_fcfs_api_ctx, ns, config_filename,  \
            FS_API_DEFAULT_FASTDIR_SECTION_NAME,  \
            FS_API_DEFAULT_FASTSTORE_SECTION_NAME)

#define fcfs_api_pooled_init(ns, config_filename)   \
    fcfs_api_pooled_init_ex(&g_fcfs_api_ctx, ns, config_filename, \
            FS_API_DEFAULT_FASTDIR_SECTION_NAME,  \
            FS_API_DEFAULT_FASTSTORE_SECTION_NAME)

#define fcfs_api_pooled_init1(ns, ini_ctx) \
    fcfs_api_pooled_init_ex1(&g_fcfs_api_ctx, ns, ini_ctx, \
            FS_API_DEFAULT_FASTDIR_SECTION_NAME,  \
            FS_API_DEFAULT_FASTSTORE_SECTION_NAME)

#define fcfs_api_destroy()  fcfs_api_destroy_ex(&g_fcfs_api_ctx)

#ifdef __cplusplus
extern "C" {
#endif

    static inline void fcfs_api_set_contexts_ex1(FSAPIContext *ctx,
            FDIRClientContext *fdir, FSClientContext *fs, const char *ns)
    {
        ctx->contexts.fdir = fdir;
        ctx->contexts.fs = fs;

        ctx->ns.str = ctx->ns_holder;
        ctx->ns.len = snprintf(ctx->ns_holder,
                sizeof(ctx->ns_holder), "%s", ns);
    }

    static inline void fcfs_api_set_contexts_ex(FSAPIContext *ctx, const char *ns)
    {
        return fcfs_api_set_contexts_ex1(ctx, &g_fdir_client_vars.client_ctx,
                &g_fs_client_vars.client_ctx, ns);
    }

    int fcfs_api_init_ex1(FSAPIContext *ctx, FDIRClientContext *fdir,
            FSClientContext *fs, const char *ns, IniFullContext *ini_ctx,
            const char *fdir_section_name, const char *fs_section_name,
            const FDIRConnectionManager *fdir_conn_manager,
            const FSConnectionManager *fs_conn_manager, const bool need_lock);

    int fcfs_api_init_ex(FSAPIContext *ctx, const char *ns,
            const char *config_filename, const char *fdir_section_name,
            const char *fs_section_name);

    int fcfs_api_init_ex2(FSAPIContext *ctx, FDIRClientContext *fdir,
            FSClientContext *fs, const char *ns, IniFullContext *ini_ctx,
            const char *fdir_section_name, const char *fs_section_name,
            const FDIRClientConnManagerType conn_manager_type,
            const FSConnectionManager *fs_conn_manager, const bool need_lock);

    static inline int fcfs_api_pooled_init_ex1(FSAPIContext *ctx, const char *ns,
            IniFullContext *ini_ctx, const char *fdir_section_name,
            const char *fs_section_name)
    {
        return fcfs_api_init_ex2(ctx, &g_fdir_client_vars.client_ctx,
                &g_fs_client_vars.client_ctx, ns, ini_ctx, fdir_section_name,
                fs_section_name, conn_manager_type_pooled, NULL, true);
    }

    int fcfs_api_pooled_init_ex(FSAPIContext *ctx, const char *ns,
            const char *config_filename, const char *fdir_section_name,
            const char *fs_section_name);

    void fcfs_api_destroy_ex(FSAPIContext *ctx);

#ifdef __cplusplus
}
#endif

#endif
