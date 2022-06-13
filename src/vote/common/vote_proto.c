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

#include "vote_proto.h"

void fcfs_vote_proto_init()
{
}

const char *fcfs_vote_get_cmd_caption(const int cmd)
{
    switch (cmd) {
        case FCFS_VOTE_SERVICE_PROTO_GET_MASTER_REQ:
            return "GET_MASTER_REQ";
        case FCFS_VOTE_SERVICE_PROTO_GET_MASTER_RESP:
            return "GET_MASTER_RESP";
        case FCFS_VOTE_SERVICE_PROTO_CLIENT_JOIN_REQ:
            return "CLIENT_JOIN_REQ";
        case FCFS_VOTE_SERVICE_PROTO_CLIENT_JOIN_RESP:
            return "CLIENT_JOIN_RESP";
        case FCFS_VOTE_SERVICE_PROTO_GET_VOTE_REQ:
            return "GET_VOTE_REQ";
        case FCFS_VOTE_SERVICE_PROTO_GET_VOTE_RESP:
            return "GET_VOTE_RESP";

        case FCFS_VOTE_SERVICE_PROTO_PRE_SET_NEXT_LEADER:
            return "VOTE_PRE_SET_NEXT_LEADER";
        case FCFS_VOTE_SERVICE_PROTO_COMMIT_NEXT_LEADER:
            return "VOTE_COMMIT_NEXT_LEADER";
        case FCFS_VOTE_SERVICE_PROTO_ACTIVE_CHECK_REQ:
            return "VOTE_ACTIVE_CHECK_REQ";
        case FCFS_VOTE_SERVICE_PROTO_ACTIVE_CHECK_RESP:
            return "VOTE_ACTIVE_CHECK_RESP";

        case FCFS_VOTE_SERVICE_PROTO_CLUSTER_STAT_REQ:
            return "CLUSTER_STAT_REQ";
        case FCFS_VOTE_SERVICE_PROTO_CLUSTER_STAT_RESP:
            return "CLUSTER_STAT_RESP";
        case FCFS_VOTE_CLUSTER_PROTO_GET_SERVER_STATUS_REQ:
            return "GET_SERVER_STATUS_REQ";
        case FCFS_VOTE_CLUSTER_PROTO_GET_SERVER_STATUS_RESP:
            return "GET_SERVER_STATUS_RESP";
        case FCFS_VOTE_CLUSTER_PROTO_JOIN_MASTER:
            return "JOIN_MASTER";
        case FCFS_VOTE_CLUSTER_PROTO_PING_MASTER_REQ:
            return "PING_MASTER_REQ";
        case FCFS_VOTE_CLUSTER_PROTO_PING_MASTER_RESP:
            return "PING_MASTER_RESP";
        case FCFS_VOTE_CLUSTER_PROTO_PRE_SET_NEXT_MASTER:
            return "PRE_SET_NEXT_MASTER";
        case FCFS_VOTE_CLUSTER_PROTO_COMMIT_NEXT_MASTER:
            return "COMMIT_NEXT_MASTER";
        default:
            return sf_get_cmd_caption(cmd);
    }
}
