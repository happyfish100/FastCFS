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
#define FUSE_USE_VERSION 312
#endif

#include "fastcommon/common_define.h"
#include "fastcfs/api/fcfs_api.h"
#include "fastcfs/api/fcfs_api_util.h"
#include "fuse3/fuse_lowlevel.h"

#ifdef __cplusplus
extern "C" {
#endif

    extern struct fuse_conn_info_opts *g_fuse_cinfo_opts;

	int fs_fuse_wrapper_init(struct fuse_lowlevel_ops *ops);

#ifdef __cplusplus
}
#endif

#endif
