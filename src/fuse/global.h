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


#ifndef _FCFS_FUSE_GLOBAL_H
#define _FCFS_FUSE_GLOBAL_H

#include "fastcfs/api/fcfs_api.h"
#include "common/fcfs_global.h"

#define FCFS_FUSE_DEFAULT_ATTRIBUTE_TIMEOUT 1.0
#define FCFS_FUSE_DEFAULT_ENTRY_TIMEOUT     1.0

typedef enum {
    allow_none,
    allow_all,
    allow_root
} FUSEAllowOthersMode;

typedef struct {
    FCFSAPINSMountpointHolder nsmp;
    bool singlethread;
    bool clone_fd;
    bool auto_unmount;
    bool xattr_enabled;
    int max_idle_threads;
    double attribute_timeout;
    double entry_timeout;
    FUSEAllowOthersMode allow_others;
    FCFSAPIOwnerInfo owner;
} FUSEGlobalVars;

#ifdef __cplusplus
extern "C" {
#endif

    extern FUSEGlobalVars g_fuse_global_vars;

	int fcfs_fuse_global_init(const char *config_filename);

#ifdef __cplusplus
}
#endif

#endif
