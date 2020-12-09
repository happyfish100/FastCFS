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
#define SORTED_TASK_PTR_ARRAY g_async_reporter_ctx.task_ptr_arrays.sorted
#define MERGED_TASK_PTR_ARRAY g_async_reporter_ctx.task_ptr_arrays.merged

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

static int int_task_ptr_array(FCFSAPIAsyncReportTaskPtrArray *task_ptr_array)
{
    task_ptr_array->alloc = 4;  //TODO
    task_ptr_array->tasks = (FCFSAPIAsyncReportTask **)
        fc_malloc(sizeof(FCFSAPIAsyncReportTask *) * task_ptr_array->alloc);
    if (task_ptr_array->tasks == NULL) {
        return ENOMEM;
    }
    return 0;
}

static int realloc_task_ptrs(FCFSAPIAsyncReportTaskPtrArray *task_ptr_array)
{
    int alloc;
    FCFSAPIAsyncReportTask **tasks;

    alloc = task_ptr_array->alloc * 2;
    tasks = (FCFSAPIAsyncReportTask **)fc_malloc(
            sizeof(FCFSAPIAsyncReportTask *) * alloc);
    if (tasks == NULL) {
        return ENOMEM;
    }

    if (task_ptr_array->count > 0) {
        memcpy(tasks, task_ptr_array->tasks,
                sizeof(FCFSAPIAsyncReportTask *) *
                task_ptr_array->count);
    }
    free(task_ptr_array->tasks);

    task_ptr_array->alloc = alloc;
    task_ptr_array->tasks = tasks;
    return 0;
}

static int to_task_ptr_array(FCFSAPIAsyncReportTask *head,
        FCFSAPIAsyncReportTaskPtrArray *task_ptr_array)
{
    FCFSAPIAsyncReportTask **task;
    int result;
    int task_count;

    result = 0;
    task_count = 0;
    task = task_ptr_array->tasks;
    do {
        if (task_count >= task_ptr_array->alloc) {
            if ((result=realloc_task_ptrs(task_ptr_array)) != 0) {
                break;
            }
            task = task_ptr_array->tasks + task_ptr_array->count;
        }

        head->id = ++task_count;
        *task++ = head;
        head = head->next;
    } while (head != NULL);

    task_ptr_array->count = task - task_ptr_array->tasks;
    logInfo("task count: %d", task_ptr_array->count);
    return result;
}

static int compare_task_ptr(const FCFSAPIAsyncReportTask **t1,
        const FCFSAPIAsyncReportTask **t2)
{
    int sub;
    if ((sub=fc_compare_int64((*t1)->dsize.inode, (*t2)->dsize.inode)) != 0) {
        return sub;
    }

    return (*t1)->id - (*t2)->id;
}

static inline void merge_task(FCFSAPIAsyncReportTask *dest,
        const FCFSAPIAsyncReportTask *src)
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

static int merge_tasks(FCFSAPIAsyncReportTask *head)
{
    int result;
    FCFSAPIAsyncReportTask **ts;
    FCFSAPIAsyncReportTask **send;
    FCFSAPIAsyncReportTask **merge;
    FCFSAPIAsyncReportTask *current;

    if ((result=to_task_ptr_array(head, &SORTED_TASK_PTR_ARRAY)) != 0) {
        return result;
    }

    if (SORTED_TASK_PTR_ARRAY.count > 1) {
        qsort(SORTED_TASK_PTR_ARRAY.tasks, SORTED_TASK_PTR_ARRAY.count,
                sizeof(FCFSAPIAsyncReportTask *), (int (*)(const void *,
                        const void *))compare_task_ptr);
    }

    send = SORTED_TASK_PTR_ARRAY.tasks + SORTED_TASK_PTR_ARRAY.count;
    ts = SORTED_TASK_PTR_ARRAY.tasks;
    merge = MERGED_TASK_PTR_ARRAY.tasks;
    while (ts < send) {
        if (merge - MERGED_TASK_PTR_ARRAY.tasks >=
                MERGED_TASK_PTR_ARRAY.alloc)
        {
            if ((result=realloc_task_ptrs(&MERGED_TASK_PTR_ARRAY)) != 0) {
                break;
            }
            merge = MERGED_TASK_PTR_ARRAY.tasks + MERGED_TASK_PTR_ARRAY.count;
        }

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
            merge_task(current, *ts);
            ts++;
        }
    }

    MERGED_TASK_PTR_ARRAY.count = merge - MERGED_TASK_PTR_ARRAY.tasks;
    return 0;
}

