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
#include "fastcommon/logger.h"
#include "fastcommon/uniq_skiplist.h"
#include "fastcommon/fast_allocator.h"
#include "sf/sf_global.h"
#include "../server_global.h"
#include "dao/user.h"
#include "auth_db.h"

#define SKIPLIST_INIT_LEVEL_COUNT   2

typedef struct db_storage_pool_info {
    FCFSAuthStoragePoolInfo pool;
    int status;
} DBStoragePoolInfo;

typedef struct db_storage_pool_granted {
    int64_t id;
    DBStoragePoolInfo *sp;
    struct {
        int fdir;
        int fstore;
    } priv;
} DBStoragePoolGranted;

typedef struct db_user_info {
    FCFSAuthUserInfo user;
    int status;
    struct {
        struct uniq_skiplist *created;  //element: DBStoragePoolInfo
        struct uniq_skiplist *granted;  //element: DBStoragePoolGranted
    } storage_pools;
} DBUserInfo;

typedef struct auth_db_context {
    pthread_mutex_t lock;
    struct fast_allocator_context name_acontext;

    struct {
        UniqSkiplistPair sl_pair;
        struct fast_mblock_man allocator; //element: DBUserInfo
    } user;

    struct {
        struct {
            UniqSkiplistFactory created;
            UniqSkiplistFactory granted;
        } factories;

        struct {
            struct fast_mblock_man created; //element: DBStoragePoolInfo
            struct fast_mblock_man granted; //element: DBStoragePoolGranted
        } allocators;
    } pool;
} AuthDBContext;

static AuthDBContext adb_ctx;

static int user_compare(const DBUserInfo *user1, const DBUserInfo *user2)
{
    return strcmp(user1->user.name.str, user2->user.name.str);
}

static int created_pool_cmp(const DBStoragePoolInfo *pool1,
        const DBStoragePoolInfo *pool2)
{
    return strcmp(pool1->pool.name.str, pool2->pool.name.str);
}

static int granted_pool_cmp(const DBStoragePoolGranted *pg1,
        const DBStoragePoolGranted *pg2)
{
    return strcmp(pg1->sp->pool.name.str, pg2->sp->pool.name.str);
}

int user_alloc_init(void *element, void *args)
{
    DBUserInfo *user;

    user = (DBUserInfo *)element;
    user->storage_pools.created = uniq_skiplist_new(&adb_ctx.pool.
            factories.created, SKIPLIST_INIT_LEVEL_COUNT);
    user->storage_pools.granted = uniq_skiplist_new(&adb_ctx.pool.
            factories.granted, SKIPLIST_INIT_LEVEL_COUNT);
    return 0;
}

static inline int init_name_allocators(
        struct fast_allocator_context *name_acontext)
{
#define NAME_REGION_COUNT 1
    struct fast_region_info regions[NAME_REGION_COUNT];

    FAST_ALLOCATOR_INIT_REGION(regions[0], 0, NAME_MAX, 8, 1024);
    return fast_allocator_init_ex(name_acontext, "name",
            regions, NAME_REGION_COUNT, 0, 0.00, 0, false);
}

static int init_allocators()
{
    int result;
    if ((result=fast_mblock_init_ex1(&adb_ctx.user.allocator,
                "user_allocator", sizeof(DBUserInfo), 256, 0,
                user_alloc_init, NULL, true)) != 0)
    {
        return result;
    }

    if ((result=fast_mblock_init_ex1(&adb_ctx.pool.allocators.created,
                "spool_created", sizeof(DBStoragePoolInfo), 1024, 0,
                NULL, NULL, true)) != 0)
    {
        return result;
    }

    if ((result=fast_mblock_init_ex1(&adb_ctx.pool.allocators.granted,
                "spool_granted", sizeof(DBStoragePoolGranted), 1024, 0,
                NULL, NULL, true)) != 0)
    {
        return result;
    }

    return 0;
}

static int init_skiplists()
{
    const int max_level_count = 12;
    const int alloc_skiplist_once = 1024;
    const int min_alloc_elements_once = 8;
    const int delay_free_seconds = 0;
    int result;

    if ((result=uniq_skiplist_init_pair(&adb_ctx.user.sl_pair,
                    SKIPLIST_INIT_LEVEL_COUNT, max_level_count,
                    (skiplist_compare_func)user_compare, NULL,
                    min_alloc_elements_once, delay_free_seconds)) != 0)
    {
        return result;
    }

    if ((result=uniq_skiplist_init_ex(&adb_ctx.pool.factories.created,
                    max_level_count, (skiplist_compare_func)created_pool_cmp,
                    NULL, alloc_skiplist_once, min_alloc_elements_once,
                    delay_free_seconds)) != 0)
    {
        return result;
    }

    if ((result=uniq_skiplist_init_ex(&adb_ctx.pool.factories.granted,
                    max_level_count, (skiplist_compare_func)granted_pool_cmp,
                    NULL, alloc_skiplist_once, min_alloc_elements_once,
                    delay_free_seconds)) != 0)
    {
        return result;
    }

    return 0;
}

int auth_db_init()
{
    int result;

    if ((result=init_pthread_lock(&adb_ctx.lock)) != 0) {
        return result;
    }

    if ((result=init_name_allocators(&adb_ctx.name_acontext)) != 0) {
        return result;
    }

    if ((result=init_skiplists()) != 0) {
        return result;
    }

    if ((result=init_allocators()) != 0) {
        return result;
    }

    return 0;
}

static inline DBUserInfo *user_get(AuthServerContext *server_ctx,
        const string_t *username)
{
    DBUserInfo target;

    target.user.name = *username;
    return (DBUserInfo *)uniq_skiplist_find(
            adb_ctx.user.sl_pair.skiplist, &target);
}

static int user_create(AuthServerContext *server_ctx, DBUserInfo *user,
        const string_t *username, const string_t *passwd, const int64_t priv)
{
    if (user == NULL) {
        user = (DBUserInfo *)fast_mblock_alloc_object(&adb_ctx.user.allocator);
        if (user == NULL) {
            return ENOMEM;
        }
    }
    return 0;
}

int adb_user_create(AuthServerContext *server_ctx, const string_t *username,
        const string_t *passwd, const int64_t priv)
{
    int result;
    DBUserInfo *user;

    PTHREAD_MUTEX_LOCK(&adb_ctx.lock);
    user = user_get(server_ctx, username);
    if (user != NULL && user->status == FCFS_AUTH_USER_STATUS_NORMAL) {
        result = EEXIST;
    } else {
        result = user_create(server_ctx, user, username, passwd, priv);
    }
    PTHREAD_MUTEX_UNLOCK(&adb_ctx.lock);

    return result;
}

const FCFSAuthUserInfo *adb_user_get(AuthServerContext *server_ctx,
        const string_t *username)
{
    DBUserInfo *user;

    PTHREAD_MUTEX_LOCK(&adb_ctx.lock);
    user = user_get(server_ctx, username);
    PTHREAD_MUTEX_UNLOCK(&adb_ctx.lock);

    if (user != NULL && user->status == FCFS_AUTH_USER_STATUS_NORMAL) {
        return &user->user;
    } else {
        return NULL;
    }
}
