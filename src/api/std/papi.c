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
#include <sys/stat.h>
#include "fastcommon/shared_func.h"
#include "fastcommon/logger.h"
#include "posix_api.h"
#include "papi.h"

#define FCFS_PAPI_MAGIC_NUMBER    1644551636

static FCFSPosixAPIFileInfo *do_open_ex(FCFSPosixAPIContext *ctx,
        const char *path, const int flags, const int mode,
        const FCFSPosixAPITPIDType tpid_type)
{
    FCFSPosixAPIFileInfo *file;
    FCFSAPIFileContext fctx;
    int result;

    if ((file=fcfs_fd_manager_alloc(path)) == NULL) {
        errno = ENOMEM;
        return NULL;
    }

    fcfs_posix_api_set_fctx(&fctx, ctx, mode, tpid_type);
    if ((result=fcfs_api_open_ex(&ctx->api_ctx, &file->fi,
                    path, flags, &fctx)) != 0)
    {
        fcfs_fd_manager_free(file);
        errno = result;
        return NULL;
    }

    if (S_ISDIR(file->fi.dentry.stat.mode)) {
        fcfs_fd_manager_normalize_path(file);
    }

    return file;
}

static inline int do_open(FCFSPosixAPIContext *ctx, const char *path,
        const int flags, const int mode, const FCFSPosixAPITPIDType tpid_type)
{
    FCFSPosixAPIFileInfo *file;

    file = do_open_ex(ctx, path, flags, mode, tpid_type);
    if (file != NULL) {
        file->tpid_type = tpid_type;
        return file->fd;
    } else {
        return -1;
    }
}

static inline char *do_getcwd(FCFSPosixAPIContext *ctx,
        char *buf, size_t size, const int overflow_errno)
{
    if (G_FCFS_PAPI_CWD == NULL) {
        if (ctx->mountpoint.len >= size) {
            errno = overflow_errno;
            return NULL;
        }

        sprintf(buf, "%.*s", ctx->mountpoint.len, ctx->mountpoint.str);
        return buf;
    }

    if (ctx->mountpoint.len + G_FCFS_PAPI_CWD->len >= size) {
        errno = overflow_errno;
        return NULL;
    }

    sprintf(buf, "%.*s%.*s", ctx->mountpoint.len, ctx->mountpoint.str,
            G_FCFS_PAPI_CWD->len, G_FCFS_PAPI_CWD->str);
    return buf;
}

