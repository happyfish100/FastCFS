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

#include "auth_proto.h"

void fcfs_auth_proto_init()
{
}

const char *fcfs_auth_get_cmd_caption(const int cmd)
{
    switch (cmd) {
        case FCFS_AUTH_SERVICE_PROTO_USER_LOGIN_REQ:
            return "USER_LOGIN_REQ";
        case FCFS_AUTH_SERVICE_PROTO_USER_LOGIN_RESP:
            return "USER_LOGIN_RESP";
        default:
            return sf_get_cmd_caption(cmd);
    }
}
