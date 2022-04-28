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


#ifndef _FCFS_VOTE_CLIENT_PROTO_H
#define _FCFS_VOTE_CLIENT_PROTO_H

#include "fastcommon/fast_mpool.h"
#include "sf/sf_proto.h"
#include "client_types.h"

typedef struct fcfs_vote_client_cluster_stat_entry {
    int server_id;
    bool is_online;
    bool is_master;
    uint16_t port;
    char ip_addr[IP_ADDRESS_SIZE];
} FCFSVoteClientClusterStatEntry;

#ifdef __cplusplus
extern "C" {
#endif

int fcfs_vote_client_get_master(FCFSVoteClientContext *client_ctx,
        FCFSVoteClientServerEntry *master);

int fcfs_vote_client_cluster_stat(FCFSVoteClientContext *client_ctx,
        FCFSVoteClientClusterStatEntry *stats, const int size, int *count);

int fcfs_vote_client_proto_session_subscribe(
        FCFSVoteClientContext *client_ctx, ConnectionInfo *conn);

int fcfs_vote_client_proto_session_validate(
        FCFSVoteClientContext *client_ctx, ConnectionInfo *conn,
        const string_t *session_id, const string_t *validate_key,
        const FCFSVoteValidatePriviledgeType priv_type,
        const int64_t pool_id, const int64_t priv_required);

int fcfs_vote_client_proto_user_login(FCFSVoteClientContext *client_ctx,
        ConnectionInfo *conn, const string_t *username,
        const string_t *passwd, const string_t *poolname,
        const int flags);

int fcfs_vote_client_proto_user_create(FCFSVoteClientContext *client_ctx,
        ConnectionInfo *conn, const FCFSVoteUserInfo *user);

int fcfs_vote_client_proto_user_passwd(FCFSVoteClientContext *client_ctx,
        ConnectionInfo *conn, const string_t *username,
        const string_t *passwd);

int fcfs_vote_client_proto_user_list(FCFSVoteClientContext *client_ctx,
        ConnectionInfo *conn, const string_t *username,
        const SFListLimitInfo *limit, struct fast_mpool_man *mpool,
        FCFSVoteUserArray *array);

int fcfs_vote_client_proto_user_grant(FCFSVoteClientContext *client_ctx,
        ConnectionInfo *conn, const string_t *username, const int64_t priv);

int fcfs_vote_client_proto_user_remove(FCFSVoteClientContext *client_ctx,
        ConnectionInfo *conn, const string_t *username);

int fcfs_vote_client_proto_spool_create(FCFSVoteClientContext *client_ctx,
        ConnectionInfo *conn, FCFSVoteStoragePoolInfo *spool,
        const int name_size, const bool dryrun);

int fcfs_vote_client_proto_spool_list(FCFSVoteClientContext *client_ctx,
        ConnectionInfo *conn, const string_t *username,
        const string_t *poolname, const SFListLimitInfo *limit,
        struct fast_mpool_man *mpool, FCFSVoteStoragePoolArray *array);

int fcfs_vote_client_proto_spool_remove(FCFSVoteClientContext *client_ctx,
        ConnectionInfo *conn, const string_t *poolname);

int fcfs_vote_client_proto_spool_set_quota(FCFSVoteClientContext *client_ctx,
        ConnectionInfo *conn, const string_t *poolname, const int64_t quota);

int fcfs_vote_client_proto_spool_get_quota(FCFSVoteClientContext *client_ctx,
        ConnectionInfo *conn, const string_t *poolname, int64_t *quota);

int fcfs_vote_client_proto_gpool_grant(FCFSVoteClientContext *client_ctx,
        ConnectionInfo *conn, const string_t *username, const string_t
        *poolname, const FCFSVoteSPoolPriviledges *privs);

int fcfs_vote_client_proto_gpool_withdraw(FCFSVoteClientContext
        *client_ctx, ConnectionInfo *conn, const string_t *username,
        const string_t *poolname);

int fcfs_vote_client_proto_gpool_list(FCFSVoteClientContext
        *client_ctx, ConnectionInfo *conn, const string_t *username,
        const string_t *poolname, const SFListLimitInfo *limit,
        struct fast_mpool_man *mpool, FCFSVoteGrantedPoolArray *array);

#ifdef __cplusplus
}
#endif

#endif
