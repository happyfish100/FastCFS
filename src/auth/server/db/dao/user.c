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

static int user_make_subdir(FDIRClientContext *client_ctx,
        const string_t *username, const char *subdir,
        const bool check_exist)
{
    AuthFullPath fp;
    FDIRDEntryInfo dentry;
    int result;

    AUTH_SET_USER_PATH(fp, username, subdir);
    if (check_exist) {
        result = fdir_client_lookup_inode_by_path_ex(client_ctx,
                &fp.fullname, LOG_DEBUG, &dentry.inode);
        if (result == 0) {
            return 0;
        } else if (result != ENOENT) {
            return result;
        }
    }

    result = fdir_client_create_dentry(client_ctx,
            &fp.fullname, &dao_omp, &dentry);
    return result == EEXIST ? 0 : result;
}

int dao_user_create(FDIRClientContext *client_ctx, FCFSAuthUserInfo *user)
{
    int64_t inode;
    int result;
    bool check_exist;
    AuthFullPath home;
    FDIRDEntryInfo dentry;

    AUTH_SET_USER_HOME(home, &user->name);
    result = fdir_client_create_dentry(client_ctx,
            &home.fullname, &dao_omp, &dentry);
    if (result == 0) {
        inode = dentry.inode;
        check_exist = false;
    } else if (result == EEXIST) {
        if ((result=fdir_client_lookup_inode_by_path_ex(client_ctx,
                        &home.fullname, LOG_ERR, &inode)) != 0)
        {
            return result;
        }
        check_exist = true;
    } else {
        return result;
    }

    if ((result=user_make_subdir(client_ctx, &user->name,
                    DIR_NAME_CREATED_STR, check_exist)) != 0)
    {
        return result;
    }
    if ((result=user_make_subdir(client_ctx, &user->name,
                    DIR_NAME_GRANTED_STR, check_exist)) != 0)
    {
        return result;
    }

    if ((result=set_xattr_string(client_ctx, inode,
                    &xttr_name_passwd, &user->passwd)) != 0)
    {
        return result;
    }
    if ((result=set_xattr_integer(client_ctx, inode,
                    &xttr_name_priv, user->priv)) != 0)
    {
        return result;
    }
    if ((result=set_xattr_integer(client_ctx, inode, &xttr_name_status,
                    FCFS_AUTH_USER_STATUS_NORMAL)) != 0)
    {
        return result;
    }

    user->id = inode;
    return 0;
}

int user_remove(const string_t *username)
{
    return 0;
}
