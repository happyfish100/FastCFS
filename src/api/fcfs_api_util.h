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

#include <utime.h>
#include <sys/xattr.h>
#include "fastcommon/logger.h"
#include "fcfs_api_types.h"
#include "inode_htable.h"
#include "async_reporter.h"

#ifdef __cplusplus
extern "C" {
#endif

#define FCFSAPI_SET_PATH_FULLNAME(fullname, ctx, path_str) \
    fullname.ns = (ctx)->ns;    \
    FC_SET_STRING(fullname.path, (char *)path_str)

#define FCFSAPI_SET_PATH_OPER_FNAME_EX(fname, ctx, \
        _uid, _gid, _gcount, _glist, path_str) \
    FDIR_SET_OPERATOR(fname.oper, _uid, _gid, _gcount, _glist); \
    FCFSAPI_SET_PATH_FULLNAME(fname.fullname, ctx, path_str)

#define FCFSAPI_SET_PATH_OPER_FNAME(fname, ctx, _oper, path_str) \
    fname.oper = _oper; \
    FCFSAPI_SET_PATH_FULLNAME(fname.fullname, ctx, path_str)

#define FCFSAPI_SET_PATH_OPER_PNAME_EX(opname, _uid, _gid, \
        _gcount, _glist, parent_inode, name) \
    FDIR_SET_OPERATOR(opname.oper, _uid, _gid, _gcount, _glist); \
    FDIR_SET_DENTRY_PNAME_PTR(&opname.pname, parent_inode, name)

#define FCFSAPI_SET_PATH_OPER_PNAME(opname, _oper, parent_inode, name) \
    opname.oper = _oper; \
    FDIR_SET_DENTRY_PNAME_PTR(&opname.pname, parent_inode, name)

#define FCFSAPI_SET_PATH_OPER_PNAME1(opname, _uid, _gid, \
        _gcount, _glist, parent_inode, name) \
    FDIR_SET_OPERATOR(opname.oper, _uid, _gid, _gcount, _glist); \
    FDIR_SET_DENTRY_PNAME_STR(&opname.pname, parent_inode, name)

#define FCFSAPI_SET_OPER_INODE_PAIR_EX(oino, \
        _uid, _gid, _gcount, _glist, _inode) \
    FDIR_SET_OPERATOR(fname.oper, _uid, _gid, _gcount, _glist); \
    oino.inode = _inode

#define FCFSAPI_SET_OPER_INODE_PAIR(oino, _oper, _inode) \
    oino.oper = _oper; \
    oino.inode = _inode

#define fcfs_api_lookup_inode_by_path(path, oper, inode) \
    fcfs_api_lookup_inode_by_path_ex(&g_fcfs_api_ctx, path,  \
            oper, LOG_DEBUG, inode)

#define fcfs_api_stat_dentry_by_path(path, flags, dentry)  \
    fcfs_api_stat_dentry_by_path_ex(&g_fcfs_api_ctx, \
            path, flags, LOG_DEBUG, dentry)

#define fcfs_api_stat_dentry_by_fullname(fullname, flags, dentry)  \
    fcfs_api_stat_dentry_by_fullname_ex(&g_fcfs_api_ctx,    \
            fullname, flags, LOG_DEBUG, dentry)

#define fcfs_api_stat_dentry_by_inode(inode, flags, dentry)  \
    fcfs_api_stat_dentry_by_inode_ex(&g_fcfs_api_ctx, inode, flags, dentry)

#define fcfs_api_stat_dentry_by_pname(opname, flags, dentry)  \
    fcfs_api_stat_dentry_by_pname_ex(&g_fcfs_api_ctx, opname, \
            flags, LOG_DEBUG, dentry)

#define fcfs_api_access_dentry_by_path(path, mask, flags, dentry) \
    fcfs_api_access_dentry_by_path_ex(&g_fcfs_api_ctx, \
            path, mask, flags, dentry)

