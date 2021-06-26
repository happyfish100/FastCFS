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

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <fcntl.h>
#include <pthread.h>
#include "fastcommon/logger.h"
#include "fastcommon/sockopt.h"
#include "fastcommon/shared_func.h"
#include "fastcommon/pthread_func.h"
#include "fastcommon/sched_thread.h"
#include "sf/sf_func.h"
#include "common/auth_proto.h"
#include "server_global.h"
#include "cluster_relationship.h"

FCFSAuthClusterServerInfo *g_next_master = NULL;

typedef struct fdir_cluster_server_status {
    FCFSAuthClusterServerInfo *cs;
    bool is_master;
    int server_id;
} FCFSAuthClusterServerStatus;

static int proto_get_server_status(ConnectionInfo *conn,
        FCFSAuthClusterServerStatus *server_status)
{
	int result;
	FCFSAuthProtoHeader *header;
    FCFSAuthProtoGetServerStatusReq *req;
    FCFSAuthProtoGetServerStatusResp *resp;
    SFResponseInfo response;
	char out_buff[sizeof(FCFSAuthProtoHeader) + sizeof(FCFSAuthProtoGetServerStatusReq)];
	char in_body[sizeof(FCFSAuthProtoGetServerStatusResp)];

    header = (FCFSAuthProtoHeader *)out_buff;
    SF_PROTO_SET_HEADER(header, FCFS_AUTH_CLUSTER_PROTO_GET_SERVER_STATUS_REQ,
            sizeof(out_buff) - sizeof(FCFSAuthProtoHeader));

    req = (FCFSAuthProtoGetServerStatusReq *)(out_buff + sizeof(FCFSAuthProtoHeader));
    int2buff(CLUSTER_MY_SERVER_ID, req->server_id);

    response.error.length = 0;
	if ((result=sf_send_and_check_response_header(conn, out_buff,
			sizeof(out_buff), &response, SF_G_NETWORK_TIMEOUT,
            FCFS_AUTH_CLUSTER_PROTO_GET_SERVER_STATUS_RESP)) != 0)
    {
        sf_log_network_error(&response, conn, result);
        return result;
    }

    if (response.header.body_len != sizeof(FCFSAuthProtoGetServerStatusResp)) {
        logError("file: "__FILE__", line: %d, "
                "server %s:%u, recv body length: %d != %d",
                __LINE__, conn->ip_addr, conn->port,
                response.header.body_len,
                (int)sizeof(FCFSAuthProtoGetServerStatusResp));
        return EINVAL;
    }

    if ((result=tcprecvdata_nb(conn->sock, in_body, response.header.body_len,
                    SF_G_NETWORK_TIMEOUT)) != 0)
    {
        logError("file: "__FILE__", line: %d, "
                "recv from server %s:%u fail, "
                "errno: %d, error info: %s",
                __LINE__, conn->ip_addr, conn->port,
                result, STRERROR(result));
        return result;
    }

    resp = (FCFSAuthProtoGetServerStatusResp *)in_body;

    server_status->is_master = resp->is_master;
    server_status->server_id = buff2int(resp->server_id);
    return 0;
}

static int proto_join_master(ConnectionInfo *conn)
{
	int result;
	FCFSAuthProtoHeader *header;
    FCFSAuthProtoJoinMasterReq *req;
    SFResponseInfo response;
	char out_buff[sizeof(FCFSAuthProtoHeader) + sizeof(FCFSAuthProtoJoinMasterReq)];

    header = (FCFSAuthProtoHeader *)out_buff;
    SF_PROTO_SET_HEADER(header, FCFS_AUTH_CLUSTER_PROTO_JOIN_MASTER,
            sizeof(out_buff) - sizeof(FCFSAuthProtoHeader));

    req = (FCFSAuthProtoJoinMasterReq *)(out_buff + sizeof(FCFSAuthProtoHeader));
    int2buff(CLUSTER_MY_SERVER_ID, req->server_id);
    memcpy(req->config_sign, CLUSTER_CONFIG_SIGN_BUF,
            SF_CLUSTER_CONFIG_SIGN_LEN);
    response.error.length = 0;
    if ((result=sf_send_and_recv_none_body_response(conn, out_buff,
                    sizeof(out_buff), &response, SF_G_NETWORK_TIMEOUT,
                    SF_PROTO_ACK)) != 0)
    {
        sf_log_network_error(&response, conn, result);
    }

    return result;
}

