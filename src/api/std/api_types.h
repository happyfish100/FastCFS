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


#ifndef _FCFS_POSIX_API_TYPES_H
#define _FCFS_POSIX_API_TYPES_H

#include "../fcfs_api.h"

#define FCFS_POSIX_API_FD_BASE  (2 << 28)

typedef struct fcfs_posix_api_context {
    string_t mountpoint;
    FCFSAPIOwnerInfo owner;
    FCFSAPIContext api_ctx;
} FCFSPosixAPIContext;

typedef struct fcfs_posix_api_file_info {
    string_t filename;
    int fd;
    FCFSAPIFileInfo fi;
} FCFSPosixAPIFileInfo;

typedef struct fcfs_posix_file_ptr_array {
    FCFSPosixAPIFileInfo **files;
    int count;
} FCFSPosixFilePtrArray;

typedef struct fcfs_posix_api_dir {
    FCFSPosixAPIFileInfo *file;
    FDIRClientCompactDentryArray darray;
    int magic;
    int offset;
} FCFSPosixAPIDIR;

#define FCFS_PAPI_IS_MY_FD(fd) \
    (fd > FCFS_POSIX_API_FD_BASE)

#endif
