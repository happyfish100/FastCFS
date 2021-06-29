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
#include "fastcommon/fc_atomic.h"
#include "sf/sf_global.h"
#include "../../common/auth_func.h"
#include "../server_global.h"
#include "dao/user.h"
#include "dao/storage_pool.h"
#include "dao/granted_pool.h"
#include "auth_db.h"

#define SKIPLIST_INIT_LEVEL_COUNT   2

static int auth_db_init();

typedef struct auth_db_context {
    pthread_mutex_t lock;
    struct fast_allocator_context name_acontext;

    struct {
        int count;
        UniqSkiplistPair sl_pair;   //element: DBUserInfo *
        struct fast_mblock_man allocator; //element: DBUserInfo
    } user;

    struct {
        volatile int64_t auto_id;
        struct {
            UniqSkiplistFactory created;
            UniqSkiplistFactory granted;
        } factories;

        struct {
            struct fast_mblock_man created; //element: DBStoragePoolInfo
            struct fast_mblock_man granted; //element: DBGrantedPoolInfo
        } allocators;

        struct {
            UniqSkiplistPair by_id;   //element: DBStoragePoolInfo *
            UniqSkiplistPair by_name; //element: DBStoragePoolInfo *
        } sl_pairs;
    } pool;
} AuthDBContext;

static AuthDBContext adb_ctx;

static int user_compare(const DBUserInfo *user1, const DBUserInfo *user2)
{
    return fc_string_compare(&user1->user.name, &user2->user.name);
}

static int pool_name_cmp(const DBStoragePoolInfo *sp1,
        const DBStoragePoolInfo *sp2)
{
    return fc_string_compare(&sp1->pool.name, &sp2->pool.name);
}

static int pool_id_cmp(const DBStoragePoolInfo *sp1,
        const DBStoragePoolInfo *sp2)
{
    return fc_compare_int64(sp1->pool.id, sp2->pool.id);
}

static int granted_pool_cmp(const DBGrantedPoolInfo *pg1,
        const DBGrantedPoolInfo *pg2)
{
    return fc_compare_int64(pg1->granted.pool_id, pg2->granted.pool_id);
}

int user_alloc_init(void *element, void *args)
{
    DBUserInfo *user;

    user = (DBUserInfo *)element;
    if ((user->storage_pools.created=uniq_skiplist_new(&adb_ctx.pool.
                    factories.created, SKIPLIST_INIT_LEVEL_COUNT)) == NULL)
    {
        return ENOMEM;
    }
    if ((user->storage_pools.granted=uniq_skiplist_new(&adb_ctx.pool.
                    factories.granted, SKIPLIST_INIT_LEVEL_COUNT)) == NULL)
    {
        return ENOMEM;
    }

    return 0;
}

static inline int init_name_allocators(
        struct fast_allocator_context *name_acontext)
{
#define NAME_REGION_COUNT 1
    struct fast_region_info regions[NAME_REGION_COUNT];

    FAST_ALLOCATOR_INIT_REGION(regions[0], 0, 256, 8, 1024);
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
                "spool_granted", sizeof(DBGrantedPoolInfo), 1024, 0,
                NULL, NULL, true)) != 0)
    {
        return result;
    }

    return 0;
}

static void destroy_allocators()
{
    fast_mblock_destroy(&adb_ctx.user.allocator);
    fast_mblock_destroy(&adb_ctx.pool.allocators.created);
    fast_mblock_destroy(&adb_ctx.pool.allocators.granted);
}

static void free_storage_pool_func(void *ptr, const int delay_seconds)
{
    DBStoragePoolInfo *dbspool;

    dbspool = (DBStoragePoolInfo *)ptr;
    uniq_skiplist_delete(adb_ctx.pool.sl_pairs.by_id.skiplist, dbspool);
    uniq_skiplist_delete(adb_ctx.pool.sl_pairs.by_name.skiplist, dbspool);

    fast_allocator_free(&adb_ctx.name_acontext, dbspool->pool.name.str);
    fast_mblock_free_object(&adb_ctx.pool.allocators.created, dbspool);
}

static void free_granted_pool_func(void *ptr, const int delay_seconds)
{
    DBGrantedPoolInfo *dbgranted;

    dbgranted = (DBGrantedPoolInfo *)ptr;
    fast_mblock_free_object(&adb_ctx.pool.allocators.granted, dbgranted);
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
                    max_level_count, (skiplist_compare_func)pool_name_cmp,
                    free_storage_pool_func, alloc_skiplist_once,
                    min_alloc_elements_once, delay_free_seconds)) != 0)
    {
        return result;
    }

    if ((result=uniq_skiplist_init_ex(&adb_ctx.pool.factories.granted,
                    max_level_count, (skiplist_compare_func)granted_pool_cmp,
                    free_granted_pool_func, alloc_skiplist_once,
                    min_alloc_elements_once, delay_free_seconds)) != 0)
    {
        return result;
    }

    if ((result=uniq_skiplist_init_pair(&adb_ctx.pool.sl_pairs.by_id,
                    SKIPLIST_INIT_LEVEL_COUNT, max_level_count,
                    (skiplist_compare_func)pool_id_cmp, NULL,
                    min_alloc_elements_once, delay_free_seconds)) != 0)
    {
        return result;
    }

    if ((result=uniq_skiplist_init_pair(&adb_ctx.pool.sl_pairs.by_name,
                    SKIPLIST_INIT_LEVEL_COUNT, max_level_count,
                    (skiplist_compare_func)pool_name_cmp, NULL,
                    min_alloc_elements_once, delay_free_seconds)) != 0)
    {
        return result;
    }

    return 0;
}

