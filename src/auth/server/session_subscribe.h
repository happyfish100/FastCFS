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


#ifndef _AUTH_SESSION_SUBSCRIBE_H
#define _AUTH_SESSION_SUBSCRIBE_H

#include "fastcommon/fast_mblock.h"
#include "server_types.h"

typedef struct auth_session_subscribe_entry {
    char operation;
    uint64_t session_id;
    SessionSyncedFields fields;
    struct auth_session_subscribe_entry *next;  //for fc_queue
} ServerSessionSubscribeEntry;

#ifdef __cplusplus
extern "C" {
#endif

    extern ServerSessionCallbacks g_server_session_callbacks;

    int session_subscribe_init();
    void session_subscribe_destroy();

    void session_subscribe_free_entries(ServerSessionSubscribeEntry *entry);

    ServerSessionSubscriber *session_subscribe_alloc();

    void session_subscribe_register(ServerSessionSubscriber *subscriber);

    void session_subscribe_unregister(ServerSessionSubscriber *subscriber);

    void session_subscribe_release(ServerSessionSubscriber *subscriber);

    void session_subscribe_clear_session();

#ifdef __cplusplus
}
#endif

#endif
