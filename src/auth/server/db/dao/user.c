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
#include "func.h"
#include "user.h"

static int user_make_subdir(FDIRClientContext *client_ctx,
        const string_t *username, const char *subdir,
        const bool check_exist)
{
    AuthFullPath fp;
    FDIRDEntryInfo dentry;
    int result;

    AUTH_SET_USER_PATH1(fp, username, subdir);
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
            &fp.fullname, &DAO_OMP_DIR, &dentry);
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
            &home.fullname, &DAO_OMP_DIR, &dentry);
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
                    AUTH_DIR_NAME_CREATED_STR, check_exist)) != 0)
    {
        return result;
    }
    if ((result=user_make_subdir(client_ctx, &user->name,
                    AUTH_DIR_NAME_GRANTED_STR, check_exist)) != 0)
    {
        return result;
    }

    if ((result=dao_set_xattr_string(client_ctx, inode,
                    &AUTH_XTTR_NAME_PASSWD, &user->passwd)) != 0)
    {
        return result;
    }
    if ((result=dao_set_xattr_integer(client_ctx, inode,
                    &AUTH_XTTR_NAME_PRIV, user->priv)) != 0)
    {
        return result;
    }
    if ((result=dao_set_xattr_integer(client_ctx, inode, &AUTH_XTTR_NAME_STATUS,
                    FCFS_AUTH_USER_STATUS_NORMAL)) != 0)
    {
        return result;
    }

    user->id = inode;
    return 0;
}

int dao_user_update_priv(FDIRClientContext *client_ctx,
        const int64_t user_id, const int64_t priv)
{
    return dao_set_xattr_integer(client_ctx, user_id, &AUTH_XTTR_NAME_PRIV, priv);
}

int dao_user_update_passwd(FDIRClientContext *client_ctx,
        const int64_t user_id, const string_t *passwd)
{
    return dao_set_xattr_string(client_ctx, user_id,
            &AUTH_XTTR_NAME_PASSWD, passwd);
}

int dao_user_remove(FDIRClientContext *client_ctx, const int64_t user_id)
{
    return dao_set_xattr_integer(client_ctx, user_id, &AUTH_XTTR_NAME_STATUS,
            FCFS_AUTH_USER_STATUS_DELETED);
}

static int dump_to_user_array(FDIRClientContext *client_ctx,
        struct fast_mpool_man *mpool, const FDIRClientDentryArray *darray,
        FCFSAuthUserArray *uarray)
{
    const FDIRClientDentry *entry;
    const FDIRClientDentry *end;
    FCFSAuthUserInfo *new_users;
    FCFSAuthUserInfo *user;
    char buff[256];
    string_t value;
    int result;

    if (darray->count > uarray->alloc) {
        new_users = (FCFSAuthUserInfo *)fc_malloc(
                sizeof(FCFSAuthUserInfo) * darray->count);
        if (new_users == NULL) {
            return ENOMEM;
        }

        if (uarray->users != uarray->fixed) {
            free(uarray->users);
        }
        uarray->users = new_users;
        uarray->alloc = darray->count;
    }

    end = darray->entries + darray->count;
    for (entry=darray->entries, user=uarray->users;
            entry<end; entry++, user++)
    {
        user->id = entry->dentry.inode;
        user->name = entry->name;

        value.str = buff;
        if ((result=dao_get_xattr_string(client_ctx, user->id,
                        &AUTH_XTTR_NAME_PASSWD, &value, sizeof(buff))) != 0)
        {
            return result;
        }
        if ((result=fast_mpool_alloc_string_ex2(mpool,
                        &user->passwd, &value)) != 0)
        {
            return result;
        }

        if ((result=dao_get_xattr_int64(client_ctx, user->id,
                        &AUTH_XTTR_NAME_PRIV, &user->priv)) != 0)
        {
            return result;
        }
        if ((result=dao_get_xattr_int32(client_ctx, user->id,
                        &AUTH_XTTR_NAME_STATUS, &user->status)) != 0)
        {
            return result;
        }
    }

    uarray->count = darray->count;
    return 0;
}

int dao_user_list(FDIRClientContext *client_ctx, struct fast_mpool_man
        *mpool, FCFSAuthUserArray *user_array)
{
    int result;
    FDIRDEntryInfo dentry;
    AuthFullPath fp;
    FDIRDEntryFullName root;
    FDIRClientDentryArray dentry_array;

    if ((result=fdir_client_dentry_array_init_ex(
                    &dentry_array, mpool)) != 0)
    {
        return result;
    }

    AUTH_SET_BASE_PATH(fp);
    if ((result=fdir_client_lookup_inode_by_path_ex(client_ctx,
                    &fp.fullname, LOG_DEBUG, &dentry.inode)) != 0)
    {
        if (result != ENOENT) {
            return result;
        }

        FC_SET_STRING_EX(root.ns, AUTH_NAMESPACE_STR,
                AUTH_NAMESPACE_LEN);
        FC_SET_STRING_EX(root.path, "/", 1);
        if ((result=fdir_client_create_dentry(client_ctx,
                        &root, &DAO_OMP_DIR, &dentry)) != 0)
        {
            return result;
        }
        if ((result=fdir_client_create_dentry(client_ctx,
                        &fp.fullname, &DAO_OMP_DIR, &dentry)) != 0)
        {
            return result;
        }
    }

    if ((result=fdir_client_list_dentry_by_inode(client_ctx,
                    &DAO_NAMESPACE, dentry.inode, &dentry_array)) != 0)
    {
        fdir_client_dentry_array_free(&dentry_array);
        return result;
    }

    result = dump_to_user_array(client_ctx, mpool,
            &dentry_array, user_array);
    fdir_client_dentry_array_free(&dentry_array);
    return result;
}
