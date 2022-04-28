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
#include "common/vote_types.h"

#define TASK_STATUS_CONTINUE           12345
#define TASK_ARG           ((VoteServerTaskArg *)task->arg)
#define TASK_CTX           TASK_ARG->context
#define REQUEST            TASK_CTX.common.request
#define RESPONSE           TASK_CTX.common.response
#define RESPONSE_STATUS    RESPONSE.header.status
#define REQUEST_STATUS     REQUEST.header.status
#define SERVER_TASK_TYPE   TASK_CTX.task_type
#define CLUSTER_PEER       TASK_CTX.shared.cluster.peer

#define VOTE_SERVER_TASK_TYPE_RELATIONSHIP 1

#define SERVER_CTX        ((VoteServerContext *)task->thread_data->arg)
#define SESSION_HOLDER    SERVER_CTX->service.session_holder

typedef struct fcfs_vote_cluster_server_info {
    FCServerInfo *server;
    volatile bool is_online;
} FCFSVoteClusterServerInfo;

typedef struct fcfs_vote_cluster_server_array {
    FCFSVoteClusterServerInfo *servers;
    int count;
} FCFSVoteClusterServerArray;

typedef struct server_task_arg {
    struct {
        SFCommonTaskContext common;
        int task_type;
        union {
            struct {
            } service;

            union {
                FCFSVoteClusterServerInfo *peer;   //the peer server in the cluster
            } cluster;
        } shared;

    } context;
} VoteServerTaskArg;

#endif
