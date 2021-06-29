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


#ifndef _SERVER_TYPES_H
#define _SERVER_TYPES_H

#include "fastcommon/uniq_skiplist.h"
#include "fastcommon/fc_queue.h"
#include "fastcommon/fast_mblock.h"
#include "fastcommon/locked_list.h"
#include "sf/sf_types.h"
#include "common/auth_types.h"
#include "common/server_session.h"

#define TASK_STATUS_CONTINUE           12345
#define TASK_ARG           ((AuthServerTaskArg *)task->arg)
#define TASK_CTX           TASK_ARG->context
#define SESSION_ENTRY      TASK_CTX.shared.service.session
#define SESSION_FIELDS     ((ServerSessionFields *)SESSION_ENTRY->fields)
#define SESSION_DBUSER     SESSION_FIELDS->dbuser
#define SESSION_DBPOOL     SESSION_FIELDS->dbpool
#define SESSION_USER       SESSION_FIELDS->dbuser->user
#define SESSION_SUBSCRIBER TASK_CTX.shared.cluster.subscriber
#define REQUEST            TASK_CTX.common.request
#define RESPONSE           TASK_CTX.common.response
#define RESPONSE_STATUS    RESPONSE.header.status
#define REQUEST_STATUS     REQUEST.header.status
#define SERVER_TASK_TYPE   TASK_CTX.task_type
#define CLUSTER_PEER       TASK_CTX.shared.cluster.peer

#define AUTH_SERVER_TASK_TYPE_SESSION      1
#define AUTH_SERVER_TASK_TYPE_SUBSCRIBE    2
#define AUTH_SERVER_TASK_TYPE_RELATIONSHIP 3

#define SERVER_CTX        ((AuthServerContext *)task->thread_data->arg)
#define SESSION_HOLDER    SERVER_CTX->service.session_holder

typedef struct fcfs_auth_cluster_server_info {
    FCServerInfo *server;
    volatile bool is_online;
} FCFSAuthClusterServerInfo;

typedef struct fcfs_auth_cluster_server_array {
    FCFSAuthClusterServerInfo *servers;
    int count;
} FCFSAuthClusterServerArray;

struct db_user_info;
struct db_storage_pool_info;

typedef struct server_session_fields {
    bool publish;
    const struct db_user_info *dbuser;
    const struct db_storage_pool_info *dbpool;
    FCFSAuthSPoolPriviledges pool_privs;

    struct fc_list_head dlink; //for publish list
} ServerSessionFields;

typedef struct server_session_subscriber {
    struct fc_queue queue;     //element: ServerSessionSubscribeEntry
    struct fc_list_head dlink; //for global subscriber's chain
    struct {
        volatile int in_queue;   //if in the subscriber queue of NIO thread
        struct fast_task_info *task;
        struct fc_list_head dlink; //for nio thread subscriber's chain
    } nio;
} ServerSessionSubscriber;

typedef struct server_task_arg {
    struct {
        SFCommonTaskContext common;
        int task_type;
        union {
            struct {
                ServerSessionEntry *session;
            } service;

            union {
                ServerSessionSubscriber *subscriber;
                FCFSAuthClusterServerInfo *peer;   //the peer server in the cluster
            } cluster;
        } shared;

    } context;
} AuthServerTaskArg;

typedef struct auth_server_context {
    void *dao_ctx;
    union {
        struct {
            FCLockedList subscribers;   //element: ServerSessionSubscriber
        } cluster;
        struct {
            ServerSessionEntry *session_holder;
        } service;
    };
} AuthServerContext;

#endif
