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


#ifndef _FCFS_API_UTIL_H
#define _FCFS_API_UTIL_H

#include "fastcommon/logger.h"
#include "fcfs_api_types.h"
#include "inode_htable.h"

#ifdef __cplusplus
extern "C" {
#endif
#define FCFSAPI_SET_PATH_FULLNAME(fullname, ctx, path_str) \
    fullname.ns = ctx->ns;    \
    FC_SET_STRING(fullname.path, (char *)path_str)

#define fcfs_api_lookup_inode_by_path(path, inode)  \
    fcfs_api_lookup_inode_by_path_ex(&g_fcfs_api_ctx, path, LOG_DEBUG, inode)

#define fcfs_api_stat_dentry_by_path(path, dentry)  \
    fcfs_api_stat_dentry_by_path_ex(&g_fcfs_api_ctx, path, LOG_DEBUG, dentry)

#define fcfs_api_stat_dentry_by_fullname(fullname, dentry)  \
    fcfs_api_stat_dentry_by_fullname_ex(&g_fcfs_api_ctx,    \
            fullname, LOG_DEBUG, dentry)

#define fcfs_api_stat_dentry_by_inode(inode, dentry)  \
    fcfs_api_stat_dentry_by_inode_ex(&g_fcfs_api_ctx, inode, dentry)

#define fcfs_api_stat_dentry_by_pname(parent_inode, name, dentry)  \
    fcfs_api_stat_dentry_by_pname_ex(&g_fcfs_api_ctx, parent_inode, \
            name, LOG_DEBUG, dentry)

#define fcfs_api_create_dentry_by_pname(parent_inode, name, omp, dentry)  \
    fcfs_api_create_dentry_by_pname_ex(&g_fcfs_api_ctx, \
            parent_inode, name, omp, dentry)

#define fcfs_api_symlink_dentry_by_pname(link, parent_inode, name, omp, dentry) \
    fcfs_api_symlink_dentry_by_pname_ex(&g_fcfs_api_ctx, link, \
            parent_inode, name, omp, dentry)

#define fcfs_api_readlink_by_pname(parent_inode, name, link, size) \
    fcfs_api_readlink_by_pname_ex(&g_fcfs_api_ctx, parent_inode, \
            name, link, size)

#define fcfs_api_readlink_by_inode(inode, link, size) \
    fcfs_api_readlink_by_inode_ex(&g_fcfs_api_ctx, inode, link, size)

#define fcfs_api_link_dentry_by_pname(src_inode, dest_parent_inode, \
        dest_name, omp, dentry)  \
    fcfs_api_link_dentry_by_pname_ex(&g_fcfs_api_ctx, src_inode, \
            dest_parent_inode, dest_name, omp, dentry)

#define fcfs_api_remove_dentry_by_pname(parent_inode, name, fctx)  \
    fcfs_api_remove_dentry_by_pname_ex(&g_fcfs_api_ctx, \
            parent_inode, name, fctx)

#define fcfs_api_rename_dentry_by_pname(src_parent_inode, src_name, \
        dest_parent_inode, dest_name, flags, fctx)  \
    fcfs_api_rename_dentry_by_pname_ex(&g_fcfs_api_ctx, src_parent_inode, \
            src_name, dest_parent_inode, dest_name, flags, fctx)

#define fcfs_api_modify_dentry_stat(inode, attr, flags, dentry)  \
    fcfs_api_modify_dentry_stat_ex(&g_fcfs_api_ctx, inode, attr, flags, dentry)

#define fcfs_api_list_dentry_by_inode(inode, array)  \
    fcfs_api_list_dentry_by_inode_ex(&g_fcfs_api_ctx, inode, array)

#define fcfs_api_alloc_opendir_session()  \
    fcfs_api_alloc_opendir_session_ex(&g_fcfs_api_ctx)

#define fcfs_api_free_opendir_session(session) \
        fcfs_api_free_opendir_session_ex(&g_fcfs_api_ctx, session)

#define fcfs_api_dentry_sys_lock(session, inode, flags, file_size, space_end) \
    fcfs_api_dentry_sys_lock_ex(&g_fcfs_api_ctx, session, inode, \
            flags, file_size, space_end)

