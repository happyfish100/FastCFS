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

#include <sys/stat.h>
#include <limits.h>
#include "fastcommon/ini_file_reader.h"
#include "fastcommon/shared_func.h"
#include "fastcommon/logger.h"
#include "fastcommon/sockopt.h"
#include "fastcommon/connection_pool.h"
#include "auth_proto.h"
#include "auth_func.h"
#include "client_global.h"
#include "client_proto.h"

static inline int pack_user_passwd_pair(const string_t *username,
        const string_t *passwd, FCFSAuthProtoUserPasswdPair *up_pair)
{
    if (username->len <= 0 || username->len > NAME_MAX) {
        logError("file: "__FILE__", line: %d, "
                "invalid username length: %d, which <= 0 or > %d",
                __LINE__, username->len, NAME_MAX);
        return EINVAL;
    }

    if (passwd->len != FCFS_AUTH_PASSWD_LEN) {
        logError("file: "__FILE__", line: %d, "
                "invalid password length: %d != %d",
                __LINE__, passwd->len, FCFS_AUTH_PASSWD_LEN);
        return EINVAL;
    }

    memcpy(up_pair->passwd, passwd->str, passwd->len);
    memcpy(up_pair->username.str, username->str, username->len);
    up_pair->username.len = username->len;

    return 0;
}

int fcfs_auth_client_proto_user_login(FCFSAuthClientContext *client_ctx,
        ConnectionInfo *conn, const string_t *username,
        const string_t *passwd)
{
    FCFSAuthProtoHeader *header;
    FCFSAuthProtoUserLoginReq *req;
    char out_buff[sizeof(FCFSAuthProtoHeader) +
        sizeof(FCFSAuthProtoUserLoginReq) +
        NAME_MAX + FCFS_AUTH_PASSWD_LEN];
    SFResponseInfo response;
    FCFSAuthProtoUserLoginResp login_resp;
    int out_bytes;
    int result;

    header = (FCFSAuthProtoHeader *)out_buff;
    req = (FCFSAuthProtoUserLoginReq *)(header + 1);
    out_bytes = sizeof(FCFSAuthProtoHeader) + sizeof(*req) + username->len;
    SF_PROTO_SET_HEADER(header, FCFS_AUTH_SERVICE_PROTO_USER_LOGIN_REQ,
            out_bytes - sizeof(FCFSAuthProtoHeader));
    if ((result=pack_user_passwd_pair(username,
                    passwd, &req->up_pair)) != 0)
    {
        return result;
    }

    response.error.length = 0;
    if ((result=sf_send_and_recv_response(conn, out_buff, out_bytes,
                    &response, client_ctx->common_cfg.network_timeout,
                    FCFS_AUTH_SERVICE_PROTO_USER_LOGIN_RESP,
                    (char *)&login_resp, sizeof(login_resp))) == 0)
    {
        memcpy(client_ctx->session_id, login_resp.session_id,
                FCFS_AUTH_SESSION_ID_LEN);
    } else {
        sf_log_network_error(&response, conn, result);
    }

    return result;
}

int fcfs_auth_client_proto_user_create(FCFSAuthClientContext *client_ctx,
        ConnectionInfo *conn, const FCFSAuthUserInfo *user)
{
    FCFSAuthProtoHeader *header;
    FCFSAuthProtoUserCreateReq *req;
    char out_buff[sizeof(FCFSAuthProtoHeader) +
        sizeof(FCFSAuthProtoUserCreateReq) +
        NAME_MAX + FCFS_AUTH_PASSWD_LEN];
    SFResponseInfo response;
    int out_bytes;
    int result;

    header = (FCFSAuthProtoHeader *)out_buff;
    req = (FCFSAuthProtoUserCreateReq *)(header + 1);
    out_bytes = sizeof(FCFSAuthProtoHeader) + sizeof(*req) + user->name.len;
    SF_PROTO_SET_HEADER(header, FCFS_AUTH_SERVICE_PROTO_USER_CREATE_REQ,
            out_bytes - sizeof(FCFSAuthProtoHeader));
    if ((result=pack_user_passwd_pair(&user->name,
                    &user->passwd, &req->up_pair)) != 0)
    {
        return result;
    }
    long2buff(user->priv, req->priv);

    response.error.length = 0;
    if ((result=sf_send_and_recv_none_body_response(conn, out_buff, out_bytes,
                    &response, client_ctx->common_cfg.network_timeout,
                    FCFS_AUTH_SERVICE_PROTO_USER_CREATE_RESP)) != 0)
    {
        sf_log_network_error(&response, conn, result);
    }

    return result;
}