static int proto_ping_master(ConnectionInfo *conn)
{
    FCFSAuthProtoHeader header;
    SFResponseInfo response;
    char in_buff[8 * 1024];
    int result;

    SF_PROTO_SET_HEADER(&header, FCFS_AUTH_CLUSTER_PROTO_PING_MASTER_REQ, 0);

    //TODO
    response.error.length = 0;
    if ((result=sf_send_and_check_response_header(conn, (char *)&header,
                    sizeof(header), &response, SF_G_NETWORK_TIMEOUT,
                    FCFS_AUTH_CLUSTER_PROTO_PING_MASTER_RESP)) == 0)
    {
        if (response.header.body_len > sizeof(in_buff)) {
            response.error.length = sprintf(response.error.message,
                    "response body length: %d is too large",
                    response.header.body_len);
            result = EOVERFLOW;
        } else {
            result = tcprecvdata_nb(conn->sock, in_buff,
                    response.header.body_len, SF_G_NETWORK_TIMEOUT);
        }
    }

    if (result != 0) {
        sf_log_network_error(&response, conn, result);
        return result;
    }

    return 0;
}

static int cluster_cmp_server_status(const void *p1, const void *p2)
{
    FCFSAuthClusterServerStatus *status1;
    FCFSAuthClusterServerStatus *status2;
    int sub;

    status1 = (FCFSAuthClusterServerStatus *)p1;
    status2 = (FCFSAuthClusterServerStatus *)p2;
    sub = (int)status1->is_master - (int)status2->is_master;
    if (sub != 0) {
        return sub;
    }

    return (int)status1->server_id - (int)status2->server_id;
}

static int cluster_get_server_status(FCFSAuthClusterServerStatus *server_status)
{
    ConnectionInfo conn;
    int result;

    if (server_status->cs == CLUSTER_MYSELF_PTR) {
        server_status->is_master = MYSELF_IS_MASTER;
        server_status->server_id = CLUSTER_MY_SERVER_ID;
        return 0;
    } else {
        if ((result=fc_server_make_connection(&CLUSTER_GROUP_ADDRESS_ARRAY(
                            server_status->cs->server), &conn,
                        SF_G_CONNECT_TIMEOUT)) != 0)
        {
            return result;
        }

        result = proto_get_server_status(&conn, server_status);
        conn_pool_disconnect_server(&conn);
        return result;
    }
}

static int cluster_get_master(FCFSAuthClusterServerStatus *server_status,
        int *active_count)
{
#define STATUS_ARRAY_FIXED_COUNT  8
	FCFSAuthClusterServerInfo *server;
	FCFSAuthClusterServerInfo *end;
	FCFSAuthClusterServerStatus *current_status;
	FCFSAuthClusterServerStatus *cs_status;
	FCFSAuthClusterServerStatus status_array[STATUS_ARRAY_FIXED_COUNT];
	int result;
	int r;
	int i;

	memset(server_status, 0, sizeof(FCFSAuthClusterServerStatus));
    if (CLUSTER_SERVER_ARRAY.count < STATUS_ARRAY_FIXED_COUNT) {
        cs_status = status_array;
    } else {
        int bytes;
        bytes = sizeof(FCFSAuthClusterServerStatus) * CLUSTER_SERVER_ARRAY.count;
        cs_status = (FCFSAuthClusterServerStatus *)fc_malloc(bytes);
        if (cs_status == NULL) {
            return ENOMEM;
        }
    }

	current_status = cs_status;
	result = 0;
	end = CLUSTER_SERVER_ARRAY.servers + CLUSTER_SERVER_ARRAY.count;
	for (server=CLUSTER_SERVER_ARRAY.servers; server<end; server++) {
		current_status->cs = server;
        r = cluster_get_server_status(current_status);
		if (r == 0) {
			current_status++;
		} else if (r != ENOENT) {
			result = r;
		}
	}

	*active_count = current_status - cs_status;
    if (*active_count == 0) {
        logError("file: "__FILE__", line: %d, "
                "get server status fail, "
                "server count: %d", __LINE__,
                CLUSTER_SERVER_ARRAY.count);
        return result == 0 ? ENOENT : result;
    }

	qsort(cs_status, *active_count, sizeof(FCFSAuthClusterServerStatus),
		cluster_cmp_server_status);

	for (i=0; i<*active_count; i++) {
        logDebug("file: "__FILE__", line: %d, "
                "server_id: %d, ip addr %s:%u, is_master: %d",
                __LINE__, cs_status[i].server_id,
                CLUSTER_GROUP_ADDRESS_FIRST_IP(cs_status[i].cs->server),
                CLUSTER_GROUP_ADDRESS_FIRST_PORT(cs_status[i].cs->server),
                cs_status[i].is_master);
    }

	memcpy(server_status, cs_status + (*active_count - 1),
			sizeof(FCFSAuthClusterServerStatus));
    if (cs_status != status_array) {
        free(cs_status);
    }
	return 0;
}

