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

//cluster_handler.h

#ifndef FCFS_VOTE_CLUSTER_HANDLER_H
#define FCFS_VOTE_CLUSTER_HANDLER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "fastcommon/fast_task_queue.h"
#include "server_types.h"

#ifdef __cplusplus

extern "C" {
#endif

int cluster_handler_init();
int cluster_handler_destroy();
int cluster_deal_task(struct fast_task_info *task, const int stage);
void cluster_task_finish_cleanup(struct fast_task_info *task);
int cluster_recv_timeout_callback(struct fast_task_info *task);

#ifdef __cplusplus
}
#endif

#endif
