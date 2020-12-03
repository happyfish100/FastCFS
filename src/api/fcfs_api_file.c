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
#include <limits.h>
#include "fastcommon/shared_func.h"
#include "fastcommon/logger.h"
#include "fastcommon/sockopt.h"
#include "fastcommon/sched_thread.h"
#include "fcfs_api_util.h"
#include "fcfs_api_file.h"

#define FCFS_API_MAGIC_NUMBER    1588076578

static int file_truncate(FCFSAPIContext *ctx, const int64_t oid,
        const int64_t new_size, const int64_t tid);

static int deal_open_flags(FCFSAPIFileInfo *fi, FDIRDEntryFullName *fullname,
        const FDIRClientOwnerModePair *omp, const int64_t tid, int result)
{
    if (result == 0) {
        if (S_ISDIR(fi->dentry.stat.mode)) {
            return EISDIR;
        }
    }

    fi->tid = tid;
    if (!((fi->flags & O_WRONLY) || (fi->flags & O_RDWR))) {
        fi->offset = 0;
        return result;
    }

    if ((fi->flags & O_CREAT)) {
        if (result == 0) {
            if ((fi->flags & O_EXCL)) {
                return EEXIST;
            }
        } else if (result == ENOENT) {
            if ((result=fdir_client_create_dentry(fi->ctx->contexts.fdir,
                    fullname, omp, &fi->dentry)) != 0)
            {
                if (result == EEXIST) {
                    if ((fi->flags & O_EXCL)) {
                        return EEXIST;
                    }

                    if ((result=fdir_client_stat_dentry_by_path_ex(
                                    fi->ctx->contexts.fdir, fullname,
                                    LOG_DEBUG, &fi->dentry)) != 0)
                    {
                        return result;
                    }
                    if (S_ISDIR(fi->dentry.stat.mode)) {
                        return EISDIR;
                    }
                } else {
                    return result;
                }
            }
        } else {
            return result;
        }
    } else if (result != 0) {
        return result;
    }

    fi->write_notify.last_modified_time = fi->dentry.stat.mtime;
    if ((fi->flags & O_TRUNC)) {
        if (fi->dentry.stat.size > 0) {
            if ((result=file_truncate(fi->ctx, fi->dentry.
                            inode, 0, tid)) != 0)
            {
                return result;
            }

            fi->dentry.stat.size = 0;
        }
    }

    if ((fi->flags & O_APPEND)) {
        fi->offset = fi->dentry.stat.size;
    } else {
        fi->offset = 0;
    }
    return 0;
}

int fcfs_api_open_ex(FCFSAPIContext *ctx, FCFSAPIFileInfo *fi,
        const char *path, const int flags,
        const FCFSAPIFileContext *fctx)
{
    FDIRDEntryFullName fullname;
    FDIRClientOwnerModePair new_omp;
    int result;

    new_omp.uid = fctx->omp.uid;
    new_omp.gid = fctx->omp.gid;
    if ((fctx->omp.mode & S_IFMT)) {
        new_omp.mode = (fctx->omp.mode & (~S_IFMT)) | S_IFREG;
    } else {
        new_omp.mode = fctx->omp.mode | S_IFREG;
    }

    fi->ctx = ctx;
    fi->flags = flags;
    fi->sessions.flock.mconn = NULL;
    fullname.ns = ctx->ns;
    FC_SET_STRING(fullname.path, (char *)path);
    result = fdir_client_stat_dentry_by_path_ex(ctx->contexts.fdir,
            &fullname, LOG_DEBUG, &fi->dentry);
    if ((result=deal_open_flags(fi, &fullname, &new_omp,
                    fctx->tid, result)) != 0)
    {
        return result;
    }

    fi->magic = FCFS_API_MAGIC_NUMBER;
    return 0;
}

int fcfs_api_open_by_dentry_ex(FCFSAPIContext *ctx, FCFSAPIFileInfo *fi,
        const FDIRDEntryInfo *dentry, const int flags,
        const FCFSAPIFileContext *fctx)
{
    int result;

    fi->dentry = *dentry;
    fi->ctx = ctx;
    fi->flags = flags;
    fi->sessions.flock.mconn = NULL;
    result = 0;
    if ((result=deal_open_flags(fi, NULL, &fctx->omp,
                    fctx->tid, result)) != 0)
    {
        return result;
    }

    fi->magic = FCFS_API_MAGIC_NUMBER;
    return 0;
}

