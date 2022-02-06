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

#include <stdarg.h>
#include <sys/uio.h>
#include "fastcommon/shared_func.h"
#include "fastcommon/logger.h"
#include "posix_api.h"
#include "fd_manager.h"
#include "papi.h"

static int do_open(FCFSPosixAPIContext *ctx, const char *path, int flags, ...)
{
    FCFSPosixAPIFileInfo *file;
    FCFSAPIFileContext fctx;
    va_list ap;
    mode_t mode;
    int result;

    if ((file=fcfs_fd_manager_alloc(path)) == NULL) {
        errno = ENOMEM;
        return -1;
    }

    va_start(ap, flags);
    mode = va_arg(ap, mode_t);
    va_end(ap);

    fcfs_posix_api_set_fctx(&fctx, ctx, mode);
    if ((result=fcfs_api_open_ex(&ctx->api_ctx, &file->fi,
                    path, flags, &fctx)) != 0)
    {
        fcfs_fd_manager_free(file);
        errno = result;
        return -1;
    }

    if (S_ISDIR(file->fi.dentry.stat.mode)) {
        fcfs_fd_manager_normalize_path(file);
    }

    return file->fd;
}

static inline int papi_resolve_path(FCFSPosixAPIContext *ctx,
        const char *func, const char **path,
        char *full_filename, const int size)
{
    if (**path != '/') {
        normalize_path(NULL, *path, full_filename,
                sizeof(full_filename));
        *path = full_filename;
    }

    FCFS_API_CHECK_PATH_MOUNTPOINT(ctx, *path, func);
    *path += ctx->mountpoint.len;
    return 0;
}

static int papi_resolve_pathat(FCFSPosixAPIContext *ctx, const char *func,
        int fd, const char **path, char *full_filename, const int size)
{
    FCFSPosixAPIFileInfo *file;

    if (fd == AT_FDCWD) {
        normalize_path(NULL, *path, full_filename, size);
        FCFS_API_CHECK_PATH_MOUNTPOINT(ctx, full_filename, func);
        *path = full_filename + ctx->mountpoint.len;
        return 0;
    }

    if (**path == '/') {
        FCFS_API_CHECK_PATH_MOUNTPOINT(ctx, *path, func);
        *path += ctx->mountpoint.len;
        return 0;
    }

    if ((file=fcfs_fd_manager_get(fd)) == NULL) {
        errno = EBADF;
        return -1;
    }

    normalize_path(file->filename.str, *path, full_filename, size);
    *path = full_filename;
    return 0;
}

int fcfs_open_ex(FCFSPosixAPIContext *ctx, const char *path, int flags, ...)
{
    char full_fname[PATH_MAX];
    va_list ap;
    int fd;
    int result;

    if ((result=papi_resolve_path(ctx, "open", &path,
                    full_fname, sizeof(full_fname))) != 0)
    {
        return result;
    }

    va_start(ap, flags);
    fd = do_open(ctx, path, flags, ap);
    va_end(ap);
    return fd;
}

int fcfs_openat_ex(FCFSPosixAPIContext *ctx, int fd,
        const char *path, int flags, ...)
{
    va_list ap;
    char full_fname[PATH_MAX];
    int new_fd;
    int result;

    if ((result=papi_resolve_pathat(ctx, "openat", fd, &path,
                    full_fname, sizeof(full_fname))) != 0)
    {
        return result;
    }

    va_start(ap, flags);
    new_fd = do_open(ctx, path, flags, ap);
    va_end(ap);
    return new_fd;
}

int fcfs_creat_ex(FCFSPosixAPIContext *ctx, const char *path, mode_t mode)
{
    return fcfs_open_ex(ctx, path, O_CREAT | O_TRUNC | O_WRONLY, mode);
}

int fcfs_close_ex(FCFSPosixAPIContext *ctx, int fd)
{
    FCFSPosixAPIFileInfo *file;

    if ((file=fcfs_fd_manager_get(fd)) == NULL) {
        errno = EBADF;
        return -1;
    }

    fcfs_api_close(&file->fi);
    fcfs_fd_manager_free(file);
    return 0;
}

