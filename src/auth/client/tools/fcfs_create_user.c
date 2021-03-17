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
#include "fastcfs/auth/fcfs_auth_client.h"

static void usage(char *argv[])
{
    fprintf(stderr, "Usage: %s [-c config_filename="
            "/etc/fastcfs/auth/client.conf]\n"
            "\t[-u admin_username=admin]\n"
            "\t[-k admin_secret_key_filename=/etc/fastcfs/auth/keys/"
            "${admin_username}.key]\n"
            "\t[-p priviledge_list=pool] <username>\n"
            "\t[user_secret_key_filename=keys/${username}.key]\n\n"
            "\tgranted priviledges seperate by comma, priviledges:\n"
            "\t  user: user management\n"
            "\t  pool: storage pool management\n"
            "\t  cluster: cluster monitor\n"
            "\t  *: for all priviledges\n\n",
            argv[0]);
}

int main(int argc, char *argv[])
{
	int ch;
    const char *config_filename = "/etc/fastcfs/auth/client.conf";
    FCFSAuthClientUserKeyPair admin;
    FCFSAuthClientUserKeyPair newuser;
    char buff[64];
    string_t passwd;
    char *priv;
	int result;

    if (argc < 2) {
        usage(argv);
        return 1;
    }

    FC_SET_STRING(admin.username, "admin");
    FC_SET_STRING_NULL(admin.key_filename);
    priv = NULL;
    while ((ch=getopt(argc, argv, "hc:u:k:p:")) != -1) {
        switch (ch) {
            case 'h':
                usage(argv);
                break;
            case 'c':
                config_filename = optarg;
                break;
            case 'u':
                FC_SET_STRING_EX(admin.username, optarg, strlen(optarg));
                break;
            case 'k':
                FC_SET_STRING_EX(admin.key_filename, optarg, strlen(optarg));
                break;
            case 'p':
                priv = optarg;
                break;
            default:
                usage(argv);
                return 1;
        }
    }

    if (optind >= argc) {
        usage(argv);
        return 1;
    }

    log_init();
    //g_log_context.log_level = LOG_DEBUG;

    FC_SET_STRING(newuser.username, argv[optind]);
    if (optind + 1 < argc) {
        FC_SET_STRING(newuser.key_filename, argv[optind + 1]);
    }

    //TODO

    if ((result=fcfs_auth_client_init(config_filename)) != 0) {
        return result;
    }

    //TODO
    passwd.str = buff;
    passwd.len = 16;
    if ((result=fcfs_auth_client_user_login(&g_fcfs_auth_client_vars.
                    client_ctx, &admin.username, &passwd)) != 0)
    {
        return result;
    }

    /*
       fcfs_auth_generate_passwd();

    int fcfs_auth_client_user_create(&g_fcfs_auth_client_vars.client_ctx,
        const FCFSAuthUserInfo *user);
        */

    return 0;
}
