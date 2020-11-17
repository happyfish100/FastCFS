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

#define fsapi_create(fi, path, omp) \
    fsapi_create_ex(&g_fcfs_api_ctx, fi, path, omp)

#define fsapi_open(fi, path, flags, omp) \
    fsapi_open_ex(&g_fcfs_api_ctx, fi, path, flags, omp)

#define fsapi_open_by_inode(fi, inode, flags, omp) \
    fsapi_open_by_inode_ex(&g_fcfs_api_ctx, fi, inode, flags, omp)

#define fsapi_open_by_dentry(fi, dentry, flags, omp) \
    fsapi_open_by_dentry_ex(&g_fcfs_api_ctx, fi, dentry, flags, omp)

#define fsapi_truncate(path, new_size) \
    fsapi_truncate_ex(&g_fcfs_api_ctx, path, new_size)

#define fsapi_unlink(path)  \
    fsapi_unlink_ex(&g_fcfs_api_ctx, path)

#define fsapi_stat(path, buf)  \
    fsapi_stat_ex(&g_fcfs_api_ctx, path, buf)

#define fsapi_rename(old_path, new_path)  \
    fsapi_rename_ex(&g_fcfs_api_ctx, old_path, new_path, 0)

#define fsapi_statvfs(path, stbuf)  \
    fsapi_statvfs_ex(&g_fcfs_api_ctx, path, stbuf)

    int fsapi_open_ex(FSAPIContext *ctx, FSAPIFileInfo *fi,
            const char *path, const int flags,
            const FDIRClientOwnerModePair *omp);

    int fsapi_open_by_inode_ex(FSAPIContext *ctx, FSAPIFileInfo *fi,
            const int64_t inode, const int flags,
            const FDIRClientOwnerModePair *omp);

    int fsapi_open_by_dentry_ex(FSAPIContext *ctx, FSAPIFileInfo *fi,
            const FDIRDEntryInfo *dentry, const int flags,
            const FDIRClientOwnerModePair *omp);

    static inline int fsapi_create_ex(FSAPIContext *ctx, FSAPIFileInfo *fi,
            const char *path, const FDIRClientOwnerModePair *omp)
    {
        const int flags = O_CREAT | O_TRUNC | O_WRONLY;
        return fsapi_open_ex(ctx, fi, path, flags, omp);
    }

    int fsapi_close(FSAPIFileInfo *fi);

    int fsapi_pwrite(FSAPIFileInfo *fi, const char *buff,
            const int size, const int64_t offset, int *written_bytes);

    int fsapi_write(FSAPIFileInfo *fi, const char *buff,
            const int size, int *written_bytes);

    int fsapi_pread(FSAPIFileInfo *fi, char *buff, const int size,
            const int64_t offset, int *read_bytes);

    int fsapi_read(FSAPIFileInfo *fi, char *buff,
            const int size, int *read_bytes);

    int fsapi_ftruncate(FSAPIFileInfo *fi, const int64_t new_size);

    int fsapi_truncate_ex(FSAPIContext *ctx, const char *path,
            const int64_t new_size);

    int fsapi_unlink_ex(FSAPIContext *ctx, const char *path);

    int fsapi_lseek(FSAPIFileInfo *fi, const int64_t offset, const int whence);

    int fsapi_fstat(FSAPIFileInfo *fi, struct stat *buf);

    int fsapi_stat_ex(FSAPIContext *ctx, const char *path, struct stat *buf);

    int fsapi_flock_ex2(FSAPIFileInfo *fi, const int operation,
            const int64_t owner_id, const pid_t pid);

    static inline int fsapi_flock_ex(FSAPIFileInfo *fi, const int operation,
            const int64_t owner_id)
    {
        return fsapi_flock_ex2(fi, operation, owner_id, getpid());
    }

    static inline int fsapi_flock(FSAPIFileInfo *fi, const int operation)
    {
        return fsapi_flock_ex2(fi, operation, (long)pthread_self(), getpid());
    }

    int fsapi_getlk(FSAPIFileInfo *fi, struct flock *lock, int64_t *owner_id);

    int fsapi_setlk(FSAPIFileInfo *fi, const struct flock *lock,
        const int64_t owner_id);

    int fsapi_fallocate(FSAPIFileInfo *fi, const int mode,
            const int64_t offset, const int64_t len);

    int fsapi_rename_ex(FSAPIContext *ctx, const char *old_path,
            const char *new_path, const int flags);

    int fsapi_symlink_ex(FSAPIContext *ctx, const char *target,
            const char *path, const FDIRClientOwnerModePair *omp);

    int fsapi_readlink(FSAPIContext *ctx, const char *path,
            char *buff, const int size);

    int fsapi_link_ex(FSAPIContext *ctx, const char *old_path,
            const char *new_path, const FDIRClientOwnerModePair *omp);

    int fsapi_statvfs_ex(FSAPIContext *ctx, const char *path,
            struct statvfs *stbuf);

#ifdef __cplusplus
}
#endif

#endif