int fcfs_fsync_ex(FCFSPosixAPIContext *ctx, int fd)
{
    FCFSPosixAPIFileInfo *file;

    if ((file=fcfs_fd_manager_get(fd)) == NULL) {
        errno = EBADF;
        return -1;
    }

    //TODO
    return 0;
}

int fcfs_fdatasync_ex(FCFSPosixAPIContext *ctx, int fd)
{
    FCFSPosixAPIFileInfo *file;

    if ((file=fcfs_fd_manager_get(fd)) == NULL) {
        errno = EBADF;
        return -1;
    }

    return 0;
}

ssize_t fcfs_write_ex(FCFSPosixAPIContext *ctx,
        int fd, const void *buff, size_t count)
{
    int result;
    int write_bytes;
    FCFSPosixAPIFileInfo *file;

    if ((file=fcfs_fd_manager_get(fd)) == NULL) {
        errno = EBADF;
        return -1;
    }

    if ((result=fcfs_api_write_ex(&file->fi, buff, count, &write_bytes,
                    fcfs_posix_api_gettid())) != 0)
    {
        errno = result;
        return -1;
    } else {
        return write_bytes;
    }
}

ssize_t fcfs_pwrite_ex(FCFSPosixAPIContext *ctx, int fd,
        const void *buff, size_t count, off_t offset)
{
    int result;
    int write_bytes;
    FCFSPosixAPIFileInfo *file;

    if ((file=fcfs_fd_manager_get(fd)) == NULL) {
        errno = EBADF;
        return -1;
    }

    if ((result=fcfs_api_pwrite_ex(&file->fi, buff, count, offset,
                    &write_bytes, fcfs_posix_api_gettid())) != 0)
    {
        errno = result;
        return -1;
    } else {
        return write_bytes;
    }
}

ssize_t fcfs_writev_ex(FCFSPosixAPIContext *ctx, int fd,
        const struct iovec *iov, int iovcnt)
{
    int result;
    int write_bytes;
    FCFSPosixAPIFileInfo *file;

    if ((file=fcfs_fd_manager_get(fd)) == NULL) {
        errno = EBADF;
        return -1;
    }

    if ((result=fcfs_api_writev_ex(&file->fi, iov, iovcnt,
                    &write_bytes, fcfs_posix_api_gettid())) != 0)
    {
        errno = result;
        return -1;
    } else {
        return write_bytes;
    }
}

ssize_t fcfs_pwritev_ex(FCFSPosixAPIContext *ctx, int fd,
        const struct iovec *iov, int iovcnt, off_t offset)
{
    int result;
    int write_bytes;
    FCFSPosixAPIFileInfo *file;

    if ((file=fcfs_fd_manager_get(fd)) == NULL) {
        errno = EBADF;
        return -1;
    }

    if ((result=fcfs_api_pwritev_ex(&file->fi, iov, iovcnt, offset,
                    &write_bytes, fcfs_posix_api_gettid())) != 0)
    {
        errno = result;
        return -1;
    } else {
        return write_bytes;
    }
}

ssize_t fcfs_read_ex(FCFSPosixAPIContext *ctx,
        int fd, void *buff, size_t count)
{
    int result;
    int read_bytes;
    FCFSPosixAPIFileInfo *file;

    if ((file=fcfs_fd_manager_get(fd)) == NULL) {
        errno = EBADF;
        return -1;
    }

    if ((result=fcfs_api_read_ex(&file->fi, buff, count, &read_bytes,
                    fcfs_posix_api_gettid())) != 0)
    {
        errno = result;
        return -1;
    } else {
        return read_bytes;
    }
}

ssize_t fcfs_pread_ex(FCFSPosixAPIContext *ctx, int fd,
        void *buff, size_t count, off_t offset)
{
    int result;
    int read_bytes;
    FCFSPosixAPIFileInfo *file;

    if ((file=fcfs_fd_manager_get(fd)) == NULL) {
        errno = EBADF;
        return -1;
    }

    if ((result=fcfs_api_pread_ex(&file->fi, buff, count, offset,
                    &read_bytes, fcfs_posix_api_gettid())) != 0)
    {
        errno = result;
        return -1;
    } else {
        return read_bytes;
    }
}

