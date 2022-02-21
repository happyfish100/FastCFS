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

#include "fastcommon/shared_func.h"
#include "fastcommon/logger.h"
#include "papi.h"
#include "capi.h"

#define FCFS_CAPI_MAGIC_NUMBER 1645431088

/*
typedef struct fcfs_posix_file_buffer {
    FCFSFileBufferType type;
    bool need_free;
    int size;
    char *base;
    char *current;
    char *buff_end;   //base + size
    char *data_end;   //base + data_length, for read
} FCFSPosixFileBuffer;

*/

#define FCFS_CAPI_CONVERT_FP_EX(fp, ...) \
    FCFSPosixCAPIFILE *file; \
    file = (FCFSPosixCAPIFILE *)fp; \
    if (file->magic != FCFS_CAPI_MAGIC_NUMBER) { \
        errno = EBADF; \
        return __VA_ARGS__; \
    }

#define FCFS_CAPI_CONVERT_FP(fp) \
    FCFS_CAPI_CONVERT_FP_EX(fp, -1)

#define FCFS_CAPI_CONVERT_FP_VOID(fp) \
    FCFS_CAPI_CONVERT_FP_EX(fp)

/*
static inline void free_file_buffer(FCFSPosixCAPIFILE *file)
{
    if (file->buffer.base != NULL && file->buffer.need_free) {
        free(file->buffer.base);
        file->buffer.base = NULL;
        file->buffer.need_free = false;
    }
}

static int alloc_file_buffer(FCFSPosixCAPIFILE *file, const int size)
{
    free_file_buffer(file);
    if ((file->buffer.base=fc_malloc(size)) == NULL) {
        return ENOMEM;
    }

    file->buffer.buff_end = file->buffer.base + size;
    file->buffer.current = file->buffer.base;
    file->buffer.size = size;
    file->buffer.need_free = true;
    file->buffer.rw = fcfs_file_buffer_mode_none;
    return 0;
}
*/

static FILE *alloc_file_handle(int fd)
{
    FCFSPosixCAPIFILE *file;
    int result;

    if ((file=fc_malloc(sizeof(FCFSPosixCAPIFILE))) == NULL) {
        errno = ENOMEM;
        return NULL;
    }

    if ((result=init_pthread_lock_cond_pair(&file->lcp)) != 0) {
        free(file);
        errno = result;
        return NULL;
    }

    file->fd = fd;
    file->magic = FCFS_CAPI_MAGIC_NUMBER;
    file->error_no = 0;
    file->buffer.base = NULL;
    file->buffer.size = 0;
    file->buffer.need_free = false;
    file->buffer.out_type = _IOFBF;
    file->buffer.rw = fcfs_file_buffer_mode_none;

    return (FILE *)file;
}

static int free_file_handle(FCFSPosixCAPIFILE *file)
{
    int result;

    //free_file_buffer(file);
    result = fcfs_close(file->fd);
    file->magic = 0;
    destroy_pthread_lock_cond_pair(&file->lcp);
    free(file);

    return result;
}

static int mode_to_flags(const char *mode, int *flags)
{
    const char *p;
    const char *end;
    int len;

    len = strlen(mode);
    if (len == 0) {
        return EINVAL;
    }

    p = mode;
    switch (*p++) {
        case 'r':
            if (*p == '+') {
                *flags = O_RDWR;
                p++;
            } else {
                *flags = O_RDONLY;
            }
            break;
        case 'w':
            if (*p == '+') {
                *flags = O_RDWR | O_CREAT | O_TRUNC;
                p++;
            } else {
                *flags = O_WRONLY | O_CREAT | O_TRUNC;
            }
            break;
        case 'a':
            if (*p == '+') {
                *flags = O_RDWR | O_CREAT | O_APPEND;
                p++;
            } else {
                *flags = O_WRONLY | O_CREAT | O_APPEND;
            }
            break;
        default:
            return EINVAL;
    }

    end = mode + len;
    while (p < end) {
        switch (*p) {
            case 'e':
                *flags |= O_CLOEXEC;
                break;
            case'x':
                *flags |= O_EXCL;
                break;
            default:
                break;
        }
        ++p;
    }

    return 0;
}