static inline int papi_resolve_path(FCFSPosixAPIContext *ctx,
        const char *func, const char **path,
        char *full_filename, const int size)
{
    char cwd[PATH_MAX];

    if (**path != '/') {
        normalize_path(do_getcwd(ctx, cwd, sizeof(cwd), EOVERFLOW),
                *path, full_filename, sizeof(full_filename));
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
    char cwd[PATH_MAX];

    if (fd == AT_FDCWD) {
        normalize_path(do_getcwd(ctx, cwd, sizeof(cwd), EOVERFLOW),
                *path, full_filename, size);
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
    int mode;
    int result;

    if ((result=papi_resolve_path(ctx, "open", &path,
                    full_fname, sizeof(full_fname))) != 0)
    {
        return result;
    }

    va_start(ap, flags);
    mode = va_arg(ap, int);
    fd = do_open(ctx, path, flags, mode, fcfs_papi_tpid_type_tid);
    va_end(ap);
    return fd;
}

int fcfs_file_open(FCFSPosixAPIContext *ctx, const char *path,
        const int flags, const int mode, const
        FCFSPosixAPITPIDType tpid_type)
{
    char full_fname[PATH_MAX];
    int result;

    if ((result=papi_resolve_path(ctx, "open", &path,
                    full_fname, sizeof(full_fname))) != 0)
    {
        return result;
    }

    return do_open(ctx, path, flags, mode, tpid_type);
}

int fcfs_openat_ex(FCFSPosixAPIContext *ctx, int fd,
        const char *path, int flags, ...)
{
    char full_fname[PATH_MAX];
    va_list ap;
    int mode;
    int new_fd;
    int result;

    if ((result=papi_resolve_pathat(ctx, "openat", fd, &path,
                    full_fname, sizeof(full_fname))) != 0)
    {
        return result;
    }

    va_start(ap, flags);
    mode = va_arg(ap, int);
    new_fd = do_open(ctx, path, flags, mode, fcfs_papi_tpid_type_tid);
    va_end(ap);
    return new_fd;
}

int fcfs_creat_ex(FCFSPosixAPIContext *ctx, const char *path, mode_t mode)
{
    return fcfs_open_ex(ctx, path, O_CREAT | O_TRUNC | O_WRONLY, mode);
}

int fcfs_close(int fd)
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

int fcfs_fsync(int fd)
{
    FCFSPosixAPIFileInfo *file;

    if ((file=fcfs_fd_manager_get(fd)) == NULL) {
        errno = EBADF;
        return -1;
    }

    return fcfs_api_fsync(&file->fi,
            fcfs_posix_api_gettid(
                file->tpid_type));
}

int fcfs_fdatasync(int fd)
{
    FCFSPosixAPIFileInfo *file;

    if ((file=fcfs_fd_manager_get(fd)) == NULL) {
        errno = EBADF;
        return -1;
    }

    return fcfs_api_fdatasync(&file->fi,
            fcfs_posix_api_gettid(
                file->tpid_type));
}

static inline ssize_t do_write(FCFSPosixAPIFileInfo *file,
        const void *buff, size_t count)
{
    int result;
    int write_bytes;

    if ((result=fcfs_api_write_ex(&file->fi, buff, count, &write_bytes,
                    fcfs_posix_api_gettid(file->tpid_type))) != 0)
    {
        errno = result;
        return -1;
    } else {
        return write_bytes;
    }
}

ssize_t fcfs_write(int fd, const void *buff, size_t count)
{
    FCFSPosixAPIFileInfo *file;

    if ((file=fcfs_fd_manager_get(fd)) == NULL) {
        errno = EBADF;
        return -1;
    }

    return do_write(file, buff, count);
}

ssize_t fcfs_file_write(int fd, const void *buff, size_t size, size_t n)
{
    FCFSPosixAPIFileInfo *file;
    size_t expect;
    size_t bytes;
    size_t count;
    size_t remain;

    if ((file=fcfs_fd_manager_get(fd)) == NULL) {
        errno = EBADF;
        return -1;
    }

    expect = size * n;
    bytes = do_write(file, buff, expect);
    if (bytes == expect) {
        count = n;
    } else if (bytes > 0) {
        count = bytes / size;
        remain = bytes - (size * count);
        fcfs_api_lseek(&file->fi, -1 * remain, SEEK_CUR);
    } else {
        count = 0;
    }

    return count;
}

ssize_t fcfs_pwrite(int fd, const void *buff, size_t count, off_t offset)
{
    int result;
    int write_bytes;
    FCFSPosixAPIFileInfo *file;

    if ((file=fcfs_fd_manager_get(fd)) == NULL) {
        errno = EBADF;
        return -1;
    }

    if ((result=fcfs_api_pwrite_ex(&file->fi, buff, count, offset,
                    &write_bytes, fcfs_posix_api_gettid(
                        file->tpid_type))) != 0)
    {
        errno = result;
        return -1;
    } else {
        return write_bytes;
    }
}

ssize_t fcfs_writev(int fd, const struct iovec *iov, int iovcnt)
{
    int result;
    int write_bytes;
    FCFSPosixAPIFileInfo *file;

    if ((file=fcfs_fd_manager_get(fd)) == NULL) {
        errno = EBADF;
        return -1;
    }

    if ((result=fcfs_api_writev_ex(&file->fi, iov, iovcnt, &write_bytes,
                    fcfs_posix_api_gettid(file->tpid_type))) != 0)
    {
        errno = result;
        return -1;
    } else {
        return write_bytes;
    }
}

ssize_t fcfs_pwritev(int fd, const struct iovec *iov,
        int iovcnt, off_t offset)
{
    int result;
    int write_bytes;
    FCFSPosixAPIFileInfo *file;

    if ((file=fcfs_fd_manager_get(fd)) == NULL) {
        errno = EBADF;
        return -1;
    }

    if ((result=fcfs_api_pwritev_ex(&file->fi, iov, iovcnt, offset,
                    &write_bytes, fcfs_posix_api_gettid(
                        file->tpid_type))) != 0)
    {
        errno = result;
        return -1;
    } else {
        return write_bytes;
    }
}

static inline ssize_t do_read(FCFSPosixAPIFileInfo *file,
        void *buff, size_t count)
{
    int result;
    int read_bytes;

    if ((result=fcfs_api_read_ex(&file->fi, buff, count, &read_bytes,
                    fcfs_posix_api_gettid(file->tpid_type))) != 0)
    {
        errno = result;
        return -1;
    } else {
        return read_bytes;
    }
}

ssize_t fcfs_read(int fd, void *buff, size_t count)
{
    FCFSPosixAPIFileInfo *file;

    if ((file=fcfs_fd_manager_get(fd)) == NULL) {
        errno = EBADF;
        return -1;
    }

    return do_read(file, buff, count);
}

ssize_t fcfs_readahead(int fd, off64_t offset, size_t count)
{
    FCFSPosixAPIFileInfo *file;

    if ((file=fcfs_fd_manager_get(fd)) == NULL) {
        errno = EBADF;
        return -1;
    }

    return 0;
}

ssize_t fcfs_file_read(int fd, void *buff, size_t size, size_t n)
{
    FCFSPosixAPIFileInfo *file;
    size_t expect;
    size_t bytes;
    size_t count;
    size_t remain;

    if ((file=fcfs_fd_manager_get(fd)) == NULL) {
        errno = EBADF;
        return -1;
    }

    expect = size * n;
    bytes = do_read(file, buff, expect);
    if (bytes == expect) {
        count = n;
    } else if (bytes > 0) {
        count = bytes / size;
        remain = bytes - (size * count);
        fcfs_api_lseek(&file->fi, -1 * remain, SEEK_CUR);
    } else {
        count = 0;
    }

    return count;
}

ssize_t fcfs_file_readline(int fd, char *s, size_t size)
{
    FCFSPosixAPIFileInfo *file;
    char *p;
    char *end;
    char *ch;
    int read_bytes_once;
    int current;
    int bytes;
    int remain;

    if (size <= 0) {
        errno = EINVAL;
        return -1;
    }

    if ((file=fcfs_fd_manager_get(fd)) == NULL) {
        errno = EBADF;
        return -1;
    }

    read_bytes_once = 64;
    remain = size;
    p = s;
    while (1) {
        current = FC_MIN(remain, read_bytes_once);
        bytes = do_read(file, p, current);
        if (bytes < 0) {
            return -1;
        } else if (bytes == 0) {
            break;
        }

        end = p + bytes;
        ch = p;
        while (ch < end && *ch != '\n') {
            ++ch;
        }

        if (ch < end) {  //found
            p = ++ch; //skip the new line
            remain = end - p;
            if (remain > 0) {
                fcfs_api_lseek(&file->fi, -1 * remain, SEEK_CUR);
            }
            break;
        }

        p = end;
        if (bytes < current) {  //end of file
            break;
        }

        remain -= bytes;
        if (remain == 0) {
            break;
        }

        read_bytes_once *= 2;
    }

    return (p - s);
}

ssize_t fcfs_file_gets(int fd, char *s, size_t size)
{
    ssize_t bytes;

    if ((bytes=fcfs_file_readline(fd, s, size - 1)) >= 0) {
        *(s + bytes) = '\0';
    }
    return bytes;
}

ssize_t fcfs_file_getdelim(int fd, char **line, size_t *size, int delim)
{
    FCFSPosixAPIFileInfo *file;
    char *buff;
    char *p;
    char *end;
    char *ch;
    ssize_t alloc;
    ssize_t bytes;
    ssize_t len;
    ssize_t remain;

    if (*line != NULL && *size < 0) {
        errno = EINVAL;
        return -1;
    }

    if ((file=fcfs_fd_manager_get(fd)) == NULL) {
        errno = EBADF;
        return -1;
    }

    if (*size < 128) {
        *size = 128;
        buff = fc_malloc(*size);
        if (buff == NULL) {
            errno = ENOMEM;
            return -1;
        }
        if (*line != NULL) {
            free(*line);
        }
        *line = buff;
    } else {
        buff = *line;
    }

    remain = *size - 1;
    p = buff;
    while (1) {
        bytes = do_read(file, p, remain);
        if (bytes < 0) {
            return -1;
        } else if (bytes == 0) {
            break;
        }

        end = p + bytes;
        ch = p;
        while (ch < end && *ch != delim) {
            ++ch;
        }

        if (ch < end) {  //found
            p = ++ch; //skip the new line
            remain = end - p;
            if (remain > 0) {
                fcfs_api_lseek(&file->fi, -1 * remain, SEEK_CUR);
            }
            break;
        }

        if (bytes < remain) {  //end of file
            p = end;
            break;
        }

        len = end - buff;
        alloc = (*size) * 2;
        buff = fc_malloc(alloc);
        if (buff == NULL) {
            errno = ENOMEM;
            return -1;
        }
        memcpy(buff, *line, len);
        free(*line);

        *line = buff;
        *size = alloc;
        p = buff + len;
        remain = (*size) - len;
    }

    *p = '\0';
    return (p - buff);
}

ssize_t fcfs_pread(int fd, void *buff, size_t count, off_t offset)
{
    int result;
    int read_bytes;
    FCFSPosixAPIFileInfo *file;

    if ((file=fcfs_fd_manager_get(fd)) == NULL) {
        errno = EBADF;
        return -1;
    }

    if ((result=fcfs_api_pread_ex(&file->fi, buff, count, offset,
                    &read_bytes, fcfs_posix_api_gettid(
                        file->tpid_type))) != 0)
    {
        errno = result;
        return -1;
    } else {
        return read_bytes;
    }
}

ssize_t fcfs_readv(int fd, const struct iovec *iov, int iovcnt)
{
    int result;
    int read_bytes;
    FCFSPosixAPIFileInfo *file;

    if ((file=fcfs_fd_manager_get(fd)) == NULL) {
        errno = EBADF;
        return -1;
    }

    if ((result=fcfs_api_readv_ex(&file->fi, iov, iovcnt, &read_bytes,
                    fcfs_posix_api_gettid(file->tpid_type))) != 0)
    {
        errno = result;
        return -1;
    } else {
        return read_bytes;
    }
}

ssize_t fcfs_preadv(int fd, const struct iovec *iov,
        int iovcnt, off_t offset)
{
    int result;
    int read_bytes;
    FCFSPosixAPIFileInfo *file;

    if ((file=fcfs_fd_manager_get(fd)) == NULL) {
        errno = EBADF;
        return -1;
    }

    if ((result=fcfs_api_preadv_ex(&file->fi, iov, iovcnt, offset,
                    &read_bytes, fcfs_posix_api_gettid(
                        file->tpid_type))) != 0)
    {
        errno = result;
        return -1;
    } else {
        return read_bytes;
    }
}

off_t fcfs_lseek(int fd, off_t offset, int whence)
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

off_t fcfs_ltell(int fd)
{
    FCFSPosixAPIFileInfo *file;

    if ((file=fcfs_fd_manager_get(fd)) == NULL) {
        errno = EBADF;
        return -1;
    }

    return file->fi.offset;
}

int fcfs_fallocate(int fd, int mode, off_t offset, off_t length)
{
    int result;
    FCFSPosixAPIFileInfo *file;

    if ((file=fcfs_fd_manager_get(fd)) == NULL) {
        errno = EBADF;
        return -1;
    }

    if ((result=fcfs_api_fallocate_ex(&file->fi, mode, offset, length,
                    fcfs_posix_api_gettid(file->tpid_type))) != 0)
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

    fcfs_posix_api_set_fctx(&fctx, ctx, ACCESSPERMS,
            fcfs_papi_tpid_type_tid);
    if ((result=fcfs_api_truncate_ex(&ctx->api_ctx,
                    path, length, &fctx)) != 0)
    {
        errno = result;
        return -1;
    } else {
        return 0;
    }
}

int fcfs_ftruncate(int fd, off_t length)
{
    int result;
    FCFSPosixAPIFileInfo *file;

    if ((file=fcfs_fd_manager_get(fd)) == NULL) {
        errno = EBADF;
        return -1;
    }

    if ((result=fcfs_api_ftruncate_ex(&file->fi, length,
                    fcfs_posix_api_gettid(file->tpid_type))) != 0)
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

int fcfs_fstat(int fd, struct stat *buf)
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

int fcfs_flock(int fd, int operation)
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

static int do_fcntl(FCFSPosixAPIFileInfo *file, int cmd, void *arg)
{
    int64_t owner_id;
    struct flock *lock;
    int flags;
    int result;


    switch (cmd) {
        /*
        case F_DUPFD:
            errno = EOPNOTSUPP;
            return -1;
        */
        case F_GETFD:
        case F_SETFD:
            return 0;
        case F_GETFL:
            return file->fi.flags;
        case F_SETFL:
            flags = (long)arg;
            result = fcfs_api_set_file_flags(&file->fi, flags);
            break;
        case F_GETLK:
        case F_SETLK:
        case F_SETLKW:
            lock = (struct flock *)arg;
            if (cmd == F_GETLK) {
                result = fcfs_api_getlk_ex(&file->fi, lock, &owner_id);
            } else {
                owner_id = fc_gettid();
                result = fcfs_api_setlk_ex(&file->fi, lock,
                        owner_id, (cmd == F_SETLKW));
            }
            break;
        default:
            logError("file: "__FILE__", line: %d, "
                    "unsupport fcntl cmd: %d", __LINE__, cmd);
            errno = EOPNOTSUPP;
            return -1;
    }

    if (result != 0) {
        errno = result;
        return -1;
    } else {
        return 0;
    }
}

int fcfs_fcntl(int fd, int cmd, ...)
{
    FCFSPosixAPIFileInfo *file;
    va_list ap;
    void *arg;

    if ((file=fcfs_fd_manager_get(fd)) == NULL) {
        errno = EBADF;
        return -1;
    }

    va_start(ap, cmd);
    arg = va_arg(ap, void *);
    va_end(ap);
    return do_fcntl(file, cmd, arg);
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

    fcfs_posix_api_set_omp(&omp, ctx, ACCESSPERMS);
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

    fcfs_posix_api_set_omp(&omp, ctx, ACCESSPERMS);
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

    fcfs_posix_api_set_omp(&omp, ctx, ACCESSPERMS);
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

    fcfs_posix_api_set_omp(&omp, ctx, ACCESSPERMS);
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

int fcfs_mkfifo_ex(FCFSPosixAPIContext *ctx,
        const char *path, mode_t mode)
{
    FDIRClientOwnerModePair omp;
    char full_fname[PATH_MAX];
    int result;

    if ((result=papi_resolve_path(ctx, "mkfifo", &path,
                    full_fname, sizeof(full_fname))) != 0)
    {
        return result;
    }

    fcfs_posix_api_set_omp(&omp, ctx, mode);
    if ((result=fcfs_api_mkfifo_ex(&ctx->api_ctx, path, &omp)) != 0) {
        errno = result;
        return -1;
    } else {
        return 0;
    }
}

int fcfs_mkfifoat_ex(FCFSPosixAPIContext *ctx, int fd,
        const char *path, mode_t mode)
{
    FDIRClientOwnerModePair omp;
    char full_fname[PATH_MAX];
    int result;

    if ((result=papi_resolve_pathat(ctx, "mkfifoat", fd, &path,
                    full_fname, sizeof(full_fname))) != 0)
    {
        return result;
    }

    fcfs_posix_api_set_omp(&omp, ctx, mode);
    if ((result=fcfs_api_mkfifo_ex(&ctx->api_ctx, path, &omp)) != 0) {
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

    fcfs_posix_api_set_omp(&omp, ctx, ACCESSPERMS);
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

    fcfs_posix_api_set_omp(&omp, ctx, ACCESSPERMS);
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

int fcfs_euidaccess_ex(FCFSPosixAPIContext *ctx,
        const char *path, int mode)
{
    const int flags = FDIR_FLAGS_FOLLOW_SYMLINK;
    FDIRClientOwnerModePair omp;
    char full_fname[PATH_MAX];
    int result;

    if ((result=papi_resolve_path(ctx, "euidaccess", &path,
                    full_fname, sizeof(full_fname))) != 0)
    {
        return result;
    }

    fcfs_posix_api_set_omp(&omp, ctx, ACCESSPERMS);
    if ((result=fcfs_api_euidaccess_ex(&ctx->api_ctx,
                    path, mode, &omp, flags)) != 0)
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
                    fcfs_posix_api_gettid(fcfs_papi_tpid_type_tid))) != 0)
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
                     FDIR_UNLINK_FLAGS_MATCH_FILE), fcfs_posix_api_gettid(
                         fcfs_papi_tpid_type_tid))) != 0)
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
                    path2, flags, fcfs_posix_api_gettid(
                        fcfs_papi_tpid_type_tid))) != 0)
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
                    path2, flags, fcfs_posix_api_gettid(
                        fcfs_papi_tpid_type_tid))) != 0)
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

