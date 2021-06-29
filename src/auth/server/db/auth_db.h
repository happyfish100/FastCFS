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


#ifndef _FCFS_AUTH_DB_H
#define _FCFS_AUTH_DB_H

#include "../server_types.h"

typedef struct db_user_info {
    FCFSAuthUserInfo user;
    struct {
        struct uniq_skiplist *created;  //element: DBStoragePoolInfo
        struct uniq_skiplist *granted;  //element: DBGrantedPoolInfo
    } storage_pools;
} DBUserInfo;

typedef struct db_storage_pool_info {
    FCFSAuthStoragePoolInfo pool;
    DBUserInfo *user;
} DBStoragePoolInfo;

typedef struct db_granted_pool_info {
    FCFSAuthGrantedPoolInfo granted;
    DBStoragePoolInfo *sp;
} DBGrantedPoolInfo;

typedef void (*adb_user_priv_change_callback)(
        const int64_t user_id, const int64_t new_priv);
typedef void (*adb_pool_priv_change_callback)(const int64_t user_id,
        const int64_t pool_id, const FCFSAuthSPoolPriviledges *privs);
typedef void (*adb_pool_quota_avail_change_callback)(
        const int64_t pool_id, const bool available);

typedef struct db_priv_change_callbacks {
    adb_user_priv_change_callback user_priv_changed;
    adb_pool_priv_change_callback pool_priv_changed;
    adb_pool_quota_avail_change_callback pool_quota_avail_changed;
} DBPrivChangeCallbacks;


#ifdef __cplusplus
extern "C" {
#endif

extern DBPrivChangeCallbacks g_db_priv_change_callbacks;

int adb_load_data(AuthServerContext *server_ctx);

void auth_db_destroy();

int adb_check_generate_admin_user(AuthServerContext *server_ctx);

int adb_user_create(AuthServerContext *server_ctx,
        const FCFSAuthUserInfo *user);

const DBUserInfo *adb_user_get(AuthServerContext *server_ctx,
        const string_t *username);

int adb_user_remove(AuthServerContext *server_ctx, const string_t *username);

int adb_user_list(AuthServerContext *server_ctx,
        const SFListLimitInfo *limit,
        FCFSAuthUserArray *array);

int adb_user_update_priv(AuthServerContext *server_ctx,
        const string_t *username, const int64_t priv);

int adb_user_update_passwd(AuthServerContext *server_ctx,
        const string_t *username, const string_t *passwd);

/* storage pool */
int64_t adb_spool_get_auto_id(AuthServerContext *server_ctx);
int adb_spool_inc_auto_id(AuthServerContext *server_ctx);
int adb_spool_next_auto_id(AuthServerContext *server_ctx, int64_t *next_id);

DBStoragePoolInfo *adb_spool_global_get(const string_t *poolname);

static inline int adb_spool_access(AuthServerContext
        *server_ctx, const string_t *poolname)
{
    return adb_spool_global_get(poolname) != NULL ? 0 : ENOENT;
}

int adb_spool_create(AuthServerContext *server_ctx, const string_t
        *username, const FCFSAuthStoragePoolInfo *pool);

const FCFSAuthStoragePoolInfo *adb_spool_get(AuthServerContext *server_ctx,
        const string_t *username, const string_t *poolname);

int adb_spool_remove(AuthServerContext *server_ctx,
        const string_t *username, const string_t *poolname);

int adb_spool_set_quota(AuthServerContext *server_ctx,
        const string_t *username, const string_t *poolname,
        const int64_t quota);

int adb_spool_get_quota(AuthServerContext *server_ctx,
        const string_t *poolname, int64_t *quota);

int adb_spool_list(AuthServerContext *server_ctx, const string_t *username,
        const SFListLimitInfo *limit, FCFSAuthStoragePoolArray *array);

int adb_spool_set_used_bytes(const string_t *poolname,
        const int64_t used_bytes);

/* granted pool */
int adb_granted_create(AuthServerContext *server_ctx, const string_t *username,
        FCFSAuthGrantedPoolInfo *granted);

int adb_granted_remove(AuthServerContext *server_ctx,
        const string_t *username, const int64_t pool_id);

int adb_granted_full_get(AuthServerContext *server_ctx, const string_t
        *username, const int64_t pool_id, FCFSAuthGrantedPoolFullInfo *gf);

int adb_granted_privs_get(AuthServerContext *server_ctx,
        const DBUserInfo *dbuser, const DBStoragePoolInfo *dbpool,
        FCFSAuthSPoolPriviledges *privs);

int adb_granted_list(AuthServerContext *server_ctx, const string_t *username,
        const SFListLimitInfo *limit, FCFSAuthGrantedPoolArray *array);

#ifdef __cplusplus
}
#endif

#endif