#define fcfs_api_access_dentry_by_inode(oino, mask, flags, dentry) \
    fcfs_api_access_dentry_by_inode_ex(&g_fcfs_api_ctx, \
            oino, mask, flags, dentry)

#define fcfs_api_access_dentry_by_pname(opname, mask, flags, dentry) \
    fcfs_api_access_dentry_by_pname_ex(&g_fcfs_api_ctx, \
            opname, mask, flags, dentry)

#define fcfs_api_create_dentry_by_pname(parent_inode, name, oper, rdev, dentry) \
    fcfs_api_create_dentry_by_pname_ex(&g_fcfs_api_ctx, \
            parent_inode, name, oper, rdev, dentry)

#define fcfs_api_symlink_dentry_by_pname(link, parent_inode, name, oper, dentry)\
    fcfs_api_symlink_dentry_by_pname_ex(&g_fcfs_api_ctx, link, \
            parent_inode, name, oper, dentry)

#define fcfs_api_readlink_by_pname(parent_inode, name, link, size) \
    fcfs_api_readlink_by_pname_ex(&g_fcfs_api_ctx, parent_inode, \
            name, link, size)

#define fcfs_api_readlink_by_inode(inode, link, size) \
    fcfs_api_readlink_by_inode_ex(&g_fcfs_api_ctx, inode, link, size)

#define fcfs_api_link_dentry_by_pname(src_inode, dest_parent_inode, \
        dest_name, oper, flags, dentry)  \
    fcfs_api_link_dentry_by_pname_ex(&g_fcfs_api_ctx, src_inode, \
            dest_parent_inode, dest_name, oper, flags, dentry)

#define fcfs_api_remove_dentry_by_pname(opname, flags, tid)  \
    fcfs_api_remove_dentry_by_pname_ex(&g_fcfs_api_ctx, \
            opname, flags, tid)

#define fcfs_api_remove_dentry(path, flags, tid)  \
    fcfs_api_remove_dentry_ex(&g_fcfs_api_ctx, path, flags, tid)

#define fcfs_api_rename_dentry_by_pname(src_parent_inode, src_name, \
        dest_parent_inode, dest_name, oper, flags, tid)  \
    fcfs_api_rename_dentry_by_pname_ex(&g_fcfs_api_ctx, src_parent_inode, \
            src_name, dest_parent_inode, dest_name, oper, flags, tid)

#define fcfs_api_rename_dentry(path1, path2, flags, tid) \
    fcfs_api_rename_dentry_ex(&g_fcfs_api_ctx, \
            path1, path2, flags, tid)

#define fcfs_api_modify_stat_by_inode(oino, attr, mflags, flags, dentry)  \
    fcfs_api_modify_stat_by_inode_ex(&g_fcfs_api_ctx, \
            oino, attr, mflags, flags, dentry)

#define fcfs_api_set_xattr_by_inode(inode, xattr, flags) \
    fcfs_api_set_xattr_by_inode_ex(&g_fcfs_api_ctx, inode, xattr, flags)

#define fcfs_api_set_xattr_by_path(path, xattr, flags) \
    fcfs_api_set_xattr_by_path_ex(&g_fcfs_api_ctx, path, xattr, flags)

#define fcfs_api_remove_xattr_by_inode(inode, name, flags) \
    fcfs_api_remove_xattr_by_inode_ex(&g_fcfs_api_ctx, inode, name, flags)

#define fcfs_api_remove_xattr_by_path(path, name, flags) \
    fcfs_api_remove_xattr_by_path_ex(&g_fcfs_api_ctx, path, name, flags)

#define fcfs_api_get_xattr_by_inode(inode, name, value, size, flags) \
    fcfs_api_get_xattr_by_inode_ex(&g_fcfs_api_ctx, inode, \
            name, LOG_DEBUG, value, size, flags)

