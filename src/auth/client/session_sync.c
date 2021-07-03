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
#include <limits.h>
#include "fastcommon/shared_func.h"
#include "fastcommon/logger.h"
#include "sf/sf_proto.h"
#include "sf/sf_global.h"
#include "auth_proto.h"
#include "auth_func.h"
#include "server_session.h"
#include "client_global.h"
#include "client_proto.h"
#include "session_sync.h"

typedef struct synced_session_entry {
    char operation;
    uint64_t session_id;
    SessionSyncedFields fields;
} SyncedSessionEntry;

typedef struct synced_session_array {
    int alloc;
    int count;
    SyncedSessionEntry *entries;
    SFProtoRecvBuffer buffer;
} SyncedSessionArray;

static SyncedSessionArray session_array;

static int synced_session_array_init(SyncedSessionArray *array)
{
    int result;

    if ((result=sf_init_recv_buffer(&array->buffer, 0)) != 0) {
        return result;
    }

    array->alloc = array->count = 0;
    array->entries = NULL;
    return 0;
}

static int check_realloc_session_array(SyncedSessionArray *array,
        const int target_count)
{
    SyncedSessionEntry *new_entries;
    int new_alloc;
    int bytes;

    if (target_count <= array->alloc) {
        return 0;
    }

    if (array->alloc == 0) {
        new_alloc = 4 * 1024;
    } else {
        new_alloc = 2 * array->alloc;
    }
    while (new_alloc < target_count) {
        new_alloc *= 2;
    }

    bytes = sizeof(SyncedSessionEntry) * new_alloc;
    new_entries = (SyncedSessionEntry *)fc_malloc(bytes);
    if (new_entries == NULL) {
        logError("file: "__FILE__", line: %d, "
                "malloc %d bytes fail", __LINE__, bytes);
        return ENOMEM;
    }

    if (array->entries != NULL) {
        free(array->entries);
    }
    array->entries = new_entries;
    array->alloc = new_alloc;
    return 0;
}

static int parse_session_push(SFResponseInfo *response)
{
    FCFSAuthProtoSessionPushRespBodyHeader *body_header;
    FCFSAuthProtoSessionPushRespBodyPart *body_part;
    SyncedSessionEntry *entry;
    SyncedSessionEntry *end;
    char *p;
    int count;
    int result;

    body_header = (FCFSAuthProtoSessionPushRespBodyHeader *)
        session_array.buffer.buff;
    p = (char *)(body_header + 1);
    count = buff2int(body_header->count);
    if ((result=check_realloc_session_array(
                    &session_array, count)) != 0)
    {
        return result;
    }

    end = session_array.entries + count;
    for (entry=session_array.entries; entry<end; entry++) {
        if ((p - session_array.buffer.buff) + sizeof(
                    FCFSAuthProtoSessionPushRespBodyPart) >
                response->header.body_len)
        {
            response->error.length = sprintf(
                    response->error.message,
                    "body length: %d is too small",
                    response->header.body_len);
            return EINVAL;
        }

        body_part = (FCFSAuthProtoSessionPushRespBodyPart *)p;
        entry->operation = body_part->operation;
        entry->session_id = buff2long(body_part->session_id);
        if (entry->operation == FCFS_AUTH_SESSION_OP_TYPE_CREATE) {
            entry->fields.user.id = buff2long(body_part->entry->user.id);
            entry->fields.user.priv = buff2long(body_part->entry->user.priv);
            entry->fields.pool.id = buff2long(body_part->entry->pool.id);
            entry->fields.pool.available = body_part->entry->pool.available;
            entry->fields.pool.privs.fdir = buff2int(
                    body_part->entry->pool.privs.fdir);
            entry->fields.pool.privs.fstore = buff2int(
                    body_part->entry->pool.privs.fstore);

            p += sizeof(FCFSAuthProtoSessionPushRespBodyPart) +
                sizeof(FCFSAuthProtoSessionPushEntry);
        } else {
            p += sizeof(FCFSAuthProtoSessionPushRespBodyPart);
        }
    }

    if (response->header.body_len != (p - session_array.buffer.buff)) {
        response->error.length = sprintf(response->error.message,
                "body length: %d != expect: %d",
                response->header.body_len,
                (int)(p - session_array.buffer.buff));
        return EINVAL;
    }

    session_array.count = count;
    return 0;
}

