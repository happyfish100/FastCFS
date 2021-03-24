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

#define USERNAME_OPTION_INCLUDED   1
#define USERNAME_OPTION_REQUIRED   2

static int current_index;
static FCFSAuthStoragePoolInfo spool;
static struct {
    string_t fdir;
    string_t fstore;
} privs;
static string_t username = {0};

static void usage(char *argv[])
{
    fprintf(stderr, "\nUsage: %s [-c config_filename="
            "/etc/fastcfs/auth/client.conf]\n"
            "\t[-u admin_username=admin]\n"
            "\t[-k admin_secret_key_filename=/etc/fastcfs/auth/keys/"
            "${admin_username}.key]\n"
            "\t[-d fastdir_access=rw]\n"
            "\t[-s faststore_access=rw]\n"
            "\t<operation> [username] [pool_name] [quota]\n\n"
            "\tthe operations and following parameters: \n"
            "\t  create <pool_name> <quota>\n"
            "\t  quota <pool_name> <quota>\n"
            "\t  grant <username> <pool_name>\n"
            "\t  delete <pool_name>\n"
            "\t  list [username] [pool_name]\n\n"
            "\tthe quota parameter is required for create and quota operations\n"
            "\tthe default unit of quota is GB, %s for no limit\n\n"
            "\tFastDIR and FastStore accesses are:\n"
            "\t  %c:  read only\n"
            "\t  %c:  write only\n"
            "\t  %c%c: read and write\n\n",
            argv[0], FCFS_AUTH_UNLIMITED_QUOTA_STR,
            POOL_ACCESS_NAME_READ_CHR, POOL_ACCESS_NAME_WRITE_CHR,
            POOL_ACCESS_NAME_READ_CHR, POOL_ACCESS_NAME_WRITE_CHR);
}

static int create_spool(int argc, char *argv[])
{
    int result;

    if ((result=fcfs_auth_client_spool_create(&g_fcfs_auth_client_vars.
                    client_ctx, &spool)) == 0)
    {
        printf("create pool %s success\n", spool.name.str);
    } else {
        fprintf(stderr, "create pool %s fail\n", spool.name.str);
    }

    return result;
}

static int quota_spool(int argc, char *argv[])
{
    int result;
    char buff[64];

    if ((result=fcfs_auth_client_spool_set_quota(&g_fcfs_auth_client_vars.
                    client_ctx, &spool.name, spool.quota)) == 0)
    {
        if (spool.quota == FCFS_AUTH_UNLIMITED_QUOTA_VAL) {
            strcpy(buff, FCFS_AUTH_UNLIMITED_QUOTA_STR);
        } else {
            long_to_comma_str(spool.quota / (1024 * 1024 * 1024), buff);
            strcat(buff, "GB");
        }
        printf("pool %s, set quota to %s success\n", spool.name.str, buff);
    } else {
        fprintf(stderr, "pool %s, set quota fail\n", spool.name.str);
    }

    return result;
}

static inline int parse_pool_access(const string_t *privs, int *pv)
{
    if (privs->str == NULL) {
        *pv = FCFS_AUTH_POOL_ACCESS_ALL;
        return 0;
    } else {
        return fcfs_auth_parse_pool_access(privs, pv);
    }
}

static int grant_privilege(int argc, char *argv[])
{
    int result;
    FCFSAuthStoragePoolGranted granted;

    granted.pool = spool;
    if ((result=parse_pool_access(&privs.fdir,
                    &granted.privs.fdir)) != 0)
    {
        usage(argv);
        return result;
    }

    if ((result=parse_pool_access(&privs.fstore,
                    &granted.privs.fstore)) != 0)
    {
        usage(argv);
        return result;
    }

    /*
    if ((result=fcfs_auth_client_spool_grant(&g_fcfs_auth_client_vars.
                    client_ctx, &spool.name, spool.priv)) == 0)
    {
        printf("grant priviledge success\n");
    } else {
        fprintf(stderr, "grant priviledge fail\n");
    }
    */

    return result;
}

static int remove_spool(int argc, char *argv[])
{
    int result;

    if ((result=fcfs_auth_client_spool_remove(&g_fcfs_auth_client_vars.
                    client_ctx, &spool.name)) == 0)
    {
        printf("remove pool %s success\n", spool.name.str);
    } else {
        fprintf(stderr, "remove pool %s fail\n", spool.name.str);
    }

    return result;
}

static void output_spools(FCFSAuthStoragePoolArray *array)
{
    FCFSAuthStoragePoolInfo *spool;
    FCFSAuthStoragePoolInfo *end;
    char buff[64];

    printf("%5s %32s %32s\n", "No.", "pool_name", "quota (GB)");
    end = array->spools + array->count;
    for (spool=array->spools; spool<end; spool++) {
        if (spool->quota == FCFS_AUTH_UNLIMITED_QUOTA_VAL) {
            strcpy(buff, FCFS_AUTH_UNLIMITED_QUOTA_STR);
        } else {
            long_to_comma_str(spool->quota / (1024 * 1024 * 1024), buff);
        }
        printf("%4d. %32.*s %32s\n", (int)((spool - array->spools) + 1),
                spool->name.len, spool->name.str, buff);
    }
}

