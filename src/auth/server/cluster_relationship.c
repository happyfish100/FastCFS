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
#include "sf/sf_service.h"
#include "sf/sf_func.h"
#include "common/auth_proto.h"
#include "db/auth_db.h"
#include "db/pool_usage_updater.h"
#include "server_global.h"
#include "cluster_relationship.h"

typedef struct fcfs_auth_cluster_server_status {
    FCFSAuthClusterServerInfo *cs;
    bool is_master;
    int server_id;
} FCFSAuthClusterServerStatus;

typedef struct fcfs_auth_cluster_server_detect_entry {
    FCFSAuthClusterServerInfo *cs;
    int next_time;
} FCFSAuthClusterServerDetectEntry;

typedef struct fcfs_auth_cluster_server_detect_array {
    FCFSAuthClusterServerDetectEntry *entries;
    int count;
    int alloc;
    pthread_mutex_t lock;
} FCFSAuthClusterServerDetectArray;

typedef struct fcfs_auth_cluster_relationship_context {
    FCFSAuthClusterServerInfo *next_master;
    FCFSAuthClusterServerDetectArray inactive_server_array;
} FCFSAuthClusterRelationshipContext;

#define INACTIVE_SERVER_ARRAY relationship_ctx.inactive_server_array

static FCFSAuthClusterRelationshipContext relationship_ctx = {
    NULL, {NULL, 0, 0}
};

#define SET_SERVER_DETECT_ENTRY(entry, server) \
    do {  \
        entry->cs = server;    \
        entry->next_time = g_current_time + 1; \
    } while (0)

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
    memcpy(req->config_sign, CLUSTER_CONFIG_SIGN_BUF,
            SF_CLUSTER_CONFIG_SIGN_LEN);

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

static void init_inactive_server_array()
{
    FCFSAuthClusterServerInfo *cs;
    FCFSAuthClusterServerInfo *end;
    FCFSAuthClusterServerDetectEntry *entry;

    PTHREAD_MUTEX_LOCK(&INACTIVE_SERVER_ARRAY.lock);
    entry = INACTIVE_SERVER_ARRAY.entries;
    end = CLUSTER_SERVER_ARRAY.servers + CLUSTER_SERVER_ARRAY.count;
    for (cs=CLUSTER_SERVER_ARRAY.servers; cs<end; cs++) {
        if (cs != CLUSTER_MYSELF_PTR) {
            SET_SERVER_DETECT_ENTRY(entry, cs);
            entry++;
        }
    }

    INACTIVE_SERVER_ARRAY.count = entry - INACTIVE_SERVER_ARRAY.entries;
    PTHREAD_MUTEX_UNLOCK(&INACTIVE_SERVER_ARRAY.lock);
}