int fcfs_api_open_by_inode_ex(FCFSAPIContext *ctx, FCFSAPIFileInfo *fi,
        const int64_t inode, const int flags,
        const FCFSAPIFileContext *fctx)
{
    int result;

    if ((result=fdir_client_stat_dentry_by_inode(ctx->contexts.fdir,
                    inode, &fi->dentry)) != 0)
    {
        return result;
    }

    fi->ctx = ctx;
    fi->flags = flags;
    fi->sessions.flock.mconn = NULL;
    if ((result=deal_open_flags(fi, NULL, &fctx->omp,
                    fctx->tid, result)) != 0)
    {
        return result;
    }

    fi->magic = FCFS_API_MAGIC_NUMBER;
    return 0;
}

int fcfs_api_close(FCFSAPIFileInfo *fi)
{
    if (fi->magic != FCFS_API_MAGIC_NUMBER) {
        return EBADF;
    }

    if (fi->sessions.flock.mconn != NULL) {
        /* force close connection to unlock */
        fdir_client_close_session(&fi->sessions.flock, true);
    }

    fi->ctx = NULL;
    fi->magic = 0;
    return 0;
}

static int fcfs_api_file_report_size_and_time(
        FCFSAPIWriteDoneCallbackArg *callback_arg, int *flags,
        FDIRDEntryInfo *dentry)
{
    int64_t new_size;
    int current_time;

    new_size = callback_arg->arg.bs_key->block.offset +
        callback_arg->arg.bs_key->slice.offset +
        callback_arg->arg.write_bytes;
    if (new_size > callback_arg->extra.file_size) {
        *flags = FDIR_DENTRY_FIELD_MODIFIED_FLAG_FILE_SIZE |
            FDIR_DENTRY_FIELD_MODIFIED_FLAG_SPACE_END;
    } else {
        if (new_size > callback_arg->extra.space_end) {
            *flags = FDIR_DENTRY_FIELD_MODIFIED_FLAG_SPACE_END;
        } else {
            *flags = 0;
        }

        current_time = get_current_time();
        if (current_time > callback_arg->extra.last_modified_time) {
            *flags |= FDIR_DENTRY_FIELD_MODIFIED_FLAG_MTIME;
        }
    }

    if (callback_arg->arg.inc_alloc != 0)  {
        *flags |= FDIR_DENTRY_FIELD_MODIFIED_FLAG_INC_ALLOC;
    }

    if (*flags == 0) {
        return 0;
    } else {
        return fdir_client_set_dentry_size(callback_arg->extra.ctx->
                contexts.fdir, &callback_arg->extra.ctx->ns, callback_arg->
                arg.bs_key->block.oid, new_size, callback_arg->arg.inc_alloc,
                false, *flags, dentry);
    }
}

void fcfs_api_file_write_done_callback(FSAPIWriteDoneCallbackArg *callback_arg)
{
    int flags;
    FDIRDEntryInfo dentry;
    fcfs_api_file_report_size_and_time((FCFSAPIWriteDoneCallbackArg *)
            callback_arg, &flags, &dentry);
}

/*
static inline void print_block_slice_key(FSBlockSliceKeyInfo *bs_key)
{
    logInfo("block {oid: %"PRId64", offset: %"PRId64"}, "
            "slice {offset: %d, length: %d}", bs_key->block.oid,
            bs_key->block.offset, bs_key->slice.offset, bs_key->slice.length);
}
*/

static int do_pwrite(FCFSAPIFileInfo *fi, const char *buff,
        const int size, const int64_t offset, int *written_bytes,
        int *total_inc_alloc, const bool need_report_modified,
        const int64_t tid)
{
    FSAPIOperationContext op_ctx;
    FSAPIWriteBuffer wbuffer;
    FCFSAPIWriteDoneCallbackArg callback_arg;
    int64_t new_offset;
    int result;
    int remain;

    FS_API_SET_CTX_AND_TID_EX(op_ctx, fi->ctx->contexts.fsapi, tid);
    wbuffer.extra_data = &callback_arg.extra;
    callback_arg.extra.ctx = fi->ctx;

    *total_inc_alloc = *written_bytes = 0;
    new_offset = offset;
    fs_set_block_slice(&op_ctx.bs_key, fi->dentry.inode, offset, size);
    while (1) {
        //print_block_slice_key(&op_ctx.bs_key);
        callback_arg.arg.bs_key = &op_ctx.bs_key;
        callback_arg.extra.file_size = fi->dentry.stat.size;
        callback_arg.extra.space_end = fi->dentry.stat.space_end;
        callback_arg.extra.last_modified_time = fi->write_notify.last_modified_time;
        wbuffer.buff = buff + *written_bytes;
        if ((result=fs_api_slice_write(&op_ctx, &wbuffer, &callback_arg.
                        arg.write_bytes, &callback_arg.arg.inc_alloc)) != 0)
        {
            if (callback_arg.arg.write_bytes == 0) {
                break;
            }
        }

        new_offset += callback_arg.arg.write_bytes;
        *written_bytes += callback_arg.arg.write_bytes;
        *total_inc_alloc += callback_arg.arg.inc_alloc;
        if (need_report_modified) {
            if (wbuffer.combined) {
                if (new_offset > fi->dentry.stat.size) {
                    fi->dentry.stat.size = new_offset;
                }
                fi->write_notify.last_modified_time = get_current_time();
            } else {
                int flags;
                fcfs_api_file_report_size_and_time(&callback_arg,
                        &flags, &fi->dentry);
                if ((flags & FDIR_DENTRY_FIELD_MODIFIED_FLAG_MTIME) != 0) {
                    fi->write_notify.last_modified_time = get_current_time();
                }
            }
        }

        remain = size - *written_bytes;
        if (remain <= 0) {
            break;
        }

        if (callback_arg.arg.write_bytes == op_ctx.bs_key.slice.length) {
            /* fully completed */
            fs_next_block_slice_key(&op_ctx.bs_key, remain);
        } else {  //partially completed, try again the remain part
            fs_set_slice_size(&op_ctx.bs_key, new_offset, remain);
        }
    }

    return (*written_bytes > 0) ? 0 : EIO;
}

