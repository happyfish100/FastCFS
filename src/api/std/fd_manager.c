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

#include "fastcommon/fast_mblock.h"
#include "fastcommon/shared_func.h"
#include "fastcommon/logger.h"
#include "fd_manager.h"

#define FILE_INFO_ALLOC_ONCE  1024

typedef struct fcfs_fd_manager_context {
    volatile int next_fd;
    struct fast_mblock_man file_info_allocator;  //element: FCFSPosixAPIFileInfo
    pthread_mutex_t lock;
    FCFSPosixFilePtrArray file_parray;
} FCFSFDManagerContext;

static FCFSFDManagerContext fd_manager_ctx;

#define FINFO_ALLOCATOR fd_manager_ctx.file_info_allocator
#define PAPI_NEXT_FD    fd_manager_ctx.next_fd
#define PAPI_LOCK       fd_manager_ctx.lock
#define FILE_PARRAY     fd_manager_ctx.file_parray


static int file_parray_realloc()
{
    FCFSPosixAPIFileInfo **files;
    int count;
    int bytes;
    int result;

    PTHREAD_MUTEX_LOCK(&PAPI_LOCK);
    do {
        count = FILE_PARRAY.count + FILE_INFO_ALLOC_ONCE;
        bytes = sizeof(FCFSPosixAPIFileInfo *) * count;
        files = (FCFSPosixAPIFileInfo **)fc_malloc(bytes);
        if (files == NULL) {
            result = ENOMEM;
            break;
        }

        memset(files, 0, bytes);
        if (FILE_PARRAY.count > 0) {
            memcpy(files, FILE_PARRAY.files, FILE_PARRAY.count *
                    sizeof(FCFSPosixAPIFileInfo *));
            free(FILE_PARRAY.files);
        }

        FILE_PARRAY.files = files;
        FILE_PARRAY.count = count;
        result = 0;
    } while (0);
    PTHREAD_MUTEX_UNLOCK(&PAPI_LOCK);

    return result;
}

static int finfo_init_func(FCFSPosixAPIFileInfo *finfo, void *args)
{
    int elt_count;

    finfo->fd = __sync_add_and_fetch(&PAPI_NEXT_FD, 1);
    elt_count = finfo->fd - FCFS_POSIX_API_FD_BASE;
    if (elt_count <= FILE_PARRAY.count) {
        return 0;
    } else {
        return file_parray_realloc();
    }
}

int fcfs_fd_manager_init()
{
    int result;

    __sync_add_and_fetch(&PAPI_NEXT_FD, FCFS_POSIX_API_FD_BASE);
    if ((result=fast_mblock_init_ex1(&FINFO_ALLOCATOR, "papi_file_info",
                    sizeof(FCFSPosixAPIFileInfo), FILE_INFO_ALLOC_ONCE, 0,
                    (fast_mblock_object_init_func)finfo_init_func,
                    NULL, true)) != 0)
    {
        return result;
    }

    if ((result=init_pthread_lock(&PAPI_LOCK)) != 0) {
        return result;
    }

    return 0;
}

void fcfs_fd_manager_destroy()
{
    fast_mblock_destroy(&FINFO_ALLOCATOR);
    pthread_mutex_destroy(&PAPI_LOCK);

    free(FILE_PARRAY.files);
    FILE_PARRAY.count = 0;
}

#define FCFS_FD_TO_INDEX(fd) ((fd - FCFS_POSIX_API_FD_BASE) - 1)

FCFSPosixAPIFileInfo *fcfs_fd_manager_alloc(const char *filename)
{
    FCFSPosixAPIFileInfo *finfo;

    if ((finfo=fast_mblock_alloc_object(&FINFO_ALLOCATOR)) == NULL) {
        return finfo;
    }

    finfo->filename.len = strlen(filename);
    finfo->filename.str = (char *)fc_malloc(finfo->filename.len + 2);
    if (finfo->filename.str == NULL) {
        fast_mblock_free_object(&FINFO_ALLOCATOR, finfo);
        return NULL;
    }
    memcpy(finfo->filename.str, filename, finfo->filename.len + 1);

    PTHREAD_MUTEX_LOCK(&PAPI_LOCK);
    FILE_PARRAY.files[FCFS_FD_TO_INDEX(finfo->fd)] = finfo;
    PTHREAD_MUTEX_UNLOCK(&PAPI_LOCK);
    return finfo;
}

FCFSPosixAPIFileInfo *fcfs_fd_manager_get(const int fd)
{
    int result;
    int index;
    FCFSPosixAPIFileInfo *finfo;

    index = FCFS_FD_TO_INDEX(fd);

    PTHREAD_MUTEX_LOCK(&PAPI_LOCK);
    if (index >= 0 && index < FILE_PARRAY.count) {
        finfo = FILE_PARRAY.files[index];
        result = finfo != NULL ? 0 : ENOENT;
    } else {
        finfo = NULL;
        result = EOVERFLOW;
    }
    PTHREAD_MUTEX_UNLOCK(&PAPI_LOCK);

    if (result != 0) {
        logError("file: "__FILE__", line: %d, "
                "get file info fail, fd: %d, errno: %d, error info: %s",
                __LINE__, fd, result, STRERROR(result));
    }
    return finfo;
}

int fcfs_fd_manager_free(FCFSPosixAPIFileInfo *finfo)
{
    int result;
    int index;

    index = FCFS_FD_TO_INDEX(finfo->fd);

    PTHREAD_MUTEX_LOCK(&PAPI_LOCK);
    if (index >= 0 && index < FILE_PARRAY.count) {
        if (finfo == FILE_PARRAY.files[index]) {
            FILE_PARRAY.files[index] = NULL;
            result = 0;
        } else {
            result = ENOENT;
        }
    } else {
        result = EOVERFLOW;
    }
    PTHREAD_MUTEX_UNLOCK(&PAPI_LOCK);

    if (result == 0) {
        free(finfo->filename.str);
        FC_SET_STRING_NULL(finfo->filename);
        fast_mblock_free_object(&FINFO_ALLOCATOR, finfo);
    } else {
        logError("file: "__FILE__", line: %d, "
                "free file info fail, fd: %d, errno: %d, error info: %s",
                __LINE__, finfo->fd, result, STRERROR(result));
    }
    return result;
}
