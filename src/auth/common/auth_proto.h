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

#ifndef _FCFS_AUTH_PROTO_H
#define _FCFS_AUTH_PROTO_H

#include "sf/sf_proto.h"
#include "auth_types.h"

#define FCFS_AUTH_SERVICE_PROTO_USER_LOGIN_REQ             9
#define FCFS_AUTH_SERVICE_PROTO_USER_LOGIN_RESP           10

#define FCFS_AUTH_SERVICE_PROTO_SESSION_SUBSCRIBE_REQ     11
#define FCFS_AUTH_SERVICE_PROTO_SESSION_SUBSCRIBE_RESP    12
#define FCFS_AUTH_SERVICE_PROTO_SESSION_PUSH_REQ          13
#define FCFS_AUTH_SERVICE_PROTO_SESSION_PUSH_RESP         14
#define FCFS_AUTH_SERVICE_PROTO_SESSION_VALIDATE_REQ      15
#define FCFS_AUTH_SERVICE_PROTO_SESSION_VALIDATE_RESP     16

#define FCFS_AUTH_SERVICE_PROTO_USER_CREATE_REQ           21
#define FCFS_AUTH_SERVICE_PROTO_USER_CREATE_RESP          22
#define FCFS_AUTH_SERVICE_PROTO_USER_LIST_REQ             23
#define FCFS_AUTH_SERVICE_PROTO_USER_LIST_RESP            24
#define FCFS_AUTH_SERVICE_PROTO_USER_GRANT_REQ            25
#define FCFS_AUTH_SERVICE_PROTO_USER_GRANT_RESP           26
#define FCFS_AUTH_SERVICE_PROTO_USER_REMOVE_REQ           27
#define FCFS_AUTH_SERVICE_PROTO_USER_REMOVE_RESP          28
#define FCFS_AUTH_SERVICE_PROTO_USER_PASSWD_REQ           29
#define FCFS_AUTH_SERVICE_PROTO_USER_PASSWD_RESP          30

#define FCFS_AUTH_SERVICE_PROTO_GET_MASTER_REQ            61
#define FCFS_AUTH_SERVICE_PROTO_GET_MASTER_RESP           62
#define FCFS_AUTH_SERVICE_PROTO_CLUSTER_STAT_REQ          63
#define FCFS_AUTH_SERVICE_PROTO_CLUSTER_STAT_RESP         64

#define FCFS_AUTH_SERVICE_PROTO_SPOOL_CREATE_REQ          71
#define FCFS_AUTH_SERVICE_PROTO_SPOOL_CREATE_RESP         72
#define FCFS_AUTH_SERVICE_PROTO_SPOOL_LIST_REQ            73
#define FCFS_AUTH_SERVICE_PROTO_SPOOL_LIST_RESP           74
#define FCFS_AUTH_SERVICE_PROTO_SPOOL_REMOVE_REQ          75
#define FCFS_AUTH_SERVICE_PROTO_SPOOL_REMOVE_RESP         76
#define FCFS_AUTH_SERVICE_PROTO_SPOOL_SET_QUOTA_REQ       77
#define FCFS_AUTH_SERVICE_PROTO_SPOOL_SET_QUOTA_RESP      78
#define FCFS_AUTH_SERVICE_PROTO_SPOOL_GET_QUOTA_REQ       79
#define FCFS_AUTH_SERVICE_PROTO_SPOOL_GET_QUOTA_RESP      80

#define FCFS_AUTH_SERVICE_PROTO_GPOOL_GRANT_REQ           91
#define FCFS_AUTH_SERVICE_PROTO_GPOOL_GRANT_RESP          92
#define FCFS_AUTH_SERVICE_PROTO_GPOOL_WITHDRAW_REQ        93
#define FCFS_AUTH_SERVICE_PROTO_GPOOL_WITHDRAW_RESP       94
#define FCFS_AUTH_SERVICE_PROTO_GPOOL_LIST_REQ            95
#define FCFS_AUTH_SERVICE_PROTO_GPOOL_LIST_RESP           96

//cluster commands
#define FCFS_AUTH_CLUSTER_PROTO_GET_SERVER_STATUS_REQ    201
#define FCFS_AUTH_CLUSTER_PROTO_GET_SERVER_STATUS_RESP   202
#define FCFS_AUTH_CLUSTER_PROTO_JOIN_MASTER              203  //slave  -> master
#define FCFS_AUTH_CLUSTER_PROTO_PING_MASTER_REQ          205
#define FCFS_AUTH_CLUSTER_PROTO_PING_MASTER_RESP         206
#define FCFS_AUTH_CLUSTER_PROTO_PRE_SET_NEXT_MASTER      207  //notify next leader to other servers
#define FCFS_AUTH_CLUSTER_PROTO_COMMIT_NEXT_MASTER       208  //commit next leader to other servers

