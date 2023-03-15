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
    bool read_only;
    bool xattr_enabled;
    bool writeback_cache;
    bool kernel_cache;
    bool groups_enabled;
    struct {
        bool enabled;
        int timeout;
        int sharding_count;
        int htable_capacity;
        int allocator_count;
        int element_limit;
    } groups_cache;
    int max_idle_threads;
    int max_threads;      //libfuse >= 3.12
    double attribute_timeout;
    double entry_timeout;
    FUSEAllowOthersMode allow_others;
    Version kernel_version;
} FUSEGlobalVars;

#define OS_KERNEL_VERSION g_fuse_global_vars.kernel_version

#define ADDITIONAL_GROUPS_ENABLED    g_fuse_global_vars.groups_enabled
#define GROUPS_CACHE_ENABLED         g_fuse_global_vars.groups_cache.enabled
#define GROUPS_CACHE_TIMEOUT         g_fuse_global_vars.groups_cache.timeout
#define GROUPS_CACHE_SHARDING_COUNT  g_fuse_global_vars.groups_cache.sharding_count
#define GROUPS_CACHE_HTABLE_CAPACITY g_fuse_global_vars.groups_cache.htable_capacity
#define GROUPS_CACHE_ALLOCATOR_COUNT g_fuse_global_vars.groups_cache.allocator_count
#define GROUPS_CACHE_ELEMENT_LIMIT   g_fuse_global_vars.groups_cache.element_limit

#ifdef __cplusplus
extern "C" {
#endif

    extern FUSEGlobalVars g_fuse_global_vars;

	int fcfs_fuse_global_init(const char *config_filename);

#ifdef __cplusplus
}
#endif

#endif