static int check_flags(const char *mode, const int flags, int *adding_flags)
{
    const char *p;
    const char *end;
    int rwflags;
    int len;

    len = strlen(mode);
    if (len == 0) {
        return EINVAL;
    }

    *adding_flags = 0;
    p = mode;
    switch (*p++) {
        case 'r':
            if (*p == '+') {
                rwflags = O_RDWR;
                p++;
            } else {
                rwflags = O_RDONLY;
            }
            break;
        case 'a':
            *adding_flags = O_APPEND;
        case 'w':
            if (*p == '+') {
                rwflags = O_RDWR;
                p++;
            } else {
                rwflags = O_WRONLY;
            }

            break;
        default:
            return EINVAL;
    }

    if ((flags & rwflags) != rwflags) {
        return EINVAL;
    }

    end = mode + len;
    while (p < end) {
        switch (*p) {
            case 'e':
                *adding_flags |= O_CLOEXEC;
                break;
            case'x':
                //just ignore
                break;
            default:
                break;
        }
        ++p;
    }

    return 0;
}

static inline int do_fdopen(FCFSPosixAPIContext *ctx,
        const char *path, const char *mode)
{
    int flags;
    int result;

    if ((result=mode_to_flags(mode, &flags)) != 0) {
        errno = result;
        return -1;
    }

    return fcfs_open_ex(ctx, path, flags, 0666);
}

FILE *fcfs_fopen_ex(FCFSPosixAPIContext *ctx,
        const char *path, const char *mode)
{
    int fd;

    if ((fd=do_fdopen(ctx, path, mode)) < 0) {
        return NULL;
    }

    return alloc_file_handle(fd);
}

FILE *fcfs_fdopen_ex(FCFSPosixAPIContext *ctx, int fd, const char *mode)
{
    int flags;
    int adding_flags;
    int result;

    if ((flags=fcfs_fcntl(fd, F_GETFL)) < 0) {
        return NULL;
    }

    if ((result=check_flags(mode, flags, &adding_flags)) != 0) {
        errno = result;
        return NULL;
    }

    if (adding_flags != 0 && (flags & adding_flags) != adding_flags) {
        if (fcfs_fcntl(fd, F_SETFL, (flags | adding_flags)) != 0) {
            return NULL;
        }
    }

    return alloc_file_handle(fd);
}

FILE *fcfs_freopen_ex(FCFSPosixAPIContext *ctx,
        const char *path, const char *mode, FILE *fp)
{
    int fd;
    FCFSPosixAPIFileInfo *finfo;

    FCFS_CAPI_CONVERT_FP_EX(fp, NULL);
    if (path == NULL) {
        if ((finfo=fcfs_get_file_handle(file->fd)) == NULL) {
            errno = EBADF;
            return NULL;
        }

        path = finfo->filename.str;
    }

    if ((fd=do_fdopen(ctx, path, mode)) < 0) {
        return NULL;
    }

    fcfs_close(file->fd);
    file->fd = fd;
    file->error_no = 0;
    return (FILE *)file;
}

int fcfs_fclose(FILE *fp)
{
    FCFS_CAPI_CONVERT_FP_EX(fp, EOF);
    return free_file_handle(file);
}

int fcfs_setvbuf(FILE *fp, char *buf, int mode, size_t size)
{
    switch (mode) {
        case _IOFBF:
            break;
        case _IOLBF:
            break;
        case _IONBF:
            break;
        default:
            break;
    }

    //alloc_file_buffer(FCFSPosixCAPIFILE *file, const int size);
    return 0;
}

int fcfs_fflush(FILE *fp)
{
    FCFS_CAPI_CONVERT_FP_EX(fp, EOF);
    return 0;
}
