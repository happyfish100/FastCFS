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

#ifdef __cplusplus
extern "C" {
#endif

int fcfs_auth_client_user_login(FCFSAuthClientContext *client_ctx,
        const string_t *username, const string_t *passwd);

int fcfs_auth_client_user_create(FCFSAuthClientContext *client_ctx,
        const FCFSAuthUserInfo *user);

int fcfs_auth_client_user_list(FCFSAuthClientContext *client_ctx,
        const string_t *username, SFProtoRecvBuffer *buffer,
        FCFSAuthUserArray *array);

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
        SFProtoRecvBuffer *buffer, FCFSAuthStoragePoolArray *array);

int fcfs_auth_client_spool_remove(FCFSAuthClientContext *client_ctx,
        const string_t *poolname);

int fcfs_auth_client_spool_set_quota(FCFSAuthClientContext *client_ctx,
        const string_t *poolname, const int64_t quota);

/* pool granted operations */
int fcfs_auth_client_gpool_grant(FCFSAuthClientContext *client_ctx,
        const string_t *username, const string_t *poolname,
        const FCFSAuthSPoolPriviledges *privs);

int fcfs_auth_client_gpool_withdraw(FCFSAuthClientContext *client_ctx,
        const string_t *username, const string_t *poolname);

int fcfs_auth_client_gpool_list(FCFSAuthClientContext *client_ctx,
        const string_t *username, const string_t *poolname,
        SFProtoRecvBuffer *buffer, FCFSAuthGrantedPoolArray *array);

#ifdef __cplusplus
}
#endif

#endif
