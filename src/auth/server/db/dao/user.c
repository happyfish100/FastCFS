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


#include <sys/stat.h>
#include "fastcommon/shared_func.h"
#include "fastcommon/logger.h"
#include "user.h"

#define DIR_NAME_CREATED_STR  "created"
#define DIR_NAME_CREATED_LEN  (sizeof(DIR_NAME_CREATED_STR) - 1)

#define DIR_NAME_GRANTED_STR  "granted"
#define DIR_NAME_GRANTED_LEN  (sizeof(DIR_NAME_GRANTED_STR) - 1)

#define XTTR_NAME_PASSWD_STR  "passwd"
#define XTTR_NAME_PASSWD_LEN  (sizeof(XTTR_NAME_PASSWD_STR) - 1)

#define XTTR_NAME_PRIV_STR    "priv"
#define XTTR_NAME_PRIV_LEN    (sizeof(XTTR_NAME_PRIV_STR) - 1)

#define XTTR_NAME_STATUS_STR  "status"
#define XTTR_NAME_STATUS_LEN  (sizeof(XTTR_NAME_STATUS_STR) - 1)

#define XTTR_NAME_QUOTA_STR   "quota"
#define XTTR_NAME_QUOTA_LEN   (sizeof(XTTR_NAME_QUOTA_STR) - 1)

#define XTTR_NAME_FDIR_STR    "fdir"
#define XTTR_NAME_FDIR_LEN    (sizeof(XTTR_NAME_FDIR_STR) - 1)

#define XTTR_NAME_FSTORE_STR  "fstore"
#define XTTR_NAME_FSTORE_LEN  (sizeof(XTTR_NAME_FSTORE_STR) - 1)

static FDIRClientOwnerModePair dao_omp = {0, 0, 0700};
static string_t dao_ns = {AUTH_NAMESPACE_STR, AUTH_NAMESPACE_LEN};

static string_t dir_name_created = {DIR_NAME_CREATED_STR, DIR_NAME_CREATED_LEN};
static string_t dir_name_granted = {DIR_NAME_GRANTED_STR, DIR_NAME_GRANTED_LEN};

static string_t xttr_name_passwd = {XTTR_NAME_PASSWD_STR, XTTR_NAME_PASSWD_LEN};
static string_t xttr_name_priv = {XTTR_NAME_PRIV_STR, XTTR_NAME_PRIV_LEN};
static string_t xttr_name_status = {XTTR_NAME_STATUS_STR, XTTR_NAME_STATUS_LEN};
static string_t xttr_name_quota = {XTTR_NAME_QUOTA_STR, XTTR_NAME_QUOTA_LEN};
static string_t xttr_name_fdir = {XTTR_NAME_FDIR_STR, XTTR_NAME_FDIR_LEN};
static string_t xttr_name_fstore = {XTTR_NAME_FSTORE_STR, XTTR_NAME_FSTORE_LEN};


static inline int set_xattr_string(FDIRClientContext *client_ctx,
        const int64_t inode, const string_t *name, const string_t *value)
{
    key_value_pair_t xattr;

    xattr.key = *name;
    xattr.value = *value;
    return fdir_client_set_xattr_by_inode(client_ctx,
            &dao_ns, inode, &xattr, 0);
}

static inline int set_xattr_integer(FDIRClientContext *client_ctx,
        const int64_t inode, const string_t *name, const int64_t nv)
{
    char buff[32];
    string_t value;

    value.str = buff;
    value.len = sprintf(buff, "%"PRId64, nv);
    return set_xattr_string(client_ctx, inode, name, &value);
}

int dao_user_create(FDIRClientContext *client_ctx, const string_t *username,
        const string_t *passwd, const int64_t priv)
{
    int result;
    AuthFullPath path;
    FDIRDEntryInfo dentry;

    AUTH_SET_USER_PATH(username, path);
    if ((result=fdir_client_create_dentry(client_ctx, &path.fullname,
                    &dao_omp, &dentry)) != 0)
    {
        return result;
    }

    //TODO
    return 0;
}

int user_remove(const string_t *username)
{
    return 0;
}
