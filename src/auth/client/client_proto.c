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

#define check_name(name, caption) check_name_ex(name, caption, true)

#define check_username_ex(username, required) \
    check_name_ex(username, "username", required)
#define check_username(username) check_username_ex(username, true)

#define pack_username_ex(username, proto_uname, required) \
    pack_name_ex(username, "username", proto_uname, 0, required)
#define pack_username(username, proto_uname) \
     pack_username_ex(username, proto_uname, true)

#define check_poolname_ex(poolname, required) \
    check_name_ex(poolname, "poolname", required)
#define check_poolname(poolname) check_poolname_ex(poolname, true)

#define pack_poolname_ex(poolname, proto_pname, str_offset, required) \
    pack_name_ex(poolname, "poolname", proto_pname, str_offset, required)
#define pack_poolname(poolname, proto_pname) \
    pack_poolname_ex(poolname, proto_pname, 0, true)

#define pack_user_pool_pair(username, poolname, up_pair) \
    pack_user_pool_pair_ex(username, poolname, up_pair, true)

static inline int check_name_ex(const string_t *name,
        const char *caption, const bool required)
{
    int min_length;

    min_length = required ? 1 : 0;
    if (name->len < min_length || name->len > NAME_MAX) {
        logError("file: "__FILE__", line: %d, "
                "invalid %s length: %d, which <= 0 or > %d",
                __LINE__, caption, name->len, NAME_MAX);
        return EINVAL;
    }

    return 0;
}

static inline int pack_name_ex(const string_t *name, const char *caption,
        FCFSAuthProtoNameInfo *proto_name, const int str_offset,
        const bool required)
{
    int result;

    if ((result=check_name_ex(name, caption, required)) != 0) {
        return result;
    }
    if (name->len > 0) {
        memcpy(proto_name->str + str_offset, name->str, name->len);
    }
    proto_name->len = name->len;
    return 0;
}

static inline int pack_user_pool_pair_ex(const string_t *username,
        const string_t *poolname, FCFSAuthProtoUserPoolPair *up_pair,
        const bool required)
{
    int result;

    if ((result=check_username_ex(username, required)) != 0) {
        return result;
    }

    if ((result=check_poolname_ex(poolname, required)) != 0) {
        return result;
    }

    fcfs_auth_pack_user_pool_pair(username, poolname, up_pair);
    return 0;
}

static inline int pack_user_passwd_pair(const string_t *username,
        const string_t *passwd, FCFSAuthProtoUserPasswdPair *up_pair)
{
    if (passwd->len != FCFS_AUTH_PASSWD_LEN) {
        logError("file: "__FILE__", line: %d, "
                "invalid password length: %d != %d",
                __LINE__, passwd->len, FCFS_AUTH_PASSWD_LEN);
        return EINVAL;
    }
    memcpy(up_pair->passwd, passwd->str, passwd->len);
    return pack_username(username, &up_pair->username);
}

int fcfs_auth_client_proto_user_login(FCFSAuthClientContext *client_ctx,
        ConnectionInfo *conn, const string_t *username,
        const string_t *passwd, const string_t *poolname,
        const int flags)
{
    FCFSAuthProtoHeader *header;
    FCFSAuthProtoUserLoginReq *req;
    FCFSAuthProtoNameInfo *proto_pname;
    char out_buff[sizeof(FCFSAuthProtoHeader) +
        sizeof(FCFSAuthProtoUserLoginReq) +
        2 * NAME_MAX + FCFS_AUTH_PASSWD_LEN];
    SFResponseInfo response;
    FCFSAuthProtoUserLoginResp login_resp;
    int out_bytes;
    int result;

    header = (FCFSAuthProtoHeader *)out_buff;
    req = (FCFSAuthProtoUserLoginReq *)(header + 1);
    out_bytes = sizeof(FCFSAuthProtoHeader) + sizeof(*req) +
        username->len + poolname->len;
    SF_PROTO_SET_HEADER(header, FCFS_AUTH_SERVICE_PROTO_USER_LOGIN_REQ,
            out_bytes - sizeof(FCFSAuthProtoHeader));
    req->flags = flags;
    if ((result=pack_user_passwd_pair(username,
                    passwd, &req->up_pair)) != 0)
    {
        return result;
    }

    proto_pname = (FCFSAuthProtoNameInfo *)(req->
            up_pair.username.str + username->len);
    if ((result=pack_poolname_ex(poolname, proto_pname, 0, false)) != 0) {
        return result;
    }

    response.error.length = 0;
    if ((result=sf_send_and_recv_response(conn, out_buff, out_bytes,
                    &response, client_ctx->common_cfg.network_timeout,
                    FCFS_AUTH_SERVICE_PROTO_USER_LOGIN_RESP,
                    (char *)&login_resp, sizeof(login_resp))) == 0)
    {
        memcpy(client_ctx->session.id, login_resp.session_id,
                FCFS_AUTH_SESSION_ID_LEN);
    } else {
        sf_log_network_error(&response, conn, result);
    }

    return result;
}

