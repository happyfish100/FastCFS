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


#ifndef _FCFS_FUSE_WRAPPER_H
#define _FCFS_FUSE_WRAPPER_H

#ifndef FUSE_USE_VERSION
#define FUSE_USE_VERSION 35
#endif

#include "fastcommon/logger.h"
#include "fastcfs/api/fcfs_api.h"
#include "fastcfs/api/fcfs_api_util.h"
#include "fuse3/fuse_lowlevel.h"

#define FCFS_FUSE_SET_OMP(omp, m, euid, egid) \
    FCFS_API_SET_OMP(omp, g_fuse_global_vars.owner, m, euid, egid)

#define FCFS_FUSE_SET_OMP_BY_REQ(omp, m, req) \
    do { \
        const struct fuse_ctx *_fctx;  \
        _fctx = fuse_req_ctx(req);     \
        FCFS_FUSE_SET_OMP(omp, m, _fctx->uid, _fctx->gid); \
    } while (0)

#define FCFS_FUSE_SET_FCTX_BY_REQ(fctx, m, req) \
    do { \
        const struct fuse_ctx *_fctx;  \
        _fctx = fuse_req_ctx(req);     \
        FCFS_FUSE_SET_OMP(fctx.omp, m, _fctx->uid, _fctx->gid); \
        fctx.tid = _fctx->pid;  \
    } while (0)

#ifdef __cplusplus
extern "C" {
#endif

	int fs_fuse_wrapper_init(struct fuse_lowlevel_ops *ops);

#ifdef __cplusplus
}
#endif

#endif
