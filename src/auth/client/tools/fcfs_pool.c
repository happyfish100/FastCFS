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

#define PARAM_OPTION_INCLUDED   1
#define PARAM_OPTION_REQUIRED   2

#define DRY_RUN_OPTION_STR  "--dryrun"

const int mpool_alloc_size_once = 16 * 1024;
const int mpool_discard_size = 8;
const SFListLimitInfo limit = {0, 0};

static int current_index;
static FCFSAuthStoragePoolInfo spool;
static struct {
    string_t fdir;
    string_t fstore;
} privs;
static string_t username = {0};
static bool dryrun;

static void usage(char *argv[])
{
    fprintf(stderr, "\nUsage: %s [-c config_filename=%s]\n"
            "\t[-u admin_username=admin]\n"
            "\t[-k admin_secret_key_filename=/etc/fastcfs/auth/keys/"
            "${admin_username}.key]\n"
            "\t[-d fastdir_access=rw]\n"
            "\t[-s faststore_access=rw]\n"
            "\t<operation> [username] [pool_name] [quota]\n\n"
            "\tthe operations and following parameters: \n"
            "\t  create [pool_name] <quota> [%s]\n"
            "\t  quota <pool_name> <quota>\n"
            "\t  delete | remove <pool_name>\n"
            "\t  plist | pool-list [username] [pool_name]\n"
            "\t  grant <username> <pool_name>\n"
            "\t  cancel | withdraw <username> <pool_name>\n"
            "\t  glist | grant-list | granted-list [username] [pool_name]\n\n"
            "\t* the pool name can contain %s for auto generated id when "
            "create pool, such as 'pool-%s'\n"
            "\t  the pool name template configurated in the server side "
            "will be used when not specify the pool name\n"
            "\t  you can set the initial value of auto increment id and "
            "the pool name template in server.conf of the server side\n\n"
            "\t* the quota parameter is required for create and quota operations\n"
            "\t  the default unit of quota is GiB, %s for no limit\n\n"
            "\tFastDIR and FastStore accesses are:\n"
            "\t  %c:  read only\n"
            "\t  %c:  write only\n"
            "\t  %c%c: read and write\n\n",
            argv[0], FCFS_AUTH_CLIENT_DEFAULT_CONFIG_FILENAME,
            DRY_RUN_OPTION_STR, FCFS_AUTH_AUTO_ID_TAG_STR,
            FCFS_AUTH_AUTO_ID_TAG_STR, FCFS_AUTH_UNLIMITED_QUOTA_STR,
            POOL_ACCESS_NAME_READ_CHR, POOL_ACCESS_NAME_WRITE_CHR,
            POOL_ACCESS_NAME_READ_CHR, POOL_ACCESS_NAME_WRITE_CHR);
}

static int create_spool(int argc, char *argv[])
{
    int result;
    FILE *fp;
    char name_buff[NAME_MAX];
    char prompt[32];

    if (spool.name.len > sizeof(name_buff)) {
        fprintf(stderr, "pool name length: %d is too large, exceeds %d",
                spool.name.len, (int)sizeof(name_buff));
        return ENAMETOOLONG;
    }

    memcpy(name_buff, spool.name.str, spool.name.len);
    spool.name.str = name_buff;
    if ((result=fcfs_auth_client_spool_create(&g_fcfs_auth_client_vars.
                    client_ctx, &spool, sizeof(name_buff), dryrun)) == 0)
    {
        strcpy(prompt, "success");
        fp = stdout;
    } else {
        strcpy(prompt, "fail");
        fp = stderr;
    }

    fprintf(fp, "%screate pool %.*s %s\n", (dryrun ? "[DRYRUN] " : ""),
            spool.name.len, spool.name.str, prompt);
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
            strcat(buff, "GiB");
        }
        printf("pool %s, set quota to %s success\n", spool.name.str, buff);
    } else {
        fprintf(stderr, "pool %s, set quota fail\n", spool.name.str);
    }

    return result;
}

