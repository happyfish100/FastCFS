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


#include <sys/stat.h>
#include <limits.h>
#include "fastcommon/shared_func.h"
#include "fastcommon/pthread_func.h"
#include "fastcommon/fast_mblock.h"
#include "server_session.h"

#define SESSION_MIN_SHARED_ALLOCATOR_COUNT          1
#define SESSION_MAX_SHARED_ALLOCATOR_COUNT       1000
#define SESSION_DEFAULT_SHARED_ALLOCATOR_COUNT     11

#define SESSION_MIN_HASHTABLE_CAPACITY           1361
#define SESSION_MAX_HASHTABLE_CAPACITY       11229331
#define SESSION_DEFAULT_HASHTABLE_CAPACITY      10949

#define SESSION_MIN_SHARED_LOCK_COUNT               1
#define SESSION_MAX_SHARED_LOCK_COUNT           10000
#define SESSION_DEFAULT_SHARED_LOCK_COUNT         163

typedef struct server_session_hash_entry {
    ServerSessionEntry entry;
    struct fast_mblock_man *allocator;  //for free
    struct server_session_hash_entry *next;  //for hashtable
} ServerSessionHashEntry;

typedef struct {
    ServerSessionHashEntry **buckets;
    int capacity;
    int count;
} ServerSessionHashtable;

typedef struct {
    int count;
    volatile uint32_t next;
    struct fast_mblock_man *allocators;
} ServerSessionAllocatorArray;

typedef struct {
    int count;
    pthread_mutex_t *locks;
} ServerSessionLockArray;

typedef struct {
    ServerSessionAllocatorArray allocator_array;
    ServerSessionLockArray lock_array;
    ServerSessionHashtable htable;
} ServerSessionContext;

static ServerSessionContext session_ctx = {{0, 0, NULL}};

#define SESSION_SET_HASHTABLE_LOCK(htable, session_id) \
    int32_t bucket_index;  \
    pthread_mutex_t *lock; \
    bucket_index = session_id % (htable).capacity; \
    lock = session_ctx.lock_array.locks + (bucket_index %  \
            session_ctx.lock_array.count)

#define SESSION_SET_BUCKET_AND_LOCK(htable, session_id) \
    ServerSessionHashEntry **bucket; \
    SESSION_SET_HASHTABLE_LOCK(htable, session_id);  \
    bucket = (htable).buckets + bucket_index

void server_session_get_cfg(ServerSessionConfig *cfg)
{
    cfg->shared_allocator_count = session_ctx.allocator_array.count;
    cfg->shared_lock_count = session_ctx.lock_array.count;
    cfg->hashtable_capacity = session_ctx.htable.capacity;
}

static void load_session_config(IniFullContext *ini_ctx)
{
    session_ctx.allocator_array.count = iniGetIntCorrectValue(
            ini_ctx, "shared_allocator_count",
            SESSION_DEFAULT_SHARED_ALLOCATOR_COUNT,
            SESSION_MIN_SHARED_ALLOCATOR_COUNT,
            SESSION_MAX_SHARED_ALLOCATOR_COUNT);

    session_ctx.lock_array.count = iniGetInt64CorrectValue(
            ini_ctx, "shared_lock_count",
            SESSION_DEFAULT_SHARED_LOCK_COUNT,
            SESSION_MIN_SHARED_LOCK_COUNT,
            SESSION_MAX_SHARED_LOCK_COUNT);

    session_ctx.htable.capacity = iniGetIntCorrectValue(
            ini_ctx, "hashtable_capacity",
            SESSION_DEFAULT_HASHTABLE_CAPACITY,
            SESSION_MIN_HASHTABLE_CAPACITY,
            SESSION_MAX_HASHTABLE_CAPACITY);
}

static int server_session_alloc_init(ServerSessionHashEntry *session,
        struct fast_mblock_man *allocator)
{
    session->allocator = allocator;
    return 0;
}

