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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <pwd.h>
#include <grp.h>
#include "fastcommon/logger.h"
#include "global.h"
#include "groups_htable.h"
#include "getgroups.h"
#include "fuse_wrapper.h"

#define FS_READDIR_BUFFER_INIT_NONE        0
#define FS_READDIR_BUFFER_INIT_NORMAL      1
#define FS_READDIR_BUFFER_INIT_PLUS        2

struct fuse_conn_info_opts *g_fuse_cinfo_opts;
static struct fast_mblock_man fh_allocator;

static inline void set_operator_by_req(const struct fuse_ctx *fctx,
        FDIRDentryOperator *oper, char *buff)
{
    int count;

    if (g_fcfs_api_ctx.owner.type == fcfs_api_owner_type_fixed) {
        *oper = g_fcfs_api_ctx.owner.oper;
        return;
    }

    if (fctx->uid == 0 || !ADDITIONAL_GROUPS_ENABLED) {
        FDIR_SET_OPERATOR(*oper, fctx->uid, fctx->gid, 0, buff);
        return;
    }

    if (GROUPS_CACHE_ENABLED) {
        if (fcfs_groups_htable_find(fctx->pid, fctx->uid,
                    fctx->gid, &count, buff) != 0)
        {
            count = fcfs_get_groups(fctx->pid, fctx->uid, fctx->gid, buff);
            fcfs_groups_htable_insert(fctx->pid, fctx->uid,
                    fctx->gid, count, buff);

            /*
            logInfo("SET pid: %d, uid: %d, gid: %d, groups count: %d, "
                    "first group: %d", fctx->pid, fctx->uid, fctx->gid,
                    count, count > 0 ? buff2int(buff) : -1);
                    */
        } else {
            /*
            logInfo("get pid: %d, uid: %d, gid: %d, groups count: %d, "
                    "first group: %d", fctx->pid, fctx->uid, fctx->gid,
                    count, count > 0 ? buff2int(buff) : -1);
                    */
        }
    } else {
        count = fcfs_get_groups(fctx->pid, fctx->uid, fctx->gid, buff);
    }
    FDIR_SET_OPERATOR(*oper, fctx->uid, fctx->gid, count, buff);
}

static inline void fill_entry_param(const FDIRDEntryInfo *dentry,
        struct fuse_entry_param *param)
{
    memset(param, 0, sizeof(*param));
    param->ino = dentry->inode;
    param->attr_timeout = g_fuse_global_vars.attribute_timeout;
    param->entry_timeout = g_fuse_global_vars.entry_timeout;
    fcfs_api_fill_stat(dentry, &param->attr);
}

static inline int fs_convert_inode(fuse_req_t req,
        const fuse_ino_t ino, int64_t *new_inode)
{
    int result;
    FDIRDentryOperator oper;
    static int64_t root_inode = 0;

    if (ino == FUSE_ROOT_ID) {
        if (root_inode == 0) {
            char groups_buff[FDIR_MAX_USER_GROUP_BYTES];
            set_operator_by_req(fuse_req_ctx(req), &oper, groups_buff);
            if ((result=fcfs_api_lookup_inode_by_path("/",
                            &oper, new_inode)) != 0)
            {
                return result;
            }
            root_inode = *new_inode;
        } else {
            *new_inode = root_inode;
        }
    } else {
        *new_inode = ino;
    }
    return 0;
}

#define SET_OPER_INODE_PAIR(req, oino, _inode) \
    char groups_buff[FDIR_MAX_USER_GROUP_BYTES]; \
    do { \
        const struct fuse_ctx *fctx; \
        fctx = fuse_req_ctx(req); \
        set_operator_by_req(fctx, &oino.oper, groups_buff); \
        oino.inode = _inode; \
    } while (0)


#define SET_OPER_PNAME_PAIR(req, opname, parent_inode, name) \
    char groups_buff[FDIR_MAX_USER_GROUP_BYTES]; \
    do { \
        set_operator_by_req(fuse_req_ctx(req), &opname.oper, groups_buff); \
        FDIR_SET_DENTRY_PNAME_STR(&opname.pname, parent_inode, name); \
    } while (0)

static inline void do_reply_attr(fuse_req_t req, FDIRDEntryInfo *dentry)
{
    struct stat stat;
    memset(&stat, 0, sizeof(stat));
    fcfs_api_fill_stat(dentry, &stat);
    fuse_reply_attr(req, &stat, g_fuse_global_vars.attribute_timeout);
}

static void fs_do_getattr(fuse_req_t req, fuse_ino_t ino,
			     struct fuse_file_info *fi)
{
    const int flags = 0;
    int result;
    int64_t new_inode;
    FDIRClientOperInodePair oino;
    FDIRDEntryInfo dentry;

    if (fs_convert_inode(req, ino, &new_inode) != 0) {
        fuse_reply_err(req, ENOENT);
        return;
    }

    SET_OPER_INODE_PAIR(req, oino, new_inode);
    if ((result=fcfs_api_stat_dentry_by_inode(&oino, flags, &dentry)) == 0) {
        do_reply_attr(req, &dentry);
    } else {
        fuse_reply_err(req, result);
    }
}

