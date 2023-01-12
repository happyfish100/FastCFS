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

#include "group_htable.h"
#include "global.h"

#define FCFS_GROUP_FIXED_COUNT  16

typedef struct {
    SFShardingHashEntry hentry;  //must be the first
    time_t expires;
    struct {
        short alloc_size;
        short count;
        char holder[FCFS_GROUP_FIXED_COUNT * 4];
        char *list;  //4 bytes network order integer array
    } groups;
} FCFSGroupHashEntry;

typedef struct {
    int *count;
    char *list;
} FCFSGroupOpContext;

static SFHtableShardingContext fcfs_group_ctx;

static int group_htable_insert_callback(SFShardingHashEntry *he,
        void *arg, const bool new_create)
{
    FCFSGroupHashEntry *entry;
    FCFSGroupOpContext *op_ctx;

    entry = (FCFSGroupHashEntry *)he;
    op_ctx = (FCFSGroupOpContext *)arg;

    if (entry->groups.alloc_size == 0) {
        entry->groups.alloc_size = FCFS_GROUP_FIXED_COUNT;
        entry->groups.list = entry->groups.holder;
    }

    if (*(op_ctx->count) > entry->groups.alloc_size) {
        int new_size;
        char *new_list;

        new_size = 2 * entry->groups.alloc_size;
        while (new_size < *(op_ctx->count)) {
            new_size *= 2;
        }

        if ((new_list=fc_malloc(new_size * 4)) == NULL) {
            return ENOMEM;
        }

        if (entry->groups.list != entry->groups.holder) {
            free(entry->groups.list);
        }

        entry->groups.alloc_size = new_size;
        entry->groups.list = new_list;
    }

    entry->groups.count = *(op_ctx->count);
    if ((*op_ctx->count) > 0) {
        memcpy(entry->groups.list, op_ctx->list,
                entry->groups.count * 4);
    }
    entry->expires = g_current_time + GROUP_CACHE_TIMEOUT;
    return 0;
}

static void *group_htable_find_callback(SFShardingHashEntry *he, void *arg)
{
    FCFSGroupHashEntry *entry;
    FCFSGroupOpContext *op_ctx;

    entry = (FCFSGroupHashEntry *)he;
    op_ctx = (FCFSGroupOpContext *)arg;
    if (entry->expires <= g_current_time) {
        *(op_ctx->count) = entry->groups.count;
        if (entry->groups.count > 0) {
            memcpy(op_ctx->list, entry->groups.list,
                    entry->groups.count * 4);
        }
        return entry;
    } else {
        *(op_ctx->count) = 0;
        return NULL;
    }
}

int fcfs_group_htable_init()
{
    const int64_t min_ttl_ms = 600 * 1000;
    const int64_t max_ttl_ms = 86400 * 1000;
    const double low_water_mark_ratio = 0.10;

    return sf_sharding_htable_init_ex(&fcfs_group_ctx,
            sf_sharding_htable_key_ids_two, group_htable_insert_callback,
            group_htable_find_callback, NULL, NULL, GROUP_CACHE_SHARDING_COUNT,
            GROUP_CACHE_HTABLE_CAPACITY, GROUP_CACHE_ALLOCATOR_COUNT,
            sizeof(FCFSGroupHashEntry), GROUP_CACHE_ELEMENT_LIMIT,
            min_ttl_ms, max_ttl_ms, low_water_mark_ratio);
}

#define FCFS_GROUP_HTABLE_SET_KEY(key, pid, uid, gid) \
    key.id1 = ((int64_t)uid << 32) | gid; \
    key.id2 = pid

int fcfs_group_htable_insert(const pid_t pid, const uid_t uid,
            const gid_t gid, const int count, const char *list)
{
    SFTwoIdsHashKey key;
    FCFSGroupOpContext op_ctx;

    FCFS_GROUP_HTABLE_SET_KEY(key, pid, uid, gid);
    op_ctx.count = (int *)&count;
    op_ctx.list = (char *)list;
    return sf_sharding_htable_insert(&fcfs_group_ctx, &key, &op_ctx);
}

int fcfs_group_htable_find(const pid_t pid, const uid_t uid,
        const gid_t gid, int *count, char *list)
{
    SFTwoIdsHashKey key;
    FCFSGroupOpContext op_ctx;

    FCFS_GROUP_HTABLE_SET_KEY(key, pid, uid, gid);
    op_ctx.count = count;
    op_ctx.list = list;
    if (sf_sharding_htable_find(&fcfs_group_ctx, &key, &op_ctx) != NULL) {
        return 0;
    } else {
        return ENOENT;
    }
}
