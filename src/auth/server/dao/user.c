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


#include <sys/stat.h>
#include "fastcommon/shared_func.h"
#include "fastcommon/logger.h"
#include "user.h"

int user_create(const string_t *username, const string_t *passwd,
        const int64_t priv)
{
    int result;
    AuthFullPath path;

    AUTH_SET_USER_PATH(username, path);
    /*
    if ((result=dentry_find(&path->fullname, &dentry)) != 0) {
        return result;
    }
    */

    return 0;
}

int user_remove(const string_t *username)
{
    return 0;
}
