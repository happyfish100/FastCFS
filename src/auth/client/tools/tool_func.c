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

#include "tool_func.h"

int fcfs_auth_parse_user_priv(const string_t *str, int64_t *priv)
{
    const bool ignore_empty = true;
    string_t parts[2 * FCFS_AUTH_USER_PRIV_COUNT];
    string_t *p;
    string_t *end;
    int64_t n;
    int count;

    *priv = 0;
    count = split_string_ex(str, ',', parts, sizeof(parts) /
            sizeof(string_t), ignore_empty);
    end = parts + count;
    for (p=parts; p<end; p++) {
        if ((n=fcfs_auth_get_user_priv(p)) == 
                FCFS_AUTH_USER_PRIV_NONE)
        {
            logError("file: "__FILE__", line: %d, "
                    "unkown priviledge: %*s", __LINE__,
                    p->len, p->str);
            return EINVAL;
        }

        *priv |= n;
    }

    return 0;
}