/*
static void output_spools(FCFSAuthStoragePoolArray *array)
{
    FCFSAuthStoragePoolInfo *spool;
    FCFSAuthStoragePoolInfo *end;
    char buff1[64];
    char buff2[64];
    string_t priv_names_fdir;
    string_t priv_names_fstore;

    printf("%5s %32s %16s %16s\n", "No.", "pool_name",
            "fdir_priv", "fstore_priv");
    priv_names_fdir.str = buff1;
    priv_names_fstore.str = buff2;
    end = array->spools + array->count;
    for (spool=array->spools; spool<end; spool++) {
        printf("%4d. %32.*s %16s %16s\n", (int)((spool - array->spools) + 1),
                spool->name.len, spool->name.str,
                fcfs_auth_pool_access_to_string(spool->privs.fdir,
                    &priv_fdir_names),
                fcfs_auth_pool_access_to_string(spool->privs.fstore,
                    &priv_fstore_names));
    }
}
*/

static int list_spool(int argc, char *argv[])
{
    SFProtoRecvBuffer buffer;
    FCFSAuthStoragePoolArray spool_array;
    int result;

    sf_init_recv_buffer(&buffer);
    fcfs_auth_spool_init_array(&spool_array);

    if ((result=fcfs_auth_client_spool_list(&g_fcfs_auth_client_vars.
                    client_ctx, &username, &spool.name, &buffer,
                    &spool_array)) == 0)
    {
        output_spools(&spool_array);
    } else {
        fprintf(stderr, "list storage pool fail\n");
    }
    result = 0;

    sf_free_recv_buffer(&buffer);
    fcfs_auth_spool_free_array(&spool_array);
    return result;
}

int main(int argc, char *argv[])
{
    int ch;
    const char *config_filename = "/etc/fastcfs/auth/client.conf";
    char *operation;
    FCFSAuthClientUserKeyPair admin;
    unsigned char passwd_buff[16];
    FilenameString admin_key_filename;
    string_t passwd;
    int username_options;
    bool need_poolname;
    bool need_quota;
    int result;

    if (argc < 2) {
        usage(argv);
        return 1;
    }

    FC_SET_STRING(admin.username, "admin");
    FC_SET_STRING(admin.key_filename,
            "/etc/fastcfs/auth/keys/${username}.key");
    FC_SET_STRING_NULL(privs.fdir);
    FC_SET_STRING_NULL(privs.fstore);
    while ((ch=getopt(argc, argv, "hc:u:k:d:s:")) != -1) {
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
            case 'd':
                FC_SET_STRING(privs.fdir, optarg);
                break;
            case 's':
                FC_SET_STRING(privs.fstore, optarg);
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
        username_options = 0;
        need_poolname = true;
        need_quota = true;
    } else if (strcasecmp(operation, "grant") == 0) {
        username_options = USERNAME_OPTION_INCLUDED |
            USERNAME_OPTION_REQUIRED;
        need_poolname = true;
        need_quota = false;
    } else if (strcasecmp(operation, "quota") == 0) {
        username_options = 0;
        need_poolname = true;
        need_quota = true;
    } else if (strcasecmp(operation, "delete") == 0 ||
            strcasecmp(operation, "remove") == 0)
    {
        username_options = 0;
        need_poolname = true;
        need_quota = false;
    } else if (strcasecmp(operation, "list") == 0) {
        username_options = USERNAME_OPTION_INCLUDED;
        need_poolname = false;
        need_quota = false;
    } else {
        fprintf(stderr, "unknow operation: %s\n", operation);
        usage(argv);
        return 1;
    }

    if ((username_options & USERNAME_OPTION_INCLUDED) != 0) {
        if (current_index < argc) {
            FC_SET_STRING(username, argv[current_index]);
            current_index++;
        } else if ((username_options & USERNAME_OPTION_REQUIRED) != 0) {
            fprintf(stderr, "expect username\n");
            usage(argv);
            return 1;
        }
    }

    if (current_index < argc) {
        FC_SET_STRING(spool.name, argv[current_index]);
        current_index++;
    } else if (need_poolname) {
        fprintf(stderr, "expect pool name\n");
        usage(argv);
        return 1;
    } else {
        FC_SET_STRING_NULL(spool.name);
    }

    if (current_index < argc) {
        if (strcasecmp(argv[current_index],
                    FCFS_AUTH_UNLIMITED_QUOTA_STR) == 0)
        {
            spool.quota = FCFS_AUTH_UNLIMITED_QUOTA_VAL;
        } else if ((result=parse_bytes(argv[current_index],
                        1024 * 1024 * 1024, &spool.quota)) != 0)
        {
            fprintf(stderr, "parse quota fail\n");
            return 1;
        }
        current_index++;
    } else if (need_quota) {
        fprintf(stderr, "expect quota\n");
        usage(argv);
        return 1;
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

    if ((result=fcfs_auth_client_user_login(&g_fcfs_auth_client_vars.
                    client_ctx, &admin.username, &passwd)) != 0)
    {
        return result;
    }

    if (strcasecmp(operation, "create") == 0) {
        return create_spool(argc, argv);
    } else if (strcasecmp(operation, "quota") == 0) {
        return quota_spool(argc, argv);
    } else if (strcasecmp(operation, "grant") == 0) {
        return grant_privilege(argc, argv);
    } else if (strcasecmp(operation, "delete") == 0 ||
            strcasecmp(operation, "remove") == 0)
    {
        return remove_spool(argc, argv);
    } else if (strcasecmp(operation, "list") == 0) {
        return list_spool(argc, argv);
    }

    return 0;
}
