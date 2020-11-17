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

#ifndef _FS_API_TYPES_H
#define _FS_API_TYPES_H

#include <limits.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "fastcommon/fast_mblock.h"
#include "fastcommon/fast_buffer.h"
#include "fastdir/fdir_client.h"
#include "faststore/fs_client.h"

typedef struct fcfs_api_opendir_session {
    FDIRClientDentryArray array;
    int btype;   //buffer type
    FastBuffer buffer;
} FSAPIOpendirSession;

typedef struct fcfs_api_context {
    string_t ns;  //namespace
    char ns_holder[NAME_MAX];
    struct {
        FDIRClientContext *fdir;
        FSClientContext *fs;
    } contexts;

    struct fast_mblock_man opendir_session_pool;
} FSAPIContext;

typedef struct fcfs_api_file_info {
    FSAPIContext *ctx;
    struct {
        FDIRClientSession flock;
        FSAPIOpendirSession *opendir;
    } sessions;
    FDIRDEntryInfo dentry;
    int flags;
    int magic;
    struct {
        int last_modified_time;
    } write_notify;
    int64_t offset;  //current offset
} FSAPIFileInfo;

#ifdef __cplusplus
extern "C" {
#endif

    extern FSAPIContext g_fcfs_api_ctx;

#ifdef __cplusplus
}
#endif

#endif
