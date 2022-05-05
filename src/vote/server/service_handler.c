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

//service_handler.c

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
#include "sf/sf_service.h"
#include "sf/sf_global.h"
#include "common/vote_proto.h"
#include "server_global.h"
#include "server_func.h"
#include "service_group_htable.h"
#include "common_handler.h"
#include "service_handler.h"

int service_handler_init()
{
    return 0;
}

int service_handler_destroy()
{
    return 0;
}

void service_task_finish_cleanup(struct fast_task_info *task)
{
    if (SERVER_TASK_TYPE == VOTE_SERVER_TASK_TYPE_VOTE_NODE) {
        __sync_bool_compare_and_swap(&SERVICE_PEER.group->leader_id,
                SERVICE_PEER.server_id, 0);
        SERVICE_PEER.server_id = 0;
        SERVICE_PEER.group = NULL;
        SERVER_TASK_TYPE = VOTE_SERVER_TASK_TYPE_NONE;
    }

    sf_task_finish_clean_up(task);
}

static int service_deal_cluster_stat(struct fast_task_info *task)
{
    int result;
    FCFSVoteProtoClusterStatRespBodyHeader *body_header;
    FCFSVoteProtoClusterStatRespBodyPart *body_part;
    FCFSVoteClusterServerInfo *cs;
    FCFSVoteClusterServerInfo *send;

    if ((result=server_expect_body_length(0)) != 0) {
        return result;
    }

    body_header = (FCFSVoteProtoClusterStatRespBodyHeader *)
        SF_PROTO_RESP_BODY(task);
    body_part = (FCFSVoteProtoClusterStatRespBodyPart *)(body_header + 1);
    int2buff(CLUSTER_SERVER_ARRAY.count, body_header->count);

    send = CLUSTER_SERVER_ARRAY.servers + CLUSTER_SERVER_ARRAY.count;
    for (cs=CLUSTER_SERVER_ARRAY.servers; cs<send; cs++, body_part++) {
        int2buff(cs->server->id, body_part->server_id);
        body_part->is_online = ((cs == CLUSTER_MASTER_ATOM_PTR ||
                    cs->is_online) ? 1 : 0);
        body_part->is_master = (cs == CLUSTER_MASTER_ATOM_PTR ? 1 : 0);
        snprintf(body_part->ip_addr, sizeof(body_part->ip_addr), "%s",
                SERVICE_GROUP_ADDRESS_FIRST_IP(cs->server));
        short2buff(SERVICE_GROUP_ADDRESS_FIRST_PORT(cs->server),
                body_part->port);
    }

    RESPONSE.header.body_len = (char *)body_part - SF_PROTO_RESP_BODY(task);
    RESPONSE.header.cmd = FCFS_VOTE_SERVICE_PROTO_CLUSTER_STAT_RESP;
    TASK_CTX.common.response_done = true;
    return 0;
}

static int service_deal_client_join(struct fast_task_info *task)
{
    int result;
    int server_id;
    int service_id;
    int group_id;
    int response_size;
    FCFSVoteProtoClientJoinReq *req;
    FCFSVoteServiceGroupInfo *group;

    if ((result=server_expect_body_length(sizeof(
                        FCFSVoteProtoClientJoinReq))) != 0)
    {
        return result;
    }

    req = (FCFSVoteProtoClientJoinReq *)REQUEST.body;
    server_id = buff2int(req->server_id);
    group_id = buff2short(req->group_id);
    response_size = buff2short(req->response_size);
    service_id = req->service_id;
    switch (service_id) {
        case FCFS_VOTE_SERVICE_ID_FAUTH:
        case FCFS_VOTE_SERVICE_ID_FDIR:
        case FCFS_VOTE_SERVICE_ID_FSTORE:
            break;
        default:
            RESPONSE.error.length = sprintf(RESPONSE.error.message,
                    "server_id: %d, unkown service id: %d",
                    server_id, service_id);
            return -EINVAL;
    }

    if (server_id <= 0) {
        RESPONSE.error.length = sprintf(RESPONSE.error.message,
                "invalid server_id: %d", server_id);
        return -EINVAL;
    }
    if (response_size <= 0 || response_size > (task->size -
                sizeof(FCFSVoteProtoHeader)))
    {
        RESPONSE.error.length = sprintf(RESPONSE.error.message,
                "server_id: %d, invalid response_size: %d",
                server_id, response_size);
        return -EINVAL;
    }

    result = service_group_htable_get(service_id, group_id,
            (req->is_leader ? server_id : 0), response_size, &group);
    if (result != 0) {
        if (result == SF_CLUSTER_ERROR_LEADER_INCONSISTENT) {
            RESPONSE.error.length = sprintf(RESPONSE.error.message,
                    "service_id: %d, group_id: %d, target server_id: %d, "
                    "current leader id: %d, leader inconsistent",
                    service_id, group_id, server_id, group->leader_id);
        }
        return result;
    }

    if (SERVICE_PEER.group != NULL) {
        RESPONSE.error.length = sprintf(RESPONSE.error.message,
                "service_id: %d, server_id: %d already joined",
                service_id, server_id);
        return EEXIST;
    }

    SERVICE_PEER.server_id = server_id;
    SERVICE_PEER.group = group;
    SERVER_TASK_TYPE = VOTE_SERVER_TASK_TYPE_VOTE_NODE;
    RESPONSE.header.cmd = FCFS_VOTE_SERVICE_PROTO_CLIENT_JOIN_RESP;
    return 0;
}