#define fcfs_api_get_xattr_by_path(path, name, value, size, flags) \
    fcfs_api_get_xattr_by_path_ex(&g_fcfs_api_ctx, path, \
            name, LOG_DEBUG, value, size, flags)

#define fcfs_api_list_xattr_by_inode(inode, list, size, flags) \
    fcfs_api_list_xattr_by_inode_ex(&g_fcfs_api_ctx, inode, list, size, flags)

#define fcfs_api_list_xattr_by_path(path, list, size, flags) \
    fcfs_api_list_xattr_by_path_ex(&g_fcfs_api_ctx, path, list, size, flags)

#define fcfs_api_list_dentry_by_inode(inode, array)  \
    fcfs_api_list_dentry_by_inode_ex(&g_fcfs_api_ctx, inode, array)

#define fcfs_api_list_compact_dentry_by_path(path, array)  \
    fcfs_api_list_compact_dentry_by_path_ex(&g_fcfs_api_ctx, path, array)

#define fcfs_api_list_compact_dentry_by_inode(inode, array)  \
    fcfs_api_list_compact_dentry_by_inode_ex(&g_fcfs_api_ctx, inode, array)

#define fcfs_api_alloc_opendir_session()  \
    fcfs_api_alloc_opendir_session_ex(&g_fcfs_api_ctx)

#define fcfs_api_free_opendir_session(session) \
        fcfs_api_free_opendir_session_ex(&g_fcfs_api_ctx, session)

#define fcfs_api_dentry_sys_lock(session, inode, flags, file_size, space_end) \
    fcfs_api_dentry_sys_lock_ex(&g_fcfs_api_ctx, session, inode, \
            flags, file_size, space_end)

static inline int fcfs_api_lookup_inode_by_fullname_ex(FCFSAPIContext *ctx,
        const FDIRClientOperFnamePair *path, const int enoent_log_level,
        int64_t *inode)
{
    return fdir_client_lookup_inode_by_path_ex(ctx->contexts.fdir,
            path, enoent_log_level, inode);
}

static inline int fcfs_api_lookup_inode_by_path_ex(FCFSAPIContext *ctx,
        const char *path, const FDIRDentryOperator *oper,
        const int enoent_log_level, int64_t *inode)
{
    FDIRClientOperFnamePair fname;
    FCFSAPI_SET_PATH_OPER_FNAME(fname, ctx, *oper, path);
    return fdir_client_lookup_inode_by_path_ex(ctx->contexts.fdir,
            &fname, enoent_log_level, inode);
}

static inline int fcfs_api_stat_dentry_by_inode_ex(FCFSAPIContext *ctx,
        const FDIRClientOperInodePair *oino, const int flags,
        FDIRDEntryInfo *dentry)
{
    if (ctx->async_report.enabled) {
        inode_htable_check_conflict_and_wait(oino->inode);
    }
    return fdir_client_stat_dentry_by_inode(ctx->contexts.fdir,
            &ctx->ns, oino, flags, dentry);
}

static inline int fcfs_api_stat_dentry_by_fullname_ex(FCFSAPIContext *ctx,
        const FDIRClientOperFnamePair *path, const int flags,
        const int enoent_log_level, FDIRDEntryInfo *dentry)
{
    if (ctx->async_report.enabled) {
        int result;
        FDIRClientOperInodePair oino;
        if ((result=fcfs_api_lookup_inode_by_fullname_ex(ctx,
                        path, enoent_log_level, &oino.inode)) != 0)
        {
            return result;
        }

        oino.oper = path->oper;
        return fcfs_api_stat_dentry_by_inode_ex(ctx, &oino, flags, dentry);
    } else {
        return fdir_client_stat_dentry_by_path_ex(ctx->contexts.fdir,
                path, flags, enoent_log_level, dentry);
    }
}