ssize_t fcfs_readv_ex(FCFSPosixAPIContext *ctx, int fd,
        const struct iovec *iov, int iovcnt)
{
    int result;
    int read_bytes;
    FCFSPosixAPIFileInfo *file;

    if ((file=fcfs_fd_manager_get(fd)) == NULL) {
        errno = EBADF;
        return -1;
    }

    if ((result=fcfs_api_readv_ex(&file->fi, iov, iovcnt,
                    &read_bytes, fcfs_posix_api_gettid())) != 0)
    {
        errno = result;
        return -1;
    } else {
        return read_bytes;
    }
}

ssize_t fcfs_preadv_ex(FCFSPosixAPIContext *ctx, int fd,
        const struct iovec *iov, int iovcnt, off_t offset)
{
    int result;
    int read_bytes;
    FCFSPosixAPIFileInfo *file;

    if ((file=fcfs_fd_manager_get(fd)) == NULL) {
        errno = EBADF;
        return -1;
    }

    if ((result=fcfs_api_preadv_ex(&file->fi, iov, iovcnt, offset,
                    &read_bytes, fcfs_posix_api_gettid())) != 0)
    {
        errno = result;
        return -1;
    } else {
        return read_bytes;
    }
}

off_t fcfs_lseek_ex(FCFSPosixAPIContext *ctx,
        int fd, off_t offset, int whence)
{
    int result;
    FCFSPosixAPIFileInfo *file;

    if ((file=fcfs_fd_manager_get(fd)) == NULL) {
        errno = EBADF;
        return -1;
    }

    if ((result=fcfs_api_lseek(&file->fi, offset, whence)) != 0) {
        errno = result;
        return -1;
    } else {
        return file->fi.offset;
    }
}

int fcfs_fallocate_ex(FCFSPosixAPIContext *ctx, int fd,
        int mode, off_t offset, off_t length)
{
    int result;
    FCFSPosixAPIFileInfo *file;

    if ((file=fcfs_fd_manager_get(fd)) == NULL) {
        errno = EBADF;
        return -1;
    }

    if ((result=fcfs_api_fallocate_ex(&file->fi, mode, offset,
                    length, fcfs_posix_api_gettid())) != 0)
    {
        errno = result;
        return -1;
    } else {
        return 0;
    }
}

int fcfs_truncate_ex(FCFSPosixAPIContext *ctx,
        const char *path, off_t length)
{
    FCFSAPIFileContext fctx;
    char full_fname[PATH_MAX];
    int result;

    if ((result=papi_resolve_path(ctx, "truncate", &path,
                    full_fname, sizeof(full_fname))) != 0)
    {
        return result;
    }

    fcfs_posix_api_set_fctx(&fctx, ctx, 0777);
    if ((result=fcfs_api_truncate_ex(&ctx->api_ctx,
                    path, length, &fctx)) != 0)
    {
        errno = result;
        return -1;
    } else {
        return 0;
    }
}

int fcfs_ftruncate_ex(FCFSPosixAPIContext *ctx, int fd, off_t length)
{
    int result;
    FCFSPosixAPIFileInfo *file;

    if ((file=fcfs_fd_manager_get(fd)) == NULL) {
        errno = EBADF;
        return -1;
    }

    if ((result=fcfs_api_ftruncate_ex(&file->fi, length,
                    fcfs_posix_api_gettid())) != 0)
    {
        errno = result;
        return -1;
    } else {
        return 0;
    }
}

int fcfs_lstat_ex(FCFSPosixAPIContext *ctx,
        const char *path, struct stat *buf)
{
    char full_fname[PATH_MAX];
    int result;

    if ((result=papi_resolve_path(ctx, "lstat", &path,
                    full_fname, sizeof(full_fname))) != 0)
    {
        return result;
    }

    if ((result=fcfs_api_lstat_ex(&ctx->api_ctx, path, buf)) != 0) {
        errno = result;
        return -1;
    } else {
        return 0;
    }
}

int fcfs_stat_ex(FCFSPosixAPIContext *ctx,
        const char *path, struct stat *buf)
{
    const int flags = FDIR_FLAGS_FOLLOW_SYMLINK;
    char full_fname[PATH_MAX];
    int result;

    if ((result=papi_resolve_path(ctx, "stat", &path,
                    full_fname, sizeof(full_fname))) != 0)
    {
        return result;
    }

    if ((result=fcfs_api_stat_ex(&ctx->api_ctx, path, buf, flags)) != 0) {
        errno = result;
        return -1;
    } else {
        return 0;
    }
}

