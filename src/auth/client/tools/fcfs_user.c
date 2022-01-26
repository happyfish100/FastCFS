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

const int mpool_alloc_size_once = 16 * 1024;
const int mpool_discard_size = 8;
const SFListLimitInfo limit = {0, 0};

static int current_index;
static FCFSAuthUserInfo user;
static bool priv_set = false;
static bool confirmed = false;  //for option -y

static void usage(char *argv[])
{
    fprintf(stderr, "\nUsage: %s [-c config_filename=%s]\n"
            "\t[-u admin_username=admin]\n"
            "\t[-k admin_secret_key_filename=/etc/fastcfs/auth/keys/"
            "${username}.key]\n"
            "\t[-p priviledges=%s]\n"
            "\t<operation> [username]\n"
            "\t[user_secret_key_filename=keys/${username}.key]\n\n"
            "\tthe operations and parameters are: \n"
            "\t  create <username> [user_secret_key_filename]\n"
            "\t  passwd | secret-key <username> [user_secret_key_filename] "
            "[-y]: regenerate user's secret key\n"
            "\t  grant <username>, the option <-p priviledges> is required\n"
            "\t  delete | remove <username>\n"
            "\t  list [username]\n\n"
            "\t[user_secret_key_filename]: specify the filename to store the "
            "generated secret key of the user\n"
            "\t[priviledges]: the granted priviledges seperate by comma, "
            "priviledges:\n"
            "\t  %s: user management\n"
            "\t  %s: create storage pool\n"
            "\t  %s: monitor cluster\n"
            "\t  %s: subscribe session for FastDIR and FastStore server side\n"
            "\t  %s: for all priviledges\n\n",
            argv[0], FCFS_AUTH_CLIENT_DEFAULT_CONFIG_FILENAME,
            USER_PRIV_NAME_CREATE_POOL_STR,
            USER_PRIV_NAME_USER_MANAGE_STR,
            USER_PRIV_NAME_CREATE_POOL_STR,
            USER_PRIV_NAME_MONITOR_CLUSTER_STR,
            USER_PRIV_NAME_SUBSCRIBE_SESSION_STR,
            USER_PRIV_NAME_ALL_PRIVS_STR);
}

static int user_create_or_passwd(int argc, char *argv[], const int cmd)
{
    string_t input_key_filename;
    FilenameString user_key_filename;
    unsigned char passwd_buff[FCFS_AUTH_PASSWD_LEN + 1];
    const char *caption;
    char *filename;
    char abs_path[PATH_MAX];
    int result;

    if (current_index < argc) {
        FC_SET_STRING(input_key_filename, argv[current_index]);
    } else {
        FC_SET_STRING(input_key_filename, "keys/${username}.key");
    }

    fcfs_auth_replace_filename_with_username(&input_key_filename,
            &user.name, &user_key_filename);

    filename = FC_FILENAME_STRING_PTR(user_key_filename);
    getAbsolutePath(filename, abs_path, sizeof(abs_path));
    if ((result=fc_mkdirs(abs_path, 0755)) != 0) {
        return result;
    }

    fcfs_auth_generate_passwd(passwd_buff);
    FC_SET_STRING_EX(user.passwd, (char *)passwd_buff, FCFS_AUTH_PASSWD_LEN);

    if (cmd == FCFS_AUTH_SERVICE_PROTO_USER_CREATE_REQ) {
        caption = "create user";
        result = fcfs_auth_client_user_create(&g_fcfs_auth_client_vars.
                client_ctx, &user);
    } else {
        caption = "regenerate secret key for user";
        result = fcfs_auth_client_user_passwd(&g_fcfs_auth_client_vars.
                client_ctx, &user.name, &user.passwd);
    }
    if (result == 0) {
        if ((result=fcfs_auth_save_passwd(filename, passwd_buff)) == 0) {
            printf("%s %s success, secret key store to file: %s\n",
                    caption, user.name.str, filename);
        } else {
            char hex_buff[2 * FCFS_AUTH_PASSWD_LEN + 1];

            bin2hex((char *)passwd_buff, FCFS_AUTH_PASSWD_LEN, hex_buff);
            printf("%s %s success, but secret key store to "
                    "file: %s fail, the secret key is:\n%s\n",
                    caption, user.name.str, filename, hex_buff);
        }
    } else {
        fprintf(stderr, "%s %s fail\n", caption, user.name.str);
    }

    return result;
}

static inline int create_user(int argc, char *argv[])
{
    return user_create_or_passwd(argc, argv,
            FCFS_AUTH_SERVICE_PROTO_USER_CREATE_REQ);
}

static inline int passwd_user(int argc, char *argv[])
{
    return user_create_or_passwd(argc, argv,
            FCFS_AUTH_SERVICE_PROTO_USER_PASSWD_REQ);
}

static int grant_privilege(int argc, char *argv[])
{
    int result;

    if (!priv_set) {
        fprintf(stderr, "expect parameter: granted priviledges!\n");
        usage(argv);
        return EINVAL;
    }

    if ((result=fcfs_auth_client_user_grant(&g_fcfs_auth_client_vars.
                    client_ctx, &user.name, user.priv)) == 0)
    {
        printf("grant user priviledge success\n");
    } else {
        fprintf(stderr, "grant user priviledge fail\n");
    }

    return result;
}

