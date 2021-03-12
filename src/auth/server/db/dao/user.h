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

#ifndef _AUTH_DAO_USER_H
#define _AUTH_DAO_USER_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

int dao_user_create(FDIRClientContext *client_ctx, const string_t *username,
        const string_t *passwd, const int64_t priv);

int dao_user_remove(FDIRClientContext *client_ctx, const string_t *username);

int dao_user_list(FDIRClientContext *client_ctx, const string_t *username,
        FCFSAuthUserArray *array);

int dao_user_update_priv(FDIRClientContext *client_ctx,
        const string_t *username, const int64_t priv);

int dao_user_update_passwd(FDIRClientContext *client_ctx,
        const string_t *username, const string_t *passwd);

#ifdef __cplusplus
}
#endif

#endif
