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


#ifndef _FCFS_API_FILE_H
#define _FCFS_API_FILE_H

#include <sys/types.h>
#include <sys/statvfs.h>
#include <fcntl.h>
#include <utime.h>
#include "fcfs_api_types.h"
#include "fcfs_api_util.h"

#ifndef FALLOC_FL_KEEP_SIZE
#define FALLOC_FL_KEEP_SIZE  0x01
#endif

#ifndef FALLOC_FL_PUNCH_HOLE
#define FALLOC_FL_PUNCH_HOLE 0x02
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define fcfs_api_create(fi, path, fctx) \
    fcfs_api_create_ex(&g_fcfs_api_ctx, fi, path, fctx)

#define fcfs_api_open(fi, path, flags, fctx) \
    fcfs_api_open_ex(&g_fcfs_api_ctx, fi, path, flags, fctx)

#define fcfs_api_open_by_inode(fi, inode, flags, fctx) \
    fcfs_api_open_by_inode_ex(&g_fcfs_api_ctx, fi, inode, flags, fctx)

#define fcfs_api_open_by_dentry(fi, dentry, flags, fctx) \
    fcfs_api_open_by_dentry_ex(&g_fcfs_api_ctx, fi, dentry, flags, fctx)

#define fcfs_api_write(fi, buff, size, written_bytes)  \
    fcfs_api_write_ex(fi, buff, size, written_bytes, (fi)->tid)

#define fcfs_api_pwrite(fi, buff, size, offset, written_bytes)  \
    fcfs_api_pwrite_ex(fi, buff, size, offset, written_bytes, (fi)->tid)

#define fcfs_api_read(fi, buff, size, read_bytes)  \
    fcfs_api_read_ex(fi, buff, size, read_bytes, (fi)->tid)

#define fcfs_api_pread(fi, buff, size, offset, read_bytes)  \
    fcfs_api_pread_ex(fi, buff, size, offset, read_bytes, (fi)->tid)

#define fcfs_api_writev(fi, iov, iovcnt, written_bytes)  \
    fcfs_api_writev_ex(fi, iov, iovcnt, written_bytes, (fi)->tid)

#define fcfs_api_pwritev(fi, iov, iovcnt, offset, written_bytes)  \
    fcfs_api_pwritev_ex(fi, iov, iovcnt, offset, written_bytes, (fi)->tid)

#define fcfs_api_readv(fi, iov, iovcnt, read_bytes)  \
    fcfs_api_readv_ex(fi, iov, iovcnt, read_bytes, (fi)->tid)

#define fcfs_api_preadv(fi, iov, iovcnt, offset, read_bytes)  \
    fcfs_api_preadv_ex(fi, iov, iovcnt, offset, read_bytes, (fi)->tid)

#define fcfs_api_ftruncate(fi, new_size) \
    fcfs_api_ftruncate_ex(fi, new_size, getpid())

#define fcfs_api_truncate(path, new_size, fctx) \
    fcfs_api_truncate_ex(&g_fcfs_api_ctx, path, new_size, fctx)

#define fcfs_api_unlink(path, tid)  \
    fcfs_api_unlink_ex(&g_fcfs_api_ctx, path, tid)

#define fcfs_api_stat(path, buf, flags)  \
    fcfs_api_stat_ex(&g_fcfs_api_ctx, path, buf, flags)

#define fcfs_api_lstat(path, buf)  \
    fcfs_api_lstat_ex(&g_fcfs_api_ctx, path, buf)

#define fcfs_api_rename(old_path, new_path, fctx)  \
    fcfs_api_rename_ex(&g_fcfs_api_ctx, old_path, new_path, 0, fctx)

