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

//cluster_handler.c

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <limits.h>
#include <fcntl.h>
#include "fastcommon/logger.h"
#include "fastcommon/sockopt.h"
#include "fastcommon/shared_func.h"
#include "fastcommon/pthread_func.h"
#include "fastcommon/sched_thread.h"
#include "fastcommon/ioevent_loop.h"
#include "sf/sf_util.h"
#include "sf/sf_func.h"
#include "sf/sf_nio.h"
#include "sf/sf_global.h"
#include "common/auth_proto.h"
#include "db/auth_db.h"
#include "server_global.h"
#include "server_func.h"
#include "session_subscribe.h"
#include "common_handler.h"
#include "cluster_handler.h"

int cluster_handler_init()
{
    return 0;
}

int cluster_handler_destroy()
{   
    return 0;
}

static void session_subscriber_cleanup(AuthServerContext *server_ctx,
        ServerSessionSubscriber *subscriber)
{
    session_subscribe_unregister(subscriber);

    if (__sync_bool_compare_and_swap(&subscriber->nio.in_queue, 1, 0)) {
        locked_list_del(&subscriber->nio.dlink,
                &server_ctx->cluster.subscribers);
    }

    session_subscribe_release(subscriber);
}

void cluster_task_finish_cleanup(struct fast_task_info *task)
{
    switch (SERVER_TASK_TYPE) {
        case AUTH_SERVER_TASK_TYPE_SUBSCRIBE:
            if (SESSION_SUBSCRIBER != NULL) {
                session_subscriber_cleanup(SERVER_CTX, SESSION_SUBSCRIBER);
                SESSION_SUBSCRIBER = NULL;
            }

            logInfo("file: "__FILE__", line: %d, "
                    "session subscriber %s:%u offline", __LINE__,
                    task->client_ip, task->port);
            SERVER_TASK_TYPE = SF_SERVER_TASK_TYPE_NONE;
            break;
        default:
            break;
    }

    sf_task_finish_clean_up(task);
}

void cluster_subscriber_queue_push_ex(ServerSessionSubscriber *subscriber,
        const bool notify)
{
    AuthServerContext *server_ctx;

    if (__sync_bool_compare_and_swap(&subscriber->nio.in_queue, 0, 1)) {
        server_ctx = (AuthServerContext *)subscriber->
            nio.task->thread_data->arg;
        locked_list_add_tail(&subscriber->nio.dlink,
                &server_ctx->cluster.subscribers);
        if (notify) {
            ioevent_notify_thread(subscriber->nio.task->thread_data);
        }
    }
}

static inline ServerSessionSubscriber *cluster_subscriber_queue_pop(
        AuthServerContext *server_context)
{
    ServerSessionSubscriber *subscriber;

    PTHREAD_MUTEX_LOCK(&server_context->cluster.subscribers.lock);
    subscriber = fc_list_first_entry(&server_context->cluster.subscribers.head,
            ServerSessionSubscriber, nio.dlink);
    if (subscriber != NULL) {
        fc_list_del_init(&subscriber->nio.dlink);
        __sync_bool_compare_and_swap(&subscriber->nio.in_queue, 1, 0);
    }
    PTHREAD_MUTEX_UNLOCK(&server_context->cluster.subscribers.lock);

    return subscriber;
}

static int cluster_deal_session_subscribe(struct fast_task_info *task)
{
    FCFSAuthProtoSessionSubscribeReq *req;
    const DBUserInfo *dbuser;
    string_t username;
    string_t passwd;
    int result;

    if ((result=server_check_min_body_length(sizeof(
                        FCFSAuthProtoSessionSubscribeReq) + 1)) != 0)
    {
        return result;
    }

    req = (FCFSAuthProtoSessionSubscribeReq *)REQUEST.body;
    FC_SET_STRING_EX(username, req->up_pair.username.str,
            req->up_pair.username.len);
    FC_SET_STRING_EX(passwd, req->up_pair.passwd,
            FCFS_AUTH_PASSWD_LEN);
    if ((result=server_expect_body_length(
                    sizeof(FCFSAuthProtoSessionSubscribeReq)
                    + username.len)) != 0)
    {
        return result;
    }

    if (SERVER_TASK_TYPE != SF_SERVER_TASK_TYPE_NONE) {
        RESPONSE.error.length = sprintf(RESPONSE.error.message,
                "task type: %d != %d", SERVER_TASK_TYPE,
                SF_SERVER_TASK_TYPE_NONE);
        return EEXIST;
    }

    if (!((dbuser=adb_user_get(SERVER_CTX, &username)) != NULL &&
            fc_string_equal(&dbuser->user.passwd, &passwd)))
    {
        RESPONSE.error.length = sprintf(RESPONSE.error.message,
                "user login fail, username or password not correct");
        return EPERM;
    }

    if ((dbuser->user.priv & FCFS_AUTH_USER_PRIV_SUBSCRIBE_SESSION) == 0) {
        RESPONSE.error.length = sprintf(RESPONSE.error.message,
                "username: %.*s, permission denied for session "
                "subscription", username.len, username.str);
        return EPERM;
    }

    if ((SESSION_SUBSCRIBER=session_subscribe_alloc()) == NULL) {
        RESPONSE.error.length = sprintf(RESPONSE.error.message,
                "session subscribe register fail");
        return ENOMEM;
    }


    SESSION_SUBSCRIBER->nio.task = task;
    session_subscribe_register(SESSION_SUBSCRIBER);
    if (!fc_queue_empty(&SESSION_SUBSCRIBER->queue)) {
        cluster_subscriber_queue_push_ex(SESSION_SUBSCRIBER, false);
    }

    logInfo("file: "__FILE__", line: %d, "
            "session subscriber %s:%u joined", __LINE__,
            task->client_ip, task->port);

    SERVER_TASK_TYPE = AUTH_SERVER_TASK_TYPE_SUBSCRIBE;
    RESPONSE.header.cmd = FCFS_AUTH_SERVICE_PROTO_SESSION_SUBSCRIBE_RESP;
    return 0;
}