static inline int fcfs_api_stat_dentry_by_path_ex(FCFSAPIContext *ctx,
        const char *path, const FDIRDentryOperator *oper, const int flags,
        const int enoent_log_level, FDIRDEntryInfo *dentry)
{
    FDIRClientOperFnamePair fname;
    FCFSAPI_SET_PATH_OPER_FNAME(fname, ctx, *oper, path);
    return fcfs_api_stat_dentry_by_fullname_ex(ctx, &fname,
            flags, enoent_log_level, dentry);
}

static inline int fcfs_api_stat_dentry_by_pname_ex(FCFSAPIContext *ctx,
        const FDIRClientOperPnamePair *opname, const int flags,
        const int enoent_log_level, FDIRDEntryInfo *dentry)
{
    if (ctx->async_report.enabled) {
        int result;
        FDIRClientOperInodePair oino;
        if ((result=fdir_client_lookup_inode_by_pname_ex(ctx->contexts.fdir,
                        &ctx->ns, opname, enoent_log_level, &oino.inode)) != 0)
        {
            return result;
        }

        oino.oper = opname->oper;
        return fcfs_api_stat_dentry_by_inode_ex(ctx, &oino, flags, dentry);
    } else {
        return fdir_client_stat_dentry_by_pname_ex(ctx->contexts.fdir,
                &ctx->ns, opname, flags, enoent_log_level, dentry);
    }
}

static inline int fcfs_api_access_dentry_by_inode_ex(FCFSAPIContext *ctx,
        const FDIRClientOperInodePair *oino, const char mask,
        const int flags, FDIRDEntryInfo *dentry)
{
    if (ctx->async_report.enabled && (flags & FDIR_FLAGS_OUTPUT_DENTRY)) {
        inode_htable_check_conflict_and_wait(oino->inode);
    }

    return fdir_client_access_dentry_by_inode(ctx->contexts.fdir,
            &ctx->ns, oino, mask, flags, dentry);
}

static inline int fcfs_api_access_dentry_by_path_ex(FCFSAPIContext *ctx,
        const FDIRClientOperFnamePair *path, const char mask,
        const int flags, FDIRDEntryInfo *dentry)
{
    if (ctx->async_report.enabled && (flags & FDIR_FLAGS_OUTPUT_DENTRY)) {
        int result;
        FDIRClientOperInodePair oino;

        if ((result=fcfs_api_lookup_inode_by_fullname_ex(ctx,
                        path, LOG_NOTHING, &oino.inode)) != 0)
        {
            return result;
        }

        oino.oper = path->oper;
        return fcfs_api_access_dentry_by_inode_ex(
                ctx, &oino, mask, flags, dentry);
    }

    return fdir_client_access_dentry_by_path(ctx->
            contexts.fdir, path, mask, flags, dentry);
}

static inline int fcfs_api_access_dentry_by_pname_ex(FCFSAPIContext *ctx,
        const FDIRClientOperPnamePair *opname, const char mask,
        const int flags, FDIRDEntryInfo *dentry)
{
    if (ctx->async_report.enabled && (flags & FDIR_FLAGS_OUTPUT_DENTRY)) {
        int result;
        FDIRClientOperInodePair oino;

        if ((result=fdir_client_lookup_inode_by_pname_ex(ctx->contexts.fdir,
                        &ctx->ns, opname, LOG_NOTHING, &oino.inode)) != 0)
        {
            return result;
        }

        oino.oper = opname->oper;
        return fcfs_api_access_dentry_by_inode_ex(
                ctx, &oino, mask, flags, dentry);
    }

    return fdir_client_access_dentry_by_pname(ctx->contexts.fdir,
            &ctx->ns, opname, mask, flags, dentry);
}

static inline int fcfs_api_create_dentry_by_pname_ex(FCFSAPIContext *ctx,
        const int64_t parent_inode, const string_t *name,
        const FDIRDentryOperator *oper, const mode_t mode,
        const dev_t rdev, FDIRDEntryInfo *dentry)
{
    FDIRClientOperPnamePair opname;
    FCFSAPI_SET_PATH_OPER_PNAME(opname, *oper, parent_inode, name);
    return fdir_client_create_dentry_by_pname_ex(ctx->contexts.fdir,
            &ctx->ns, &opname, mode, rdev, dentry);
}

