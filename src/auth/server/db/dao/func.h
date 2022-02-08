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

#ifndef _AUTH_DAO_FUNC_H
#define _AUTH_DAO_FUNC_H

#include "fastcommon/fast_mpool.h"
#include "types.h"

#define AUTH_DIR_NAME_CREATED_STR  "created"
#define AUTH_DIR_NAME_CREATED_LEN  (sizeof(AUTH_DIR_NAME_CREATED_STR) - 1)

#define AUTH_DIR_NAME_GRANTED_STR  "granted"
#define AUTH_DIR_NAME_GRANTED_LEN  (sizeof(AUTH_DIR_NAME_GRANTED_STR) - 1)

#define AUTH_XTTR_NAME_PASSWD_STR  "passwd"
#define AUTH_XTTR_NAME_PASSWD_LEN  (sizeof(AUTH_XTTR_NAME_PASSWD_STR) - 1)

#define AUTH_XTTR_NAME_PRIV_STR    "priv"
#define AUTH_XTTR_NAME_PRIV_LEN    (sizeof(AUTH_XTTR_NAME_PRIV_STR) - 1)

#define AUTH_XTTR_NAME_STATUS_STR  "status"
#define AUTH_XTTR_NAME_STATUS_LEN  (sizeof(AUTH_XTTR_NAME_STATUS_STR) - 1)

#define AUTH_XTTR_NAME_QUOTA_STR   "quota"
#define AUTH_XTTR_NAME_QUOTA_LEN   (sizeof(AUTH_XTTR_NAME_QUOTA_STR) - 1)

#define AUTH_XTTR_NAME_FDIR_STR    "fdir"
#define AUTH_XTTR_NAME_FDIR_LEN    (sizeof(AUTH_XTTR_NAME_FDIR_STR) - 1)

#define AUTH_XTTR_NAME_FSTORE_STR  "fstore"
#define AUTH_XTTR_NAME_FSTORE_LEN  (sizeof(AUTH_XTTR_NAME_FSTORE_STR) - 1)

#define AUTH_XTTR_NAME_POOL_AUTO_ID_STR  "auto_pid"
#define AUTH_XTTR_NAME_POOL_AUTO_ID_LEN  \
    (sizeof(AUTH_XTTR_NAME_POOL_AUTO_ID_STR) - 1)

typedef struct {
    string_t ns;

    struct {
        FDIRClientOwnerModePair dir;
        FDIRClientOwnerModePair file;
    } omps;

    struct {
        string_t passwd;
        string_t priv;
        string_t status;
        string_t quota;
        string_t fdir;
        string_t fstore;
        string_t pool_auto_id;
    } xttr_names;

    int64_t base_path_inode;
} FCFSAuthDAOVariables;

#define DAO_NAMESPACE g_auth_dao_vars.ns
#define DAO_OMP_DIR   g_auth_dao_vars.omps.dir
#define DAO_OMP_FILE  g_auth_dao_vars.omps.file
#define DAO_BASE_PATH_INODE  g_auth_dao_vars.base_path_inode

#define AUTH_XTTR_NAME_PASSWD g_auth_dao_vars.xttr_names.passwd
#define AUTH_XTTR_NAME_PRIV   g_auth_dao_vars.xttr_names.priv
#define AUTH_XTTR_NAME_STATUS g_auth_dao_vars.xttr_names.status
#define AUTH_XTTR_NAME_QUOTA  g_auth_dao_vars.xttr_names.quota
#define AUTH_XTTR_NAME_FDIR   g_auth_dao_vars.xttr_names.fdir
#define AUTH_XTTR_NAME_FSTORE g_auth_dao_vars.xttr_names.fstore
#define AUTH_XTTR_NAME_POOL_AUTO_ID g_auth_dao_vars.xttr_names.pool_auto_id

#ifdef __cplusplus
extern "C" {
#endif
extern FCFSAuthDAOVariables g_auth_dao_vars;

static inline int dao_set_xattr_string(FDIRClientContext *client_ctx,
        const int64_t inode, const string_t *name, const string_t *value)
{
    key_value_pair_t xattr;

    xattr.key = *name;
    xattr.value = *value;
    return fdir_client_set_xattr_by_inode(client_ctx,
            &DAO_NAMESPACE, inode, &xattr, 0);
}

static inline int dao_set_xattr_integer(FDIRClientContext *client_ctx,
        const int64_t inode, const string_t *name, const int64_t nv)
{
    char buff[32];
    string_t value;

    value.str = buff;
    value.len = sprintf(buff, "%"PRId64, nv);
    return dao_set_xattr_string(client_ctx, inode, name, &value);
}

static inline int dao_get_xattr_string(FDIRClientContext *client_ctx,
        const int64_t inode, const string_t *name, string_t *value,
        const int size)
{
    const int flags = 0;
    int result;
    result = fdir_client_get_xattr_by_inode_ex(client_ctx,
            &DAO_NAMESPACE, inode, name, LOG_WARNING,
            value, size, flags);
    if (result == ENODATA) {
        value->len = 0;
        return 0;
    } else {
        return result;
    }
}

static inline int dao_get_xattr_int64(FDIRClientContext *client_ctx,
        const int64_t inode, const string_t *name, int64_t *nv)
{
    char buff[32];
    string_t value;
    char *endptr;
    int result;

    value.str = buff;
    if ((result=dao_get_xattr_string(client_ctx, inode,
                    name, &value, sizeof(buff) - 1)) != 0)
    {
        return result;
    }

    if (value.len == 0) {
        *nv = 0;
        return 0;
    }

    *(value.str + value.len) = '\0';
    *nv = strtoll(value.str, &endptr, 10);
    if (endptr != NULL && *endptr != '\0') {
        logError("file: "__FILE__", line: %d, "
                "field: %.*s, invalid digital string: %s",
                __LINE__, name->len, name->str, value.str);
        return EINVAL;
    }

    return 0;
}

static inline int dao_get_xattr_int32(FDIRClientContext *client_ctx,
        const int64_t inode, const string_t *name, int32_t *nv)
{
    int64_t n;
    int result;

    if ((result=dao_get_xattr_int64(client_ctx, inode, name, &n)) == 0) {
        *nv = n;
    }

    return result;
}

#ifdef __cplusplus
}
#endif

#endif