static inline int parse_pool_access(const string_t *privs,
        const char *caption, int *pv)
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
    FILE *fp;
    char prompt[32];
    FCFSAuthSPoolPriviledges pvs;

    if ((result=parse_pool_access(&privs.fdir, "FastDIR", &pvs.fdir)) != 0) {
        usage(argv);
        return result;
    }

    if ((result=parse_pool_access(&privs.fstore, "FastStore",
                    &pvs.fstore)) != 0)
    {
        usage(argv);
        return result;
    }

    if ((pvs.fdir == FCFS_AUTH_POOL_ACCESS_NONE) && 
            (pvs.fstore == FCFS_AUTH_POOL_ACCESS_NONE))
    {
        fprintf(stderr, "no access priviledges to grant\n");
        usage(argv);
        return ENOENT;
    }

    if ((result=fcfs_auth_client_gpool_grant(&g_fcfs_auth_client_vars.
                    client_ctx, &username, &spool.name, &pvs)) == 0)
    {
        strcpy(prompt, "success");
        fp = stdout;
    } else {
        strcpy(prompt, "fail");
        fp = stderr;
    }

    fprintf(fp, "grant access priviledge of pool %.*s to user %.*s %s\n",
            spool.name.len, spool.name.str, username.len, username.str, prompt);
    return result;
}

static int withdraw_privilege(int argc, char *argv[])
{
    int result;
    FILE *fp;
    char prompt[32];

    if ((result=fcfs_auth_client_gpool_withdraw(&g_fcfs_auth_client_vars.
                    client_ctx, &username, &spool.name)) == 0)
    {
        strcpy(prompt, "success");
        fp = stdout;
    } else {
        strcpy(prompt, "fail");
        fp = stderr;
    }

    fprintf(fp, "withdraw access priviledge of pool %.*s from user %.*s %s\n",
            spool.name.len, spool.name.str, username.len, username.str, prompt);
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

    printf("%5s %50s %16s\n", "No.", "pool_name", "quota (GiB)");
    end = array->spools + array->count;
    for (spool=array->spools; spool<end; spool++) {
        if (spool->quota == FCFS_AUTH_UNLIMITED_QUOTA_VAL) {
            strcpy(buff, FCFS_AUTH_UNLIMITED_QUOTA_STR);
        } else {
            long_to_comma_str(spool->quota / (1024 * 1024 * 1024), buff);
        }
        printf("%4d. %50.*s %16s\n", (int)((spool - array->spools) + 1),
                spool->name.len, spool->name.str, buff);
    }
}

static void output_gpools(FCFSAuthGrantedPoolArray *array)
{
    FCFSAuthGrantedPoolFullInfo *gpool;
    FCFSAuthGrantedPoolFullInfo *end;
    char buff1[64];
    char buff2[64];
    string_t priv_names_fdir;
    string_t priv_names_fstore;

    printf("%5s %32s %50s %16s %16s\n", "No.", "owner_name", "pool_name",
            "fdir_priv", "fstore_priv");
    priv_names_fdir.str = buff1;
    priv_names_fstore.str = buff2;
    end = array->gpools + array->count;
    for (gpool=array->gpools; gpool<end; gpool++) {
        printf("%4d. %32.*s %50.*s %16s %16s\n",
                (int)((gpool - array->gpools) + 1),
                gpool->username.len, gpool->username.str,
                gpool->pool_name.len, gpool->pool_name.str,
                fcfs_auth_pool_access_to_string(gpool->
                    granted.privs.fdir, &priv_names_fdir),
                fcfs_auth_pool_access_to_string(gpool->
                    granted.privs.fstore, &priv_names_fstore));
    }
}

static int list_spool(int argc, char *argv[])
{
    struct fast_mpool_man mpool;
    FCFSAuthStoragePoolArray spool_array;
    int result;

    if ((result=fast_mpool_init(&mpool, mpool_alloc_size_once,
                    mpool_discard_size)) != 0)
    {
        return result;
    }

    fcfs_auth_spool_init_array(&spool_array);
    if ((result=fcfs_auth_client_spool_list(&g_fcfs_auth_client_vars.
                    client_ctx, &username, &spool.name, &limit,
                    &mpool, &spool_array)) == 0)
    {
        output_spools(&spool_array);
    } else {
        fprintf(stderr, "list storage pool fail\n");
    }

    fcfs_auth_spool_free_array(&spool_array);
    fast_mpool_destroy(&mpool);
    return result;
}

