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
#include "granted_pool.h"

int dao_granted_create(FDIRClientContext *client_ctx, const string_t *username,
        FCFSAuthGrantedPoolInfo *granted)
{
    int result;
    int64_t inode;
    AuthFullPath pool_path;
    FDIRDEntryInfo dentry;

    AUTH_SET_GRANTED_POOL_PATH(pool_path, username, granted->pool_id);
    if ((result=fdir_client_lookup_inode_by_path_ex(client_ctx,
                    &pool_path.fullname, LOG_DEBUG, &inode)) != 0)
    {
        if (result != ENOENT) {
            return result;
        }

        if ((result=fdir_client_create_dentry(client_ctx, &pool_path.
                        fullname, &DAO_OMP_FILE, &dentry)) != 0)
        {
            return result;
        }
        inode = dentry.inode;
    }

    if ((result=dao_set_xattr_integer(client_ctx, inode,
                    &AUTH_XTTR_NAME_FDIR, granted->privs.fdir)) != 0)
    {
        return result;
    }
    if ((result=dao_set_xattr_integer(client_ctx, inode,
                    &AUTH_XTTR_NAME_FSTORE, granted->privs.fstore)) != 0)
    {
        return result;
    }

    granted->id = inode;
    return 0;
}

int dao_granted_remove(FDIRClientContext *client_ctx,
        const string_t *username, const int64_t pool_id)
{
    const int flags = 0;
    AuthFullPath pool_path;

    AUTH_SET_GRANTED_POOL_PATH(pool_path, username, pool_id);
    return fdir_client_remove_dentry(client_ctx, &pool_path.fullname, flags);
}

static int dump_to_granted_array(FDIRClientContext *client_ctx,
        const FDIRClientDentryArray *darray,
        FCFSAuthGrantedPoolArray *parray)
{
    const FDIRClientDentry *entry;
    const FDIRClientDentry *end;
    char *endptr;
    FCFSAuthGrantedPoolFullInfo *new_gpools;
    FCFSAuthGrantedPoolFullInfo *gpool;
    FCFSAuthGrantedPoolInfo *granted;
    int result;

    if (darray->count > parray->alloc) {
        new_gpools = (FCFSAuthGrantedPoolFullInfo *)fc_malloc(
                sizeof(FCFSAuthGrantedPoolFullInfo) * darray->count);
        if (new_gpools == NULL) {
            return ENOMEM;
        }

        if (parray->gpools != parray->fixed) {
            free(parray->gpools);
        }
        parray->gpools = new_gpools;
        parray->alloc = darray->count;
    }

    end = darray->entries + darray->count;
    for (entry=darray->entries, gpool=parray->gpools;
            entry<end; entry++, gpool++)
    {
        granted = &gpool->granted;
        granted->id = entry->dentry.inode;
        granted->pool_id = strtoll(entry->name.str, &endptr, 10);
        if ((result=dao_get_xattr_int32(client_ctx, granted->id,
                        &AUTH_XTTR_NAME_FDIR, &granted->privs.fdir)) != 0)
        {
            return result;
        }
        if ((result=dao_get_xattr_int32(client_ctx, granted->id,
                        &AUTH_XTTR_NAME_FSTORE, &granted->privs.fstore)) != 0)
        {
            return result;
        }
    }

    parray->count = darray->count;
    return 0;
}

int dao_granted_list(FDIRClientContext *client_ctx, const string_t *username,
        FCFSAuthGrantedPoolArray *granted_array)
{
    int result;
    AuthFullPath fp;
    FDIRClientDentryArray dentry_array;

    if ((result=fdir_client_dentry_array_init(&dentry_array)) != 0) {
        return result;
    }

    AUTH_SET_USER_PATH1(fp, username, AUTH_DIR_NAME_GRANTED_STR);
    if ((result=fdir_client_list_dentry_by_path(client_ctx,
                    &fp.fullname, &dentry_array)) != 0)
    {
        fdir_client_dentry_array_free(&dentry_array);
        return result;
    }

    result = dump_to_granted_array(client_ctx,
            &dentry_array, granted_array);
    fdir_client_dentry_array_free(&dentry_array);
    return result;
}
