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

#ifndef _FCFS_API_ASYNC_REPORTER_H
#define _FCFS_API_ASYNC_REPORTER_H

#include "fastcommon/fc_queue.h"
#include "fastcommon/fc_atomic.h"
#include "fcfs_api_types.h"
#include "fcfs_api_allocator.h"
#include "inode_htable.h"

#define ASYNC_REPORTER_STAGE_DEALING   0
#define ASYNC_REPORTER_STAGE_SLEEPING  1
#define ASYNC_REPORTER_STAGE_KEEPING   2

typedef struct {
    FCFSAPIContext *fcfs_api_ctx;
    struct fc_queue queue;
    volatile int waiting_count;
    volatile int stage;
    pthread_lock_cond_pair_t lcp;  //for timed wait
    FDIRSetDEntrySizeInfo *dsizes;
    struct {
        int notify_count;   //notify event count
        FCFSAPIAsyncReportEventPtrArray sorted;  //for sort
        FCFSAPIAsyncReportEventPtrArray merged;
    } event_ptr_arrays;
} AsyncReporterContext;

#ifdef __cplusplus
extern "C" {
#endif

    extern AsyncReporterContext g_async_reporter_ctx;

    int async_reporter_init(FCFSAPIContext *fcfs_api_ctx);

    void async_reporter_terminate();

    static inline int async_reporter_push(const FDIRSetDEntrySizeInfo *dsize)
    {
        int result;
        FCFSAPIAsyncReportEvent *event;
        FCFSAPIAllocatorContext *allocator_ctx;

        allocator_ctx = fcfs_api_allocator_get(dsize->inode);
        event = (FCFSAPIAsyncReportEvent *)fast_mblock_alloc_object(
                &allocator_ctx->async_report_event);
        if (event == NULL) {
            return ENOMEM;
        }

        event->type = fcfs_api_event_type_report;
        event->dsize = *dsize;
        result = inode_htable_insert(event);
        __sync_fetch_and_add(&g_async_reporter_ctx.waiting_count, 1);
        fc_queue_push(&g_async_reporter_ctx.queue, event);
        return result;
    }

    static inline int async_reporter_push_notify(const uint64_t inode,
            FCFSAPIWaitingTask **waiting_task)
    {
        FCFSAPIAllocatorContext *allocator_ctx;
        FCFSAPIAsyncReportEvent *event;

        allocator_ctx = fcfs_api_allocator_get(inode);
        event = (FCFSAPIAsyncReportEvent *)fast_mblock_alloc_object(
                &allocator_ctx->async_report_event);
        if (event == NULL) {
            return ENOMEM;
        }

        *waiting_task = (FCFSAPIWaitingTask *)fast_mblock_alloc_object(
                &allocator_ctx->waiting_task);
        if (*waiting_task == NULL) {
            return ENOMEM;
        }

        (*waiting_task)->next = event->waitings.head;
        event->waitings.head = *waiting_task;
        event->type = fcfs_api_event_type_notify;
        event->dsize.inode = inode;
        event->dsize.flags = 0;
        event->dsize.force = false;
        fc_queue_push(&g_async_reporter_ctx.queue, event);
        return 0;
    }

    static inline void async_reporter_notify()
    {
        if (g_async_reporter_ctx.fcfs_api_ctx->async_report.interval_ms <= 0) {
            return;
        }

        switch (__sync_fetch_and_add(&g_async_reporter_ctx.stage, 0)) {
            case ASYNC_REPORTER_STAGE_DEALING:
                __sync_bool_compare_and_swap(
                        &g_async_reporter_ctx.stage,
                        ASYNC_REPORTER_STAGE_DEALING,
                        ASYNC_REPORTER_STAGE_KEEPING);
                break;
            case ASYNC_REPORTER_STAGE_SLEEPING:
                pthread_cond_signal(&g_async_reporter_ctx.lcp.cond);
                break;
            case ASYNC_REPORTER_STAGE_KEEPING:
                //do nothing
                break;
            default:
                break;
        }
    }

    static inline int async_reporter_wait_all(const uint64_t inode)
    {
        FCFSAPIWaitingTask *waiting_task;
        int result;

        if (FC_ATOMIC_GET(g_async_reporter_ctx.waiting_count) == 0) {
            return 0;
        }

        if ((result=async_reporter_push_notify(inode, &waiting_task)) != 0) {
            return result;
        }
        async_reporter_notify();

        fcfs_api_wait_report_done_and_release(waiting_task);
        return 0;
    }

#ifdef __cplusplus
}
#endif

#endif
