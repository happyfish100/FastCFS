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

#ifndef _FCFS_VOTE_PROTO_H
#define _FCFS_VOTE_PROTO_H

#include "sf/sf_proto.h"
#include "vote_types.h"

#define FCFS_VOTE_SERVICE_PROTO_GET_MASTER_REQ            61
#define FCFS_VOTE_SERVICE_PROTO_GET_MASTER_RESP           62
#define FCFS_VOTE_SERVICE_PROTO_CLUSTER_STAT_REQ          63
#define FCFS_VOTE_SERVICE_PROTO_CLUSTER_STAT_RESP         64

//cluster commands
#define FCFS_VOTE_CLUSTER_PROTO_GET_SERVER_STATUS_REQ    201
#define FCFS_VOTE_CLUSTER_PROTO_GET_SERVER_STATUS_RESP   202
#define FCFS_VOTE_CLUSTER_PROTO_JOIN_MASTER              203  //slave -> master
#define FCFS_VOTE_CLUSTER_PROTO_PING_MASTER_REQ          205
#define FCFS_VOTE_CLUSTER_PROTO_PING_MASTER_RESP         206
#define FCFS_VOTE_CLUSTER_PROTO_PRE_SET_NEXT_MASTER      207  //notify next master to other servers
#define FCFS_VOTE_CLUSTER_PROTO_COMMIT_NEXT_MASTER       208  //commit next master to other servers

typedef SFCommonProtoHeader  FCFSVoteProtoHeader;

typedef struct fcfs_vote_proto_name_info {
    unsigned char len;
    char str[0];
} FCFSVoteProtoNameInfo;

typedef struct fcfs_vote_proto_get_server_resp {
    char server_id[4];
    char port[2];
    char padding[2];
    char ip_addr[IP_ADDRESS_SIZE];
} FCFSVoteProtoGetServerResp;

typedef struct fcfs_vote_proto_get_server_status_req {
    char server_id[4];
    char config_sign[SF_CLUSTER_CONFIG_SIGN_LEN];
} FCFSVoteProtoGetServerStatusReq;

typedef struct fcfs_vote_proto_get_server_status_resp {
    char server_id[4];
    char is_master;
    char padding[3];
} FCFSVoteProtoGetServerStatusResp;

typedef struct fcfs_vote_proto_join_master_req {
    char server_id[4];     //the slave server id
    char padding[4];
    char config_sign[SF_CLUSTER_CONFIG_SIGN_LEN];
} FCFSVoteProtoJoinMasterReq;

typedef struct fcfs_vote_proto_cluster_stat_resp_body_header {
    char count[4];
    char padding[4];
} FCFSVoteProtoClusterStatRespBodyHeader;

typedef struct fcfs_vote_proto_cluster_stat_resp_body_part {
    char server_id[4];
    char is_online;
    char is_master;
    char padding[1];
    char port[2];
    char ip_addr[IP_ADDRESS_SIZE];
} FCFSVoteProtoClusterStatRespBodyPart;

#ifdef __cplusplus
extern "C" {
#endif

void fcfs_vote_proto_init();

const char *fcfs_vote_get_cmd_caption(const int cmd);

#ifdef __cplusplus
}
#endif

#endif