int fcfs_fstat_ex(FCFSPosixAPIContext *ctx, int fd, struct stat *buf)
{
    int result;
    FCFSPosixAPIFileInfo *file;

    if ((file=fcfs_fd_manager_get(fd)) == NULL) {
        errno = EBADF;
        return -1;
    }

    if ((result=fcfs_api_fstat(&file->fi, buf)) != 0) {
        errno = result;
        return -1;
    } else {
        return 0;
    }
}

int fcfs_fstatat_ex(FCFSPosixAPIContext *ctx, int fd,
        const char *path, struct stat *buf, int flags)
{
    char full_fname[PATH_MAX];
    int result;

    if ((result=papi_resolve_pathat(ctx, "fstatat", fd, &path,
                    full_fname, sizeof(full_fname))) != 0)
    {
        return result;
    }

    if ((result=fcfs_api_stat_ex(&ctx->api_ctx, path, buf,
                    ((flags & AT_SYMLINK_NOFOLLOW) != 0 ? 0 :
                     FDIR_FLAGS_FOLLOW_SYMLINK))) != 0)
    {
        errno = result;
        return -1;
    } else {
        return 0;
    }
}

int fcfs_flock_ex(FCFSPosixAPIContext *ctx, int fd, int operation)
{
    FCFSPosixAPIFileInfo *file;
    int result;

    if ((file=fcfs_fd_manager_get(fd)) == NULL) {
        errno = EBADF;
        return -1;
    }

    if ((result=fcfs_api_flock(&file->fi, operation)) != 0) {
        errno = result;
        return -1;
    } else {
        return 0;
    }
}

int fcfs_symlink_ex(FCFSPosixAPIContext *ctx,
        const char *link, const char *path)
{
    char full_fname[PATH_MAX];
    FDIRClientOwnerModePair omp;
    int result;

    if (*link == '/') {
        FCFS_API_CHECK_PATH_MOUNTPOINT(ctx, link, "symlink");
        link += ctx->mountpoint.len;
    }

    if ((result=papi_resolve_path(ctx, "symlink", &path,
                    full_fname, sizeof(full_fname))) != 0)
    {
        return result;
    }

    fcfs_posix_api_set_omp(&omp, ctx, 0777);
    if ((result=fcfs_api_symlink_ex(&ctx->api_ctx,
                    link, path, &omp)) != 0)
    {
        errno = result;
        return -1;
    } else {
        return 0;
    }
}

int fcfs_symlinkat_ex(FCFSPosixAPIContext *ctx, const char *link,
        int fd, const char *path)
{
    char full_fname[PATH_MAX];
    FDIRClientOwnerModePair omp;
    int result;

    if (*link == '/') {
        FCFS_API_CHECK_PATH_MOUNTPOINT(ctx, link, "symlinkat");
        link += ctx->mountpoint.len;
    }

    if ((result=papi_resolve_pathat(ctx, "symlinkat", fd, &path,
                    full_fname, sizeof(full_fname))) != 0)
    {
        return result;
    }

    fcfs_posix_api_set_omp(&omp, ctx, 0777);
    if ((result=fcfs_api_symlink_ex(&ctx->api_ctx,
                    link, path, &omp)) != 0)
    {
        errno = result;
        return -1;
    } else {
        return 0;
    }
}

int fcfs_link_ex(FCFSPosixAPIContext *ctx,
        const char *path1, const char *path2)
{
    const int flags = FDIR_FLAGS_FOLLOW_SYMLINK;
    char full_fname1[PATH_MAX];
    char full_fname2[PATH_MAX];
    FDIRClientOwnerModePair omp;
    int result;

    if ((result=papi_resolve_path(ctx, "linkat", &path1,
                    full_fname1, sizeof(full_fname1))) != 0)
    {
        return result;
    }
    if ((result=papi_resolve_path(ctx, "linkat", &path2,
                    full_fname2, sizeof(full_fname2))) != 0)
    {
        return result;
    }

    fcfs_posix_api_set_omp(&omp, ctx, 0777);
    if ((result=fcfs_api_link_ex(&ctx->api_ctx, path1,
                    path2, &omp, flags)) != 0)
    {
        errno = result;
        return -1;
    } else {
        return 0;
    }
}

