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

//service commands
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

typedef SFCommonProtoHeader  FCFSAuthProtoHeader;

typedef struct fcfs_auth_proto_name_info {
    char padding[7];
    char len;
    char str[0];
} FCFSAuthProtoNameInfo;

typedef struct fcfs_auth_proto_user_passwd_pair {
    char passwd[FCFS_AUTH_PASSWD_LEN];
    FCFSAuthProtoNameInfo username;
} FCFSAuthProtoUserPasswdPair;

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

typedef struct fcfs_auth_proto_user_list_resp_header {
    char count[8];
} FCFSAuthProtoUserListRespHeader;

typedef struct fcfs_auth_proto_user_list_resp_body_part {
    char priv[8];
    struct {
        char len;
        char str[0];
    } username;
} FCFSAuthProtoUserListRespBodyPart;

typedef struct fcfs_auth_proto_user_grant_req {
    char priv[8];
    FCFSAuthProtoNameInfo username;
} FCFSAuthProtoUserGrantReq;

typedef struct fcfs_auth_proto_user_remove_req {
    FCFSAuthProtoNameInfo username;
} FCFSAuthProtoUserRemoveReq;


#ifdef __cplusplus
extern "C" {
#endif

void fcfs_auth_proto_init();

const char *fcfs_auth_get_cmd_caption(const int cmd);

#ifdef __cplusplus
}
#endif

#endif