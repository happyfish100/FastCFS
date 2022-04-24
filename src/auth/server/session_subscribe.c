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

#include <limits.h>
#include <sys/stat.h>
#include <sys/xattr.h>
#include "fastcommon/shared_func.h"
#include "fastcommon/sched_thread.h"
#include "fastcommon/logger.h"
#include "fastcommon/locked_list.h"
#include "sf/sf_global.h"
#include "sf/sf_service.h"
#include "db/auth_db.h"
#include "server_global.h"
#include "cluster_handler.h"
#include "server_session.h"
#include "session_subscribe.h"

typedef struct server_session_subscribe_context {
    struct fast_mblock_man entry_allocator; //element: ServerSessionSubscribeEntry
    struct fast_mblock_man subs_allocator;  //element: ServerSessionSubscriber

    /* queue element: ServerSessionSubscriber */
    FCLockedList subscribers;

    /* queue element: ServerSessionFields */
    FCLockedList sessions;   //publish session only
} ServerSessionSubscribeContext;

static ServerSessionSubscribeContext subscribe_ctx;

static void set_session_subscribe_entry(const ServerSessionEntry *session,
        ServerSessionSubscribeEntry *subs_entry)
{
    ServerSessionFields *fields;

    subs_entry->operation = FCFS_AUTH_SESSION_OP_TYPE_CREATE;
    subs_entry->session_id = session->id_info.id;

    fields = (ServerSessionFields *)(session->fields);
    subs_entry->fields.user.id = fields->dbuser->user.id;
    subs_entry->fields.user.priv = fields->dbuser->user.priv;

    if (fields->dbpool != NULL) {
        subs_entry->fields.pool.id = fields->dbpool->pool.id;
        subs_entry->fields.pool.available = FCFS_AUTH_POOL_AVAILABLE(
                fields->dbpool->pool);
        subs_entry->fields.pool.privs = fields->pool_privs;
    } else {
        subs_entry->fields.pool.id = 0;
        subs_entry->fields.pool.available = 0;
        subs_entry->fields.pool.privs.fdir = 0;
        subs_entry->fields.pool.privs.fstore = 0;
    }
}

static int publish_entry_to_all_subscribers(
        const ServerSessionSubscribeEntry *src_entry)
{
    int result;
    bool notify;
    ServerSessionSubscriber *subscriber;
    ServerSessionSubscribeEntry *subs_entry;

    result = 0;
    PTHREAD_MUTEX_LOCK(&subscribe_ctx.subscribers.lock);
    fc_list_for_each_entry(subscriber, &subscribe_ctx.
            subscribers.head, dlink)
    {
        subs_entry = (ServerSessionSubscribeEntry *)
            fast_mblock_alloc_object(&subscribe_ctx.entry_allocator);
        if (subs_entry == NULL) {
            result = ENOMEM;
            break;
        }

        *subs_entry = *src_entry;
        fc_queue_push_ex(&subscriber->queue, subs_entry, &notify);
        if (notify) {
            cluster_subscriber_queue_push(subscriber);
        }
    }
    PTHREAD_MUTEX_UNLOCK(&subscribe_ctx.subscribers.lock);

    return result;
}

static inline int publish_session_to_all_subscribers(
        const ServerSessionEntry *session)
{
    ServerSessionSubscribeEntry subs_entry;

    set_session_subscribe_entry(session, &subs_entry);
    return publish_entry_to_all_subscribers(&subs_entry);
}

static void server_session_add_callback(ServerSessionEntry *session)
{
    ServerSessionFields *fields;

    fields = (ServerSessionFields *)(session->fields);
    if (fields->publish) {
        locked_list_add_tail(&fields->dlink, &subscribe_ctx.sessions);
        publish_session_to_all_subscribers(session);
    }
}

static void server_session_del_callback(ServerSessionEntry *session)
{
    ServerSessionFields *fields;
    ServerSessionSubscribeEntry subs_entry;

    fields = (ServerSessionFields *)(session->fields);
    if (fields->publish && MYSELF_IS_MASTER) {
        locked_list_del(&fields->dlink, &subscribe_ctx.sessions);

        memset(&subs_entry, 0, sizeof(subs_entry));
        subs_entry.operation = FCFS_AUTH_SESSION_OP_TYPE_REMOVE;
        subs_entry.session_id = session->id_info.id;
        publish_entry_to_all_subscribers(&subs_entry);
    }
}

ServerSessionCallbacks g_server_session_callbacks = {
    server_session_add_callback, server_session_del_callback
};

typedef struct {
    int64_t user_id;
    int64_t pool_id;
    const FCFSAuthSPoolPriviledges *pool_privs;
} ServerSessionMatchParam;

static void publish_matched_server_sessions(
        const ServerSessionMatchParam *mparam)
{
    ServerSessionFields *fields;
    ServerSessionEntry *session;
    int matched_count;

    if (fc_list_empty(&subscribe_ctx.subscribers.head)) {
        return;
    }

    matched_count = 0;
    PTHREAD_MUTEX_LOCK(&subscribe_ctx.sessions.lock);
    fc_list_for_each_entry(fields, &subscribe_ctx.sessions.head, dlink) {
        session = FCFS_AUTH_SERVER_SESSION_BY_FIELDS(fields);
        if (mparam->user_id != 0) {
            if (fields->dbuser->user.id != mparam->user_id) {
                continue;
            }
        }

        if (mparam->pool_id != 0) {
            if (!(fields->dbpool != NULL && fields->dbpool->
                        pool.id == mparam->pool_id))
            {
                continue;
            }
        }

        if (mparam->pool_privs != NULL) {
            fields->pool_privs = *mparam->pool_privs;
        }

        ++matched_count;
        publish_session_to_all_subscribers(session);
    }
    PTHREAD_MUTEX_UNLOCK(&subscribe_ctx.sessions.lock);

    if (matched_count > 0) {
    }
}

