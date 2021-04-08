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
#include "sf/sf_global.h"
#include "db/auth_db.h"
#include "server_global.h"
#include "session_subscribe.h"

typedef struct {
    struct fc_list_head head;
    pthread_mutex_t lock;
} AuthServerDLinkChain;

typedef struct server_session_subscribe_context {
    struct fast_mblock_man entry_allocator; //element: ServerSessionSubscribeEntry
    struct fast_mblock_man subs_allocator;  //element: ServerSessionSubscriber

    /* queue element: ServerSessionSubscriber */
    AuthServerDLinkChain subscribers;

    /* queue element: ServerSessionFields */
    AuthServerDLinkChain sessions;   //publish session only
} ServerSessionSubscribeContext;

static ServerSessionSubscribeContext subscribe_ctx;

static void server_session_add_callback(ServerSessionEntry *session)
{
    ServerSessionFields *fields;

    fields = (ServerSessionFields *)(session->fields);
    if (fields->publish) {
        PTHREAD_MUTEX_LOCK(&subscribe_ctx.sessions.lock);
        fc_list_add_tail(&fields->dlink, &subscribe_ctx.sessions.head);
        PTHREAD_MUTEX_UNLOCK(&subscribe_ctx.sessions.lock);
    }
}

static void server_session_del_callback(ServerSessionEntry *session)
{
    ServerSessionFields *fields;

    fields = (ServerSessionFields *)(session->fields);
    if (fields->publish) {
        PTHREAD_MUTEX_LOCK(&subscribe_ctx.sessions.lock);
        fc_list_del_init(&fields->dlink);
        PTHREAD_MUTEX_UNLOCK(&subscribe_ctx.sessions.lock);
    }
}

ServerSessionCallbacks g_server_session_callbacks = {
    server_session_add_callback, server_session_del_callback
};

static void set_session_entry(const ServerSessionEntry *session,
        ServerSessionSubscribeEntry *subs_entry)
{
    ServerSessionFields *fields;

    subs_entry->session_id = session->session_id;

    fields = (ServerSessionFields *)(session->fields);
    subs_entry->fields.user.id = fields->dbuser->user.id;
    subs_entry->fields.user.priv = fields->dbuser->user.priv;

    subs_entry->fields.pool.id = fields->dbpool->pool.id;
    subs_entry->fields.pool.available = (fields->dbpool->pool.quota ==
            FCFS_AUTH_UNLIMITED_QUOTA_VAL) || (fields->dbpool->pool.used <
                fields->dbpool->pool.quota);
    subs_entry->fields.pool.privs = fields->pool_privs;
}

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
        subs_entry = (ServerSessionSubscribeEntry *)
            fast_mblock_alloc_object(&subscribe_ctx.entry_allocator);
        if (subs_entry == NULL) {
            result = ENOMEM;
            break;
        }

        session = ((ServerSessionEntry *)fields) - 1;
        set_session_entry(session, subs_entry);

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


static inline int init_dlink_chain(AuthServerDLinkChain *chain)
{
    int result;
    if ((result=init_pthread_lock(&chain->lock)) != 0) {
        return result;
    }

    FC_INIT_LIST_HEAD(&chain->head);
    return 0;
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
                    1024, 0, (fast_mblock_alloc_init_func)
                    subscriber_alloc_init_func, NULL, true)) != 0)
    {
        return result;
    }

    if ((result=init_dlink_chain(&subscribe_ctx.subscribers)) != 0) {
        return result;
    }

    if ((result=init_dlink_chain(&subscribe_ctx.sessions)) != 0) {
        return result;
    }

    return 0;
}

void session_subscribe_destroy()
{
}

ServerSessionSubscriber *session_subscribe_register()
{
    ServerSessionSubscriber *subscriber;

    subscriber = (ServerSessionSubscriber *)fast_mblock_alloc_object(
            &subscribe_ctx.subs_allocator);
    if (subscriber != NULL) {
        push_all_sessions_to_queue(subscriber);

        PTHREAD_MUTEX_LOCK(&subscribe_ctx.subscribers.lock);
        fc_list_add_tail(&subscriber->dlink,
                &subscribe_ctx.subscribers.head);
        PTHREAD_MUTEX_UNLOCK(&subscribe_ctx.subscribers.lock);
    }
    return subscriber;
}

void session_subscribe_unregister(ServerSessionSubscriber *subscriber)
{
    ServerSessionSubscribeEntry *entry;
    ServerSessionSubscribeEntry *current;

    PTHREAD_MUTEX_LOCK(&subscribe_ctx.subscribers.lock);
    fc_list_del_init(&subscriber->dlink);
    PTHREAD_MUTEX_UNLOCK(&subscribe_ctx.subscribers.lock);

    entry = (ServerSessionSubscribeEntry *)fc_queue_try_pop_all(
            &subscriber->queue);
    while (entry != NULL) {
        current = entry;
        entry = entry->next;

        fast_mblock_free_object(&subscribe_ctx.entry_allocator, current);
    }

    fast_mblock_free_object(&subscribe_ctx.subs_allocator, subscriber);
}
