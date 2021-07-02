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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "fastcommon/logger.h"
#include "../fcfs_auth_client.h"

static void usage(char *argv[])
{
    fprintf(stderr, "Usage: %s [-c config_filename=%s]\n",
            argv[0], FCFS_AUTH_CLIENT_DEFAULT_CONFIG_FILENAME);
}

static void output(FCFSAuthClientClusterStatEntry *stats, const int count)
{
    FCFSAuthClientClusterStatEntry *stat;
    FCFSAuthClientClusterStatEntry *end;

    end = stats + count;
    for (stat=stats; stat<end; stat++) {
        printf( "server_id: %d, host: %s:%u, "
                "is_online: %d, is_master: %d\n",
                stat->server_id, stat->ip_addr, stat->port,
                stat->is_online, stat->is_master);
    }
    printf("\nserver count: %d\n\n", count);
}

int main(int argc, char *argv[])
{
#define CLUSTER_MAX_SERVER_COUNT  16
	int ch;
    const char *config_filename = FCFS_AUTH_CLIENT_DEFAULT_CONFIG_FILENAME;
    int count;
    FCFSAuthClientClusterStatEntry stats[CLUSTER_MAX_SERVER_COUNT];
	int result;

    while ((ch=getopt(argc, argv, "hc:")) != -1) {
        switch (ch) {
            case 'h':
                usage(argv);
                break;
            case 'c':
                config_filename = optarg;
                break;
            default:
                usage(argv);
                return 1;
        }
    }

    log_init();
    //g_log_context.log_level = LOG_DEBUG;

    if ((result=fcfs_auth_client_init(config_filename)) != 0) {
        return result;
    }

    if ((result=fcfs_auth_client_cluster_stat(&g_fcfs_auth_client_vars.client_ctx,
                    stats, CLUSTER_MAX_SERVER_COUNT, &count)) != 0)
    {
        fprintf(stderr, "fcfs_auth_client_cluster_stat fail, "
                "errno: %d, error info: %s\n", result, STRERROR(result));
        return result;
    }

    output(stats, count);
    return 0;
}