static int session_validate(const int64_t session_id,
        const FCFSAuthValidatePriviledgeType priv_type,
        const int64_t pool_id, const int64_t priv_required)
{
    int result;
    int64_t session_priv;
    ServerSessionFields fields;

    if ((result=server_session_get_fields(session_id, &fields)) != 0) {
        return result;
    }

    switch (priv_type) {
        case fcfs_auth_validate_priv_type_user:
            session_priv = (fields.dbuser != NULL) ? fields.dbuser->
                user.priv : (FCFS_AUTH_USER_PRIV_MONITOR_CLUSTER |
                        FCFS_AUTH_USER_PRIV_SUBSCRIBE_SESSION);
            break;
        case fcfs_auth_validate_priv_type_pool_fdir:
            /*
            //TODO
            if (fields.dbpool != NULL) {
                if (fields.dbpool->pool.id != pool_id) {
                    return EACCES;
                }
            }
            */
            session_priv = fields.pool_privs.fdir;
            break;
        case fcfs_auth_validate_priv_type_pool_fstore:
            session_priv = fields.pool_privs.fstore;
            break;
        default:
            session_priv = 0;
            break;
    }

    /*
    logInfo("session_id: %"PRId64", session_priv: %"PRId64", "
            "priv_required: %"PRId64", check result: %d",
            session_id, session_priv, priv_required,
            ((session_priv & priv_required) == priv_required) ? 0 : EPERM);
            */

    return ((session_priv & priv_required) == priv_required) ? 0 : EPERM;
}

static int cluster_deal_session_validate(struct fast_task_info *task)
{
    FCFSAuthProtoSessionValidateReq *req;
    FCFSAuthProtoSessionValidateResp *resp;
    int64_t session_id;
    int64_t pool_id;
    string_t validate_key;
    int64_t priv_required;
    FCFSAuthValidatePriviledgeType priv_type;
    int result;

    if ((result=server_expect_body_length(sizeof(*req))) != 0) {
        return result;
    }

    req = (FCFSAuthProtoSessionValidateReq *)REQUEST.body;
    session_id = buff2long(req->session_id);
    FC_SET_STRING_EX(validate_key, req->validate_key, FCFS_AUTH_PASSWD_LEN);
    priv_type = req->priv_type;
    pool_id = buff2long(req->pool_id);
    priv_required = buff2long(req->priv_required);
    if (!fc_string_equal(&validate_key, &g_server_session_cfg.validate_key)) {
        RESPONSE.error.length = sprintf(RESPONSE.error.message,
                "session validate key not correct");
        return EACCES;
    }

    if ((result=session_validate(session_id, priv_type, pool_id,
                    priv_required)) == SF_SESSION_ERROR_NOT_EXIST)
    {
        RESPONSE.error.length = sprintf(RESPONSE.error.message,
                "session id: %"PRId64" not exist", session_id);
        return result;
    }

    resp = (FCFSAuthProtoSessionValidateResp *)REQUEST.body;
    int2buff(result, resp->result);
    RESPONSE.header.body_len = sizeof(*resp);
    RESPONSE.header.cmd = FCFS_AUTH_SERVICE_PROTO_SESSION_VALIDATE_RESP;
    TASK_ARG->context.common.response_done = true;
    return 0;
}

int cluster_deal_task(struct fast_task_info *task, const int stage)
{
    int result;

    if (stage == SF_NIO_STAGE_CONTINUE) {
        if (task->continue_callback != NULL) {
            result = task->continue_callback(task);
        } else {
            result = RESPONSE_STATUS;
            if (result == TASK_STATUS_CONTINUE) {
                logError("file: "__FILE__", line: %d, "
                        "unexpect status: %d", __LINE__, result);
                result = EBUSY;
            }
        }
    } else {
        sf_proto_init_task_context(task, &TASK_CTX.common);
        switch (REQUEST.header.cmd) {
            case SF_PROTO_ACTIVE_TEST_REQ:
                RESPONSE.header.cmd = SF_PROTO_ACTIVE_TEST_RESP;
                result = sf_proto_deal_active_test(task, &REQUEST, &RESPONSE);
                break;
            case FCFS_AUTH_SERVICE_PROTO_SESSION_SUBSCRIBE_REQ:
                result =  cluster_deal_session_subscribe(task);
                break;
            case FCFS_AUTH_SERVICE_PROTO_SESSION_VALIDATE_REQ:
                result = cluster_deal_session_validate(task);
                break;
            default:
                RESPONSE.error.length = sprintf(RESPONSE.error.message,
                        "unkown cmd: %d", REQUEST.header.cmd);
                result = -EINVAL;
                break;
        }
    }

    if (result == TASK_STATUS_CONTINUE) {
        return 0;
    } else {
        RESPONSE_STATUS = result;
        return sf_proto_deal_task_done(task, &TASK_CTX.common);
    }
}

