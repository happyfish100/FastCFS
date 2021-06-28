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

typedef struct {
    bool inited;
    volatile bool running;
    FDIRClientNamespaceStatArray nss_array;
} PoolUsageUpdaterContext;

static PoolUsageUpdaterContext updater_ctx = {false, false};

#define NSS_ARRAY updater_ctx.nss_array

static int nss_fetch(ConnectionInfo *conn)
{
    int result;
    bool is_last;
    FDIRClientNamespaceStatEntry *entry;
    FDIRClientNamespaceStatEntry *end;

    do {
        if ((result=fdir_client_proto_nss_fetch(&g_fdir_client_vars.
                        client_ctx, conn, &NSS_ARRAY, &is_last)) != 0)
        {
            break;
        }

        if (NSS_ARRAY.count == 0) {
            break;
        }

        end = NSS_ARRAY.entries + NSS_ARRAY.count;
        for (entry=NSS_ARRAY.entries; entry<end; entry++) {
            /*
            logInfo("%d. ns name: %.*s, used bytes: %.3lf GB",
                    (int)(entry - NSS_ARRAY.entries + 1),
                    entry->ns_name.len, entry->ns_name.str,
                    (double)entry->used_bytes / (1024 * 1024 * 1024));
                    */
            adb_spool_set_used_bytes(&entry->ns_name, entry->used_bytes);
        }

    } while (!is_last);

    return result;
}

static int pool_usage_refresh(ConnectionInfo *conn)
{
    int result;

    if ((result=fdir_client_proto_nss_subscribe(&g_fdir_client_vars.
                    client_ctx, conn)) != 0)
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

#ifdef OS_LINUX
    prctl(PR_SET_NAME, "pool-usage-updater");
#endif

    updater_ctx.running = true;
    cm = &g_fdir_client_vars.client_ctx.cm;
    while (SF_G_CONTINUE_FLAG) {
        if ((conn=cm->ops.get_master_connection(cm, 0, &result)) == NULL) {
            sleep(3);
            continue;
        }

        pool_usage_refresh(conn);
        cm->ops.close_connection(cm, conn);
        sleep(1);
    }

    updater_ctx.running = false;
    return NULL;
}

static int pool_usage_updater_init()
{
    int result;

    if ((result=fdir_client_namespace_stat_array_init(&NSS_ARRAY)) != 0) {
        return result;
    }

    if ((result=fdir_client_simple_init(g_server_global_vars.
                    fdir_client_cfg_filename)) != 0)
    {
        return result;
    }
    return 0;
}

int pool_usage_updater_start()
{
    int result;
    int count;
    pthread_t tid;

    if (!updater_ctx.inited) {
        if ((result=pool_usage_updater_init()) != 0) {
            return result;
        }

        updater_ctx.inited = true;
    }

    count = 0;
    while (updater_ctx.running && count++ < 3) {
        sleep(1);
    }

    if (updater_ctx.running) {
        return 0;
    }

    return fc_create_thread(&tid, pool_usage_refresh_thread_func,
            NULL, SF_G_THREAD_STACK_SIZE);
}