static inline int service_check_login(struct fast_task_info *task)
{
    if (!(SERVER_TASK_TYPE == VOTE_SERVER_TASK_TYPE_VOTE_NODE &&
                SERVICE_PEER.group != NULL))
    {
        RESPONSE.error.length = sprintf(RESPONSE.error.message,
                "please login first");
        return EPERM;
    }

    return 0;
}

static int service_deal_get_vote(struct fast_task_info *task)
{
    int result;
    int server_id;
    SFProtoGetServerStatusReq *req;

    if ((result=server_expect_body_length(sizeof(
                        SFProtoGetServerStatusReq))) != 0)
    {
        return result;
    }

    if ((result=service_check_login(task)) != 0) {
        return result;
    }

    req = (SFProtoGetServerStatusReq *)REQUEST.body;
    server_id = buff2int(req->server_id);
    memset(REQUEST.body, 0, SERVICE_PEER.group->response_size);
    RESPONSE.header.body_len = SERVICE_PEER.group->response_size;
    RESPONSE.header.cmd = FCFS_VOTE_SERVICE_PROTO_GET_VOTE_RESP;
    TASK_CTX.common.response_done = true;
    return 0;
}

static int service_deal_active_check(struct fast_task_info *task)
{
    int result;

    if ((result=server_expect_body_length(0)) != 0) {
        return result;
    }

    if ((result=service_check_login(task)) != 0) {
        return result;
    }

    if (FC_ATOMIC_GET(SERVICE_PEER.group->leader_id) !=
            SERVICE_PEER.server_id)
    {
        RESPONSE.error.length = sprintf(RESPONSE.error.message,
                "service: %s, leader id: %d != peer server id: %d",
                fcfs_vote_get_service_name(SERVICE_PEER.group->service_id),
                FC_ATOMIC_GET(SERVICE_PEER.group->leader_id),
                SERVICE_PEER.server_id);
        return SF_CLUSTER_ERROR_LEADER_INCONSISTENT;
    }

    RESPONSE.header.cmd = FCFS_VOTE_SERVICE_PROTO_ACTIVE_CHECK_RESP;
    return 0;
}

static int service_deal_next_leader(struct fast_task_info *task)
{
    int result;
    int next_leader;
    int old_leader_id;
    int leader_id;

    if ((result=server_expect_body_length(0)) != 0) {
        return result;
    }

    if ((result=service_check_login(task)) != 0) {
        return result;
    }

    leader_id = SERVICE_PEER.server_id;
    old_leader_id = FC_ATOMIC_GET(SERVICE_PEER.group->leader_id);
    if (old_leader_id != 0) {
        if (old_leader_id == leader_id) {
            return 0;
        } else {
            RESPONSE.error.length = sprintf(RESPONSE.error.message,
                    "service: %s, leader id: %d != peer server id: %d",
                    fcfs_vote_get_service_name(SERVICE_PEER.group->
                        service_id), old_leader_id, leader_id);
            return SF_CLUSTER_ERROR_LEADER_INCONSISTENT;
        }
    }

    next_leader = FC_ATOMIC_GET(SERVICE_PEER.group->next_leader);
    if (REQUEST.header.cmd == FCFS_VOTE_SERVICE_PROTO_PRE_SET_NEXT_LEADER) {
        if (next_leader == 0) {
            if (__sync_bool_compare_and_swap(&SERVICE_PEER.group->
                        next_leader, 0, leader_id))
            {
                return 0;
            }
        } else if (next_leader == leader_id) {
            return 0;
        } else {
            __sync_bool_compare_and_swap(&SERVICE_PEER.group->
                    next_leader, next_leader, 0);
        }
    } else {  //commit leader
        if (next_leader != leader_id) {
            __sync_bool_compare_and_swap(&SERVICE_PEER.group->
                    next_leader, next_leader, 0);
        } else if (__sync_bool_compare_and_swap(&SERVICE_PEER.
                    group->leader_id, 0, leader_id))
        {
            return 0;
        } else if (FC_ATOMIC_GET(SERVICE_PEER.group->
                    leader_id) == leader_id)
        {
            return 0;
        }
    }

    RESPONSE.error.length = sprintf(RESPONSE.error.message,
            "service: %s, next leader id: %d != peer server id: %d",
            fcfs_vote_get_service_name(SERVICE_PEER.group->
                service_id), next_leader, leader_id);
    return SF_CLUSTER_ERROR_LEADER_INCONSISTENT;
}

