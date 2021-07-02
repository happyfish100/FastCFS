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
#include "fastcommon/fc_atomic.h"
#include "fastdir/client/fdir_client.h"
#include "../server_global.h"
#include "auth_db.h"
#include "pool_usage_updater.h"

typedef struct {
    bool inited;
    volatile char running;
    struct {
        volatile uint32_t current;
        volatile uint32_t next;
    } generation;
    FDIRClientNamespaceStatArray nss_array;
} PoolUsageUpdaterContext;

static PoolUsageUpdaterContext updater_ctx = {false, 0, {0, 0}};

#define NSS_ARRAY updater_ctx.nss_array

#define IS_SAME_GENERATION (updater_ctx.generation.current == \
        updater_ctx.generation.next)

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

    } while (!is_last && IS_SAME_GENERATION);

    return result;
}

static int pool_usage_refresh(ConnectionInfo *conn)
{
    int i;
    int result;

    if ((result=fdir_client_proto_nss_subscribe(&g_fdir_client_vars.
                    client_ctx, conn)) != 0)
    {
        return result;
    }

    while (SF_G_CONTINUE_FLAG && IS_SAME_GENERATION) {
        if ((result=nss_fetch(conn)) != 0) {
            break;
        }

        for (i=0; i<POOL_USAGE_REFRESH_INTERVAL &&
                IS_SAME_GENERATION; i++)
        {
            sleep(1);
        }
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

    FC_ATOMIC_SET(updater_ctx.running, 1);
    cm = &g_fdir_client_vars.client_ctx.cm;
    while (SF_G_CONTINUE_FLAG) {
        if ((conn=cm->ops.get_master_connection(cm, 0, &result)) == NULL) {
            sleep(1);
            continue;
        }

        pool_usage_refresh(conn);
        cm->ops.close_connection(cm, conn);
        if (IS_SAME_GENERATION) {
            sleep(1);
        } else {
            break;
        }
    }

    FC_ATOMIC_SET(updater_ctx.running, 0);
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
    pthread_t tid;

    if (!updater_ctx.inited) {
        if ((result=pool_usage_updater_init()) != 0) {
            return result;
        }

        updater_ctx.inited = true;
    }

    if (FC_ATOMIC_GET(updater_ctx.running) != 0) {
        logWarning("file: "__FILE__", line: %d, "
                "pool_usage_updater thread already running",
                __LINE__);
        return 0;
    }

    updater_ctx.generation.current = ++updater_ctx.generation.next;
    return fc_create_thread(&tid, pool_usage_refresh_thread_func,
            NULL, SF_G_THREAD_STACK_SIZE);
}

void pool_usage_updater_terminate()
{
    int count;

    if (FC_ATOMIC_GET(updater_ctx.running) == 0) {
        return;
    }

    count = 0;
    ++updater_ctx.generation.next;
    while (FC_ATOMIC_GET(updater_ctx.running) != 0) {
        sleep(1);
        count++;
    }

    logInfo("file: "__FILE__", line: %d, "
            "pool_usage_updater thread exit after waiting count: %d",
            __LINE__, count);
}