static inline void cluster_unset_master()
{
    FCFSAuthClusterServerInfo *old_master;

    old_master = CLUSTER_MASTER_ATOM_PTR;
    if (old_master != NULL) {
        __sync_bool_compare_and_swap(&CLUSTER_MASTER_PTR, old_master, NULL);
        if (old_master == CLUSTER_MYSELF_PTR) {
            auth_db_destroy();
        }
    }
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
    int result;

    SF_PROTO_SET_HEADER(&header, FCFS_AUTH_CLUSTER_PROTO_PING_MASTER_REQ, 0);
    response.error.length = 0;
    if ((result=sf_send_and_recv_none_body_response(conn, (char *)&header,
                    sizeof(header), &response, SF_G_NETWORK_TIMEOUT,
                    FCFS_AUTH_CLUSTER_PROTO_PING_MASTER_RESP)) != 0)
    {
        sf_log_network_error(&response, conn, result);
    }

    return result;
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

#define cluster_get_server_status(server_status) \
    cluster_get_server_status_ex(server_status, true)

static int cluster_get_server_status_ex(FCFSAuthClusterServerStatus
        *server_status, const bool log_connect_error)
{
    ConnectionInfo conn;
    int result;

    if (server_status->cs == CLUSTER_MYSELF_PTR) {
        server_status->is_master = MYSELF_IS_MASTER;
        server_status->server_id = CLUSTER_MY_SERVER_ID;
        return 0;
    } else {
        if ((result=fc_server_make_connection_ex(&CLUSTER_GROUP_ADDRESS_ARRAY(
                            server_status->cs->server), &conn,
                        SF_G_CONNECT_TIMEOUT, NULL, log_connect_error)) != 0)
        {
            return result;
        }

        result = proto_get_server_status(&conn, server_status);
        conn_pool_disconnect_server(&conn);
        return result;
    }
}

static int do_check_brainsplit(FCFSAuthClusterServerInfo *cs)
{
    int result;
    const bool log_connect_error = false;
    FCFSAuthClusterServerStatus server_status;

    server_status.cs = cs;
    if ((result=cluster_get_server_status_ex(&server_status,
                    log_connect_error)) != 0)
    {
        return result;
    }

    if (server_status.is_master) {
        logWarning("file: "__FILE__", line: %d, "
                "two masters occurs, anonther master id: %d, ip %s:%u, "
                "trigger re-select master ...", __LINE__, cs->server->id,
                CLUSTER_GROUP_ADDRESS_FIRST_IP(cs->server),
                CLUSTER_GROUP_ADDRESS_FIRST_PORT(cs->server));
        cluster_unset_master();
        return EEXIST;
    }

    return 0;
}

static int cluster_check_brainsplit(const int inactive_count)
{
    FCFSAuthClusterServerDetectEntry *entry;
    FCFSAuthClusterServerDetectEntry *end;
    int result;

    end = INACTIVE_SERVER_ARRAY.entries + inactive_count;
    for (entry=INACTIVE_SERVER_ARRAY.entries; entry<end; entry++) {
        if (entry >= INACTIVE_SERVER_ARRAY.entries +
                INACTIVE_SERVER_ARRAY.count)
        {
            break;
        }
        if (entry->next_time > g_current_time) {
            continue;
        }

        result = do_check_brainsplit(entry->cs);
        if (result == EEXIST) {  //brain-split occurs
            return result;
        }

        entry->next_time = g_current_time + 1;
    }

    return 0;
}

static int master_check()
{
    int result;
    int inactive_count;

    PTHREAD_MUTEX_LOCK(&INACTIVE_SERVER_ARRAY.lock);
    inactive_count = INACTIVE_SERVER_ARRAY.count;
    PTHREAD_MUTEX_UNLOCK(&INACTIVE_SERVER_ARRAY.lock);
    if (inactive_count > 0) {
        if ((result=cluster_check_brainsplit(inactive_count)) != 0) {
            return result;
        }
    }

    return 0;
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

    next_master = relationship_ctx.next_master;
    if (next_master == NULL) {
        relationship_ctx.next_master = master;
    } else if (next_master != master) {
        logError("file: "__FILE__", line: %d, "
                "try to set next master id: %d, "
                "but next master: %d already exist",
                __LINE__, master->server->id, next_master->server->id);
        relationship_ctx.next_master = NULL;
        return EEXIST;
    }

    return 0;
}

static int load_data()
{
    int result;

    if ((result=adb_load_data((AuthServerContext *)
                    sf_get_random_thread_data()->arg)) != 0)
    {
        return result;
    }
    if ((result=adb_check_generate_admin_user((AuthServerContext *)
                    sf_get_random_thread_data()->arg)) != 0)
    {
        return result;
    }

    logInfo("load data done.");

    return 0;
}

static int cluster_relationship_set_master(FCFSAuthClusterServerInfo *new_master)
{
    int result;
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
        if ((result=load_data()) != 0) {
            sf_terminate_myself();
            return result;
        }
        init_inactive_server_array();
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

    if (CLUSTER_MYSELF_PTR == new_master) {
        if ((result=pool_usage_updater_start()) != 0) {
            sf_terminate_myself();
            return result;
        }
        logInfo("pool_usage_updater_start.");
    }

    return 0;
}

int cluster_relationship_commit_master(FCFSAuthClusterServerInfo *master)
{
    FCFSAuthClusterServerInfo *next_master;
    int result;

    next_master = relationship_ctx.next_master;
    if (next_master == NULL) {
        logError("file: "__FILE__", line: %d, "
                "next master is NULL", __LINE__);
        return EBUSY;
    }
    if (next_master != master) {
        logError("file: "__FILE__", line: %d, "
                "next master server id: %d != expected server id: %d",
                __LINE__, next_master->server->id, master->server->id);
        relationship_ctx.next_master = NULL;
        return EBUSY;
    }

    result = cluster_relationship_set_master(master);
    relationship_ctx.next_master = NULL;
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

static int cluster_ping_master(FCFSAuthClusterServerInfo *master,
        ConnectionInfo *conn, bool *is_ping)
{
    int result;

    if (CLUSTER_MYSELF_PTR == master) {
        *is_ping = false;
        return master_check();
    }

    *is_ping = true;
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
    bool is_ping;
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
            if (cluster_ping_master(master, &mconn, &is_ping) == 0) {
                fail_count = 0;
                sleep_seconds = 1;
            } else if (is_ping) {
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
            } else {
                sleep_seconds = 1;
            }
        }

        sleep(sleep_seconds);
    }

    return NULL;
}

