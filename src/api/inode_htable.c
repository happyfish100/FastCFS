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
#include "async_reporter.h"
#include "inode_htable.h"

static SFHtableShardingContext inode_sharding_ctx;

typedef struct fcfs_api_insert_event_context {
    FCFSAPIAsyncReportEvent *event;
} FCFSAPIInsertEventContext;

typedef struct fcfs_api_find_callback_arg {
    FCFSAPIAllocatorContext *allocator_ctx;
    FCFSAPIWaitingTask *waiting_task;
} FCFSAPIFindCallbackArg;

static int inode_htable_insert_callback(SFShardingHashEntry *he,
        void *arg, const bool new_create)
{
    FCFSAPIInodeHEntry *inode;
    FCFSAPIInsertEventContext *ictx;

    inode = (FCFSAPIInodeHEntry *)he;
    ictx = (FCFSAPIInsertEventContext *)arg;
    if (new_create) {
        FC_INIT_LIST_HEAD(&inode->head);
    }

    ictx->event->inode_hentry = inode;
    fc_list_add(&ictx->event->dlink, &inode->head);
    return 0;
}

static void *inode_htable_find_callback(SFShardingHashEntry *he, void *arg)
{
    FCFSAPIInodeHEntry *inode;
    FCFSAPIFindCallbackArg *farg;
    FCFSAPIAsyncReportEvent *event;

    inode = (FCFSAPIInodeHEntry *)he;
    event = fc_list_first_entry(&inode->head, FCFSAPIAsyncReportEvent, dlink);
    if (event == NULL) {
        return NULL;
    }

    farg = (FCFSAPIFindCallbackArg *)arg;
    farg->waiting_task = (FCFSAPIWaitingTask *)fast_mblock_alloc_object(
            &farg->allocator_ctx->waiting_task);
    if (farg->waiting_task == NULL) {
        return NULL;
    }

    //add to event waiting queue
    farg->waiting_task->next = event->waitings.head;
    event->waitings.head = farg->waiting_task;
    return event;
}

static bool inode_htable_accept_reclaim_callback(SFShardingHashEntry *he)
{
    return fc_list_empty(&((FCFSAPIInodeHEntry *)he)->head);
}

int inode_htable_init(const int sharding_count,
        const int64_t htable_capacity,
        const int allocator_count, int64_t element_limit,
        const int64_t min_ttl_sec, const int64_t max_ttl_sec)
{
    return sf_sharding_htable_init(&inode_sharding_ctx,
            sf_sharding_htable_key_ids_one, inode_htable_insert_callback,
            inode_htable_find_callback, NULL,
            inode_htable_accept_reclaim_callback, sharding_count,
            htable_capacity, allocator_count, sizeof(FCFSAPIInodeHEntry),
            element_limit, min_ttl_sec, max_ttl_sec);
}

int inode_htable_insert(FCFSAPIAsyncReportEvent *event)
{
    SFTwoIdsHashKey key;
    FCFSAPIInsertEventContext ictx;

    key.oid = event->dsize.inode;
    ictx.event = event;
    return sf_sharding_htable_insert(&inode_sharding_ctx,
            &key, &ictx);
}

int inode_htable_check_conflict_and_wait(const uint64_t inode)
{
    SFTwoIdsHashKey key;
    FCFSAPIFindCallbackArg callback_arg;

    key.oid = inode;
    callback_arg.allocator_ctx = fcfs_api_allocator_get(inode);
    callback_arg.waiting_task = NULL;
    if (sf_sharding_htable_find(&inode_sharding_ctx,
                &key, &callback_arg) == NULL)
    {
        return 0;
    }

    if (callback_arg.waiting_task != NULL) {
        async_reporter_notify();
        //logInfo("oid: %"PRId64", wait_report_done_and_release", inode);
        fcfs_api_wait_report_done_and_release(callback_arg.waiting_task);
    }

    return 0;
}
