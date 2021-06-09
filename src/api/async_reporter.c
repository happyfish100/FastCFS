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
#include "fastcommon/shared_func.h"
#include "fastcommon/pthread_func.h"
#include "sf/sf_global.h"
#include "sf/sf_func.h"
#include "async_reporter.h"

AsyncReporterContext g_async_reporter_ctx;

#define FCFS_API_CTX  g_async_reporter_ctx.fcfs_api_ctx
#define SORTED_EVENT_PTR_ARRAY g_async_reporter_ctx.event_ptr_arrays.sorted
#define MERGED_EVENT_PTR_ARRAY g_async_reporter_ctx.event_ptr_arrays.merged
#define NOTIFY_EVENT_COUNT g_async_reporter_ctx.event_ptr_arrays.notify_count

static inline int batch_set_dentry_size(const int count)
{
    int result;

    while (1) {
        result = fdir_client_batch_set_dentry_size(FCFS_API_CTX->contexts.
                fdir, &FCFS_API_CTX->ns, g_async_reporter_ctx.dsizes, count);
        if (result == 0 || result == ENOENT || result == EINVAL) {
            break;
        }
        sleep(1);
    }

    return result;
}

static int int_event_ptr_array(FCFSAPIAsyncReportEventPtrArray *event_ptr_array)
{
    event_ptr_array->alloc = 1024;
    event_ptr_array->events = (FCFSAPIAsyncReportEvent **)
        fc_malloc(sizeof(FCFSAPIAsyncReportEvent *) * event_ptr_array->alloc);
    if (event_ptr_array->events == NULL) {
        return ENOMEM;
    }
    return 0;
}

static FCFSAPIAsyncReportEvent **realloc_event_ptrs(
        FCFSAPIAsyncReportEventPtrArray *event_ptr_array, const int count)
{
    int alloc;
    FCFSAPIAsyncReportEvent **events;

    alloc = event_ptr_array->alloc * 2;
    events = (FCFSAPIAsyncReportEvent **)fc_malloc(
            sizeof(FCFSAPIAsyncReportEvent *) * alloc);
    if (events == NULL) {
        return NULL;
    }

    if (count > 0) {
        memcpy(events, event_ptr_array->events,
                sizeof(FCFSAPIAsyncReportEvent *) * count);
    }
    free(event_ptr_array->events);

    event_ptr_array->alloc = alloc;
    event_ptr_array->events = events;
    return events + count;
}

static int to_event_ptr_array(FCFSAPIAsyncReportEvent *head,
        FCFSAPIAsyncReportEventPtrArray *event_ptr_array)
{
    FCFSAPIAsyncReportEvent **event;
    int result;
    int event_count;

    result = 0;
    event_count = 0;
    event = event_ptr_array->events;
    do {
        if (event_count >= event_ptr_array->alloc) {
            if ((event=realloc_event_ptrs(event_ptr_array,
                            event_count)) == NULL)
            {
                result = ENOMEM;
                break;
            }
        }

        head->id = ++event_count;
        *event++ = head;
        head = head->next;
    } while (head != NULL);

    event_ptr_array->count = event_count;
    return result;
}

static int compare_event_ptr(const FCFSAPIAsyncReportEvent **t1,
        const FCFSAPIAsyncReportEvent **t2)
{
    int sub;
    if ((sub=fc_compare_int64((*t1)->dsize.inode, (*t2)->dsize.inode)) != 0) {
        return sub;
    }

    return (int)(*t1)->id - (int)(*t2)->id;
}

static inline void merge_event(FCFSAPIAsyncReportEvent *dest,
        const FCFSAPIAsyncReportEvent *src)
{
    if ((src->dsize.flags & (FDIR_DENTRY_FIELD_MODIFIED_FLAG_FILE_SIZE |
                FDIR_DENTRY_FIELD_MODIFIED_FLAG_SPACE_END)) != 0)
    {
        if ((dest->dsize.flags & (FDIR_DENTRY_FIELD_MODIFIED_FLAG_FILE_SIZE |
                        FDIR_DENTRY_FIELD_MODIFIED_FLAG_SPACE_END)) == 0)
        {
            dest->dsize.file_size = src->dsize.file_size;
        } else if (dest->dsize.file_size < src->dsize.file_size) {
            dest->dsize.file_size = src->dsize.file_size;
        }
    }

    if ((src->dsize.flags & FDIR_DENTRY_FIELD_MODIFIED_FLAG_INC_ALLOC) != 0) {
        dest->dsize.inc_alloc += src->dsize.inc_alloc;
    }

    dest->dsize.flags |= src->dsize.flags;
}