static int do_notify_master_changed(FCFSAuthClusterServerInfo *cs,
		FCFSAuthClusterServerInfo *master, const unsigned char cmd,
        bool *bConnectFail)
{
    char out_buff[sizeof(FCFSAuthProtoHeader) + 4];
    ConnectionInfo conn;
    FCFSAuthProtoHeader *header;
    SFResponseInfo response;
    int result;

    if ((result=fc_server_make_connection(&CLUSTER_GROUP_ADDRESS_ARRAY(
                        cs->server), &conn, SF_G_CONNECT_TIMEOUT)) != 0)
    {
        *bConnectFail = true;
        return result;
    }
    *bConnectFail = false;

    header = (FCFSAuthProtoHeader *)out_buff;
    SF_PROTO_SET_HEADER(header, cmd, sizeof(out_buff) -
            sizeof(FCFSAuthProtoHeader));
    int2buff(master->server->id, out_buff + sizeof(FCFSAuthProtoHeader));
    response.error.length = 0;
    if ((result=sf_send_and_recv_none_body_response(&conn, out_buff,
                    sizeof(out_buff), &response, SF_G_NETWORK_TIMEOUT,
                    SF_PROTO_ACK)) != 0)
    {
        sf_log_network_error(&response, &conn, result);
    }

    conn_pool_disconnect_server(&conn);
    return result;
}

int cluster_relationship_pre_set_master(FCFSAuthClusterServerInfo *master)
{
    FCFSAuthClusterServerInfo *next_master;

    next_master = g_next_master;
    if (next_master == NULL) {
        g_next_master = master;
    } else if (next_master != master) {
        logError("file: "__FILE__", line: %d, "
                "try to set next master id: %d, "
                "but next master: %d already exist",
                __LINE__, master->server->id, next_master->server->id);
        g_next_master = NULL;
        return EEXIST;
    }

    return 0;
}

static inline void cluster_unset_master()
{
    FCFSAuthClusterServerInfo *old_master;

    old_master = CLUSTER_MASTER_ATOM_PTR;
    if (old_master != NULL) {
        __sync_bool_compare_and_swap(&CLUSTER_MASTER_PTR, old_master, NULL);
    }
}

static int cluster_relationship_set_master(FCFSAuthClusterServerInfo *new_master)
{
    FCFSAuthClusterServerInfo *old_master;

    old_master = CLUSTER_MASTER_ATOM_PTR;
    if (new_master == old_master) {
        logDebug("file: "__FILE__", line: %d, "
                "the server id: %d, ip %s:%u already is master",
                __LINE__, new_master->server->id,
                CLUSTER_GROUP_ADDRESS_FIRST_IP(new_master->server),
                CLUSTER_GROUP_ADDRESS_FIRST_PORT(new_master->server));
        return 0;
    }

    if (CLUSTER_MYSELF_PTR == new_master) {
        //TODO
    } else {
        logInfo("file: "__FILE__", line: %d, "
                "the master server id: %d, ip %s:%u",
                __LINE__, new_master->server->id,
                CLUSTER_GROUP_ADDRESS_FIRST_IP(new_master->server),
                CLUSTER_GROUP_ADDRESS_FIRST_PORT(new_master->server));
    }

    do {
        if (__sync_bool_compare_and_swap(&CLUSTER_MASTER_PTR,
                    old_master, new_master))
        {
            break;
        }
        old_master = CLUSTER_MASTER_ATOM_PTR;
    } while (old_master != new_master);

    return 0;
}

