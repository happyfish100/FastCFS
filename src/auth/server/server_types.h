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

#include "fastcommon/uniq_skiplist.h"
#include "sf/sf_types.h"
#include "common/auth_types.h"
#include "common/server_session.h"

#define TASK_STATUS_CONTINUE           12345
#define TASK_ARG          ((AuthServerTaskArg *)task->arg)
#define TASK_CTX          TASK_ARG->context
#define SESSION_ENTRY     TASK_CTX.session.entry
#define SESSION_USER      TASK_CTX.session.user
#define REQUEST           TASK_CTX.common.request
#define RESPONSE          TASK_CTX.common.response
#define RESPONSE_STATUS   RESPONSE.header.status
#define REQUEST_STATUS    REQUEST.header.status
#define SERVER_TASK_TYPE  TASK_CTX.task_type

#define SERVER_CTX        ((AuthServerContext *)task->thread_data->arg)

typedef struct server_task_arg {
    struct {
        SFCommonTaskContext common;
        int task_type;
        struct {
            const FCFSAuthUserInfo *user;
            ServerSessionEntry *entry;
        } session;
    } context;
} AuthServerTaskArg;

typedef struct auth_server_context {
    void *dao_ctx;
} AuthServerContext;

#endif