static int merge_events(FCFSAPIAsyncReportEvent *head)
{
    int result;
    //int merge_count;
    FCFSAPIAsyncReportEvent **ts;
    FCFSAPIAsyncReportEvent **send;
    FCFSAPIAsyncReportEvent **merge;
    FCFSAPIAsyncReportEvent *current;

    if ((result=to_event_ptr_array(head, &SORTED_EVENT_PTR_ARRAY)) != 0) {
        return result;
    }

    if (SORTED_EVENT_PTR_ARRAY.count > 1) {
        qsort(SORTED_EVENT_PTR_ARRAY.events, SORTED_EVENT_PTR_ARRAY.count,
                sizeof(FCFSAPIAsyncReportEvent *), (int (*)(const void *,
                        const void *))compare_event_ptr);
    }

    NOTIFY_EVENT_COUNT = 0;
    send = SORTED_EVENT_PTR_ARRAY.events + SORTED_EVENT_PTR_ARRAY.count;
    ts = SORTED_EVENT_PTR_ARRAY.events;
    merge = MERGED_EVENT_PTR_ARRAY.events;
    while (ts < send) {
        if ((*ts)->type == fcfs_api_event_type_notify) {
            NOTIFY_EVENT_COUNT++;
            ts++;
            continue;
        }

        if (merge - MERGED_EVENT_PTR_ARRAY.events >=
                MERGED_EVENT_PTR_ARRAY.alloc)
        {
            if ((merge=realloc_event_ptrs(&MERGED_EVENT_PTR_ARRAY,
                            merge - MERGED_EVENT_PTR_ARRAY.events)) == NULL)
            {
                result = ENOMEM;
                break;
            }
        }

        //merge_count = 1;
        current = *ts;
        *merge++ = *ts++;
        if (current->dsize.force) {
            continue;
        }
        if (ts == send) {
            break;
        }

        while ((ts < send) && (!(*ts)->dsize.force) &&
                ((*ts)->dsize.inode == current->dsize.inode))
        {
            if ((*ts)->type == fcfs_api_event_type_notify) {
                NOTIFY_EVENT_COUNT++;
            } else {
                //merge_count++;
                merge_event(current, *ts);
            }
            ts++;
        }

        /*
        logInfo("event oid: %"PRId64", merged count: %d",
                current->dsize.inode, merge_count);
         */
    }

    MERGED_EVENT_PTR_ARRAY.count = merge - MERGED_EVENT_PTR_ARRAY.events;
    return 0;
}

static inline void notify_waiting_tasks(FCFSAPIAsyncReportEvent *event)
{
    FCFSAPIWaitingTask *task;

    if (event->type == fcfs_api_event_type_report) {
        PTHREAD_MUTEX_LOCK(&event->inode_hentry->hentry.sharding->lock);
    }
    while (event->waitings.head != NULL) {
        task = event->waitings.head;
        event->waitings.head = event->waitings.head->next;

        fcfs_api_notify_waiting_task(task);
    }
    if (event->type == fcfs_api_event_type_report) {
        fc_list_del_init(&event->dlink);
        PTHREAD_MUTEX_UNLOCK(&event->inode_hentry->hentry.sharding->lock);
    }
}

static inline void notify_waiting_tasks_and_free_events(
        FCFSAPIAsyncReportEvent *head)
{
    FCFSAPIAsyncReportEvent *event;
    do {
        event = head;
        head = head->next;

        notify_waiting_tasks(event);
        fast_mblock_free_object(event->allocator, event);
    } while (head != NULL);
}

