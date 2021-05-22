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
#include "sf/sf_cluster_cfg.h"
#include "../client_types.h"

static void usage(char *argv[])
{
    fprintf(stderr, "\nUsage: %s <cluster_config_filename>\n", argv[0]);
}

int main(int argc, char *argv[])
{
    const bool share_between_groups = false;
    const bool calc_sign = false;
    const char *config_filename;
    SFClusterConfig cluster;
    FCServerInfo *server;
    FCServerInfo *end;
    FCAddressInfo **addrs;
    int i;
    int result;

    if (argc < 2) {
        usage(argv);
        return EINVAL;
    }

    config_filename = argv[1];
    log_init();

    if ((result=sf_load_cluster_config_by_file(&cluster, config_filename,
                    FCFS_AUTH_DEFAULT_CLUSTER_PORT, share_between_groups,
                    calc_sign)) != 0)
    {
        return result;
    }

    printf("fauth_group=(");

    end = FC_SID_SERVERS(cluster.server_cfg) +
        FC_SID_SERVER_COUNT(cluster.server_cfg);
    for (server=FC_SID_SERVERS(cluster.server_cfg), i=0;
            server<end; server++, i++)
    {
        addrs = server->group_addrs[cluster.cluster_group_index].
            address_array.addrs;
        if (i > 0) {
            printf(" ");
        }
        printf("%s", (*addrs)->conn.ip_addr);
    }

    printf(")\n");
    return 0;
}
