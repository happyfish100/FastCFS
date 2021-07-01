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

//common_handler.h

#ifndef FCFS_AUTH_COMMON_HANDLER_H
#define FCFS_AUTH_COMMON_HANDLER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "server_types.h"

#ifdef __cplusplus

extern "C" {
#endif

void common_handler_init();

int fcfs_auth_deal_get_master(struct fast_task_info *task,
        const int group_index);

#ifdef __cplusplus
}
#endif

#endif