int fcfs_mkdir_ex(FCFSPosixAPIContext *ctx,
        const char *path, mode_t mode)
{
    FDIRClientOwnerModePair omp;
    char full_fname[PATH_MAX];
    int result;

    if ((result=papi_resolve_path(ctx, "mkdir", &path,
                    full_fname, sizeof(full_fname))) != 0)
    {
        return result;
    }

    fcfs_posix_api_set_omp(&omp, ctx, mode);
    if ((result=fcfs_api_mkdir_ex(&ctx->api_ctx, path, &omp)) != 0) {
        errno = result;
        return -1;
    } else {
        return 0;
    }
}

int fcfs_mkdirat_ex(FCFSPosixAPIContext *ctx, int fd,
        const char *path, mode_t mode)
{
    FDIRClientOwnerModePair omp;
    char full_fname[PATH_MAX];
    int result;

    if ((result=papi_resolve_pathat(ctx, "mkdirat", fd, &path,
                    full_fname, sizeof(full_fname))) != 0)
    {
        return result;
    }

    fcfs_posix_api_set_omp(&omp, ctx, mode);
    if ((result=fcfs_api_mkdir_ex(&ctx->api_ctx, path, &omp)) != 0) {
        errno = result;
        return -1;
    } else {
        return 0;
    }
}