static int remove_user(int argc, char *argv[])
{
    int result;

    if ((result=fcfs_auth_client_user_remove(&g_fcfs_auth_client_vars.
                    client_ctx, &user.name)) == 0)
    {
        printf("remove user %s success\n", user.name.str);
    } else {
        fprintf(stderr, "remove user %s fail\n", user.name.str);
    }

    return result;
}

static void output_users(FCFSAuthUserArray *array)
{
    FCFSAuthUserInfo *user;
    FCFSAuthUserInfo *end;
    char buff[256];
    string_t priv_names;

    printf("%5s %32s %32s\n", "No.", "username", "priviledges");
    priv_names.str = buff;
    end = array->users + array->count;
    for (user=array->users; user<end; user++) {
        printf("%4d. %32.*s %32s\n", (int)((user - array->users) + 1),
                user->name.len, user->name.str,
                fcfs_auth_user_priv_to_string(user->priv, &priv_names));
    }
}

static int list_user(int argc, char *argv[])
{
    struct fast_mpool_man mpool;
    FCFSAuthUserArray user_array;
    int result;

    if ((result=fast_mpool_init(&mpool, mpool_alloc_size_once,
                    mpool_discard_size)) != 0)
    {
        return result;
    }

    fcfs_auth_user_init_array(&user_array);
    if ((result=fcfs_auth_client_user_list(&g_fcfs_auth_client_vars.
                    client_ctx, &user.name, &limit, &mpool,
                    &user_array)) == 0)
    {
        output_users(&user_array);
    } else {
        fprintf(stderr, "list user fail\n");
    }

    fcfs_auth_user_free_array(&user_array);
    fast_mpool_destroy(&mpool);
    return result;
}

int main(int argc, char *argv[])
{
    int ch;
    const char *config_filename = FCFS_AUTH_CLIENT_DEFAULT_CONFIG_FILENAME;
    char *operation;
    FCFSAuthClientUserKeyPair admin;
    unsigned char passwd_buff[FCFS_AUTH_PASSWD_LEN + 1];
    FilenameString admin_key_filename;
    string_t passwd;
    string_t privs;
    bool need_username;
    int result;

    if (argc < 2) {
        usage(argv);
        return 1;
    }

    FC_SET_STRING(admin.username, "admin");
    FC_SET_STRING(admin.key_filename,
            "/etc/fastcfs/auth/keys/${username}.key");
    FC_SET_STRING_NULL(privs);
    while ((ch=getopt(argc, argv, "hc:u:k:p:y")) != -1) {
        switch (ch) {
            case 'h':
                usage(argv);
                return 0;
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
            case 'y':
                confirmed = true;
                break;
            default:
                usage(argv);
                return 1;
        }
    }

    current_index = optind;
    if (current_index >= argc) {
        fprintf(stderr, "expect operation\n");
        usage(argv);
        return 1;
    }

    srand(time(NULL));
    log_init();
    //g_log_context.log_level = LOG_DEBUG;

    operation = argv[current_index++];
    if (strcasecmp(operation, "create") == 0) {
        need_username = true;
    } else if (strcasecmp(operation, "passwd") == 0 ||
            strcasecmp(operation, "secret-key") == 0)
    {
        need_username = true;
    } else if (strcasecmp(operation, "grant") == 0) {
        need_username = true;
    } else if (strcasecmp(operation, "delete") == 0 ||
            strcasecmp(operation, "remove") == 0)
    {
        need_username = true;
    } else if (strcasecmp(operation, "list") == 0) {
        need_username = false;
    } else {
        fprintf(stderr, "unknow operation: %s\n", operation);
        usage(argv);
        return 1;
    }

    if (current_index < argc) {
        FC_SET_STRING(user.name, argv[current_index]);
        current_index++;
    } else if (need_username) {
        fprintf(stderr, "expect username\n");
        usage(argv);
        return 1;
    } else {
        FC_SET_STRING_NULL(user.name);
    }

    if (privs.str == NULL) {
        user.priv = FCFS_AUTH_USER_PRIV_CREATE_POOL;
    } else {
        if ((result=fcfs_auth_parse_user_priv(&privs, &user.priv)) != 0) {
            usage(argv);
            return 1;
        }
        priv_set = true;
    }

    passwd.str = (char *)passwd_buff;
    passwd.len = FCFS_AUTH_PASSWD_LEN;
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

    if ((result=fcfs_auth_client_user_login(&g_fcfs_auth_client_vars.
                    client_ctx, &admin.username, &passwd)) != 0)
    {
        return result;
    }

    if (strcasecmp(operation, "create") == 0) {
        return create_user(argc, argv);
    } else if (strcasecmp(operation, "passwd") == 0 ||
            strcasecmp(operation, "secret-key") == 0)
    {
        if (fc_string_equal(&admin.username, &user.name) && !confirmed) {
            fprintf(stderr, "you MUST use -y option to regenerate "
                    "the secret key \nwhen the user is same with "
                    "the admin user!\n\n");
            return EAGAIN;
        }
        return passwd_user(argc, argv);
    } else if (strcasecmp(operation, "grant") == 0) {
        return grant_privilege(argc, argv);
    } else if (strcasecmp(operation, "delete") == 0 ||
            strcasecmp(operation, "remove") == 0)
    {
        return remove_user(argc, argv);
    } else if (strcasecmp(operation, "list") == 0) {
        return list_user(argc, argv);
    }

    return 0;
}
