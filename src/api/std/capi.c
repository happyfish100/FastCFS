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
    file->buffer.base = NULL;
    file->buffer.size = 0;
    file->buffer.need_free = false;
    file->buffer.out_type = _IOFBF;
    file->buffer.rw = fcfs_file_buffer_mode_none;

    return (FILE *)file;
}

static void free_file_handle(FCFSPosixCAPIFILE *file)
{
    //free_file_buffer(file);
    file->magic = 0;
    destroy_pthread_lock_cond_pair(&file->lcp);
    free(file);
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
        if (*p == 'e') {
            *flags |= O_CLOEXEC;
        } else if (*p == 'x') {
            *flags |= EEXIST;
        }
        ++p;
    }

    return 0;
}

FILE *fcfs_fopen_ex(FCFSPosixAPIContext *ctx,
        const char *path, const char *mode)
{
    int flags;
    int fd;
    int result;

    if ((result=mode_to_flags(mode, &flags)) != 0) {
        errno = result;
        return NULL;
    }

    if ((fd=fcfs_open_ex(ctx, path, flags, 0666)) < 0) {
        return NULL;
    }

    return alloc_file_handle(fd);
}

int fcfs_fclose(FILE *fp)
{
    FCFS_CAPI_CONVERT_FP_EX(fp, EOF);
    free_file_handle(file);
    return 0;
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