int fcfs_api_pwrite_ex(FCFSAPIFileInfo *fi, const char *buff,
        const int size, const int64_t offset, int *written_bytes,
        const int64_t tid)
{
    int total_inc_alloc;

    if (size == 0) {
        return 0;
    } else if (size < 0) {
        return EINVAL;
    }

    if (fi->magic != FCFS_API_MAGIC_NUMBER || !((fi->flags & O_WRONLY) ||
                (fi->flags & O_RDWR)))
    {
        return EBADF;
    }

    return do_pwrite(fi, buff, size, offset, written_bytes,
            &total_inc_alloc, true, tid);
}

int fcfs_api_write_ex(FCFSAPIFileInfo *fi, const char *buff,
        const int size, int *written_bytes, const int64_t tid)
{
    FDIRClientSession session;
    bool need_report_modified;
    int result;
    int total_inc_alloc;
    int flags;
    int64_t old_size;
    int64_t space_end;

    if (size == 0) {
        return 0;
    } else if (size < 0) {
        return EINVAL;
    }

    if (fi->magic != FCFS_API_MAGIC_NUMBER || !((fi->flags & O_WRONLY) ||
                (fi->flags & O_RDWR)))
    {
        return EBADF;
    }

    if ((fi->flags & O_APPEND)) {
        if ((result=fcfs_api_dentry_sys_lock(&session, fi->dentry.inode,
                        0, &old_size, &space_end)) != 0)
        {
            return result;
        }

        fi->offset = old_size;
        need_report_modified = false;
    } else {
        old_size = 0;
        need_report_modified = true;
    }

    if ((result=do_pwrite(fi, buff, size, fi->offset, written_bytes,
                    &total_inc_alloc, need_report_modified, tid)) == 0)
    {
        fi->offset += *written_bytes;
    }

    if ((fi->flags & O_APPEND)) {
        string_t *ns;
        if (fi->offset > old_size) {
            ns = &fi->ctx->ns; //set ns for update file_size, space_end etc.
        } else {
            ns = NULL;  //do NOT update
        }

        flags = FDIR_DENTRY_FIELD_MODIFIED_FLAG_FILE_SIZE |
            FDIR_DENTRY_FIELD_MODIFIED_FLAG_SPACE_END;
        fcfs_api_dentry_sys_unlock(&session, ns, fi->dentry.inode,
                false, old_size, fi->offset, total_inc_alloc, flags);
    }

    return result;
}