static void destroy_skiplists()
{
    uniq_skiplist_destroy(&adb_ctx.user.sl_pair.factory);
    uniq_skiplist_destroy(&adb_ctx.pool.factories.created);
    uniq_skiplist_destroy(&adb_ctx.pool.factories.granted);
    uniq_skiplist_destroy(&adb_ctx.pool.sl_pairs.by_id.factory);
    uniq_skiplist_destroy(&adb_ctx.pool.sl_pairs.by_name.factory);
}

static int auth_db_init()
{
    int result;

    if ((result=init_pthread_lock(&adb_ctx.lock)) != 0) {
        return result;
    }

    if ((result=init_name_allocators(&adb_ctx.name_acontext)) != 0) {
        return result;
    }

    if ((result=init_allocators()) != 0) {
        return result;
    }

    if ((result=init_skiplists()) != 0) {
        return result;
    }

    return 0;
}

void auth_db_destroy()
{
    destroy_skiplists();
    destroy_allocators();
    fast_allocator_destroy(&adb_ctx.name_acontext);
    pthread_mutex_destroy(&adb_ctx.lock);
}

static inline DBUserInfo *user_get(AuthServerContext *server_ctx,
        const string_t *username)
{
    DBUserInfo target;

    target.user.name = *username;
    return (DBUserInfo *)uniq_skiplist_find(
            adb_ctx.user.sl_pair.skiplist, &target);
}

static inline int user_set_passwd(DBUserInfo *user, const string_t *passwd)
{
    int result;
    char *old_passwd;

    old_passwd = user->user.passwd.str;
    result = fast_allocator_alloc_string(&adb_ctx.name_acontext,
            &user->user.passwd, passwd);
    if (old_passwd != NULL) {
        fast_allocator_free(&adb_ctx.name_acontext, old_passwd);
    }
    return result;
}

static int user_create(AuthServerContext *server_ctx, DBUserInfo **dbuser,
        const FCFSAuthUserInfo *user, const bool addto_backend)
{
    int result;
    bool need_insert;

    if (*dbuser == NULL) {
        *dbuser = (DBUserInfo *)fast_mblock_alloc_object(
                &adb_ctx.user.allocator);
        if (*dbuser == NULL) {
            return ENOMEM;
        }

        if ((result=fast_allocator_alloc_string(&adb_ctx.name_acontext,
                        &(*dbuser)->user.name, &user->name)) != 0)
        {
            return result;
        }
        need_insert = true;
    } else {
        need_insert = false;
    }

    if ((result=user_set_passwd(*dbuser, &user->passwd)) != 0) {
        return result;
    }
    (*dbuser)->user.priv = user->priv;
    if (addto_backend) {
        if ((result=dao_user_create(server_ctx->dao_ctx,
                        &(*dbuser)->user)) != 0)
        {
            return result;
        }
    } else {
        (*dbuser)->user.id = user->id;
    }

    PTHREAD_MUTEX_LOCK(&adb_ctx.lock);
    (*dbuser)->user.status = user->status;
    if (need_insert) {
        if ((result=uniq_skiplist_insert(adb_ctx.user.sl_pair.
                        skiplist, *dbuser)) == 0)
        {
            adb_ctx.user.count++;
        }
    }
    PTHREAD_MUTEX_UNLOCK(&adb_ctx.lock);

    return 0;
}

int adb_user_create(AuthServerContext *server_ctx,
        const FCFSAuthUserInfo *user)
{
    const bool addto_backend = true;
    int result;
    DBUserInfo *dbuser;

    PTHREAD_MUTEX_LOCK(&adb_ctx.lock);
    dbuser = user_get(server_ctx, &user->name);
    if (dbuser != NULL && dbuser->user.status == FCFS_AUTH_USER_STATUS_NORMAL) {
        result = EEXIST;
    } else {
        result = ENOENT;
    }
    PTHREAD_MUTEX_UNLOCK(&adb_ctx.lock);

    if (result == ENOENT) {
        result = user_create(server_ctx, &dbuser, user, addto_backend);
    }
    return result;
}

const DBUserInfo *adb_user_get(AuthServerContext *server_ctx,
        const string_t *username)
{
    DBUserInfo *user;

    PTHREAD_MUTEX_LOCK(&adb_ctx.lock);
    user = user_get(server_ctx, username);
    PTHREAD_MUTEX_UNLOCK(&adb_ctx.lock);

    if (user != NULL && user->user.status == FCFS_AUTH_USER_STATUS_NORMAL) {
        return user;
    } else {
        return NULL;
    }
}

