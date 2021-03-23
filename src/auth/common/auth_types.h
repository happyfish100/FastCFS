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

#ifndef _FCFS_AUTH_TYPES_H
#define _FCFS_AUTH_TYPES_H

#include "fastcommon/common_define.h"
#include "sf/sf_types.h"

#define FCFS_AUTH_DEFAULT_SERVICE_PORT  31012

#define FCFS_AUTH_USERNAME_MAX_LEN   64
#define FCFS_AUTH_PASSWD_LEN         16
#define FCFS_AUTH_SESSION_ID_LEN      8

#define FCFS_AUTH_FIXED_USER_COUNT  256
#define FCFS_AUTH_FIXED_POOL_COUNT  256

#define FCFS_AUTH_USER_STATUS_NORMAL   0
#define FCFS_AUTH_USER_STATUS_DELETED  1

#define FCFS_AUTH_POOL_STATUS_NORMAL   0
#define FCFS_AUTH_POOL_STATUS_DELETED  1

#define FCFS_AUTH_USER_PRIV_NONE             0
#define FCFS_AUTH_USER_PRIV_USER_MANAGE      (1 << 0)
#define FCFS_AUTH_USER_PRIV_CREATE_POOL      (1 << 1)
#define FCFS_AUTH_USER_PRIV_MONITOR_CLUSTER  (1 << 2)

#define FCFS_AUTH_USER_PRIV_ALL  (FCFS_AUTH_USER_PRIV_USER_MANAGE | \
        FCFS_AUTH_USER_PRIV_CREATE_POOL | \
        FCFS_AUTH_USER_PRIV_MONITOR_CLUSTER)
#define FCFS_AUTH_USER_PRIV_COUNT      3

#define FCFS_AUTH_POOL_ACCESS_NONE       0
#define FCFS_AUTH_POOL_ACCESS_WRITE      (1 << 1)
#define FCFS_AUTH_POOL_ACCESS_READ       (1 << 2)

#define FCFS_AUTH_POOL_ACCESS_ALL  (FCFS_AUTH_POOL_ACCESS_WRITE | \
        FCFS_AUTH_POOL_ACCESS_READ)
#define FCFS_AUTH_POOL_ACCESS_COUNT      2

#define FCFS_AUTH_UNLIMITED_QUOTA_STR  "unlimited"
#define FCFS_AUTH_UNLIMITED_QUOTA_VAL  -1

typedef struct fcfs_auth_storage_pool_info {
    int64_t id;
    string_t name;
    int64_t quota;
    int status;
} FCFSAuthStoragePoolInfo;

typedef struct fcfs_auth_storage_pool_granted {
    int64_t id;
    FCFSAuthStoragePoolInfo pool;
    struct {
        int fdir;
        int fstore;
    } privs;
} FCFSAuthStoragePoolGranted;

typedef struct fcfs_auth_user_info {
    int64_t id;
    string_t name;
    string_t passwd;
    int64_t priv;
    int status;
} FCFSAuthUserInfo;

typedef struct fcfs_auth_storage_pool_array {
    FCFSAuthStoragePoolInfo *spools;
    FCFSAuthStoragePoolInfo fixed[FCFS_AUTH_FIXED_POOL_COUNT];
    int count;
    int alloc;
} FCFSAuthStoragePoolArray;

typedef struct fcfs_auth_user_array {
    FCFSAuthUserInfo *users;
    FCFSAuthUserInfo fixed[FCFS_AUTH_FIXED_USER_COUNT];
    int count;
    int alloc;
} FCFSAuthUserArray;

#endif
