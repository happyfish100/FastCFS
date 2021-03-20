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

#ifndef _FCFS_AUTH_TOOL_FUNC_H
#define _FCFS_AUTH_TOOL_FUNC_H

#include "sf/sf_proto.h"
#include "auth_types.h"

#define USER_PRIV_NAME_USER_MANAGE_STR     "user"
#define USER_PRIV_NAME_CREATE_POOL_STR     "pool"
#define USER_PRIV_NAME_MONITOR_CLUSTER_STR "cluster"
#define USER_PRIV_NAME_ALL_PRIVS_STR       "*"

#define USER_PRIV_NAME_USER_MANAGE_LEN  \
    (sizeof(USER_PRIV_NAME_USER_MANAGE_STR) - 1)

#define USER_PRIV_NAME_CREATE_POOL_LEN  \
    (sizeof(USER_PRIV_NAME_CREATE_POOL_STR) - 1)

#define USER_PRIV_NAME_MONITOR_CLUSTER_LEN  \
    (sizeof(USER_PRIV_NAME_MONITOR_CLUSTER_STR) - 1)

#define USER_PRIV_NAME_ALL_PRIVS_LEN  \
    (sizeof(USER_PRIV_NAME_ALL_PRIVS_STR) - 1)

#ifdef __cplusplus
extern "C" {
#endif

    int fcfs_auth_parse_user_priv(const string_t *str, int64_t *priv);

    const char *fcfs_auth_user_priv_to_string(
            const int64_t priv, string_t *str);

#ifdef __cplusplus
}
#endif

#endif