typedef SFCommonProtoHeader  FCFSAuthProtoHeader;

typedef struct fcfs_auth_proto_name_info {
    unsigned char len;
    char str[0];
} FCFSAuthProtoNameInfo;

typedef struct fcfs_auth_proto_user_passwd_pair {
    char passwd[FCFS_AUTH_PASSWD_LEN];
    FCFSAuthProtoNameInfo username;
} FCFSAuthProtoUserPasswdPair;

typedef struct fcfs_auth_proto_user_pool_pair {
    FCFSAuthProtoNameInfo username;
    FCFSAuthProtoNameInfo poolname;
} FCFSAuthProtoUserPoolPair;

typedef struct fcfs_auth_proto_pool_priviledges {
    char fdir[4];
    char fstore[4];
} FCFSAuthProtoPoolPriviledges;

typedef struct fcfs_auth_proto_user_login_req {
    unsigned char flags;
    FCFSAuthProtoUserPasswdPair up_pair;
    FCFSAuthProtoNameInfo poolname;
} FCFSAuthProtoUserLoginReq;

typedef struct fcfs_auth_proto_user_login_resp {
    char session_id[FCFS_AUTH_SESSION_ID_LEN];
} FCFSAuthProtoUserLoginResp;

typedef struct fcfs_auth_proto_session_subscribe_req {
    FCFSAuthProtoUserPasswdPair up_pair;
} FCFSAuthProtoSessionSubscribeReq;

typedef struct fcfs_auth_proto_session_validate_req {
    char session_id[FCFS_AUTH_SESSION_ID_LEN];
    char validate_key[FCFS_AUTH_PASSWD_LEN];
    char priv_type;
    char padding[7];
    char pool_id[8];
    char priv_required[8];
} FCFSAuthProtoSessionValidateReq;

typedef struct fcfs_auth_proto_session_validate_resp {
    char result[4];
} FCFSAuthProtoSessionValidateResp;

typedef struct fcfs_auth_proto_session_push_resp_body_header {
    char count[4];
} FCFSAuthProtoSessionPushRespBodyHeader;

typedef struct fcfs_auth_proto_session_push_entry {
    struct {
        char available;
        char id[8];
        FCFSAuthProtoPoolPriviledges privs;
    } pool;

    struct {
        char id[8];
        char priv[8];
    } user;
} FCFSAuthProtoSessionPushEntry;

typedef struct fcfs_auth_proto_session_push_resp_body_part {
    char session_id[FCFS_AUTH_SESSION_ID_LEN];
    char operation;
    FCFSAuthProtoSessionPushEntry entry[0];
} FCFSAuthProtoSessionPushRespBodyPart;

typedef struct fcfs_auth_proto_user_create_req {
    char priv[8];
    FCFSAuthProtoUserPasswdPair up_pair;
} FCFSAuthProtoUserCreateReq;

typedef struct fcfs_auth_proto_user_passwd_req {
    FCFSAuthProtoUserPasswdPair up_pair;
} FCFSAuthProtoUserPasswdReq;

typedef struct fcfs_auth_proto_user_list_req {
    SFProtoLimitInfo limit;
    FCFSAuthProtoNameInfo username;
} FCFSAuthProtoUserListReq;

typedef struct fcfs_auth_proto_list_resp_header {
    char count[4];
    char is_last;
    char padding[3];
} FCFSAuthProtoListRespHeader;

typedef struct fcfs_auth_proto_user_list_resp_body_part {
    char priv[8];
    FCFSAuthProtoNameInfo username;
} FCFSAuthProtoUserListRespBodyPart;

typedef struct fcfs_auth_proto_user_grant_req {
    char priv[8];
    FCFSAuthProtoNameInfo username;
} FCFSAuthProtoUserGrantReq;

typedef struct fcfs_auth_proto_user_remove_req {
    FCFSAuthProtoNameInfo username;
} FCFSAuthProtoUserRemoveReq;


typedef struct fcfs_auth_proto_spool_create_req {
    char quota[8];
    char dryrun;
    FCFSAuthProtoNameInfo poolname;
} FCFSAuthProtoSPoolCreateReq;

typedef struct fcfs_auth_proto_spool_create_resp {
    FCFSAuthProtoNameInfo poolname;
} FCFSAuthProtoSPoolCreateResp;