int fcfs_auth_client_proto_session_subscribe(
        FCFSAuthClientContext *client_ctx, ConnectionInfo *conn)
{
    FCFSAuthProtoHeader *header;
    FCFSAuthProtoSessionSubscribeReq *req;
    char out_buff[sizeof(FCFSAuthProtoHeader) +
        sizeof(FCFSAuthProtoSessionSubscribeReq) +
        NAME_MAX + FCFS_AUTH_PASSWD_LEN];
    SFResponseInfo response;
    int out_bytes;
    int result;

    header = (FCFSAuthProtoHeader *)out_buff;
    req = (FCFSAuthProtoSessionSubscribeReq *)(header + 1);
    out_bytes = sizeof(FCFSAuthProtoHeader) + sizeof(*req) +
        client_ctx->auth_cfg.username.len;
    SF_PROTO_SET_HEADER(header, FCFS_AUTH_SERVICE_PROTO_SESSION_SUBSCRIBE_REQ,
            out_bytes - sizeof(FCFSAuthProtoHeader));
    if ((result=pack_user_passwd_pair(&client_ctx->auth_cfg.username,
                    &client_ctx->auth_cfg.passwd, &req->up_pair)) != 0)
    {
        return result;
    }

    response.error.length = 0;
    if ((result=sf_send_and_recv_none_body_response(conn, out_buff, out_bytes,
                    &response, client_ctx->common_cfg.network_timeout,
                    FCFS_AUTH_SERVICE_PROTO_SESSION_SUBSCRIBE_RESP)) != 0)
    {
        sf_log_network_error(&response, conn, result);
    }

    return result;
}