static inline int fcfs_api_symlink_dentry_by_pname_ex(FCFSAPIContext *ctx,
        const string_t *link, const int64_t parent_inode,
        const string_t *name, const FDIRDentryOperator *oper,
        const mode_t mode, FDIRDEntryInfo *dentry)
{
    FDIRClientOperPnamePair opname;
    FCFSAPI_SET_PATH_OPER_PNAME(opname, *oper, parent_inode, name);
    return fdir_client_symlink_dentry_by_pname(ctx->contexts.fdir,
            link, &ctx->ns, &opname, mode, dentry);
}

static inline int fcfs_api_readlink_by_pname_ex(FCFSAPIContext *ctx,
        const FDIRClientOperPnamePair *opname,
        string_t *link, const int size)
{
    return fdir_client_readlink_by_pname(ctx->contexts.fdir,
            &ctx->ns, opname, link, size);
}

static inline int fcfs_api_readlink_by_inode_ex(FCFSAPIContext *ctx,
        const FDIRClientOperInodePair *oino, string_t *link, const int size)
{
    return fdir_client_readlink_by_inode(ctx->contexts.fdir,
            &ctx->ns, oino, link, size);
}

int fcfs_api_remove_dentry_by_pname_ex(FCFSAPIContext *ctx,
        const FDIRClientOperPnamePair *opname,
        const int flags, const int64_t tid);

int fcfs_api_remove_dentry_ex(FCFSAPIContext *ctx,
        const FDIRClientOperFnamePair *path,
        const int flags, const int64_t tid);

int fcfs_api_rename_dentry_by_pname_ex(FCFSAPIContext *ctx,
        const int64_t src_parent_inode, const string_t *src_name,
        const int64_t dest_parent_inode, const string_t *dest_name,
        const FDIRDentryOperator *oper, const int flags,
        const int64_t tid);

int fcfs_api_rename_dentry_ex(FCFSAPIContext *ctx, const char *path1,
        const char *path2, const FDIRDentryOperator *oper,
        const int flags, const int64_t tid);

static inline int fcfs_api_modify_stat_by_inode_ex(FCFSAPIContext *ctx,
        const FDIRClientOperInodePair *oino, const struct stat *attr,
        const int64_t mflags, const int flags, FDIRDEntryInfo *dentry)
{
    FDIRDEntryStat stat;

    if (ctx->async_report.enabled) {
        inode_htable_check_conflict_and_wait(oino->inode);
    }
    stat.mode = attr->st_mode;
    stat.gid = attr->st_gid;
    stat.uid = attr->st_uid;
    stat.atime = attr->st_atime;
    stat.ctime = attr->st_ctime;
    stat.mtime = attr->st_mtime;
    stat.size = attr->st_size;
    return fdir_client_modify_stat_by_inode(ctx->contexts.fdir,
            &ctx->ns, oino, mflags, &stat, flags, dentry);
}

#define FCFS_API_SET_UTIMES(stat, options, times) \
    do { \
        options.flags = 0; \
        options.atime = options.mtime = 1; \
        memset(&stat, 0, sizeof(stat)); \
        if (times == NULL) { \
            stat.atime = stat.mtime = get_current_time(); \
        } else { \
            stat.atime = times[0].tv_sec; \
            stat.mtime = times[1].tv_sec; \
        } \
    } while (0)


#define SET_ONE_UTIMENS(var, option_flag, option_now, tp) \
    do { \
        if (tp.tv_nsec == UTIME_NOW) { \
            var = get_current_time();  \
            option_flag = 1; \
            option_now = 1;  \
        } else if (tp.tv_nsec == UTIME_OMIT) { \
            option_flag = 0; \
        } else { \
            var = tp.tv_sec; \
            option_flag = 1; \
        } \
    } while (0)