static int init_allocator_array(ServerSessionAllocatorArray *array)
{
    int result;
    int bytes;
    struct fast_mblock_man *mblock;
    struct fast_mblock_man *end;

    bytes = sizeof(struct fast_mblock_man) * array->count;
    array->allocators = (struct fast_mblock_man *)fc_malloc(bytes);
    if (array->allocators == NULL) {
        return ENOMEM;
    }

    end = array->allocators + array->count;
    for (mblock=array->allocators; mblock<end; mblock++) {
        if ((result=fast_mblock_init_ex1(mblock, "session_entry",
                        sizeof(ServerSessionHashEntry), 4096, 0,
                        (fast_mblock_alloc_init_func)
                        server_session_alloc_init,
                        mblock, true)) != 0)
        {
            return result;
        }
    }

    return 0;
}

static int init_lock_array(ServerSessionLockArray *array)
{
    int result;
    int bytes;
    pthread_mutex_t *lock;
    pthread_mutex_t *end;

    bytes = sizeof(pthread_mutex_t) * array->count;
    array->locks = (pthread_mutex_t *)fc_malloc(bytes);
    if (array->locks == NULL) {
        return ENOMEM;
    }

    end = array->locks + array->count;
    for (lock=array->locks; lock<end; lock++) {
        if ((result=init_pthread_lock(lock)) != 0) {
            return result;
        }
    }

    return 0;
}

static int init_hashtable(ServerSessionHashtable *htable)
{
    int bytes;

    bytes = sizeof(ServerSessionHashEntry *) * htable->capacity;
    htable->buckets = (ServerSessionHashEntry **)fc_malloc(bytes);
    if (htable->buckets == NULL) {
        return ENOMEM;
    }
    memset(htable->buckets, 0, bytes);
    htable->count = 0;

    return 0;
}

int server_session_init(IniFullContext *ini_ctx)
{
    int result;

    load_session_config(ini_ctx);

    if ((result=init_allocator_array(&session_ctx.
                    allocator_array)) != 0)
    {
        return result;
    }

    if ((result=init_lock_array(&session_ctx.lock_array)) != 0) {
        return result;
    }

    if ((result=init_hashtable(&session_ctx.htable)) != 0) {
        return result;
    }

    srand(time(NULL));
    return 0;
}

static inline ServerSessionHashEntry *session_htable_find(ServerSessionHashEntry
        **bucket, const uint64_t session_id, ServerSessionHashEntry **prev)
{
    ServerSessionHashEntry *current;

    if (*bucket == NULL) {
        *prev = NULL;
        return NULL;
    } else if ((*bucket)->entry.session_id == session_id) {
        *prev = NULL;
        return *bucket;
    } else if ((*bucket)->entry.session_id > session_id) {
        *prev = NULL;
        return NULL;
    }

    *prev = *bucket;
    while ((current=(*prev)->next) != NULL) {
        if (current->entry.session_id == session_id) {
            return current;
        } else if (current->entry.session_id > session_id) {
            return NULL;
        }

        *prev = current;
    }

    return NULL;
}

static int session_htable_insert(ServerSessionHashEntry *se, const bool replace)
{
    int result;
    ServerSessionHashEntry *previous;
    ServerSessionHashEntry *found;

    SESSION_SET_BUCKET_AND_LOCK(session_ctx.htable, se->entry.session_id);
    PTHREAD_MUTEX_LOCK(lock);
    if ((found=session_htable_find(bucket, se->entry.
                    session_id, &previous)) == NULL)
    {
        if (previous == NULL) {
            se->next = *bucket;
            *bucket = se;
        } else {
            se->next = previous->next;
            previous->next = se;
        }
        result = 0;
    } else {
        if (replace) {
            found->entry = se->entry;
            fast_mblock_free_object(se->allocator, se);
            result = 0;
        } else {
            result = EEXIST;
        }
    }
    PTHREAD_MUTEX_UNLOCK(lock);

    return result;
}