int fcfs_rmdir_ex(FCFSPosixAPIContext *ctx, const char *path)
{
    const int flags = FDIR_UNLINK_FLAGS_MATCH_DIR;
    char full_fname[PATH_MAX];
    int result;

    if ((result=papi_resolve_path(ctx, "rmdir", &path,
                    full_fname, sizeof(full_fname))) != 0)
    {
        return result;
    }

    if ((result=fcfs_api_remove_dentry_ex(&ctx->api_ctx, path, flags,
                    fcfs_posix_api_gettid(fcfs_papi_tpid_type_tid))) != 0)
    {
        errno = result;
        return -1;
    } else {
        return 0;
    }
}

int fcfs_chown_ex(FCFSPosixAPIContext *ctx, const char *path,
        uid_t owner, gid_t group)
{
    const int flags = FDIR_FLAGS_FOLLOW_SYMLINK;
    char full_fname[PATH_MAX];
    int result;

    if ((result=papi_resolve_path(ctx, "chown", &path,
                    full_fname, sizeof(full_fname))) != 0)
    {
        return result;
    }

    if ((result=fcfs_api_chown_ex(&ctx->api_ctx, path,
                    owner, group, flags)) != 0)
    {
        errno = result;
        return -1;
    } else {
        return 0;
    }
}

int fcfs_lchown_ex(FCFSPosixAPIContext *ctx, const char *path,
        uid_t owner, gid_t group)
{
    const int flags = 0;
    char full_fname[PATH_MAX];
    int result;

    if ((result=papi_resolve_path(ctx, "lchown", &path,
                    full_fname, sizeof(full_fname))) != 0)
    {
        return result;
    }

    if ((result=fcfs_api_chown_ex(&ctx->api_ctx, path,
                    owner, group, flags)) != 0)
    {
        errno = result;
        return -1;
    } else {
        return 0;
    }
}

int fcfs_fchown_ex(FCFSPosixAPIContext *ctx, int fd,
        uid_t owner, gid_t group)
{
    const int flags = FDIR_FLAGS_FOLLOW_SYMLINK;
    FCFSPosixAPIFileInfo *file;
    int result;

    if ((file=fcfs_fd_manager_get(fd)) == NULL) {
        errno = EBADF;
        return -1;
    }

    if ((result=fcfs_api_chown_by_inode_ex(&ctx->api_ctx, file->
                    fi.dentry.inode, owner, group, flags)) != 0)
    {
        errno = result;
        return -1;
    } else {
        return 0;
    }
}

int fcfs_fchownat_ex(FCFSPosixAPIContext *ctx, int fd,
        const char *path, uid_t owner, gid_t group, int flags)
{
    char full_fname[PATH_MAX];
    int result;

    if ((result=papi_resolve_pathat(ctx, "fchownat", fd, &path,
                    full_fname, sizeof(full_fname))) != 0)
    {
        return result;
    }

    if ((result=fcfs_api_chown_ex(&ctx->api_ctx, path, owner,
                    group, (flags & AT_SYMLINK_NOFOLLOW) ?
                    0 : FDIR_FLAGS_FOLLOW_SYMLINK)) != 0)
    {
        errno = result;
        return -1;
    } else {
        return 0;
    }
}

int fcfs_chmod_ex(FCFSPosixAPIContext *ctx,
        const char *path, mode_t mode)
{
    const int flags = 0;
    char full_fname[PATH_MAX];
    int result;

    if ((result=papi_resolve_path(ctx, "chmod", &path,
                    full_fname, sizeof(full_fname))) != 0)
    {
        return result;
    }

    if ((result=fcfs_api_chmod_ex(&ctx->api_ctx,
                    path, mode, flags)) != 0)
    {
        errno = result;
        return -1;
    } else {
        return 0;
    }
}

