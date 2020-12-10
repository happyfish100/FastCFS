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

#ifndef _INODE_HTABLE_H
#define _INODE_HTABLE_H

#include "fcfs_api_types.h"
#include "sf/sf_sharding_htable.h"

typedef struct fcfs_api_inode_hentry {
    SFShardingHashEntry hentry; //must be the first
    struct fc_list_head head;   //element: FCFSAPIAsyncReportEvent
} FCFSAPIInodeHEntry;

#ifdef __cplusplus
extern "C" {
#endif

    int inode_htable_init(const int sharding_count,
            const int64_t htable_capacity,
            const int allocator_count, int64_t element_limit,
            const int64_t min_ttl_sec, const int64_t max_ttl_sec);

    int inode_htable_insert(FCFSAPIAsyncReportEvent *event);

    int inode_htable_check_conflict_and_wait(const uint64_t inode);

    static inline void fcfs_api_notify_waiting_task(FCFSAPIWaitingTask *task)
    {
        PTHREAD_MUTEX_LOCK(&task->lcp.lock);
        task->finished = true;
        pthread_cond_signal(&task->lcp.cond);
        PTHREAD_MUTEX_UNLOCK(&task->lcp.lock);
    }

    static inline void fcfs_api_wait_report_done_and_release(
            FCFSAPIWaitingTask *waiting_task)
    {
        PTHREAD_MUTEX_LOCK(&waiting_task->lcp.lock);
        while (!waiting_task->finished) {
            pthread_cond_wait(&waiting_task->lcp.cond,
                    &waiting_task->lcp.lock);
        }
        waiting_task->finished = false;  //reset for next use
        PTHREAD_MUTEX_UNLOCK(&waiting_task->lcp.lock);
        fast_mblock_free_object(waiting_task->allocator, waiting_task);
    }

#ifdef __cplusplus
}
#endif

#endif