int fcfs_api_pread_ex(FCFSAPIFileInfo *fi, char *buff, const int size,
        const int64_t offset, int *read_bytes, const int64_t tid)
{
    FSAPIOperationContext op_ctx;
    int result;
    int current_read;
    int remain;
    int64_t current_offset;
    int64_t hole_bytes;
    int fill_bytes;

    *read_bytes = 0;
    if (size == 0) {
        return 0;
    } else if (size < 0) {
        return EINVAL;
    }

    if (fi->magic != FCFS_API_MAGIC_NUMBER || (fi->flags & O_WRONLY)) {
        return EBADF;
    }

    FS_API_SET_CTX_AND_TID_EX(op_ctx, fi->ctx->contexts.fsapi, tid);
    fs_set_block_slice(&op_ctx.bs_key, fi->dentry.inode, offset, size);
    while (1) {
        //print_block_slice_key(&op_ctx.bs_key);
        if ((result=fs_api_slice_read(&op_ctx, buff + *read_bytes,
                        &current_read)) != 0)
        {
            if (result == ENODATA) {
                result = 0;
            } else {
                break;
            }
        }

        while (current_read < op_ctx.bs_key.slice.length) {
            /* deal file hole caused by ftruncate and lseek */
            current_offset = offset + *read_bytes + current_read;
            if (current_offset == fi->dentry.stat.size) {
                break;
            }

            if (current_offset > fi->dentry.stat.size) {
                if ((result=fdir_client_stat_dentry_by_inode(fi->
                                ctx->contexts.fdir, fi->dentry.inode,
                                &fi->dentry)) != 0)
                {
                    return result;
                }
            }

            hole_bytes = fi->dentry.stat.size - current_offset;
            if (hole_bytes > 0) {
                if (current_read + hole_bytes > (int64_t)
                        op_ctx.bs_key.slice.length)
                {
                    fill_bytes = op_ctx.bs_key.slice.length - current_read;
                } else {
                    fill_bytes = hole_bytes;
                }

                /*
                logInfo("=====offset: %"PRId64", current_read: %d, "
                        "hole_bytes: %"PRId64", fill_bytes: %d, "
                        "buff offset: %d =====", offset, current_read,
                        hole_bytes, fill_bytes, *read_bytes + current_read);
                        */

                memset(buff + *read_bytes + current_read, 0, fill_bytes);
                current_read += fill_bytes;
            }

            break;
        }

        /*
        logInfo("current read: %d, total read: %d, slice length: %d",
                current_read, *read_bytes, op_ctx.bs_key.slice.length);
                */

        *read_bytes += current_read;
        remain = size - *read_bytes;
        if (remain <= 0 || current_read < op_ctx.bs_key.slice.length) {
            break;
        }

        fs_next_block_slice_key(&op_ctx.bs_key, remain);
    }

    return result;
}

int fcfs_api_read_ex(FCFSAPIFileInfo *fi, char *buff, const int size,
        int *read_bytes, const int64_t tid)
{
    int result;

    if ((result=fcfs_api_pread_ex(fi, buff, size, fi->offset,
                    read_bytes, tid)) != 0)
    {
        return result;
    }

    fi->offset += *read_bytes;
    return 0;
}

static int do_truncate(FCFSAPIContext *ctx, const int64_t oid,
        const int64_t old_space_end, const int64_t offset,
        const int64_t length, int64_t *total_dec_alloc, const int64_t tid)
{
    FSAPIOperationContext op_ctx;
    int64_t remain;
    int dec_alloc;
    int result;

    *total_dec_alloc = 0;
    if (offset >= old_space_end) {
        return 0;
    }

    if (offset + length > old_space_end) {
        remain = old_space_end - offset;
    } else {
        remain = length;
    }

    FS_API_SET_CTX_AND_TID_EX(op_ctx, ctx->contexts.fsapi, tid);
    fs_set_block_slice(&op_ctx.bs_key, oid, offset, remain);
    while (1) {
        //print_block_slice_key(&op_ctx.bs_key);
        if (op_ctx.bs_key.slice.length == FS_FILE_BLOCK_SIZE) {
            result = fs_api_block_delete(&op_ctx,
                    &dec_alloc);
        } else {
            result = fs_api_slice_delete(&op_ctx,
                    &dec_alloc);
        }
        if (result == 0) {
            *total_dec_alloc -= dec_alloc;
        } else if (result == ENOENT) {
            result = 0;
        } else if (result != 0) {
            break;
        }

        remain -= op_ctx.bs_key.slice.length;
        if (remain <= 0) {
            break;
        }

        fs_next_block_slice_key(&op_ctx.bs_key, remain);
    }

    return result;
}

static int do_allocate(FCFSAPIContext *ctx, const int64_t oid,
        const int64_t offset, const int64_t length,
        int64_t *total_inc_alloc, const int64_t tid)
{
    FSAPIOperationContext op_ctx;
    int64_t remain;
    int inc_alloc;
    int result;

    *total_inc_alloc = 0;
    if (length <= 0) {
        return 0;
    }

    FS_API_SET_CTX_AND_TID_EX(op_ctx, ctx->contexts.fsapi, tid);
    remain = length;
    fs_set_block_slice(&op_ctx.bs_key, oid, offset, remain);
    while (1) {
        //print_block_slice_key(&op_ctx.bs_key);
        if ((result=fs_api_slice_allocate(&op_ctx, &inc_alloc)) != 0) {
            break;
        }
        *total_inc_alloc += inc_alloc;

        remain -= op_ctx.bs_key.slice.length;
        if (remain <= 0) {
            break;
        }

        fs_next_block_slice_key(&op_ctx.bs_key, remain);
    }

    return result;
}