int fcfs_auth_client_proto_session_validate(
        FCFSAuthClientContext *client_ctx, ConnectionInfo *conn,
        const string_t *session_id, const string_t *validate_key,
        const FCFSAuthValidatePriviledgeType priv_type,
        const int64_t pool_id, const int64_t priv_required)
{
    FCFSAuthProtoHeader *header;
    FCFSAuthProtoSessionValidateReq *req;
    char out_buff[sizeof(FCFSAuthProtoHeader) +
        sizeof(FCFSAuthProtoSessionValidateReq) +
        NAME_MAX + FCFS_AUTH_PASSWD_LEN];
    FCFSAuthProtoSessionValidateResp validate_resp;
    SFResponseInfo response;
    int out_bytes;
    int result;

    header = (FCFSAuthProtoHeader *)out_buff;
    req = (FCFSAuthProtoSessionValidateReq *)(header + 1);
    out_bytes = sizeof(FCFSAuthProtoHeader) + sizeof(*req);
    SF_PROTO_SET_HEADER(header, FCFS_AUTH_SERVICE_PROTO_SESSION_VALIDATE_REQ,
            out_bytes - sizeof(FCFSAuthProtoHeader));
    if (validate_key->len != FCFS_AUTH_PASSWD_LEN) {
        logError("file: "__FILE__", line: %d, "
                "invalid session validate key length: %d != %d",
                __LINE__, validate_key->len, FCFS_AUTH_PASSWD_LEN);
        return EINVAL;
    }
    memcpy(req->validate_key, validate_key->str, validate_key->len);

    if (session_id->len != FCFS_AUTH_SESSION_ID_LEN) {
        logError("file: "__FILE__", line: %d, "
                "invalid session id length: %d != %d", __LINE__,
                session_id->len, FCFS_AUTH_SESSION_ID_LEN);
        return EINVAL;
    }
    memcpy(req->session_id, session_id->str, session_id->len);
    req->priv_type = priv_type;
    long2buff(pool_id, req->pool_id);
    long2buff(priv_required, req->priv_required);

    response.error.length = 0;
    if ((result=sf_send_and_recv_response(conn, out_buff, out_bytes,
                    &response, client_ctx->common_cfg.network_timeout,
                    FCFS_AUTH_SERVICE_PROTO_SESSION_VALIDATE_RESP,
                    (char *)&validate_resp, sizeof(validate_resp))) == 0)
    {
        result = buff2int(validate_resp.result);
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

int fcfs_auth_client_proto_user_passwd(FCFSAuthClientContext *client_ctx,
        ConnectionInfo *conn, const string_t *username,
        const string_t *passwd)
{
    FCFSAuthProtoHeader *header;
    FCFSAuthProtoUserPasswdReq *req;
    char out_buff[sizeof(FCFSAuthProtoHeader) +
        sizeof(FCFSAuthProtoUserPasswdReq) +
        NAME_MAX + FCFS_AUTH_PASSWD_LEN];
    SFResponseInfo response;
    int out_bytes;
    int result;

    header = (FCFSAuthProtoHeader *)out_buff;
    req = (FCFSAuthProtoUserPasswdReq *)(header + 1);
    out_bytes = sizeof(FCFSAuthProtoHeader) + sizeof(*req) + username->len;
    SF_PROTO_SET_HEADER(header, FCFS_AUTH_SERVICE_PROTO_USER_PASSWD_REQ,
            out_bytes - sizeof(FCFSAuthProtoHeader));
    if ((result=pack_user_passwd_pair(username, passwd,
                    &req->up_pair)) != 0)
    {
        return result;
    }

    response.error.length = 0;
    if ((result=sf_send_and_recv_none_body_response(conn, out_buff, out_bytes,
                    &response, client_ctx->common_cfg.network_timeout,
                    FCFS_AUTH_SERVICE_PROTO_USER_PASSWD_RESP)) != 0)
    {
        sf_log_network_error(&response, conn, result);
    }

    return result;
}

typedef int (*client_proto_list_func)(FCFSAuthClientContext *client_ctx,
        ConnectionInfo *conn, const string_t *username,
        const string_t *poolname, const SFListLimitInfo *limit,
        SFProtoRecvBuffer *buffer, struct fast_mpool_man *mpool,
        void *array, bool *is_last);

static int client_proto_user_list_do(FCFSAuthClientContext *client_ctx,
        ConnectionInfo *conn, const string_t *username,
        const string_t *poolname, const SFListLimitInfo *limit,
        SFProtoRecvBuffer *buffer, struct fast_mpool_man *mpool,
        FCFSAuthUserArray *array, bool *is_last)
{
    FCFSAuthProtoHeader *header;
    FCFSAuthProtoUserListReq *req;
    char out_buff[sizeof(FCFSAuthProtoHeader) +
        sizeof(FCFSAuthProtoUserListReq) + NAME_MAX];
    SFResponseInfo response;
    FCFSAuthProtoListRespHeader *resp_header;
    FCFSAuthProtoUserListRespBodyPart *body_part;
    FCFSAuthUserInfo *user;
    FCFSAuthUserInfo *end;
    char *p;
    int out_bytes;
    int count;
    int result;

    header = (FCFSAuthProtoHeader *)out_buff;
    req = (FCFSAuthProtoUserListReq *)(header + 1);
    if ((result=pack_username_ex(username, &req->username, false)) != 0) {
        return result;
    }

    out_bytes = sizeof(FCFSAuthProtoHeader) +
        sizeof(FCFSAuthProtoUserListReq) + username->len;
    SF_PROTO_SET_HEADER(header, FCFS_AUTH_SERVICE_PROTO_USER_LIST_REQ,
            out_bytes - sizeof(FCFSAuthProtoHeader));
    sf_proto_pack_limit(limit, &req->limit);

    response.error.length = 0;
    if ((result=sf_send_and_recv_vary_response(conn, out_buff, out_bytes,
                    &response, client_ctx->common_cfg.network_timeout,
                    FCFS_AUTH_SERVICE_PROTO_USER_LIST_RESP, buffer,
                    sizeof(FCFSAuthProtoListRespHeader))) != 0)
    {
        sf_log_network_error(&response, conn, result);
        return result;
    }

    resp_header = (FCFSAuthProtoListRespHeader *)buffer->buff;
    count = buff2int(resp_header->count);
    *is_last = resp_header->is_last;
    if ((result=fcfs_auth_user_check_realloc_array(array,
                    array->count + count)) != 0)
    {
        return result;
    }

    p = (char *)(resp_header + 1);
    end = array->users + array->count + count;
    for (user=array->users+array->count; user<end; user++) {
        body_part = (FCFSAuthProtoUserListRespBodyPart *)p;
        user->priv = buff2long(body_part->priv);
        if ((result=fast_mpool_alloc_string_ex(mpool,
                        &user->name, body_part->username.str,
                        body_part->username.len)) != 0)
        {
            return result;
        }
        p += sizeof(FCFSAuthProtoUserListRespBodyPart) + user->name.len;
    }

    if (response.header.body_len != p - buffer->buff) {
        logError("file: "__FILE__", line: %d, "
                "response body length: %d != expect: %d",
                __LINE__, response.header.body_len,
                (int)(p - buffer->buff));
        return EINVAL;
    }

    array->count += count;
    return result;
}

static int client_proto_list_wrapper(client_proto_list_func list_func,
        FCFSAuthClientContext *client_ctx, ConnectionInfo *conn,
        const string_t *username, const string_t *poolname,
        const SFListLimitInfo *limit, struct fast_mpool_man *mpool,
        void *array, int *count)
{
    int result;
    bool is_last;
    SFProtoRBufferFixedWrapper buffer_wrapper;
    SFListLimitInfo new_limit;

    sf_init_recv_buffer_by_wrapper(&buffer_wrapper);
    if (limit->count > 0 && limit->count <= 64) {
        result = list_func(client_ctx, conn,
                username, poolname, limit, &buffer_wrapper.buffer,
                mpool, array, &is_last);
        sf_free_recv_buffer(&buffer_wrapper.buffer);
        return result;
    }

    result = 0;
    is_last = false;
    while (!is_last) {
        new_limit.offset = limit->offset + *count;
        if (limit->count <= 0) {
            new_limit.count = 0;
        } else {
            new_limit.count = limit->count - *count;
            if (new_limit.count == 0) {
                break;
            }
        }

        if ((result=list_func(client_ctx, conn, username,
                        poolname, &new_limit, &buffer_wrapper.buffer,
                        mpool, array, &is_last)) != 0)
        {
            break;
        }
    }

    sf_free_recv_buffer(&buffer_wrapper.buffer);
    return result;
}

int fcfs_auth_client_proto_user_list(FCFSAuthClientContext *client_ctx,
        ConnectionInfo *conn, const string_t *username,
        const SFListLimitInfo *limit, struct fast_mpool_man *mpool,
        FCFSAuthUserArray *array)
{
    const string_t *poolname = NULL;
    return client_proto_list_wrapper((client_proto_list_func)
            client_proto_user_list_do, client_ctx, conn, username,
            poolname, limit, mpool, array, &array->count);
}

int fcfs_auth_client_proto_user_grant(FCFSAuthClientContext *client_ctx,
        ConnectionInfo *conn, const string_t *username, const int64_t priv)
{
    FCFSAuthProtoHeader *header;
    FCFSAuthProtoUserGrantReq *req;
    char out_buff[sizeof(FCFSAuthProtoHeader) +
        sizeof(FCFSAuthProtoUserGrantReq) + NAME_MAX];
    SFResponseInfo response;
    int out_bytes;
    int result;

    header = (FCFSAuthProtoHeader *)out_buff;
    req = (FCFSAuthProtoUserGrantReq *)(header + 1);
    out_bytes = sizeof(FCFSAuthProtoHeader) + sizeof(*req) + username->len;
    SF_PROTO_SET_HEADER(header, FCFS_AUTH_SERVICE_PROTO_USER_GRANT_REQ,
            out_bytes - sizeof(FCFSAuthProtoHeader));
    if ((result=pack_username(username, &req->username)) != 0) {
        return result;
    }
    long2buff(priv, req->priv);

    response.error.length = 0;
    if ((result=sf_send_and_recv_none_body_response(conn, out_buff, out_bytes,
                    &response, client_ctx->common_cfg.network_timeout,
                    FCFS_AUTH_SERVICE_PROTO_USER_GRANT_RESP)) != 0)
    {
        sf_log_network_error(&response, conn, result);
    }

    return result;
}

int fcfs_auth_client_proto_user_remove(FCFSAuthClientContext *client_ctx,
        ConnectionInfo *conn, const string_t *username)
{
    FCFSAuthProtoHeader *header;
    FCFSAuthProtoUserRemoveReq *req;
    char out_buff[sizeof(FCFSAuthProtoHeader) +
        sizeof(FCFSAuthProtoUserRemoveReq) + NAME_MAX];
    SFResponseInfo response;
    int out_bytes;
    int result;

    header = (FCFSAuthProtoHeader *)out_buff;
    req = (FCFSAuthProtoUserRemoveReq *)(header + 1);
    out_bytes = sizeof(FCFSAuthProtoHeader) + sizeof(*req) + username->len;
    SF_PROTO_SET_HEADER(header, FCFS_AUTH_SERVICE_PROTO_USER_REMOVE_REQ,
            out_bytes - sizeof(FCFSAuthProtoHeader));
    if ((result=pack_username(username, &req->username)) != 0) {
        return result;
    }

    response.error.length = 0;
    if ((result=sf_send_and_recv_none_body_response(conn, out_buff, out_bytes,
                    &response, client_ctx->common_cfg.network_timeout,
                    FCFS_AUTH_SERVICE_PROTO_USER_REMOVE_RESP)) != 0)
    {
        sf_log_network_error(&response, conn, result);
    }

    return result;
}

int fcfs_auth_client_proto_spool_create(FCFSAuthClientContext *client_ctx,
        ConnectionInfo *conn, FCFSAuthStoragePoolInfo *spool,
        const int name_size, const bool dryrun)
{
    FCFSAuthProtoHeader *header;
    FCFSAuthProtoSPoolCreateReq *req;
    FCFSAuthProtoSPoolCreateResp *resp;
    char out_buff[sizeof(FCFSAuthProtoHeader) +
        sizeof(FCFSAuthProtoSPoolCreateReq) + NAME_MAX];
    char in_buff[sizeof(FCFSAuthProtoSPoolCreateResp) + NAME_MAX];
    SFResponseInfo response;
    int out_bytes;
    int body_len;
    int buff_size;
    int result;

    header = (FCFSAuthProtoHeader *)out_buff;
    req = (FCFSAuthProtoSPoolCreateReq *)(header + 1);
    out_bytes = sizeof(FCFSAuthProtoHeader) + sizeof(*req) + spool->name.len;
    SF_PROTO_SET_HEADER(header, FCFS_AUTH_SERVICE_PROTO_SPOOL_CREATE_REQ,
            out_bytes - sizeof(FCFSAuthProtoHeader));
    if ((result=pack_poolname_ex(&spool->name,
                    &req->poolname, 0, false)) != 0)
    {
        return result;
    }
    long2buff(spool->quota, req->quota);
    req->dryrun = (dryrun ? 1 : 0);

    if (name_size < sizeof(in_buff) - sizeof(*resp)) {
        buff_size = name_size;
    } else {
        buff_size = sizeof(in_buff);
    }
    response.error.length = 0;
    if ((result=sf_send_and_recv_response_ex1(conn, out_buff, out_bytes,
                    &response, client_ctx->common_cfg.network_timeout,
                    FCFS_AUTH_SERVICE_PROTO_SPOOL_CREATE_RESP, in_buff,
                    buff_size, &body_len)) != 0)
    {
        sf_log_network_error(&response, conn, result);
        return result;
    }

    resp = (FCFSAuthProtoSPoolCreateResp *)in_buff;
    if (body_len != sizeof(*resp) + resp->poolname.len) {
        logError("file: "__FILE__", line: %d, "
                "response body length: %d != expect: %d", __LINE__,
                body_len, (int)sizeof(*resp) + resp->poolname.len);
        return EINVAL;
    }

    spool->name.len = resp->poolname.len;
    memcpy(spool->name.str, resp->poolname.str, spool->name.len);
    return 0;
}

int client_proto_spool_list_do(FCFSAuthClientContext *client_ctx,
        ConnectionInfo *conn, const string_t *username,
        const string_t *poolname, const SFListLimitInfo *limit,
        SFProtoRecvBuffer *buffer, struct fast_mpool_man *mpool,
        FCFSAuthStoragePoolArray *array, bool *is_last)
{
    FCFSAuthProtoHeader *header;
    char out_buff[sizeof(FCFSAuthProtoHeader) +
        sizeof(FCFSAuthProtoSPoolListReq) + 2 * NAME_MAX];
    SFResponseInfo response;
    FCFSAuthProtoSPoolListReq *req;
    FCFSAuthProtoListRespHeader *resp_header;
    FCFSAuthProtoSPoolListRespBodyPart *body_part;
    FCFSAuthStoragePoolInfo *spool;
    FCFSAuthStoragePoolInfo *end;
    char *p;
    int out_bytes;
    int count;
    int result;

    header = (FCFSAuthProtoHeader *)out_buff;
    out_bytes = sizeof(FCFSAuthProtoHeader) +
        sizeof(FCFSAuthProtoSPoolListReq) +
        username->len + poolname->len;
    SF_PROTO_SET_HEADER(header, FCFS_AUTH_SERVICE_PROTO_SPOOL_LIST_REQ,
            out_bytes - sizeof(FCFSAuthProtoHeader));

    req = (FCFSAuthProtoSPoolListReq *)(header + 1);
    if ((result=pack_user_pool_pair_ex(username, poolname,
                    &req->up_pair, false)) != 0)
    {
        return result;
    }
    sf_proto_pack_limit(limit, &req->limit);

    response.error.length = 0;
    if ((result=sf_send_and_recv_vary_response(conn, out_buff, out_bytes,
                    &response, client_ctx->common_cfg.network_timeout,
                    FCFS_AUTH_SERVICE_PROTO_SPOOL_LIST_RESP, buffer,
                    sizeof(FCFSAuthProtoListRespHeader))) != 0)
    {
        sf_log_network_error(&response, conn, result);
        return result;
    }

    resp_header = (FCFSAuthProtoListRespHeader *)buffer->buff;
    count = buff2int(resp_header->count);
    *is_last = resp_header->is_last;
    if ((result=fcfs_auth_spool_check_realloc_array(array,
                    array->count + count)) != 0)
    {
        return result;
    }

    p = (char *)(resp_header + 1);
    end = array->spools + array->count + count;
    for (spool=array->spools+array->count; spool<end; spool++) {
        body_part = (FCFSAuthProtoSPoolListRespBodyPart *)p;
        spool->quota = buff2long(body_part->quota);
        if ((result=fast_mpool_alloc_string_ex(mpool,
                        &spool->name, body_part->poolname.str,
                        body_part->poolname.len)) != 0)
        {
            return result;
        }
        p += sizeof(FCFSAuthProtoSPoolListRespBodyPart) + spool->name.len;
    }

    if (response.header.body_len != p - buffer->buff) {
        logError("file: "__FILE__", line: %d, "
                "response body length: %d != expect: %d",
                __LINE__, response.header.body_len,
                (int)(p - buffer->buff));
        return EINVAL;
    }

    array->count += count;
    return result;
}

int fcfs_auth_client_proto_spool_list(FCFSAuthClientContext *client_ctx,
        ConnectionInfo *conn, const string_t *username,
        const string_t *poolname, const SFListLimitInfo *limit,
        struct fast_mpool_man *mpool, FCFSAuthStoragePoolArray *array)
{
    return client_proto_list_wrapper((client_proto_list_func)
            client_proto_spool_list_do, client_ctx, conn, username,
            poolname, limit, mpool, array, &array->count);
}

int fcfs_auth_client_proto_spool_remove(FCFSAuthClientContext *client_ctx,
        ConnectionInfo *conn, const string_t *poolname)
{
    FCFSAuthProtoHeader *header;
    FCFSAuthProtoSPoolRemoveReq *req;
    char out_buff[sizeof(FCFSAuthProtoHeader) +
        sizeof(FCFSAuthProtoSPoolRemoveReq) + NAME_MAX];
    SFResponseInfo response;
    int out_bytes;
    int result;

    header = (FCFSAuthProtoHeader *)out_buff;
    req = (FCFSAuthProtoSPoolRemoveReq *)(header + 1);
    out_bytes = sizeof(FCFSAuthProtoHeader) + sizeof(*req) + poolname->len;
    SF_PROTO_SET_HEADER(header, FCFS_AUTH_SERVICE_PROTO_SPOOL_REMOVE_REQ,
            out_bytes - sizeof(FCFSAuthProtoHeader));
    if ((result=pack_poolname(poolname, &req->poolname)) != 0) {
        return result;
    }

    response.error.length = 0;
    if ((result=sf_send_and_recv_none_body_response(conn, out_buff, out_bytes,
                    &response, client_ctx->common_cfg.network_timeout,
                    FCFS_AUTH_SERVICE_PROTO_SPOOL_REMOVE_RESP)) != 0)
    {
        sf_log_network_error(&response, conn, result);
    }

    return result;
}

int fcfs_auth_client_proto_spool_set_quota(FCFSAuthClientContext *client_ctx,
        ConnectionInfo *conn, const string_t *poolname, const int64_t quota)
{
    FCFSAuthProtoHeader *header;
    FCFSAuthProtoSPoolSetQuotaReq *req;
    char out_buff[sizeof(FCFSAuthProtoHeader) +
        sizeof(FCFSAuthProtoSPoolSetQuotaReq) + NAME_MAX];
    SFResponseInfo response;
    int out_bytes;
    int result;

    header = (FCFSAuthProtoHeader *)out_buff;
    req = (FCFSAuthProtoSPoolSetQuotaReq *)(header + 1);
    out_bytes = sizeof(FCFSAuthProtoHeader) + sizeof(*req) + poolname->len;
    SF_PROTO_SET_HEADER(header, FCFS_AUTH_SERVICE_PROTO_SPOOL_SET_QUOTA_REQ,
            out_bytes - sizeof(FCFSAuthProtoHeader));
    if ((result=pack_poolname(poolname, &req->poolname)) != 0) {
        return result;
    }
    long2buff(quota, req->quota);

    response.error.length = 0;
    if ((result=sf_send_and_recv_none_body_response(conn, out_buff, out_bytes,
                    &response, client_ctx->common_cfg.network_timeout,
                    FCFS_AUTH_SERVICE_PROTO_SPOOL_SET_QUOTA_RESP)) != 0)
    {
        sf_log_network_error(&response, conn, result);
    }

    return result;
}

int fcfs_auth_client_proto_spool_get_quota(FCFSAuthClientContext *client_ctx,
        ConnectionInfo *conn, const string_t *poolname, int64_t *quota)
{
    FCFSAuthProtoHeader *header;
    FCFSAuthProtoSPoolGetQuotaReq *req;
    FCFSAuthProtoSPoolGetQuotaResp resp;
    char out_buff[sizeof(FCFSAuthProtoHeader) +
        sizeof(FCFSAuthProtoSPoolGetQuotaReq) + NAME_MAX];
    SFResponseInfo response;
    int out_bytes;
    int result;

    header = (FCFSAuthProtoHeader *)out_buff;
    req = (FCFSAuthProtoSPoolGetQuotaReq *)(header + 1);
    out_bytes = sizeof(FCFSAuthProtoHeader) + sizeof(*req) + poolname->len;
    SF_PROTO_SET_HEADER(header, FCFS_AUTH_SERVICE_PROTO_SPOOL_GET_QUOTA_REQ,
            out_bytes - sizeof(FCFSAuthProtoHeader));
    if ((result=pack_poolname(poolname, &req->poolname)) != 0) {
        return result;
    }

    response.error.length = 0;
    if ((result=sf_send_and_recv_response(conn, out_buff, out_bytes,
                    &response, client_ctx->common_cfg.network_timeout,
                    FCFS_AUTH_SERVICE_PROTO_SPOOL_GET_QUOTA_RESP,
                    (char *)&resp, sizeof(resp))) == 0)
    {
        *quota = buff2long(resp.quota);
    } else {
        sf_log_network_error(&response, conn, result);
    }

    return result;
}

int fcfs_auth_client_proto_gpool_grant(FCFSAuthClientContext *client_ctx,
        ConnectionInfo *conn, const string_t *username, const string_t
        *poolname, const FCFSAuthSPoolPriviledges *privs)
{
    FCFSAuthProtoHeader *header;
    char out_buff[sizeof(FCFSAuthProtoHeader) +
        sizeof(FCFSAuthProtoSPoolGrantReq) + 2 * NAME_MAX];
    SFResponseInfo response;
    FCFSAuthProtoSPoolGrantReq *req;
    int out_bytes;
    int result;

    header = (FCFSAuthProtoHeader *)out_buff;
    out_bytes = sizeof(FCFSAuthProtoHeader) +
        sizeof(FCFSAuthProtoSPoolGrantReq) +
        username->len + poolname->len;
    SF_PROTO_SET_HEADER(header, FCFS_AUTH_SERVICE_PROTO_GPOOL_GRANT_REQ,
            out_bytes - sizeof(FCFSAuthProtoHeader));

    req = (FCFSAuthProtoSPoolGrantReq *)(header + 1);
    if ((result=pack_user_pool_pair(username, poolname,
                    &req->up_pair)) != 0)
    {
        return result;
    }
    int2buff(privs->fdir, req->privs.fdir);
    int2buff(privs->fstore, req->privs.fstore);

    response.error.length = 0;
    if ((result=sf_send_and_recv_none_body_response(conn, out_buff, out_bytes,
                    &response, client_ctx->common_cfg.network_timeout,
                    FCFS_AUTH_SERVICE_PROTO_GPOOL_GRANT_RESP)) != 0)
    {
        sf_log_network_error(&response, conn, result);
    }

    return result;
}

int fcfs_auth_client_proto_gpool_withdraw(FCFSAuthClientContext
        *client_ctx, ConnectionInfo *conn, const string_t *username,
        const string_t *poolname)
{
    FCFSAuthProtoHeader *header;
    char out_buff[sizeof(FCFSAuthProtoHeader) +
        sizeof(FCFSAuthProtoSPoolWithdrawReq) + 2 * NAME_MAX];
    SFResponseInfo response;
    FCFSAuthProtoSPoolWithdrawReq *req;
    int out_bytes;
    int result;

    header = (FCFSAuthProtoHeader *)out_buff;
    out_bytes = sizeof(FCFSAuthProtoHeader) +
        sizeof(FCFSAuthProtoSPoolWithdrawReq) +
        username->len + poolname->len;
    SF_PROTO_SET_HEADER(header, FCFS_AUTH_SERVICE_PROTO_GPOOL_WITHDRAW_REQ,
            out_bytes - sizeof(FCFSAuthProtoHeader));

    req = (FCFSAuthProtoSPoolWithdrawReq *)(header + 1);
    if ((result=pack_user_pool_pair(username, poolname,
                    &req->up_pair)) != 0)
    {
        return result;
    }

    response.error.length = 0;
    if ((result=sf_send_and_recv_none_body_response(conn, out_buff, out_bytes,
                    &response, client_ctx->common_cfg.network_timeout,
                    FCFS_AUTH_SERVICE_PROTO_GPOOL_WITHDRAW_RESP)) != 0)
    {
        sf_log_network_error(&response, conn, result);
    }

    return result;
}

static int client_proto_gpool_list_do(FCFSAuthClientContext
        *client_ctx, ConnectionInfo *conn, const string_t *username,
        const string_t *poolname, const SFListLimitInfo *limit,
        SFProtoRecvBuffer *buffer, struct fast_mpool_man *mpool,
        FCFSAuthGrantedPoolArray *array, bool *is_last)
{
    FCFSAuthProtoHeader *header;
    char out_buff[sizeof(FCFSAuthProtoHeader) +
        sizeof(FCFSAuthProtoGPoolListReq) + 2 * NAME_MAX];
    SFResponseInfo response;
    FCFSAuthProtoGPoolListReq *req;
    FCFSAuthProtoListRespHeader *resp_header;
    FCFSAuthProtoGPoolListRespBodyPart *body_part;
    FCFSAuthGrantedPoolFullInfo *gpool;
    FCFSAuthGrantedPoolFullInfo *end;
    char *p;
    int out_bytes;
    int count;
    int result;

    header = (FCFSAuthProtoHeader *)out_buff;
    out_bytes = sizeof(FCFSAuthProtoHeader) +
        sizeof(FCFSAuthProtoGPoolListReq) +
        username->len + poolname->len;
    SF_PROTO_SET_HEADER(header, FCFS_AUTH_SERVICE_PROTO_GPOOL_LIST_REQ,
            out_bytes - sizeof(FCFSAuthProtoHeader));

    req = (FCFSAuthProtoGPoolListReq *)(header + 1);
    if ((result=pack_user_pool_pair_ex(username, poolname,
                    &req->up_pair, false)) != 0)
    {
        return result;
    }
    sf_proto_pack_limit(limit, &req->limit);

    response.error.length = 0;
    if ((result=sf_send_and_recv_vary_response(conn, out_buff, out_bytes,
                    &response, client_ctx->common_cfg.network_timeout,
                    FCFS_AUTH_SERVICE_PROTO_GPOOL_LIST_RESP, buffer,
                    sizeof(FCFSAuthProtoListRespHeader))) != 0)
    {
        sf_log_network_error(&response, conn, result);
        return result;
    }

    resp_header = (FCFSAuthProtoListRespHeader *)buffer->buff;
    count = buff2int(resp_header->count);
    *is_last = resp_header->is_last;
    if ((result=fcfs_auth_gpool_check_realloc_array(array,
                    array->count + count)) != 0)
    {
        return result;
    }

    p = (char *)(resp_header + 1);
    end = array->gpools + array->count + count;
    for (gpool=array->gpools+array->count; gpool<end; gpool++) {
        body_part = (FCFSAuthProtoGPoolListRespBodyPart *)p;
        gpool->granted.privs.fdir = buff2int(body_part->privs.fdir);
        gpool->granted.privs.fstore = buff2int(body_part->privs.fstore);
        if ((result=fast_mpool_alloc_string_ex(mpool,
                        &gpool->username, body_part->up_pair.username.str,
                        body_part->up_pair.username.len)) != 0)
        {
            return result;
        }
        if ((result=fast_mpool_alloc_string_ex(mpool,
                        &gpool->pool_name, body_part->up_pair.poolname.str,
                        body_part->up_pair.poolname.len)) != 0)
        {
            return result;
        }

        p += sizeof(FCFSAuthProtoGPoolListRespBodyPart)
            + gpool->username.len + gpool->pool_name.len;
    }

    if (response.header.body_len != p - buffer->buff) {
        logError("file: "__FILE__", line: %d, "
                "response body length: %d != expect: %d",
                __LINE__, response.header.body_len,
                (int)(p - buffer->buff));
        return EINVAL;
    }

    array->count += count;
    return result;
}

int fcfs_auth_client_proto_gpool_list(FCFSAuthClientContext
        *client_ctx, ConnectionInfo *conn, const string_t *username,
        const string_t *poolname, const SFListLimitInfo *limit,
        struct fast_mpool_man *mpool, FCFSAuthGrantedPoolArray *array)
{
    return client_proto_list_wrapper((client_proto_list_func)
            client_proto_gpool_list_do, client_ctx, conn, username,
            poolname, limit, mpool, array, &array->count);
}

int fcfs_auth_client_get_master(FCFSAuthClientContext *client_ctx,
        FCFSAuthClientServerEntry *master)
{
    int result;
    ConnectionInfo *conn;
    FCFSAuthProtoHeader *header;
    SFResponseInfo response;
    FCFSAuthProtoGetServerResp server_resp;
    char out_buff[sizeof(FCFSAuthProtoHeader)];

    conn = client_ctx->cm.ops.get_connection(&client_ctx->cm, 0, &result);
    if (conn == NULL) {
        return result;
    }

    header = (FCFSAuthProtoHeader *)out_buff;
    SF_PROTO_SET_HEADER(header, FCFS_AUTH_SERVICE_PROTO_GET_MASTER_REQ,
            sizeof(out_buff) - sizeof(FCFSAuthProtoHeader));
    response.error.length = 0;
    if ((result=sf_send_and_recv_response(conn, out_buff, sizeof(out_buff),
                    &response, client_ctx->common_cfg.network_timeout,
                    FCFS_AUTH_SERVICE_PROTO_GET_MASTER_RESP,
                    (char *)&server_resp, sizeof(FCFSAuthProtoGetServerResp))) != 0)
    {
        sf_log_network_error(&response, conn, result);
    } else {
        master->server_id = buff2int(server_resp.server_id);
        memcpy(master->conn.ip_addr, server_resp.ip_addr, IP_ADDRESS_SIZE);
        *(master->conn.ip_addr + IP_ADDRESS_SIZE - 1) = '\0';
        master->conn.port = buff2short(server_resp.port);
    }

    SF_CLIENT_RELEASE_CONNECTION(&client_ctx->cm, conn, result);
    return result;
}

int fcfs_auth_client_cluster_stat(FCFSAuthClientContext *client_ctx,
        FCFSAuthClientClusterStatEntry *stats, const int size, int *count)
{
    FCFSAuthProtoHeader *header;
    FCFSAuthProtoClusterStatRespBodyHeader *body_header;
    FCFSAuthProtoClusterStatRespBodyPart *body_part;
    FCFSAuthProtoClusterStatRespBodyPart *body_end;
    FCFSAuthClientClusterStatEntry *stat;
    ConnectionInfo *conn;
    char out_buff[sizeof(FCFSAuthProtoHeader)];
    char fixed_buff[8 * 1024];
    char *in_buff;
    SFResponseInfo response;
    int result;
    int calc_size;

    if ((conn=client_ctx->cm.ops.get_master_connection(
                    &client_ctx->cm, 0, &result)) == NULL)
    {
        return result;
    }

    header = (FCFSAuthProtoHeader *)out_buff;
    SF_PROTO_SET_HEADER(header, FCFS_AUTH_SERVICE_PROTO_CLUSTER_STAT_REQ,
            sizeof(out_buff) - sizeof(FCFSAuthProtoHeader));

    response.error.length = 0;
    in_buff = fixed_buff;
    if ((result=sf_send_and_check_response_header(conn, out_buff,
                    sizeof(out_buff), &response, client_ctx->common_cfg.
                    network_timeout, FCFS_AUTH_SERVICE_PROTO_CLUSTER_STAT_RESP)) == 0)
    {
        if (response.header.body_len > sizeof(fixed_buff)) {
            in_buff = (char *)fc_malloc(response.header.body_len);
            if (in_buff == NULL) {
                response.error.length = sprintf(response.error.message,
                        "malloc %d bytes fail", response.header.body_len);
                result = ENOMEM;
            }
        }

        if (result == 0) {
            result = tcprecvdata_nb(conn->sock, in_buff,
                    response.header.body_len, client_ctx->
                    common_cfg.network_timeout);
        }
    }

    body_header = (FCFSAuthProtoClusterStatRespBodyHeader *)in_buff;
    body_part = (FCFSAuthProtoClusterStatRespBodyPart *)(in_buff +
            sizeof(FCFSAuthProtoClusterStatRespBodyHeader));
    if (result == 0) {
        *count = buff2int(body_header->count);

        calc_size = sizeof(FCFSAuthProtoClusterStatRespBodyHeader) +
            (*count) * sizeof(FCFSAuthProtoClusterStatRespBodyPart);
        if (calc_size != response.header.body_len) {
            response.error.length = sprintf(response.error.message,
                    "response body length: %d != calculate size: %d, "
                    "server count: %d", response.header.body_len,
                    calc_size, *count);
            result = EINVAL;
        } else if (size < *count) {
            response.error.length = sprintf(response.error.message,
                    "entry size %d too small < %d", size, *count);
            *count = 0;
            result = ENOSPC;
        }
    } else {
        *count = 0;
    }

    if (result != 0) {
        sf_log_network_error(&response, conn, result);
    } else {
        body_end = body_part + (*count);
        for (stat=stats; body_part<body_end; body_part++, stat++) {
            stat->is_online = body_part->is_online;
            stat->is_master = body_part->is_master;
            stat->server_id = buff2int(body_part->server_id);
            memcpy(stat->ip_addr, body_part->ip_addr, IP_ADDRESS_SIZE);
            *(stat->ip_addr + IP_ADDRESS_SIZE - 1) = '\0';
            stat->port = buff2short(body_part->port);
        }
    }

    SF_CLIENT_RELEASE_CONNECTION(&client_ctx->cm, conn, result);
    if (in_buff != fixed_buff) {
        if (in_buff != NULL) {
            free(in_buff);
        }
    }

    return result;
}
