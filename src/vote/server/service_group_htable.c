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

#include "fastcommon/logger.h"
#include "fastcommon/shared_func.h"
#include "fastcommon/fc_atomic.h"
#include "sf/sf_nio.h"
#include "server_global.h"
#include "service_group_htable.h"

typedef struct fcfs_vote_shared_lock_array {
    pthread_mutex_t *locks;
    int count;
} FCFSVoteSharedLockArray;

typedef struct fcfs_vote_service_group_htable {
    FCFSVoteServiceGroupInfo **buckets;
    int capacity;
} FCFSVoteServiceGroupHtable;

typedef struct fcfs_vote_service_group_context {
    struct fast_mblock_man allocator;  //element: FCFSVoteServiceGroupInfo
    FCFSVoteSharedLockArray lock_array;
    FCFSVoteServiceGroupHtable htable;
} FCFSVoteServiceGroupContext;

static FCFSVoteServiceGroupContext service_group_ctx;

int service_group_htable_init()
{
    int result;
    int bytes;
    pthread_mutex_t *lock;
    pthread_mutex_t *end;

    if ((result=fast_mblock_init_ex1(&service_group_ctx.allocator,
                    "service_group", sizeof(FCFSVoteServiceGroupInfo),
                    4 * 1024, 0, NULL, NULL, true)) != 0)
    {
        return result;
    }

    service_group_ctx.lock_array.count = 17;
    bytes = sizeof(pthread_mutex_t) * service_group_ctx.lock_array.count;
    service_group_ctx.lock_array.locks = fc_malloc(bytes);
    if (service_group_ctx.lock_array.locks == NULL) {
        return ENOMEM;
    }

    end = service_group_ctx.lock_array.locks +
        service_group_ctx.lock_array.count;
    for (lock=service_group_ctx.lock_array.locks; lock<end; lock++) {
        if ((result=init_pthread_lock(lock)) != 0) {
            return result;
        }
    }

    service_group_ctx.htable.capacity = 1361;
    bytes = sizeof(FCFSVoteServiceGroupInfo *) *
        service_group_ctx.htable.capacity;
    service_group_ctx.htable.buckets = fc_malloc(bytes);
    if (service_group_ctx.htable.buckets == NULL) {
        return ENOMEM;
    }
    memset(service_group_ctx.htable.buckets, 0, bytes);

    return 0;
}

int service_group_htable_get(const short service_id, const int group_id,
        const int leader_id, const short response_size,
        struct fast_task_info *task, FCFSVoteServiceGroupInfo **group)
{
    uint64_t hash_code;
    int bucket_index;
    int old_leader_id;
    int result;
    pthread_mutex_t *lock;
    FCFSVoteServiceGroupInfo **bucket;
    FCFSVoteServiceGroupInfo *current;

    hash_code = ((int64_t)service_id << 32) | (int64_t)group_id;
    bucket_index = hash_code % service_group_ctx.htable.capacity;
    lock = service_group_ctx.lock_array.locks + bucket_index %
        service_group_ctx.lock_array.count;
    bucket = service_group_ctx.htable.buckets + bucket_index;

    PTHREAD_MUTEX_LOCK(lock);
    current = *bucket;
    while (current != NULL) {
        if (hash_code == current->hash_code) {
            break;
        }
        current = current->next;
    }

    if (current == NULL) {
        current = fast_mblock_alloc_object(&service_group_ctx.allocator);
        if (current != NULL) {
            current->hash_code = hash_code;
            current->service_id = service_id;
            current->response_size = response_size;
            current->group_id = group_id;
            current->leader_id = leader_id;
            current->task = (leader_id > 0 ? task : NULL);
            current->next = *bucket;
            *bucket = current;
            result = 0;
        } else {
            result = ENOMEM;
        }
    } else {
        if (leader_id > 0) {
            old_leader_id = FC_ATOMIC_GET(current->leader_id);
            if (old_leader_id == 0) {
                if (__sync_bool_compare_and_swap(&current->leader_id,
                            old_leader_id, leader_id))
                {
                    result = 0;
                } else {
                    result = SF_CLUSTER_ERROR_LEADER_INCONSISTENT;
                }
            } else if (old_leader_id == leader_id) {
                result = 0;
            } else {
                result = SF_CLUSTER_ERROR_LEADER_INCONSISTENT;
            }

            if (result == 0 && task != NULL && task != current->task) {
                if (current->task != NULL) {
                    logWarning("file: "__FILE__", line: %d, "
                            "network task already exist, clean it!",
                            __LINE__);
                    sf_nio_notify(current->task, SF_NIO_STAGE_CLOSE);
                }
                current->task = task;
            }
        } else {
            result = 0;
        }
    }

    PTHREAD_MUTEX_UNLOCK(lock);

    *group = current;
    return result;
}

void service_group_htable_unset_task(FCFSVoteServiceGroupInfo *group)
{
    int bucket_index;
    pthread_mutex_t *lock;

    bucket_index = group->hash_code % service_group_ctx.htable.capacity;
    lock = service_group_ctx.lock_array.locks + bucket_index %
        service_group_ctx.lock_array.count;
    PTHREAD_MUTEX_LOCK(lock);
    group->task = NULL;
    PTHREAD_MUTEX_UNLOCK(lock);
}

void service_group_htable_clear_tasks()
{
    FCFSVoteServiceGroupInfo **bucket;
    FCFSVoteServiceGroupInfo **end;
    FCFSVoteServiceGroupInfo *current;
    int bucket_index;
    int group_count;
    int clear_count;
    pthread_mutex_t *lock;

    group_count = clear_count = 0;
    end = service_group_ctx.htable.buckets +
        service_group_ctx.htable.capacity;
    for (bucket=service_group_ctx.htable.buckets; bucket<end; bucket++) {
        bucket_index = bucket - service_group_ctx.htable.buckets;
        lock = service_group_ctx.lock_array.locks + bucket_index %
            service_group_ctx.lock_array.count;
        PTHREAD_MUTEX_LOCK(lock);
        if (*bucket != NULL) {
            current = *bucket;
            do {
                ++group_count;
                if (current->task != NULL) {
                    ++clear_count;
                    sf_nio_notify(current->task, SF_NIO_STAGE_CLOSE);
                }
            } while ((current=current->next) != NULL);
        }
        PTHREAD_MUTEX_UNLOCK(lock);
    }

    logInfo("file: "__FILE__", line: %d, "
            "service group count: %d, clear count: %d",
            __LINE__, group_count, clear_count);
}
