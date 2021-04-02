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

#include "sf/idempotency/client/rpc_wrapper.h"
#include "../common/auth_func.h"
#include "fcfs_auth_client.h"

#define GET_CONNECTION(cm, arg1, result) \
    (cm)->ops.get_connection(cm, arg1, result)

static int load_auth_user_passwd(FCFSAuthClientCommonCfg *auth_cfg,
        IniContext *ini_context, const char *auth_config_filename)
{
    int result;
    string_t username;
    string_t secret_key_filename;
    FilenameString new_key_filename;
    string_t new_filename;

    username.str = iniGetStrValue(NULL, "username", ini_context);
    if (username.str == NULL || *(username.str) == '\0') {
        username.str = "admin";
    }
    username.len = strlen(username.str);

    secret_key_filename.str = iniGetStrValue(NULL,
            "secret_key_filename", ini_context);
    if (secret_key_filename.str == NULL || *(secret_key_filename.str) == '\0') {
        secret_key_filename.str = "/etc/fastcfs/auth/keys/${username}.key";
    }
    secret_key_filename.len = strlen(secret_key_filename.str);

    auth_cfg->passwd.str = (char *)auth_cfg->passwd_buff;
    auth_cfg->passwd.len = FCFS_AUTH_PASSWD_LEN;
    fcfs_auth_replace_filename_with_username(&secret_key_filename,
            &username, &new_key_filename);
    new_filename.str = FC_FILENAME_STRING_PTR(new_key_filename);
    new_filename.len = strlen(new_filename.str);
    if ((result=fcfs_auth_load_passwd(new_filename.str,
                    auth_cfg->passwd_buff)) != 0)
    {
        logError("file: "__FILE__", line: %d, "
                "config file: %s, secret_key_filename: %s, "
                "load password fail, ret code: %d", __LINE__,
                auth_config_filename, new_filename.str, result);
        return result;
    }

    auth_cfg->buff = (char *)fc_malloc(username.len + 1 +
            new_filename.len + 1);
    if (auth_cfg->buff == NULL) {
        return ENOMEM;
    }

    auth_cfg->username.str = auth_cfg->buff;
    auth_cfg->username.len = username.len;
    memcpy(auth_cfg->username.str, username.str, username.len + 1);

    auth_cfg->secret_key_filename.str =
        auth_cfg->username.str + username.len + 1;
    auth_cfg->secret_key_filename.len = new_filename.len;
    memcpy(auth_cfg->secret_key_filename.str,
            new_filename.str, new_filename.len + 1);
    return 0;
}

static int load_auth_config(FCFSAuthClientContext *client_ctx,
        bool *auth_enabled, const char *auth_config_filename)
{
    int result;
    IniContext ini_context;
    char *client_config_filename;
    char full_config_filename[PATH_MAX];

    if ((result=iniLoadFromFile(auth_config_filename, &ini_context)) != 0) {
        logError("file: "__FILE__", line: %d, "
                "load conf file \"%s\" fail, ret code: %d",
                __LINE__, auth_config_filename, result);
        return result;
    }

    *auth_enabled = iniGetBoolValue(NULL, "auth_enabled", &ini_context, false);
    do {
        if (client_ctx->inited) {
            break;
        }

        if ((result=load_auth_user_passwd(&client_ctx->auth,
                        &ini_context, auth_config_filename)) != 0)
        {
            break;
        }

        client_config_filename = iniGetStrValue(NULL,
                "client_config_filename", &ini_context);
        if (client_config_filename == NULL ||
                *client_config_filename == '\0')
        {
            logError("file: "__FILE__", line: %d, "
                    "config file: %s, item \"client_config_filename\" "
                    "not exist or empty", __LINE__, auth_config_filename);
            result = ENOENT;
            break;
        }

        resolve_path(auth_config_filename, client_config_filename,
                full_config_filename, sizeof(full_config_filename));
        result = fcfs_auth_client_init_ex(client_ctx,
                full_config_filename, NULL);
    } while (0);

    iniFreeContext(&ini_context);
    return result;
}

int fcfs_auth_load_config(FCFSAuthClientContext *client_ctx,
        bool *auth_enabled, IniFullContext *ini_ctx)
{
    char *auth_config_filename;
    char full_auth_filename[PATH_MAX];

    auth_config_filename = iniGetStrValue(ini_ctx->section_name,
            "auth_config_filename", ini_ctx->context);
    if (auth_config_filename == NULL || *auth_config_filename == '\0') {
        logError("file: "__FILE__", line: %d, "
                "config file: %s, item \"auth_config_filename\" "
                "not exist or empty", __LINE__, ini_ctx->filename);
        return ENOENT;
    }

    resolve_path(ini_ctx->filename, auth_config_filename,
            full_auth_filename, sizeof(full_auth_filename));
    return load_auth_config(client_ctx, auth_enabled, full_auth_filename);
}