void fs_do_setattr(fuse_req_t req, fuse_ino_t ino, struct stat *attr,
             int to_set, struct fuse_file_info *fi)
{
    const int flags = 0;
    int result;
    int64_t new_inode;
    FDIRStatModifyFlags options;
    FDIRClientOperInodePair oino;
    const struct fuse_ctx *fctx;
    FDIRDEntryInfo *pe;
    FDIRDEntryInfo dentry;

    /*
    logInfo("=====file: "__FILE__", line: %d, func: %s, "
            "ino: %"PRId64", to_set: %d, fi: %p, size bit: %d====",
            __LINE__, __FUNCTION__, ino, to_set, fi,
            (to_set & FUSE_SET_ATTR_SIZE));
            */

    if (fs_convert_inode(req, ino, &new_inode) != 0) {
        fuse_reply_err(req, ENOENT);
        return;
    }
    fctx = fuse_req_ctx(req);
    SET_OPER_INODE_PAIR(req, oino, new_inode);

    options.flags = 0;
    if ((to_set & FUSE_SET_ATTR_MODE)) {
        options.mode = 1;
    }

    if ((to_set & FUSE_SET_ATTR_UID)) {
        options.uid = 1;
    }

    if ((to_set & FUSE_SET_ATTR_GID)) {
        options.gid = 1;
    }

    if ((to_set & FUSE_SET_ATTR_SIZE)) {
        FCFSAPIFileInfo *fh;

        if (fi == NULL) {
            if ((result=fcfs_api_file_truncate(&oino, attr->st_size,
                            fctx->pid, &dentry)) != 0)
            {
                fuse_reply_err(req, result);
                return;
            }

            dentry.stat.size = attr->st_size;
            pe = &dentry;
        } else {
            fh = (FCFSAPIFileInfo *)fi->fh;
            if (fh == NULL) {
                fuse_reply_err(req, EBADF);
                return;
            }

            /*
            logInfo("file: "__FILE__", line: %d, func: %s, "
                    "SET file size from %"PRId64" to: %"PRId64,
                    __LINE__, __FUNCTION__, fh->dentry.stat.size,
                    (int64_t)attr->st_size);
                    */

            if ((result=fcfs_api_ftruncate_ex(fh,
                            attr->st_size, fctx->pid)) != 0)
            {
                fuse_reply_err(req, result);
                return;
            }

            fh->dentry.stat.size = attr->st_size;
            pe = &fh->dentry;
        }
    } else {
        pe = NULL;
    }

    if ((to_set & FUSE_SET_ATTR_CTIME)) {
        options.ctime = 1;
    }

    if ((to_set & (FUSE_SET_ATTR_ATIME | FUSE_SET_ATTR_ATIME_NOW))) {
        options.atime = 1;
        if ((to_set & FUSE_SET_ATTR_ATIME_NOW)) {
            options.atime_now = 1;
        }
    }

    if ((to_set & (FUSE_SET_ATTR_MTIME | FUSE_SET_ATTR_MTIME_NOW))) {
        options.mtime = 1;
        if ((to_set & FUSE_SET_ATTR_MTIME_NOW)) {
            options.mtime_now = 1;
        } else if (options.atime_now && attr->st_atime == attr->st_mtime) {
            options.mtime_now = 1;
        }
    }

    /*
    logInfo("file: "__FILE__", line: %d, func: %s, new_inode: %"PRId64", "
            "flags: %"PRId64", to_set: %d, atime bit: %d, mtime bit: %d, ctime bit: %d, "
            "uid bit: %d, uid_to_set: %d, gid bit: %d, gid_to_set: %d, "
            "oper {uid: %d, gid: %d}, atime_now: %d, atime: %ld, "
            "mtime_now: %d, mtime: %ld", __LINE__, __FUNCTION__,
            new_inode, options.flags, to_set, (to_set & FUSE_SET_ATTR_ATIME),
            (to_set & FUSE_SET_ATTR_MTIME), options.ctime, options.uid,
            attr->st_uid, options.gid, attr->st_gid, oino.oper.uid,
            oino.oper.gid, (to_set & FUSE_SET_ATTR_ATIME_NOW),
            attr->st_atim.tv_sec, (to_set & FUSE_SET_ATTR_MTIME_NOW),
            attr->st_mtim.tv_sec);
            */

    if (options.flags == 0) {
        if (pe == NULL) {
            pe = &dentry;
            result = fcfs_api_stat_dentry_by_inode(&oino, flags, &dentry);
        } else {
            result = 0;
        }
    } else {
        pe = &dentry;
        result = fcfs_api_modify_stat_by_inode(&oino,
                attr, options.flags, flags, &dentry);
    }
    if (result != 0) {
        fuse_reply_err(req, result);
    } else {
        do_reply_attr(req, pe);
    }
}

static void fs_do_lookup(fuse_req_t req, fuse_ino_t parent, const char *name)
{
    const int flags = 0;
    int result;
    int64_t parent_inode;
    FDIRDEntryInfo dentry;
    FDIRClientOperPnamePair opname;
    struct fuse_entry_param param;

    if (fs_convert_inode(req, parent, &parent_inode) != 0) {
        fuse_reply_err(req, ENOENT);
        return;
    }

    SET_OPER_PNAME_PAIR(req, opname, parent_inode, name);
    if ((result=fcfs_api_stat_dentry_by_pname(&opname, flags, &dentry)) != 0) {
        /*
        logError("file: "__FILE__", line: %d, func: %s, "
                "parent: %"PRId64", name: %s(%d), result: %d",
                __LINE__, __FUNCTION__, parent, name, (int)strlen(name), result);
                */
        fuse_reply_err(req, result);
        return;
    }

    fill_entry_param(&dentry, &param);
    fuse_reply_entry(req, &param);
}

