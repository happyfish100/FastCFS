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

#include "sf/idempotency/client/rpc_wrapper.h"
#include "fcfs_auth_client.h"

#define GET_CONNECTION(cm, arg1, result) \
    (cm)->ops.get_connection(cm, arg1, result)

int fcfs_auth_client_user_login(FCFSAuthClientContext *client_ctx,
        const string_t *username, const string_t *passwd)
{
    SF_CLIENT_IDEMPOTENCY_QUERY_WRAPPER(client_ctx, &client_ctx->cm,
            GET_CONNECTION, 0, fcfs_auth_client_proto_user_login,
            username, passwd);
}

int fcfs_auth_client_user_create(FCFSAuthClientContext *client_ctx,
        const FCFSAuthUserInfo *user)
{
    SF_CLIENT_IDEMPOTENCY_QUERY_WRAPPER(client_ctx, &client_ctx->cm,
            GET_CONNECTION, 0, fcfs_auth_client_proto_user_create, user);
}

int fcfs_auth_client_user_list(FCFSAuthClientContext *client_ctx,
        const string_t *username, SFProtoRecvBuffer *buffer,
        FCFSAuthUserArray *array)
{
    SF_CLIENT_IDEMPOTENCY_QUERY_WRAPPER(client_ctx, &client_ctx->cm,
            GET_CONNECTION, 0, fcfs_auth_client_proto_user_list,
            username, buffer, array);
}

int fcfs_auth_client_user_grant(FCFSAuthClientContext *client_ctx,
        const string_t *username, const int64_t priv)
{
    SF_CLIENT_IDEMPOTENCY_QUERY_WRAPPER(client_ctx, &client_ctx->cm,
            GET_CONNECTION, 0, fcfs_auth_client_proto_user_grant,
            username, priv);
}

int fcfs_auth_client_user_remove(FCFSAuthClientContext *client_ctx,
        const string_t *username)
{
    SF_CLIENT_IDEMPOTENCY_QUERY_WRAPPER(client_ctx, &client_ctx->cm,
            GET_CONNECTION, 0, fcfs_auth_client_proto_user_remove,
            username);
}

int fcfs_auth_client_spool_create(FCFSAuthClientContext *client_ctx,
        FCFSAuthStoragePoolInfo *spool, const int name_size,
        const bool dryrun)
{
    SF_CLIENT_IDEMPOTENCY_QUERY_WRAPPER(client_ctx, &client_ctx->cm,
            GET_CONNECTION, 0, fcfs_auth_client_proto_spool_create,
            spool, name_size, dryrun);
}

int fcfs_auth_client_spool_list(FCFSAuthClientContext *client_ctx,
        const string_t *username, const string_t *poolname,
        SFProtoRecvBuffer *buffer, FCFSAuthStoragePoolArray *array)
{
    SF_CLIENT_IDEMPOTENCY_QUERY_WRAPPER(client_ctx, &client_ctx->cm,
            GET_CONNECTION, 0, fcfs_auth_client_proto_spool_list,
            username, poolname, buffer, array);
}

int fcfs_auth_client_spool_remove(FCFSAuthClientContext *client_ctx,
        const string_t *poolname)
{
    SF_CLIENT_IDEMPOTENCY_QUERY_WRAPPER(client_ctx, &client_ctx->cm,
            GET_CONNECTION, 0, fcfs_auth_client_proto_spool_remove,
            poolname);
}

int fcfs_auth_client_spool_set_quota(FCFSAuthClientContext *client_ctx,
        const string_t *poolname, const int64_t quota)
{
    SF_CLIENT_IDEMPOTENCY_QUERY_WRAPPER(client_ctx, &client_ctx->cm,
            GET_CONNECTION, 0, fcfs_auth_client_proto_spool_set_quota,
            poolname, quota);
}

int fcfs_auth_client_gpool_grant(FCFSAuthClientContext *client_ctx,
        const string_t *username, const string_t *poolname,
        const FCFSAuthSPoolPriviledges *privs)
{
    SF_CLIENT_IDEMPOTENCY_QUERY_WRAPPER(client_ctx, &client_ctx->cm,
            GET_CONNECTION, 0, fcfs_auth_client_proto_gpool_grant,
            username, poolname, privs);
}

int fcfs_auth_client_gpool_withdraw(FCFSAuthClientContext *client_ctx,
        const string_t *username, const string_t *poolname)
{
    SF_CLIENT_IDEMPOTENCY_QUERY_WRAPPER(client_ctx, &client_ctx->cm,
            GET_CONNECTION, 0, fcfs_auth_client_proto_gpool_withdraw,
            username, poolname);
}

int fcfs_auth_client_gpool_list(FCFSAuthClientContext *client_ctx,
        const string_t *username, const string_t *poolname,
        SFProtoRecvBuffer *buffer, FCFSAuthGrantedPoolArray *array)
{
    SF_CLIENT_IDEMPOTENCY_QUERY_WRAPPER(client_ctx, &client_ctx->cm,
            GET_CONNECTION, 0, fcfs_auth_client_proto_gpool_list,
            username, poolname, buffer, array);
}