static void user_priv_change_callback(const int64_t user_id,
        const int64_t new_priv)
{
    ServerSessionMatchParam mparam;

    mparam.user_id = user_id;
    mparam.pool_id = 0;
    mparam.pool_privs = NULL;
    publish_matched_server_sessions(&mparam);
}

static void pool_priv_change_callback(const int64_t user_id,
    const int64_t pool_id, const FCFSAuthSPoolPriviledges *pool_privs)
{
    ServerSessionMatchParam mparam;

    mparam.user_id = user_id;
    mparam.pool_id = pool_id;
    mparam.pool_privs = pool_privs;
    publish_matched_server_sessions(&mparam);
}

static void pool_quota_avail_change_callback(
        const int64_t pool_id, const bool available)
{
    ServerSessionMatchParam mparam;

    mparam.user_id = 0;
    mparam.pool_id = pool_id;
    mparam.pool_privs = NULL;
    publish_matched_server_sessions(&mparam);
}

DBPrivChangeCallbacks g_db_priv_change_callbacks = {
    user_priv_change_callback,
    pool_priv_change_callback,
    pool_quota_avail_change_callback
};

static int push_all_sessions_to_queue(ServerSessionSubscriber *subscriber)
{
    int result;
    ServerSessionFields *fields;
    ServerSessionEntry *session;
    ServerSessionSubscribeEntry *head;
    ServerSessionSubscribeEntry *tail;
    ServerSessionSubscribeEntry *subs_entry;

    result = 0;
    head = tail = NULL;
    PTHREAD_MUTEX_LOCK(&subscribe_ctx.sessions.lock);
    fc_list_for_each_entry(fields, &subscribe_ctx.sessions.head, dlink) {
        session = FCFS_AUTH_SERVER_SESSION_BY_FIELDS(fields);
        if (!session->id_info.fields.publish) {
            continue;
        }

        subs_entry = (ServerSessionSubscribeEntry *)
            fast_mblock_alloc_object(&subscribe_ctx.entry_allocator);
        if (subs_entry == NULL) {
            result = ENOMEM;
            break;
        }

        set_session_subscribe_entry(session, subs_entry);

        if (head == NULL) {
            head = subs_entry;
        } else {
            tail->next = subs_entry;
        }
        tail = subs_entry;
    }
    PTHREAD_MUTEX_UNLOCK(&subscribe_ctx.sessions.lock);

    if (result == 0 && head != NULL) {
        struct fc_queue_info qinfo;

        tail->next = NULL;
        qinfo.head = head;
        qinfo.tail = tail;
        fc_queue_push_queue_to_tail_silence(&subscriber->queue, &qinfo);
    }
    return result;
}

int subscriber_alloc_init_func(ServerSessionSubscriber *subscriber, void *args)
{
    FC_INIT_LIST_HEAD(&subscriber->dlink);
    return fc_queue_init(&subscriber->queue, (long)
            (&((ServerSessionSubscribeEntry *)NULL)->next));
}

int session_subscribe_init()
{
    int result;

    if ((result=fast_mblock_init_ex1(&subscribe_ctx.entry_allocator,
                    "subscribe_entry", sizeof(ServerSessionSubscribeEntry),
                    8 * 1024, 0, NULL, NULL, true)) != 0)
    {
        return result;
    }

    if ((result=fast_mblock_init_ex1(&subscribe_ctx.subs_allocator,
                    "session_subscriber", sizeof(ServerSessionSubscriber),
                    1024, 0, (fast_mblock_object_init_func)
                    subscriber_alloc_init_func, NULL, true)) != 0)
    {
        return result;
    }

    if ((result=locked_list_init(&subscribe_ctx.subscribers)) != 0) {
        return result;
    }

    if ((result=locked_list_init(&subscribe_ctx.sessions)) != 0) {
        return result;
    }

    return 0;
}

void session_subscribe_destroy()
{
}

ServerSessionSubscriber *session_subscribe_alloc()
{
    return (ServerSessionSubscriber *)fast_mblock_alloc_object(
            &subscribe_ctx.subs_allocator);
}

void session_subscribe_register(ServerSessionSubscriber *subscriber)
{
    push_all_sessions_to_queue(subscriber);
    locked_list_add_tail(&subscriber->dlink,
            &subscribe_ctx.subscribers);
}

void session_subscribe_unregister(ServerSessionSubscriber *subscriber)
{
    locked_list_del(&subscriber->dlink, &subscribe_ctx.subscribers);
}

void session_subscribe_free_entries(ServerSessionSubscribeEntry *entry)
{
    ServerSessionSubscribeEntry *current;
    while (entry != NULL) {
        current = entry;
        entry = entry->next;

        fast_mblock_free_object(&subscribe_ctx.entry_allocator, current);
    }
}

void session_subscribe_release(ServerSessionSubscriber *subscriber)
{
    ServerSessionSubscribeEntry *entry;

    entry = (ServerSessionSubscribeEntry *)fc_queue_try_pop_all(
            &subscriber->queue);
    if (entry != NULL) {
        session_subscribe_free_entries(entry);
    }
    fast_mblock_free_object(&subscribe_ctx.subs_allocator, subscriber);
}

void session_subscribe_clear_session()
{
    ServerSessionFields *fields;
    ServerSessionFields *tmp;

    PTHREAD_MUTEX_LOCK(&subscribe_ctx.sessions.lock);
    fc_list_for_each_entry_safe(fields, tmp,
            &subscribe_ctx.sessions.head, dlink)
    {
        fc_list_del_init(&fields->dlink);
    }
    PTHREAD_MUTEX_UNLOCK(&subscribe_ctx.sessions.lock);
}