int fcfs_fchmod_ex(FCFSPosixAPIContext *ctx,
        int fd, mode_t mode)
{
    const int flags = FDIR_FLAGS_FOLLOW_SYMLINK;
    FCFSPosixAPIFileInfo *file;
    int result;

    if ((file=fcfs_fd_manager_get(fd)) == NULL) {
        errno = EBADF;
        return -1;
    }

    if ((result=fcfs_api_chmod_by_inode_ex(&ctx->api_ctx, file->
                    fi.dentry.inode, mode, flags)) != 0)
    {
        errno = result;
        return -1;
    } else {
        return 0;
    }
}

int fcfs_fchmodat_ex(FCFSPosixAPIContext *ctx, int fd,
        const char *path, mode_t mode, int flags)
{
    char full_fname[PATH_MAX];
    int result;

    if ((result=papi_resolve_pathat(ctx, "fchmodat", fd, &path,
                    full_fname, sizeof(full_fname))) != 0)
    {
        return result;
    }

    if ((result=fcfs_api_chmod_ex(&ctx->api_ctx, path,
                    mode, (flags & AT_SYMLINK_NOFOLLOW) ?
                    0 : FDIR_FLAGS_FOLLOW_SYMLINK)) != 0)
    {
        errno = result;
        return -1;
    } else {
        return 0;
    }
}

int fcfs_statvfs_ex(FCFSPosixAPIContext *ctx,
        const char *path, struct statvfs *buf)
{
    char full_fname[PATH_MAX];
    int result;

    if ((result=papi_resolve_path(ctx, "chmod", &path,
                    full_fname, sizeof(full_fname))) != 0)
    {
        return result;
    }

    if ((result=fcfs_api_statvfs(path, buf)) != 0) {
        errno = result;
        return -1;
    } else {
        return 0;
    }
}

int fcfs_fstatvfs_ex(FCFSPosixAPIContext *ctx, int fd,
        struct statvfs *buf)
{
    FCFSPosixAPIFileInfo *file;
    int result;

    if ((file=fcfs_fd_manager_get(fd)) == NULL) {
        errno = EBADF;
        return -1;
    }

    if ((result=fcfs_api_statvfs(file->filename.str, buf)) != 0) {
        errno = result;
        return -1;
    } else {
        return 0;
    }
}

static inline int do_setxattr(FCFSPosixAPIContext *ctx, const char *func,
        const char *path, const char *name, const void *value,
        size_t size, int flags)
{
    char full_fname[PATH_MAX];
    key_value_pair_t xattr;
    int result;

    if ((result=papi_resolve_path(ctx, func, &path,
                    full_fname, sizeof(full_fname))) != 0)
    {
        return result;
    }

    FC_SET_STRING(xattr.key, (char *)name);
    FC_SET_STRING_EX(xattr.value, (char *)value, size);
    if ((result=fcfs_api_set_xattr_by_path_ex(&ctx->api_ctx,
                    path, &xattr, flags)) != 0)
    {
        errno = result;
        return -1;
    } else {
        return 0;
    }
}

int fcfs_setxattr_ex(FCFSPosixAPIContext *ctx, const char *path,
        const char *name, const void *value, size_t size, int flags)
{
    return do_setxattr(ctx, "setxattr", path, name, value, size,
            (flags | FDIR_FLAGS_FOLLOW_SYMLINK));
}

int fcfs_lsetxattr_ex(FCFSPosixAPIContext *ctx, const char *path,
        const char *name, const void *value, size_t size, int flags)
{
    return do_setxattr(ctx, "lsetxattr", path, name, value, size,
            (flags & (~FDIR_FLAGS_FOLLOW_SYMLINK)));
}

int fcfs_fsetxattr_ex(FCFSPosixAPIContext *ctx, int fd, const char *name,
        const void *value, size_t size, int flags)
{
    key_value_pair_t xattr;
    FCFSPosixAPIFileInfo *file;
    int result;

    if ((file=fcfs_fd_manager_get(fd)) == NULL) {
        errno = EBADF;
        return -1;
    }

    FC_SET_STRING(xattr.key, (char *)name);
    FC_SET_STRING_EX(xattr.value, (char *)value, size);
    if ((result=fcfs_api_set_xattr_by_inode_ex(&ctx->api_ctx,
                    file->fi.dentry.inode, &xattr, (flags |
                        FDIR_FLAGS_FOLLOW_SYMLINK))) != 0)
    {
        errno = result;
        return -1;
    } else {
        return 0;
    }
}

#define GET_XATTR_FLAGS_BY_VSIZE(size, flags) \
    ((size == 0) ? (flags | FDIR_FLAGS_XATTR_GET_SIZE) : flags)

static inline ssize_t do_getxattr(FCFSPosixAPIContext *ctx,
        const char *func, const char *path, const char *name,
        void *value, size_t size, int flags)
{
    char full_fname[PATH_MAX];
    string_t nm;
    string_t vl;
    int result;

    if ((result=papi_resolve_path(ctx, func, &path,
                    full_fname, sizeof(full_fname))) != 0)
    {
        return result;
    }

    FC_SET_STRING(nm, (char *)name);
    FC_SET_STRING_EX(vl, (char *)value, 0);
    if ((result=fcfs_api_get_xattr_by_path_ex(&ctx->api_ctx,
                    path, &nm, LOG_DEBUG, &vl, size,
                    GET_XATTR_FLAGS_BY_VSIZE(size, flags))) != 0)
    {
        errno = result;
        return -1;
    } else {
        return vl.len;
    }
}

ssize_t fcfs_getxattr_ex(FCFSPosixAPIContext *ctx, const char *path,
        const char *name, void *value, size_t size)
{
    const int flags = FDIR_FLAGS_FOLLOW_SYMLINK;
    return do_getxattr(ctx, "getxattr", path, name, value, size, flags);
}

ssize_t fcfs_lgetxattr_ex(FCFSPosixAPIContext *ctx, const char *path,
        const char *name, void *value, size_t size)
{
    const int flags = 0;
    return do_getxattr(ctx, "lgetxattr", path, name, value, size, flags);
}