static int dentry_list_to_buff(fuse_req_t req, FCFSAPIOpendirSession *session)
{
    FDIRClientDentry *cd;
    FDIRClientDentry *end;
    struct stat stat;
    struct fuse_entry_param param;
    int result;
    int len;
    int next_offset;
    char name[NAME_MAX];

    fast_buffer_reset(&session->buffer);
    if (session->array.count == 0) {
        return 0;
    }

    end = session->array.entries + session->array.count;
    for (cd=session->array.entries; cd<end; cd++) {
        snprintf(name, sizeof(name), "%.*s",
                cd->name.len, cd->name.str);
        if (session->btype == FS_READDIR_BUFFER_INIT_NORMAL) {
            len = fuse_add_direntry(req, NULL, 0, name, NULL, 0);
        } else {
            len = fuse_add_direntry_plus(req, NULL, 0, name, NULL, 0);
        }
        next_offset = session->buffer.length + len;
        if (next_offset > session->buffer.alloc_size) {
            if ((result=fast_buffer_set_capacity(&session->buffer,
                            next_offset)) != 0)
            {
                return result;
            }
        }

        if (session->btype == FS_READDIR_BUFFER_INIT_NORMAL) {
            memset(&stat, 0, sizeof(stat));
            fcfs_api_fill_stat(&cd->dentry, &stat);
            fuse_add_direntry(req, session->buffer.data +
                    session->buffer.length, session->buffer.
                    alloc_size - session->buffer.length,
                    name, &stat, next_offset);
        } else {
            fill_entry_param(&cd->dentry, &param);
            fuse_add_direntry_plus(req, session->buffer.data +
                    session->buffer.length, session->buffer.
                    alloc_size - session->buffer.length,
                    name, &param, next_offset);
        }

        session->buffer.length = next_offset;
    }

    return 0;
}

static void fs_do_opendir(fuse_req_t req, fuse_ino_t ino,
        struct fuse_file_info *fi)
{
    int64_t new_inode;
    FCFSAPIOpendirSession *session;
    FDIRClientOperInodePair oino;
    int result;

    if (fs_convert_inode(req, ino, &new_inode) != 0) {
        fuse_reply_err(req, ENOENT);
        return;
    }
    if ((session=fcfs_api_alloc_opendir_session()) == NULL) {
        fuse_reply_err(req, ENOMEM);
        return;
    }

    SET_OPER_INODE_PAIR(req, oino, new_inode);
    session->btype = FS_READDIR_BUFFER_INIT_NONE;
    if ((result=fcfs_api_list_dentry_by_inode(&oino,
                    &session->array)) != 0)
    {
        fcfs_api_free_opendir_session(session);
        fuse_reply_err(req, result);
        return;
    }

    fi->fh = (long)session;
    fuse_reply_open(req, fi);
}

static void do_readdir(fuse_req_t req, fuse_ino_t ino, size_t size,
        off_t offset, struct fuse_file_info *fi, const int buffer_type)
{
    FCFSAPIOpendirSession *session;
    int result;

    /*
    logInfo("file: "__FILE__", line: %d, func: %s, buffer_type: %d, "
            "ino: %"PRId64", fh: %"PRId64", offset: %"PRId64", size: %"PRId64,
            __LINE__, __FUNCTION__, buffer_type, ino, fi->fh,
            (int64_t)offset, (int64_t)size);
            */

    session = (FCFSAPIOpendirSession *)fi->fh;
    if (session == NULL) {
        fuse_reply_err(req, EBUSY);
        return;
    }

    if (session->btype == FS_READDIR_BUFFER_INIT_NONE) {
        session->btype = buffer_type;
        if ((result=dentry_list_to_buff(req, session)) != 0) {
            fuse_reply_err(req, result);
            return;
        }
    } else if (session->btype != buffer_type) {
        logWarning("file: "__FILE__", line: %d, func: %s, "
                "ino: %"PRId64", unexpect buffer type: %d != %d",
                __LINE__, __FUNCTION__, ino, session->btype,
                buffer_type);
        fuse_reply_buf(req, NULL, 0);
        return;
    }

    if (offset < session->buffer.length) {
        fuse_reply_buf(req, session->buffer.data + offset,
                FC_MIN(session->buffer.length - offset, size));
    } else {
        fuse_reply_buf(req, NULL, 0);
    }
}

/*
static void fs_do_readdir(fuse_req_t req, fuse_ino_t ino, size_t size,
        off_t offset, struct fuse_file_info *fi)
{
    do_readdir(req, ino, size, offset, fi, FS_READDIR_BUFFER_INIT_NORMAL);
}
*/

static void fs_do_readdirplus(fuse_req_t req, fuse_ino_t ino, size_t size,
		off_t offset, struct fuse_file_info *fi)
{
    do_readdir(req, ino, size, offset, fi, FS_READDIR_BUFFER_INIT_PLUS);
}

static void fs_do_releasedir(fuse_req_t req, fuse_ino_t ino,
        struct fuse_file_info *fi)
{
    FCFSAPIOpendirSession *session;

    session = (FCFSAPIOpendirSession *)fi->fh;
    if (session != NULL) {
        fcfs_api_free_opendir_session(session);
        fi->fh = 0;
    }

    fuse_reply_err(req, 0);
}

static int do_open(fuse_req_t req, FDIRDEntryInfo *dentry,
        struct fuse_file_info *fi, const FCFSAPIFileContext *fctx)
{
    int result;
    FCFSAPIFileInfo *fh;

    fh = (FCFSAPIFileInfo *)fast_mblock_alloc_object(&fh_allocator);
    if (fh == NULL) {
        return ENOMEM;
    }

    if (fi->flags & O_DIRECT) {
        if (OS_KERNEL_VERSION.major > 4 || (OS_KERNEL_VERSION.major == 4 &&
            OS_KERNEL_VERSION.minor >= 18))
        {
             fi->direct_io = 1;
        }
    } else {
        if (g_fuse_global_vars.kernel_cache) {
            fi->keep_cache = 1;
        }
    }

    if ((result=fcfs_api_open_by_dentry(fh, dentry, fi->flags, fctx)) != 0) {
        fast_mblock_free_object(&fh_allocator, fh);
        if (!(result == EISDIR || result == ENOENT)) {
            result = EACCES;
        }
        return result;
    }

    fi->fh = (long)fh;
    return 0;
}

