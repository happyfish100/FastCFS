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

#define FCFS_USER_PRIV_ARRAY_COUNT  (FCFS_AUTH_USER_PRIV_COUNT + 1)

static id_name_pair_t user_priv_list[FCFS_USER_PRIV_ARRAY_COUNT] = {
    {FCFS_AUTH_USER_PRIV_USER_MANAGE,
        {USER_PRIV_NAME_USER_MANAGE_STR,
            USER_PRIV_NAME_USER_MANAGE_LEN}},

    {FCFS_AUTH_USER_PRIV_CREATE_POOL,
        {USER_PRIV_NAME_CREATE_POOL_STR,
            USER_PRIV_NAME_CREATE_POOL_LEN}},

    {FCFS_AUTH_USER_PRIV_SUBSCRIBE_SESSION,
        {USER_PRIV_NAME_SUBSCRIBE_SESSION_STR,
            USER_PRIV_NAME_SUBSCRIBE_SESSION_LEN}},

    {FCFS_AUTH_USER_PRIV_MONITOR_CLUSTER,
        {USER_PRIV_NAME_MONITOR_CLUSTER_STR,
            USER_PRIV_NAME_MONITOR_CLUSTER_LEN}},

    {FCFS_AUTH_USER_PRIV_ALL,
        {USER_PRIV_NAME_ALL_PRIVS_STR,
            USER_PRIV_NAME_ALL_PRIVS_LEN}}
};

static inline int64_t fcfs_auth_get_user_priv(const string_t *str)
{
    id_name_pair_t *pair;
    id_name_pair_t *end;

    end = user_priv_list + FCFS_USER_PRIV_ARRAY_COUNT;
    for (pair=user_priv_list; pair<end; pair++) {
        if (fc_string_case_equal(&pair->name, str)) {
            return pair->id;
        }
    }

    return FCFS_AUTH_USER_PRIV_NONE;
}

static inline const string_t *fcfs_auth_get_user_priv_name(const int64_t priv)
{
    id_name_pair_t *pair;
    id_name_pair_t *end;

    end = user_priv_list + FCFS_USER_PRIV_ARRAY_COUNT;
    for (pair=user_priv_list; pair<end; pair++) {
        if (pair->id == priv) {
            return &pair->name;
        }
    }

    return NULL;
}

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

const char *fcfs_auth_user_priv_to_string(
        const int64_t priv, string_t *str)
{
    id_name_pair_t *pair;
    id_name_pair_t *end;
    char *p;
    const string_t *name;

    if ((name=fcfs_auth_get_user_priv_name(priv)) != NULL) {
        memcpy(str->str, name->str, name->len);
        str->len = name->len;
    } else {
        p = str->str;
        end = user_priv_list + FCFS_USER_PRIV_ARRAY_COUNT;
        for (pair=user_priv_list; pair<end; pair++) {
            if ((priv & pair->id) == pair->id) {
                memcpy(p, pair->name.str, pair->name.len);
                p += pair->name.len;
                *p++ = ',';
            }
        }

        str->len = p - str->str;
        if (str->len > 0) {
            str->len--;  //remove last comma
        }
    }

    *(str->str + str->len) = '\0';
    return str->str;
}

int fcfs_auth_parse_pool_access(const string_t *str, int *priv)
{
    const char *p;
    const char *end;

    *priv = FCFS_AUTH_POOL_ACCESS_NONE;
    end = str->str + str->len;
    for (p=str->str; p<end; p++) {
        switch (*p) {
            case POOL_ACCESS_NAME_READ_CHR:
                *priv |= FCFS_AUTH_POOL_ACCESS_READ;
                break;
            case POOL_ACCESS_NAME_WRITE_CHR:
                *priv |= FCFS_AUTH_POOL_ACCESS_WRITE;
                break;
            default:
                logError("file: "__FILE__", line: %d, "
                        "unkown pool access: %c (0x%02x)",
                        __LINE__, *p, *p);
                return EINVAL;
        }
    }

    return 0;
}

const char *fcfs_auth_pool_access_to_string(const int priv, string_t *str)
{
    char *p;

    p = str->str;
    if ((priv & FCFS_AUTH_POOL_ACCESS_READ) != 0) {
        *p++ = POOL_ACCESS_NAME_READ_CHR;
    }
    if ((priv & FCFS_AUTH_POOL_ACCESS_WRITE) != 0) {
        *p++ = POOL_ACCESS_NAME_WRITE_CHR;
    }

    *p = '\0';
    str->len = p - str->str;
    return str->str;
}