ssize_t fcfs_fgetxattr_ex(FCFSPosixAPIContext *ctx, int fd,
        const char *name, void *value, size_t size)
{
    const int flags = FDIR_FLAGS_FOLLOW_SYMLINK;
    FCFSPosixAPIFileInfo *file;
    string_t nm;
    string_t vl;
    int result;

    if ((file=fcfs_fd_manager_get(fd)) == NULL) {
        errno = EBADF;
        return -1;
    }


    FC_SET_STRING(nm, (char *)name);
    FC_SET_STRING_EX(vl, (char *)value, 0);
    if ((result=fcfs_api_get_xattr_by_inode_ex(&ctx->api_ctx, file->
                    fi.dentry.inode, &nm, LOG_DEBUG, &vl, size,
                    GET_XATTR_FLAGS_BY_VSIZE(size, flags))) != 0)
    {
        errno = result;
        return -1;
    } else {
        return vl.len;
    }
}

static inline ssize_t do_listxattr(FCFSPosixAPIContext *ctx,
        const char *func, const char *path, char *list,
        size_t size, int flags)
{
    char full_fname[PATH_MAX];
    string_t ls;
    int result;

    if ((result=papi_resolve_path(ctx, func, &path,
                    full_fname, sizeof(full_fname))) != 0)
    {
        return result;
    }

    ls.str = list;
    if ((result=fcfs_api_list_xattr_by_path_ex(&ctx->api_ctx, path, &ls,
                    size, GET_XATTR_FLAGS_BY_VSIZE(size, flags))) != 0)
    {
        errno = result;
        return -1;
    } else {
        return ls.len;
    }
}

ssize_t fcfs_listxattr_ex(FCFSPosixAPIContext *ctx,
        const char *path, char *list, size_t size)
{
    const int flags = FDIR_FLAGS_FOLLOW_SYMLINK;
    return do_listxattr(ctx, "listattr", path, list, size, flags);
}

ssize_t fcfs_llistxattr_ex(FCFSPosixAPIContext *ctx,
        const char *path, char *list, size_t size)
{
    const int flags = 0;
    return do_listxattr(ctx, "llistattr", path, list, size, flags);
}

ssize_t fcfs_flistxattr_ex(FCFSPosixAPIContext *ctx,
        int fd, char *list, size_t size)
{
    const int flags = FDIR_FLAGS_FOLLOW_SYMLINK;
    FCFSPosixAPIFileInfo *file;
    string_t ls;
    int result;

    if ((file=fcfs_fd_manager_get(fd)) == NULL) {
        errno = EBADF;
        return -1;
    }

    ls.str = list;
    if ((result=fcfs_api_list_xattr_by_inode_ex(&ctx->api_ctx,
                    file->fi.dentry.inode, &ls, size,
                    GET_XATTR_FLAGS_BY_VSIZE(size, flags))) != 0)
    {
        errno = result;
        return -1;
    } else {
        return ls.len;
    }
}

static inline int do_removexattr(FCFSPosixAPIContext *ctx, const char *func,
        const char *path, const char *name, int flags)
{
    char full_fname[PATH_MAX];
    string_t nm;
    int result;

    if ((result=papi_resolve_path(ctx, func, &path,
                    full_fname, sizeof(full_fname))) != 0)
    {
        return result;
    }

    FC_SET_STRING(nm, (char *)name);
    if ((result=fcfs_api_remove_xattr_by_path_ex(&ctx->api_ctx,
                    path, &nm, flags)) != 0)
    {
        errno = result;
        return -1;
    } else {
        return 0;
    }
}

int fcfs_removexattr_ex(FCFSPosixAPIContext *ctx,
        const char *path, const char *name)
{
    const int flags = FDIR_FLAGS_FOLLOW_SYMLINK;
    return do_removexattr(ctx, "removexattr", path, name, flags);
}

int fcfs_lremovexattr_ex(FCFSPosixAPIContext *ctx,
        const char *path, const char *name)
{
    const int flags = 0;
    return do_removexattr(ctx, "lremovexattr", path, name, flags);
}

int fcfs_fremovexattr_ex(FCFSPosixAPIContext *ctx,
        int fd, const char *name)
{
    const int flags = FDIR_FLAGS_FOLLOW_SYMLINK;
    FCFSPosixAPIFileInfo *file;
    string_t nm;
    int result;

    if ((file=fcfs_fd_manager_get(fd)) == NULL) {
        errno = EBADF;
        return -1;
    }

    FC_SET_STRING(nm, (char *)name);
    if ((result=fcfs_api_remove_xattr_by_inode_ex(&ctx->api_ctx,
                    file->fi.dentry.inode, &nm, flags)) != 0)
    {
        errno = result;
        return -1;
    } else {
        return 0;
    }
}

static FCFSPosixAPIDIR *do_opendir(FCFSPosixAPIContext *ctx,
        FCFSPosixAPIFileInfo *file)
{
    FCFSPosixAPIDIR *dir;
    int result;

    dir = (FCFSPosixAPIDIR *)fc_malloc(sizeof(FCFSPosixAPIDIR));
    if (dir == NULL) {
        errno = ENOMEM;
        return NULL;
    }

    dir->file = file;
    dir->magic = FCFS_PAPI_MAGIC_NUMBER;
    dir->offset = 0;
    fdir_client_compact_dentry_array_init(&dir->darray);
    if ((result=fcfs_api_list_compact_dentry_by_inode_ex(&ctx->api_ctx,
                    file->fi.dentry.inode, &dir->darray)) != 0)
    {
        free(dir);
        errno = result;
        return NULL;
    }

    return dir;
}

DIR *fcfs_opendir_ex(FCFSPosixAPIContext *ctx, const char *path)
{
    const int flags = O_RDONLY;
    const mode_t mode = ACCESSPERMS | S_IFDIR;
    FCFSPosixAPIFileInfo *file;
    FCFSPosixAPIDIR *dir;
    char full_fname[PATH_MAX];

    if (papi_resolve_path(ctx, "opendir", &path,
                full_fname, sizeof(full_fname)) != 0)
    {
        return NULL;
    }

    if ((file=do_open_ex(ctx, path, flags, mode,
                    fcfs_papi_tpid_type_tid)) == NULL)
    {
        return NULL;
    }

    if ((dir=do_opendir(ctx, file)) == NULL) {
        fcfs_api_close(&file->fi);
        fcfs_fd_manager_free(file);
    }
    return (DIR *)dir;
}

DIR *fcfs_fdopendir_ex(FCFSPosixAPIContext *ctx, int fd)
{
    FCFSPosixAPIFileInfo *file;
    FCFSPosixAPIDIR *dir;

    if ((file=fcfs_fd_manager_get(fd)) == NULL) {
        errno = EBADF;
        return NULL;
    }

    if ((dir=do_opendir(ctx, file)) == NULL) {
        fcfs_api_close(&file->fi);
        fcfs_fd_manager_free(file);
    }
    return (DIR *)dir;
}