static int cluster_deal_queue(AuthServerContext *server_context,
        ServerSessionSubscriber *subscriber)
{
    struct fast_task_info *task;
    struct fc_queue_info qinfo;
    ServerSessionSubscribeEntry *previous;
    ServerSessionSubscribeEntry *entry;
    FCFSAuthProtoHeader *proto_header;
    FCFSAuthProtoSessionPushRespBodyHeader *body_header;
    FCFSAuthProtoSessionPushRespBodyPart *body_part;
    char *p;
    char *end;
    int count;
    int result;

    task = subscriber->nio.task;
    if (ioevent_is_canceled(task) || !sf_nio_task_is_idle(
                subscriber->nio.task))
    {
        return EAGAIN;
    }

    fc_queue_try_pop_to_queue(&subscriber->queue, &qinfo);
    if (qinfo.head == NULL) {
        return 0;
    }

    entry = NULL;
    entry = (ServerSessionSubscribeEntry *)qinfo.head;
    proto_header = (FCFSAuthProtoHeader *)task->data;
    body_header = (FCFSAuthProtoSessionPushRespBodyHeader *)
        (proto_header + 1);
    p = (char *)(body_header + 1);
    end = task->data + task->size;
    count = 0;
    while (entry != NULL) {
        body_part = (FCFSAuthProtoSessionPushRespBodyPart *)p;
        if (p + sizeof(FCFSAuthProtoSessionPushRespBodyPart) +
            sizeof(FCFSAuthProtoSessionPushEntry) > end)
        {
            break;
        }

        long2buff(entry->session_id, body_part->session_id);
        body_part->operation = entry->operation;
        if (entry->operation == FCFS_AUTH_SESSION_OP_TYPE_CREATE) {
            long2buff(entry->fields.user.id, body_part->entry->user.id);
            long2buff(entry->fields.user.priv, body_part->entry->user.priv);
            long2buff(entry->fields.pool.id, body_part->entry->pool.id);
            body_part->entry->pool.available = entry->fields.pool.available;
            int2buff(entry->fields.pool.privs.fdir,
                    body_part->entry->pool.privs.fdir);
            int2buff(entry->fields.pool.privs.fstore,
                    body_part->entry->pool.privs.fstore);

            p += sizeof(FCFSAuthProtoSessionPushRespBodyPart) +
                sizeof(FCFSAuthProtoSessionPushEntry);
        } else {
            p += sizeof(FCFSAuthProtoSessionPushRespBodyPart);
        }

        ++count;
        previous = entry;
        entry = entry->next;
    }

    if (entry == NULL) {
        result = 0;
    } else {
        struct fc_queue_info remain_qinfo;

        previous->next = NULL;
        remain_qinfo.head = entry;
        remain_qinfo.tail = qinfo.tail;
        fc_queue_push_queue_to_head_silence(
                &subscriber->queue, &remain_qinfo);
        result = EAGAIN;
    }
    session_subscribe_free_entries(qinfo.head);

    int2buff(count, body_header->count);
    task->length = p - task->data;
    SF_PROTO_SET_HEADER(proto_header, FCFS_AUTH_SERVICE_PROTO_SESSION_PUSH_REQ,
            task->length - sizeof(FCFSAuthProtoHeader));
    sf_send_add_event(task);
    return result;
}

int cluster_thread_loop_callback(struct nio_thread_data *thread_data)
{
    AuthServerContext *server_context;
    ServerSessionSubscriber *subscriber;
    int count;
    int i;

    server_context = (AuthServerContext *)thread_data->arg;
    count = locked_list_count(&server_context->cluster.subscribers);
    for (i=0; i<count; i++) {
        if ((subscriber=cluster_subscriber_queue_pop(
                        server_context)) == NULL)
        {
            break;
        }

        if (cluster_deal_queue(server_context, subscriber) != 0) {
            cluster_subscriber_queue_push_ex(subscriber, false);
        }
    }

    return 0;
}

void *cluster_alloc_thread_extra_data(const int thread_index)
{
    int alloc_size;
    AuthServerContext *server_context;

    alloc_size = sizeof(AuthServerContext);
    server_context = (AuthServerContext *)fc_malloc(alloc_size);
    if (server_context == NULL) {
        return NULL;
    }

    memset(server_context, 0, alloc_size);
    if (locked_list_init(&server_context->cluster.subscribers) != 0) {
        return NULL;
    }

    return server_context;
}
