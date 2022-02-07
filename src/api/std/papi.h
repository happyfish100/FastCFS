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


#ifndef _FCFS_PAPI_H
#define _FCFS_PAPI_H

#include <utime.h>
#include <dirent.h>
#include "api_types.h"

#define fcfs_open(path, flags, ...) \
    fcfs_open_ex(&g_fcfs_papi_ctx, path, flags, ##__VA_ARGS__)

#define fcfs_openat(fd, path, flags, ...) \
    fcfs_openat_ex(&g_fcfs_papi_ctx, fd, path, flags, ##__VA_ARGS__)

#define fcfs_creat(path, mode) \
    fcfs_creat_ex(&g_fcfs_papi_ctx, path, mode)

#define fcfs_close(fd) \
    fcfs_close_ex(&g_fcfs_papi_ctx, fd)

#define fcfs_fsync(fd) \
    fcfs_fsync_ex(&g_fcfs_papi_ctx, fd)

#define fcfs_fdatasync(fd) \
    fcfs_fdatasync_ex(&g_fcfs_papi_ctx, fd)

#define fcfs_write(fd, buff, count) \
    fcfs_write_ex(&g_fcfs_papi_ctx, fd, buff, count)

#define fcfs_pwrite(fd, buff, count, offset) \
    fcfs_pwrite_ex(&g_fcfs_papi_ctx, fd, buff, count, offset)

#define fcfs_writev(fd, iov, iovcnt) \
    fcfs_writev_ex(&g_fcfs_papi_ctx, fd, iov, iovcnt)

#define fcfs_pwritev(fd, iov, iovcnt, offset) \
    fcfs_pwritev_ex(&g_fcfs_papi_ctx, fd, iov, iovcnt, offset)

#define fcfs_read(fd, buff, count) \
    fcfs_read_ex(&g_fcfs_papi_ctx, fd, buff, count)

#define fcfs_pread(fd, buff, count, offset) \
    fcfs_pread_ex(&g_fcfs_papi_ctx, fd, buff, count, offset)

#define fcfs_readv(fd, iov, iovcnt) \
    fcfs_readv_ex(&g_fcfs_papi_ctx, fd, iov, iovcnt)

#define fcfs_preadv(fd, iov, iovcnt, offset) \
    fcfs_preadv_ex(&g_fcfs_papi_ctx, fd, iov, iovcnt, offset)

#define fcfs_lseek(fd, offset, whence) \
    fcfs_lseek_ex(&g_fcfs_papi_ctx, fd, offset, whence)

#define fcfs_fallocate(fd, mode, offset, length) \
    fcfs_fallocate_ex(&g_fcfs_papi_ctx, fd, mode, offset, length)

#define fcfs_truncate(path, length) \
    fcfs_truncate_ex(&g_fcfs_papi_ctx, path, length)

#define fcfs_ftruncate(fd, length) \
    fcfs_ftruncate_ex(&g_fcfs_papi_ctx, fd, length)

#define fcfs_lstat(path, buf) \
    fcfs_lstat_ex(&g_fcfs_papi_ctx, path, buf)

#define fcfs_stat(path, buf) \
    fcfs_stat_ex(&g_fcfs_papi_ctx, path, buf)

#define fcfs_fstat(fd, buf) \
    fcfs_fstat_ex(&g_fcfs_papi_ctx, fd, buf)

#define fcfs_fstatat(fd, path, buf, flags) \
    fcfs_fstatat_ex(&g_fcfs_papi_ctx, fd, path, buf, flags)

#define fcfs_flock(fd, operation) \
    fcfs_flock_ex(&g_fcfs_papi_ctx, fd, operation)

#define fcfs_symlink(link, path) \
    fcfs_symlink_ex(&g_fcfs_papi_ctx, link, path)

#define fcfs_symlinkat(link, fd, path) \
    fcfs_symlinkat_ex(&g_fcfs_papi_ctx, link, fd, path)

#define fcfs_link(path1, path2) \
    fcfs_link_ex(&g_fcfs_papi_ctx, path1, path2)

#define fcfs_linkat(fd1, path1, fd2, path2, flags) \
    fcfs_linkat_ex(&g_fcfs_papi_ctx, fd1, path1, fd2, path2, flags)

#define fcfs_readlink(path, buff, size) \
    fcfs_readlink_ex(&g_fcfs_papi_ctx, path, buff, size)

#define fcfs_readlinkat(fd, path, buff, size) \
    fcfs_readlinkat_ex(&g_fcfs_papi_ctx, fd, path, buff, size)

#define fcfs_mknod(path, mode, dev) \
    fcfs_mknod_ex(&g_fcfs_papi_ctx, path, mode, dev)

#define fcfs_mknodat(fd, path, mode, dev) \
    fcfs_mknodat_ex(&g_fcfs_papi_ctx, fd, path, mode, dev)

#define fcfs_access(path, mode) \
    fcfs_access_ex(&g_fcfs_papi_ctx, path, mode)

#define fcfs_faccessat(fd, path, mode, flags) \
    fcfs_faccessat_ex(&g_fcfs_papi_ctx, fd, path, mode, flags)

#define fcfs_utime(path, times) \
    fcfs_utime_ex(&g_fcfs_papi_ctx, path, times)

#define fcfs_utimes(path, times) \
    fcfs_utimes_ex(&g_fcfs_papi_ctx, path, times)

#define fcfs_futimes(fd, times) \
    fcfs_futimes_ex(&g_fcfs_papi_ctx, fd, times)

#define fcfs_futimesat(fd, path, times) \
    fcfs_futimesat_ex(&g_fcfs_papi_ctx, fd, path, times)

#define fcfs_futimens(fd, times) \
    fcfs_futimens_ex(&g_fcfs_papi_ctx, fd, times)

#define fcfs_utimensat(fd, path, times, flags) \
    fcfs_utimensat_ex(&g_fcfs_papi_ctx, fd, path, times, flags)

#define fcfs_unlink(path) \
    fcfs_unlink_ex(&g_fcfs_papi_ctx, path)

#define fcfs_unlinkat(fd, path, flags) \
    fcfs_unlinkat_ex(&g_fcfs_papi_ctx, fd, path, flags)

#define fcfs_rename(path1, path2) \
    fcfs_rename_ex(&g_fcfs_papi_ctx, path1, path2)

#define fcfs_renameat(fd1, path1, fd2, path2) \
    fcfs_renameat_ex(&g_fcfs_papi_ctx, fd1, path1, fd2, path2)

#define fcfs_renameat2(fd1, path1, fd2, path2, flags) \
    fcfs_renameat2_ex(&g_fcfs_papi_ctx, fd1, path1, fd2, path2, flags)

#define fcfs_mkdir(path, mode) \
    fcfs_mkdir_ex(&g_fcfs_papi_ctx, path, mode)

#define fcfs_mkdirat(fd, path, mode) \
    fcfs_mkdirat_ex(&g_fcfs_papi_ctx, fd, path, mode)

#define fcfs_rmdir(path) \
    fcfs_rmdir_ex(&g_fcfs_papi_ctx, path)

#define fcfs_chown(path, owner, group) \
    fcfs_chown_ex(&g_fcfs_papi_ctx, path, owner, group)

#define fcfs_lchown(path, owner, group) \
    fcfs_lchown_ex(&g_fcfs_papi_ctx, path, owner, group)

#define fcfs_fchown(fd, owner, group) \
    fcfs_fchown_ex(&g_fcfs_papi_ctx, fd, owner, group)

#define fcfs_fchownat(fd, path, owner, group, flags) \
    fcfs_fchownat_ex(&g_fcfs_papi_ctx, fd, path, owner, group, flags)

#define fcfs_chmod(path, mode) \
    fcfs_chmod_ex(&g_fcfs_papi_ctx, path, mode)

#define fcfs_fchmod(fd, mode) \
    fcfs_fchmod_ex(&g_fcfs_papi_ctx, fd, mode)

#define fcfs_fchmodat(fd, path, mode, flags) \
    fcfs_fchmodat_ex(&g_fcfs_papi_ctx, fd, path, mode, flags)

#define fcfs_statvfs(path, buf) \
    fcfs_statvfs_ex(&g_fcfs_papi_ctx, path, buf)

#define fcfs_fstatvfs(fd, buf) \
    fcfs_fstatvfs_ex(&g_fcfs_papi_ctx, fd, buf)

#define fcfs_setxattr(path, name, value, size, flags) \
    fcfs_setxattr_ex(&g_fcfs_papi_ctx, path, name, value, size, flags)

#define fcfs_lsetxattr(path, name, value, size, flags) \
    fcfs_lsetxattr_ex(&g_fcfs_papi_ctx, path, name, value, size, flags)

#define fcfs_fsetxattr(fd, name, value, size, flags) \
    fcfs_fsetxattr_ex(&g_fcfs_papi_ctx, fd, name, value, size, flags)

#define fcfs_getxattr(path, name, value, size) \
    fcfs_getxattr_ex(&g_fcfs_papi_ctx, path, name, value, size)

#define fcfs_lgetxattr(path, name, value, size) \
    fcfs_lgetxattr_ex(&g_fcfs_papi_ctx, path, name, value, size)

#define fcfs_fgetxattr(fd, name, value, size) \
    fcfs_fgetxattr_ex(&g_fcfs_papi_ctx, fd, name, value, size)

#define fcfs_listxattr(path, list, size) \
    fcfs_listxattr_ex(&g_fcfs_papi_ctx, path, list, size)

#define fcfs_llistxattr(path, list, size) \
    fcfs_llistxattr_ex(&g_fcfs_papi_ctx, path, list, size)

#define fcfs_flistxattr(fd, list, size) \
    fcfs_flistxattr_ex(&g_fcfs_papi_ctx, fd, list, size)

#define fcfs_removexattr(path, name) \
    fcfs_removexattr_ex(&g_fcfs_papi_ctx, path, name)

#define fcfs_lremovexattr(path, name) \
    fcfs_lremovexattr_ex(&g_fcfs_papi_ctx, path, name)

#define fcfs_fremovexattr(fd, name) \
    fcfs_fremovexattr_ex(&g_fcfs_papi_ctx, fd, name)

#define fcfs_opendir(path) \
    fcfs_opendir_ex(&g_fcfs_papi_ctx, path)

#define fcfs_fdopendir(fd) \
    fcfs_fdopendir_ex(&g_fcfs_papi_ctx, fd)

#define fcfs_closedir(dirp) \
    fcfs_closedir_ex(&g_fcfs_papi_ctx, dirp)

#define fcfs_readdir(dirp) \
    fcfs_readdir_ex(&g_fcfs_papi_ctx, dirp)

#define fcfs_readdir_r(dirp, entry, result) \
    fcfs_readdir_r_ex(&g_fcfs_papi_ctx, dirp, entry, result)

#define fcfs_seekdir(dirp, loc) \
    fcfs_seekdir_ex(&g_fcfs_papi_ctx, dirp, loc)

#define fcfs_telldir(dirp) \
    fcfs_telldir_ex(&g_fcfs_papi_ctx, dirp)

#define fcfs_rewinddir(dirp) \
    fcfs_rewinddir_ex(&g_fcfs_papi_ctx, dirp)

#define fcfs_dirfd(dirp) \
    fcfs_dirfd_ex(&g_fcfs_papi_ctx, dirp)

#define fcfs_scandir(path, namelist, filter, compar) \
    fcfs_scandir_ex(&g_fcfs_papi_ctx, path, namelist, filter, compar)

#define fcfs_scandirat(fd, path, namelist, filter, compar) \
    fcfs_scandirat_ex(&g_fcfs_papi_ctx, fd, path, namelist, filter, compar)

#ifdef __cplusplus
extern "C" {
#endif

    int fcfs_open_ex(FCFSPosixAPIContext *ctx,
            const char *path, int flags, ...);

    int fcfs_openat_ex(FCFSPosixAPIContext *ctx, int fd,
            const char *path, int flags, ...);

    int fcfs_creat_ex(FCFSPosixAPIContext *ctx,
            const char *path, mode_t mode);

    int fcfs_close_ex(FCFSPosixAPIContext *ctx, int fd);

    int fcfs_fsync_ex(FCFSPosixAPIContext *ctx, int fd);

    int fcfs_fdatasync_ex(FCFSPosixAPIContext *ctx, int fd);

    ssize_t fcfs_write_ex(FCFSPosixAPIContext *ctx,
            int fd, const void *buff, size_t count);

    ssize_t fcfs_pwrite_ex(FCFSPosixAPIContext *ctx, int fd,
            const void *buff, size_t count, off_t offset);

    ssize_t fcfs_writev_ex(FCFSPosixAPIContext *ctx, int fd,
            const struct iovec *iov, int iovcnt);

    ssize_t fcfs_pwritev_ex(FCFSPosixAPIContext *ctx, int fd,
            const struct iovec *iov, int iovcnt, off_t offset);

    ssize_t fcfs_read_ex(FCFSPosixAPIContext *ctx,
            int fd, void *buff, size_t count);

    ssize_t fcfs_pread_ex(FCFSPosixAPIContext *ctx, int fd,
            void *buff, size_t count, off_t offset);

    ssize_t fcfs_readv_ex(FCFSPosixAPIContext *ctx, int fd,
            const struct iovec *iov, int iovcnt);

    ssize_t fcfs_preadv_ex(FCFSPosixAPIContext *ctx, int fd,
            const struct iovec *iov, int iovcnt, off_t offset);

    off_t fcfs_lseek_ex(FCFSPosixAPIContext *ctx,
            int fd, off_t offset, int whence);

    int fcfs_fallocate_ex(FCFSPosixAPIContext *ctx, int fd,
            int mode, off_t offset, off_t length);

    int fcfs_truncate_ex(FCFSPosixAPIContext *ctx,
            const char *path, off_t length);

    int fcfs_ftruncate_ex(FCFSPosixAPIContext *ctx, int fd, off_t length);

    int fcfs_lstat_ex(FCFSPosixAPIContext *ctx,
            const char *path, struct stat *buf);

    int fcfs_stat_ex(FCFSPosixAPIContext *ctx,
            const char *path, struct stat *buf);

    int fcfs_fstat_ex(FCFSPosixAPIContext *ctx, int fd, struct stat *buf);

    int fcfs_fstatat_ex(FCFSPosixAPIContext *ctx, int fd,
            const char *path, struct stat *buf, int flags);

    int fcfs_flock_ex(FCFSPosixAPIContext *ctx, int fd, int operation);

    //TODO
    int fcfs_fcntl_ex(FCFSPosixAPIContext *ctx, int fd, int cmd, ...);

    int fcfs_symlink_ex(FCFSPosixAPIContext *ctx,
            const char *link, const char *path);

    int fcfs_symlinkat_ex(FCFSPosixAPIContext *ctx,
            const char *link, int fd, const char *path);

    int fcfs_link_ex(FCFSPosixAPIContext *ctx,
            const char *path1, const char *path2);

    int fcfs_linkat_ex(FCFSPosixAPIContext *ctx, int fd1,
            const char *path1, int fd2, const char *path2,
            int flags);

    ssize_t fcfs_readlink_ex(FCFSPosixAPIContext *ctx,
            const char *path, char *buff, size_t size);

    ssize_t fcfs_readlinkat_ex(FCFSPosixAPIContext *ctx, int fd,
            const char *path, char *buff, size_t size);

    int fcfs_mknod_ex(FCFSPosixAPIContext *ctx,
            const char *path, mode_t mode, dev_t dev);

    int fcfs_mknodat_ex(FCFSPosixAPIContext *ctx, int fd,
            const char *path, mode_t mode, dev_t dev);

    int fcfs_access_ex(FCFSPosixAPIContext *ctx,
            const char *path, int mode);

    int fcfs_faccessat_ex(FCFSPosixAPIContext *ctx, int fd,
            const char *path, int mode, int flags);

    int fcfs_utime_ex(FCFSPosixAPIContext *ctx, const char *path,
            const struct utimbuf *times);

    int fcfs_utimes_ex(FCFSPosixAPIContext *ctx, const char *path,
            const struct timeval times[2]);

    int fcfs_futimes_ex(FCFSPosixAPIContext *ctx,
            int fd, const struct timeval times[2]);

    int fcfs_futimesat_ex(FCFSPosixAPIContext *ctx, int fd,
            const char *path, const struct timeval times[2]);

    int fcfs_futimens_ex(FCFSPosixAPIContext *ctx, int fd,
            const struct timespec times[2]);

    int fcfs_utimensat_ex(FCFSPosixAPIContext *ctx, int fd,
            const char *path, const struct timespec times[2], int flags);

    int fcfs_unlink_ex(FCFSPosixAPIContext *ctx, const char *path);

    int fcfs_unlinkat_ex(FCFSPosixAPIContext *ctx, int fd,
            const char *path, int flags);

    int fcfs_rename_ex(FCFSPosixAPIContext *ctx,
            const char *path1, const char *path2);

    int fcfs_renameat_ex(FCFSPosixAPIContext *ctx, int fd1,
            const char *path1, int fd2, const char *path2);

    //renameatx_np for FreeBSD
    int fcfs_renameat2_ex(FCFSPosixAPIContext *ctx, int fd1,
            const char *path1, int fd2, const char *path2,
            unsigned int flags);

    int fcfs_mkdir_ex(FCFSPosixAPIContext *ctx,
            const char *path, mode_t mode);

    int fcfs_mkdirat_ex(FCFSPosixAPIContext *ctx, int fd,
            const char *path, mode_t mode);

    int fcfs_rmdir_ex(FCFSPosixAPIContext *ctx, const char *path);

    int fcfs_chown_ex(FCFSPosixAPIContext *ctx, const char *path,
            uid_t owner, gid_t group);

    int fcfs_lchown_ex(FCFSPosixAPIContext *ctx, const char *path,
            uid_t owner, gid_t group);

    int fcfs_fchown_ex(FCFSPosixAPIContext *ctx, int fd,
            uid_t owner, gid_t group);

    int fcfs_fchownat_ex(FCFSPosixAPIContext *ctx, int fd,
            const char *path, uid_t owner, gid_t group, int flags);

    int fcfs_chmod_ex(FCFSPosixAPIContext *ctx,
            const char *path, mode_t mode);

    int fcfs_fchmod_ex(FCFSPosixAPIContext *ctx,
            int fd, mode_t mode);

    int fcfs_fchmodat_ex(FCFSPosixAPIContext *ctx, int fd,
            const char *path, mode_t mode, int flags);

    int fcfs_statvfs_ex(FCFSPosixAPIContext *ctx,
            const char *path, struct statvfs *buf);

    int fcfs_fstatvfs_ex(FCFSPosixAPIContext *ctx,
            int fd, struct statvfs *buf);

    int fcfs_setxattr_ex(FCFSPosixAPIContext *ctx, const char *path,
            const char *name, const void *value, size_t size, int flags);

    int fcfs_lsetxattr_ex(FCFSPosixAPIContext *ctx, const char *path,
            const char *name, const void *value, size_t size, int flags);

    int fcfs_fsetxattr_ex(FCFSPosixAPIContext *ctx, int fd, const char *name,
            const void *value, size_t size, int flags);

    ssize_t fcfs_getxattr_ex(FCFSPosixAPIContext *ctx, const char *path,
            const char *name, void *value, size_t size);

    ssize_t fcfs_lgetxattr_ex(FCFSPosixAPIContext *ctx, const char *path,
            const char *name, void *value, size_t size);

    ssize_t fcfs_fgetxattr_ex(FCFSPosixAPIContext *ctx, int fd,
            const char *name, void *value, size_t size);

    ssize_t fcfs_listxattr_ex(FCFSPosixAPIContext *ctx,
            const char *path, char *list, size_t size);

    ssize_t fcfs_llistxattr_ex(FCFSPosixAPIContext *ctx,
            const char *path, char *list, size_t size);

    ssize_t fcfs_flistxattr_ex(FCFSPosixAPIContext *ctx,
            int fd, char *list, size_t size);

    int fcfs_removexattr_ex(FCFSPosixAPIContext *ctx,
            const char *path, const char *name);

    int fcfs_lremovexattr_ex(FCFSPosixAPIContext *ctx,
            const char *path, const char *name);

    int fcfs_fremovexattr_ex(FCFSPosixAPIContext *ctx,
            int fd, const char *name);


    DIR *fcfs_opendir_ex(FCFSPosixAPIContext *ctx, const char *path);

    DIR *fcfs_fdopendir_ex(FCFSPosixAPIContext *ctx, int fd);

    int fcfs_closedir_ex(FCFSPosixAPIContext *ctx, DIR *dirp);

    struct dirent *fcfs_readdir_ex(FCFSPosixAPIContext *ctx, DIR *dirp);

    int fcfs_readdir_r_ex(FCFSPosixAPIContext *ctx, DIR *dirp,
            struct dirent *entry, struct dirent **result);

    void fcfs_seekdir_ex(FCFSPosixAPIContext *ctx, DIR *dirp, long loc);

    long fcfs_telldir_ex(FCFSPosixAPIContext *ctx, DIR *dirp);

    void fcfs_rewinddir_ex(FCFSPosixAPIContext *ctx, DIR *dirp);

    int fcfs_dirfd_ex(FCFSPosixAPIContext *ctx, DIR *dirp);

    int fcfs_scandir_ex(FCFSPosixAPIContext *ctx, const char *path,
            struct dirent ***namelist, int (*filter)(const struct dirent *),
            int (*compar)(const struct dirent **, const struct dirent **));

    int fcfs_scandirat_ex(FCFSPosixAPIContext *ctx, int fd, const char *path,
            struct dirent ***namelist, int (*filter)(const struct dirent *),
            int (*compar)(const struct dirent **, const struct dirent **));

    //TODO
    int fcfs_dup_ex(FCFSPosixAPIContext *ctx, int fd);

    int fcfs_dup2_ex(FCFSPosixAPIContext *ctx, int fd1, int fd2);

    int fcfs_chdir_ex(FCFSPosixAPIContext *ctx, const char *path);

    int fcfs_fchdir_ex(FCFSPosixAPIContext *ctx, int fd);

    char *fcfs_getcwd_ex(FCFSPosixAPIContext *ctx, char *buf, size_t size);

    char *fcfs_getwd_ex(FCFSPosixAPIContext *ctx, char *buf);

    int fcfs_chroot_ex(FCFSPosixAPIContext *ctx, const char *path);

    void *fcfs_mmap_ex(FCFSPosixAPIContext *ctx, void *addr, size_t length,
            int prot, int flags, int fd, off_t offset);

    int fcfs_munmap_ex(FCFSPosixAPIContext *ctx, void *addr, size_t length);

#ifdef __cplusplus
}
#endif

#endif
