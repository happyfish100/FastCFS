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

#include <pthread.h>
#include "fastcommon/logger.h"
#include "fastcommon/shared_func.h"
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
        FCFSVoteServiceGroupInfo **group)
{
    int64_t hash_code;
    int bucket_index;
    pthread_mutex_t *lock;
    FCFSVoteServiceGroupInfo **buckets;

    hash_code = ((int64_t)service_id << 32) | (int64_t)group_id;

    bucket_index = hash_code % service_group_ctx.htable.capacity;
    lock = service_group_ctx.lock_array.locks + bucket_index %
        service_group_ctx.lock_array.count;
    buckets = service_group_ctx.htable.buckets + bucket_index;

    PTHREAD_MUTEX_LOCK(lock);
    //TODO
    PTHREAD_MUTEX_UNLOCK(lock);
    return 0;
}