int cluster_relationship_commit_master(FCFSAuthClusterServerInfo *master)
{
    FCFSAuthClusterServerInfo *next_master;
    int result;

    next_master = g_next_master;
    if (next_master == NULL) {
        logError("file: "__FILE__", line: %d, "
                "next master is NULL", __LINE__);
        return EBUSY;
    }
    if (next_master != master) {
        logError("file: "__FILE__", line: %d, "
                "next master server id: %d != expected server id: %d",
                __LINE__, next_master->server->id, master->server->id);
        g_next_master = NULL;
        return EBUSY;
    }

    result = cluster_relationship_set_master(master);
    g_next_master = NULL;
    return result;
}

void cluster_relationship_trigger_reselect_master()
{
    cluster_unset_master();
}

static int cluster_notify_next_master(FCFSAuthClusterServerInfo *cs,
        FCFSAuthClusterServerStatus *server_status, bool *bConnectFail)
{
    FCFSAuthClusterServerInfo *master;
    master = server_status->cs;
    if (cs == CLUSTER_MYSELF_PTR) {
        return cluster_relationship_pre_set_master(master);
    } else {
        return do_notify_master_changed(cs, master,
                FCFS_AUTH_CLUSTER_PROTO_PRE_SET_NEXT_MASTER, bConnectFail);
    }
}

static int cluster_commit_next_master(FCFSAuthClusterServerInfo *cs,
        FCFSAuthClusterServerStatus *server_status, bool *bConnectFail)
{
    FCFSAuthClusterServerInfo *master;

    master = server_status->cs;
    if (cs == CLUSTER_MYSELF_PTR) {
        return cluster_relationship_commit_master(master);
    } else {
        return do_notify_master_changed(cs, master,
                FCFS_AUTH_CLUSTER_PROTO_COMMIT_NEXT_MASTER, bConnectFail);
    }
}

static int cluster_notify_master_changed(FCFSAuthClusterServerStatus *server_status)
{
	FCFSAuthClusterServerInfo *server;
	FCFSAuthClusterServerInfo *send;
	int result;
	bool bConnectFail;
	int success_count;

	result = ENOENT;
	send = CLUSTER_SERVER_ARRAY.servers + CLUSTER_SERVER_ARRAY.count;
	success_count = 0;
	for (server=CLUSTER_SERVER_ARRAY.servers; server<send; server++) {
		if ((result=cluster_notify_next_master(server,
				server_status, &bConnectFail)) != 0)
		{
			if (!bConnectFail) {
				return result;
			}
		} else {
			success_count++;
		}
	}

	if (success_count == 0) {
		return result;
	}

	result = ENOENT;
	success_count = 0;
	for (server=CLUSTER_SERVER_ARRAY.servers; server<send; server++) {
		if ((result=cluster_commit_next_master(server,
				server_status, &bConnectFail)) != 0)
		{
			if (!bConnectFail) {
				return result;
			}
		} else {
			success_count++;
		}
	}

	if (success_count == 0) {
		return result;
	}

	return 0;
}