static int file_truncate(FCFSAPIContext *ctx, const int64_t oid,
        const int64_t new_size, const int64_t tid)
{
    FDIRClientSession session;
    int64_t old_size;
    string_t *ns;
    int64_t alloc_bytes;
    int64_t space_end;
    int result;
    int flags;
    int unlock_res;

    if (new_size < 0) {
        return EINVAL;
    }

    if ((result=fcfs_api_dentry_sys_lock(&session, oid, 0,
                    &old_size, &space_end)) != 0)
    {
        return result;
    }

    if (new_size >= old_size) {
        result = 0;
        alloc_bytes = 0;
        if (new_size == old_size) {
            flags = 0;
        } else {
            flags = FDIR_DENTRY_FIELD_MODIFIED_FLAG_FILE_SIZE;
        }
    } else {
        result = do_truncate(ctx, oid, space_end, new_size,
                old_size - new_size, &alloc_bytes, tid);

        flags = FDIR_DENTRY_FIELD_MODIFIED_FLAG_FILE_SIZE;
        if (new_size < space_end) {
            flags |= FDIR_DENTRY_FIELD_MODIFIED_FLAG_SPACE_END;
        }
    }

    if (result == 0) {
        ns = &ctx->ns;  //set ns for update file_size, space_end etc.
    } else {
        ns = NULL;  //do NOT update
    }
    unlock_res = fcfs_api_dentry_sys_unlock(&session, ns, oid,
            true, old_size, new_size, alloc_bytes, flags);
    return result == 0 ? unlock_res : result;
}

int fcfs_api_ftruncate_ex(FCFSAPIFileInfo *fi, const int64_t new_size,
        const int64_t tid)
{
    if (fi->magic != FCFS_API_MAGIC_NUMBER || !((fi->flags & O_WRONLY) ||
                (fi->flags & O_RDWR)))
    {
        return EBADF;
    }

    return file_truncate(fi->ctx, fi->dentry.inode, new_size, tid);
}

static int get_regular_file_inode(FCFSAPIContext *ctx, const char *path,
        int64_t *inode)
{
    FDIRDEntryFullName fullname;
    FDIRDEntryInfo dentry;
    int result;

    fullname.ns = ctx->ns;
    FC_SET_STRING(fullname.path, (char *)path);
    if ((result=fdir_client_stat_dentry_by_path_ex(ctx->contexts.fdir,
                    &fullname, LOG_DEBUG, &dentry)) != 0)
    {
        return result;
    }
    if (S_ISDIR(dentry.stat.mode)) {
        return EISDIR;
    }

    *inode = dentry.inode;
    return 0;
}

int fcfs_api_truncate_ex(FCFSAPIContext *ctx, const char *path,
        const int64_t new_size, const FCFSAPIFileContext *fctx)
{
    int result;
    int64_t inode;

    if ((result=get_regular_file_inode(ctx, path, &inode)) != 0) {
        return result;
    }

    return file_truncate(ctx, inode, new_size, fctx->tid);
}

int fcfs_api_unlink_ex(FCFSAPIContext *ctx, const char *path,
        const FCFSAPIFileContext *fctx)
{
    FDIRDEntryFullName fullname;
    FDIRDEntryInfo dentry;
    int result;

    fullname.ns = ctx->ns;
    FC_SET_STRING(fullname.path, (char *)path);
    if ((result=fdir_client_stat_dentry_by_path_ex(ctx->contexts.fdir,
                    &fullname, LOG_DEBUG, &dentry)) != 0)
    {
        return result;
    }

    if (S_ISDIR(dentry.stat.mode)) {
        return EISDIR;
    }

    if ((result=fs_api_unlink_file(ctx->contexts.fsapi, dentry.inode,
                    dentry.stat.size, fctx->tid)) == 0)
    {
        result = fdir_client_remove_dentry(ctx->contexts.fdir, &fullname);
    }

    return result;
}

#define calc_file_offset(fi, offset, whence, new_offset)  \
    calc_file_offset_ex(fi, offset, whence, false, new_offset)

static inline int calc_file_offset_ex(FCFSAPIFileInfo *fi,
        const int64_t offset, const int whence,
        const bool refresh_fsize, int64_t *new_offset)
{
    int result;

    switch (whence) {
        case SEEK_SET:
            if (offset < 0) {
                return EINVAL;
            }
            *new_offset = offset;
            break;
        case SEEK_CUR:
            *new_offset = fi->offset + offset;
            if (*new_offset < 0) {
                return EINVAL;
            }
            break;
        case SEEK_END:
            if (refresh_fsize) {
                if ((result=fdir_client_stat_dentry_by_inode(
                                fi->ctx->contexts.fdir,
                                fi->dentry.inode, &fi->dentry)) != 0)
                {
                    return result;
                }
            }

            *new_offset = fi->dentry.stat.size + offset;
            if (*new_offset < 0) {
                return EINVAL;
            }
            break;
        default:
            logError("file: "__FILE__", line: %d, "
                    "invalid whence: %d", __LINE__, whence);
            return EINVAL;
    }

    return 0;
}

