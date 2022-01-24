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
        case FCFS_AUTH_SERVICE_PROTO_SESSION_SUBSCRIBE_REQ:
            return "SESSION_SUBSCRIBE_REQ";
        case FCFS_AUTH_SERVICE_PROTO_SESSION_SUBSCRIBE_RESP:
            return "SESSION_SUBSCRIBE_RESP";
        case FCFS_AUTH_SERVICE_PROTO_SESSION_PUSH_REQ:
            return "SESSION_PUSH_REQ";
        case FCFS_AUTH_SERVICE_PROTO_SESSION_PUSH_RESP:
            return "SESSION_PUSH_RESP";
        case FCFS_AUTH_SERVICE_PROTO_SESSION_VALIDATE_REQ:
            return "SESSION_VALIDATE_REQ";
        case FCFS_AUTH_SERVICE_PROTO_SESSION_VALIDATE_RESP:
            return "SESSION_VALIDATE_RESP";
        case FCFS_AUTH_SERVICE_PROTO_USER_CREATE_REQ:
            return "USER_CREATE_REQ";
        case FCFS_AUTH_SERVICE_PROTO_USER_CREATE_RESP:
            return "USER_CREATE_RESP";
        case FCFS_AUTH_SERVICE_PROTO_USER_LIST_REQ:
            return "USER_LIST_REQ";
        case FCFS_AUTH_SERVICE_PROTO_USER_LIST_RESP:
            return "USER_LIST_RESP";
        case FCFS_AUTH_SERVICE_PROTO_USER_GRANT_REQ:
            return "USER_GRANT_REQ";
        case FCFS_AUTH_SERVICE_PROTO_USER_GRANT_RESP:
            return "USER_GRANT_RESP";
        case FCFS_AUTH_SERVICE_PROTO_USER_REMOVE_REQ:
            return "USER_REMOVE_REQ";
        case FCFS_AUTH_SERVICE_PROTO_USER_REMOVE_RESP:
            return "USER_REMOVE_RESP";
        case FCFS_AUTH_SERVICE_PROTO_USER_PASSWD_REQ:
            return "USER_PASSWD_REQ";
        case FCFS_AUTH_SERVICE_PROTO_USER_PASSWD_RESP:
            return "USER_PASSWD_RESP";

        case FCFS_AUTH_SERVICE_PROTO_SPOOL_CREATE_REQ:
            return "SPOOL_CREATE_REQ";
        case FCFS_AUTH_SERVICE_PROTO_SPOOL_CREATE_RESP:
            return "SPOOL_CREATE_RESP";
        case FCFS_AUTH_SERVICE_PROTO_SPOOL_LIST_REQ:
            return "SPOOL_LIST_REQ";
        case FCFS_AUTH_SERVICE_PROTO_SPOOL_LIST_RESP:
            return "SPOOL_LIST_RESP";
        case FCFS_AUTH_SERVICE_PROTO_SPOOL_REMOVE_REQ:
            return "SPOOL_REMOVE_REQ";
        case FCFS_AUTH_SERVICE_PROTO_SPOOL_REMOVE_RESP:
            return "SPOOL_REMOVE_RESP";
        case FCFS_AUTH_SERVICE_PROTO_SPOOL_SET_QUOTA_REQ:
            return "SPOOL_SET_QUOTA_REQ";
        case FCFS_AUTH_SERVICE_PROTO_SPOOL_SET_QUOTA_RESP:
            return "SPOOL_SET_QUOTA_RESP";
        case FCFS_AUTH_SERVICE_PROTO_SPOOL_GET_QUOTA_REQ:
            return "SPOOL_GET_QUOTA_REQ";
        case FCFS_AUTH_SERVICE_PROTO_SPOOL_GET_QUOTA_RESP:
            return "SPOOL_GET_QUOTA_RESP";
        case FCFS_AUTH_SERVICE_PROTO_GPOOL_GRANT_REQ:
            return "GPOOL_GRANT_REQ";
        case FCFS_AUTH_SERVICE_PROTO_GPOOL_GRANT_RESP:
            return "GPOOL_GRANT_RESP";
        case FCFS_AUTH_SERVICE_PROTO_GPOOL_WITHDRAW_REQ:
            return "GPOOL_WITHDRAW_REQ";
        case FCFS_AUTH_SERVICE_PROTO_GPOOL_WITHDRAW_RESP:
            return "GPOOL_WITHDRAW_RESP";
        case FCFS_AUTH_SERVICE_PROTO_GPOOL_LIST_REQ:
            return "GPOOL_LIST_REQ";
        case FCFS_AUTH_SERVICE_PROTO_GPOOL_LIST_RESP:
            return "GPOOL_LIST_RESP";

        case FCFS_AUTH_SERVICE_PROTO_GET_MASTER_REQ:
            return "GET_MASTER_REQ";
        case FCFS_AUTH_SERVICE_PROTO_GET_MASTER_RESP:
            return "GET_MASTER_RESP";
        case FCFS_AUTH_SERVICE_PROTO_CLUSTER_STAT_REQ:
            return "CLUSTER_STAT_REQ";
        case FCFS_AUTH_SERVICE_PROTO_CLUSTER_STAT_RESP:
            return "CLUSTER_STAT_RESP";
        case FCFS_AUTH_CLUSTER_PROTO_GET_SERVER_STATUS_REQ:
            return "GET_SERVER_STATUS_REQ";
        case FCFS_AUTH_CLUSTER_PROTO_GET_SERVER_STATUS_RESP:
            return "GET_SERVER_STATUS_RESP";
        case FCFS_AUTH_CLUSTER_PROTO_JOIN_MASTER:
            return "JOIN_MASTER";
        case FCFS_AUTH_CLUSTER_PROTO_PING_MASTER_REQ:
            return "PING_MASTER_REQ";
        case FCFS_AUTH_CLUSTER_PROTO_PING_MASTER_RESP:
            return "PING_MASTER_RESP";
        case FCFS_AUTH_CLUSTER_PROTO_PRE_SET_NEXT_MASTER:
            return "PRE_SET_NEXT_MASTER";
        case FCFS_AUTH_CLUSTER_PROTO_COMMIT_NEXT_MASTER:
            return "COMMIT_NEXT_MASTER";
        default:
            return sf_get_cmd_caption(cmd);
    }
}
