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
#include "auth_func.h"
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
    int fields_size;
    volatile uint16_t sn;  //for generate session id
    ServerSessionAllocatorArray allocator_array;
    ServerSessionLockArray lock_array;
    ServerSessionHashtable htable;
    ServerSessionCallbacks callbacks;
} ServerSessionContext;

ServerSessionConfig g_server_session_cfg;
static ServerSessionContext session_ctx = {0, 0, {0, 0, NULL}};

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

void server_session_cfg_to_string(char *buff, const int size)
{
    snprintf(buff, size, "session {shared_allocator_count: %d, "
            "shared_lock_count: %d, hashtable_capacity: %d, "
            "validate_key_filename: %s}",
            g_server_session_cfg.shared_allocator_count,
            g_server_session_cfg.shared_lock_count,
            g_server_session_cfg.hashtable_capacity,
            g_server_session_cfg.validate_key_filename.str);
}

static int load_session_validate_key(IniFullContext *ini_ctx)
{
    int result;
    string_t validate_key_filename;

    validate_key_filename.str = iniGetStrValue(ini_ctx->section_name,
            "validate_key_filename", ini_ctx->context);
    if (validate_key_filename.str == NULL ||
            *(validate_key_filename.str) == '\0')
    {
        validate_key_filename.str =
            "/etc/fastcfs/auth/keys/session_validate.key";
    }
    validate_key_filename.len = strlen(validate_key_filename.str);
    if ((result=fcfs_auth_load_passwd(validate_key_filename.str,
                    g_server_session_cfg.validate_key_buff)) != 0)
    {
        logError("file: "__FILE__", line: %d, "
                "config file: %s, validate_key_filename: %s, "
                "load password fail, ret code: %d", __LINE__,
                ini_ctx->filename, validate_key_filename.str, result);
        return result;
    }
    g_server_session_cfg.validate_key.str = (char *)
        g_server_session_cfg.validate_key_buff;
    g_server_session_cfg.validate_key.len = FCFS_AUTH_PASSWD_LEN;

    g_server_session_cfg.validate_key_filename.str = (char *)
        fc_malloc(validate_key_filename.len + 1);
    if (g_server_session_cfg.validate_key_filename.str == NULL) {
        return ENOMEM;
    }

    g_server_session_cfg.validate_key_filename.len =
        validate_key_filename.len;
    memcpy(g_server_session_cfg.validate_key_filename.str,
            validate_key_filename.str,
            validate_key_filename.len + 1);
    return 0;
}

static int do_load_session_cfg(const char *session_filename)
{
    IniContext ini_context;
    IniFullContext ini_ctx;
    int result;

    if ((result=iniLoadFromFile(session_filename, &ini_context)) != 0) {
        logError("file: "__FILE__", line: %d, "
                "load session config file \"%s\" fail, ret code: %d",
                __LINE__, session_filename, result);
        return result;
    }

    FAST_INI_SET_FULL_CTX_EX(ini_ctx, session_filename, NULL, &ini_context);
    session_ctx.allocator_array.count = iniGetIntCorrectValue(
            &ini_ctx, "shared_allocator_count",
            SESSION_DEFAULT_SHARED_ALLOCATOR_COUNT,
            SESSION_MIN_SHARED_ALLOCATOR_COUNT,
            SESSION_MAX_SHARED_ALLOCATOR_COUNT);

    session_ctx.lock_array.count = iniGetInt64CorrectValue(
            &ini_ctx, "shared_lock_count",
            SESSION_DEFAULT_SHARED_LOCK_COUNT,
            SESSION_MIN_SHARED_LOCK_COUNT,
            SESSION_MAX_SHARED_LOCK_COUNT);

    session_ctx.htable.capacity = iniGetIntCorrectValue(
            &ini_ctx, "hashtable_capacity",
            SESSION_DEFAULT_HASHTABLE_CAPACITY,
            SESSION_MIN_HASHTABLE_CAPACITY,
            SESSION_MAX_HASHTABLE_CAPACITY);

    g_server_session_cfg.shared_allocator_count =
        session_ctx.allocator_array.count;
    g_server_session_cfg.shared_lock_count = session_ctx.lock_array.count;
    g_server_session_cfg.hashtable_capacity = session_ctx.htable.capacity;

    result = load_session_validate_key(&ini_ctx);
    iniFreeContext(&ini_context);
    return result;
}

static int load_session_config(IniFullContext *ini_ctx)
{
    char *session_config_filename;
    char full_session_filename[PATH_MAX];

    session_config_filename = iniGetStrValue(NULL,
            "session_config_filename", ini_ctx->context);
    if (session_config_filename == NULL || *session_config_filename == '\0') {
        logWarning("file: "__FILE__", line: %d, "
                "config file: %s, item \"session_config_filename\" "
                "not exist or empty", __LINE__, ini_ctx->filename);

        session_ctx.allocator_array.count =
            SESSION_DEFAULT_SHARED_ALLOCATOR_COUNT;
        session_ctx.lock_array.count = SESSION_DEFAULT_SHARED_LOCK_COUNT;
        session_ctx.htable.capacity = SESSION_DEFAULT_HASHTABLE_CAPACITY;
        return 0;
    }

    resolve_path(ini_ctx->filename, session_config_filename,
            full_session_filename, sizeof(full_session_filename));
    return do_load_session_cfg(full_session_filename);
}

static int server_session_alloc_init(ServerSessionHashEntry *session,
        struct fast_mblock_man *allocator)
{
    session->allocator = allocator;
    session->entry.fields = session + 1;
    return 0;
}

