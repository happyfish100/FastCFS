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
#include "../../common/auth_func.h"
#include "../server_global.h"
#include "dao/user.h"
#include "auth_db.h"

#define SKIPLIST_INIT_LEVEL_COUNT   2

typedef struct db_storage_pool_info {
    FCFSAuthStoragePoolInfo pool;
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
    struct {
        struct uniq_skiplist *created;  //element: DBStoragePoolInfo
        struct uniq_skiplist *granted;  //element: DBStoragePoolGranted
    } storage_pools;
} DBUserInfo;

typedef struct auth_db_context {
    pthread_mutex_t lock;
    struct fast_allocator_context name_acontext;

    struct {
        int count;
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
    return fc_string_compare(&user1->user.name, &user2->user.name);
}

static int created_pool_cmp(const DBStoragePoolInfo *pool1,
        const DBStoragePoolInfo *pool2)
{
    return fc_string_compare(&pool1->pool.name, &pool2->pool.name);
}

static int granted_pool_cmp(const DBStoragePoolGranted *pg1,
        const DBStoragePoolGranted *pg2)
{
    return fc_string_compare(&pg1->sp->pool.name, &pg2->sp->pool.name);
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
    }

    PTHREAD_MUTEX_LOCK(&adb_ctx.lock);
    (*dbuser)->user.status = FCFS_AUTH_USER_STATUS_NORMAL;
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

const FCFSAuthUserInfo *adb_user_get(AuthServerContext *server_ctx,
        const string_t *username)
{
    DBUserInfo *user;

    PTHREAD_MUTEX_LOCK(&adb_ctx.lock);
    user = user_get(server_ctx, username);
    PTHREAD_MUTEX_UNLOCK(&adb_ctx.lock);

    if (user != NULL && user->user.status == FCFS_AUTH_USER_STATUS_NORMAL) {
        return &user->user;
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

int adb_user_list(AuthServerContext *server_ctx, FCFSAuthUserArray *array)
{
    UniqSkiplistIterator it;
    DBUserInfo *dbuser;
    int result;

    result = 0;
    PTHREAD_MUTEX_LOCK(&adb_ctx.lock);
    uniq_skiplist_iterator(adb_ctx.user.sl_pair.skiplist, &it);
    while ((dbuser=(DBUserInfo *)uniq_skiplist_next(&it)) != NULL) {
        if (dbuser->user.status == FCFS_AUTH_USER_STATUS_NORMAL) {
            if ((result=fcfs_auth_user_check_realloc_array(array,
                            array->count + 1)) != 0)
            {
                break;
            }
            array->users[array->count++] = dbuser->user;
        }
    }
    PTHREAD_MUTEX_UNLOCK(&adb_ctx.lock);

    return result;
}

static int convert_user_array(AuthServerContext *server_ctx,
        const FCFSAuthUserArray *user_array)
{
    const bool addto_backend = false;
    int result;
    DBUserInfo *dbuser;
    const FCFSAuthUserInfo *user;
    const FCFSAuthUserInfo *end;

    end = user_array->users + user_array->count;
    for (user=user_array->users; user<end; user++) {
        dbuser = NULL;
        if ((result=user_create(server_ctx, &dbuser,
                        user, addto_backend)) != 0)
        {
            return result;
        }

        //TODO
    }

    return 0;
}

int adb_load_data(AuthServerContext *server_ctx)
{
    const int alloc_size_once = 64 * 1024;
    const int discard_size = 8;
    int result;
    struct fast_mpool_man mpool;
    FCFSAuthUserArray user_array;

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

    result = convert_user_array(server_ctx, &user_array);
    fcfs_auth_user_free_array(&user_array);
    fast_mpool_destroy(&mpool);
    return result;
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