#define FCFS_CONVERT_DIRP_EX(dirp, ...) \
    FCFSPosixAPIDIR *dir; \
    dir = (FCFSPosixAPIDIR *)dirp; \
    if (dir->magic != FCFS_PAPI_MAGIC_NUMBER) { \
        errno = EBADF; \
        return __VA_ARGS__; \
    }

#define FCFS_CONVERT_DIRP(dirp) \
    FCFS_CONVERT_DIRP_EX(dirp, -1)

#define FCFS_CONVERT_DIRP_VOID(dirp) \
    FCFS_CONVERT_DIRP_EX(dirp)

int fcfs_closedir_ex(FCFSPosixAPIContext *ctx, DIR *dirp)
{
    FCFS_CONVERT_DIRP(dirp);

    fdir_client_compact_dentry_array_free(&dir->darray);
    fcfs_api_close(&dir->file->fi);
    fcfs_fd_manager_free(dir->file);
    dir->magic = 0;
    free(dir);
    return 0;
}

static inline struct dirent *do_readdir(FCFSPosixAPIDIR *dir)
{
    if (dir->offset < 0 || dir->offset >= dir->darray.count) {
        return NULL;
    }

    return dir->darray.entries + dir->offset++;
}

struct dirent *fcfs_readdir_ex(FCFSPosixAPIContext *ctx, DIR *dirp)
{
    FCFS_CONVERT_DIRP_EX(dirp, NULL);
    return do_readdir(dir);
}

int fcfs_readdir_r_ex(FCFSPosixAPIContext *ctx, DIR *dirp,
        struct dirent *entry, struct dirent **result)
{
    struct dirent *current;
    FCFS_CONVERT_DIRP(dirp);

    if ((current=do_readdir(dir)) != NULL) {
        memcpy(entry, current, sizeof(FDIRDirent));
        *result = entry;
    } else {
        *result = NULL;
    }

    return 0;
}

void fcfs_seekdir_ex(FCFSPosixAPIContext *ctx, DIR *dirp, long loc)
{
    FCFS_CONVERT_DIRP_VOID(dirp);
    dir->offset = loc;
}

long fcfs_telldir_ex(FCFSPosixAPIContext *ctx, DIR *dirp)
{
    FCFS_CONVERT_DIRP(dirp);
    return dir->offset;
}

void fcfs_rewinddir_ex(FCFSPosixAPIContext *ctx, DIR *dirp)
{
    FCFS_CONVERT_DIRP_VOID(dirp);
    dir->offset = 0;
}

int fcfs_dirfd_ex(FCFSPosixAPIContext *ctx, DIR *dirp)
{
    FCFS_CONVERT_DIRP(dirp);

    if (fcfs_fd_manager_get(dir->file->fd) != dir->file) {
        errno = EBADF;
        return -1;
    }

    return dir->file->fd;
}

static int do_scandir(FCFSPosixAPIContext *ctx, const char *path,
        struct dirent ***namelist, int (*filter)(const struct dirent *),
        int (*compar)(const struct dirent **, const struct dirent **))
{
    FCFSPosixAPIDIR dir;
    FDIRDirent *ent;
    FDIRDirent *end;
    FDIRDirent **cur;
    int result;
    int count;

    fdir_client_compact_dentry_array_init(&dir.darray);
    if ((result=fcfs_api_list_compact_dentry_by_path_ex(&ctx->
                    api_ctx, path, &dir.darray)) != 0)
    {
        errno = result;
        return -1;
    }

    do {
        if ((*namelist=fc_malloc(sizeof(FDIRDirent *) *
                        dir.darray.count)) == NULL)
        {
            errno = ENOMEM;
            count = -1;
            break;
        }

        count = 0;
        if (dir.darray.count == 0) {
            break;
        }

        cur = (FDIRDirent **)*namelist;
        end = dir.darray.entries + dir.darray.count;
        for (ent=dir.darray.entries; ent<end; ent++) {
            if (filter != NULL && ((int (*)(const FDIRDirent *))
                    filter)(ent) == 0)
            {
                continue;
            }

            if ((*cur=fc_malloc(sizeof(FDIRDirent))) == NULL) {
                errno = ENOMEM;
                count = -1;
                break;
            }
            memcpy(*cur, ent, sizeof(FDIRDirent));
            cur++;
        }

        if (count < 0) {  //error
            FDIRDirent **endp;
            endp = cur;
            for (cur=(FDIRDirent **)*namelist; cur<endp; cur++) {
                free(*cur);
            }
            free(*namelist);
            *namelist = NULL;
        } else {
            count = cur - (FDIRDirent **)(*namelist);
            if (compar != NULL && count > 1) {
                qsort(*namelist, count, sizeof(struct dirent *),
                        (int (*)(const void *, const void *))compar);
            }
        }
    } while (0);

    fdir_client_compact_dentry_array_free(&dir.darray);
    return count;
}

int fcfs_scandir_ex(FCFSPosixAPIContext *ctx, const char *path,
        struct dirent ***namelist, int (*filter)(const struct dirent *),
        int (*compar)(const struct dirent **, const struct dirent **))
{
    char full_fname[PATH_MAX];

    if (papi_resolve_path(ctx, "scandir", &path,
                full_fname, sizeof(full_fname)) != 0)
    {
        return -1;
    }

    return do_scandir(ctx, path, namelist, filter, compar);
}

int fcfs_scandirat_ex(FCFSPosixAPIContext *ctx, int fd, const char *path,
        struct dirent ***namelist, int (*filter)(const struct dirent *),
        int (*compar)(const struct dirent **, const struct dirent **))
{
    char full_fname[PATH_MAX];

    if (papi_resolve_pathat(ctx, "scandirat", fd, &path,
                full_fname, sizeof(full_fname)) != 0)
    {
        return -1;
    }

    return do_scandir(ctx, path, namelist, filter, compar);
}

static int do_chdir(const string_t *path)
{
    string_t *cwd;
    char *old_cwd;

    if ((cwd=fc_malloc(sizeof(string_t) + path->len + 1)) == NULL) {
        errno = ENOMEM;
        return -1;
    }

    cwd->str = (char *)(cwd + 1);
    memcpy(cwd->str, path->str, path->len + 1);
    cwd->len = path->len;

    old_cwd = (G_FCFS_PAPI_CWD != NULL ? G_FCFS_PAPI_CWD->str : NULL);
    G_FCFS_PAPI_CWD = cwd;
    if (old_cwd != NULL) {
        free(old_cwd);
    }
    return 0;
}