static inline int fcfs_api_lookup_inode_by_fullname_ex(FCFSAPIContext *ctx,
        const FDIRDEntryFullName *fullname, const int enoent_log_level,
        int64_t *inode)
{
    return fdir_client_lookup_inode_by_path_ex(ctx->contexts.fdir,
            fullname, enoent_log_level, inode);
}

static inline int fcfs_api_lookup_inode_by_path_ex(FCFSAPIContext *ctx,
        const char *path, const int enoent_log_level, int64_t *inode)
{
    FDIRDEntryFullName fullname;
    FCFSAPI_SET_PATH_FULLNAME(fullname, ctx, path);
    return fdir_client_lookup_inode_by_path_ex(ctx->contexts.fdir,
            &fullname, enoent_log_level, inode);
}

static inline int fcfs_api_stat_dentry_by_inode_ex(FCFSAPIContext *ctx,
        const int64_t inode, FDIRDEntryInfo *dentry)
{
    if (ctx->async_report.enabled) {
        inode_htable_check_conflict_and_wait(inode);
    }
    return fdir_client_stat_dentry_by_inode(ctx->contexts.fdir,
            inode, dentry);
}

static inline int fcfs_api_stat_dentry_by_fullname_ex(FCFSAPIContext *ctx,
        const FDIRDEntryFullName *fullname, const int enoent_log_level,
        FDIRDEntryInfo *dentry)
{
    if (ctx->async_report.enabled) {
        int result;
        int64_t inode;
        if ((result=fcfs_api_lookup_inode_by_fullname_ex(ctx,
                        fullname, enoent_log_level, &inode)) != 0)
        {
            return result;
        }

        return fcfs_api_stat_dentry_by_inode_ex(ctx, inode, dentry);
    } else {
        return fdir_client_stat_dentry_by_path_ex(ctx->contexts.fdir,
                fullname, enoent_log_level, dentry);
    }
}

static inline int fcfs_api_stat_dentry_by_path_ex(FCFSAPIContext *ctx,
        const char *path, const int enoent_log_level, FDIRDEntryInfo *dentry)
{
    FDIRDEntryFullName fullname;
    FCFSAPI_SET_PATH_FULLNAME(fullname, ctx, path);
    return fcfs_api_stat_dentry_by_fullname_ex(ctx,
            &fullname, enoent_log_level, dentry);
}

static inline int fcfs_api_stat_dentry_by_pname_ex(FCFSAPIContext *ctx,
        const int64_t parent_inode, const string_t *name,
        const int enoent_log_level, FDIRDEntryInfo *dentry)
{
    FDIRDEntryPName pname;
    FDIR_SET_DENTRY_PNAME_PTR(&pname, parent_inode, name);

    if (ctx->async_report.enabled) {
        int result;
        int64_t inode;
        if ((result=fdir_client_lookup_inode_by_pname_ex(ctx->contexts.
                        fdir, &pname, enoent_log_level, &inode)) != 0)
        {
            return result;
        }

        return fcfs_api_stat_dentry_by_inode_ex(ctx, inode, dentry);
    } else {
        return fdir_client_stat_dentry_by_pname_ex(ctx->contexts.fdir,
                &pname, enoent_log_level, dentry);
    }
}

static inline int fcfs_api_create_dentry_by_pname_ex(FCFSAPIContext *ctx,
        const int64_t parent_inode, const string_t *name,
        const FDIRClientOwnerModePair *omp, FDIRDEntryInfo *dentry)
{
    FDIRDEntryPName pname;
    FDIR_SET_DENTRY_PNAME_PTR(&pname, parent_inode, name);
    return fdir_client_create_dentry_by_pname(ctx->contexts.fdir,
            &ctx->ns, &pname, omp, dentry);
}

static inline int fcfs_api_symlink_dentry_by_pname_ex(FCFSAPIContext *ctx,
        const string_t *link, const int64_t parent_inode,
        const string_t *name, const FDIRClientOwnerModePair *omp,
        FDIRDEntryInfo *dentry)
{
    FDIRDEntryPName pname;
    FDIR_SET_DENTRY_PNAME_PTR(&pname, parent_inode, name);
    return fdir_client_symlink_dentry_by_pname(ctx->contexts.fdir,
            link, &ctx->ns, &pname, omp, dentry);
}

