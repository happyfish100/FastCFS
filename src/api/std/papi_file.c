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
#include "fastcommon/shared_func.h"
#include "fastcommon/logger.h"
#include "posix_api.h"
#include "fd_manager.h"
#include "papi_file.h"

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

    return file->fd;
}

int fcfs_open_ex(FCFSPosixAPIContext *ctx, const char *path, int flags, ...)
{
    char full_filename[PATH_MAX];
    va_list ap;
    int fd;

    if (*path != '/') {
        normalize_path(NULL, path, full_filename,
                sizeof(full_filename));
        path = full_filename;
    }

    va_start(ap, flags);
    if (!FCFS_PAPI_IS_MY_MOUNT_PATH(ctx, path)) {
        fd = open(path, flags, ap);
    } else {
        fd = do_open(ctx, path + ctx->mountpoint.len, flags, ap);
    }

    va_end(ap);
    return fd;
}

int fcfs_openat_ex(FCFSPosixAPIContext *ctx, int fd,
        const char *path, int flags, ...)
{
    va_list ap;
    FCFSPosixAPIFileInfo *file;
    char full_filename[PATH_MAX];
    int new_fd;

    if (*path == '/') {
        va_start(ap, flags);
        new_fd = fcfs_open_ex(ctx, path, flags, ap);
        va_end(ap);
        return new_fd;
    }

    if (!FCFS_PAPI_IS_MY_FD(fd)) {
        va_start(ap, flags);
        new_fd = openat(fd, path, flags, ap);
        va_end(ap);

        return new_fd;
    }

    if ((file=fcfs_fd_manager_get(fd)) == NULL) {
        errno = EBADF;
        return -1;
    }

    normalize_path(file->filename, path, full_filename,
            sizeof(full_filename));
    va_start(ap, flags);
    new_fd = do_open(ctx, full_filename, flags, ap);
    va_end(ap);
    return new_fd;
}
