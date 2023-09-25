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
#include "common/vote_proto.h"
#include "server_global.h"
#include "server_func.h"
#include "cluster_info.h"
#include "cluster_relationship.h"
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

int cluster_recv_timeout_callback(struct fast_task_info *task)
{
    if (SERVER_TASK_TYPE == VOTE_SERVER_TASK_TYPE_RELATIONSHIP &&
            CLUSTER_PEER != NULL)
    {
        logWarning("file: "__FILE__", line: %d, "
                "cluster client ip: %s, server id: %d, recv timeout",
                __LINE__, task->client_ip, CLUSTER_PEER->server->id);
        return ETIMEDOUT;
    }

    return 0;
}

void cluster_task_finish_cleanup(struct fast_task_info *task)
{
    switch (SERVER_TASK_TYPE) {
        case VOTE_SERVER_TASK_TYPE_RELATIONSHIP:
            if (CLUSTER_PEER != NULL) {
                CLUSTER_PEER->is_online = false;
                cluster_relationship_add_to_inactive_sarray(CLUSTER_PEER);
                CLUSTER_PEER = NULL;
            } else {
                logError("file: "__FILE__", line: %d, "
                        "mistake happen! task: %p, SERVER_TASK_TYPE: %d, "
                        "CLUSTER_PEER is NULL", __LINE__, task,
                        SERVER_TASK_TYPE);
            }
            SERVER_TASK_TYPE = SF_SERVER_TASK_TYPE_NONE;
            break;
        default:
            break;
    }

    sf_task_finish_clean_up(task);
}

static int cluster_check_config_sign(struct fast_task_info *task,
        const int server_id, const char *config_sign)
{
    if (memcmp(config_sign, CLUSTER_CONFIG_SIGN_BUF,
                SF_CLUSTER_CONFIG_SIGN_LEN) != 0)
    {
        char peer_hex[2 * SF_CLUSTER_CONFIG_SIGN_LEN + 1];
        char my_hex[2 * SF_CLUSTER_CONFIG_SIGN_LEN + 1];

        bin2hex(config_sign, SF_CLUSTER_CONFIG_SIGN_LEN, peer_hex);
        bin2hex((const char *)CLUSTER_CONFIG_SIGN_BUF,
                SF_CLUSTER_CONFIG_SIGN_LEN, my_hex);

        RESPONSE.error.length = sprintf(RESPONSE.error.message,
                "server #%d 's cluster config md5: %s != mine: %s",
                server_id, peer_hex, my_hex);
        return EFAULT;
    }

    return 0;
}

static int cluster_deal_get_server_status(struct fast_task_info *task)
{
    int result;
    int server_id;
    FCFSVoteProtoGetServerStatusReq *req;
    FCFSVoteProtoGetServerStatusResp *resp;

    if ((result=server_expect_body_length(sizeof(
                        FCFSVoteProtoGetServerStatusReq))) != 0)
    {
        return result;
    }

    req = (FCFSVoteProtoGetServerStatusReq *)REQUEST.body;
    server_id = buff2int(req->server_id);
    if ((result=cluster_check_config_sign(task, server_id,
                    req->config_sign)) != 0)
    {
        return result;
    }

    resp = (FCFSVoteProtoGetServerStatusResp *)SF_PROTO_SEND_BODY(task);
    resp->is_master = MYSELF_IS_MASTER;
    int2buff(CLUSTER_MY_SERVER_ID, resp->server_id);

    RESPONSE.header.body_len = sizeof(FCFSVoteProtoGetServerStatusResp);
    RESPONSE.header.cmd = FCFS_VOTE_CLUSTER_PROTO_GET_SERVER_STATUS_RESP;
    TASK_CTX.common.response_done = true;
    return 0;
}

static int cluster_deal_join_master(struct fast_task_info *task)
{
    int result;
    int server_id;
    FCFSVoteProtoJoinMasterReq *req;
    FCFSVoteClusterServerInfo *peer;

    if ((result=server_expect_body_length(sizeof(
                        FCFSVoteProtoJoinMasterReq))) != 0)
    {
        return result;
    }

    req = (FCFSVoteProtoJoinMasterReq *)REQUEST.body;
    server_id = buff2int(req->server_id);
    peer = fcfs_vote_get_server_by_id(server_id);
    if (peer == NULL) {
        RESPONSE.error.length = sprintf(
                RESPONSE.error.message,
                "peer server id: %d not exist", server_id);
        return ENOENT;
    }

    if ((result=cluster_check_config_sign(task, server_id,
                    req->config_sign)) != 0)
    {
        return result;
    }

    if (CLUSTER_MYSELF_PTR != CLUSTER_MASTER_ATOM_PTR &&
            CLUSTER_MYSELF_PTR != CLUSTER_NEXT_MASTER)
    {
        RESPONSE.error.length = sprintf(
                RESPONSE.error.message,
                "i am not master");
        return SF_RETRIABLE_ERROR_NOT_MASTER;
    }

    if (peer == CLUSTER_MYSELF_PTR) {
        RESPONSE.error.length = sprintf(
                RESPONSE.error.message,
                "can't join self");
        return EINVAL;
    }

    if (CLUSTER_PEER != NULL) {
        RESPONSE.error.length = sprintf(
                RESPONSE.error.message,
                "peer server id: %d already joined", server_id);
        return EEXIST;
    }

    SERVER_TASK_TYPE = VOTE_SERVER_TASK_TYPE_RELATIONSHIP;
    CLUSTER_PEER = peer;
    CLUSTER_PEER->is_online = true;
    cluster_relationship_remove_from_inactive_sarray(peer);
    return 0;
}

