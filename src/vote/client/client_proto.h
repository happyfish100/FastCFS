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

#ifdef __cplusplus
}
#endif

#endif
