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

#ifndef _AUTH_DAO_STORAGE_POOL_H
#define _AUTH_DAO_STORAGE_POOL_H

#include "fastcommon/fast_mpool.h"
#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

int dao_spool_set_base_path_inode(FDIRClientContext *client_ctx);

int dao_spool_get_auto_id(FDIRClientContext *client_ctx, int64_t *auto_id);

int dao_spool_set_auto_id(FDIRClientContext *client_ctx, const int64_t auto_id);

int dao_spool_create(FDIRClientContext *client_ctx,
        const string_t *username, FCFSAuthStoragePoolInfo *spool);

int dao_spool_remove(FDIRClientContext *client_ctx, const int64_t spool_id);

int dao_spool_set_quota(FDIRClientContext *client_ctx,
        const int64_t spool_id, const int64_t quota);

int dao_spool_list(FDIRClientContext *client_ctx, const string_t *username,
        struct fast_mpool_man *mpool, FCFSAuthStoragePoolArray *spool_array);

#ifdef __cplusplus
}
#endif

#endif