int adb_user_remove(AuthServerContext *server_ctx, const string_t *username)
{
    DBUserInfo *user;
    int result;

    PTHREAD_MUTEX_LOCK(&adb_ctx.lock);
    user = user_get(server_ctx, username);
    if (user != NULL && user->user.status == FCFS_AUTH_USER_STATUS_NORMAL) {
        if ((result=dao_user_remove(server_ctx->dao_ctx,
                        user->user.id)) == 0)
        {
            user->user.status = FCFS_AUTH_USER_STATUS_DELETED;
            adb_ctx.user.count--;
        }
    } else {
        result = ENOENT;
    }
    PTHREAD_MUTEX_UNLOCK(&adb_ctx.lock);

    return result;
}

int adb_user_update_priv(AuthServerContext *server_ctx,
        const string_t *username, const int64_t priv)
{
    DBUserInfo *user;
    int result;
    bool changed;

    PTHREAD_MUTEX_LOCK(&adb_ctx.lock);
    user = user_get(server_ctx, username);
    if (user != NULL && user->user.status == FCFS_AUTH_USER_STATUS_NORMAL) {
        if (user->user.priv == priv) {
            result = 0;
            changed = false;
        } else if ((result=dao_user_update_priv(server_ctx->dao_ctx,
                        user->user.id, priv)) == 0)
        {
            user->user.priv = priv;
            changed = true;
        } else {
            changed = false;
        }
    } else {
        result = ENOENT;
        changed = false;
    }
    PTHREAD_MUTEX_UNLOCK(&adb_ctx.lock);

    if (changed) {
        g_db_priv_change_callbacks.user_priv_changed(user->user.id, priv);
    }
    return result;
}

int adb_user_update_passwd(AuthServerContext *server_ctx,
        const string_t *username, const string_t *passwd)
{
    DBUserInfo *user;
    int result;

    PTHREAD_MUTEX_LOCK(&adb_ctx.lock);
    user = user_get(server_ctx, username);
    if (user != NULL && user->user.status == FCFS_AUTH_USER_STATUS_NORMAL) {
        if ((result=dao_user_update_passwd(server_ctx->dao_ctx,
                        user->user.id, passwd)) == 0)
        {
            user_set_passwd(user, passwd);
        }
    } else {
        result = ENOENT;
    }
    PTHREAD_MUTEX_UNLOCK(&adb_ctx.lock);

    return result;
}

int adb_user_list(AuthServerContext *server_ctx,
        const SFListLimitInfo *limit,
        FCFSAuthUserArray *array)
{
    UniqSkiplistIterator it;
    DBUserInfo *dbuser;
    int result;

    array->count = 0;
    if ((result=fcfs_auth_user_check_realloc_array(array,
                    limit->count)) != 0)
    {
        return result;
    }

    PTHREAD_MUTEX_LOCK(&adb_ctx.lock);
    uniq_skiplist_iterator_at(adb_ctx.user.sl_pair.skiplist,
            limit->offset, &it);
    while ((array->count < limit->count) && (dbuser=(DBUserInfo *)
                uniq_skiplist_next(&it)) != NULL)
    {
        if (dbuser->user.status == FCFS_AUTH_USER_STATUS_NORMAL) {
            array->users[array->count++] = dbuser->user;
        }
    }
    PTHREAD_MUTEX_UNLOCK(&adb_ctx.lock);

    return 0;
}

static inline DBStoragePoolInfo *get_spool_by_id(const int64_t pool_id)
{
    DBStoragePoolInfo target;

    target.pool.id = pool_id;
    return (DBStoragePoolInfo *)uniq_skiplist_find(
            adb_ctx.pool.sl_pairs.by_id.skiplist, &target);
}

static inline DBStoragePoolInfo *get_spool_by_name(const string_t *name)
{
    DBStoragePoolInfo target;

    target.pool.name = *name;
    return (DBStoragePoolInfo *)uniq_skiplist_find(
            adb_ctx.pool.sl_pairs.by_name.skiplist, &target);
}


static inline DBStoragePoolInfo *user_spool_get(AuthServerContext
        *server_ctx, DBUserInfo *user, const string_t *poolname)
{
    DBStoragePoolInfo target;

    target.pool.name = *poolname;
    return (DBStoragePoolInfo *)uniq_skiplist_find(
            user->storage_pools.created, &target);
}

static inline int spool_global_skiplists_insert(DBStoragePoolInfo *dbspool)
{
    int result;
    if ((result=uniq_skiplist_insert(adb_ctx.pool.sl_pairs.
                    by_id.skiplist, dbspool)) == 0)
    {
        result = uniq_skiplist_insert(adb_ctx.pool.
                sl_pairs.by_name.skiplist, dbspool);
    }

    return result;
}