static inline int deal_events(FCFSAPIAsyncReportEvent *head)
{
    FCFSAPIAsyncReportEvent **event;
    FCFSAPIAsyncReportEvent **tend;
    FDIRSetDEntrySizeInfo *dsize;
    int result;
    int current_count;

    if ((result=merge_events(head)) != 0) {
        notify_waiting_tasks_and_free_events(head);
        return result;
    }

    dsize = g_async_reporter_ctx.dsizes;
    tend = MERGED_EVENT_PTR_ARRAY.events + MERGED_EVENT_PTR_ARRAY.count;
    for (event=MERGED_EVENT_PTR_ARRAY.events; event<tend; event++) {
        *dsize++ = (*event)->dsize;
        if ((current_count=dsize - g_async_reporter_ctx.dsizes) ==
                FDIR_BATCH_SET_MAX_DENTRY_COUNT)
        {
            batch_set_dentry_size(current_count);
            dsize = g_async_reporter_ctx.dsizes;
        }
    }

    if ((current_count=dsize - g_async_reporter_ctx.dsizes) > 0) {
        batch_set_dentry_size(current_count);
    }

    __sync_fetch_and_sub(&g_async_reporter_ctx.waiting_count,
            SORTED_EVENT_PTR_ARRAY.count - NOTIFY_EVENT_COUNT);
    notify_waiting_tasks_and_free_events(head);

    /*
    logInfo("total (input) event count: %d, report (output) count: %d, "
            "notify event count: %d", SORTED_EVENT_PTR_ARRAY.count,
            MERGED_EVENT_PTR_ARRAY.count, NOTIFY_EVENT_COUNT);
            */
    return 0;
}

void async_reporter_terminate()
{
    FCFSAPIAsyncReportEvent *head;
    int count;

    if (!g_async_reporter_ctx.fcfs_api_ctx->async_report.enabled) {
        return;
    }

    head = (FCFSAPIAsyncReportEvent *)fc_queue_try_pop_all(
            &g_async_reporter_ctx.queue);
    if (head != NULL) {
        count = deal_events(head);
    } else {
        count = 0;
    }

    logInfo("file: "__FILE__", line: %d, "
            "async_reporter_terminate, remain count: %d",
            __LINE__, count);
}

static inline void check_and_set_stage()
{
    int old_stage;

    if (__sync_bool_compare_and_swap(&g_async_reporter_ctx.stage,
                ASYNC_REPORTER_STAGE_DEALING, ASYNC_REPORTER_STAGE_SLEEPING))
    {
        fc_timedwait_ms(&g_async_reporter_ctx.lcp.lock,
                &g_async_reporter_ctx.lcp.cond, g_async_reporter_ctx.
                fcfs_api_ctx->async_report.interval_ms);
        if (__sync_bool_compare_and_swap(
                    &g_async_reporter_ctx.stage,
                    ASYNC_REPORTER_STAGE_SLEEPING,
                    ASYNC_REPORTER_STAGE_DEALING))
        {
            return;
        }
    }

    old_stage = __sync_fetch_and_add(&g_async_reporter_ctx.stage, 0);
    __sync_bool_compare_and_swap(&g_async_reporter_ctx.stage,
            old_stage, ASYNC_REPORTER_STAGE_DEALING);
}

static void *async_reporter_thread_func(void *arg)
{
    FCFSAPIAsyncReportEvent *head;

#ifdef OS_LINUX
    prctl(PR_SET_NAME, "dir-async-reporter");
#endif

    while (SF_G_CONTINUE_FLAG) {
        head = (FCFSAPIAsyncReportEvent *)fc_queue_pop_all(
                &g_async_reporter_ctx.queue);
        if (head != NULL) {
            deal_events(head);
        }

        if (g_async_reporter_ctx.fcfs_api_ctx->async_report.interval_ms > 0) {
            check_and_set_stage();
        }
    }

    return NULL;
}

int async_reporter_init(FCFSAPIContext *fcfs_api_ctx)
{
    int result;
    pthread_t tid;

    g_async_reporter_ctx.fcfs_api_ctx = fcfs_api_ctx;
    g_async_reporter_ctx.dsizes = (FDIRSetDEntrySizeInfo *)
        fc_malloc(sizeof(FDIRSetDEntrySizeInfo) *
                FDIR_BATCH_SET_MAX_DENTRY_COUNT);
    if (g_async_reporter_ctx.dsizes == NULL) {
        return ENOMEM;
    }

    if ((result=int_event_ptr_array(&SORTED_EVENT_PTR_ARRAY)) != 0) {
        return result;
    }
    if ((result=int_event_ptr_array(&MERGED_EVENT_PTR_ARRAY)) != 0) {
        return result;
    }

    if ((result=init_pthread_lock_cond_pair(&g_async_reporter_ctx.lcp)) != 0) {
        return result;
    }

    if ((result=fc_queue_init(&g_async_reporter_ctx.queue, (long)
                    (&((FCFSAPIAsyncReportEvent *)NULL)->next))) != 0)
    {
        return result;
    }

    g_async_reporter_ctx.stage = ASYNC_REPORTER_STAGE_DEALING;
    return fc_create_thread(&tid, async_reporter_thread_func, NULL,
            SF_G_THREAD_STACK_SIZE);
}
