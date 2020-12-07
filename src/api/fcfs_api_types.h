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

#ifndef _FCFS_API_TYPES_H
#define _FCFS_API_TYPES_H

#include <limits.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "fastcommon/fast_mblock.h"
#include "fastcommon/fast_buffer.h"
#include "fastdir/client/fdir_client.h"
#include "fastsore/api/fs_api.h"

typedef struct fcfs_api_opendir_session {
    FDIRClientDentryArray array;
    int btype;   //buffer type
    FastBuffer buffer;
} FCFSAPIOpendirSession;

typedef struct fcfs_api_context {
    bool use_sys_lock_for_append;
    string_t ns;  //namespace
    char ns_holder[NAME_MAX];
    struct {
        FDIRClientContext *fdir;
        FSAPIContext *fsapi;
    } contexts;

    struct fast_mblock_man opendir_session_pool;
} FCFSAPIContext;

typedef struct fcfs_api_file_info {
    FCFSAPIContext *ctx;
    int64_t tid;
    struct {
        FDIRClientSession flock;
        FCFSAPIOpendirSession *opendir;
    } sessions;
    FDIRDEntryInfo dentry;
    int flags;
    int magic;
    struct {
        int last_modified_time;
    } write_notify;
    int64_t offset;  //current offset
} FCFSAPIFileInfo;

typedef struct fcfs_api_file_context {
    FDIRClientOwnerModePair omp;
    int64_t tid;
} FCFSAPIFileContext;

typedef struct fcfs_api_write_done_callback_extra_data {
    FCFSAPIContext *ctx;
    int64_t file_size;
    int64_t space_end;
    int last_modified_time;
} FCFSAPIWriteDoneCallbackExtraData;

typedef struct fcfs_api_write_done_callback_arg {
    FSAPIWriteDoneCallbackArg arg;  //must be the first
    FCFSAPIWriteDoneCallbackExtraData extra;
} FCFSAPIWriteDoneCallbackArg;

#ifdef __cplusplus
extern "C" {
#endif

    extern FCFSAPIContext g_fcfs_api_ctx;

#ifdef __cplusplus
}
#endif

#endif