static void fs_do_access(fuse_req_t req, fuse_ino_t ino, int mask)
{
    const int flags = 0;
    int result;
    int64_t new_inode;
    FDIRClientOperInodePair oino;
    FDIRDEntryInfo dentry;

    if (fs_convert_inode(req, ino, &new_inode) != 0) {
        fuse_reply_err(req, ENOENT);
        return;
    }

    SET_OPER_INODE_PAIR(req, oino, new_inode);
    result = fcfs_api_access_dentry_by_inode(&oino, mask, flags, &dentry);
    fuse_reply_err(req, result);
}

static void fs_do_create(fuse_req_t req, fuse_ino_t parent,
        const char *name, mode_t mode, struct fuse_file_info *fi)
{
    const dev_t rdev = 0;
    FCFSAPIFileContext fctx;
    int result;
    int64_t parent_inode;
    FDIRClientOperPnamePair opname;
    FDIRDEntryInfo dentry;
    struct fuse_entry_param param;

    if (fs_convert_inode(req, parent, &parent_inode) != 0) {
        fuse_reply_err(req, ENOENT);
        return;
    }

    /*
    logInfo("file: "__FILE__", line: %d, func: %s, "
            "parent ino: %"PRId64", name: %s, mode: %03o",
            __LINE__, __FUNCTION__, parent_inode, name, mode);
            */

    SET_OPER_PNAME_PAIR(req, opname, parent_inode, name);
    if ((result=fdir_client_create_dentry_by_pname_ex(g_fcfs_api_ctx.
                    contexts.fdir, &g_fcfs_api_ctx.ns, &opname, mode,
                    rdev, &dentry)) != 0)
    {
        if (result != EEXIST) {
            fuse_reply_err(req, result);
            return;
        }

        if ((fi->flags & O_EXCL)) {
            fuse_reply_err(req, EEXIST);
            return;
        }

        if ((result=fcfs_api_access_dentry_by_pname(&opname,
                        FCFS_API_GET_ACCESS_MASK(fi->flags),
                        FCFS_API_GET_ACCESS_FLAGS(fi->flags),
                        &dentry)) != 0)
        {
            fuse_reply_err(req, result);
            return;
        }
    }

    FCFS_API_SET_FCTX(fctx, opname.oper, mode, fuse_req_ctx(req)->pid);
    fi->flags &= ~(O_CREAT | O_EXCL);
    if ((result=do_open(req, &dentry, fi, &fctx)) != 0) {
        fuse_reply_err(req, result);
        return;
    }

    fill_entry_param(&dentry, &param);
    fuse_reply_create(req, &param, fi);
}

static void do_mknod(fuse_req_t req, fuse_ino_t parent,
        const char *name, mode_t mode, const dev_t rdev)
{
    int result;
    int64_t parent_inode;
    FDIRClientOperPnamePair opname;
    FDIRDEntryInfo dentry;
    struct fuse_entry_param param;

    if (fs_convert_inode(req, parent, &parent_inode) != 0) {
        fuse_reply_err(req, ENOENT);
        return;
    }

    /*
    logInfo("file: "__FILE__", line: %d, func: %s, "
            "parent ino: %"PRId64", name: %s, mode: %03o, "
            "rdev: %lx, isdir: %d", __LINE__, __FUNCTION__,
            parent_inode, name, mode, (long)rdev, S_ISDIR(mode));
            */

    SET_OPER_PNAME_PAIR(req, opname, parent_inode, name);
    if ((result=fdir_client_create_dentry_by_pname_ex(g_fcfs_api_ctx.
                    contexts.fdir, &g_fcfs_api_ctx.ns, &opname, mode,
                    rdev, &dentry)) != 0)
    {
        fuse_reply_err(req, result);
        return;
    }

    fill_entry_param(&dentry, &param);
    fuse_reply_entry(req, &param);
}

static void fs_do_mknod(fuse_req_t req, fuse_ino_t parent,
        const char *name, mode_t mode, dev_t rdev)
{
    do_mknod(req, parent, name, mode, rdev);
}

static void fs_do_mkdir(fuse_req_t req, fuse_ino_t parent,
        const char *name, mode_t mode)
{
    mode |= S_IFDIR;
    do_mknod(req, parent, name, mode, 0);
}

static int remove_dentry(fuse_req_t req, fuse_ino_t parent,
        const char *name, const int flags)
{
    const struct fuse_ctx *fctx;
    int64_t parent_inode;
    FDIRClientOperPnamePair opname;

    if (fs_convert_inode(req, parent, &parent_inode) != 0) {
        return ENOENT;
    }

    fctx = fuse_req_ctx(req);

    /*
    logInfo("file: "__FILE__", line: %d, func: %s, "
            "parent ino: %"PRId64", name: %s, pid: %d, uid: %d, gid: %d",
            __LINE__, __FUNCTION__, parent_inode, name, fctx->pid, fctx->uid, fctx->gid);
            */

    SET_OPER_PNAME_PAIR(req, opname, parent_inode, name);
    return fcfs_api_remove_dentry_by_pname(&opname, flags, fctx->pid);
}

static void fs_do_rmdir(fuse_req_t req, fuse_ino_t parent, const char *name)
{
    int result;

    result = remove_dentry(req, parent, name, FDIR_UNLINK_FLAGS_MATCH_DIR);
    fuse_reply_err(req, result);
}

static void fs_do_unlink(fuse_req_t req, fuse_ino_t parent, const char *name)
{
    int result;

    result = remove_dentry(req, parent, name, FDIR_UNLINK_FLAGS_MATCH_FILE);
    fuse_reply_err(req, result);
}

