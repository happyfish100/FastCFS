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

#include <sys/types.h>
#include <limits.h>
#include <fcntl.h>
#include "fastcommon/fast_mblock.h"
#include "fastcommon/fast_buffer.h"
#include "fastdir/client/fdir_client.h"
#include "fastsore/api/fs_api.h"

#define FCFS_FUSE_DEFAULT_CONFIG_FILENAME "/etc/fastcfs/fcfs/fuse.conf"

typedef enum {
    fcfs_api_owner_type_caller,
    fcfs_api_owner_type_fixed
} FCFSAPIOwnerType;

typedef struct fcfs_api_owner_info {
    FCFSAPIOwnerType type;
    uid_t uid;
    gid_t gid;
} FCFSAPIOwnerInfo;

typedef struct fcfs_api_ns_mountpoint_holder {
    char *ns;
    char *mountpoint;
} FCFSAPINSMountpointHolder;

typedef struct fcfs_api_opendir_session {
    FDIRClientDentryArray array;
    int btype;   //buffer type
    FastBuffer buffer;
} FCFSAPIOpendirSession;

typedef struct fcfs_api_context {
    bool use_sys_lock_for_append;
    struct {
        bool enabled;
        int interval_ms;
        int shared_allocator_count;
        int hashtable_sharding_count;
        int64_t hashtable_total_capacity;
    } async_report;
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

struct fcfs_api_inode_hentry;
struct fcfs_api_insert_event_context;

typedef struct fcfs_api_waiting_task {
    pthread_lock_cond_pair_t lcp;  //for notify
    bool finished;
    struct fast_mblock_man *allocator;  //for free
    struct fcfs_api_waiting_task *next; //for event waitings queue
} FCFSAPIWaitingTask;

typedef enum {
    fcfs_api_event_type_report,
    fcfs_api_event_type_notify
} FCFSAPIEventType;

typedef struct fcfs_api_async_report_event {
    FCFSAPIEventType type;
    int id;   //used by async_reporter for stable sort
    FDIRSetDEntrySizeInfo dsize;
    struct fcfs_api_inode_hentry *inode_hentry;
    struct {
        FCFSAPIWaitingTask *head; //use lock of inode sharding
    } waitings;
    struct fc_list_head dlink;  //for inode sharding htable

    struct fast_mblock_man *allocator;  //for free
    struct fcfs_api_async_report_event *next; //for async_reporter's queue
} FCFSAPIAsyncReportEvent;

typedef struct fcfs_api_async_report_event_ptr_array {
    int alloc;
    int count;
    FCFSAPIAsyncReportEvent **events;
} FCFSAPIAsyncReportEventPtrArray;


#define FCFS_API_SET_OMP(omp, owner, m, euid, egid) \
    do {  \
        omp.mode = m;  \
        if ((owner).type == fcfs_api_owner_type_fixed) { \
            omp.uid = (owner).uid; \
            omp.gid = (owner).gid; \
        } else {  \
            omp.uid = euid; \
            omp.gid = egid; \
        }  \
    } while (0)


#ifdef __cplusplus
extern "C" {
#endif

    extern FCFSAPIContext g_fcfs_api_ctx;

#ifdef __cplusplus
}
#endif

#endif