static int init_allocator_array(ServerSessionAllocatorArray *array)
{
    int result;
    int bytes;
    int element_size;
    struct fast_mblock_man *mblock;
    struct fast_mblock_man *end;

    bytes = sizeof(struct fast_mblock_man) * array->count;
    array->allocators = (struct fast_mblock_man *)fc_malloc(bytes);
    if (array->allocators == NULL) {
        return ENOMEM;
    }

    element_size = sizeof(ServerSessionHashEntry) + session_ctx.fields_size;
    end = array->allocators + array->count;
    for (mblock=array->allocators; mblock<end; mblock++) {
        if ((result=fast_mblock_init_ex1(mblock, "session_entry",
                        element_size, 4096, 0,
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

int server_session_init_ex(IniFullContext *ini_ctx, const int fields_size,
        ServerSessionCallbacks *callbacks)
{
    int result;

    if ((result=load_session_config(ini_ctx)) != 0) {
        return result;
    }

    session_ctx.fields_size = fields_size;
    if ((result=init_allocator_array(&session_ctx.allocator_array)) != 0) {
        return result;
    }

    if ((result=init_lock_array(&session_ctx.lock_array)) != 0) {
        return result;
    }

    if ((result=init_hashtable(&session_ctx.htable)) != 0) {
        return result;
    }

    if (callbacks != NULL) {
        session_ctx.callbacks =  *callbacks;
    } else {
        session_ctx.callbacks.add_func = NULL;
        session_ctx.callbacks.del_func = NULL;
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

ServerSessionEntry *server_session_add(const ServerSessionEntry *entry,
        const bool publish)
{
    int result;
    ServerSessionIdInfo sid;
    bool replace;
    struct fast_mblock_man *allocator;
    ServerSessionHashEntry *se;

    allocator = session_ctx.allocator_array.allocators +
        (__sync_fetch_and_add(&session_ctx.allocator_array.next, 1) %
         session_ctx.allocator_array.count);
    se = (ServerSessionHashEntry *)fast_mblock_alloc_object(allocator);
    if (se == NULL) {
        return NULL;
    }

    memcpy(se->entry.fields, entry->fields, session_ctx.fields_size);
    if (entry->session_id == 0) {
        replace = false;
        do {
            sid.fields.ts = g_current_time;
            sid.fields.publish = (publish ? 1 : 0);
            sid.fields.rn = (int64_t)rand() * 32768LL / RAND_MAX;
            sid.fields.sn = __sync_add_and_fetch(&session_ctx.sn, 1);
            se->entry.session_id = sid.id;
            logInfo("session_id: %"PRId64", ts: %d, publish: %d, "
                    "rand: %u, sn: %u", se->entry.session_id, sid.fields.ts,
                    publish, sid.fields.rn, sid.fields.sn);
            result = session_htable_insert(se, replace);
        } while (result == EEXIST);
    } else {
        replace = true;
        se->entry.session_id = entry->session_id;
        session_htable_insert(se, replace);
    }

    if (session_ctx.callbacks.add_func != NULL) {
        session_ctx.callbacks.add_func(&se->entry);
    }
    return &se->entry;
}

int server_session_get_fields(const uint64_t session_id, void *fields)
{
    int result;
    ServerSessionHashEntry *previous;
    ServerSessionHashEntry *found;

    SESSION_SET_BUCKET_AND_LOCK(session_ctx.htable, session_id);
    PTHREAD_MUTEX_LOCK(lock);
    if ((found=session_htable_find(bucket, session_id,
                    &previous)) != NULL)
    {
        memcpy(fields, found->entry.fields, session_ctx.fields_size);
        result = 0;
    } else {
        result = ENOENT;
    }
    PTHREAD_MUTEX_UNLOCK(lock);

    return result;
}

int server_session_user_priv_granted(const uint64_t session_id,
        const int64_t the_priv)
{
    int result;
    ServerSessionHashEntry *previous;
    ServerSessionHashEntry *found;
    SessionSyncedFields *fields;

    SESSION_SET_BUCKET_AND_LOCK(session_ctx.htable, session_id);
    PTHREAD_MUTEX_LOCK(lock);
    if ((found=session_htable_find(bucket, session_id,
                    &previous)) != NULL)
    {
        fields = (SessionSyncedFields *)found->entry.fields;
        if ((fields->user.priv & the_priv) == the_priv) {
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
    SessionSyncedFields *fields;

    SESSION_SET_BUCKET_AND_LOCK(session_ctx.htable, session_id);
    PTHREAD_MUTEX_LOCK(lock);
    if ((found=session_htable_find(bucket, session_id,
                    &previous)) != NULL)
    {
        fields = (SessionSyncedFields *)found->entry.fields;
        if ((fields->pool.privs.fstore & the_priv) == the_priv) {
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
        const int the_priv)
{
    int result;
    ServerSessionHashEntry *previous;
    ServerSessionHashEntry *found;
    SessionSyncedFields *fields;

    SESSION_SET_BUCKET_AND_LOCK(session_ctx.htable, session_id);
    PTHREAD_MUTEX_LOCK(lock);
    if ((found=session_htable_find(bucket, session_id,
                    &previous)) != NULL)
    {
        fields = (SessionSyncedFields *)found->entry.fields;
        if ((fields->pool.privs.fdir & the_priv) == the_priv) {
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
    }
    PTHREAD_MUTEX_UNLOCK(lock);

    if (found != NULL) {
        if (session_ctx.callbacks.del_func != NULL) {
            session_ctx.callbacks.del_func(&found->entry);
        }

        fast_mblock_free_object(found->allocator, found);
        return 0;
    } else {
        return ENOENT;
    }
}
