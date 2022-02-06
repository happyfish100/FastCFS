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


#ifndef _FCFS_FD_MANAGER_H
#define _FCFS_FD_MANAGER_H

#include "api_types.h"

#ifdef __cplusplus
extern "C" {
#endif

    int fcfs_fd_manager_init();

    void fcfs_fd_manager_destroy();

    FCFSPosixAPIFileInfo *fcfs_fd_manager_alloc(const char *filename);

    FCFSPosixAPIFileInfo *fcfs_fd_manager_get(const int fd);

    static inline void fcfs_fd_manager_normalize_path(
            FCFSPosixAPIFileInfo *finfo)
    {
        if (!(finfo->filename.len > 0 &&  finfo->filename.str
                    [finfo->filename.len - 1] == '/'))
        {
            *(finfo->filename.str + finfo->filename.len++) = '/';
            *(finfo->filename.str + finfo->filename.len) = '\0';
        }
    }

    int fcfs_fd_manager_free(FCFSPosixAPIFileInfo *finfo);

#ifdef __cplusplus
}
#endif

#endif