static int storage_pool_create(AuthServerContext *server_ctx,
        DBUserInfo *dbuser, DBStoragePoolInfo **dbspool,
        const FCFSAuthStoragePoolInfo *pool, const bool addto_backend)
{
    int result;
    bool need_insert;

    if (*dbspool == NULL) {
        *dbspool = (DBStoragePoolInfo *)fast_mblock_alloc_object(
                &adb_ctx.pool.allocators.created);
        if (*dbspool == NULL) {
            return ENOMEM;
        }

        if ((result=fast_allocator_alloc_string(&adb_ctx.name_acontext,
                        &(*dbspool)->pool.name, &pool->name)) != 0)
        {
            return result;
        }
        (*dbspool)->user = dbuser;
        need_insert = true;
    } else {
        result = 0;
        need_insert = false;
    }

    (*dbspool)->pool.quota = pool->quota;
    if (addto_backend) {
        if ((result=dao_spool_create(server_ctx->dao_ctx, &dbuser->
                        user.name, &(*dbspool)->pool)) != 0)
        {
            fast_allocator_free(&adb_ctx.name_acontext,
                    (*dbspool)->pool.name.str);
            fast_mblock_free_object(&adb_ctx.pool.
                    allocators.created, *dbspool);
            return result;
        }
    } else {
        (*dbspool)->pool.id = pool->id;
    }

    PTHREAD_MUTEX_LOCK(&adb_ctx.lock);
    (*dbspool)->pool.status = pool->status;
    if (need_insert) {
        if ((result=spool_global_skiplists_insert(*dbspool)) == 0) {
            result = uniq_skiplist_insert(dbuser->storage_pools.
                    created, *dbspool);
        }
    }
    PTHREAD_MUTEX_UNLOCK(&adb_ctx.lock);

    if (result != 0) {
        dao_spool_remove(server_ctx->dao_ctx, (*dbspool)->pool.id);  //rollback

        fast_allocator_free(&adb_ctx.name_acontext,
                (*dbspool)->pool.name.str);
        fast_mblock_free_object(&adb_ctx.pool.
                allocators.created, *dbspool);
    }

    return result;
}

int adb_spool_create(AuthServerContext *server_ctx, const string_t
        *username, const FCFSAuthStoragePoolInfo *pool)
{
    const bool addto_backend = true;
    int result;
    DBUserInfo *user;
    DBStoragePoolInfo *dbspool;

    result = ENOENT;
    dbspool = NULL;
    PTHREAD_MUTEX_LOCK(&adb_ctx.lock);
    user = user_get(server_ctx, username);
    if (user != NULL) {
        if (user->user.status == FCFS_AUTH_USER_STATUS_NORMAL) {
            dbspool = user_spool_get(server_ctx, user, &pool->name);
            if (dbspool != NULL) {
                if (dbspool->pool.status == FCFS_AUTH_POOL_STATUS_NORMAL) {
                    result = EEXIST;
                }
            } else {
                if (get_spool_by_name(&pool->name) != NULL) {
                    result = EEXIST; //occupied by other user
                }
            }
        } else {
            user = NULL;
        }
    }
    PTHREAD_MUTEX_UNLOCK(&adb_ctx.lock);

    if (user != NULL && result == ENOENT) {
        return storage_pool_create(server_ctx, user,
                &dbspool, pool, addto_backend);
    } else {
        return result;
    }
}

const FCFSAuthStoragePoolInfo *adb_spool_get(AuthServerContext *server_ctx,
        const string_t *username, const string_t *poolname)
{
    DBUserInfo *user;
    DBStoragePoolInfo *spool;

    PTHREAD_MUTEX_LOCK(&adb_ctx.lock);
    user = user_get(server_ctx, username);
    if (user != NULL) {
        spool = user_spool_get(server_ctx, user, poolname);
    } else {
        spool = NULL;
    }
    PTHREAD_MUTEX_UNLOCK(&adb_ctx.lock);

    if (spool != NULL && spool->pool.status == FCFS_AUTH_POOL_STATUS_NORMAL) {
        return &spool->pool;
    } else {
        return NULL;
    }
}

int adb_spool_remove(AuthServerContext *server_ctx,
        const string_t *username, const string_t *poolname)
{
    DBUserInfo *user;
    DBStoragePoolInfo *spool;
    int result;

    PTHREAD_MUTEX_LOCK(&adb_ctx.lock);
    user = user_get(server_ctx, username);
    if (user != NULL) {
        spool = user_spool_get(server_ctx, user, poolname);
    } else {
        spool = NULL;
    }
    PTHREAD_MUTEX_UNLOCK(&adb_ctx.lock);

    if (!(spool != NULL && spool->pool.status ==
            FCFS_AUTH_POOL_STATUS_NORMAL))
    {
        return ENOENT;
    }

    if ((result=dao_spool_remove(server_ctx->dao_ctx,
                    spool->pool.id)) != 0)
    {
        return result;
    }

    PTHREAD_MUTEX_LOCK(&adb_ctx.lock);
    spool->pool.status = FCFS_AUTH_POOL_STATUS_DELETED;
    PTHREAD_MUTEX_UNLOCK(&adb_ctx.lock);

    return 0;
}

