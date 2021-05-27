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
#include "fastcommon/connection_pool.h"
#include "sf/sf_proto.h"

static void usage(char *argv[])
{
    fprintf(stderr, "Usage: %s [options] <host:port>\n"
            "\t-C | --connect-timeout=2: connect timeout in seconds\n"
            "\t-N | --network-timeout=10: network timeout in seconds\n\n",
            argv[0]);
}

int main(int argc, char *argv[])
{
    int ch;
    const struct option longopts[] = {
        {"connect-timeout", required_argument, NULL, 'C'},
        {"network-timeout", required_argument, NULL, 'N'},
        {NULL, 0, NULL, 0}
    };
    char host[256];
    ConnectionInfo conn;
    SFResponseInfo response;
    int connect_timeout;
    int network_timeout;
	int result;

    if (argc < 2) {
        usage(argv);
        return 1;
    }

    log_init();
    //g_log_context.log_level = LOG_DEBUG;

    connect_timeout = 2;
    network_timeout = 10;
    while ((ch=getopt_long(argc, argv, "C:N:", longopts, NULL)) != -1) {
        switch (ch) {
            case 'C':
                connect_timeout = strtol(optarg, NULL, 10);
                break;
            case 'N':
                network_timeout = strtol(optarg, NULL, 10);
                break;
            default:
                usage(argv);
                return 1;
        }
    }

    if (argc - optind >= 2) {
        if (strchr(argv[optind], ':') != NULL) {
            fprintf(stderr, "Error: too many arguments!\n");
            usage(argv);
            return 1;
        }
        snprintf(host, sizeof(host), "%s:%s",
                argv[optind], argv[optind+1]);
    } else if (argc - optind >= 1) {
        if (strchr(argv[optind], ':') == NULL) {
            fprintf(stderr, "Error: expect service port!\n");
            usage(argv);
            return 1;
        }
        snprintf(host, sizeof(host), "%s", argv[optind]);
    } else {
        fprintf(stderr, "Error: expect host!\n");
        usage(argv);
        return 1;
    }

    if ((result=conn_pool_parse_server_info(host, &conn, 0)) != 0) {
        return result;
    }

    if ((result=conn_pool_connect_server(&conn, connect_timeout)) != 0) {
        return result;
    }

    response.error.length = 0;
    if ((result=sf_active_test(&conn, &response, network_timeout)) != 0) {
        sf_log_network_error(&response, &conn, result);
        return result;
    }

    printf("OK\n");
    return 0;
}