#define FCFS_API_SET_UTIMENS(stat, options, times) \
    do { \
        options.flags = 0; \
        memset(&stat, 0, sizeof(stat)); \
        if (times == NULL) { \
            stat.atime = stat.mtime = get_current_time(); \
        } else { \
            SET_ONE_UTIMENS(stat.atime, options.atime, \
		options.atime_now, times[0]); \
            SET_ONE_UTIMENS(stat.mtime, options.mtime, \
		options.mtime_now, times[1]); \
        } \
    } while (0)

static inline int fcfs_api_utimes_by_inode_ex(FCFSAPIContext *ctx,
        const FDIRClientOperInodePair *oino, const struct timeval times[2],
        const int flags)
{
    FDIRStatModifyFlags options;
    FDIRDEntryStat stat;
    FDIRDEntryInfo dentry;

    if (ctx->async_report.enabled) {
        inode_htable_check_conflict_and_wait(oino->inode);
    }

    FCFS_API_SET_UTIMES(stat, options, times);
    return fdir_client_modify_stat_by_inode(ctx->contexts.fdir,
            &ctx->ns, oino, options.flags, &stat, flags, &dentry);
}

static inline int fcfs_api_utimes_by_path_ex(FCFSAPIContext *ctx,
        const FDIRClientOperFnamePair *path,
        const struct timeval times[2], const int flags)
{
    FDIRStatModifyFlags options;
    FDIRDEntryStat stat;
    FDIRDEntryInfo dentry;

    FCFS_API_SET_UTIMES(stat, options, times);
    return fdir_client_modify_stat_by_path(ctx->contexts.fdir,
            path, options.flags, &stat, flags, &dentry);
}

static inline int fcfs_api_utimens_by_inode_ex(FCFSAPIContext *ctx,
        const FDIRClientOperInodePair *oino, const struct timespec times[2],
        const int flags)
{
    FDIRStatModifyFlags options;
    FDIRDEntryStat stat;
    FDIRDEntryInfo dentry;

    if (ctx->async_report.enabled) {
        inode_htable_check_conflict_and_wait(oino->inode);
    }

    FCFS_API_SET_UTIMENS(stat, options, times);
    if (options.flags == 0) {
        return 0;
    } else {
        return fdir_client_modify_stat_by_inode(ctx->contexts.fdir,
                &ctx->ns, oino, options.flags, &stat, flags, &dentry);
    }
}

static inline int fcfs_api_utimens_by_path_ex(FCFSAPIContext *ctx,
        const FDIRClientOperFnamePair *path,
        const struct timespec times[2], const int flags)
{
    FDIRStatModifyFlags options;
    FDIRDEntryStat stat;
    FDIRDEntryInfo dentry;

    FCFS_API_SET_UTIMENS(stat, options, times);
    if (options.flags == 0) {
        return 0;
    } else {
        return fdir_client_modify_stat_by_path(ctx->contexts.fdir,
                path, options.flags, &stat, flags, &dentry);
    }
}

static inline int fcfs_api_utime_ex(FCFSAPIContext *ctx,
        const FDIRClientOperFnamePair *path,
        const struct utimbuf *times)
{
    const int flags = FDIR_FLAGS_FOLLOW_SYMLINK;
    FDIRStatModifyFlags options;
    FDIRDEntryStat stat;
    FDIRDEntryInfo dentry;

    options.flags = 0;
    options.atime = options.mtime = 1;
    memset(&stat, 0, sizeof(stat));
    if (times == NULL) {
        stat.atime = stat.mtime = get_current_time();
    } else {
        stat.atime = times->actime;
        stat.mtime = times->modtime;
    }

    return fdir_client_modify_stat_by_path(ctx->contexts.fdir,
            path, options.flags, &stat, flags, &dentry);
}