static void call_pool_quota_avail_func(const DBStoragePoolInfo *dbpool,
        const bool old_avail)
{
    bool new_avail;

    new_avail = FCFS_AUTH_POOL_AVAILABLE(dbpool->pool);
    if (new_avail != old_avail) {
        g_db_priv_change_callbacks.pool_quota_avail_changed(
                dbpool->pool.id, new_avail);
    }
}

int adb_spool_set_quota(AuthServerContext *server_ctx,
        const string_t *username, const string_t *poolname,
        const int64_t quota)
{
    DBUserInfo *user;
    DBStoragePoolInfo *spool;
    int64_t old_quota;
    int result;
    bool changed;
    bool old_avail;

    old_quota = 0;
    PTHREAD_MUTEX_LOCK(&adb_ctx.lock);
    user = user_get(server_ctx, username);
    if (user != NULL) {
        if ((spool=user_spool_get(server_ctx, user, poolname)) != NULL) {
            old_quota = spool->pool.quota;
        }
    } else {
        spool = NULL;
    }
    PTHREAD_MUTEX_UNLOCK(&adb_ctx.lock);

    if (!(spool != NULL && spool->pool.status ==
                FCFS_AUTH_POOL_STATUS_NORMAL))
    {
        return ENOENT;
    }

    if (old_quota != quota) {
        if ((result=dao_spool_set_quota(server_ctx->dao_ctx,
                        spool->pool.id, quota)) != 0)
        {
            return result;
        }
        old_avail = FCFS_AUTH_POOL_AVAILABLE(spool->pool);
        changed = true;
    } else {
        old_avail = false;
        changed = false;
    }

    PTHREAD_MUTEX_LOCK(&adb_ctx.lock);
    if (changed) {
        spool->pool.quota = quota;
    }
    PTHREAD_MUTEX_UNLOCK(&adb_ctx.lock);

    if (changed) {
        call_pool_quota_avail_func(spool, old_avail);
    }

    return 0;
}

int adb_spool_get_quota(AuthServerContext *server_ctx,
        const string_t *poolname, int64_t *quota)
{
    DBStoragePoolInfo *spool;
    int result;

    PTHREAD_MUTEX_LOCK(&adb_ctx.lock);
    if ((spool=get_spool_by_name(poolname)) != NULL) {
        if (spool->pool.status == FCFS_AUTH_POOL_STATUS_NORMAL) {
            *quota = spool->pool.quota;
            result = 0;
        } else {
            *quota = 0;
            result = ENOENT;
        }
    } else {
        *quota = 0;
        result = ENOENT;
    }
    PTHREAD_MUTEX_UNLOCK(&adb_ctx.lock);

    return result;
}

int adb_spool_set_used_bytes(const string_t *poolname,
        const int64_t used_bytes)
{
    DBStoragePoolInfo *dbpool;
    bool old_avail;

    if ((dbpool=adb_spool_global_get(poolname)) == NULL) {
        return ENOENT;
    }

    if (dbpool->pool.used == used_bytes) {
        return 0;
    }

    old_avail = FCFS_AUTH_POOL_AVAILABLE(dbpool->pool);
    dbpool->pool.used = used_bytes;
    call_pool_quota_avail_func(dbpool, old_avail);
    return 0;
}

int adb_spool_list(AuthServerContext *server_ctx, const string_t *username,
        const SFListLimitInfo *limit, FCFSAuthStoragePoolArray *array)
{
    UniqSkiplistIterator it;
    DBUserInfo *user;
    DBStoragePoolInfo *spool;
    int result;

    array->count = 0;
    if ((result=fcfs_auth_spool_check_realloc_array(array,
                    limit->count)) != 0)
    {
        return result;
    }

    PTHREAD_MUTEX_LOCK(&adb_ctx.lock);
    user = user_get(server_ctx, username);
    if (user != NULL) {
        uniq_skiplist_iterator_at(user->storage_pools.created,
                limit->offset, &it);
        while ((array->count < limit->count) && (spool=(DBStoragePoolInfo *)
                    uniq_skiplist_next(&it)) != NULL)
        {
            if (spool->pool.status == FCFS_AUTH_POOL_STATUS_NORMAL) {
                array->spools[array->count++] = spool->pool;
            }
        }
    } else {
        result = ENOENT;
    }
    PTHREAD_MUTEX_UNLOCK(&adb_ctx.lock);

    return result;
}

static int convert_spool_array(AuthServerContext *server_ctx,
        DBUserInfo *dbuser, const FCFSAuthStoragePoolArray *spool_array)
{
    const bool addto_backend = false;
    int result;
    DBStoragePoolInfo *dbspool;
    const FCFSAuthStoragePoolInfo *spool;
    const FCFSAuthStoragePoolInfo *end;

    end = spool_array->spools + spool_array->count;
    for (spool=spool_array->spools; spool<end; spool++) {
        dbspool = NULL;

        if ((result=storage_pool_create(server_ctx, dbuser,
                        &dbspool, spool, addto_backend)) != 0)
        {
            return result;
        }
    }

    return 0;
}

static int load_user_spool(AuthServerContext *server_ctx,
        struct fast_mpool_man *mpool, DBUserInfo *dbuser)
{
    FCFSAuthStoragePoolArray spool_array;
    int result;