int fcfs_linkat_ex(FCFSPosixAPIContext *ctx, int fd1,
        const char *path1, int fd2, const char *path2, int flags)
{
    char full_fname1[PATH_MAX];
    char full_fname2[PATH_MAX];
    FDIRClientOwnerModePair omp;
    int result;

    if ((result=papi_resolve_pathat(ctx, "linkat", fd1, &path1,
                    full_fname1, sizeof(full_fname1))) != 0)
    {
        return result;
    }

    if ((result=papi_resolve_pathat(ctx, "linkat", fd2, &path2,
                    full_fname2, sizeof(full_fname2))) != 0)
    {
        return result;
    }

    fcfs_posix_api_set_omp(&omp, ctx, 0777);
    if ((result=fcfs_api_link_ex(&ctx->api_ctx, path1, path2,
                    &omp, ((flags & AT_SYMLINK_FOLLOW) != 0) ?
                    FDIR_FLAGS_FOLLOW_SYMLINK : 0)) != 0)
    {
        errno = result;
        return -1;
    } else {
        return 0;
    }
}

ssize_t fcfs_readlink_ex(FCFSPosixAPIContext *ctx,
        const char *path, char *buff, size_t size)
{
    char full_fname[PATH_MAX];
    int result;

    if ((result=papi_resolve_path(ctx, "readlink", &path,
                    full_fname, sizeof(full_fname))) != 0)
    {
        return result;
    }

    if ((result=fcfs_api_readlink(&ctx->api_ctx, path, buff, size)) != 0) {
        errno = result;
        return -1;
    } else {
        return 0;
    }
}

ssize_t fcfs_readlinkat_ex(FCFSPosixAPIContext *ctx, int fd,
        const char *path, char *buff, size_t size)
{
    char full_fname[PATH_MAX];
    int result;

    if ((result=papi_resolve_pathat(ctx, "readlinkat", fd, &path,
                    full_fname, sizeof(full_fname))) != 0)
    {
        return result;
    }

    if ((result=fcfs_api_readlink(&ctx->api_ctx, path, buff, size)) != 0) {
        errno = result;
        return -1;
    } else {
        return 0;
    }
}

int fcfs_mknod_ex(FCFSPosixAPIContext *ctx,
        const char *path, mode_t mode, dev_t dev)
{
    FDIRClientOwnerModePair omp;
    char full_fname[PATH_MAX];
    int result;

    if ((result=papi_resolve_path(ctx, "mknod", &path,
                    full_fname, sizeof(full_fname))) != 0)
    {
        return result;
    }

    fcfs_posix_api_set_omp(&omp, ctx, mode);
    if ((result=fcfs_api_mknod_ex(&ctx->api_ctx,
                    path, &omp, dev)) != 0)
    {
        errno = result;
        return -1;
    } else {
        return 0;
    }
}

int fcfs_mknodat_ex(FCFSPosixAPIContext *ctx, int fd,
        const char *path, mode_t mode, dev_t dev)
{
    FDIRClientOwnerModePair omp;
    char full_fname[PATH_MAX];
    int result;

    if ((result=papi_resolve_pathat(ctx, "mknodat", fd, &path,
                    full_fname, sizeof(full_fname))) != 0)
    {
        return result;
    }

    fcfs_posix_api_set_omp(&omp, ctx, mode);
    if ((result=fcfs_api_mknod_ex(&ctx->api_ctx,
                    path, &omp, dev)) != 0)
    {
        errno = result;
        return -1;
    } else {
        return 0;
    }
}

int fcfs_access_ex(FCFSPosixAPIContext *ctx,
        const char *path, int mode)
{
    const int flags = FDIR_FLAGS_FOLLOW_SYMLINK;
    FDIRClientOwnerModePair omp;
    char full_fname[PATH_MAX];
    int result;

    if ((result=papi_resolve_path(ctx, "access", &path,
                    full_fname, sizeof(full_fname))) != 0)
    {
        return result;
    }

    fcfs_posix_api_set_omp(&omp, ctx, 0777);
    if ((result=fcfs_api_access_ex(&ctx->api_ctx,
                    path, mode, &omp, flags)) != 0)
    {
        errno = result;
        return -1;
    } else {
        return 0;
    }
}