static int service_process(struct fast_task_info *task)
{
    int result;

    if (!MYSELF_IS_MASTER) {
        if (!(REQUEST.header.cmd == FCFS_VOTE_SERVICE_PROTO_GET_MASTER_REQ ||
                    REQUEST.header.cmd == SF_SERVICE_PROTO_GET_LEADER_REQ))
        {
            RESPONSE.error.length = sprintf(
                    RESPONSE.error.message,
                    "i am not master");
            return SF_RETRIABLE_ERROR_NOT_MASTER;
        }
    }

    switch (REQUEST.header.cmd) {
        case SF_PROTO_ACTIVE_TEST_REQ:
            RESPONSE.header.cmd = SF_PROTO_ACTIVE_TEST_RESP;
            return sf_proto_deal_active_test(task, &REQUEST, &RESPONSE);
        case FCFS_VOTE_SERVICE_PROTO_CLUSTER_STAT_REQ:
            return service_deal_cluster_stat(task);
        case FCFS_VOTE_SERVICE_PROTO_GET_MASTER_REQ:
            if ((result=fcfs_vote_deal_get_master(task,
                            SERVICE_GROUP_INDEX)) == 0)
            {
                RESPONSE.header.cmd = FCFS_VOTE_SERVICE_PROTO_GET_MASTER_RESP;
            }
            return result;
        case SF_SERVICE_PROTO_GET_LEADER_REQ:
            if ((result=fcfs_vote_deal_get_master(task,
                            SERVICE_GROUP_INDEX)) == 0)
            {
                RESPONSE.header.cmd = SF_SERVICE_PROTO_GET_LEADER_RESP;
            }
            return result;
        case FCFS_VOTE_SERVICE_PROTO_CLIENT_JOIN_REQ:
            return service_deal_client_join(task);
        case FCFS_VOTE_SERVICE_PROTO_GET_VOTE_REQ:
            return service_deal_get_vote(task);
        case FCFS_VOTE_SERVICE_PROTO_PRE_SET_NEXT_LEADER:
            return service_deal_next_leader(task);
        case FCFS_VOTE_SERVICE_PROTO_COMMIT_NEXT_LEADER:
            return service_deal_next_leader(task);
        case FCFS_VOTE_SERVICE_PROTO_ACTIVE_CHECK_REQ:
            return service_deal_active_check(task);
        default:
            RESPONSE.error.length = sprintf(
                    RESPONSE.error.message,
                    "unkown cmd: %d", REQUEST.header.cmd);
            return -EINVAL;
    }
}

int service_deal_task(struct fast_task_info *task, const int stage)
{
    int result;

    /*
    logInfo("file: "__FILE__", line: %d, "
            "task: %p, sock: %d, nio stage: %d, continue: %d, "
            "cmd: %d (%s)", __LINE__, task, task->event.fd, stage,
            stage == SF_NIO_STAGE_CONTINUE,
            ((FCFSVoteProtoHeader *)task->data)->cmd,
            fdir_get_cmd_caption(((FCFSVoteProtoHeader *)task->data)->cmd));
            */

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
        result = service_process(task);
    }

    if (result == TASK_STATUS_CONTINUE) {
        return 0;
    } else {
        RESPONSE_STATUS = result;
        return sf_proto_deal_task_done(task, &TASK_CTX.common);
    }
}