void fs_do_rename(fuse_req_t req, fuse_ino_t oldparent, const char *oldname,
            fuse_ino_t newparent, const char *newname, unsigned int flags)
{
    int64_t old_parent_inode;
    int64_t new_parent_inode;
    const struct fuse_ctx *fctx;
    string_t old_nm;
    string_t new_nm;
    FDIRDentryOperator oper;
    char groups_buff[FDIR_MAX_USER_GROUP_BYTES];
    int result;

    if (fs_convert_inode(req, oldparent, &old_parent_inode) != 0) {
        fuse_reply_err(req, ENOENT);
        return;
    }

    if (fs_convert_inode(req, newparent, &new_parent_inode) != 0) {
        fuse_reply_err(req, ENOENT);
        return;
    }

    /*
    logInfo("file: "__FILE__", line: %d, func: %s, "
            "parent ino: %"PRId64", name: %s, "
            "newparent ino: %"PRId64", new name: %s",
            __LINE__, __FUNCTION__, old_parent_inode, oldname,
            new_parent_inode, newname);
            */

    fctx = fuse_req_ctx(req);
    FC_SET_STRING(old_nm, (char *)oldname);
    FC_SET_STRING(new_nm, (char *)newname);

    set_operator_by_req(fctx, &oper, groups_buff);
    result = fcfs_api_rename_dentry_by_pname(old_parent_inode, &old_nm,
            new_parent_inode, &new_nm, &oper, flags, fctx->pid);
    fuse_reply_err(req, result);
}

static void fs_do_link(fuse_req_t req, fuse_ino_t ino,
        fuse_ino_t parent, const char *name)
{
    const int flags = FDIR_FLAGS_FOLLOW_SYMLINK;
    const struct fuse_ctx *fctx;
    FDIRClientOperPnamePair opname;
    FDIRDEntryInfo dentry;
    struct fuse_entry_param param;
    int64_t parent_inode;
    int result;

    if (fs_convert_inode(req, parent, &parent_inode) != 0) {
        fuse_reply_err(req, ENOENT);
        return;
    }

    fctx = fuse_req_ctx(req);
    SET_OPER_PNAME_PAIR(req, opname, parent_inode, name);
    if ((result=fdir_client_link_dentry_by_pname(g_fcfs_api_ctx.contexts.fdir,
                    ino, &g_fcfs_api_ctx.ns, &opname, (0777 & (~fctx->umask)),
                    flags, &dentry)) == 0)
    {
        fill_entry_param(&dentry, &param);
        fuse_reply_entry(req, &param);
    } else {
        fuse_reply_err(req, result);
    }
}

static void fs_do_symlink(fuse_req_t req, const char *link,
        fuse_ino_t parent, const char *name)
{
    const struct fuse_ctx *fctx;
    int64_t parent_inode;
    FDIRClientOperPnamePair opname;
    string_t lk;
    FDIRDEntryInfo dentry;
    struct fuse_entry_param param;
    int result;

    if (fs_convert_inode(req, parent, &parent_inode) != 0) {
        fuse_reply_err(req, ENOENT);
        return;
    }

    /*
    logInfo("file: "__FILE__", line: %d, func: %s, "
            "link length: %d, parent ino: %"PRId64", "
            "name length: %d", __LINE__, __FUNCTION__,
            (int)strlen(link), parent_inode, (int)strlen(name));
            */

    fctx = fuse_req_ctx(req);
    FC_SET_STRING(lk, (char *)link);
    SET_OPER_PNAME_PAIR(req, opname, parent_inode, name);
    if ((result=fdir_client_symlink_dentry_by_pname(g_fcfs_api_ctx.
                    contexts.fdir, &lk, &g_fcfs_api_ctx.ns, &opname,
                    (0777 & (~fctx->umask)), &dentry)) == 0)
    {
        fill_entry_param(&dentry, &param);
        fuse_reply_entry(req, &param);
    } else {
        fuse_reply_err(req, result);
    }
}

static void fs_do_readlink(fuse_req_t req, fuse_ino_t ino)
{
    int result;
    char buff[PATH_MAX];
    FDIRClientOperInodePair oino;
    string_t link;

    SET_OPER_INODE_PAIR(req, oino, ino);
    link.str = buff;
    if ((result=fcfs_api_readlink_by_inode(&oino, &link, PATH_MAX)) == 0) {
        fuse_reply_readlink(req, link.str);
    } else {
        fuse_reply_err(req, result);
    }
}

static void fs_do_forget(fuse_req_t req, fuse_ino_t ino, uint64_t nlookup)
{
    /*
    logInfo("file: "__FILE__", line: %d, func: %s, "
            "ino: %"PRId64", nlookup: %"PRId64,
            __LINE__, __FUNCTION__, ino, nlookup);
            */
    fuse_reply_none(req);
}

static void fs_do_forget_multi(fuse_req_t req, size_t count,
        struct fuse_forget_data *forgets)
{
    /*
    logInfo("file: "__FILE__", line: %d, func: %s, "
            "count: %d", __LINE__, __FUNCTION__, (int)count);
            */
    fuse_reply_none(req);
}

static void fs_do_open(fuse_req_t req, fuse_ino_t ino,
			  struct fuse_file_info *fi)
{
    int result;
    int64_t new_inode;
    FCFSAPIFileContext fctx;
    FDIRClientOperInodePair oino;
    const struct fuse_ctx *fuse_ctx;
    FDIRDEntryInfo dentry;

    /*
    logInfo("file: "__FILE__", line: %d, func: %s, "
            "ino: %"PRId64", fh: %"PRId64", O_APPEND flag: %d, "
            "O_WRONLY: %d, O_RDWR: %d, O_NONBLOCK flag: %d",
            __LINE__, __FUNCTION__, ino, fi->fh,
            (fi->flags & O_APPEND), (fi->flags & O_WRONLY),
            (fi->flags & O_RDWR), (fi->flags & O_NONBLOCK));
            */

    if (fs_convert_inode(req, ino, &new_inode) != 0) {
        fuse_reply_err(req, ENOENT);
        return;
    }

