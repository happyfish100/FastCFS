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

int adb_user_create(AuthServerContext *server_ctx, const string_t *username,
        const string_t *passwd, const int64_t priv);

const FCFSAuthUserInfo *adb_user_get(AuthServerContext *server_ctx,
        const string_t *username);

int adb_user_remove(AuthServerContext *server_ctx, const string_t *username);

int adb_user_list(AuthServerContext *server_ctx, const string_t *username,
        FCFSAuthUserArray *array);

int adb_user_update_priv(AuthServerContext *server_ctx,
        const string_t *username, const int64_t priv);

int adb_user_update_passwd(AuthServerContext *server_ctx,
        const string_t *username, const string_t *passwd);

/*
    int user_login(const string_t *username,
            const string_t *passwd, int64_t *priv);
            */

#ifdef __cplusplus
}
#endif

#endif