int fcfs_api_lseek(FCFSAPIFileInfo *fi, const int64_t offset, const int whence)
{
    int64_t new_offset;
    int result;

    if (fi->magic != FCFS_API_MAGIC_NUMBER) {
        return EBADF;
    }

    if ((result=calc_file_offset_ex(fi, offset, whence,
                    true, &new_offset)) != 0)
    {
        return result;
    }

    fi->offset = new_offset;
    return 0;
}

static void fill_stat(const FDIRDEntryInfo *dentry, struct stat *buf)
{
    memset(buf, 0, sizeof(struct stat));
    buf->st_ino = dentry->inode;
    buf->st_mode = dentry->stat.mode;
    buf->st_size = dentry->stat.size;
    buf->st_mtime = dentry->stat.mtime;
    buf->st_ctime = dentry->stat.ctime;
}

int fcfs_api_fstat(FCFSAPIFileInfo *fi, struct stat *buf)
{
    int result;

    if (fi->magic != FCFS_API_MAGIC_NUMBER) {
        return EBADF;
    }

    if ((result=fdir_client_stat_dentry_by_inode(fi->ctx->contexts.
                    fdir, fi->dentry.inode, &fi->dentry)) != 0)
    {
        return result;
    }

    fill_stat(&fi->dentry, buf);
    return 0;
}

int fcfs_api_stat_ex(FCFSAPIContext *ctx, const char *path, struct stat *buf)
{
    int result;
    FDIRDEntryFullName fullname;
    FDIRDEntryInfo dentry;

    fullname.ns = ctx->ns;
    FC_SET_STRING(fullname.path, (char *)path);
    if ((result=fdir_client_stat_dentry_by_path_ex(ctx->contexts.fdir,
                    &fullname, LOG_DEBUG, &dentry)) != 0)
    {
        return result;
    }

    fill_stat(&dentry, buf);
    return 0;
}

static inline int flock_lock(FCFSAPIFileInfo *fi, const int operation,
        const int64_t owner_id, const pid_t pid)
{
    int result;
    const int64_t offset = 0;
    const int64_t length = 0;

    if ((result=fdir_client_init_session(fi->ctx->contexts.fdir,
                    &fi->sessions.flock)) != 0)
    {
        return result;
    }

    if ((result=fdir_client_flock_dentry_ex2(&fi->sessions.flock,
                    fi->dentry.inode, operation, offset, length,
                    owner_id, pid)) != 0)
    {
        fdir_client_close_session(&fi->sessions.flock, result != 0);
    }

    return result;
}

static inline int flock_unlock(FCFSAPIFileInfo *fi, const int operation,
        const int64_t owner_id, const pid_t pid)
{
    int result;
    const int64_t offset = 0;
    const int64_t length = 0;

    result = fdir_client_flock_dentry_ex2(&fi->sessions.flock,
            fi->dentry.inode, operation, offset, length, owner_id, pid);
    fdir_client_close_session(&fi->sessions.flock, result != 0);
    return result;
}

int fcfs_api_flock_ex2(FCFSAPIFileInfo *fi, const int operation,
        const int64_t owner_id, const pid_t pid)
{
    if (fi->magic != FCFS_API_MAGIC_NUMBER) {
        return EBADF;
    }

    if ((operation & LOCK_UN)) {
        if (fi->sessions.flock.mconn == NULL) {
            return ENOENT;
        }
        return flock_unlock(fi, operation, owner_id, pid);
    } else if ((operation & LOCK_SH) || (operation & LOCK_EX)) {
        if (fi->sessions.flock.mconn != NULL) {
            logError("file: "__FILE__", line: %d, "
                    "flock on inode: %"PRId64" already exist",
                    __LINE__, fi->dentry.inode);
            return EEXIST;
        }
        return flock_lock(fi, operation, owner_id, pid);
    } else {
        return EINVAL;
    }
}

static inline int fcntl_lock(FCFSAPIFileInfo *fi, const int operation,
        const int64_t offset, const int64_t length,
        const int64_t owner_id, const pid_t pid)
{
    int result;
    if ((result=fdir_client_init_session(fi->ctx->contexts.fdir,
                    &fi->sessions.flock)) != 0)
    {
        return result;
    }

    if ((result=fdir_client_flock_dentry_ex2(&fi->sessions.flock,
                    fi->dentry.inode, operation, offset, length,
                    owner_id, pid)) != 0)
    {
        fdir_client_close_session(&fi->sessions.flock, result != 0);
    }

    return result;
}

static inline int fcntl_unlock(FCFSAPIFileInfo *fi, const int operation,
        const int64_t offset, const int64_t length,
        const int64_t owner_id, const pid_t pid)
{
    int result;
    result = fdir_client_flock_dentry_ex2(&fi->sessions.flock,
            fi->dentry.inode, operation, offset, length, owner_id, pid);
    fdir_client_close_session(&fi->sessions.flock, result != 0);
    return result;
}