static inline void free_tasks(FCFSAPIAsyncReportTask *head)
{
    FCFSAPIAsyncReportTask *task;
    do {
        task = head;
        head = head->next;

        fast_mblock_free_object(task->allocator, task);
    } while (head != NULL);
}

static inline int deal_tasks(FCFSAPIAsyncReportTask *head)
{
    FCFSAPIAsyncReportTask **task;
    FCFSAPIAsyncReportTask **tend;
    FDIRSetDEntrySizeInfo *dsize;
    int result;
    int current_count;

    if ((result=merge_tasks(head)) != 0) {
        free_tasks(head);
        return result;
    }

    dsize = g_async_reporter_ctx.dsizes;
    tend = MERGED_TASK_PTR_ARRAY.tasks + MERGED_TASK_PTR_ARRAY.count;
    for (task=MERGED_TASK_PTR_ARRAY.tasks; task<tend; task++) {
        *dsize++ = (*task)->dsize;
        if ((current_count=dsize - g_async_reporter_ctx.dsizes) ==
                FDIR_CLIENT_BATCH_SET_DENTRY_MAX_COUNT)
        {
            batch_set_dentry_size(current_count);
            dsize = g_async_reporter_ctx.dsizes;
        }
    }

    if ((current_count=dsize - g_async_reporter_ctx.dsizes) > 0) {
        batch_set_dentry_size(current_count);
    }

    free_tasks(head);
    logInfo("total task count: %d, report (merged) count: %d",
            SORTED_TASK_PTR_ARRAY.count, MERGED_TASK_PTR_ARRAY.count);
    return 0;
}

void async_reporter_terminate()
{
    FCFSAPIAsyncReportTask *head;
    int count;

    if (!g_async_reporter_ctx.fcfs_api_ctx->async_report_enabled) {
        return;
    }

    head = (FCFSAPIAsyncReportTask *)fc_queue_try_pop_all(
            &g_async_reporter_ctx.queue);
    if (head != NULL) {
        count = deal_tasks(head);
    } else {
        count = 0;
    }

    logInfo("file: "__FILE__", line: %d, "
            "async_reporter_terminate, remain count: %d",
            __LINE__, count);
}

static void *async_reporter_thread_func(void *arg)
{
    FCFSAPIAsyncReportTask *head;

    while (SF_G_CONTINUE_FLAG) {
        head = (FCFSAPIAsyncReportTask *)fc_queue_pop_all(
                &g_async_reporter_ctx.queue);
        if (head != NULL) {
            deal_tasks(head);
        }
        if (g_async_reporter_ctx.fcfs_api_ctx->async_report_interval_ms > 0) {
            fc_sleep_ms(g_async_reporter_ctx.fcfs_api_ctx->async_report_interval_ms);
        }
    }

    return NULL;
}

int async_reporter_init(FCFSAPIContext *fcfs_api_ctx)
{
    int result;
    pthread_t tid;

    g_async_reporter_ctx.fcfs_api_ctx = fcfs_api_ctx;
    if (!fcfs_api_ctx->async_report_enabled) {
        return 0;
    } 

    g_async_reporter_ctx.dsizes = (FDIRSetDEntrySizeInfo *)
        fc_malloc(sizeof(FDIRSetDEntrySizeInfo) *
                FDIR_CLIENT_BATCH_SET_DENTRY_MAX_COUNT);
    if (g_async_reporter_ctx.dsizes == NULL) {
        return ENOMEM;
    }

    if ((result=int_task_ptr_array(&SORTED_TASK_PTR_ARRAY)) != 0) {
        return result;
    }
    if ((result=int_task_ptr_array(&MERGED_TASK_PTR_ARRAY)) != 0) {
        return result;
    }

    if ((result=fc_queue_init(&g_async_reporter_ctx.queue, (long)
                    (&((FCFSAPIAsyncReportTask *)NULL)->next))) != 0)
    {
        return result;
    }

    return fc_create_thread(&tid, async_reporter_thread_func, NULL,
            SF_G_THREAD_STACK_SIZE);
}
