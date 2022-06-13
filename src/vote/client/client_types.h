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

#ifndef _FCFS_VOTE_CLIENT_TYPES_H
#define _FCFS_VOTE_CLIENT_TYPES_H

#include "fastcommon/common_define.h"
#include "sf/sf_configs.h"
#include "sf/sf_connection_manager.h"
#include "sf/sf_cluster_cfg.h"
#include "vote_types.h"

#define FCFS_VOTE_CLIENT_DEFAULT_CONFIG_FILENAME "/etc/fastcfs/vote/client.conf"

typedef struct fcfs_vote_client_server_entry {
    int server_id;
    ConnectionInfo conn;
    char status;
} FCFSVoteClientServerEntry;

typedef struct fcfs_vote_client_context {
    SFClusterConfig cluster;
    int connect_timeout;
    int network_timeout;
} FCFSVoteClientContext;

#endif