    fuse_ctx = fuse_req_ctx(req);
    SET_OPER_INODE_PAIR(req, oino, new_inode);
    if ((result=fcfs_api_access_dentry_by_inode(&oino,
                    FCFS_API_GET_ACCESS_MASK(fi->flags),
                    FCFS_API_GET_ACCESS_FLAGS(fi->flags),
                    &dentry)) != 0)
    {
        fuse_reply_err(req, result);
        return;
    }

    FCFS_API_SET_FCTX(fctx, oino.oper, dentry.stat.mode, fuse_ctx->pid);
    if ((result=do_open(req, &dentry, fi, &fctx)) != 0) {
        fuse_reply_err(req, result);
        return;
    }

    fuse_reply_open(req, fi);
}

static void fs_do_flush(fuse_req_t req, fuse_ino_t ino,
        struct fuse_file_info *fi)
{
    /*
    logInfo("file: "__FILE__", line: %d, func: %s, "
            "ino: %"PRId64", fh: %"PRId64"\n",
            __LINE__, __FUNCTION__, ino, fi->fh);
            */

    fuse_reply_err(req, 0);
}

static void fs_do_fsync(fuse_req_t req, fuse_ino_t ino,
        int datasync, struct fuse_file_info *fi)
{
    FCFSAPIFileInfo *fh;
    const struct fuse_ctx *fctx;
    int result;

    /*
    logInfo("file: "__FILE__", line: %d, func: %s, "
            "ino: %"PRId64", fh: %"PRId64", datasync: %d",
            __LINE__, __FUNCTION__, ino, fi->fh, datasync);
            */

    fh = (FCFSAPIFileInfo *)fi->fh;
    if (fh != NULL) {
        fctx = fuse_req_ctx(req);
        if (datasync) {
            result = fcfs_api_fdatasync(fh, fctx->pid);
        } else {
            result = fcfs_api_fsync(fh, fctx->pid);
        }
    } else {
        result = EBADF;
    }
    fuse_reply_err(req, result);
}

static void fs_do_release(fuse_req_t req, fuse_ino_t ino,
             struct fuse_file_info *fi)
{
    int result;
    FCFSAPIFileInfo *fh;

    /*
    logInfo("file: "__FILE__", line: %d, func: %s, "
            "ino: %"PRId64", fh: %"PRId64"\n",
            __LINE__, __FUNCTION__, ino, fi->fh);
            */

    fh = (FCFSAPIFileInfo *)fi->fh;
    if (fh != NULL) {
        result = fcfs_api_close(fh);
        fast_mblock_free_object(&fh_allocator, fh);
    } else {
        result = EBADF;
    }
    fuse_reply_err(req, result);
}

static void fs_do_read(fuse_req_t req, fuse_ino_t ino, size_t size,
			  off_t offset, struct fuse_file_info *fi)
{
    FCFSAPIFileInfo *fh;
    const struct fuse_ctx *fctx;
    int result;
    int read_bytes;
    char fixed_buff[256 * 1024];
    char *buff;
    
    if (size <= sizeof(fixed_buff)) {
        buff = fixed_buff;
    } else if ((buff=(char *)fc_malloc(size)) == NULL) {
        fuse_reply_err(req, ENOMEM);
        return;
    }

    fh = (FCFSAPIFileInfo *)fi->fh;
    if (fh == NULL) {
        fuse_reply_err(req, EBADF);
        return;
    }

    fctx = fuse_req_ctx(req);
    if ((result=fcfs_api_pread_ex(fh, buff, size, offset,
                    &read_bytes, fctx->pid)) != 0)
    {
        fuse_reply_err(req, result);
        return;
    }

    /*
    logInfo("file: "__FILE__", line: %d, func: %s, "
            "ino: %"PRId64", fh: %p, size: %"PRId64", offset: %"PRId64", "
            "read_bytes: %d, flags: %d, O_DIRECT: %d, O_SYNC: %d, "
            "O_DSYNC: %d", __LINE__, __FUNCTION__, ino, fh, size,
            offset, read_bytes, fh->flags, fh->flags & O_DIRECT,
            fh->flags & O_SYNC, fh->flags & O_DSYNC);
            */

    fuse_reply_buf(req, buff, read_bytes);
    if (buff != fixed_buff) {
        free(buff);
    }
}

void fs_do_write(fuse_req_t req, fuse_ino_t ino, const char *buff,
        size_t size, off_t offset, struct fuse_file_info *fi)
{
    FCFSAPIFileInfo *fh;
    const struct fuse_ctx *fctx;
    int result;
    int written_bytes;

    fh = (FCFSAPIFileInfo *)fi->fh;
    if (fh == NULL) {
        fuse_reply_err(req, EBADF);
        return;
    }

    fctx = fuse_req_ctx(req);
    if ((result=fcfs_api_pwrite_ex(fh, buff, size, offset,
                    &written_bytes, fctx->pid)) != 0)
    {
        fuse_reply_err(req, result);
        return;
    }

    /*
    logInfo("=======file: "__FILE__", line: %d, func: %s, "
            "ino: %"PRId64", size: %"PRId64", offset: %"PRId64", "
            "written_bytes: %d, flags: %d, O_DIRECT: %d, O_SYNC: %d, "
            "O_DSYNC: %d", __LINE__, __FUNCTION__, ino, size, offset,
            written_bytes, fh->flags, fh->flags & O_DIRECT,
            fh->flags & O_SYNC, fh->flags & O_DSYNC);
            */

    fuse_reply_write(req, written_bytes);
}

void fs_do_lseek(fuse_req_t req, fuse_ino_t ino, off_t offset,
        int whence, struct fuse_file_info *fi)
{
    FCFSAPIFileInfo *fh;
    int result;

    fh = (FCFSAPIFileInfo *)fi->fh;
    if (fh == NULL) {
        fuse_reply_err(req, EBADF);
        return;
    }

    if ((result=fcfs_api_lseek(fh, offset, whence)) != 0) {
        fuse_reply_err(req, result);
        return;
    }

    fuse_reply_lseek(req, fh->offset);
}