    fcfs_auth_spool_init_array(&spool_array);
    if ((result=dao_spool_list(server_ctx->dao_ctx, &dbuser->
                    user.name, mpool, &spool_array)) != 0)
    {
        return result;
    }

    if ((result=convert_spool_array(server_ctx,
                    dbuser, &spool_array)) != 0)
    {
        return result;
    }

    fcfs_auth_spool_free_array(&spool_array);
    return 0;
}

static int granted_pool_create(AuthServerContext *server_ctx,
        DBUserInfo *dbuser, DBGrantedPoolInfo **dbgranted,
        const FCFSAuthGrantedPoolInfo *granted, const bool addto_backend)
{
    int result;
    bool need_insert;

    if (*dbgranted == NULL) {
        *dbgranted = (DBGrantedPoolInfo *)fast_mblock_alloc_object(
                &adb_ctx.pool.allocators.granted);
        if (*dbgranted == NULL) {
            return ENOMEM;
        }
        need_insert = true;
    } else {
        need_insert = false;
    }

    (*dbgranted)->granted.pool_id = granted->pool_id;
    (*dbgranted)->granted.privs = granted->privs;
    if (addto_backend) {
        if ((result=dao_granted_create(server_ctx->dao_ctx, &dbuser->
                        user.name, &(*dbgranted)->granted)) != 0)
        {
            fast_mblock_free_object(&adb_ctx.pool.
                    allocators.granted, *dbgranted);
            return result;
        }
    } else {
        (*dbgranted)->granted.id = granted->id;
        result = 0;
    }

    if (need_insert) {
        PTHREAD_MUTEX_LOCK(&adb_ctx.lock);
        if (((*dbgranted)->sp=get_spool_by_id(granted->pool_id)) != NULL) {
            result = uniq_skiplist_insert(dbuser->storage_pools.
                    granted, *dbgranted);
        } else {
            logError("file: "__FILE__", line: %d, "
                    "storage pool not exist, pool id: %"PRId64,
                    __LINE__, granted->pool_id);
            result = ENOENT;
        }
        PTHREAD_MUTEX_UNLOCK(&adb_ctx.lock);

        if (result != 0) {
            fast_mblock_free_object(&adb_ctx.pool.
                    allocators.granted, *dbgranted);
        }
    }

    return result;
}

static int convert_granted_array(AuthServerContext *server_ctx,
        DBUserInfo *dbuser, const FCFSAuthGrantedPoolArray *granted_array)
{
    const bool addto_backend = false;
    int result;
    DBGrantedPoolInfo *dbgranted;
    const FCFSAuthGrantedPoolFullInfo *gf;
    const FCFSAuthGrantedPoolFullInfo *end;

    end = granted_array->gpools + granted_array->count;
    for (gf=granted_array->gpools; gf<end; gf++) {
        dbgranted = NULL;
        if ((result=granted_pool_create(server_ctx, dbuser, &dbgranted,
                        &gf->granted, addto_backend)) != 0)
        {
            return result;
        }
    }

    return 0;
}

static int load_granted_pool(AuthServerContext *server_ctx, DBUserInfo *dbuser)
{
    FCFSAuthGrantedPoolArray granted_array;
    int result;

    fcfs_auth_granted_init_array(&granted_array);
    if ((result=dao_granted_list(server_ctx->dao_ctx, &dbuser->
                    user.name, &granted_array)) != 0)
    {
        return result;
    }

    if ((result=convert_granted_array(server_ctx,
                    dbuser, &granted_array)) != 0)
    {
        return result;
    }

    fcfs_auth_granted_free_array(&granted_array);
    return 0;
}

static int convert_user_array(AuthServerContext *server_ctx,
        struct fast_mpool_man *mpool,
        const FCFSAuthUserArray *user_array)
{
    const bool addto_backend = false;
    int result;
    DBUserInfo *dbuser;
    const FCFSAuthUserInfo *user;
    const FCFSAuthUserInfo *end;
    UniqSkiplistIterator it;

    end = user_array->users + user_array->count;
    for (user=user_array->users; user<end; user++) {
        dbuser = NULL;
        if ((result=user_create(server_ctx, &dbuser,
                        user, addto_backend)) != 0)
        {
            return result;
        }

        if ((result=load_user_spool(server_ctx, mpool, dbuser)) != 0) {
            return result;
        }
    }

    uniq_skiplist_iterator(adb_ctx.user.sl_pair.skiplist, &it);
    while ((dbuser=(DBUserInfo *)uniq_skiplist_next(&it)) != NULL) {
        if ((result=load_granted_pool(server_ctx, dbuser)) != 0) {
            return result;
        }
    }

    return 0;
}

static int load_pool_auto_id(AuthServerContext *server_ctx)
{
    int result;
    int64_t auto_id;

    if ((result=dao_spool_set_base_path_inode(server_ctx->dao_ctx)) != 0) {
        return result;
    }

    if ((result=dao_spool_get_auto_id(server_ctx->dao_ctx, &auto_id)) != 0) {
        return result;
    }

    if (AUTO_ID_INITIAL > auto_id) {
        auto_id = AUTO_ID_INITIAL;
    }
    FC_ATOMIC_SET(adb_ctx.pool.auto_id, auto_id);
    return 0;
}