static inline int fcfs_api_chown_by_inode_ex(FCFSAPIContext *ctx,
        const FDIRClientOperInodePair *oino, const uid_t uid,
        const gid_t gid, const int flags)
{
    FDIRStatModifyFlags options;
    FDIRDEntryStat stat;
    FDIRDEntryInfo dentry;

    options.flags = 0;
    options.uid = options.gid = 1;
    memset(&stat, 0, sizeof(stat));
    stat.uid = uid;
    stat.gid = gid;
    return fdir_client_modify_stat_by_inode(ctx->contexts.fdir,
            &ctx->ns, oino, options.flags, &stat, flags, &dentry);
}

static inline int fcfs_api_chown_ex(FCFSAPIContext *ctx,
        const FDIRClientOperFnamePair *path, const uid_t uid,
        const gid_t gid, const int flags)
{
    FDIRStatModifyFlags options;
    FDIRDEntryStat stat;
    FDIRDEntryInfo dentry;

    options.flags = 0;
    options.uid = options.gid = 1;
    memset(&stat, 0, sizeof(stat));
    stat.uid = uid;
    stat.gid = gid;
    return fdir_client_modify_stat_by_path(ctx->contexts.fdir,
            path, options.flags, &stat, flags, &dentry);
}

static inline int fcfs_api_chmod_by_inode_ex(FCFSAPIContext *ctx,
        const FDIRClientOperInodePair *oino, const mode_t mode,
        const int flags)
{
    FDIRStatModifyFlags options;
    FDIRDEntryStat stat;
    FDIRDEntryInfo dentry;

    options.flags = 0;
    options.mode = 1;
    memset(&stat, 0, sizeof(stat));
    stat.mode = (mode & ALLPERMS);
    return fdir_client_modify_stat_by_inode(ctx->contexts.fdir,
            &ctx->ns, oino, options.flags, &stat, flags, &dentry);
}

static inline int fcfs_api_chmod_ex(FCFSAPIContext *ctx,
        const FDIRClientOperFnamePair *path,
        const mode_t mode, const int flags)
{
    FDIRStatModifyFlags options;
    FDIRDEntryStat stat;
    FDIRDEntryInfo dentry;

    options.flags = 0;
    options.mode = 1;
    memset(&stat, 0, sizeof(stat));
    stat.mode = (mode & ALLPERMS);
    return fdir_client_modify_stat_by_path(ctx->contexts.fdir,
            path, options.flags, &stat, flags, &dentry);
}

static inline int convert_xattr_flags(const int flags)
{
    int new_flags;

    if (flags == 0) {
        return 0;
    }

    new_flags = (flags & FDIR_FLAGS_FOLLOW_SYMLINK);
    if ((flags & XATTR_CREATE) != 0) {
        return (new_flags | FDIR_FLAGS_XATTR_CREATE);
    } else if ((flags & XATTR_REPLACE) != 0) {
        return (new_flags | FDIR_FLAGS_XATTR_REPLACE);
    } else {
        return new_flags;
    }
}

static inline int fcfs_api_set_xattr_by_inode_ex(FCFSAPIContext *ctx,
        const FDIRClientOperInodePair *oino, const key_value_pair_t *xattr,
        const int flags)
{
    return fdir_client_set_xattr_by_inode(ctx->contexts.fdir,
            &ctx->ns, oino, xattr, convert_xattr_flags(flags));
}

static inline int fcfs_api_set_xattr_by_path_ex(FCFSAPIContext *ctx,
        const FDIRClientOperFnamePair *path,
        const key_value_pair_t *xattr, const int flags)
{
    return fdir_client_set_xattr_by_path(ctx->contexts.fdir,
            path, xattr, convert_xattr_flags(flags));
}

