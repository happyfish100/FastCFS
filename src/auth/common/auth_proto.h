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

#define FCFS_AUTH_SERVICE_PROTO_USER_CREATE_REQ           21
#define FCFS_AUTH_SERVICE_PROTO_USER_CREATE_RESP          22
#define FCFS_AUTH_SERVICE_PROTO_USER_LIST_REQ             23
#define FCFS_AUTH_SERVICE_PROTO_USER_LIST_RESP            24
#define FCFS_AUTH_SERVICE_PROTO_USER_GRANT_REQ            25
#define FCFS_AUTH_SERVICE_PROTO_USER_GRANT_RESP           26
#define FCFS_AUTH_SERVICE_PROTO_USER_REMOVE_REQ           27
#define FCFS_AUTH_SERVICE_PROTO_USER_REMOVE_RESP          28

#define FCFS_AUTH_SERVICE_PROTO_SPOOL_CREATE_REQ          71
#define FCFS_AUTH_SERVICE_PROTO_SPOOL_CREATE_RESP         72
#define FCFS_AUTH_SERVICE_PROTO_SPOOL_LIST_REQ            73
#define FCFS_AUTH_SERVICE_PROTO_SPOOL_LIST_RESP           74
#define FCFS_AUTH_SERVICE_PROTO_SPOOL_REMOVE_REQ          75
#define FCFS_AUTH_SERVICE_PROTO_SPOOL_REMOVE_RESP         76
#define FCFS_AUTH_SERVICE_PROTO_SPOOL_SET_QUOTA_REQ       77
#define FCFS_AUTH_SERVICE_PROTO_SPOOL_SET_QUOTA_RESP      78

#define FCFS_AUTH_SERVICE_PROTO_GPOOL_GRANT_REQ           81
#define FCFS_AUTH_SERVICE_PROTO_GPOOL_GRANT_RESP          82
#define FCFS_AUTH_SERVICE_PROTO_GPOOL_WITHDRAW_REQ        83
#define FCFS_AUTH_SERVICE_PROTO_GPOOL_WITHDRAW_RESP       84
#define FCFS_AUTH_SERVICE_PROTO_GPOOL_LIST_REQ            85
#define FCFS_AUTH_SERVICE_PROTO_GPOOL_LIST_RESP           86


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

typedef struct fcfs_auth_proto_user_login_req {
    FCFSAuthProtoUserPasswdPair up_pair;
} FCFSAuthProtoUserLoginReq;

typedef struct fcfs_auth_proto_user_login_resp {
    char session_id[FCFS_AUTH_SESSION_ID_LEN];
} FCFSAuthProtoUserLoginResp;

typedef struct fcfs_auth_proto_user_create_req {
    char priv[8];
    FCFSAuthProtoUserPasswdPair up_pair;
} FCFSAuthProtoUserCreateReq;

typedef struct fcfs_auth_proto_user_list_req {
    char username[0];
} FCFSAuthProtoUserListReq;

typedef struct fcfs_auth_proto_list_resp_header {
    char count[4];
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

typedef struct fcfs_auth_proto_spool_grant_req {
    struct {
        char fdir[4];
        char fstore[4];
    } privs;
    FCFSAuthProtoUserPoolPair up_pair;
} FCFSAuthProtoSPoolGrantReq;

typedef struct fcfs_auth_proto_spool_withdraw_req {
    FCFSAuthProtoUserPoolPair up_pair;
} FCFSAuthProtoSPoolWithdrawReq;

typedef FCFSAuthProtoSPoolListReq FCFSAuthProtoGPoolListReq;

typedef struct fcfs_auth_proto_gpool_list_resp_body_part {
    struct {
        char fdir[4];
        char fstore[4];
    } privs;

    FCFSAuthProtoUserPoolPair up_pair;
} FCFSAuthProtoGPoolListRespBodyPart;

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
