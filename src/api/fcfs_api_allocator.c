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

#include <stdlib.h>
#include "fcfs_api_allocator.h"

FCFSAPIAllocatorCtxArray g_fcfs_api_allocator_array;

static int async_report_event_alloc_init(FCFSAPIAsyncReportEvent *event,
        struct fast_mblock_man *allocator)
{
    event->allocator = allocator;
    return 0;
}

static int init_allocator_context(FCFSAPIAllocatorContext *ctx)
{
    int result;
    if ((result=fast_mblock_init_ex1(&ctx->async_report_event,
                    "async_report_event", sizeof(FCFSAPIAsyncReportEvent),
                    4096, 0, (fast_mblock_alloc_init_func)
                    async_report_event_alloc_init,
                    &ctx->async_report_event, true)) != 0)
    {
        return result;
    }

    return 0;
}

int fcfs_api_allocator_init(FCFSAPIContext *api_ctx)
{
    int result;
    int bytes;
    FCFSAPIAllocatorContext *ctx;
    FCFSAPIAllocatorContext *end;

    g_fcfs_api_allocator_array.count = api_ctx->shared_allocator_count;
    bytes = sizeof(FCFSAPIAllocatorContext) * g_fcfs_api_allocator_array.count;
    g_fcfs_api_allocator_array.allocators = (FCFSAPIAllocatorContext *)
        fc_malloc(bytes);
    if (g_fcfs_api_allocator_array.allocators == NULL) {
        return ENOMEM;
    }

    end = g_fcfs_api_allocator_array.allocators +
        g_fcfs_api_allocator_array.count;
    for (ctx=g_fcfs_api_allocator_array.allocators; ctx<end; ctx++) {
        if ((result=init_allocator_context(ctx)) != 0) {
            return result;
        }
    }

    return 0;
}
