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
#include "fastcommon/connection_pool.h"
#include "sf/sf_proto.h"
#include "client_types.h"

typedef struct fcfs_vote_client_cluster_stat_ex_entry {
    int server_id;
    bool is_online;
    bool is_master;
    uint16_t port;
    char ip_addr[IP_ADDRESS_SIZE];
} FCFSVoteClientClusterStatEntry;

typedef struct fcfs_vote_client_join_request {
    int server_id;
    int group_id;
    short response_size;
    short service_id;
    bool is_leader;
} FCFSVoteClientJoinRequest;

#ifdef __cplusplus
extern "C" {
#endif

int vote_client_proto_get_master_connection_ex(FCFSVoteClientContext
        *client_ctx, ConnectionInfo *conn);

int vote_client_proto_join_ex(FCFSVoteClientContext *client_ctx,
        ConnectionInfo *conn, const FCFSVoteClientJoinRequest *r);

int vote_client_proto_get_vote_ex(FCFSVoteClientContext *client_ctx,
        ConnectionInfo *conn, const SFGetServerStatusRequest *r,
        char *in_buff, const int in_len);

int vote_client_proto_notify_next_leader_ex(FCFSVoteClientContext *client_ctx,
        ConnectionInfo *conn, const unsigned char req_cmd);

static inline int vote_client_proto_pre_set_next_leader_ex(
        FCFSVoteClientContext *client_ctx, ConnectionInfo *conn)
{
    return vote_client_proto_notify_next_leader_ex(client_ctx, conn,
            FCFS_VOTE_SERVICE_PROTO_PRE_SET_NEXT_LEADER);
}

static inline int vote_client_proto_commit_next_leader_ex(
        FCFSVoteClientContext *client_ctx, ConnectionInfo *conn)
{
    return vote_client_proto_notify_next_leader_ex(client_ctx, conn,
            FCFS_VOTE_SERVICE_PROTO_COMMIT_NEXT_LEADER);
}

int vote_client_proto_active_check_ex(FCFSVoteClientContext
        *client_ctx, ConnectionInfo *conn);

static inline void vote_client_proto_close_connection_ex(
        FCFSVoteClientContext *client_ctx, ConnectionInfo *conn)
{
    conn_pool_disconnect_server(conn);
}

static inline int fcfs_vote_client_join_ex(FCFSVoteClientContext *client_ctx,
        ConnectionInfo *conn, const FCFSVoteClientJoinRequest *r)
{
    int result;

    if ((result=vote_client_proto_get_master_connection_ex(
                    client_ctx, conn)) != 0)
    {
        return result;
    }

    if ((result=vote_client_proto_join_ex(client_ctx, conn, r)) != 0) {
        vote_client_proto_close_connection_ex(client_ctx, conn);
    }
    return result;
}

int fcfs_vote_client_get_vote_ex(FCFSVoteClientContext *client_ctx,
        const FCFSVoteClientJoinRequest *join_request,
        const unsigned char *servers_sign,
        const unsigned char *cluster_sign,
        char *in_buff, const int in_len);

int fcfs_vote_client_cluster_stat_ex(FCFSVoteClientContext *client_ctx,
        FCFSVoteClientClusterStatEntry *stats, const int size, int *count);


#define vote_client_proto_get_master_connection(conn) \
    vote_client_proto_get_master_connection_ex(&g_fcfs_vote_client_vars. \
            client_ctx, conn)

#define vote_client_proto_join(conn, r) \
    vote_client_proto_join_ex(&g_fcfs_vote_client_vars.client_ctx, conn, r)

#define vote_client_proto_get_vote(conn, r, in_buff, in_len) \
    vote_client_proto_get_vote_ex(&g_fcfs_vote_client_vars. \
            client_ctx, conn, r, in_buff, in_len)

#define vote_client_proto_notify_next_leader(conn, req_cmd) \
    vote_client_proto_notify_next_leader_ex(&g_fcfs_vote_client_vars. \
            client_ctx, conn, req_cmd)

#define vote_client_proto_pre_set_next_leader(conn) \
    vote_client_proto_pre_set_next_leader_ex(&g_fcfs_vote_client_vars. \
            client_ctx, conn)

#define vote_client_proto_commit_next_leader(conn) \
    vote_client_proto_commit_next_leader_ex(&g_fcfs_vote_client_vars. \
            client_ctx, conn)

#define vote_client_proto_active_check(conn) \
    vote_client_proto_active_check_ex(&g_fcfs_vote_client_vars. \
            client_ctx, conn)

#define vote_client_proto_close_connection(conn) \
    vote_client_proto_close_connection_ex(&g_fcfs_vote_client_vars. \
            client_ctx, conn)

#define fcfs_vote_client_join(conn, r) \
    fcfs_vote_client_join_ex(&g_fcfs_vote_client_vars.client_ctx, conn, r)

#define fcfs_vote_client_get_vote(join_request, servers_sign, \
        cluster_sign, in_buff, in_len) \
        fcfs_vote_client_get_vote_ex(&g_fcfs_vote_client_vars.client_ctx, \
                join_request, servers_sign, cluster_sign, in_buff, in_len)

#define fcfs_vote_client_cluster_stat(stats, size, count) \
    fcfs_vote_client_cluster_stat_ex(&g_fcfs_vote_client_vars. \
            client_ctx, stats, size, count)

#ifdef __cplusplus
}
#endif

#endif
