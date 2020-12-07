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

static inline void deal_tasks(FCFSAPIAsyncReportTask *head)
{
    FCFSAPIAsyncReportTask *task;

    do {
        task = head;
        head = head->next;

        //TODO
    } while (head != NULL);
}

void async_reporter_terminate()
{
    FCFSAPIAsyncReportTask *head;

    head = (FCFSAPIAsyncReportTask *)fc_queue_try_pop_all(
            &g_async_reporter_ctx.queue);
    if (head != NULL) {
        deal_tasks(head);
    }

    logInfo("file: "__FILE__", line: %d, "
            "async_reporter_terminate", __LINE__);
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
    }

    return NULL;
}

int async_reporter_init(FCFSAPIContext *fcfs_api_ctx)
{
    int result;
    pthread_t tid;

    if ((result=fc_queue_init(&g_async_reporter_ctx.queue, (long)
                    (&((FCFSAPIAsyncReportTask *)NULL)->next))) != 0)
    {
        return result;
    }

    g_async_reporter_ctx.fcfs_api_ctx = fcfs_api_ctx;
    return fc_create_thread(&tid, async_reporter_thread_func, NULL,
            SF_G_THREAD_STACK_SIZE);
}
