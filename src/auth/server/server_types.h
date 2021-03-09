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


#ifndef _SERVER_TYPES_H
#define _SERVER_TYPES_H

#define AUTH_FIXED_USER_COUNT  256

typedef struct auth_user_info {
    string_t username;
    int64_t priv;
} AuthUserInfo;

typedef struct auth_user_array {
    AuthUserInfo *users;
    AuthUserInfo fixed[AUTH_FIXED_USER_COUNT];
    int count;
    int alloc;
} AuthUserArray;

#endif