int fcfs_chdir_ex(FCFSPosixAPIContext *ctx, const char *path)
{
    char full_fname[PATH_MAX];
    string_t cwd;

    if (papi_resolve_path(ctx, "chdir", &path,
                full_fname, sizeof(full_fname)) != 0)
    {
        return -1;
    }

    FC_SET_STRING(cwd, (char *)path);
    return do_chdir(&cwd);
}

int fcfs_fchdir_ex(FCFSPosixAPIContext *ctx, int fd)
{
    FCFSPosixAPIFileInfo *file;
    char full_path[PATH_MAX];
    string_t cwd;
    char *p;
    int len;

    if ((file=fcfs_fd_manager_get(fd)) == NULL) {
        errno = EBADF;
        return -1;
    }

    p = strrchr(file->filename.str, '/');
    if (p == NULL) {
        errno = EBUSY;
        return -1;
    }

    len = (p - file->filename.str) + 1;
    snprintf(full_path, sizeof(full_path), "%.*s",
            len, file->filename.str);
    FC_SET_STRING_EX(cwd, full_path, len);
    return do_chdir(&cwd);
}

char *fcfs_getcwd_ex(FCFSPosixAPIContext *ctx, char *buf, size_t size)
{
    return do_getcwd(ctx, buf, size, ERANGE);
}

char *fcfs_getwd_ex(FCFSPosixAPIContext *ctx, char *buf)
{
    return do_getcwd(ctx, buf, PATH_MAX, ENAMETOOLONG);
}

#define FCFS_API_NOT_IMPLEMENTED(api_name, retval)  \
    logError("file: "__FILE__", line: %d, " \
            "function \"%s\" not implemented!", \
            __LINE__, api_name); \
    errno = EOPNOTSUPP;  \
    return retval

int fcfs_chroot_ex(FCFSPosixAPIContext *ctx, const char *path)
{
    FCFS_API_NOT_IMPLEMENTED("chroot", -1);
}

int fcfs_dup_ex(FCFSPosixAPIContext *ctx, int fd)
{
    logInfo("file: "__FILE__", line: %d, func: %s, "
            "fd: %d", __LINE__, __FUNCTION__, fd);
    FCFS_API_NOT_IMPLEMENTED("dup", -1);
}

int fcfs_dup2_ex(FCFSPosixAPIContext *ctx, int fd1, int fd2)
{
    logInfo("file: "__FILE__", line: %d, func: %s, "
            "fd1: %d, fd2: %d", __LINE__, __FUNCTION__, fd1, fd2);
    FCFS_API_NOT_IMPLEMENTED("dup2", -1);
}

void *fcfs_mmap_ex(FCFSPosixAPIContext *ctx, void *addr, size_t length,
        int prot, int flags, int fd, off_t offset)
{
    FCFS_API_NOT_IMPLEMENTED("mmap", NULL);
}

int fcfs_munmap_ex(FCFSPosixAPIContext *ctx, void *addr, size_t length)
{
    FCFS_API_NOT_IMPLEMENTED("munmap", -1);
}

int fcfs_lockf_ex(FCFSPosixAPIContext *ctx, int fd, int cmd, off_t len)
{
    FCFSPosixAPIFileInfo *file;
    struct flock lock;
    int fcntl_cmd;

    if ((file=fcfs_fd_manager_get(fd)) == NULL) {
        errno = EBADF;
        return -1;
    }

    switch (cmd) {
        case F_LOCK:
            fcntl_cmd = F_SETLKW;
            lock.l_type = F_WRLCK;
            break;
        case F_TLOCK:
            fcntl_cmd = F_SETLK;
            lock.l_type = F_WRLCK;
            break;
        case F_ULOCK:
            fcntl_cmd = F_SETLK;
            lock.l_type = F_UNLCK;
            break;
        case F_TEST:
            fcntl_cmd = F_GETLK;
            lock.l_type = F_UNLCK;
            break;
        default:
            errno = EOPNOTSUPP;
            return -1;
    }

    lock.l_whence = SEEK_SET;
    lock.l_start = file->fi.offset;
    lock.l_len = len;
    lock.l_pid = fcfs_posix_api_getpid();
    return do_fcntl(file, fcntl_cmd, &lock);
}

int fcfs_posix_fallocate_ex(FCFSPosixAPIContext *ctx,
        int fd, off_t offset, off_t len)
{
    const int mode = 0;
    int result;
    FCFSPosixAPIFileInfo *file;

    if ((file=fcfs_fd_manager_get(fd)) == NULL) {
        errno = EBADF;
        return -1;
    }

    if ((result=fcfs_api_fallocate_ex(&file->fi, mode, offset, len,
                    fcfs_posix_api_gettid(file->tpid_type))) != 0)
    {
        errno = result;
        return -1;
    } else {
        return 0;
    }
}

int fcfs_posix_fadvise_ex(FCFSPosixAPIContext *ctx, int fd,
            off_t offset, off_t len, int advice)
{
    FCFSPosixAPIFileInfo *file;

    if ((file=fcfs_fd_manager_get(fd)) == NULL) {
        errno = EBADF;
        return -1;
    }

    return 0;
}

static int do_vdprintf(FCFSPosixAPIFileInfo *file,
        const char *format, va_list ap)
{
#define FIXED_BUFFUER_SIZE (4 * 1024)
    char fixed[FIXED_BUFFUER_SIZE];
    char *buff;
    int length;
    int result;
    va_list new_ap;

    va_copy(new_ap, ap);
    buff = fixed;
    length = vsnprintf(buff, FIXED_BUFFUER_SIZE, format, ap);
    if (length > FIXED_BUFFUER_SIZE - 1) {  //overflow
        buff = (char *)fc_malloc(length + 1);
        if (buff == NULL) {
            errno = ENOMEM;
            return -1;
        }
        length = vsprintf(buff, format, new_ap);
    }
    va_end(new_ap);

    result = do_write(file, buff, length);
    if (buff != fixed) {
        free(buff);
    }
    if (result != 0) {
        errno = result;
        return -1;
    } else {
        return length;
    }
}

int fcfs_dprintf(int fd, const char *format, ...)
{
    FCFSPosixAPIFileInfo *file;
    va_list ap;
    int bytes;

    if ((file=fcfs_fd_manager_get(fd)) == NULL) {
        errno = EBADF;
        return -1;
    }

    va_start(ap, format);
    bytes = do_vdprintf(file, format, ap);
    va_end(ap);
    return bytes;
}

int fcfs_vdprintf(int fd, const char *format, va_list ap)
{
    FCFSPosixAPIFileInfo *file;

    if ((file=fcfs_fd_manager_get(fd)) == NULL) {
        errno = EBADF;
        return -1;
    }

    return do_vdprintf(file, format, ap);
}