int adb_load_data(AuthServerContext *server_ctx)
{
    const int alloc_size_once = 64 * 1024;
    const int discard_size = 8;
    int result;
    struct fast_mpool_man mpool;
    FCFSAuthUserArray user_array;

    if ((result=auth_db_init()) != 0) {
        return result;
    }

    if ((result=fast_mpool_init(&mpool, alloc_size_once,
                    discard_size)) != 0)
    {
        return result;
    }

    fcfs_auth_user_init_array(&user_array);
    if ((result=dao_user_list(server_ctx->dao_ctx,
                    &mpool, &user_array)) != 0)
    {
        return result;
    }

    result = convert_user_array(server_ctx, &mpool, &user_array);
    fcfs_auth_user_free_array(&user_array);
    fast_mpool_destroy(&mpool);

    if (result != 0) {
        return result;
    }
    return load_pool_auto_id(server_ctx);
}

int adb_check_generate_admin_user(AuthServerContext *server_ctx)
{
    int result;
    bool need_create;
    unsigned char passwd[FCFS_AUTH_PASSWD_LEN];
    FCFSAuthUserInfo user;

    if (ADMIN_GENERATE_MODE == AUTH_ADMIN_GENERATE_MODE_FIRST) {
        need_create = (adb_ctx.user.count == 0);
    } else {
        need_create = adb_user_get(server_ctx,
                &ADMIN_GENERATE_USERNAME) == NULL;
    }

    if (need_create) {
        user.name = ADMIN_GENERATE_USERNAME;
        fcfs_auth_generate_passwd(passwd);
        FC_SET_STRING_EX(user.passwd, (char *)passwd, FCFS_AUTH_PASSWD_LEN);
        user.priv = FCFS_AUTH_USER_PRIV_ALL;
        if ((result=adb_user_create(server_ctx, &user)) == 0) {
            if ((result=fcfs_auth_save_passwd(ADMIN_GENERATE_KEY_FILENAME.
                            str, passwd)) == 0)
            {
                logInfo("file: "__FILE__", line: %d, "
                        "scret key for user \"%s\" store to file: %s",
                        __LINE__, ADMIN_GENERATE_USERNAME.str,
                        ADMIN_GENERATE_KEY_FILENAME.str);
            }
        }
        return result;
    } else {
        return 0;
    }
}

static inline DBGrantedPoolInfo *granted_pool_get(
        const DBUserInfo *dbuser, const int64_t pool_id)
{
    DBGrantedPoolInfo target;

    target.granted.pool_id = pool_id;
    return (DBGrantedPoolInfo *)uniq_skiplist_find(
            dbuser->storage_pools.granted, &target);
}

int adb_granted_create(AuthServerContext *server_ctx, const string_t *username,
        FCFSAuthGrantedPoolInfo *granted)
{
    const bool addto_backend = true;
    bool changed;
    int result;
    DBUserInfo *dbuser;
    DBGrantedPoolInfo *dbgranted;

    dbgranted = NULL;
    PTHREAD_MUTEX_LOCK(&adb_ctx.lock);
    dbuser = user_get(server_ctx, username);
    if (dbuser != NULL) {
        if (dbuser->user.status == FCFS_AUTH_USER_STATUS_NORMAL) {
            dbgranted = granted_pool_get(dbuser, granted->pool_id);
        } else {
            dbuser = NULL;
        }
    }
    PTHREAD_MUTEX_UNLOCK(&adb_ctx.lock);

    /*
    logInfo("username: %.*s, ptr: %p, dbgranted: %p", username->len,
            username->str, user, dbgranted);
            */

    if (dbuser == NULL) {
        return ENOENT;
    }

    if (dbgranted != NULL) {
        changed = !(dbgranted->granted.privs.fdir == granted->privs.fdir &&
                dbgranted->granted.privs.fstore == granted->privs.fstore);
        if (!changed) {
            return 0;
        }
    } else {
        changed = false;
    }

    if ((result=granted_pool_create(server_ctx, dbuser,
                    &dbgranted, granted, addto_backend)) != 0)
    {
        return result;
    }

    if (changed) {
        g_db_priv_change_callbacks.pool_priv_changed(dbuser->user.id,
                dbgranted->granted.pool_id, &dbgranted->granted.privs);
    }
    return 0;
}

