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
#include "client_global.h"
#include "client_proto.h"

static inline int get_spec_connection(FCFSVoteClientContext *client_ctx,
        const ConnectionInfo *target, ConnectionInfo *conn)
{
    memcpy(conn, target, sizeof(ConnectionInfo));
    conn->sock = -1;
    return conn_pool_connect_server(conn, client_ctx->connect_timeout);
}

static inline int make_connection(FCFSVoteClientContext *client_ctx,
        FCServerInfo *server, ConnectionInfo *conn)
{
    return fc_server_make_connection(&server->group_addrs[client_ctx->
            cluster.service_group_index].address_array, conn,
            client_ctx->connect_timeout);
}

static int get_connection(FCFSVoteClientContext *client_ctx,
        ConnectionInfo *conn)
{
    int index;
    int i;
    int result;
    FCServerInfo *server;

    index = rand() % FC_SID_SERVER_COUNT(client_ctx->cluster.server_cfg);
    server = FC_SID_SERVERS(client_ctx->cluster.server_cfg) + index;
    if ((result=make_connection(client_ctx, server, conn)) == 0) {
        return 0;
    }

    i = (index + 1) % FC_SID_SERVER_COUNT(client_ctx->cluster.server_cfg);
    while (i != index) {
        server = FC_SID_SERVERS(client_ctx->cluster.server_cfg) + i;
        if ((result=make_connection(client_ctx, server, conn)) == 0) {
            return 0;
        }

        i = (i + 1) % FC_SID_SERVER_COUNT(client_ctx->cluster.server_cfg);
    }

    logError("file: "__FILE__", line: %d, "
            "get_connection fail, configured server count: %d",
            __LINE__, FC_SID_SERVER_COUNT(client_ctx->cluster.server_cfg));
    return result;
}

int vote_client_proto_get_master_connection_ex(FCFSVoteClientContext
        *client_ctx, ConnectionInfo *conn)
{
    int result;
    FCFSVoteProtoHeader *header;
    FCFSVoteClientServerEntry master;
    SFResponseInfo response;
    FCFSVoteProtoGetServerResp server_resp;
    char out_buff[sizeof(FCFSVoteProtoHeader)];

    if ((result=get_connection(client_ctx, conn)) != 0) {
        return result;
    }

    header = (FCFSVoteProtoHeader *)out_buff;
    SF_PROTO_SET_HEADER(header, FCFS_VOTE_SERVICE_PROTO_GET_MASTER_REQ,
            sizeof(out_buff) - sizeof(FCFSVoteProtoHeader));
    response.error.length = 0;
    if ((result=sf_send_and_recv_response(conn, out_buff, sizeof(out_buff),
                    &response, client_ctx->network_timeout,
                    FCFS_VOTE_SERVICE_PROTO_GET_MASTER_RESP, (char *)
                    &server_resp, sizeof(FCFSVoteProtoGetServerResp))) != 0)
    {
        sf_log_network_error(&response, conn, result);
        conn_pool_disconnect_server(conn);
        return result;
    }

    master.server_id = buff2int(server_resp.server_id);
    memcpy(master.conn.ip_addr, server_resp.ip_addr, IP_ADDRESS_SIZE);
    *(master.conn.ip_addr + IP_ADDRESS_SIZE - 1) = '\0';
    master.conn.port = buff2short(server_resp.port);
    if (FC_CONNECTION_SERVER_EQUAL1(*conn, master.conn)) {
        return 0;
    }

    conn_pool_disconnect_server(conn);
    return get_spec_connection(client_ctx, &master.conn, conn);
}

int fcfs_vote_client_cluster_stat_ex(FCFSVoteClientContext *client_ctx,
        FCFSVoteClientClusterStatEntry *stats, const int size, int *count)
{
    FCFSVoteProtoHeader *header;
    FCFSVoteProtoClusterStatRespBodyHeader *body_header;
    FCFSVoteProtoClusterStatRespBodyPart *body_part;
    FCFSVoteProtoClusterStatRespBodyPart *body_end;
    FCFSVoteClientClusterStatEntry *stat;
    ConnectionInfo conn;
    char out_buff[sizeof(FCFSVoteProtoHeader)];
    char fixed_buff[8 * 1024];
    char *in_buff;
    SFResponseInfo response;
    int result;
    int calc_size;

    if ((result=vote_client_proto_get_master_connection_ex(
                    client_ctx, &conn)) != 0)
    {
        return result;
    }

    header = (FCFSVoteProtoHeader *)out_buff;
    SF_PROTO_SET_HEADER(header, FCFS_VOTE_SERVICE_PROTO_CLUSTER_STAT_REQ,
            sizeof(out_buff) - sizeof(FCFSVoteProtoHeader));

    response.error.length = 0;
    in_buff = fixed_buff;
    if ((result=sf_send_and_check_response_header(&conn,
                    out_buff, sizeof(out_buff), &response,
                    client_ctx->network_timeout,
                    FCFS_VOTE_SERVICE_PROTO_CLUSTER_STAT_RESP)) == 0)
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
            result = tcprecvdata_nb(conn.sock, in_buff, response.
                    header.body_len, client_ctx->network_timeout);
        }
    }

    body_header = (FCFSVoteProtoClusterStatRespBodyHeader *)in_buff;
    body_part = (FCFSVoteProtoClusterStatRespBodyPart *)(in_buff +
            sizeof(FCFSVoteProtoClusterStatRespBodyHeader));
    if (result == 0) {
        *count = buff2int(body_header->count);

        calc_size = sizeof(FCFSVoteProtoClusterStatRespBodyHeader) +
            (*count) * sizeof(FCFSVoteProtoClusterStatRespBodyPart);
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
        sf_log_network_error(&response, &conn, result);
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

    vote_client_proto_close_connection_ex(client_ctx, &conn);
    if (in_buff != fixed_buff) {
        if (in_buff != NULL) {
            free(in_buff);
        }
    }

    return result;
}