static inline int fcntl_type_to_flock_op(const short type, int *operation)
{
    switch (type) {
        case F_RDLCK:
            *operation = LOCK_SH;
            break;
        case F_WRLCK:
            *operation = LOCK_EX;
            break;
        case F_UNLCK:
            *operation = LOCK_UN;
            break;
        default:
            return EINVAL;
    }

    return 0;
}

static inline int flock_op_to_fcntl_type(const int operation, short *type)
{
    switch (operation) {
        case LOCK_SH:
            *type = F_RDLCK;
            break;
        case LOCK_EX:
            *type = F_WRLCK;
            break;
        case LOCK_UN:
            *type = F_UNLCK;
            break;
        default:
            return EINVAL;
    }

    return 0;
}

int fcfs_api_setlk(FCFSAPIFileInfo *fi, const struct flock *lock,
        const int64_t owner_id)
{
    int operation;
    int result;
    int64_t offset;

    if (fi->magic != FCFS_API_MAGIC_NUMBER) {
        return EBADF;
    }

    if ((result=fcntl_type_to_flock_op(lock->l_type, &operation)) != 0) {
        return result;
    }

    if ((result=calc_file_offset(fi, lock->l_start,
                    lock->l_whence, &offset)) != 0)
    {
        return result;
    }

    if (operation == LOCK_UN) {
        if (fi->sessions.flock.mconn == NULL) {
            return ENOENT;
        }
        return fcntl_unlock(fi, operation, offset, lock->l_len,
                owner_id, lock->l_pid);
    } else {
        if (fi->sessions.flock.mconn != NULL) {
            logError("file: "__FILE__", line: %d, "
                    "flock on inode: %"PRId64" already exist",
                    __LINE__, fi->dentry.inode);
            return EEXIST;
        }
        return fcntl_lock(fi, operation, offset, lock->l_len,
                owner_id, lock->l_pid);
    }
}

int fcfs_api_getlk(FCFSAPIFileInfo *fi, struct flock *lock, int64_t *owner_id)
{
    int operation;
    int result;
    int64_t offset;
    int64_t length;

    if (fi->magic != FCFS_API_MAGIC_NUMBER) {
        return EBADF;
    }

    if ((result=fcntl_type_to_flock_op(lock->l_type, &operation)) != 0) {
        return result;
    }

    if ((result=calc_file_offset(fi, lock->l_start,
                    lock->l_whence, &offset)) != 0)
    {
        return result;
    }

    if (operation == LOCK_UN) {
        return EINVAL;
    }

    length = lock->l_len;
    if ((result=fdir_client_getlk_dentry(fi->ctx->contexts.fdir,
                    fi->dentry.inode, &operation, &offset, &length,
                    owner_id, &lock->l_pid)) == 0)
    {
        flock_op_to_fcntl_type(operation, &lock->l_type);
        lock->l_whence = SEEK_SET;
        lock->l_start = offset;
        lock->l_len = length;
    }

    return result;
}

int fcfs_api_fallocate_ex(FCFSAPIFileInfo *fi, const int mode,
        const int64_t offset, const int64_t length, const int64_t tid)
{
    FDIRClientSession session;
    int64_t old_size;
    int64_t new_size;
    int64_t space_end;
    int64_t alloc_bytes;
    string_t *ns;
    int op;
    int flags;
    int result;
    int unlock_res;

    if (offset < 0 || length < 0) {
        return EINVAL;
    }

    if (fi->magic != FCFS_API_MAGIC_NUMBER) {
        return EBADF;
    }

    op = mode & (~FALLOC_FL_KEEP_SIZE);
    if (!(op == 0 || op == FALLOC_FL_PUNCH_HOLE)) {
        return EOPNOTSUPP;
    }

    if (length == 0) {
        return 0;
    }

    if ((result=fcfs_api_dentry_sys_lock(&session, fi->dentry.inode,
                    0, &old_size, &space_end)) != 0)
    {
        return result;
    }

    flags = 0;
    if (op == 0) {   //allocate space
        result = do_allocate(fi->ctx, fi->dentry.inode,
                offset, length, &alloc_bytes, tid);
        new_size = offset + length;
        if (new_size > space_end) {
            flags |= FDIR_DENTRY_FIELD_MODIFIED_FLAG_SPACE_END;
        }
        if (new_size > old_size) {
            flags |= (mode & FALLOC_FL_KEEP_SIZE) ? 0 :
                FDIR_DENTRY_FIELD_MODIFIED_FLAG_FILE_SIZE;
        }
    } else {  //deallocate space
        result = do_truncate(fi->ctx, fi->dentry.inode, space_end,
                offset, length, &alloc_bytes, tid);
        if (offset + length >= old_size) {
            new_size = offset;
            flags |= (mode & FALLOC_FL_KEEP_SIZE) ? 0 :
                FDIR_DENTRY_FIELD_MODIFIED_FLAG_FILE_SIZE;
            if (new_size < space_end) {
                flags |= FDIR_DENTRY_FIELD_MODIFIED_FLAG_SPACE_END;
            }
        } else {
            if (offset < space_end && offset + length >= space_end) {
                new_size = offset;
                flags |= FDIR_DENTRY_FIELD_MODIFIED_FLAG_SPACE_END;
            } else {
                new_size = old_size;
            }
        }
    }

    if (result == 0) {
        ns = &fi->ctx->ns; //set ns for update file_size, space_end etc.
    } else {
        ns = NULL;  //do NOT update
    }
    unlock_res = fcfs_api_dentry_sys_unlock(&session, ns, fi->dentry.inode,
            true, old_size, new_size, alloc_bytes, flags);

    return result == 0 ? unlock_res : result;
}

