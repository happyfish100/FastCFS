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

#include "dao_types.h"

#ifdef __cplusplus
extern "C" {
#endif

    int user_create(const string_t *username, const string_t *passwd,
            const int64_t priv);

    int user_remove(const string_t *username);

    int user_list(const string_t *username, AuthUserArray *array);

    int user_update_priv(const string_t *username, const int64_t priv);

    int user_update_passwd(const string_t *username, const string_t *passwd);

    int user_login(const string_t *username,
            const string_t *passwd, int64_t *priv);

    static inline void user_init_array(AuthUserArray *array)
    {
        array->users = array->fixed;
        array->alloc = AUTH_FIXED_USER_COUNT;
        array->count = 0;
    }

    static inline void user_free_array(AuthUserArray *array)
    {
        if (array->users != array->fixed) {
            free(array->users);
            array->users = array->fixed;
            array->alloc = AUTH_FIXED_USER_COUNT;
        }
    }

#ifdef __cplusplus
}
#endif

#endif