static void deal_session_array()
{
    const bool publish = true;
    ServerSessionEntry session;
    SyncedSessionEntry *entry;
    SyncedSessionEntry *end;

    end = session_array.entries + session_array.count;
    for (entry=session_array.entries; entry<end; entry++) {
        if (0) {
            char buff[512];
            int len;
            len = sprintf(buff, "%d. session id: %"PRId64", operation: %c",
                    (int)(entry - session_array.entries + 1),
                    entry->session_id, entry->operation);
            if (entry->operation == FCFS_AUTH_SESSION_OP_TYPE_CREATE) {
                len += sprintf(buff + len, ", user {id: %"PRId64
                        ", priv: %"PRId64"}", entry->fields.user.id,
                        entry->fields.user.priv);
                if (entry->fields.pool.id != 0) {
                    len += sprintf(buff + len, ", pool {id: %"PRId64
                            ", avail: %d, privs: {fdir: %d, fstore: %d}}",
                            entry->fields.pool.id, entry->fields.pool.available,
                            entry->fields.pool.privs.fdir,
                            entry->fields.pool.privs.fstore);
                }
            }
            logInfo("%s", buff);
        }

        if (entry->operation == FCFS_AUTH_SESSION_OP_TYPE_CREATE) {
            session.id_info.id = entry->session_id;
            session.fields = &entry->fields;
            server_session_add(&session, publish);
        } else {
            server_session_delete(entry->session_id);
        }
    }
}

static int session_sync(ConnectionInfo *conn)
{
    const int network_timeout = 2;
    int result;
    SFResponseInfo response;

    response.error.length = 0;
    while (SF_G_CONTINUE_FLAG) {
        result = sf_recv_vary_response(conn, &response, network_timeout,
                FCFS_AUTH_SERVICE_PROTO_SESSION_PUSH_REQ, &session_array.
                buffer, sizeof(FCFSAuthProtoSessionPushRespBodyHeader));
        if (result == 0) {
            if ((result=parse_session_push(&response)) != 0) {
                if (response.error.length > 0) {
                    sf_log_network_error(&response, conn, result);
                }
                break;
            }
            deal_session_array();
        } else if (result == ETIMEDOUT) {
            if ((result=sf_active_test(conn, &response,
                            g_fcfs_auth_client_vars.client_ctx.
                            common_cfg.network_timeout)) != 0)
            {
                break;
            }
        } else {
            sf_log_network_error(&response, conn, result);
            break;
        }
    }

    if (SF_G_CONTINUE_FLAG) {
        server_session_clear();
    }

    return result;
}

static void *session_sync_thread_func(void *arg)
{
    SFConnectionManager *cm;
    ConnectionInfo *conn;
    int result;

#ifdef OS_LINUX
    prctl(PR_SET_NAME, "session-sync");
#endif

    cm = &g_fcfs_auth_client_vars.client_ctx.cm;
    while (SF_G_CONTINUE_FLAG) {
        if ((conn=cm->ops.get_master_connection(cm, 0, &result)) == NULL) {
            sleep(1);
            continue;
        }

        if ((result=fcfs_auth_client_proto_session_subscribe(
                        &g_fcfs_auth_client_vars.client_ctx, conn)) == 0)
        {
            session_sync(conn);
        } else if (result == ENOENT) {
            sleep(5);
        } else if (result == EPERM) {
            if ((result=fcfs_auth_load_passwd_ex(g_fcfs_auth_client_vars.
                            client_ctx.auth_cfg.secret_key_filename.str,
                            g_fcfs_auth_client_vars.client_ctx.auth_cfg.
                            passwd_buff, g_fcfs_auth_client_vars.
                            ignore_when_passwd_not_exist)) != 0)
            {
                logError("file: "__FILE__", line: %d, "
                        "secret_key_filename: %s, "
                        "load password fail, ret code: %d", __LINE__,
                        g_fcfs_auth_client_vars.client_ctx.auth_cfg.
                        secret_key_filename.str, result);
                sleep(30);
            }

            sleep(10);
        }

        cm->ops.close_connection(cm, conn);
        sleep(1);
    }

    return NULL;
}

int session_sync_init()
{
    int result;

    if ((result=synced_session_array_init(&session_array)) != 0) {
        return result;
    }

    return 0;
}

int session_sync_start()
{
    pthread_t tid;
    return fc_create_thread(&tid, session_sync_thread_func,
            NULL, SF_G_THREAD_STACK_SIZE);
}