static int list_gpool(int argc, char *argv[])
{
    struct fast_mpool_man mpool;
    FCFSAuthGrantedPoolArray gpool_array;
    int result;

    if ((result=fast_mpool_init(&mpool, mpool_alloc_size_once,
                    mpool_discard_size)) != 0)
    {
        return result;
    }

    fcfs_auth_granted_init_array(&gpool_array);
    if ((result=fcfs_auth_client_gpool_list(&g_fcfs_auth_client_vars.
                    client_ctx, &username, &spool.name, &limit,
                    &mpool, &gpool_array)) == 0)
    {
        output_gpools(&gpool_array);
    } else {
        fprintf(stderr, "list storage pool fail\n");
    }

    fcfs_auth_granted_free_array(&gpool_array);
    fast_mpool_destroy(&mpool);
    return result;
}

int main(int argc, char *argv[])
{
    int ch;
    int op_type;
    const char *config_filename = FCFS_AUTH_CLIENT_DEFAULT_CONFIG_FILENAME;
    char *operation;
    FCFSAuthClientUserKeyPair admin;
    unsigned char passwd_buff[FCFS_AUTH_PASSWD_LEN + 1];
    FilenameString admin_key_filename;
    string_t passwd;
    int username_options;
    int poolname_options;
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
    if (current_index < argc) {
        char *last;
        last = argv[argc - 1];
        dryrun = (strcasecmp(last, DRY_RUN_OPTION_STR) == 0);
        if (dryrun) {
            --argc;  //remove the last parameter
        }
    }

    if (strcasecmp(operation, "create") == 0) {
        op_type = FCFS_AUTH_SERVICE_PROTO_SPOOL_CREATE_REQ;
        username_options = 0;
        if (current_index + 1 < argc) {
            poolname_options = PARAM_OPTION_INCLUDED |
                PARAM_OPTION_REQUIRED;
        } else {
            poolname_options = 0;
        }
        need_quota = true;
    } else if (strcasecmp(operation, "grant") == 0) {
        op_type = FCFS_AUTH_SERVICE_PROTO_GPOOL_GRANT_REQ;
        username_options = PARAM_OPTION_INCLUDED |
            PARAM_OPTION_REQUIRED;
        poolname_options = PARAM_OPTION_INCLUDED |
            PARAM_OPTION_REQUIRED;
        need_quota = false;
    } else if (strcasecmp(operation, "quota") == 0) {
        op_type = FCFS_AUTH_SERVICE_PROTO_SPOOL_SET_QUOTA_REQ;
        username_options = 0;
        poolname_options = PARAM_OPTION_INCLUDED |
            PARAM_OPTION_REQUIRED;
        need_quota = true;
    } else if (strcasecmp(operation, "delete") == 0 ||
            strcasecmp(operation, "remove") == 0)
    {
        op_type = FCFS_AUTH_SERVICE_PROTO_SPOOL_REMOVE_REQ;
        username_options = 0;
        poolname_options = PARAM_OPTION_INCLUDED |
            PARAM_OPTION_REQUIRED;
        need_quota = false;
    } else if (strcasecmp(operation, "plist") == 0 ||
            strcasecmp(operation, "pool-list") == 0 ||
            strcasecmp(operation, "pool_list") == 0)
    {
        op_type = FCFS_AUTH_SERVICE_PROTO_SPOOL_LIST_REQ;
        username_options = PARAM_OPTION_INCLUDED;
        poolname_options = PARAM_OPTION_INCLUDED;
        need_quota = false;
    } else if (strcasecmp(operation, "withdraw") == 0 ||
            strcasecmp(operation, "cancel") == 0)
    {
        op_type = FCFS_AUTH_SERVICE_PROTO_GPOOL_WITHDRAW_REQ;
        username_options = PARAM_OPTION_INCLUDED |
            PARAM_OPTION_REQUIRED;
        poolname_options = PARAM_OPTION_INCLUDED |
            PARAM_OPTION_REQUIRED;
        need_quota = false;
    } else if (strcasecmp(operation, "glist") == 0 ||
            strcasecmp(operation, "grant-list") == 0 ||
            strcasecmp(operation, "grant_list") == 0 ||
            strcasecmp(operation, "granted-list") == 0 ||
            strcasecmp(operation, "granted_list") == 0)
    {
        op_type = FCFS_AUTH_SERVICE_PROTO_GPOOL_LIST_REQ;
        username_options = PARAM_OPTION_INCLUDED;
        poolname_options = PARAM_OPTION_INCLUDED;
        need_quota = false;
    } else if (strcasecmp(operation, "config-setid") == 0 ||
            strcasecmp(operation, "config_setid") == 0 ||
            strcasecmp(operation, "cfg-setid") == 0 ||
            strcasecmp(operation, "cfg_setid") == 0)
    {
        op_type = FCFS_AUTH_SERVICE_PROTO_GPOOL_LIST_REQ;
        username_options = PARAM_OPTION_INCLUDED;
        poolname_options = PARAM_OPTION_INCLUDED;
        need_quota = false;
    } else {
        fprintf(stderr, "unknow operation: %s\n", operation);
        usage(argv);
        return 1;
    }

    if ((username_options & PARAM_OPTION_INCLUDED) != 0) {
        if (current_index < argc) {
            FC_SET_STRING(username, argv[current_index]);
            current_index++;
        } else if ((username_options & PARAM_OPTION_REQUIRED) != 0) {
            fprintf(stderr, "expect username\n");
            usage(argv);
            return 1;
        }
    }

    if ((poolname_options & PARAM_OPTION_INCLUDED) != 0) {
        if (current_index < argc) {
            FC_SET_STRING(spool.name, argv[current_index]);
            current_index++;
        } else if ((poolname_options & PARAM_OPTION_REQUIRED) != 0) {
            fprintf(stderr, "expect pool name\n");
            usage(argv);
            return 1;
        } else {
            FC_SET_STRING_NULL(spool.name);
        }
    }

    if (current_index < argc) {
        if (strcasecmp(argv[current_index],
                    FCFS_AUTH_UNLIMITED_QUOTA_STR) == 0)
        {
            spool.quota = FCFS_AUTH_UNLIMITED_QUOTA_VAL;
        } else if ((result=parse_bytes(argv[current_index],
                        1024 * 1024 * 1024, &spool.quota)) != 0)
        {
            fprintf(stderr, "parse quota field fail!\n");
            if (op_type == FCFS_AUTH_SERVICE_PROTO_SPOOL_CREATE_REQ) {
                fprintf(stderr, "\nUsage: %s create [pool_name] <quota> "
                        "[%s]\nFor example: %s create %s %s %s\n\n",
                        argv[0], DRY_RUN_OPTION_STR, argv[0],
                        (spool.name.str != NULL ? spool.name.str :
                         argv[current_index]), FCFS_AUTH_UNLIMITED_QUOTA_STR,
                        (dryrun ? DRY_RUN_OPTION_STR : ""));
            }
            return 1;
        }

        current_index++;
    } else if (need_quota) {
        fprintf(stderr, "expect quota\n");
        usage(argv);
        return 1;
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

    switch (op_type) {
        case FCFS_AUTH_SERVICE_PROTO_SPOOL_CREATE_REQ:
            return create_spool(argc, argv);
        case FCFS_AUTH_SERVICE_PROTO_SPOOL_LIST_REQ:
            return list_spool(argc, argv);
        case FCFS_AUTH_SERVICE_PROTO_SPOOL_REMOVE_REQ:
            return remove_spool(argc, argv);
        case FCFS_AUTH_SERVICE_PROTO_SPOOL_SET_QUOTA_REQ:
            return quota_spool(argc, argv);
        case FCFS_AUTH_SERVICE_PROTO_GPOOL_GRANT_REQ:
            return grant_privilege(argc, argv);
        case FCFS_AUTH_SERVICE_PROTO_GPOOL_WITHDRAW_REQ:
            return withdraw_privilege(argc, argv);
        case FCFS_AUTH_SERVICE_PROTO_GPOOL_LIST_REQ:
            return list_gpool(argc, argv);
    }

    return 0;
}