static void fs_do_getlk(fuse_req_t req, fuse_ino_t ino,
        struct fuse_file_info *fi, struct flock *lock)
{
    int result;
    FCFSAPIFileInfo *fh;
    int64_t owner_id;

    fh = (FCFSAPIFileInfo *)fi->fh;
    if (fh == NULL) {
        result = EBADF;
        owner_id = 0;
    } else {
        owner_id = fi->lock_owner;
        result = fcfs_api_getlk_ex(fh, lock, &owner_id);
    }

    /*
    logInfo("file: "__FILE__", line: %d, func: %s, "
            "ino: %"PRId64", fh: %"PRId64", type: %d, "
            "whence: %d, start: %"PRId64", len: %"PRId64", pid: %d, "
            "owner_id: %"PRId64", result: %d", __LINE__, __FUNCTION__,
            ino, fi->fh, lock->l_type, lock->l_whence, lock->l_start,
            lock->l_len, lock->l_pid, owner_id, result);
            */

    if (result == 0) {
        fuse_reply_lock(req, lock);
    } else {
        fuse_reply_err(req, result);
    }
}

static void fs_do_setlk(fuse_req_t req, fuse_ino_t ino,
        struct fuse_file_info *fi, struct flock *lock, int sleep)
{
    const bool blocked = false;
    int result;
    FCFSAPIFileInfo *fh;

    fh = (FCFSAPIFileInfo *)fi->fh;
    if (fh == NULL) {
        result = EBADF;
    } else {
        result = fcfs_api_setlk_ex(fh, lock, fi->lock_owner, blocked);
    }

    /*
    logInfo("file: "__FILE__", line: %d, func: %s, "
            "ino: %"PRId64", fh: %"PRId64", lock_owner: %"PRId64", "
            "type: %d, whence: %d, start: %"PRId64", len: %"PRId64", "
            "pid: %d, sleep: %d, result: %d", __LINE__, __FUNCTION__,
            ino, fi->fh, fi->lock_owner, lock->l_type, lock->l_whence,
            lock->l_start, lock->l_len, lock->l_pid, sleep, result);
            */

    fuse_reply_err(req, result);
}

static void fs_do_flock(fuse_req_t req, fuse_ino_t ino,
        struct fuse_file_info *fi, int op)
{
    int result;
    FCFSAPIFileInfo *fh;

    /*
    logInfo("file: "__FILE__", line: %d, func: %s, "
            "ino: %"PRId64", fh: %"PRId64", lock_owner: %"PRId64", "
            "op: %d, operation: %d", __LINE__, __FUNCTION__,
            ino, fi->fh, fi->lock_owner, op,
            (op & (LOCK_SH | LOCK_EX | LOCK_UN)));
            */

    fh = (FCFSAPIFileInfo *)fi->fh;
    if (fh == NULL) {
        result = EBADF;
    } else {
        result = fcfs_api_flock_ex(fh, op, fi->lock_owner);
    }
    fuse_reply_err(req, result);
}

static void fs_do_statfs(fuse_req_t req, fuse_ino_t ino)
{
    int result;
    struct statvfs stbuf;

    if ((result=fcfs_api_statvfs("/", &stbuf)) == 0) {
        fuse_reply_statfs(req, &stbuf);
    } else {
        fuse_reply_err(req, result);
    }

    /*
    logInfo("file: "__FILE__", line: %d, func: %s, "
            "ino: %"PRId64, __LINE__, __FUNCTION__, ino);
            */
}

static void fs_do_fallocate(fuse_req_t req, fuse_ino_t ino, int mode,
        off_t offset, off_t length, struct fuse_file_info *fi)
{
    int result;
    FCFSAPIFileInfo *fh;
    const struct fuse_ctx *fctx;

    fh = (FCFSAPIFileInfo *)fi->fh;
    if (fh == NULL) {
        result = EBADF;
    } else {
        fctx = fuse_req_ctx(req);
        result = fcfs_api_fallocate_ex(fh, mode, offset, length, fctx->pid);
    }

    fuse_reply_err(req, result);
}

static void fs_do_setxattr(fuse_req_t req, fuse_ino_t ino, const char *name,
        const char *value, size_t size, int flags)
{
    int64_t new_inode;
    int result;
    FDIRClientOperInodePair oino;
    key_value_pair_t xattr;

    if (!g_fuse_global_vars.xattr_enabled) {
        fuse_reply_err(req, ENOSYS);
        return;
    }

    if (fs_convert_inode(req, ino, &new_inode) != 0) {
        fuse_reply_err(req, ENOENT);
        return;
    }

    SET_OPER_INODE_PAIR(req, oino, new_inode);
    FC_SET_STRING(xattr.key, (char *)name);
    FC_SET_STRING_EX(xattr.value, (char *)value, size);
    result = fcfs_api_set_xattr_by_inode(&oino, &xattr, flags);
    fuse_reply_err(req, result);
}

static void fs_do_removexattr(fuse_req_t req, fuse_ino_t ino,
        const char *name)
{
    const int flags = 0;
    int result;
    int64_t new_inode;
    FDIRClientOperInodePair oino;
    string_t nm;

    if (!g_fuse_global_vars.xattr_enabled) {
        fuse_reply_err(req, ENOSYS);
        return;
    }

    if (fs_convert_inode(req, ino, &new_inode) != 0) {
        fuse_reply_err(req, ENOENT);
        return;
    }

    SET_OPER_INODE_PAIR(req, oino, new_inode);
    FC_SET_STRING(nm, (char *)name);
    result = fcfs_api_remove_xattr_by_inode(&oino, &nm, flags);
    fuse_reply_err(req, result);
}

