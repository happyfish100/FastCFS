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

#ifndef _FCFS_API_ALLOCATOR_H
#define _FCFS_API_ALLOCATOR_H

#include "fastcommon/pthread_func.h"
#include "fcfs_api_types.h"

typedef struct fcfs_api_allocator_context {
    struct fast_mblock_man async_report_event; //element: FCFSAPIAsyncReportEvent
    struct fast_mblock_man waiting_task;       //element: FCFSAPIWaitingTask
} FCFSAPIAllocatorContext;

typedef struct fcfs_api_allocator_ctx_array {
    int count;
    FCFSAPIAllocatorContext *allocators;
} FCFSAPIAllocatorCtxArray;

#ifdef __cplusplus
extern "C" {
#endif

    extern FCFSAPIAllocatorCtxArray g_fcfs_api_allocator_array;

    int fcfs_api_allocator_init(FCFSAPIContext *api_ctx);

    static inline FCFSAPIAllocatorContext *fcfs_api_allocator_get(
            const uint64_t id)
    {
        return g_fcfs_api_allocator_array.allocators +
            id % g_fcfs_api_allocator_array.count;
    }

#ifdef __cplusplus
}
#endif

#endif