int vote_client_proto_join_ex(FCFSVoteClientContext *client_ctx,
        ConnectionInfo *conn, const FCFSVoteClientJoinRequest *r)
{
    int result;
    FCFSVoteProtoHeader *header;
    FCFSVoteProtoClientJoinReq *req;
    SFResponseInfo response;
    char out_buff[sizeof(FCFSVoteProtoHeader) +
        sizeof(FCFSVoteProtoClientJoinReq)];

    header = (FCFSVoteProtoHeader *)out_buff;
    SF_PROTO_SET_HEADER(header, FCFS_VOTE_SERVICE_PROTO_CLIENT_JOIN_REQ,
            sizeof(out_buff) - sizeof(FCFSVoteProtoHeader));
    req = (FCFSVoteProtoClientJoinReq *)(header + 1);
    int2buff(r->server_id, req->server_id);
    int2buff(r->group_id, req->group_id);
    short2buff(r->response_size, req->response_size);
    req->service_id = r->service_id;
    req->is_leader = (r->is_leader ? 1 : 0);

    response.error.length = 0;
    if ((result=sf_send_and_recv_none_body_response(conn,
                    out_buff, sizeof(out_buff), &response,
                    client_ctx->network_timeout,
                    FCFS_VOTE_SERVICE_PROTO_CLIENT_JOIN_RESP)) != 0)
    {
        sf_log_network_error(&response, conn, result);
    }
    return result;
}

int vote_client_proto_get_vote_ex(FCFSVoteClientContext *client_ctx,
        ConnectionInfo *conn, const SFGetServerStatusRequest *r,
        char *in_buff, const int in_len)
{
    int result;
    FCFSVoteProtoHeader *header;
    SFProtoGetServerStatusReq *req;
    SFResponseInfo response;
    char out_buff[sizeof(FCFSVoteProtoHeader) +
        sizeof(SFProtoGetServerStatusReq)];

    header = (FCFSVoteProtoHeader *)out_buff;
    SF_PROTO_SET_HEADER(header, FCFS_VOTE_SERVICE_PROTO_GET_VOTE_REQ,
            sizeof(out_buff) - sizeof(FCFSVoteProtoHeader));
    req = (SFProtoGetServerStatusReq *)(header + 1);
    sf_proto_get_server_status_pack(r, req);

    response.error.length = 0;
    if ((result=sf_send_and_recv_response(conn, out_buff, sizeof(out_buff),
                    &response, client_ctx->network_timeout,
                    FCFS_VOTE_SERVICE_PROTO_GET_VOTE_RESP, in_buff,
                    in_len)) != 0)
    {
        sf_log_network_error(&response, conn, result);
    }
    return result;
}

int vote_client_proto_notify_next_leader_ex(FCFSVoteClientContext *client_ctx,
        ConnectionInfo *conn, const unsigned char req_cmd)
{
    int result;
    FCFSVoteProtoHeader *header;
    SFResponseInfo response;
    char out_buff[sizeof(FCFSVoteProtoHeader)];

    header = (FCFSVoteProtoHeader *)out_buff;
    SF_PROTO_SET_HEADER(header, req_cmd, sizeof(out_buff) -
            sizeof(FCFSVoteProtoHeader));
    response.error.length = 0;
    if ((result=sf_send_and_recv_none_body_response(conn, out_buff,
                    sizeof(out_buff), &response, client_ctx->
                    network_timeout, SF_PROTO_ACK)) != 0)
    {
        sf_log_network_error(&response, conn, result);
    }
    return result;
}

int vote_client_proto_active_check_ex(FCFSVoteClientContext
        *client_ctx, ConnectionInfo *conn)
{
    int result;
    FCFSVoteProtoHeader *header;
    SFResponseInfo response;
    char out_buff[sizeof(FCFSVoteProtoHeader)];

    header = (FCFSVoteProtoHeader *)out_buff;
    SF_PROTO_SET_HEADER(header, FCFS_VOTE_SERVICE_PROTO_ACTIVE_CHECK_REQ,
            sizeof(out_buff) - sizeof(FCFSVoteProtoHeader));
    response.error.length = 0;
    if ((result=sf_send_and_recv_none_body_response(conn,
                    out_buff, sizeof(out_buff), &response,
                    client_ctx->network_timeout,
                    FCFS_VOTE_SERVICE_PROTO_ACTIVE_CHECK_RESP)) != 0)
    {
        sf_log_network_error(&response, conn, result);
    }
    return result;
}

int fcfs_vote_client_get_vote_ex(FCFSVoteClientContext *client_ctx,
        const FCFSVoteClientJoinRequest *join_request,
        const unsigned char *servers_sign,
        const unsigned char *cluster_sign,
        char *in_buff, const int in_len)
{
    int result;
    ConnectionInfo conn;
    SFGetServerStatusRequest status_request;

    if ((result=fcfs_vote_client_join_ex(client_ctx,
                    &conn, join_request)) == 0)
    {
        status_request.servers_sign = servers_sign;
        status_request.cluster_sign = cluster_sign;
        status_request.server_id = join_request->server_id;
        status_request.is_leader = join_request->is_leader;
        result = vote_client_proto_get_vote_ex(client_ctx,
                &conn, &status_request, in_buff, in_len);
        vote_client_proto_close_connection_ex(client_ctx, &conn);
    }

    return result;
}