static inline int fcfs_api_readlink_by_pname_ex(FCFSAPIContext *ctx,
        const int64_t parent_inode, const string_t *name,
        string_t *link, const int size)
{
    FDIRDEntryPName pname;
    FDIR_SET_DENTRY_PNAME_PTR(&pname, parent_inode, name);
    return fdir_client_readlink_by_pname(ctx->contexts.fdir,
            &pname, link, size);
}

static inline int fcfs_api_readlink_by_inode_ex(FCFSAPIContext *ctx,
        const int64_t inode, string_t *link, const int size)
{
    return fdir_client_readlink_by_inode(ctx->contexts.fdir,
            inode, link, size);
}

int fcfs_api_remove_dentry_by_pname_ex(FCFSAPIContext *ctx,
        const int64_t parent_inode, const string_t *name,
        const FCFSAPIFileContext *fctx);

int fcfs_api_rename_dentry_by_pname_ex(FCFSAPIContext *ctx,
        const int64_t src_parent_inode, const string_t *src_name,
        const int64_t dest_parent_inode, const string_t *dest_name,
        const int flags, const FCFSAPIFileContext *fctx);

static inline int fcfs_api_modify_dentry_stat_ex(FCFSAPIContext *ctx,
        const int64_t inode, const struct stat *attr, const int64_t flags,
        FDIRDEntryInfo *dentry)
{
    FDIRDEntryStatus stat;

    if (ctx->async_report.enabled) {
        inode_htable_check_conflict_and_wait(inode);
    }
    stat.mode = attr->st_mode;
    stat.gid = attr->st_gid;
    stat.uid = attr->st_uid;
    stat.atime = attr->st_atime;
    stat.ctime = attr->st_ctime;
    stat.mtime = attr->st_mtime;
    stat.size = attr->st_size;
    return fdir_client_modify_dentry_stat(ctx->contexts.fdir,
            &ctx->ns, inode, flags, &stat, dentry);
}

static inline int fcfs_api_list_dentry_by_inode_ex(FCFSAPIContext *ctx,
        const int64_t inode, FDIRClientDentryArray *array)
{
    return fdir_client_list_dentry_by_inode(ctx->contexts.fdir, inode, array);
}

static inline FCFSAPIOpendirSession *fcfs_api_alloc_opendir_session_ex(
        FCFSAPIContext *ctx)
{
    return (FCFSAPIOpendirSession *)fast_mblock_alloc_object(
            &ctx->opendir_session_pool);
}

static inline void fcfs_api_free_opendir_session_ex(
        FCFSAPIContext *ctx, FCFSAPIOpendirSession *session)
{
    fast_mblock_free_object(&ctx->opendir_session_pool, session);
}

static inline int fcfs_api_dentry_sys_lock_ex(FCFSAPIContext *ctx,
        FDIRClientSession *session, const int64_t inode, const int flags,
        int64_t *file_size, int64_t *space_end)
{
    int result;
    if ((result=fdir_client_init_session(ctx->contexts.fdir, session)) != 0) {
        return result;
    }
    return fdir_client_dentry_sys_lock(session, inode,
            flags, file_size, space_end);
}

static inline int fcfs_api_dentry_sys_unlock(FDIRClientSession *session,
        const string_t *ns, const int64_t old_size,
        const FDIRSetDEntrySizeInfo *dsize)
{
    int result;
    result = fdir_client_dentry_sys_unlock_ex(session, ns, old_size, dsize);
    fdir_client_close_session(session, result != 0);
    return result;
}

static inline int fcfs_api_link_dentry_by_pname_ex(FCFSAPIContext *ctx,
        const int64_t src_inode, const int64_t dest_parent_inode,
        const string_t *dest_name, const FDIRClientOwnerModePair *omp,
        FDIRDEntryInfo *dentry)
{
    FDIRDEntryPName dest_pname;
    FDIR_SET_DENTRY_PNAME_PTR(&dest_pname, dest_parent_inode, dest_name);
    return fdir_client_link_dentry_by_pname(ctx->contexts.fdir,
            src_inode, &ctx->ns, &dest_pname, omp, dentry);
}

#ifdef __cplusplus
}
#endif

#endif
