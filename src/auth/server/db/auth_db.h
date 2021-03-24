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

#ifdef __cplusplus
extern "C" {
#endif

int auth_db_init();

int adb_load_data(AuthServerContext *server_ctx);

int adb_check_generate_admin_user(AuthServerContext *server_ctx);

int adb_user_create(AuthServerContext *server_ctx,
        const FCFSAuthUserInfo *user);

const FCFSAuthUserInfo *adb_user_get(AuthServerContext *server_ctx,
        const string_t *username);

int adb_user_remove(AuthServerContext *server_ctx, const string_t *username);

int adb_user_list(AuthServerContext *server_ctx, FCFSAuthUserArray *array);

int adb_user_update_priv(AuthServerContext *server_ctx,
        const string_t *username, const int64_t priv);

int adb_user_update_passwd(AuthServerContext *server_ctx,
        const string_t *username, const string_t *passwd);

/* storage pool */
int adb_spool_create(AuthServerContext *server_ctx, const string_t
        *username, const FCFSAuthStoragePoolInfo *pool);

const FCFSAuthStoragePoolInfo *adb_spool_get(AuthServerContext *server_ctx,
        const string_t *username, const string_t *poolname);

int adb_spool_remove(AuthServerContext *server_ctx,
        const string_t *username, const string_t *poolname);

int adb_spool_set_quota(AuthServerContext *server_ctx,
        const string_t *username, const string_t *poolname,
        const int64_t quota);

int adb_spool_list(AuthServerContext *server_ctx, const string_t *username,
        FCFSAuthStoragePoolArray *array);

/* granted pool */

#ifdef __cplusplus
}
#endif

#endif