int fcfs_api_rename_ex(FCFSAPIContext *ctx, const char *old_path,
        const char *new_path, const int flags,
        const FCFSAPIFileContext *fctx)
{
    FDIRDEntryFullName src_fullname;
    FDIRDEntryFullName dest_fullname;
    FDIRDEntryInfo dentry;
    FDIRDEntryInfo *pe;
    int result;

    src_fullname.ns = ctx->ns;
    FC_SET_STRING(src_fullname.path, (char *)old_path);
    dest_fullname.ns = ctx->ns;
    FC_SET_STRING(dest_fullname.path, (char *)new_path);

    pe = &dentry;
    if ((result=fdir_client_rename_dentry_ex(ctx->contexts.fdir,
                    &src_fullname, &dest_fullname, flags, &pe)) != 0)
    {
        return result;
    }

    if (pe != NULL && S_ISREG(pe->stat.mode)) {
        fs_api_unlink_file(ctx->contexts.fsapi,
                pe->inode, pe->stat.size, fctx->tid);
    }

    return result;
}

int fcfs_api_symlink_ex(FCFSAPIContext *ctx, const char *target,
        const char *path, const FDIRClientOwnerModePair *omp)
{
    FDIRDEntryFullName fullname;
    string_t link;
    FDIRDEntryInfo dentry;

    fullname.ns = ctx->ns;
    FC_SET_STRING(fullname.path, (char *)path);
    FC_SET_STRING(link, (char *)target);
    return fdir_client_symlink_dentry(ctx->contexts.fdir,
            &link, &fullname, omp, &dentry);
}

int fcfs_api_readlink(FCFSAPIContext *ctx, const char *path,
        char *buff, const int size)
{
    FDIRDEntryFullName fullname;
    string_t link;

    fullname.ns = ctx->ns;
    FC_SET_STRING(fullname.path, (char *)path);
    link.str = buff;
    return fdir_client_readlink_by_path(ctx->contexts.fdir,
        &fullname, &link, size);
}

int fcfs_api_link_ex(FCFSAPIContext *ctx, const char *old_path,
        const char *new_path, const FDIRClientOwnerModePair *omp)
{
    FDIRDEntryFullName src_fullname;
    FDIRDEntryFullName dest_fullname;
    FDIRDEntryInfo dentry;

    src_fullname.ns = ctx->ns;
    FC_SET_STRING(src_fullname.path, (char *)old_path);
    dest_fullname.ns = ctx->ns;
    FC_SET_STRING(dest_fullname.path, (char *)new_path);

    return fdir_client_link_dentry(ctx->contexts.fdir,
            &src_fullname, &dest_fullname, omp, &dentry);
}

int fcfs_api_statvfs_ex(FCFSAPIContext *ctx, const char *path,
        struct statvfs *stbuf)
{
    int result;
    FSClusterSpaceStat sstat;
    FDIRInodeStat istat;

    if ((result=fs_api_cluster_space_stat(ctx->
                    contexts.fsapi, &sstat)) != 0)
    {
        return result;
    }

    if ((result=fdir_client_namespace_stat(ctx->contexts.
                    fdir, &ctx->ns, &istat)) != 0)
    {
        return result;
    }

    stbuf->f_bsize = stbuf->f_frsize = 512;
    stbuf->f_blocks = sstat.total / stbuf->f_frsize;
    stbuf->f_bfree = (sstat.total - sstat.used) / stbuf->f_frsize;
    stbuf->f_bavail = sstat.avail / stbuf->f_frsize;

    stbuf->f_files = istat.total;
    stbuf->f_ffree = istat.total - istat.used;
    stbuf->f_favail = istat.avail;

    stbuf->f_namemax = NAME_MAX;
    stbuf->f_fsid = 0;
    stbuf->f_flag = 0;
    return 0;
}
