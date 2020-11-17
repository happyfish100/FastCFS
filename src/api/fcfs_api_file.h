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


#ifndef _FS_API_FILE_H
#define _FS_API_FILE_H

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <fcntl.h>
#include "fcfs_api_types.h"

#ifndef FALLOC_FL_KEEP_SIZE
#define FALLOC_FL_KEEP_SIZE  0x01
#endif

#ifndef FALLOC_FL_PUNCH_HOLE
#define FALLOC_FL_PUNCH_HOLE 0x02
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define fcfs_api_create(fi, path, omp) \
    fcfs_api_create_ex(&g_fcfs_api_ctx, fi, path, omp)

#define fcfs_api_open(fi, path, flags, omp) \
    fcfs_api_open_ex(&g_fcfs_api_ctx, fi, path, flags, omp)

#define fcfs_api_open_by_inode(fi, inode, flags, omp) \
    fcfs_api_open_by_inode_ex(&g_fcfs_api_ctx, fi, inode, flags, omp)

#define fcfs_api_open_by_dentry(fi, dentry, flags, omp) \
    fcfs_api_open_by_dentry_ex(&g_fcfs_api_ctx, fi, dentry, flags, omp)

#define fcfs_api_truncate(path, new_size) \
    fcfs_api_truncate_ex(&g_fcfs_api_ctx, path, new_size)

#define fcfs_api_unlink(path)  \
    fcfs_api_unlink_ex(&g_fcfs_api_ctx, path)

#define fcfs_api_stat(path, buf)  \
    fcfs_api_stat_ex(&g_fcfs_api_ctx, path, buf)

#define fcfs_api_rename(old_path, new_path)  \
    fcfs_api_rename_ex(&g_fcfs_api_ctx, old_path, new_path, 0)

#define fcfs_api_statvfs(path, stbuf)  \
    fcfs_api_statvfs_ex(&g_fcfs_api_ctx, path, stbuf)

    int fcfs_api_open_ex(FSAPIContext *ctx, FSAPIFileInfo *fi,
            const char *path, const int flags,
            const FDIRClientOwnerModePair *omp);

    int fcfs_api_open_by_inode_ex(FSAPIContext *ctx, FSAPIFileInfo *fi,
            const int64_t inode, const int flags,
            const FDIRClientOwnerModePair *omp);

    int fcfs_api_open_by_dentry_ex(FSAPIContext *ctx, FSAPIFileInfo *fi,
            const FDIRDEntryInfo *dentry, const int flags,
            const FDIRClientOwnerModePair *omp);

    static inline int fcfs_api_create_ex(FSAPIContext *ctx, FSAPIFileInfo *fi,
            const char *path, const FDIRClientOwnerModePair *omp)
    {
        const int flags = O_CREAT | O_TRUNC | O_WRONLY;
        return fcfs_api_open_ex(ctx, fi, path, flags, omp);
    }

    int fcfs_api_close(FSAPIFileInfo *fi);

    int fcfs_api_pwrite(FSAPIFileInfo *fi, const char *buff,
            const int size, const int64_t offset, int *written_bytes);

    int fcfs_api_write(FSAPIFileInfo *fi, const char *buff,
            const int size, int *written_bytes);

    int fcfs_api_pread(FSAPIFileInfo *fi, char *buff, const int size,
            const int64_t offset, int *read_bytes);

    int fcfs_api_read(FSAPIFileInfo *fi, char *buff,
            const int size, int *read_bytes);

    int fcfs_api_ftruncate(FSAPIFileInfo *fi, const int64_t new_size);

    int fcfs_api_truncate_ex(FSAPIContext *ctx, const char *path,
            const int64_t new_size);

    int fcfs_api_unlink_ex(FSAPIContext *ctx, const char *path);

    int fcfs_api_lseek(FSAPIFileInfo *fi, const int64_t offset, const int whence);

    int fcfs_api_fstat(FSAPIFileInfo *fi, struct stat *buf);

    int fcfs_api_stat_ex(FSAPIContext *ctx, const char *path, struct stat *buf);

    int fcfs_api_flock_ex2(FSAPIFileInfo *fi, const int operation,
            const int64_t owner_id, const pid_t pid);

    static inline int fcfs_api_flock_ex(FSAPIFileInfo *fi, const int operation,
            const int64_t owner_id)
    {
        return fcfs_api_flock_ex2(fi, operation, owner_id, getpid());
    }

    static inline int fcfs_api_flock(FSAPIFileInfo *fi, const int operation)
    {
        return fcfs_api_flock_ex2(fi, operation, (long)pthread_self(), getpid());
    }

    int fcfs_api_getlk(FSAPIFileInfo *fi, struct flock *lock, int64_t *owner_id);

    int fcfs_api_setlk(FSAPIFileInfo *fi, const struct flock *lock,
        const int64_t owner_id);

    int fcfs_api_fallocate(FSAPIFileInfo *fi, const int mode,
            const int64_t offset, const int64_t len);

    int fcfs_api_rename_ex(FSAPIContext *ctx, const char *old_path,
            const char *new_path, const int flags);

    int fcfs_api_symlink_ex(FSAPIContext *ctx, const char *target,
            const char *path, const FDIRClientOwnerModePair *omp);

    int fcfs_api_readlink(FSAPIContext *ctx, const char *path,
            char *buff, const int size);

    int fcfs_api_link_ex(FSAPIContext *ctx, const char *old_path,
            const char *new_path, const FDIRClientOwnerModePair *omp);

    int fcfs_api_statvfs_ex(FSAPIContext *ctx, const char *path,
            struct statvfs *stbuf);

#ifdef __cplusplus
}
#endif

#endif
