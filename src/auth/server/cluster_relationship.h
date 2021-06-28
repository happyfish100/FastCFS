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

//cluster_relationship.h

#ifndef _CLUSTER_RELATIONSHIP_H_
#define _CLUSTER_RELATIONSHIP_H_

#include <time.h>
#include <pthread.h>
#include "server_types.h"

#ifdef __cplusplus
extern "C" {
#endif

int cluster_relationship_init();
int cluster_relationship_destroy();

int cluster_relationship_pre_set_master(FCFSAuthClusterServerInfo *master);

int cluster_relationship_commit_master(FCFSAuthClusterServerInfo *master);

void cluster_relationship_trigger_reselect_master();

void cluster_relationship_add_to_inactive_sarray(FCFSAuthClusterServerInfo *cs);

void cluster_relationship_remove_from_inactive_sarray(FCFSAuthClusterServerInfo *cs);

#ifdef __cplusplus
}
#endif

#endif
