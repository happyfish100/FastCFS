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


#ifndef _FCFS_AUTH_CLIENT_H
#define _FCFS_AUTH_CLIENT_H

#include "auth_proto.h"
#include "client_types.h"
#include "client_func.h"
#include "client_global.h"
#include "client_proto.h"
#include "session_regenerate.h"

#define fcfs_auth_load_config(auth, config_filename)  \
    fcfs_auth_load_config_ex(auth, config_filename, NULL, \
            sf_server_group_index_type_service)

#ifdef __cplusplus
extern "C" {
#endif

int fcfs_auth_load_config_ex(FCFSAuthClientFullContext *auth,
        const char *config_filename, const char *section_name,
        const SFServerGroupIndexType index_type);

void fcfs_auth_config_to_string_ex(const FCFSAuthClientFullContext *auth,
        const char *caption, char *output, const int size);

static inline void fcfs_auth_config_to_string(const FCFSAuthClientFullContext
        *auth, char *output, const int size)
{
    const char *caption = "auth ";
    fcfs_auth_config_to_string_ex(auth, caption, output, size);
}

int fcfs_auth_client_user_login_ex(FCFSAuthClientContext *client_ctx,
        const string_t *username, const string_t *passwd,
        const string_t *poolname, const int flags);

static inline int fcfs_auth_client_user_login(
        FCFSAuthClientContext *client_ctx,
        const string_t *username, const string_t *passwd)
{
    const string_t poolname = {NULL, 0};
    const int flags = 0;
    return fcfs_auth_client_user_login_ex(client_ctx,
            username, passwd, &poolname, flags);
}

static inline int fcfs_auth_client_session_create_ex(
        FCFSAuthClientFullContext *auth, string_t *poolname,
        const bool publish)
{
    int result;
    int flags;

    /*
       logInfo("file: "__FILE__", line: %d, "
       "auth_enabled: %d, username: %s, secret_key_filename: %s",
       __LINE__, auth->enabled, auth->ctx->auth_cfg.username.str,
       auth->ctx->auth_cfg.secret_key_filename.str);
     */

    if (auth->enabled) {
        auth->ctx->session.poolname = poolname;

        if (publish) {
            if ((result=session_regenerate_init()) != 0) {
                return result;
            }
            flags = FCFS_AUTH_SESSION_FLAGS_PUBLISH;
        } else {
            flags = 0;
        }
        return fcfs_auth_client_user_login_ex(auth->ctx, &auth->
                ctx->auth_cfg.username, &auth->ctx->auth_cfg.passwd,
                poolname, flags);
    } else {
        return 0;
    }
}

static inline int fcfs_auth_client_session_create(
        FCFSAuthClientFullContext *auth, const bool publish)
{
#define AUTH_EMPTY_POOL_NAME SF_G_EMPTY_STRING
    return fcfs_auth_client_session_create_ex(auth,
            &AUTH_EMPTY_POOL_NAME, publish);
}

int fcfs_auth_client_session_subscribe(FCFSAuthClientContext *client_ctx);

int fcfs_auth_client_session_validate(FCFSAuthClientContext *client_ctx,
        const string_t *session_id, const string_t *validate_key,
        const FCFSAuthValidatePriviledgeType priv_type,
        const int64_t pool_id, const int64_t priv_required);

int fcfs_auth_client_user_create(FCFSAuthClientContext *client_ctx,
        const FCFSAuthUserInfo *user);

int fcfs_auth_client_user_passwd(FCFSAuthClientContext *client_ctx,
        const string_t *username, const string_t *passwd);

int fcfs_auth_client_user_list(FCFSAuthClientContext *client_ctx,
        const string_t *username, const SFListLimitInfo *limit,
        struct fast_mpool_man *mpool, FCFSAuthUserArray *array);

int fcfs_auth_client_user_grant(FCFSAuthClientContext *client_ctx,
        const string_t *username, const int64_t priv);

int fcfs_auth_client_user_remove(FCFSAuthClientContext *client_ctx,
        const string_t *username);

/* storage pool operations */
int fcfs_auth_client_spool_create(FCFSAuthClientContext *client_ctx,
        FCFSAuthStoragePoolInfo *spool, const int name_size,
        const bool dryrun);

int fcfs_auth_client_spool_list(FCFSAuthClientContext *client_ctx,
        const string_t *username, const string_t *poolname,
        const SFListLimitInfo *limit, struct fast_mpool_man *mpool,
        FCFSAuthStoragePoolArray *array);

int fcfs_auth_client_spool_remove(FCFSAuthClientContext *client_ctx,
        const string_t *poolname);

int fcfs_auth_client_spool_set_quota(FCFSAuthClientContext *client_ctx,
        const string_t *poolname, const int64_t quota);

int fcfs_auth_client_spool_get_quota(FCFSAuthClientContext *client_ctx,
        const string_t *poolname, int64_t *quota);

/* pool granted operations */
int fcfs_auth_client_gpool_grant(FCFSAuthClientContext *client_ctx,
        const string_t *username, const string_t *poolname,
        const FCFSAuthSPoolPriviledges *privs);

int fcfs_auth_client_gpool_withdraw(FCFSAuthClientContext *client_ctx,
        const string_t *username, const string_t *poolname);

int fcfs_auth_client_gpool_list(FCFSAuthClientContext *client_ctx,
        const string_t *username, const string_t *poolname,
        const SFListLimitInfo *limit, struct fast_mpool_man *mpool,
        FCFSAuthGrantedPoolArray *array);

#ifdef __cplusplus
}
#endif

#endif