int fcfs_faccessat_ex(FCFSPosixAPIContext *ctx, int fd,
            const char *path, int mode, int flags)
{
    FDIRClientOwnerModePair omp;
    char full_fname[PATH_MAX];
    int result;

    if ((result=papi_resolve_pathat(ctx, "faccessat", fd, &path,
                    full_fname, sizeof(full_fname))) != 0)
    {
        return result;
    }

    fcfs_posix_api_set_omp(&omp, ctx, 0777);
    if ((result=fcfs_api_access_ex(&ctx->api_ctx, path, mode, &omp,
                    (flags & AT_SYMLINK_NOFOLLOW) ? 0 :
                    FDIR_FLAGS_FOLLOW_SYMLINK)) != 0)
    {
        errno = result;
        return -1;
    } else {
        return 0;
    }
}

int fcfs_utime_ex(FCFSPosixAPIContext *ctx, const char *path,
        const struct utimbuf *times)
{
    char full_fname[PATH_MAX];
    int result;

    if ((result=papi_resolve_path(ctx, "utime", &path,
                    full_fname, sizeof(full_fname))) != 0)
    {
        return result;
    }

    if ((result=fcfs_api_utime_ex(&ctx->api_ctx, path, times)) != 0) {
        errno = result;
        return -1;
    } else {
        return 0;
    }
}

int fcfs_utimes_ex(FCFSPosixAPIContext *ctx, const char *path,
        const struct timeval times[2])
{
    const int flags = FDIR_FLAGS_FOLLOW_SYMLINK;
    char full_fname[PATH_MAX];
    int result;

    if ((result=papi_resolve_path(ctx, "utimes", &path,
                    full_fname, sizeof(full_fname))) != 0)
    {
        return result;
    }

    if ((result=fcfs_api_utimes_by_path_ex(&ctx->api_ctx,
                    path, times, flags)) != 0)
    {
        errno = result;
        return -1;
    } else {
        return 0;
    }
}

int fcfs_futimes_ex(FCFSPosixAPIContext *ctx,
        int fd, const struct timeval times[2])
{
    const int flags = FDIR_FLAGS_FOLLOW_SYMLINK;
    FCFSPosixAPIFileInfo *file;
    int result;

    if ((file=fcfs_fd_manager_get(fd)) == NULL) {
        errno = EBADF;
        return -1;
    }

    if ((result=fcfs_api_utimes_by_inode_ex(&ctx->api_ctx, file->
                    fi.dentry.inode, times, flags)) != 0)
    {
        errno = result;
        return -1;
    } else {
        return 0;
    }
}

int fcfs_futimesat_ex(FCFSPosixAPIContext *ctx, int fd,
        const char *path, const struct timeval times[2])
{
    const int flags = FDIR_FLAGS_FOLLOW_SYMLINK;
    char full_fname[PATH_MAX];
    int result;

    if ((result=papi_resolve_pathat(ctx, "futimesat", fd, &path,
                    full_fname, sizeof(full_fname))) != 0)
    {
        return result;
    }

    if ((result=fcfs_api_utimes_by_path_ex(&ctx->api_ctx,
                    path, times, flags)) != 0)
    {
        errno = result;
        return -1;
    } else {
        return 0;
    }
}

int fcfs_futimens_ex(FCFSPosixAPIContext *ctx, int fd,
        const struct timespec times[2])
{
    const int flags = FDIR_FLAGS_FOLLOW_SYMLINK;
    FCFSPosixAPIFileInfo *file;
    int result;

    if ((file=fcfs_fd_manager_get(fd)) == NULL) {
        errno = EBADF;
        return -1;
    }

    if ((result=fcfs_api_utimens_by_inode_ex(&ctx->api_ctx, file->
                    fi.dentry.inode, times, flags)) != 0)
    {
        errno = result;
        return -1;
    } else {
        return 0;
    }
}

int fcfs_utimensat_ex(FCFSPosixAPIContext *ctx, int fd,
        const char *path, const struct timespec times[2], int flags)
{
    char full_fname[PATH_MAX];
    int result;

    if ((result=papi_resolve_pathat(ctx, "futimensat", fd, &path,
                    full_fname, sizeof(full_fname))) != 0)
    {
        return result;
    }

    if ((result=fcfs_api_utimens_by_path_ex(&ctx->api_ctx, path,
                    times, (flags & AT_SYMLINK_NOFOLLOW) ?
                    0 : FDIR_FLAGS_FOLLOW_SYMLINK)) != 0)
    {
        errno = result;
        return -1;
    } else {
        return 0;
    }
}