#define fcfs_api_statvfs(path, stbuf)  \
    fcfs_api_statvfs_ex(&g_fcfs_api_ctx, path, stbuf)

    int fcfs_api_open_ex(FCFSAPIContext *ctx, FCFSAPIFileInfo *fi,
            const char *path, const int flags,
            const FCFSAPIFileContext *fctx);

    int fcfs_api_open_by_inode_ex(FCFSAPIContext *ctx, FCFSAPIFileInfo *fi,
            const int64_t inode, const int flags,
            const FCFSAPIFileContext *fctx);

    int fcfs_api_open_by_dentry_ex(FCFSAPIContext *ctx, FCFSAPIFileInfo *fi,
            const FDIRDEntryInfo *dentry, const int flags,
            const FCFSAPIFileContext *fctx);

    static inline int fcfs_api_create_ex(FCFSAPIContext *ctx,
            FCFSAPIFileInfo *fi, const char *path,
            const FCFSAPIFileContext *fctx)
    {
        const int flags = O_CREAT | O_TRUNC | O_WRONLY;
        return fcfs_api_open_ex(ctx, fi, path, flags, fctx);
    }

    int fcfs_api_close(FCFSAPIFileInfo *fi);

    void fcfs_api_file_write_done_callback(
            FSAPIWriteDoneCallbackArg *callback_arg);

    int fcfs_api_pwrite_ex(FCFSAPIFileInfo *fi, const char *buff,
            const int size, const int64_t offset, int *written_bytes,
            const int64_t tid);

    int fcfs_api_write_ex(FCFSAPIFileInfo *fi, const char *buff,
            const int size, int *written_bytes, const int64_t tid);

    int fcfs_api_pread_ex(FCFSAPIFileInfo *fi, char *buff, const int size,
            const int64_t offset, int *read_bytes, const int64_t tid);

    int fcfs_api_read_ex(FCFSAPIFileInfo *fi, char *buff,
            const int size, int *read_bytes, const int64_t tid);

    int fcfs_api_pwritev_ex(FCFSAPIFileInfo *fi, const struct iovec *iov,
            const int iovcnt, const int64_t offset, int *written_bytes,
            const int64_t tid);

    int fcfs_api_writev_ex(FCFSAPIFileInfo *fi, const struct iovec *iov,
            const int iovcnt, int *written_bytes, const int64_t tid);

    int fcfs_api_preadv_ex(FCFSAPIFileInfo *fi, const struct iovec *iov,
            const int iovcnt, const int64_t offset, int *read_bytes,
            const int64_t tid);

    int fcfs_api_readv_ex(FCFSAPIFileInfo *fi, const struct iovec *iov,
            const int iovcnt, int *read_bytes, const int64_t tid);

    int fcfs_api_ftruncate_ex(FCFSAPIFileInfo *fi, const int64_t new_size,
            const int64_t tid);

    int fcfs_api_truncate_ex(FCFSAPIContext *ctx, const char *path,
            const int64_t new_size, const FCFSAPIFileContext *fctx);

    int fcfs_api_fallocate_ex(FCFSAPIFileInfo *fi, const int mode,
            const int64_t offset, const int64_t len, const int64_t tid);

    static inline int fcfs_api_unlink_ex(FCFSAPIContext *ctx,
            const char *path, const int64_t tid)
    {
        const int flags = FDIR_UNLINK_FLAGS_MATCH_FILE;
        return fcfs_api_remove_dentry_ex(ctx, path, flags, tid);
    }

    int fcfs_api_lseek(FCFSAPIFileInfo *fi, const int64_t offset,
            const int whence);

    int fcfs_api_fstat(FCFSAPIFileInfo *fi, struct stat *buf);

    int fcfs_api_lstat_ex(FCFSAPIContext *ctx, const char *path,
            struct stat *buf);

    int fcfs_api_stat_ex(FCFSAPIContext *ctx, const char *path,
            struct stat *buf, const int flags);

    int fcfs_api_flock_ex2(FCFSAPIFileInfo *fi, const int operation,
            const int64_t owner_id, const pid_t pid);

    static inline int fcfs_api_flock_ex(FCFSAPIFileInfo *fi,
            const int operation, const int64_t owner_id)
    {
        return fcfs_api_flock_ex2(fi, operation, owner_id, getpid());
    }

    static inline int fcfs_api_flock(FCFSAPIFileInfo *fi, const int operation)
    {
        return fcfs_api_flock_ex2(fi, operation,
                (long)pthread_self(), getpid());
    }

    int fcfs_api_getlk_ex(FCFSAPIFileInfo *fi, struct flock *lock,
            int64_t *owner_id);

    static inline int fcfs_api_getlk(FCFSAPIFileInfo *fi, struct flock *lock)
    {
        int64_t owner_id;
        return fcfs_api_getlk_ex(fi, lock, &owner_id);
    }

    int fcfs_api_setlk_ex(FCFSAPIFileInfo *fi, const struct flock *lock,
            const int64_t owner_id, const bool blocked);

    static inline int fcfs_api_setlk(FCFSAPIFileInfo *fi,
            const struct flock *lock)
    {
        const bool blocked = false;
        int64_t owner_id;

        owner_id = fc_gettid();
        return fcfs_api_setlk_ex(fi, lock, owner_id, blocked);
    }

    static inline int fcfs_api_setlkw(FCFSAPIFileInfo *fi,
            const struct flock *lock)
    {
        const bool blocked = true;
        int64_t owner_id;

        owner_id = fc_gettid();
        return fcfs_api_setlk_ex(fi, lock, owner_id, blocked);
    }

    int fcfs_api_rename_ex(FCFSAPIContext *ctx, const char *old_path,
            const char *new_path, const int flags,
            const FCFSAPIFileContext *fctx);

    int fcfs_api_symlink_ex(FCFSAPIContext *ctx, const char *target,
            const char *path, const FDIRClientOwnerModePair *omp);

    int fcfs_api_readlink(FCFSAPIContext *ctx, const char *path,
            char *buff, const int size);

    int fcfs_api_link_ex(FCFSAPIContext *ctx, const char *old_path,
            const char *new_path, const FDIRClientOwnerModePair *omp,
            const int flags);

    int fcfs_api_mknod_ex(FCFSAPIContext *ctx, const char *path,
            const FDIRClientOwnerModePair *omp, const dev_t dev);

    int fcfs_api_mkfifo_ex(FCFSAPIContext *ctx, const char *path,
            FDIRClientOwnerModePair *omp);

    int fcfs_api_mkdir_ex(FCFSAPIContext *ctx, const char *path,
            FDIRClientOwnerModePair *omp);

    int fcfs_api_access_ex(FCFSAPIContext *ctx, const char *path,
            const int mask, const FDIRClientOwnerModePair *omp,
            const int flags);

    int fcfs_api_euidaccess_ex(FCFSAPIContext *ctx, const char *path,
            const int mask, const FDIRClientOwnerModePair *omp,
            const int flags);

    static inline int fcfs_api_eaccess_ex(FCFSAPIContext *ctx, const char *path,
            const int mask, const FDIRClientOwnerModePair *omp,
            const int flags)
    {
        return fcfs_api_euidaccess_ex(ctx, path, mask, omp, flags);
    }

    int fcfs_api_statvfs_ex(FCFSAPIContext *ctx, const char *path,
            struct statvfs *stbuf);

    int fcfs_api_set_file_flags(FCFSAPIFileInfo *fi, const int flags);

    static inline int fcfs_api_fdatasync(FCFSAPIFileInfo *fi,
            const int64_t tid)
    {
        fs_api_datasync(fi->ctx->contexts.fsapi, fi->dentry.inode, tid);
        return 0;
    }

    static inline int fcfs_api_fsync(FCFSAPIFileInfo *fi, const int64_t tid)
    {
        fs_api_datasync(fi->ctx->contexts.fsapi, fi->dentry.inode, tid);
        if (fi->ctx->async_report.enabled) {
            inode_htable_check_conflict_and_wait(fi->dentry.inode);
        }
        return 0;
    }

#ifdef __cplusplus
}
#endif

#endif
