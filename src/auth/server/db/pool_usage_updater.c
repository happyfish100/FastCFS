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
#include "fastdir/client/fdir_client.h"
#include "../server_global.h"
#include "auth_db.h"
#include "pool_usage_updater.h"

static FDIRClientNamespaceStatArray nss_array;

static int nss_fetch(ConnectionInfo *conn)
{
    int result;
    bool is_last;
    FDIRClientNamespaceStatEntry *entry;
    FDIRClientNamespaceStatEntry *end;

    do {
        if ((result=fdir_client_proto_nss_fetch(&g_fdir_client_vars.
                        client_ctx, conn, &nss_array, &is_last)) != 0)
        {
            break;
        }

        if (nss_array.count == 0) {
            break;
        }

        end = nss_array.entries + nss_array.count;
        for (entry=nss_array.entries; entry<end; entry++) {
            logInfo("%d. ns name: %.*s, used bytes: %.3lf GB",
                    (int)(entry - nss_array.entries + 1),
                    entry->ns_name.len, entry->ns_name.str,
                    (double)entry->used_bytes / (1024 * 1024 * 1024));
            adb_spool_set_used_bytes(&entry->ns_name, entry->used_bytes);
        }

    } while (!is_last);

    return result;
}

static int pool_usage_refresh(ConnectionInfo *conn)
{
    int result;
    const int64_t session_id = 0;

    if ((result=fdir_client_proto_nss_subscribe(&g_fdir_client_vars.
                    client_ctx, conn, session_id)) != 0)
    {
        return result;
    }

    while (SF_G_CONTINUE_FLAG) {
        if ((result=nss_fetch(conn)) != 0) {
            break;
        }

        sleep(POOL_USAGE_REFRESH_INTERVAL);
    }

    return result;
}

static void *pool_usage_refresh_thread_func(void *arg)
{
    SFConnectionManager *cm;
    ConnectionInfo *conn;
    int result;

    cm = &g_fdir_client_vars.client_ctx.cm;
    while (SF_G_CONTINUE_FLAG) {
        if ((conn=cm->ops.get_master_connection(cm, 0, &result)) == NULL) {
            sleep(1);
            continue;
        }

        pool_usage_refresh(conn);
        cm->ops.close_connection(cm, conn);
        sleep(1);
    }

    return NULL;
}

int pool_usage_updater_init()
{
    int result;
    pthread_t tid;

    if ((result=fdir_client_namespace_stat_array_init(&nss_array)) != 0) {
        return result;
    }

    if ((result=fdir_client_simple_init(g_server_global_vars.
                    fdir_client_cfg_filename)) != 0)
    {
        return result;
    }
    return fc_create_thread(&tid, pool_usage_refresh_thread_func,
            NULL, SF_G_THREAD_STACK_SIZE);
}
