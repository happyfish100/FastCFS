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
#include "fd_manager.h"

#define G_FCFS_PAPI_CTX  g_fcfs_papi_global_vars.ctx
#define G_FCFS_PAPI_CWD  g_fcfs_papi_global_vars.cwd

#define fcfs_open(path, flags, ...) \
    fcfs_open_ex(&G_FCFS_PAPI_CTX, path, flags, ##__VA_ARGS__)

#define fcfs_openat(fd, path, flags, ...) \
    fcfs_openat_ex(&G_FCFS_PAPI_CTX, fd, path, flags, ##__VA_ARGS__)

#define fcfs_creat(path, mode) \
    fcfs_creat_ex(&G_FCFS_PAPI_CTX, path, mode)

#define fcfs_truncate(path, length) \
    fcfs_truncate_ex(&G_FCFS_PAPI_CTX, path, length)

#define fcfs_lstat(path, buf) \
    fcfs_lstat_ex(&G_FCFS_PAPI_CTX, path, buf)

#define fcfs_stat(path, buf) \
    fcfs_stat_ex(&G_FCFS_PAPI_CTX, path, buf)

#define fcfs_fstatat(fd, path, buf, flags) \
    fcfs_fstatat_ex(&G_FCFS_PAPI_CTX, fd, path, buf, flags)

#define fcfs_symlink(link, path) \
    fcfs_symlink_ex(&G_FCFS_PAPI_CTX, link, path)

#define fcfs_symlinkat(link, fd, path) \
    fcfs_symlinkat_ex(&G_FCFS_PAPI_CTX, link, fd, path)

#define fcfs_link(path1, path2) \
    fcfs_link_ex(&G_FCFS_PAPI_CTX, path1, path2)

#define fcfs_linkat(fd1, path1, fd2, path2, flags) \
    fcfs_linkat_ex(&G_FCFS_PAPI_CTX, fd1, path1, fd2, path2, flags)

#define fcfs_readlink(path, buff, size) \
    fcfs_readlink_ex(&G_FCFS_PAPI_CTX, path, buff, size)

#define fcfs_readlinkat(fd, path, buff, size) \
    fcfs_readlinkat_ex(&G_FCFS_PAPI_CTX, fd, path, buff, size)

#define fcfs_mknod(path, mode, dev) \
    fcfs_mknod_ex(&G_FCFS_PAPI_CTX, path, mode, dev)

#define fcfs_mknodat(fd, path, mode, dev) \
    fcfs_mknodat_ex(&G_FCFS_PAPI_CTX, fd, path, mode, dev)

#define fcfs_mkfifo(path, mode) \
    fcfs_mkfifo_ex(&G_FCFS_PAPI_CTX, path, mode)

#define fcfs_mkfifoat(fd, path, mode) \
    fcfs_mkfifoat_ex(&G_FCFS_PAPI_CTX, fd, path, mode)

#define fcfs_access(path, mode) \
    fcfs_access_ex(&G_FCFS_PAPI_CTX, path, mode)

#define fcfs_faccessat(fd, path, mode, flags) \
    fcfs_faccessat_ex(&G_FCFS_PAPI_CTX, fd, path, mode, flags)

#define fcfs_euidaccess(path, mode) \
    fcfs_euidaccess_ex(&G_FCFS_PAPI_CTX, path, mode)

#define fcfs_eaccess(path, mode) \
    fcfs_eaccess_ex(&G_FCFS_PAPI_CTX, path, mode)

#define fcfs_utime(path, times) \
    fcfs_utime_ex(&G_FCFS_PAPI_CTX, path, times)

#define fcfs_utimes(path, times) \
    fcfs_utimes_ex(&G_FCFS_PAPI_CTX, path, times)

#define fcfs_futimes(fd, times) \
    fcfs_futimes_ex(&G_FCFS_PAPI_CTX, fd, times)

#define fcfs_futimesat(fd, path, times) \
    fcfs_futimesat_ex(&G_FCFS_PAPI_CTX, fd, path, times)

#define fcfs_futimens(fd, times) \
    fcfs_futimens_ex(&G_FCFS_PAPI_CTX, fd, times)

#define fcfs_utimensat(fd, path, times, flags) \
    fcfs_utimensat_ex(&G_FCFS_PAPI_CTX, fd, path, times, flags)

#define fcfs_unlink(path) \
    fcfs_unlink_ex(&G_FCFS_PAPI_CTX, path)

#define fcfs_unlinkat(fd, path, flags) \
    fcfs_unlinkat_ex(&G_FCFS_PAPI_CTX, fd, path, flags)

#define fcfs_rename(path1, path2) \
    fcfs_rename_ex(&G_FCFS_PAPI_CTX, path1, path2)

#define fcfs_renameat(fd1, path1, fd2, path2) \
    fcfs_renameat_ex(&G_FCFS_PAPI_CTX, fd1, path1, fd2, path2)

#define fcfs_renameat2(fd1, path1, fd2, path2, flags) \
    fcfs_renameat2_ex(&G_FCFS_PAPI_CTX, fd1, path1, fd2, path2, flags)

#define fcfs_mkdir(path, mode) \
    fcfs_mkdir_ex(&G_FCFS_PAPI_CTX, path, mode)

#define fcfs_mkdirat(fd, path, mode) \
    fcfs_mkdirat_ex(&G_FCFS_PAPI_CTX, fd, path, mode)

#define fcfs_rmdir(path) \
    fcfs_rmdir_ex(&G_FCFS_PAPI_CTX, path)

#define fcfs_chown(path, owner, group) \
    fcfs_chown_ex(&G_FCFS_PAPI_CTX, path, owner, group)

#define fcfs_lchown(path, owner, group) \
    fcfs_lchown_ex(&G_FCFS_PAPI_CTX, path, owner, group)

#define fcfs_fchown(fd, owner, group) \
    fcfs_fchown_ex(&G_FCFS_PAPI_CTX, fd, owner, group)

#define fcfs_fchownat(fd, path, owner, group, flags) \
    fcfs_fchownat_ex(&G_FCFS_PAPI_CTX, fd, path, owner, group, flags)

#define fcfs_chmod(path, mode) \
    fcfs_chmod_ex(&G_FCFS_PAPI_CTX, path, mode)

#define fcfs_fchmod(fd, mode) \
    fcfs_fchmod_ex(&G_FCFS_PAPI_CTX, fd, mode)

#define fcfs_fchmodat(fd, path, mode, flags) \
    fcfs_fchmodat_ex(&G_FCFS_PAPI_CTX, fd, path, mode, flags)

#define fcfs_statvfs(path, buf) \
    fcfs_statvfs_ex(&G_FCFS_PAPI_CTX, path, buf)

#define fcfs_fstatvfs(fd, buf) \
    fcfs_fstatvfs_ex(&G_FCFS_PAPI_CTX, fd, buf)

#define fcfs_setxattr(path, name, value, size, flags) \
    fcfs_setxattr_ex(&G_FCFS_PAPI_CTX, path, name, value, size, flags)

#define fcfs_lsetxattr(path, name, value, size, flags) \
    fcfs_lsetxattr_ex(&G_FCFS_PAPI_CTX, path, name, value, size, flags)

#define fcfs_fsetxattr(fd, name, value, size, flags) \
    fcfs_fsetxattr_ex(&G_FCFS_PAPI_CTX, fd, name, value, size, flags)

#define fcfs_getxattr(path, name, value, size) \
    fcfs_getxattr_ex(&G_FCFS_PAPI_CTX, path, name, value, size)

#define fcfs_lgetxattr(path, name, value, size) \
    fcfs_lgetxattr_ex(&G_FCFS_PAPI_CTX, path, name, value, size)

#define fcfs_fgetxattr(fd, name, value, size) \
    fcfs_fgetxattr_ex(&G_FCFS_PAPI_CTX, fd, name, value, size)

#define fcfs_listxattr(path, list, size) \
    fcfs_listxattr_ex(&G_FCFS_PAPI_CTX, path, list, size)

#define fcfs_llistxattr(path, list, size) \
    fcfs_llistxattr_ex(&G_FCFS_PAPI_CTX, path, list, size)

#define fcfs_flistxattr(fd, list, size) \
    fcfs_flistxattr_ex(&G_FCFS_PAPI_CTX, fd, list, size)

#define fcfs_removexattr(path, name) \
    fcfs_removexattr_ex(&G_FCFS_PAPI_CTX, path, name)

#define fcfs_lremovexattr(path, name) \
    fcfs_lremovexattr_ex(&G_FCFS_PAPI_CTX, path, name)

#define fcfs_fremovexattr(fd, name) \
    fcfs_fremovexattr_ex(&G_FCFS_PAPI_CTX, fd, name)

#define fcfs_opendir(path) \
    fcfs_opendir_ex(&G_FCFS_PAPI_CTX, path)

#define fcfs_fdopendir(fd) \
    fcfs_fdopendir_ex(&G_FCFS_PAPI_CTX, fd)

#define fcfs_closedir(dirp) \
    fcfs_closedir_ex(&G_FCFS_PAPI_CTX, dirp)

#define fcfs_readdir(dirp) \
    fcfs_readdir_ex(&G_FCFS_PAPI_CTX, dirp)

#define fcfs_readdir_r(dirp, entry, result) \
    fcfs_readdir_r_ex(&G_FCFS_PAPI_CTX, dirp, entry, result)

#define fcfs_seekdir(dirp, loc) \
    fcfs_seekdir_ex(&G_FCFS_PAPI_CTX, dirp, loc)

#define fcfs_telldir(dirp) \
    fcfs_telldir_ex(&G_FCFS_PAPI_CTX, dirp)

#define fcfs_rewinddir(dirp) \
    fcfs_rewinddir_ex(&G_FCFS_PAPI_CTX, dirp)

#define fcfs_dirfd(dirp) \
    fcfs_dirfd_ex(&G_FCFS_PAPI_CTX, dirp)

#define fcfs_scandir(path, namelist, filter, compar) \
    fcfs_scandir_ex(&G_FCFS_PAPI_CTX, path, namelist, filter, compar)

#define fcfs_scandirat(fd, path, namelist, filter, compar) \
    fcfs_scandirat_ex(&G_FCFS_PAPI_CTX, fd, path, namelist, filter, compar)

#define fcfs_lockf(fd, cmd, len) \
    fcfs_lockf_ex(&G_FCFS_PAPI_CTX, fd, cmd, len)

#define fcfs_posix_fallocate(fd, offset, len) \
    fcfs_posix_fallocate_ex(&G_FCFS_PAPI_CTX, fd, offset, len)

#define fcfs_posix_fadvise(fd, offset, len, advice) \
    fcfs_posix_fadvise_ex(&G_FCFS_PAPI_CTX, fd, offset, len, advice)

#define fcfs_chdir(path) \
    fcfs_chdir_ex(&G_FCFS_PAPI_CTX, path)

#define fcfs_fchdir(fd) \
    fcfs_fchdir_ex(&G_FCFS_PAPI_CTX, fd)

#define fcfs_getcwd(buf, size) \
    fcfs_getcwd_ex(&G_FCFS_PAPI_CTX, buf, size)

#define fcfs_getwd(buf) \
    fcfs_getwd_ex(&G_FCFS_PAPI_CTX, buf)

#define fcfs_chroot(path) \
    fcfs_chroot_ex(&G_FCFS_PAPI_CTX, path)

#define fcfs_dup(fd) \
    fcfs_dup_ex(&G_FCFS_PAPI_CTX, fd)

#define fcfs_dup2(fd1, fd2) \
    fcfs_dup2_ex(&G_FCFS_PAPI_CTX, fd1, fd2)

#define fcfs_mmap(addr, length, prot, flags, fd, offset) \
    fcfs_mmap_ex(&G_FCFS_PAPI_CTX, addr, length, prot, flags, fd, offset)

#define fcfs_munmap(addr, length) \
    fcfs_munmap_ex(&G_FCFS_PAPI_CTX, addr, length)

#ifdef __cplusplus
extern "C" {
#endif

    int fcfs_open_ex(FCFSPosixAPIContext *ctx,
            const char *path, int flags, ...);

    int fcfs_openat_ex(FCFSPosixAPIContext *ctx, int fd,
            const char *path, int flags, ...);

    int fcfs_creat_ex(FCFSPosixAPIContext *ctx,
            const char *path, mode_t mode);

    int fcfs_close(int fd);

    int fcfs_fsync(int fd);

    int fcfs_fdatasync(int fd);

    ssize_t fcfs_write(int fd, const void *buff, size_t count);

    ssize_t fcfs_pwrite(int fd, const void *buff,
            size_t count, off_t offset);

    ssize_t fcfs_writev(int fd, const struct iovec *iov, int iovcnt);

    ssize_t fcfs_pwritev(int fd, const struct iovec *iov,
            int iovcnt, off_t offset);

    ssize_t fcfs_read(int fd, void *buff, size_t count);

    ssize_t fcfs_pread(int fd, void *buff, size_t count, off_t offset);

    ssize_t fcfs_readv(int fd, const struct iovec *iov, int iovcnt);

    ssize_t fcfs_preadv(int fd, const struct iovec *iov,
            int iovcnt, off_t offset);

    ssize_t fcfs_readahead(int fd, off64_t offset, size_t count);

    off_t fcfs_lseek(int fd, off_t offset, int whence);

    off_t fcfs_ltell(int fd);

    int fcfs_fallocate(int fd, int mode, off_t offset, off_t length);

    int fcfs_truncate_ex(FCFSPosixAPIContext *ctx,
            const char *path, off_t length);

    int fcfs_ftruncate(int fd, off_t length);

    int fcfs_lstat_ex(FCFSPosixAPIContext *ctx,
            const char *path, struct stat *buf);

    int fcfs_stat_ex(FCFSPosixAPIContext *ctx,
            const char *path, struct stat *buf);

    int fcfs_fstat(int fd, struct stat *buf);

    int fcfs_fstatat_ex(FCFSPosixAPIContext *ctx, int fd,
            const char *path, struct stat *buf, int flags);

    int fcfs_flock(int fd, int operation);

    int fcfs_fcntl(int fd, int cmd, ...);

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

    int fcfs_mkfifo_ex(FCFSPosixAPIContext *ctx,
            const char *path, mode_t mode);

    int fcfs_mkfifoat_ex(FCFSPosixAPIContext *ctx, int fd,
            const char *path, mode_t mode);

    int fcfs_access_ex(FCFSPosixAPIContext *ctx,
            const char *path, int mode);

    int fcfs_faccessat_ex(FCFSPosixAPIContext *ctx, int fd,
            const char *path, int mode, int flags);

    int fcfs_euidaccess_ex(FCFSPosixAPIContext *ctx,
            const char *path, int mode);

    static inline int fcfs_eaccess_ex(FCFSPosixAPIContext *ctx,
            const char *path, int mode)
    {
        return fcfs_euidaccess_ex(ctx, path, mode);
    }

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

    int fcfs_lockf_ex(FCFSPosixAPIContext *ctx, int fd, int cmd, off_t len);

    int fcfs_posix_fallocate_ex(FCFSPosixAPIContext *ctx,
            int fd, off_t offset, off_t len);

    int fcfs_posix_fadvise_ex(FCFSPosixAPIContext *ctx, int fd,
            off_t offset, off_t len, int advice);

    int fcfs_dprintf(int fd, const char *format, ...)
        __gcc_attribute__ ((format (printf, 2, 3)));

    int fcfs_vdprintf(int fd, const char *format, va_list ap);

    int fcfs_chdir_ex(FCFSPosixAPIContext *ctx, const char *path);

    int fcfs_fchdir_ex(FCFSPosixAPIContext *ctx, int fd);

    char *fcfs_getcwd_ex(FCFSPosixAPIContext *ctx, char *buf, size_t size);

    char *fcfs_getwd_ex(FCFSPosixAPIContext *ctx, char *buf);

    int fcfs_chroot_ex(FCFSPosixAPIContext *ctx, const char *path);

    int fcfs_dup_ex(FCFSPosixAPIContext *ctx, int fd);

    int fcfs_dup2_ex(FCFSPosixAPIContext *ctx, int fd1, int fd2);

    void *fcfs_mmap_ex(FCFSPosixAPIContext *ctx, void *addr, size_t length,
            int prot, int flags, int fd, off_t offset);

    int fcfs_munmap_ex(FCFSPosixAPIContext *ctx, void *addr, size_t length);

    /* following functions for internal use only */
    static inline FCFSPosixAPIFileInfo *fcfs_get_file_handle(int fd)
    {
        return fcfs_fd_manager_get(fd);
    }

    int fcfs_file_open(FCFSPosixAPIContext *ctx, const char *path,
            const int flags, const int mode, const
            FCFSPosixAPITPIDType tpid_type);

    //for fread
    ssize_t fcfs_file_read(int fd, void *buff, size_t size, size_t n);

    //for fwrite
    ssize_t fcfs_file_write(int fd, const void *buff, size_t size, size_t n);

    ssize_t fcfs_file_readline(int fd, char *s, size_t size);

    //for fgets
    ssize_t fcfs_file_gets(int fd, char *s, size_t size);

    //for getdelim
    ssize_t fcfs_file_getdelim(int fd, char **line, size_t *size, int delim);

#ifdef __cplusplus
}
#endif

#endif