typedef struct fcfs_auth_proto_spool_list_req {
    SFProtoLimitInfo limit;
    FCFSAuthProtoUserPoolPair up_pair;
} FCFSAuthProtoSPoolListReq;

typedef struct fcfs_auth_proto_spool_list_resp_body_part {
    char quota[8];
    FCFSAuthProtoNameInfo poolname;
} FCFSAuthProtoSPoolListRespBodyPart;

typedef struct fcfs_auth_proto_spool_remove_req {
    FCFSAuthProtoNameInfo poolname;
} FCFSAuthProtoSPoolRemoveReq;

typedef struct fcfs_auth_proto_spool_set_quota_req {
    char quota[8];
    FCFSAuthProtoNameInfo poolname;
} FCFSAuthProtoSPoolSetQuotaReq;

typedef struct fcfs_auth_proto_spool_get_quota_req {
    FCFSAuthProtoNameInfo poolname;
} FCFSAuthProtoSPoolGetQuotaReq;

typedef struct fcfs_auth_proto_spool_get_quota_resp {
    char quota[8];
} FCFSAuthProtoSPoolGetQuotaResp;

typedef struct fcfs_auth_proto_spool_grant_req {
    FCFSAuthProtoPoolPriviledges privs;
    FCFSAuthProtoUserPoolPair up_pair;
} FCFSAuthProtoSPoolGrantReq;

typedef struct fcfs_auth_proto_spool_withdraw_req {
    FCFSAuthProtoUserPoolPair up_pair;
} FCFSAuthProtoSPoolWithdrawReq;

typedef FCFSAuthProtoSPoolListReq FCFSAuthProtoGPoolListReq;

typedef struct fcfs_auth_proto_gpool_list_resp_body_part {
    FCFSAuthProtoPoolPriviledges privs;
    FCFSAuthProtoUserPoolPair up_pair;
} FCFSAuthProtoGPoolListRespBodyPart;

typedef struct fcfs_auth_proto_get_server_resp {
    char server_id[4];
    char port[2];
    char padding[2];
    char ip_addr[IP_ADDRESS_SIZE];
} FCFSAuthProtoGetServerResp;

typedef struct fcfs_auth_proto_get_server_status_req {
    char server_id[4];
    char config_sign[SF_CLUSTER_CONFIG_SIGN_LEN];
} FCFSAuthProtoGetServerStatusReq;

typedef struct fcfs_auth_proto_get_server_status_resp {
    char server_id[4];
    char is_master;
    char padding[3];
} FCFSAuthProtoGetServerStatusResp;

typedef struct fcfs_auth_proto_join_master_req {
    char server_id[4];     //the slave server id
    char padding[4];
    char config_sign[SF_CLUSTER_CONFIG_SIGN_LEN];
} FCFSAuthProtoJoinMasterReq;

typedef struct fcfs_auth_proto_cluster_stat_resp_body_header {
    char count[4];
    char padding[4];
} FCFSAuthProtoClusterStatRespBodyHeader;

typedef struct fcfs_auth_proto_cluster_stat_resp_body_part {
    char server_id[4];
    char is_online;
    char is_master;
    char padding[1];
    char port[2];
    char ip_addr[IP_ADDRESS_SIZE];
} FCFSAuthProtoClusterStatRespBodyPart;

#ifdef __cplusplus
extern "C" {
#endif

void fcfs_auth_proto_init();

const char *fcfs_auth_get_cmd_caption(const int cmd);

static inline void fcfs_auth_parse_user_pool_pair(FCFSAuthProtoUserPoolPair
        *up_pair, string_t *username, string_t *poolname)
{
    FCFSAuthProtoNameInfo *proto_pname;

    FC_SET_STRING_EX(*username, up_pair->username.str,
            up_pair->username.len);
    proto_pname = (FCFSAuthProtoNameInfo *)(up_pair->username.str +
            up_pair->username.len);
    FC_SET_STRING_EX(*poolname, proto_pname->str, proto_pname->len);
}

static inline void fcfs_auth_pack_user_pool_pair(const string_t *username,
        const string_t *poolname, FCFSAuthProtoUserPoolPair *up_pair)
{
    FCFSAuthProtoNameInfo *proto_pname;

    up_pair->username.len = username->len;
    if (username->len > 0) {
        memcpy(up_pair->username.str, username->str, username->len);
    }

    proto_pname = (FCFSAuthProtoNameInfo *)(up_pair->
            username.str + username->len);
    proto_pname->len = poolname->len;
    if (poolname->len > 0) {
        memcpy(proto_pname->str, poolname->str, poolname->len);
    }
}


#ifdef __cplusplus
}
#endif

#endif