static inline int fcfs_api_remove_xattr_by_inode_ex(FCFSAPIContext *ctx,
        const FDIRClientOperInodePair *oino,
        const string_t *name, const int flags)
{
    return fdir_client_remove_xattr_by_inode_ex(ctx->contexts.fdir,
            &ctx->ns, oino, name, flags, LOG_DEBUG);
}

static inline int fcfs_api_remove_xattr_by_path_ex(FCFSAPIContext *ctx,
        const FDIRClientOperFnamePair *path,
        const string_t *name, const int flags)
{
    return fdir_client_remove_xattr_by_path_ex(ctx->contexts.fdir,
            path, name, flags, LOG_DEBUG);
}

static inline int fcfs_api_get_xattr_by_inode_ex(FCFSAPIContext *ctx,
        const FDIRClientOperInodePair *oino, const string_t *name,
        const int enoattr_log_level, string_t *value, const int size,
        const int flags)
{
    return fdir_client_get_xattr_by_inode_ex(ctx->contexts.fdir, &ctx->ns,
            oino, name, enoattr_log_level, value, size, flags);
}

static inline int fcfs_api_get_xattr_by_path_ex(FCFSAPIContext *ctx,
        const FDIRClientOperFnamePair *path, const string_t *name,
        const int enoattr_log_level, string_t *value,
        const int size, const int flags)
{
    return fdir_client_get_xattr_by_path_ex(ctx->contexts.fdir,
            path, name, enoattr_log_level, value, size, flags);
}

static inline int fcfs_api_list_xattr_by_inode_ex(FCFSAPIContext *ctx,
        const FDIRClientOperInodePair *oino, string_t *list,
        const int size, const int flags)
{
    return fdir_client_list_xattr_by_inode(ctx->contexts.fdir,
            &ctx->ns, oino, list, size, flags);
}

static inline int fcfs_api_list_xattr_by_path_ex(FCFSAPIContext *ctx,
        const FDIRClientOperFnamePair *path, string_t *list,
        const int size, const int flags)
{
    return fdir_client_list_xattr_by_path(ctx->contexts.fdir,
            path, list, size, flags);
}

static inline int fcfs_api_list_dentry_by_inode_ex(FCFSAPIContext *ctx,
        const FDIRClientOperInodePair *oino, FDIRClientDentryArray *array)
{
    const int flags = FDIR_LIST_DENTRY_FLAGS_OUTPUT_SPECIAL;
    if (ctx->async_report.enabled) {
        async_reporter_wait_all(oino->inode);
    }
    return fdir_client_list_dentry_by_inode(ctx->contexts.fdir,
            &ctx->ns, oino, array, flags);
}

static inline int fcfs_api_list_compact_dentry_by_path_ex(FCFSAPIContext *ctx,
        const FDIRClientOperFnamePair *path,
        FDIRClientCompactDentryArray *array)
{
    return fdir_client_list_compact_dentry_by_path(
            ctx->contexts.fdir, path, array);
}

static inline int fcfs_api_list_compact_dentry_by_inode_ex(
        FCFSAPIContext *ctx, const FDIRClientOperInodePair *oino,
        FDIRClientCompactDentryArray *array)
{
    return fdir_client_list_compact_dentry_by_inode(
            ctx->contexts.fdir, &ctx->ns, oino, array);
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
    return fdir_client_dentry_sys_lock(session, &ctx->ns,
            inode, flags, file_size, space_end);
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
        const string_t *dest_name, const FDIRDentryOperator *oper,
        const mode_t mode, const int flags, FDIRDEntryInfo *dentry)
{
    FDIRClientOperPnamePair dest_opname;
    FCFSAPI_SET_PATH_OPER_PNAME(dest_opname, *oper,
            dest_parent_inode, dest_name);
    return fdir_client_link_dentry_by_pname(ctx->contexts.fdir,
            src_inode, &ctx->ns, &dest_opname, mode, flags, dentry);
}

#ifdef __cplusplus
}
#endif

#endif
