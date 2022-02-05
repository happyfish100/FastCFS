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


#ifndef _FCFS_PAPI_H
#define _FCFS_PAPI_H

#include "api_types.h"

#define fcfs_open(path, flags, ...) \
    fcfs_open_ex(&g_fcfs_papi_ctx, path, flags, ##__VA_ARGS__)

#define fcfs_openat(fd, path, flags, ...) \
    fcfs_openat_ex(&g_fcfs_papi_ctx, fd, path, flags, ##__VA_ARGS__)

#define fcfs_creat(path, mode) \
    fcfs_creat_ex(&g_fcfs_papi_ctx, path, mode)

#define fcfs_close(fd) \
    fcfs_close_ex(&g_fcfs_papi_ctx, fd)

#define fcfs_fsync(fd) \
    fcfs_fsync_ex(&g_fcfs_papi_ctx, fd)

#define fcfs_fdatasync(fd) \
    fcfs_fdatasync_ex(&g_fcfs_papi_ctx, fd)

#define fcfs_write(fd, buff, count) \
    fcfs_write_ex(&g_fcfs_papi_ctx, fd, buff, count)

#define fcfs_pwrite(fd, buff, count, offset) \
    fcfs_pwrite_ex(&g_fcfs_papi_ctx, fd, buff, count, offset)

#define fcfs_writev(fd, iov, iovcnt) \
    fcfs_writev_ex(&g_fcfs_papi_ctx, fd, iov, iovcnt)

#define fcfs_pwritev(fd, iov, iovcnt, offset) \
    fcfs_pwritev_ex(&g_fcfs_papi_ctx, fd, iov, iovcnt, offset)

#define fcfs_read(fd, buff, count) \
    fcfs_read_ex(&g_fcfs_papi_ctx, fd, buff, count)

#define fcfs_pread(fd, buff, count, offset) \
    fcfs_pread_ex(&g_fcfs_papi_ctx, fd, buff, count, offset)

#define fcfs_readv(fd, iov, iovcnt) \
    fcfs_readv_ex(&g_fcfs_papi_ctx, fd, iov, iovcnt)

#define fcfs_preadv(fd, iov, iovcnt, offset) \
    fcfs_preadv_ex(&g_fcfs_papi_ctx, fd, iov, iovcnt, offset)

#define fcfs_lseek(fd, offset, whence) \
    fcfs_lseek_ex(&g_fcfs_papi_ctx, fd, offset, whence)

#define fcfs_fallocate(fd, mode, offset, length) \
    fcfs_fallocate_ex(&g_fcfs_papi_ctx, fd, mode, offset, length)

#define fcfs_ftruncate(fd, length) \
    fcfs_ftruncate_ex(&g_fcfs_papi_ctx, fd, length)

#define fcfs_fstat(fd, buf) \
    fcfs_fstat_ex(&g_fcfs_papi_ctx, fd, buf)

#define fcfs_fstatat(fd, path, buf, flags) \
    fcfs_fstatat_ex(&g_fcfs_papi_ctx, fd, path, buf, flags)

#define fcfs_readlinkat(fd, path, buff, size) \
    fcfs_readlinkat_ex(&g_fcfs_papi_ctx, fd, path, buff, size)

#ifdef __cplusplus
extern "C" {
#endif

    int fcfs_open_ex(FCFSPosixAPIContext *ctx,
            const char *path, int flags, ...);

    int fcfs_openat_ex(FCFSPosixAPIContext *ctx, int fd,
            const char *path, int flags, ...);

    int fcfs_creat_ex(FCFSPosixAPIContext *ctx,
            const char *path, mode_t mode);

    int fcfs_dup_ex(FCFSPosixAPIContext *ctx, int fd);

    int fcfs_dup2_ex(FCFSPosixAPIContext *ctx, int fd1, int fd2);

    int fcfs_close_ex(FCFSPosixAPIContext *ctx, int fd);

    int fcfs_fsync_ex(FCFSPosixAPIContext *ctx, int fd);

    int fcfs_fdatasync_ex(FCFSPosixAPIContext *ctx, int fd);

    ssize_t fcfs_write_ex(FCFSPosixAPIContext *ctx,
            int fd, const void *buff, size_t count);

    ssize_t fcfs_pwrite_ex(FCFSPosixAPIContext *ctx, int fd,
            const void *buff, size_t count, off_t offset);

    ssize_t fcfs_writev_ex(FCFSPosixAPIContext *ctx, int fd,
            const struct iovec *iov, int iovcnt);

    ssize_t fcfs_pwritev_ex(FCFSPosixAPIContext *ctx, int fd,
            const struct iovec *iov, int iovcnt, off_t offset);

    ssize_t fcfs_read_ex(FCFSPosixAPIContext *ctx,
            int fd, void *buff, size_t count);

    ssize_t fcfs_pread_ex(FCFSPosixAPIContext *ctx, int fd,
            void *buff, size_t count, off_t offset);

    ssize_t fcfs_readv_ex(FCFSPosixAPIContext *ctx, int fd,
            const struct iovec *iov, int iovcnt);

    ssize_t fcfs_preadv_ex(FCFSPosixAPIContext *ctx, int fd,
            const struct iovec *iov, int iovcnt, off_t offset);

    off_t fcfs_lseek_ex(FCFSPosixAPIContext *ctx,
            int fd, off_t offset, int whence);

    int fcfs_fallocate_ex(FCFSPosixAPIContext *ctx, int fd,
            int mode, off_t offset, off_t length);

    int fcfs_ftruncate_ex(FCFSPosixAPIContext *ctx, int fd, off_t length);

    int fcfs_fstat_ex(FCFSPosixAPIContext *ctx, int fd, struct stat *buf);

    int fcfs_fstatat_ex(FCFSPosixAPIContext *ctx, int fd,
            const char *path, struct stat *buf, int flags);

    int fcfs_flock_ex(FCFSPosixAPIContext *ctx, int fd, int operation);

    int fcfs_fcntl_ex(FCFSPosixAPIContext *ctx, int fd, int cmd, ...);

    int fcfs_symlinkat_ex(FCFSPosixAPIContext *ctx, const char *link,
            int fd, const char *path);

    int fcfs_linkat_ex(FCFSPosixAPIContext *ctx, int fd1, const char *path1,
            int fd2, const char *path2, int flags);

    ssize_t fcfs_readlinkat_ex(FCFSPosixAPIContext *ctx, int fd,
            const char *path, char *buff, size_t size);

    //TODO
    //
    int fcfs_mknodat_ex(FCFSPosixAPIContext *ctx, int fd,
            const char *path, mode_t mode, dev_t dev);

    int fcfs_faccessat_ex(FCFSPosixAPIContext *ctx, int fd,
            const char *path, int mode, int flags);

    int fcfs_futimes_ex(FCFSPosixAPIContext *ctx,
            int fd, const struct timeval times[2]);

    int fcfs_futimesat_ex(FCFSPosixAPIContext *ctx, int fd,
            const char *path, const struct timeval times[2]);

    int fcfs_futimens_ex(FCFSPosixAPIContext *ctx, int fd,
            const struct timespec times[2]);

    int fcfs_utimensat_ex(FCFSPosixAPIContext *ctx, int fd,
            const char *path, const struct timespec times[2], int flag);

    int fcfs_unlinkat_ex(FCFSPosixAPIContext *ctx, int fd,
            const char *path, int flag);

    int fcfs_renameat_ex(FCFSPosixAPIContext *ctx, int fd1,
            const char *path1, int fd2, const char *path2);

    int fcfs_renameat2_ex(FCFSPosixAPIContext *ctx, int fd1,
            const char *path1, int fd2, const char *path2,
            unsigned int flags);

    int fcfs_mkdirat_ex(FCFSPosixAPIContext *ctx, int fd,
            const char *path, mode_t mode);

    int fcfs_fchown_ex(FCFSPosixAPIContext *ctx, int fd,
            uid_t owner, gid_t group);

    int fcfs_fchownat_ex(FCFSPosixAPIContext *ctx, int fd,
            const char *path, uid_t owner, gid_t group, int flag);

    int fcfs_fchmod_ex(FCFSPosixAPIContext *ctx,
            int fd, mode_t mode);

    int fcfs_fchmodat_ex(FCFSPosixAPIContext *ctx, int fd,
            const char *path, mode_t mode, int flag);

    int fcfs_fchdir_ex(FCFSPosixAPIContext *ctx, int fd);

    int fcfs_fstatvfs_ex(FCFSPosixAPIContext *ctx, int fd,
            struct statvfs *buf);

#ifdef __cplusplus
}
#endif

#endif