int fcfs_auth_client_user_login_ex(FCFSAuthClientContext *client_ctx,
        const string_t *username, const string_t *passwd,
        const string_t *poolname, const int flags)
{
    SF_CLIENT_IDEMPOTENCY_QUERY_WRAPPER(client_ctx, &client_ctx->cm,
            GET_CONNECTION, 0, fcfs_auth_client_proto_user_login,
            username, passwd, poolname, flags);
}

int fcfs_auth_client_user_create(FCFSAuthClientContext *client_ctx,
        const FCFSAuthUserInfo *user)
{
    SF_CLIENT_IDEMPOTENCY_QUERY_WRAPPER(client_ctx, &client_ctx->cm,
            GET_CONNECTION, 0, fcfs_auth_client_proto_user_create, user);
}

int fcfs_auth_client_user_list(FCFSAuthClientContext *client_ctx,
        const string_t *username, SFProtoRecvBuffer *buffer,
        FCFSAuthUserArray *array)
{
    SF_CLIENT_IDEMPOTENCY_QUERY_WRAPPER(client_ctx, &client_ctx->cm,
            GET_CONNECTION, 0, fcfs_auth_client_proto_user_list,
            username, buffer, array);
}

int fcfs_auth_client_user_grant(FCFSAuthClientContext *client_ctx,
        const string_t *username, const int64_t priv)
{
    SF_CLIENT_IDEMPOTENCY_QUERY_WRAPPER(client_ctx, &client_ctx->cm,
            GET_CONNECTION, 0, fcfs_auth_client_proto_user_grant,
            username, priv);
}

int fcfs_auth_client_user_remove(FCFSAuthClientContext *client_ctx,
        const string_t *username)
{
    SF_CLIENT_IDEMPOTENCY_QUERY_WRAPPER(client_ctx, &client_ctx->cm,
            GET_CONNECTION, 0, fcfs_auth_client_proto_user_remove,
            username);
}

int fcfs_auth_client_spool_create(FCFSAuthClientContext *client_ctx,
        FCFSAuthStoragePoolInfo *spool, const int name_size,
        const bool dryrun)
{
    SF_CLIENT_IDEMPOTENCY_QUERY_WRAPPER(client_ctx, &client_ctx->cm,
            GET_CONNECTION, 0, fcfs_auth_client_proto_spool_create,
            spool, name_size, dryrun);
}

int fcfs_auth_client_spool_list(FCFSAuthClientContext *client_ctx,
        const string_t *username, const string_t *poolname,
        SFProtoRecvBuffer *buffer, FCFSAuthStoragePoolArray *array)
{
    SF_CLIENT_IDEMPOTENCY_QUERY_WRAPPER(client_ctx, &client_ctx->cm,
            GET_CONNECTION, 0, fcfs_auth_client_proto_spool_list,
            username, poolname, buffer, array);
}

int fcfs_auth_client_spool_remove(FCFSAuthClientContext *client_ctx,
        const string_t *poolname)
{
    SF_CLIENT_IDEMPOTENCY_QUERY_WRAPPER(client_ctx, &client_ctx->cm,
            GET_CONNECTION, 0, fcfs_auth_client_proto_spool_remove,
            poolname);
}

int fcfs_auth_client_spool_set_quota(FCFSAuthClientContext *client_ctx,
        const string_t *poolname, const int64_t quota)
{
    SF_CLIENT_IDEMPOTENCY_QUERY_WRAPPER(client_ctx, &client_ctx->cm,
            GET_CONNECTION, 0, fcfs_auth_client_proto_spool_set_quota,
            poolname, quota);
}

int fcfs_auth_client_gpool_grant(FCFSAuthClientContext *client_ctx,
        const string_t *username, const string_t *poolname,
        const FCFSAuthSPoolPriviledges *privs)
{
    SF_CLIENT_IDEMPOTENCY_QUERY_WRAPPER(client_ctx, &client_ctx->cm,
            GET_CONNECTION, 0, fcfs_auth_client_proto_gpool_grant,
            username, poolname, privs);
}

int fcfs_auth_client_gpool_withdraw(FCFSAuthClientContext *client_ctx,
        const string_t *username, const string_t *poolname)
{
    SF_CLIENT_IDEMPOTENCY_QUERY_WRAPPER(client_ctx, &client_ctx->cm,
            GET_CONNECTION, 0, fcfs_auth_client_proto_gpool_withdraw,
            username, poolname);
}

int fcfs_auth_client_gpool_list(FCFSAuthClientContext *client_ctx,
        const string_t *username, const string_t *poolname,
        SFProtoRecvBuffer *buffer, FCFSAuthGrantedPoolArray *array)
{
    SF_CLIENT_IDEMPOTENCY_QUERY_WRAPPER(client_ctx, &client_ctx->cm,
            GET_CONNECTION, 0, fcfs_auth_client_proto_gpool_list,
            username, poolname, buffer, array);
}
