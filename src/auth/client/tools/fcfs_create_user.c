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
#include "tool_func.h"
#include "../../common/auth_func.h"
#include "../fcfs_auth_client.h"

static void usage(char *argv[])
{
    fprintf(stderr, "\nUsage: %s [-c config_filename="
            "/etc/fastcfs/auth/client.conf]\n"
            "\t[-u admin_username=admin]\n"
            "\t[-k admin_secret_key_filename=/etc/fastcfs/auth/keys/"
            "${admin_username}.key]\n"
            "\t[-p priviledge_list=pool] <username>\n"
            "\t[user_secret_key_filename=keys/${username}.key]\n\n"
            "\tgranted priviledges seperate by comma, priviledges:\n"
            "\t  %s: user management\n"
            "\t  %s: create storage pool\n"
            "\t  %s: monitor cluster\n"
            "\t  %s: for all priviledges\n\n",
            argv[0], USER_PRIV_NAME_USER_MANAGE_STR,
            USER_PRIV_NAME_CREATE_POOL_STR,
            USER_PRIV_NAME_MONITOR_CLUSTER_STR,
            USER_PRIV_NAME_ALL_PRIVS_STR);
}

int main(int argc, char *argv[])
{
    int ch;
    const char *config_filename = "/etc/fastcfs/auth/client.conf";
    FCFSAuthClientUserKeyPair admin;
    FCFSAuthUserInfo user;
    unsigned char passwd_buff[16];
    FilenameString admin_key_filename;
    string_t input_key_filename;
    FilenameString user_key_filename;
    char *filename;
    char abs_path[PATH_MAX];
    string_t passwd;
    string_t privs;
    int result;

    if (argc < 2) {
        usage(argv);
        return 1;
    }

    FC_SET_STRING(admin.username, "admin");
    FC_SET_STRING(admin.key_filename,
            "/etc/fastcfs/auth/keys/${username}.key");
    FC_SET_STRING_NULL(privs);
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
                FC_SET_STRING(privs, optarg);
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

    srand(time(NULL));
    log_init();
    //g_log_context.log_level = LOG_DEBUG;

    FC_SET_STRING(user.name, argv[optind]);
    if (optind + 1 < argc) {
        FC_SET_STRING(input_key_filename, argv[optind + 1]);
    } else {
        FC_SET_STRING(input_key_filename, "keys/${username}.key");
    }

    if (privs.str == NULL) {
        user.priv = FCFS_AUTH_USER_PRIV_CREATE_POOL;
    } else {
        if ((result=fcfs_auth_parse_user_priv(&privs, &user.priv)) != 0) {
            usage(argv);
            return 1;
        }
    }

    passwd.str = (char *)passwd_buff;
    passwd.len = 16;
    fcfs_auth_replace_filename_with_username(&admin.key_filename,
            &admin.username, &admin_key_filename);
    if ((result=fcfs_auth_load_passwd(
                    FC_FILENAME_STRING_PTR(admin_key_filename),
                    passwd_buff)) != 0)
    {
        return result;
    }

    if ((result=fcfs_auth_client_init(config_filename)) != 0) {
        return result;
    }

    fcfs_auth_replace_filename_with_username(&input_key_filename,
            &user.name, &user_key_filename);
    if ((result=fcfs_auth_client_user_login(&g_fcfs_auth_client_vars.
                    client_ctx, &admin.username, &passwd)) != 0)
    {
        return result;
    }

    filename = FC_FILENAME_STRING_PTR(user_key_filename);
    getAbsolutePath(filename, abs_path, sizeof(abs_path));
    if ((result=fc_mkdirs(abs_path, 0755)) != 0) {
        return result;
    }

    fcfs_auth_generate_passwd(passwd_buff);
    FC_SET_STRING_EX(user.passwd, (char *)passwd_buff, FCFS_AUTH_PASSWD_LEN);
    if ((result=fcfs_auth_client_user_create(&g_fcfs_auth_client_vars.
                    client_ctx, &user)) == 0)
    {
        if ((result=fcfs_auth_save_passwd(filename, passwd_buff)) == 0) {
            printf("create user %s success, secret key store to file: %s\n",
                    user.name.str, filename);
        } else {
            char hex_buff[2 * FCFS_AUTH_PASSWD_LEN + 1];

            bin2hex((char *)passwd_buff, FCFS_AUTH_PASSWD_LEN, hex_buff);
            printf("create user %s success, but secret key store to "
                    "file: %s fail, the secret key is:\n%s\n",
                    user.name.str, filename, hex_buff);
        }
    } else {
        printf("create user %s fail\n", user.name.str);
    }
    return result;
}