int adb_granted_remove(AuthServerContext *server_ctx,
        const string_t *username, const int64_t pool_id)
{
    int result;
    DBUserInfo *dbuser;
    DBGrantedPoolInfo *dbgranted;

    dbgranted = NULL;
    PTHREAD_MUTEX_LOCK(&adb_ctx.lock);
    dbuser = user_get(server_ctx, username);
    if (dbuser != NULL) {
        if (dbuser->user.status == FCFS_AUTH_USER_STATUS_NORMAL) {
            dbgranted = granted_pool_get(dbuser, pool_id);
        }
    }
    PTHREAD_MUTEX_UNLOCK(&adb_ctx.lock);

    if (dbgranted != NULL) {
        if ((result=dao_granted_remove(server_ctx->dao_ctx,
                        username, pool_id)) == 0)
        {
            FCFSAuthSPoolPriviledges privs;
            privs.fdir = privs.fstore = FCFS_AUTH_POOL_ACCESS_NONE;
            g_db_priv_change_callbacks.pool_priv_changed(dbuser->user.id,
                    dbgranted->granted.pool_id, &privs);

            PTHREAD_MUTEX_LOCK(&adb_ctx.lock);
            uniq_skiplist_delete(dbuser->storage_pools.granted, dbgranted);
            PTHREAD_MUTEX_UNLOCK(&adb_ctx.lock);
        }
        return result;
    } else {
        return ENOENT;
    }
}

static inline void set_gpool_full_info(FCFSAuthGrantedPoolFullInfo *gf,
        const DBGrantedPoolInfo *dbgpool)
{
    gf->granted = dbgpool->granted;
    gf->username = dbgpool->sp->user->user.name;
    gf->pool_name = dbgpool->sp->pool.name;
}

int adb_granted_full_get(AuthServerContext *server_ctx, const string_t
        *username, const int64_t pool_id, FCFSAuthGrantedPoolFullInfo *gf)
{
    DBUserInfo *dbuser;
    DBGrantedPoolInfo *dbgranted;

    dbgranted = NULL;
    PTHREAD_MUTEX_LOCK(&adb_ctx.lock);
    dbuser = user_get(server_ctx, username);
    if (dbuser != NULL) {
        if (dbuser->user.status == FCFS_AUTH_USER_STATUS_NORMAL) {
            if ((dbgranted=granted_pool_get(dbuser, pool_id)) != NULL) {
                set_gpool_full_info(gf, dbgranted);
            }
        }
    }
    PTHREAD_MUTEX_UNLOCK(&adb_ctx.lock);

    return (dbgranted != NULL) ? 0 : ENOENT;
}

int adb_granted_privs_get(AuthServerContext *server_ctx,
        const DBUserInfo *dbuser, const DBStoragePoolInfo *dbpool,
        FCFSAuthSPoolPriviledges *privs)
{
    DBGrantedPoolInfo *dbgranted;
    int result;

    PTHREAD_MUTEX_LOCK(&adb_ctx.lock);
    if (dbpool->user == dbuser) {
        privs->fdir = privs->fstore = FCFS_AUTH_POOL_ACCESS_ALL;
        result = 0;
    } else if ((dbgranted=granted_pool_get(dbuser,
                    dbpool->pool.id)) != NULL)
    {
        *privs = dbgranted->granted.privs;
        result = 0;
    } else {
        result = ENOENT;
    }
    PTHREAD_MUTEX_UNLOCK(&adb_ctx.lock);

    return result;
}

int adb_granted_list(AuthServerContext *server_ctx, const string_t *username,
        const SFListLimitInfo *limit, FCFSAuthGrantedPoolArray *array)
{
    UniqSkiplistIterator it;
    DBUserInfo *user;
    DBGrantedPoolInfo *dbgpool;
    int result;

    array->count = 0;
    if ((result=fcfs_auth_gpool_check_realloc_array(
                    array, limit->count)) != 0)
    {
        return result;
    }

    PTHREAD_MUTEX_LOCK(&adb_ctx.lock);
    user = user_get(server_ctx, username);
    if (user != NULL) {
        uniq_skiplist_iterator_at(user->storage_pools.granted,
                limit->offset, &it);
        while ((array->count < limit->count) && (dbgpool=(DBGrantedPoolInfo *)
                    uniq_skiplist_next(&it)) != NULL)
        {
            set_gpool_full_info(array->gpools + array->count, dbgpool);
            array->count++;
        }
    } else {
        result = ENOENT;
    }
    PTHREAD_MUTEX_UNLOCK(&adb_ctx.lock);

    return result;
}

int64_t adb_spool_get_auto_id(AuthServerContext *server_ctx)
{
    return __sync_add_and_fetch(&adb_ctx.pool.auto_id, 0);
}

int adb_spool_inc_auto_id(AuthServerContext *server_ctx)
{
    int64_t auto_id;
    auto_id = __sync_add_and_fetch(&adb_ctx.pool.auto_id, 1);
    return dao_spool_set_auto_id(server_ctx->dao_ctx, auto_id);
}

int adb_spool_next_auto_id(AuthServerContext *server_ctx, int64_t *auto_id)
{
    *auto_id = __sync_add_and_fetch(&adb_ctx.pool.auto_id, 1);
    return dao_spool_set_auto_id(server_ctx->dao_ctx, *auto_id);
}

DBStoragePoolInfo *adb_spool_global_get(const string_t *poolname)
{
    DBStoragePoolInfo *dbpool;

    PTHREAD_MUTEX_LOCK(&adb_ctx.lock);
    dbpool = get_spool_by_name(poolname);
    PTHREAD_MUTEX_UNLOCK(&adb_ctx.lock);

    return dbpool;
}
