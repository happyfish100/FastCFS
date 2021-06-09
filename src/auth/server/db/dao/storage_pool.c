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
#include "storage_pool.h"

int dao_spool_create(FDIRClientContext *client_ctx,
        const string_t *username, FCFSAuthStoragePoolInfo *spool)
{
    int result;
    int64_t inode;
    AuthFullPath pool_path;
    FDIRDEntryInfo dentry;

    AUTH_SET_USER_PATH2(pool_path, username,
            AUTH_DIR_NAME_CREATED_STR, spool->name);
    if ((result=fdir_client_create_dentry(client_ctx,
            &pool_path.fullname, &DAO_OMP_FILE, &dentry)) == 0)
    {
        inode = dentry.inode;
    } else if (result == EEXIST) {
        if ((result=fdir_client_lookup_inode_by_path_ex(client_ctx,
                        &pool_path.fullname, LOG_ERR, &inode)) != 0)
        {
            return result;
        }
    } else {
        return result;
    }

    if ((result=dao_set_xattr_integer(client_ctx, inode,
                    &AUTH_XTTR_NAME_QUOTA, spool->quota)) != 0)
    {
        return result;
    }
    if ((result=dao_set_xattr_integer(client_ctx, inode,
                    &AUTH_XTTR_NAME_STATUS,
                    FCFS_AUTH_POOL_STATUS_NORMAL)) != 0)
    {
        return result;
    }

    spool->id = inode;
    return 0;
}

int dao_spool_remove(FDIRClientContext *client_ctx, const int64_t spool_id)
{
    return dao_set_xattr_integer(client_ctx, spool_id, &AUTH_XTTR_NAME_STATUS,
            FCFS_AUTH_POOL_STATUS_DELETED);
}

int dao_spool_set_quota(FDIRClientContext *client_ctx,
        const int64_t spool_id, const int64_t quota)
{
    return dao_set_xattr_integer(client_ctx, spool_id,
            &AUTH_XTTR_NAME_QUOTA, quota);
}

static int dump_to_spool_array(FDIRClientContext *client_ctx,
        struct fast_mpool_man *mpool, const FDIRClientDentryArray *darray,
        FCFSAuthStoragePoolArray *parray)
{
    const FDIRClientDentry *entry;
    const FDIRClientDentry *end;
    FCFSAuthStoragePoolInfo *new_spools;
    FCFSAuthStoragePoolInfo *spool;
    int result;

    if (darray->count > parray->alloc) {
        new_spools = (FCFSAuthStoragePoolInfo *)fc_malloc(
                sizeof(FCFSAuthStoragePoolInfo) * darray->count);
        if (new_spools == NULL) {
            return ENOMEM;
        }

        if (parray->spools != parray->fixed) {
            free(parray->spools);
        }
        parray->spools = new_spools;
        parray->alloc = darray->count;
    }

    end = darray->entries + darray->count;
    for (entry=darray->entries, spool=parray->spools;
            entry<end; entry++, spool++)
    {
        spool->id = entry->dentry.inode;
        spool->name = entry->name;
        if ((result=dao_get_xattr_int64(client_ctx, spool->id,
                        &AUTH_XTTR_NAME_QUOTA, &spool->quota)) != 0)
        {
            return result;
        }
        if ((result=dao_get_xattr_int32(client_ctx, spool->id,
                        &AUTH_XTTR_NAME_STATUS, &spool->status)) != 0)
        {
            return result;
        }
    }

    parray->count = darray->count;
    return 0;
}

int dao_spool_list(FDIRClientContext *client_ctx, const string_t *username,
        struct fast_mpool_man *mpool, FCFSAuthStoragePoolArray *spool_array)
{
    int result;
    AuthFullPath fp;
    FDIRClientDentryArray dentry_array;

    if ((result=fdir_client_dentry_array_init_ex(
                    &dentry_array, mpool)) != 0)
    {
        return result;
    }

    AUTH_SET_USER_PATH1(fp, username, AUTH_DIR_NAME_CREATED_STR);
    if ((result=fdir_client_list_dentry_by_path(client_ctx,
                    &fp.fullname, &dentry_array)) != 0)
    {
        fdir_client_dentry_array_free(&dentry_array);
        return result;
    }

    result = dump_to_spool_array(client_ctx, mpool,
            &dentry_array, spool_array);
    fdir_client_dentry_array_free(&dentry_array);
    return result;
}

int dao_spool_set_base_path_inode(FDIRClientContext *client_ctx)
{
    AuthFullPath fp;

    AUTH_SET_BASE_PATH(fp);
    return fdir_client_lookup_inode_by_path(client_ctx,
            &fp.fullname, &DAO_BASE_PATH_INODE);
}

int dao_spool_get_auto_id(FDIRClientContext *client_ctx, int64_t *auto_id)
{
    return dao_get_xattr_int64(client_ctx, DAO_BASE_PATH_INODE,
            &AUTH_XTTR_NAME_POOL_AUTO_ID, auto_id);
}

int dao_spool_set_auto_id(FDIRClientContext *client_ctx, const int64_t auto_id)
{
    return dao_set_xattr_integer(client_ctx, DAO_BASE_PATH_INODE,
            &AUTH_XTTR_NAME_POOL_AUTO_ID, auto_id);
}
