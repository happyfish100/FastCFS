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


#ifndef _FCFS_PAPI_FILE_H
#define _FCFS_PAPI_FILE_H

#include "api_types.h"

#define fcfs_open(path, flags, ...) \
    fcfs_open_ex(&g_fcfs_papi_ctx, path, flags, ##__VA_ARGS__)

#define fcfs_openat(fd, path, flags, ...) \
    fcfs_openat_ex(&g_fcfs_papi_ctx, fd, path, flags, ##__VA_ARGS__)

#define fcfs_creat(path, mode) \
    fcfs_creat_ex(&g_fcfs_papi_ctx, path, mode)

#define fcfs_close(fd) \
    fcfs_close_ex(&g_fcfs_papi_ctx, fd)

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

#ifdef __cplusplus
extern "C" {
#endif

    int fcfs_open_ex(FCFSPosixAPIContext *ctx,
            const char *path, int flags, ...);

    int fcfs_openat_ex(FCFSPosixAPIContext *ctx, int fd,
            const char *path, int flags, ...);

    int fcfs_creat_ex(FCFSPosixAPIContext *ctx,
            const char *path, mode_t mode);

    int fcfs_close_ex(FCFSPosixAPIContext *ctx, int fd);

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

#ifdef __cplusplus
}
#endif

#endif