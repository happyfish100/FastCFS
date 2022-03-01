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

#include "fastcommon/fc_list.h"
#include "fastcommon/pthread_func.h"
#include "../fcfs_api.h"

#ifndef OS_LINUX
typedef off_t off64_t;
#endif

#define FCFS_POSIX_API_FD_BASE  (2 << 28)

struct dirent;
typedef int (*fcfs_dir_filter_func)(const struct dirent *ent);
typedef int (*fcfs_dir_compare_func)(const struct dirent **ent1,
        const struct dirent **ent2);

typedef struct fcfs_posix_api_context {
    FCFSAPINSMountpointHolder nsmp;
    string_t mountpoint;
    FCFSAPIOwnerInfo owner;
    FCFSAPIContext api_ctx;
} FCFSPosixAPIContext;

typedef enum {
    fcfs_papi_tpid_type_tid,
    fcfs_papi_tpid_type_pid
} FCFSPosixAPITPIDType;

typedef struct fcfs_posix_api_file_info {
    string_t filename;
    int fd;
    FCFSPosixAPITPIDType tpid_type;  //use pid or tid
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

typedef struct fcfs_posix_capi_file {
    int magic;
    int fd;
    int error_no;
    int eof;
    pthread_mutex_t lock;
    struct fc_list_head dlink;  //for opened files chain
} FCFSPosixCAPIFILE;

typedef struct fcfs_posix_api_global_vars {
    FCFSPosixAPIContext ctx;
    string_t *cwd;
} FCFSPosixAPIGlobalVars;

#define FCFS_PAPI_IS_MY_FD(fd) \
    (fd > FCFS_POSIX_API_FD_BASE)

#endif