int cluster_relationship_init()
{
    int result;
    int bytes;
    pthread_t tid;

    bytes = sizeof(FCFSAuthClusterServerDetectEntry) * CLUSTER_SERVER_ARRAY.count;
    INACTIVE_SERVER_ARRAY.entries = (FCFSAuthClusterServerDetectEntry *)
        fc_malloc(bytes);
    if (INACTIVE_SERVER_ARRAY.entries == NULL) {
        return ENOMEM;
    }
    if ((result=init_pthread_lock(&INACTIVE_SERVER_ARRAY.lock)) != 0) {
        logError("file: "__FILE__", line: %d, "
                "init_pthread_lock fail, errno: %d, error info: %s",
                __LINE__, result, STRERROR(result));
        return result;
    }
    INACTIVE_SERVER_ARRAY.alloc = CLUSTER_SERVER_ARRAY.count;

    return fc_create_thread(&tid, cluster_thread_entrance, NULL,
            SF_G_THREAD_STACK_SIZE);
}

int cluster_relationship_destroy()
{
	return 0;
}

void cluster_relationship_add_to_inactive_sarray(FCFSAuthClusterServerInfo *cs)
{
    FCFSAuthClusterServerDetectEntry *entry;
    FCFSAuthClusterServerDetectEntry *end;
    bool found;

    found = false;
    PTHREAD_MUTEX_LOCK(&INACTIVE_SERVER_ARRAY.lock);
    if (INACTIVE_SERVER_ARRAY.count > 0) {
        end = INACTIVE_SERVER_ARRAY.entries + INACTIVE_SERVER_ARRAY.count;
        for (entry=INACTIVE_SERVER_ARRAY.entries; entry<end; entry++) {
            if (entry->cs == cs) {
                found = true;
                break;
            }
        }
    }
    if (!found) {
        if (INACTIVE_SERVER_ARRAY.count < INACTIVE_SERVER_ARRAY.alloc) {
            entry = INACTIVE_SERVER_ARRAY.entries + INACTIVE_SERVER_ARRAY.count;
            SET_SERVER_DETECT_ENTRY(entry, cs);
            INACTIVE_SERVER_ARRAY.count++;
        } else {
            logError("file: "__FILE__", line: %d, "
                    "server id: %d, add to inactive array fail "
                    "because array is full", __LINE__, cs->server->id);
        }
    } else {
        logWarning("file: "__FILE__", line: %d, "
                "server id: %d, already in inactive array!",
                __LINE__, cs->server->id);
    }
    PTHREAD_MUTEX_UNLOCK(&INACTIVE_SERVER_ARRAY.lock);
}

void cluster_relationship_remove_from_inactive_sarray(FCFSAuthClusterServerInfo *cs)
{
    FCFSAuthClusterServerDetectEntry *entry;
    FCFSAuthClusterServerDetectEntry *p;
    FCFSAuthClusterServerDetectEntry *end;

    PTHREAD_MUTEX_LOCK(&INACTIVE_SERVER_ARRAY.lock);
    end = INACTIVE_SERVER_ARRAY.entries + INACTIVE_SERVER_ARRAY.count;
    for (entry=INACTIVE_SERVER_ARRAY.entries; entry<end; entry++) {
        if (entry->cs == cs) {
            break;
        }
    }

    if (entry < end) {
        for (p=entry+1; p<end; p++) {
            *(p - 1) = *p;
        }
        INACTIVE_SERVER_ARRAY.count--;
    }
    PTHREAD_MUTEX_UNLOCK(&INACTIVE_SERVER_ARRAY.lock);
}