int server_session_add(ServerSessionEntry *entry)
{
    int result;
    bool replace;
    struct fast_mblock_man *allocator;
    ServerSessionHashEntry *se;

    allocator = session_ctx.allocator_array.allocators +
        (__sync_fetch_and_add(&session_ctx.allocator_array.next, 1) %
         session_ctx.allocator_array.count);
    se = (ServerSessionHashEntry *)fast_mblock_alloc_object(allocator);
    if (se == NULL) {
        return ENOMEM;
    }

    se->entry = *entry;
    if (se->entry.session_id == 0) {
        replace = false;
        do {
            se->entry.session_id = ((int64_t)g_current_time << 32) |
                (int64_t)rand();
            result = session_htable_insert(se, replace);
        } while (result == EEXIST);
    } else {
        replace = true;
        result = session_htable_insert(se, replace);
    }

    return result;
}

int server_session_user_priv_granted(const uint64_t session_id,
        const int64_t the_priv)
{
    int result;
    ServerSessionHashEntry *previous;
    ServerSessionHashEntry *found;

    SESSION_SET_BUCKET_AND_LOCK(session_ctx.htable, session_id);
    PTHREAD_MUTEX_LOCK(lock);
    if ((found=session_htable_find(bucket, session_id,
                    &previous)) != NULL)
    {
        if ((found->entry.user_priv & the_priv) != 0) {
            result = 0;
        } else {
            result = EPERM;
        }
    } else {
        result = ENOENT;
    }
    PTHREAD_MUTEX_UNLOCK(lock);

    return result;
}

int server_session_fstore_priv_granted(const uint64_t session_id,
        const int the_priv)
{
    int result;
    ServerSessionHashEntry *previous;
    ServerSessionHashEntry *found;

    SESSION_SET_BUCKET_AND_LOCK(session_ctx.htable, session_id);
    PTHREAD_MUTEX_LOCK(lock);
    if ((found=session_htable_find(bucket, session_id,
                    &previous)) != NULL)
    {
        if ((found->entry.pool_priv.fstore & the_priv) != 0) {
            result = 0;
        } else {
            result = EPERM;
        }
    } else {
        result = ENOENT;
    }
    PTHREAD_MUTEX_UNLOCK(lock);

    return result;
}

int server_session_fdir_priv_granted(const uint64_t session_id,
        const int64_t pool_id, const int the_priv)
{
    int result;
    ServerSessionHashEntry *previous;
    ServerSessionHashEntry *found;

    SESSION_SET_BUCKET_AND_LOCK(session_ctx.htable, session_id);
    PTHREAD_MUTEX_LOCK(lock);
    if ((found=session_htable_find(bucket, session_id,
                    &previous)) != NULL)
    {
        if (found->entry.pool_id != pool_id) {
            result = EACCES;
        } else if ((found->entry.pool_priv.fdir & the_priv) != 0) {
            result = 0;
        } else {
            result = EPERM;
        }
    } else {
        result = ENOENT;
    }
    PTHREAD_MUTEX_UNLOCK(lock);

    return result;
}

int server_session_delete(const uint64_t session_id)
{
    int result;
    ServerSessionHashEntry *previous;
    ServerSessionHashEntry *found;

    SESSION_SET_BUCKET_AND_LOCK(session_ctx.htable, session_id);
    PTHREAD_MUTEX_LOCK(lock);
    if ((found=session_htable_find(bucket, session_id,
                    &previous)) != NULL)
    {
        if (previous == NULL) {
            *bucket = (*bucket)->next;
        } else {
            previous->next = found->next;
        }
        result = 0;
    } else {
        result = ENOENT;
    }
    PTHREAD_MUTEX_UNLOCK(lock);

    if (found != NULL) {
        fast_mblock_free_object(found->allocator, found);
    }
    return result;
}