static void fs_do_getxattr(fuse_req_t req, fuse_ino_t ino,
        const char *name, size_t size)
{
    int result;
    int flags;
    int value_size;
    int64_t new_inode;
    char v[FDIR_XATTR_MAX_VALUE_SIZE];
    FDIRClientOperInodePair oino;
    string_t nm;
    string_t value;

    if (!g_fuse_global_vars.xattr_enabled) {
        fuse_reply_err(req, ENOSYS);
        return;
    }

    if (fs_convert_inode(req, ino, &new_inode) != 0) {
        fuse_reply_err(req, ENOENT);
        return;
    }

    /*
    logInfo("getxattr ino: %"PRId64", name: %s, size: %d, req: %p",
            ino, name, (int)size, req);
            */

    if (size == 0) {
        value_size = 0;
        flags = FDIR_FLAGS_XATTR_GET_SIZE;
    } else if (size <= FDIR_XATTR_MAX_VALUE_SIZE) {
        value_size = size;
        flags = 0;
    } else {
        value_size = FDIR_XATTR_MAX_VALUE_SIZE;
        flags = 0;
    }

    SET_OPER_INODE_PAIR(req, oino, new_inode);
    value.str = v;
    FC_SET_STRING(nm, (char *)name);
    if ((result=fcfs_api_get_xattr_by_inode(&oino,
                    &nm, &value, value_size, flags)) != 0)
    {
        fuse_reply_err(req, result == EOVERFLOW ? ERANGE : result);
        return;
    }

    if (size == 0) {
        fuse_reply_xattr(req, value.len);
    } else {
        fuse_reply_buf(req, value.str, value.len);
    }
}

static void fs_do_listxattr(fuse_req_t req, fuse_ino_t ino, size_t size)
{
#define MAX_LIST_SIZE  (8 * 1024)

    int result;
    int flags;
    int list_size;
    int64_t new_inode;
    FDIRClientOperInodePair oino;
    char v[MAX_LIST_SIZE];
    string_t list;

    if (!g_fuse_global_vars.xattr_enabled) {
        fuse_reply_err(req, ENOSYS);
        return;
    }

    if (fs_convert_inode(req, ino, &new_inode) != 0) {
        fuse_reply_err(req, ENOENT);
        return;
    }

    if (size == 0) {
        list_size = 0;
        flags = FDIR_FLAGS_XATTR_GET_SIZE;
    } else if (size <= MAX_LIST_SIZE) {
        list_size = size;
        flags = 0;
    } else {
        list_size = MAX_LIST_SIZE;
        flags = 0;
    }

    SET_OPER_INODE_PAIR(req, oino, new_inode);
    list.str = v;
    if ((result=fcfs_api_list_xattr_by_inode(&oino,
                    &list, list_size, flags)) != 0)
    {
        fuse_reply_err(req, result == EOVERFLOW ? ERANGE : result);
        return;
    }

    /*
    logInfo("listxattr ino: %"PRId64", size: %d, list: %.*s, list len: %d",
            ino, (int)size, list.len, list.str, list.len);
            */

    if (size == 0) {
        fuse_reply_xattr(req, list.len);
    } else {
        fuse_reply_buf(req, list.str, list.len);
    }
}

static void fs_do_init(void *userdata, struct fuse_conn_info *conn)
{
    fuse_apply_conn_info_opts(g_fuse_cinfo_opts, conn);
    conn->want |= FUSE_CAP_EXPORT_SUPPORT;
}

int fs_fuse_wrapper_init(struct fuse_lowlevel_ops *ops)
{
    int result;
    if ((result=fast_mblock_init_ex1(&fh_allocator, "fuse_fh",
                    sizeof(FCFSAPIFileInfo), 4096, 0, NULL, NULL, true)) != 0)
    {
        return result;
    }

    if (GROUPS_CACHE_ENABLED && g_fcfs_api_ctx.owner.type !=
            fcfs_api_owner_type_fixed)
    {
        if ((result=fcfs_groups_htable_init()) != 0) {
            return result;
        }
    }

    memset(ops, 0, sizeof(*ops));
    ops->init = fs_do_init;
    ops->lookup  = fs_do_lookup;
    ops->getattr = fs_do_getattr;
    ops->setattr = fs_do_setattr;
    ops->opendir = fs_do_opendir;
    //ops->readdir = fs_do_readdir;
    ops->readdirplus = fs_do_readdirplus;
    ops->releasedir  = fs_do_releasedir;
    ops->create  = fs_do_create;
    ops->access  = fs_do_access;
    ops->open    = fs_do_open;
    ops->fsync   = fs_do_fsync;
    ops->flush   = fs_do_flush;
    ops->release = fs_do_release;
    ops->read    = fs_do_read;
    ops->write   = fs_do_write;
    ops->mknod   = fs_do_mknod;
    ops->mkdir   = fs_do_mkdir;
    ops->rmdir   = fs_do_rmdir;
    ops->unlink  = fs_do_unlink;
    ops->link    = fs_do_link;
    ops->symlink = fs_do_symlink;
    ops->readlink = fs_do_readlink;
    ops->rename  = fs_do_rename;
    ops->forget  = fs_do_forget;
    ops->forget_multi = fs_do_forget_multi;
    ops->lseek   = fs_do_lseek;
    ops->getlk   = fs_do_getlk;
    ops->setlk   = fs_do_setlk;
    ops->flock   = fs_do_flock;
    ops->statfs  = fs_do_statfs;
    ops->fallocate = fs_do_fallocate;
    ops->setxattr = fs_do_setxattr;
    ops->getxattr = fs_do_getxattr;
    ops->listxattr = fs_do_listxattr;
    ops->removexattr = fs_do_removexattr;

    return 0;
}