static int cluster_deal_ping_master(struct fast_task_info *task)
{
    int result;

    if ((result=server_expect_body_length(0)) != 0) {
        return result;
    }

    if (CLUSTER_PEER == NULL) {
        RESPONSE.error.length = sprintf(
                RESPONSE.error.message,
                "please join first");
        return EINVAL;
    }

    if (CLUSTER_MYSELF_PTR != CLUSTER_MASTER_ATOM_PTR &&
            CLUSTER_MYSELF_PTR != CLUSTER_NEXT_MASTER)
    {
        RESPONSE.error.length = sprintf(
                RESPONSE.error.message,
                "i am not master");
        return SF_RETRIABLE_ERROR_NOT_MASTER;
    }

    RESPONSE.header.cmd = FCFS_VOTE_CLUSTER_PROTO_PING_MASTER_RESP;
    return 0;
}

static int cluster_deal_next_master(struct fast_task_info *task)
{
    int result;
    int master_id;
    FCFSVoteClusterServerInfo *master;

    if ((result=server_expect_body_length(4)) != 0) {
        return result;
    }

    if (CLUSTER_MYSELF_PTR == CLUSTER_MASTER_ATOM_PTR) {
        RESPONSE.error.length = sprintf(
                RESPONSE.error.message,
                "i am already master");
        cluster_relationship_trigger_reselect_master();
        return EEXIST;
    }

    master_id = buff2int(REQUEST.body);
    master = fcfs_vote_get_server_by_id(master_id);
    if (master == NULL) {
        RESPONSE.error.length = sprintf(
                RESPONSE.error.message,
                "master server id: %d not exist", master_id);
        return ENOENT;
    }

    if (REQUEST.header.cmd == FCFS_VOTE_CLUSTER_PROTO_PRE_SET_NEXT_MASTER) {
        return cluster_relationship_pre_set_master(master);
    } else {
        return cluster_relationship_commit_master(master);
    }
}

static int cluster_process(struct fast_task_info *task)
{
    int result;
    int cmd;

    cmd = REQUEST.header.cmd;
    if (!MYSELF_IS_MASTER) {
    }

    switch (cmd) {
        case SF_PROTO_ACTIVE_TEST_REQ:
            /*
            if (SERVER_TASK_TYPE == VOTE_SERVER_TASK_TYPE_SUBSCRIBE &&
                    !MYSELF_IS_MASTER)
            {
                RESPONSE.error.length = sprintf(
                        RESPONSE.error.message,
                        "i am not master");
                return SF_RETRIABLE_ERROR_NOT_MASTER;
            }
            */
            RESPONSE.header.cmd = SF_PROTO_ACTIVE_TEST_RESP;
            return sf_proto_deal_active_test(task, &REQUEST, &RESPONSE);
        case FCFS_VOTE_CLUSTER_PROTO_GET_SERVER_STATUS_REQ:
            return cluster_deal_get_server_status(task);
        case FCFS_VOTE_CLUSTER_PROTO_PRE_SET_NEXT_MASTER:
        case FCFS_VOTE_CLUSTER_PROTO_COMMIT_NEXT_MASTER:
            return cluster_deal_next_master(task);
        case FCFS_VOTE_CLUSTER_PROTO_JOIN_MASTER:
            return cluster_deal_join_master(task);
        case FCFS_VOTE_CLUSTER_PROTO_PING_MASTER_REQ:
            return cluster_deal_ping_master(task);
        case FCFS_VOTE_SERVICE_PROTO_GET_MASTER_REQ:
            if ((result=fcfs_vote_deal_get_master(task,
                            CLUSTER_GROUP_INDEX)) == 0)
            {
                RESPONSE.header.cmd = FCFS_VOTE_SERVICE_PROTO_GET_MASTER_RESP;
            }
            return result;
        case SF_SERVICE_PROTO_GET_LEADER_REQ:
            if ((result=fcfs_vote_deal_get_master(task,
                            CLUSTER_GROUP_INDEX)) == 0)
            {
                RESPONSE.header.cmd = SF_SERVICE_PROTO_GET_LEADER_RESP;
            }
            return result;
        default:
            RESPONSE.error.length = sprintf(RESPONSE.error.message,
                    "unkown cluster cmd: %d", REQUEST.header.cmd);
            return -EINVAL;
    }
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
        result = cluster_process(task);
    }

    if (result == TASK_STATUS_CONTINUE) {
        return 0;
    } else {
        RESPONSE_STATUS = result;
        return sf_proto_deal_task_done(task, "cluster", &TASK_CTX.common);
    }
}
