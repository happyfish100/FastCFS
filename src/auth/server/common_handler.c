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

//common_handler.c

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <limits.h>
#include <fcntl.h>
#include "fastcommon/logger.h"
#include "fastcommon/sockopt.h"
#include "fastcommon/shared_func.h"
#include "fastcommon/sched_thread.h"
#include "common/auth_proto.h"
#include "server_global.h"
#include "common_handler.h"

#define LOG_LEVEL_FOR_DEBUG  LOG_DEBUG

static int fcfs_auth_get_cmd_log_level(const int cmd)
{
    switch (cmd) {
        case SF_PROTO_ACTIVE_TEST_REQ:
        case SF_PROTO_ACTIVE_TEST_RESP:
            return LOG_NOTHING;
        case SF_SERVICE_PROTO_REPORT_REQ_RECEIPT_REQ:
            return LOG_DEBUG;
        default:
            return LOG_LEVEL_FOR_DEBUG;
    }
}

void common_handler_init()
{
    SFHandlerContext handler_ctx;

    fcfs_auth_proto_init();

    handler_ctx.slow_log = &SLOW_LOG;
    handler_ctx.callbacks.get_cmd_caption = fcfs_auth_get_cmd_caption;
    if (FC_LOG_BY_LEVEL(LOG_LEVEL_FOR_DEBUG)) {
        handler_ctx.callbacks.get_cmd_log_level = fcfs_auth_get_cmd_log_level;
    } else {
        handler_ctx.callbacks.get_cmd_log_level = NULL;
    }
    sf_proto_set_handler_context(&handler_ctx);
}

int fcfs_auth_deal_get_master(struct fast_task_info *task,
        const int group_index)
{
    int result;
    FCFSAuthProtoGetServerResp *resp;
    FCFSAuthClusterServerInfo *master;
    const FCAddressInfo *addr;

    if ((result=server_expect_body_length(0)) != 0) {
        return result;
    }

    master = (CLUSTER_MASTER_ATOM_PTR != NULL) ?
        CLUSTER_MASTER_ATOM_PTR : CLUSTER_NEXT_MASTER;
    if (master == NULL) {
        RESPONSE.error.length = sprintf(
                RESPONSE.error.message,
                "the master NOT exist");
        return SF_RETRIABLE_ERROR_NO_SERVER;
    }

    resp = (FCFSAuthProtoGetServerResp *)SF_PROTO_RESP_BODY(task);
    if (group_index == SERVICE_GROUP_INDEX) {
        addr = fc_server_get_address_by_peer(&SERVICE_GROUP_ADDRESS_ARRAY(
                    master->server), task->client_ip);
    } else {
        addr = fc_server_get_address_by_peer(&CLUSTER_GROUP_ADDRESS_ARRAY(
                    master->server), task->client_ip);
    }

    int2buff(master->server->id, resp->server_id);
    snprintf(resp->ip_addr, sizeof(resp->ip_addr), "%s",
            addr->conn.ip_addr);
    short2buff(addr->conn.port, resp->port);

    RESPONSE.header.body_len = sizeof(FCFSAuthProtoGetServerResp);
    TASK_CTX.common.response_done = true;
    return 0;
}
