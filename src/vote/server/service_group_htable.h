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

//service_group_htable.h

#ifndef _SERVICE_GROUP_HTABLE_H_
#define _SERVICE_GROUP_HTABLE_H_

#include <time.h>
#include "server_global.h"

typedef struct fcfs_vote_service_group_info {
    uint64_t hash_code;
    short service_id;
    short response_size;
    int group_id;
    volatile int next_leader;
    volatile int leader_id;
    struct fast_task_info *task;
    struct fcfs_vote_service_group_info *next;  //for htable
} FCFSVoteServiceGroupInfo;

#ifdef __cplusplus
extern "C" {
#endif

int service_group_htable_init();

int service_group_htable_get(const short service_id, const int group_id,
        const int leader_id, const short response_size,
        struct fast_task_info *task, FCFSVoteServiceGroupInfo **group);

void service_group_htable_unset_task(FCFSVoteServiceGroupInfo *group);

void service_group_htable_clear_tasks();

#ifdef __cplusplus
}
#endif

#endif
