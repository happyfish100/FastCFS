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


#ifndef _FCFS_AUTH_CLIENT_PROTO_H
#define _FCFS_AUTH_CLIENT_PROTO_H

#include "fastcommon/fast_mpool.h"
#include "sf/sf_proto.h"
#include "client_types.h"

typedef struct fcfs_auth_client_cluster_stat_entry {
    int server_id;
    bool is_online;
    bool is_master;
    uint16_t port;
    char ip_addr[IP_ADDRESS_SIZE];
} FCFSAuthClientClusterStatEntry;

#ifdef __cplusplus
extern "C" {
#endif

int fcfs_auth_client_get_master(FCFSAuthClientContext *client_ctx,
        FCFSAuthClientServerEntry *master);

int fcfs_auth_client_cluster_stat(FCFSAuthClientContext *client_ctx,
        FCFSAuthClientClusterStatEntry *stats, const int size, int *count);

int fcfs_auth_client_proto_session_subscribe(
        FCFSAuthClientContext *client_ctx, ConnectionInfo *conn);

int fcfs_auth_client_proto_session_validate(
        FCFSAuthClientContext *client_ctx, ConnectionInfo *conn,
        const string_t *session_id, const string_t *validate_key,
        const FCFSAuthValidatePriviledgeType priv_type,
        const int64_t pool_id, const int64_t priv_required);

int fcfs_auth_client_proto_user_login(FCFSAuthClientContext *client_ctx,
        ConnectionInfo *conn, const string_t *username,
        const string_t *passwd, const string_t *poolname,
        const int flags);

int fcfs_auth_client_proto_user_create(FCFSAuthClientContext *client_ctx,
        ConnectionInfo *conn, const FCFSAuthUserInfo *user);

int fcfs_auth_client_proto_user_passwd(FCFSAuthClientContext *client_ctx,
        ConnectionInfo *conn, const string_t *username,
        const string_t *passwd);

int fcfs_auth_client_proto_user_list(FCFSAuthClientContext *client_ctx,
        ConnectionInfo *conn, const string_t *username,
        const SFListLimitInfo *limit, struct fast_mpool_man *mpool,
        FCFSAuthUserArray *array);

int fcfs_auth_client_proto_user_grant(FCFSAuthClientContext *client_ctx,
        ConnectionInfo *conn, const string_t *username, const int64_t priv);

int fcfs_auth_client_proto_user_remove(FCFSAuthClientContext *client_ctx,
        ConnectionInfo *conn, const string_t *username);

int fcfs_auth_client_proto_spool_create(FCFSAuthClientContext *client_ctx,
        ConnectionInfo *conn, FCFSAuthStoragePoolInfo *spool,
        const int name_size, const bool dryrun);

int fcfs_auth_client_proto_spool_list(FCFSAuthClientContext *client_ctx,
        ConnectionInfo *conn, const string_t *username,
        const string_t *poolname, const SFListLimitInfo *limit,
        struct fast_mpool_man *mpool, FCFSAuthStoragePoolArray *array);

int fcfs_auth_client_proto_spool_remove(FCFSAuthClientContext *client_ctx,
        ConnectionInfo *conn, const string_t *poolname);

int fcfs_auth_client_proto_spool_set_quota(FCFSAuthClientContext *client_ctx,
        ConnectionInfo *conn, const string_t *poolname, const int64_t quota);

int fcfs_auth_client_proto_spool_get_quota(FCFSAuthClientContext *client_ctx,
        ConnectionInfo *conn, const string_t *poolname, int64_t *quota);

int fcfs_auth_client_proto_gpool_grant(FCFSAuthClientContext *client_ctx,
        ConnectionInfo *conn, const string_t *username, const string_t
        *poolname, const FCFSAuthSPoolPriviledges *privs);

int fcfs_auth_client_proto_gpool_withdraw(FCFSAuthClientContext
        *client_ctx, ConnectionInfo *conn, const string_t *username,
        const string_t *poolname);

int fcfs_auth_client_proto_gpool_list(FCFSAuthClientContext
        *client_ctx, ConnectionInfo *conn, const string_t *username,
        const string_t *poolname, const SFListLimitInfo *limit,
        struct fast_mpool_man *mpool, FCFSAuthGrantedPoolArray *array);

#ifdef __cplusplus
}
#endif

#endif
