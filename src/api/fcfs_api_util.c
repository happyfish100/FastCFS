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
#include "fastcommon/sockopt.h"
#include "fastcommon/sched_thread.h"
#include "fcfs_api_util.h"

int fcfs_api_remove_dentry_by_pname_ex(FCFSAPIContext *ctx,
        const int64_t parent_inode, const string_t *name,
        const int flags, const int64_t tid)
{
    FDIRDEntryPName pname;
    FDIRDEntryInfo dentry;
    int result;

    FDIR_SET_DENTRY_PNAME_PTR(&pname, parent_inode, name);
    if ((result=fdir_client_remove_dentry_by_pname_ex(
            ctx->contexts.fdir, &ctx->ns, &pname,
            flags, &dentry)) != 0)
    {
        return result;
    }

    if (S_ISREG(dentry.stat.mode) && dentry.stat.nlink == 0) {
        result = fs_api_unlink_file(ctx->contexts.fsapi,
                dentry.inode, dentry.stat.size, tid);
    }
    return result;
}

int fcfs_api_remove_dentry_ex(FCFSAPIContext *ctx,
        const char *path, const int flags, const int64_t tid)
{
    FDIRDEntryFullName fullname;
    FDIRDEntryInfo dentry;
    int result;

    FCFSAPI_SET_PATH_FULLNAME(fullname, ctx, path);
    if ((result=fdir_client_remove_dentry_ex(ctx->contexts.fdir,
                    &fullname, flags, &dentry)) != 0)
    {
        return result;
    }

    if (S_ISREG(dentry.stat.mode) && dentry.stat.nlink == 0) {
        result = fs_api_unlink_file(ctx->contexts.fsapi,
                dentry.inode, dentry.stat.size, tid);
    }
    return result;
}

int fcfs_api_rename_dentry_by_pname_ex(FCFSAPIContext *ctx,
        const int64_t src_parent_inode, const string_t *src_name,
        const int64_t dest_parent_inode, const string_t *dest_name,
        const int flags, const int64_t tid)
{
    FDIRDEntryPName src_pname;
    FDIRDEntryPName dest_pname;
    FDIRDEntryInfo dentry;
    FDIRDEntryInfo *pe;
    int result;

    FDIR_SET_DENTRY_PNAME_PTR(&src_pname, src_parent_inode, src_name);
    FDIR_SET_DENTRY_PNAME_PTR(&dest_pname, dest_parent_inode, dest_name);
    pe = &dentry;
    if ((result=fdir_client_rename_dentry_by_pname_ex(ctx->contexts.fdir,
                    &ctx->ns, &src_pname, &ctx->ns, &dest_pname, flags,
                    &pe)) != 0)
    {
        return result;
    }

    if (pe != NULL && S_ISREG(pe->stat.mode) && pe->stat.nlink == 0) {
        fs_api_unlink_file(ctx->contexts.fsapi, pe->inode,
                pe->stat.size, tid);
    }
    return result;
}

int fcfs_api_rename_dentry_ex(FCFSAPIContext *ctx,
        const char *path1, const char *path2,
        const int flags, const int64_t tid)
{
    FDIRDEntryFullName src;
    FDIRDEntryFullName dest;
    FDIRDEntryInfo dentry;
    FDIRDEntryInfo *pe;
    int result;

    FCFSAPI_SET_PATH_FULLNAME(src, ctx, path1);
    FCFSAPI_SET_PATH_FULLNAME(dest, ctx, path2);
    pe = &dentry;
    if ((result=fdir_client_rename_dentry_ex(ctx->contexts.fdir,
                    &src, &dest, flags, &pe)) != 0)
    {
        return result;
    }

    if (pe != NULL && S_ISREG(pe->stat.mode) && pe->stat.nlink == 0) {
        fs_api_unlink_file(ctx->contexts.fsapi, pe->inode,
                pe->stat.size, tid);
    }
    return result;
}
