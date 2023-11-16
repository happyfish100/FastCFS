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
    volatile int generation;
    volatile int in_realloc;
    volatile int next_fd;
    struct fast_mblock_man file_info_allocator;  //element: FCFSPosixAPIFileInfo
    FCFSPosixFilePtrArray file_parray;
} FCFSFDManagerContext;

static FCFSFDManagerContext fd_manager_ctx;

#define PARRAY_IN_REALLOC fd_manager_ctx.in_realloc
#define PARRAY_GENERATION fd_manager_ctx.generation
#define FINFO_ALLOCATOR   fd_manager_ctx.file_info_allocator
#define PAPI_NEXT_FD      fd_manager_ctx.next_fd
#define FILE_PARRAY       fd_manager_ctx.file_parray

static int file_parray_realloc()
{
    FCFSPosixAPIFileInfo **new_files;
    volatile FCFSPosixAPIFileInfo **old_files;
    int new_count;
    int old_count;
    int bytes;

    if (!__sync_bool_compare_and_swap(&PARRAY_IN_REALLOC, 0, 1)) {
        return 0;
    }

    old_count = FC_ATOMIC_GET(FILE_PARRAY.count);
    new_count = old_count + FILE_INFO_ALLOC_ONCE;
    bytes = sizeof(FCFSPosixAPIFileInfo *) * new_count;
    new_files = (FCFSPosixAPIFileInfo **)fc_malloc(bytes);
    if (new_files == NULL) {
        __sync_bool_compare_and_swap(&PARRAY_IN_REALLOC, 1, 0);
        return ENOMEM;
    }
    memset(new_files, 0, bytes);

    old_files = FC_ATOMIC_GET(FILE_PARRAY.files);
    if (old_count > 0) {
        memcpy(new_files, old_files, old_count *
                sizeof(FCFSPosixAPIFileInfo *));
    }

    __sync_bool_compare_and_swap(&FILE_PARRAY.files, old_files,
            (volatile FCFSPosixAPIFileInfo **)new_files);
    __sync_bool_compare_and_swap(&FILE_PARRAY.count,
            old_count, new_count);
    FC_ATOMIC_INC(PARRAY_GENERATION);
    __sync_bool_compare_and_swap(&PARRAY_IN_REALLOC, 1, 0);

    if (old_files != NULL) {
        usleep(100000);   //delay free
        free(old_files);
    }

    return 0;
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

    return 0;
}

void fcfs_fd_manager_destroy()
{
    fast_mblock_destroy(&FINFO_ALLOCATOR);

    free(FILE_PARRAY.files);
    FILE_PARRAY.count = 0;
}

#define FCFS_FD_TO_INDEX(fd) ((fd - FCFS_POSIX_API_FD_BASE) - 1)

static inline int set_file_info(const int index,
        FCFSPosixAPIFileInfo *old_finfo,
        FCFSPosixAPIFileInfo *new_finfo)
{
    int old_generation;

    do {
        old_generation = FC_ATOMIC_GET(PARRAY_GENERATION);
        if (!__sync_bool_compare_and_swap(&FILE_PARRAY.
                    files[index], old_finfo, new_finfo))
        {
            return EBUSY;
        }
    } while (FC_ATOMIC_GET(PARRAY_IN_REALLOC) || old_generation !=
            FC_ATOMIC_GET(PARRAY_GENERATION));
    return 0;
}

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
    if (set_file_info(FCFS_FD_TO_INDEX(finfo->fd), NULL, finfo) == 0) {
        return finfo;
    } else {
        logError("file: "__FILE__", line: %d, "
                "set file info fail, filename: %s, fd: %d",
                __LINE__, filename, finfo->fd);
        free(finfo->filename.str);
        FC_SET_STRING_NULL(finfo->filename);
        fast_mblock_free_object(&FINFO_ALLOCATOR, finfo);
        return NULL;
    }
}

FCFSPosixAPIFileInfo *fcfs_fd_manager_get(const int fd)
{
    int result;
    int index;
    FCFSPosixAPIFileInfo *finfo;

    index = FCFS_FD_TO_INDEX(fd);
    if (index >= 0 && index < FC_ATOMIC_GET(FILE_PARRAY.count)) {
        finfo = (FCFSPosixAPIFileInfo *)FC_ATOMIC_GET(
                FILE_PARRAY.files[index]);
        result = finfo != NULL ? 0 : ENOENT;
    } else {
        finfo = NULL;
        result = EOVERFLOW;
    }

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
    if (index >= 0 && index < FC_ATOMIC_GET(FILE_PARRAY.count)) {
        if (finfo == FC_ATOMIC_GET(FILE_PARRAY.files[index])) {
            result = set_file_info(index, finfo, NULL);
        } else {
            result = ENOENT;
        }
    } else {
        result = EOVERFLOW;
    }

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