static int cluster_select_master()
{
	int result;
    int active_count;
    int i;
    int sleep_secs;
	FCFSAuthClusterServerStatus server_status;
    FCFSAuthClusterServerInfo *next_master;

	logInfo("file: "__FILE__", line: %d, "
		"selecting master...", __LINE__);

    sleep_secs = 2;
    i = 0;
    while (1) {
        if ((result=cluster_get_master(&server_status, &active_count)) != 0) {
            return result;
        }
        if ((active_count == CLUSTER_SERVER_ARRAY.count) ||
                (active_count >= 2 && server_status.is_master))
        {
            break;
        }

        if (++i == 5) {
            break;
        }

        logInfo("file: "__FILE__", line: %d, "
                "round %dth select master, alive server count: %d "
                "< server count: %d, try again after %d seconds.",
                __LINE__, i, active_count, CLUSTER_SERVER_ARRAY.count,
                sleep_secs);
        sleep(sleep_secs);
        if (sleep_secs < 32) {
            sleep_secs *= 2;
        }
    }

    next_master = server_status.cs;
    if (CLUSTER_MYSELF_PTR == next_master) {
		if ((result=cluster_notify_master_changed(
                        &server_status)) != 0)
		{
			return result;
		}

		logInfo("file: "__FILE__", line: %d, "
			"I am the new master, id: %d, ip %s:%u",
			__LINE__, next_master->server->id,
            CLUSTER_GROUP_ADDRESS_FIRST_IP(next_master->server),
            CLUSTER_GROUP_ADDRESS_FIRST_PORT(next_master->server));
    } else {
        if (server_status.is_master) {
            cluster_relationship_set_master(next_master);
        } else if (CLUSTER_MASTER_ATOM_PTR == NULL) {
            logInfo("file: "__FILE__", line: %d, "
                    "waiting for the candidate master server id: %d, "
                    "ip %s:%u notify ...", __LINE__, next_master->server->id,
                    CLUSTER_GROUP_ADDRESS_FIRST_IP(next_master->server),
                    CLUSTER_GROUP_ADDRESS_FIRST_PORT(next_master->server));
            return ENOENT;
        }
    }

	return 0;
}

static int cluster_ping_master(ConnectionInfo *conn)
{
    int result;
    FCFSAuthClusterServerInfo *master;

    master = CLUSTER_MASTER_ATOM_PTR;
    if (CLUSTER_MYSELF_PTR == master) {
        return 0;  //do not need ping myself
    }

    if (master == NULL) {
        return ENOENT;
    }

    if (conn->sock < 0) {
        if ((result=fc_server_make_connection(&CLUSTER_GROUP_ADDRESS_ARRAY(
                            master->server), conn,
                        SF_G_CONNECT_TIMEOUT)) != 0)
        {
            return result;
        }

        if ((result=proto_join_master(conn)) != 0) {
            conn_pool_disconnect_server(conn);
            return result;
        }
    }

    if ((result=proto_ping_master(conn)) != 0) {
        conn_pool_disconnect_server(conn);
    }

    return result;
}

static void *cluster_thread_entrance(void* arg)
{
#define MAX_SLEEP_SECONDS  10

    int fail_count;
    int sleep_seconds;
    FCFSAuthClusterServerInfo *master;
    ConnectionInfo mconn;  //master connection

#ifdef OS_LINUX
    prctl(PR_SET_NAME, "relationship");
#endif

    memset(&mconn, 0, sizeof(mconn));
    mconn.sock = -1;

    fail_count = 0;
    sleep_seconds = 1;
    while (SF_G_CONTINUE_FLAG) {
        master = CLUSTER_MASTER_ATOM_PTR;
        if (master == NULL) {
            if (cluster_select_master() != 0) {
                sleep_seconds = 1 + (int)((double)rand()
                        * (double)MAX_SLEEP_SECONDS / RAND_MAX);
            } else {
                if (mconn.sock >= 0) {
                    conn_pool_disconnect_server(&mconn);
                }
                sleep_seconds = 1;
            }
        } else {
            if (cluster_ping_master(&mconn) == 0) {
                fail_count = 0;
                sleep_seconds = 1;
            } else {
                ++fail_count;
                logError("file: "__FILE__", line: %d, "
                        "%dth ping master id: %d, ip %s:%u fail",
                        __LINE__, fail_count, master->server->id,
                        CLUSTER_GROUP_ADDRESS_FIRST_IP(master->server),
                        CLUSTER_GROUP_ADDRESS_FIRST_PORT(master->server));

                sleep_seconds *= 2;
                if (fail_count >= 4) {
                    cluster_unset_master();

                    fail_count = 0;
                    sleep_seconds = 1;
                }
            }
        }

        sleep(sleep_seconds);
    }

    return NULL;
}

int cluster_relationship_init()
{
	pthread_t tid;

	return fc_create_thread(&tid, cluster_thread_entrance, NULL,
            SF_G_THREAD_STACK_SIZE);
}

int cluster_relationship_destroy()
{
	return 0;
}
