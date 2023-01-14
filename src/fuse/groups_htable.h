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

#ifndef _FCFS_GROUPS_HTABLE_H
#define _FCFS_GROUPS_HTABLE_H

#include "sf/sf_sharding_htable.h"

#ifdef __cplusplus
extern "C" {
#endif

    int fcfs_groups_htable_init();

    int fcfs_groups_htable_insert(const pid_t pid, const uid_t uid,
            const gid_t gid, const int count, const char *list);

    int fcfs_groups_htable_find(const pid_t pid, const uid_t uid,
            const gid_t gid, int *count, char *list);

#ifdef __cplusplus
}
#endif

#endif
