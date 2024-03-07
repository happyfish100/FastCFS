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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <limits.h>
#include <pthread.h>
#include "fastcommon/logger.h"
#include "fastcommon/shared_func.h"
#include "fastcommon/sched_thread.h"
#include "fastcommon/local_ip_func.h"
#include "server_global.h"
#include "cluster_info.h"

static int init_cluster_server_array()
{
    int bytes;
    FCFSAuthClusterServerInfo *cs;
    FCServerInfo *server;
    FCServerInfo *end;

    bytes = sizeof(FCFSAuthClusterServerInfo) *
        FC_SID_SERVER_COUNT(CLUSTER_SERVER_CONFIG);
    if ((CLUSTER_SERVER_ARRAY.servers=(FCFSAuthClusterServerInfo *)
                fc_malloc(bytes)) == NULL)
    {
        return ENOMEM;
    }
    memset(CLUSTER_SERVER_ARRAY.servers, 0, bytes);

    end = FC_SID_SERVERS(CLUSTER_SERVER_CONFIG) +
        FC_SID_SERVER_COUNT(CLUSTER_SERVER_CONFIG);
    for (server=FC_SID_SERVERS(CLUSTER_SERVER_CONFIG),
            cs=CLUSTER_SERVER_ARRAY.servers; server<end; server++, cs++)
    {
        cs->server = server;
    }

    CLUSTER_SERVER_ARRAY.count = FC_SID_SERVER_COUNT(CLUSTER_SERVER_CONFIG);
    return 0;
}

static int find_myself_in_cluster_config(const char *filename)
{
    const char *local_ip;
    struct {
        const char *ip_addr;
        int port;
    } found;
    FCServerInfo *server;
    FCFSAuthClusterServerInfo *myself;
    SFNetworkHandler *service_handler;
    SFNetworkHandler *cluster_handler;
    char formatted_found_ip[FORMATTED_IP_SIZE];
    char formatted_local_ip[FORMATTED_IP_SIZE];
    int ports[4];
    int count;
    int i;

    service_handler = SERVICE_SF_CTX.handlers[SF_IPV4_ADDRESS_FAMILY_INDEX].
        handlers + SF_SOCKET_NETWORK_HANDLER_INDEX;
    cluster_handler = CLUSTER_SF_CTX.handlers[SF_IPV4_ADDRESS_FAMILY_INDEX].
        handlers + SF_SOCKET_NETWORK_HANDLER_INDEX;
    count = 0;
    ports[count++] = service_handler->inner.port;
    if (service_handler->outer.port != service_handler->inner.port) {
        ports[count++] = service_handler->outer.port;
    }

    ports[count++] = cluster_handler->inner.port;
    if (cluster_handler->outer.port != cluster_handler->inner.port) {
        ports[count++] = cluster_handler->outer.port;
    }

    found.ip_addr = NULL;
    found.port = 0;
    local_ip = get_first_local_ip();
    while (local_ip != NULL) {
        for (i=0; i<count; i++) {
            server = fc_server_get_by_ip_port(&CLUSTER_SERVER_CONFIG,
                    local_ip, ports[i]);
            if (server != NULL) {
                myself = CLUSTER_SERVER_ARRAY.servers +
                    (server - FC_SID_SERVERS(CLUSTER_SERVER_CONFIG));
                if (CLUSTER_MYSELF_PTR == NULL) {
                    CLUSTER_MYSELF_PTR = myself;
                } else if (myself != CLUSTER_MYSELF_PTR) {
                    format_ip_address(found.ip_addr, formatted_found_ip);
                    format_ip_address(local_ip, formatted_local_ip);
                    logError("file: "__FILE__", line: %d, "
                            "cluster config file: %s, my ip and port "
                            "in more than one instances, %s:%u in "
                            "server id %d, and %s:%u in server id %d",
                            __LINE__, filename, formatted_found_ip, found.port,
                            CLUSTER_MY_SERVER_ID, formatted_local_ip,
                            ports[i], myself->server->id);
                    return EEXIST;
                }

                found.ip_addr = local_ip;
                found.port = ports[i];
            }
        }

        local_ip = get_next_local_ip(local_ip);
    }

    if (CLUSTER_MYSELF_PTR == NULL) {
        logError("file: "__FILE__", line: %d, "
                "cluster config file: %s, can't find myself "
                "by my local ip and listen port", __LINE__, filename);
        return ENOENT;
    }

    return 0;
}

FCFSAuthClusterServerInfo *fcfs_auth_get_server_by_id(const int server_id)
{
    FCServerInfo *server;
    server = fc_server_get_by_id(&CLUSTER_SERVER_CONFIG, server_id);
    if (server == NULL) {
        return NULL;
    }

    return CLUSTER_SERVER_ARRAY.servers + (server -
            FC_SID_SERVERS(CLUSTER_SERVER_CONFIG));
}

int cluster_info_init(const char *cluster_config_filename)
{
    int result;

    if ((result=init_cluster_server_array()) != 0) {
        return result;
    }

    if ((result=find_myself_in_cluster_config(cluster_config_filename)) != 0) {
        return result;
    }

    return 0;
}