int fcfs_unlink_ex(FCFSPosixAPIContext *ctx, const char *path)
{
    char full_fname[PATH_MAX];
    int result;

    if ((result=papi_resolve_path(ctx, "unlink", &path,
                    full_fname, sizeof(full_fname))) != 0)
    {
        return result;
    }

    if ((result=fcfs_api_unlink_ex(&ctx->api_ctx, path,
                    fcfs_posix_api_gettid())) != 0)
    {
        errno = result;
        return -1;
    } else {
        return 0;
    }
}

int fcfs_unlinkat_ex(FCFSPosixAPIContext *ctx, int fd,
        const char *path, int flags)
{
    char full_fname[PATH_MAX];
    int result;

    if ((result=papi_resolve_pathat(ctx, "unlinkat", fd, &path,
                    full_fname, sizeof(full_fname))) != 0)
    {
        return result;
    }

    if ((result=fcfs_api_remove_dentry_ex(&ctx->api_ctx, path,
                    ((flags & AT_REMOVEDIR) ? FDIR_UNLINK_FLAGS_MATCH_DIR :
                     FDIR_UNLINK_FLAGS_MATCH_FILE),
                    fcfs_posix_api_gettid())) != 0)
    {
        errno = result;
        return -1;
    } else {
        return 0;
    }
}

int fcfs_rename_ex(FCFSPosixAPIContext *ctx,
        const char *path1, const char *path2)
{
    const int flags = 0;
    char full_fname1[PATH_MAX];
    char full_fname2[PATH_MAX];
    int result;

    if ((result=papi_resolve_path(ctx, "rename", &path1,
                    full_fname1, sizeof(full_fname1))) != 0)
    {
        return result;
    }

    if ((result=papi_resolve_path(ctx, "rename", &path2,
                    full_fname2, sizeof(full_fname2))) != 0)
    {
        return result;
    }

    if ((result=fcfs_api_rename_dentry_ex(&ctx->api_ctx, path1,
                    path2, flags, fcfs_posix_api_gettid())) != 0)
    {
        errno = result;
        return -1;
    } else {
        return 0;
    }
}

static inline int do_renameat(FCFSPosixAPIContext *ctx,
        const char *func_name, int fd1, const char *path1,
        int fd2, const char *path2, const int flags)
{
    char full_fname1[PATH_MAX];
    char full_fname2[PATH_MAX];
    int result;

    if ((result=papi_resolve_pathat(ctx, func_name, fd1, &path1,
                    full_fname1, sizeof(full_fname1))) != 0)
    {
        return result;
    }

    if ((result=papi_resolve_pathat(ctx, func_name, fd2, &path2,
                    full_fname2, sizeof(full_fname2))) != 0)
    {
        return result;
    }

    if ((result=fcfs_api_rename_dentry_ex(&ctx->api_ctx, path1,
                    path2, flags, fcfs_posix_api_gettid())) != 0)
    {
        errno = result;
        return -1;
    } else {
        return 0;
    }
}

int fcfs_renameat_ex(FCFSPosixAPIContext *ctx, int fd1,
        const char *path1, int fd2, const char *path2)
{
    const int flags = 0;
    return do_renameat(ctx, "renameat", fd1, path1, fd2, path2, flags);
}

int fcfs_renameat2_ex(FCFSPosixAPIContext *ctx, int fd1,
        const char *path1, int fd2, const char *path2,
        unsigned int flags)
{

    /* flags convert from FreeBSD to Linux */
#if defined(RENAME_SWAP) && defined(RENAME_EXCL)
    if ((flags & RENAME_SWAP)) {
        flags = ((flags & (~RENAME_SWAP)) | RENAME_EXCHANGE);
    } else if ((flags & RENAME_EXCL)) {
        flags = ((flags & (~RENAME_EXCL)) | RENAME_NOREPLACE);
    }
#endif

    return do_renameat(ctx, "renameat2", fd1, path1, fd2, path2, flags);
}
