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

#include <stdarg.h>
#include <sys/syscall.h>
#include "fastcommon/logger.h"
#include "global.h"
#include "api.h"

__attribute__ ((constructor)) static void preload_global_init(void)
{
    int result;
    pid_t pid;
    char *ns = "fs";
    char *config_filename = FCFS_FUSE_DEFAULT_CONFIG_FILENAME;

    pid = getpid();
    fprintf(stderr, "pid: %d, file: "__FILE__", line: %d, inited: %d, "
            "constructor\n", pid, __LINE__, g_fcfs_preload_global_vars.inited);

    log_init();
    if ((result=fcfs_preload_global_init()) != 0) {
        return;
    }

    fprintf(stderr, "pid: %d, file: "__FILE__", line: %d, "
            "constructor\n", pid, __LINE__);

    if ((result=fcfs_posix_api_init(ns, config_filename)) != 0) {
        return;
    }

    fprintf(stderr, "pid: %d, file: "__FILE__", line: %d, "
            "constructor\n", pid, __LINE__);
    if ((result=fcfs_posix_api_start()) != 0) {
        return;
    }

    g_fcfs_preload_global_vars.inited = true;
    fprintf(stderr, "pid: %d, file: "__FILE__", line: %d, "
            "constructor\n", pid, __LINE__);
}

__attribute__ ((destructor)) static void preload_global_destroy(void)
{
    fprintf(stderr, "pid: %d, file: "__FILE__", line: %d, "
            "destructor\n", getpid(), __LINE__);
}

int open(const char *path, int flags, ...)
{
    va_list ap;
    int mode;
    int result;

    va_start(ap, flags);
    mode = va_arg(ap, int);
    if (FCFS_PRELOAD_IS_MY_MOUNTPOINT(path)) {
        result = fcfs_open(path, flags, mode); 
    } else {
        result = g_fcfs_preload_global_vars.open(path, flags, mode);
    }
    va_end(ap);

    return result;
}

int creat(const char *path, mode_t mode)
{
    if (FCFS_PRELOAD_IS_MY_MOUNTPOINT(path)) {
        return fcfs_creat(path, mode);
    } else {
        return g_fcfs_preload_global_vars.creat(path, mode);
    }
}

int truncate(const char *path, off_t length)
{
    if (FCFS_PRELOAD_IS_MY_MOUNTPOINT(path)) {
        return fcfs_truncate(path, length);
    } else {
        return g_fcfs_preload_global_vars.truncate(path, length);
    }
}

int lstat(const char *path, struct stat *buf)
{
    if (FCFS_PRELOAD_IS_MY_MOUNTPOINT(path)) {
        return fcfs_lstat(path, buf);
    } else {
        return g_fcfs_preload_global_vars.lstat(path, buf);
    }
}

int stat(const char *path, struct stat *buf)
{
    if (FCFS_PRELOAD_IS_MY_MOUNTPOINT(path)) {
        return fcfs_stat(path, buf);
    } else {
        return g_fcfs_preload_global_vars.stat(path, buf);
    }
}

int link(const char *path1, const char *path2)
{
    if (FCFS_PRELOAD_IS_MY_MOUNTPOINT(path1) || FCFS_PRELOAD_IS_MY_MOUNTPOINT(path2)) {
        return fcfs_link(path1, path2);
    } else {
        return g_fcfs_preload_global_vars.link(path1, path2);
    }
}

int symlink(const char *link, const char *path)
{
    if (FCFS_PRELOAD_IS_MY_MOUNTPOINT(path)) {
        return fcfs_symlink(link, path);
    } else {
        return g_fcfs_preload_global_vars.symlink(link, path);
    }
}

ssize_t readlink(const char *path, char *buff, size_t size)
{
    if (FCFS_PRELOAD_IS_MY_MOUNTPOINT(path)) {
        return fcfs_readlink(path, buff, size);
    } else {
        return g_fcfs_preload_global_vars.readlink(path, buff, size);
    }
}

int mknod(const char *path, mode_t mode, dev_t dev)
{
    if (FCFS_PRELOAD_IS_MY_MOUNTPOINT(path)) {
        return fcfs_mknod(path, mode, dev);
    } else {
        return g_fcfs_preload_global_vars.mknod(path, mode, dev);
    }
}

int mkfifo(const char *path, mode_t mode)
{
    if (FCFS_PRELOAD_IS_MY_MOUNTPOINT(path)) {
        return fcfs_mkfifo(path, mode);
    } else {
        return g_fcfs_preload_global_vars.mkfifo(path, mode);
    }
}

int access(const char *path, int mode)
{
    if (FCFS_PRELOAD_IS_MY_MOUNTPOINT(path)) {
        return fcfs_access(path, mode);
    } else {
        if (g_fcfs_preload_global_vars.access != NULL) {
            return g_fcfs_preload_global_vars.access(path, mode);
        } else {
            //fprintf(stderr, "line: %d, path: %s, mode: %d\n", __LINE__, path, mode);
            return syscall(SYS_access, path, mode);
        }
    }
}

int utime(const char *path, const struct utimbuf *times)
{
    if (FCFS_PRELOAD_IS_MY_MOUNTPOINT(path)) {
        return fcfs_utime(path, times);
    } else {
        return g_fcfs_preload_global_vars.utime(path, times);
    }
}

int utimes(const char *path, const struct timeval times[2])
{
    if (FCFS_PRELOAD_IS_MY_MOUNTPOINT(path)) {
        return fcfs_utimes(path, times);
    } else {
        return g_fcfs_preload_global_vars.utimes(path, times);
    }
}

int unlink(const char *path)
{
    if (FCFS_PRELOAD_IS_MY_MOUNTPOINT(path)) {
        return fcfs_unlink(path);
    } else {
        return g_fcfs_preload_global_vars.unlink(path);
    }
}

int rename(const char *path1, const char *path2)
{
    if (FCFS_PRELOAD_IS_MY_MOUNTPOINT(path1) || FCFS_PRELOAD_IS_MY_MOUNTPOINT(path2)) {
        return fcfs_rename(path1, path2);
    } else {
        return g_fcfs_preload_global_vars.rename(path1, path2);
    }
}

int mkdir(const char *path, mode_t mode)
{
    if (FCFS_PRELOAD_IS_MY_MOUNTPOINT(path)) {
        return fcfs_mkdir(path, mode);
    } else {
        return g_fcfs_preload_global_vars.mkdir(path, mode);
    }
}

int rmdir(const char *path)
{
    if (FCFS_PRELOAD_IS_MY_MOUNTPOINT(path)) {
        return fcfs_rmdir(path);
    } else {
        return g_fcfs_preload_global_vars.rmdir(path);
    }
}

int chown(const char *path, uid_t owner, gid_t group)
{
    if (FCFS_PRELOAD_IS_MY_MOUNTPOINT(path)) {
        return fcfs_chown(path, owner, group);
    } else {
        return g_fcfs_preload_global_vars.chown(path, owner, group);
    }
}

int lchown(const char *path, uid_t owner, gid_t group)
{
    if (FCFS_PRELOAD_IS_MY_MOUNTPOINT(path)) {
        return fcfs_lchown(path, owner, group);
    } else {
        return g_fcfs_preload_global_vars.lchown(path, owner, group);
    }
}

int chmod(const char *path, mode_t mode)
{
    if (FCFS_PRELOAD_IS_MY_MOUNTPOINT(path)) {
        return fcfs_chmod(path, mode);
    } else {
        return g_fcfs_preload_global_vars.chmod(path, mode);
    }
}

int statvfs(const char *path, struct statvfs *buf)
{
    if (FCFS_PRELOAD_IS_MY_MOUNTPOINT(path)) {
        return fcfs_statvfs(path, buf);
    } else {
        return g_fcfs_preload_global_vars.statvfs(path, buf);
    }
}

int setxattr(const char *path, const char *name,
        const void *value, size_t size, int flags)
{
    if (FCFS_PRELOAD_IS_MY_MOUNTPOINT(path)) {
        return fcfs_setxattr(path, name, value, size, flags);
    } else {
        return g_fcfs_preload_global_vars.setxattr(
                path, name, value, size, flags);
    }
}

int lsetxattr(const char *path, const char *name,
        const void *value, size_t size, int flags)
{
    if (FCFS_PRELOAD_IS_MY_MOUNTPOINT(path)) {
        return fcfs_lsetxattr(path, name, value, size, flags);
    } else {
        return g_fcfs_preload_global_vars.lsetxattr(
                path, name, value, size, flags);
    }
}

ssize_t getxattr(const char *path, const char *name, void *value, size_t size)
{
    if (FCFS_PRELOAD_IS_MY_MOUNTPOINT(path)) {
        return fcfs_getxattr(path, name, value, size);
    } else {
        return g_fcfs_preload_global_vars.getxattr(path, name, value, size);
    }
}

ssize_t lgetxattr(const char *path, const char *name, void *value, size_t size)
{
    if (FCFS_PRELOAD_IS_MY_MOUNTPOINT(path)) {
        return fcfs_lgetxattr(path, name, value, size);
    } else {
        return g_fcfs_preload_global_vars.lgetxattr(path, name, value, size);
    }
}

ssize_t listxattr(const char *path, char *list, size_t size)
{
    if (FCFS_PRELOAD_IS_MY_MOUNTPOINT(path)) {
        return fcfs_listxattr(path, list, size);
    } else {
        return g_fcfs_preload_global_vars.listxattr(path, list, size);
    }
}

ssize_t llistxattr(const char *path, char *list, size_t size)
{
    if (FCFS_PRELOAD_IS_MY_MOUNTPOINT(path)) {
        return fcfs_llistxattr(path, list, size);
    } else {
        return g_fcfs_preload_global_vars.llistxattr(path, list, size);
    }
}

int removexattr(const char *path, const char *name)
{
    if (FCFS_PRELOAD_IS_MY_MOUNTPOINT(path)) {
        return fcfs_removexattr(path, name);
    } else {
        return g_fcfs_preload_global_vars.removexattr(path, name);
    }
}

int lremovexattr(const char *path, const char *name)
{
    if (FCFS_PRELOAD_IS_MY_MOUNTPOINT(path)) {
        return fcfs_lremovexattr(path, name);
    } else {
        return g_fcfs_preload_global_vars.lremovexattr(path, name);
    }
}

int chdir(const char *path)
{
    if (FCFS_PRELOAD_IS_MY_MOUNTPOINT(path)) {
        return fcfs_chdir(path);
    } else {
        return g_fcfs_preload_global_vars.chdir(path);
    }
}

int chroot(const char *path)
{
    if (FCFS_PRELOAD_IS_MY_MOUNTPOINT(path)) {
        return fcfs_chroot(path);
    } else {
        return g_fcfs_preload_global_vars.chroot(path);
    }
}

DIR *opendir(const char *path)
{
    FCFSPreloadDIRWrapper *wapper;
    DIR *dirp;
    FCFSPreloadCallType call_type;

    if (FCFS_PRELOAD_IS_MY_MOUNTPOINT(path)) {
        call_type = fcfs_preload_call_fastcfs;
        dirp = (DIR *)fcfs_opendir(path);
    } else {
        call_type = fcfs_preload_call_system;
        dirp = g_fcfs_preload_global_vars.opendir(path);
    }

    if (dirp != NULL) {
        wapper = fc_malloc(sizeof(FCFSPreloadDIRWrapper));
        wapper->call_type = call_type;
        wapper->dirp = dirp;
        return (DIR *)wapper;
    } else {
        return NULL;
    }
}

static inline int do_scandir(const char *path, struct dirent ***namelist,
        fcfs_dir_filter_func filter, fcfs_dir_compare_func compar)
{
    if (FCFS_PRELOAD_IS_MY_MOUNTPOINT(path)) {
        return fcfs_scandir(path, namelist, filter, compar);
    } else {
        return g_fcfs_preload_global_vars.scandir(path, namelist, filter, compar);
    }
}

int scandir(const char *path, struct dirent ***namelist,
        fcfs_dir_filter_func filter, fcfs_dir_compare_func compar)
{
    return do_scandir(path, namelist, filter, compar);
}

int scandir64(const char *path, struct dirent ***namelist,
        fcfs_dir_filter_func filter, fcfs_dir_compare_func compar)
{
    return do_scandir(path, namelist, filter, compar);
}

int close(int fd)
{
    if (FCFS_PAPI_IS_MY_FD(fd)) {
        return fcfs_close(fd);
    } else {
        return g_fcfs_preload_global_vars.close(fd);
    }
}

int fsync(int fd)
{
    if (FCFS_PAPI_IS_MY_FD(fd)) {
        return fcfs_fsync(fd);
    } else {
        return g_fcfs_preload_global_vars.fsync(fd);
    }
}

int fdatasync(int fd)
{
    if (FCFS_PAPI_IS_MY_FD(fd)) {
        return fcfs_fdatasync(fd);
    } else {
        return g_fcfs_preload_global_vars.fdatasync(fd);
    }
}

ssize_t write(int fd, const void *buff, size_t count)
{
    if (FCFS_PAPI_IS_MY_FD(fd)) {
        return fcfs_write(fd, buff, count);
    } else {
        return g_fcfs_preload_global_vars.write(fd, buff, count);
    }
}

ssize_t pwrite(int fd, const void *buff, size_t count, off_t offset)
{
    if (FCFS_PAPI_IS_MY_FD(fd)) {
        return fcfs_pwrite(fd, buff, count, offset);
    } else {
        return g_fcfs_preload_global_vars.pwrite(fd, buff, count, offset);
    }
}

ssize_t writev(int fd, const struct iovec *iov, int iovcnt)
{
    if (FCFS_PAPI_IS_MY_FD(fd)) {
        return fcfs_writev(fd, iov, iovcnt);
    } else {
        return g_fcfs_preload_global_vars.writev(fd, iov, iovcnt);
    }
}

ssize_t pwritev(int fd, const struct iovec *iov, int iovcnt, off_t offset)
{
    if (FCFS_PAPI_IS_MY_FD(fd)) {
        return fcfs_pwritev(fd, iov, iovcnt, offset);
    } else {
        return g_fcfs_preload_global_vars.pwritev(fd, iov, iovcnt, offset);
    }
}

ssize_t read(int fd, void *buff, size_t count)
{
    if (FCFS_PAPI_IS_MY_FD(fd)) {
        return fcfs_read(fd, buff, count);
    } else {
        return g_fcfs_preload_global_vars.read(fd, buff, count);
    }
}

ssize_t pread(int fd, void *buff, size_t count, off_t offset)
{
    if (FCFS_PAPI_IS_MY_FD(fd)) {
        return fcfs_pread(fd, buff, count, offset);
    } else {
        return g_fcfs_preload_global_vars.pread(fd, buff, count, offset);
    }
}

ssize_t readv(int fd, const struct iovec *iov, int iovcnt)
{
    if (FCFS_PAPI_IS_MY_FD(fd)) {
        return fcfs_readv(fd, iov, iovcnt);
    } else {
        return g_fcfs_preload_global_vars.readv(fd, iov, iovcnt);
    }
}

ssize_t preadv(int fd, const struct iovec *iov, int iovcnt, off_t offset)
{
    if (FCFS_PAPI_IS_MY_FD(fd)) {
        return fcfs_preadv(fd, iov, iovcnt, offset);
    } else {
        return g_fcfs_preload_global_vars.preadv(fd, iov, iovcnt, offset);
    }
}

off_t lseek(int fd, off_t offset, int whence)
{
    if (FCFS_PAPI_IS_MY_FD(fd)) {
        return fcfs_lseek(fd, offset, whence);
    } else {
        return g_fcfs_preload_global_vars.lseek(fd, offset, whence);
    }
}

int fallocate(int fd, int mode, off_t offset, off_t length)
{
    if (FCFS_PAPI_IS_MY_FD(fd)) {
        return fcfs_fallocate(fd, mode, offset, length);
    } else {
        return g_fcfs_preload_global_vars.fallocate(fd, mode, offset, length);
    }
}

int ftruncate(int fd, off_t length)
{
    if (FCFS_PAPI_IS_MY_FD(fd)) {
        return fcfs_ftruncate(fd, length);
    } else {
        return g_fcfs_preload_global_vars.ftruncate(fd, length);
    }
}

int fstat(int fd, struct stat *buf)
{
    if (FCFS_PAPI_IS_MY_FD(fd)) {
        return fcfs_fstat(fd, buf);
    } else {
        return g_fcfs_preload_global_vars.fstat(fd, buf);
    }
}

int flock(int fd, int operation)
{
    if (FCFS_PAPI_IS_MY_FD(fd)) {
        return fcfs_flock(fd, operation);
    } else {
        return g_fcfs_preload_global_vars.flock(fd, operation);
    }
}

int fcntl(int fd, int cmd, ...)
{
    va_list ap;
    struct flock *lock;
    int flags;

    switch (cmd) {
        case F_GETLK:
        case F_SETLK:
        case F_SETLKW:
            va_start(ap, cmd);
            lock = va_arg(ap, struct flock *);
            va_end(ap);
            if (FCFS_PAPI_IS_MY_FD(fd)) {
                return fcfs_fcntl(fd, cmd, lock); 
            } else {
                return g_fcfs_preload_global_vars.fcntl(fd, cmd, lock);
            }
        default:
            va_start(ap, cmd);
            flags = va_arg(ap, int);
            va_end(ap);
            if (FCFS_PAPI_IS_MY_FD(fd)) {
                return fcfs_fcntl(fd, cmd, flags); 
            } else {
                return g_fcfs_preload_global_vars.fcntl(fd, cmd, flags);
            }
    }
}

int futimes(int fd, const struct timeval times[2])
{
    if (FCFS_PAPI_IS_MY_FD(fd)) {
        return fcfs_futimes(fd, times);
    } else {
        return g_fcfs_preload_global_vars.futimes(fd, times);
    }
}

int futimens(int fd, const struct timespec times[2])
{
    if (FCFS_PAPI_IS_MY_FD(fd)) {
        return fcfs_futimens(fd, times);
    } else {
        return g_fcfs_preload_global_vars.futimens(fd, times);
    }
}

int fchown(int fd, uid_t owner, gid_t group)
{
    if (FCFS_PAPI_IS_MY_FD(fd)) {
        return fcfs_fchown(fd, owner, group);
    } else {
        return g_fcfs_preload_global_vars.fchown(fd, owner, group);
    }
}

int fchmod(int fd, mode_t mode)
{
    if (FCFS_PAPI_IS_MY_FD(fd)) {
        return fcfs_fchmod(fd, mode);
    } else {
        return g_fcfs_preload_global_vars.fchmod(fd, mode);
    }
}

int fsetxattr(int fd, const char *name, const
        void *value, size_t size, int flags)
{
    if (FCFS_PAPI_IS_MY_FD(fd)) {
        return fcfs_fsetxattr(fd, name, value, size, flags);
    } else {
        return g_fcfs_preload_global_vars.fsetxattr(
                fd, name, value, size, flags);
    }
}

ssize_t fgetxattr(int fd, const char *name, void *value, size_t size)
{
    if (FCFS_PAPI_IS_MY_FD(fd)) {
        return fcfs_fgetxattr(fd, name, value, size);
    } else {
        return g_fcfs_preload_global_vars.fgetxattr(fd, name, value, size);
    }
}

ssize_t flistxattr(int fd, char *list, size_t size)
{
    if (FCFS_PAPI_IS_MY_FD(fd)) {
        return fcfs_flistxattr(fd, list, size);
    } else {
        return g_fcfs_preload_global_vars.flistxattr(fd, list, size);
    }
}

int fremovexattr(int fd, const char *name)
{
    if (FCFS_PAPI_IS_MY_FD(fd)) {
        return fcfs_fremovexattr(fd, name);
    } else {
        return g_fcfs_preload_global_vars.fremovexattr(fd, name);
    }
}

int fchdir(int fd)
{
    if (FCFS_PAPI_IS_MY_FD(fd)) {
        return fcfs_fchdir(fd);
    } else {
        return g_fcfs_preload_global_vars.fchdir(fd);
    }
}

int fstatvfs(int fd, struct statvfs *buf)
{
    if (FCFS_PAPI_IS_MY_FD(fd)) {
        return fcfs_fstatvfs(fd, buf);
    } else {
        return g_fcfs_preload_global_vars.fstatvfs(fd, buf);
    }
}

int dup(int fd)
{
    if (FCFS_PAPI_IS_MY_FD(fd)) {
        return fcfs_dup(fd);
    } else {
        return g_fcfs_preload_global_vars.dup(fd);
    }
}

int dup2(int fd1, int fd2)
{
    if (FCFS_PAPI_IS_MY_FD(fd1) || FCFS_PAPI_IS_MY_FD(fd2)) {
        return fcfs_dup2(fd1, fd2);
    } else {
        return g_fcfs_preload_global_vars.dup2(fd1, fd2);
    }
}

void *mmap(void *addr, size_t length, int prot,
        int flags, int fd, off_t offset)
{
    if (FCFS_PAPI_IS_MY_FD(fd)) {
        return fcfs_mmap(addr, length, prot, flags, fd, offset);
    } else {
        return g_fcfs_preload_global_vars.mmap(addr,
                length, prot, flags, fd, offset);
    }
}

DIR *fdopendir(int fd)
{
    FCFSPreloadDIRWrapper *wapper;
    DIR *dirp;
    FCFSPreloadCallType call_type;

    if (FCFS_PAPI_IS_MY_FD(fd)) {
        call_type = fcfs_preload_call_fastcfs;
        dirp = (DIR *)fcfs_fdopendir(fd);
    } else {
        call_type = fcfs_preload_call_system;
        dirp = g_fcfs_preload_global_vars.fdopendir(fd);
    }

    if (dirp != NULL) {
        wapper = fc_malloc(sizeof(FCFSPreloadDIRWrapper));
        wapper->call_type = call_type;
        wapper->dirp = dirp;
        return (DIR *)wapper;
    } else {
        return NULL;
    }
}

int symlinkat(const char *link, int fd, const char *path)
{
    if (FCFS_PRELOAD_IS_MY_FD_MOUNTPOINT(fd, path)) {
        return fcfs_symlinkat(link, fd, path);
    } else {
        return g_fcfs_preload_global_vars.symlinkat(link, fd, path);
    }
}

int openat(int fd, const char *path, int flags, ...)
{
    va_list ap;
    int mode;
    int result;

    va_start(ap, flags);
    mode = va_arg(ap, int);
    if (FCFS_PRELOAD_IS_MY_FD_MOUNTPOINT(fd, path)) {
        result = fcfs_openat(fd, path, flags, mode);
    } else {
        result = g_fcfs_preload_global_vars.openat(fd, path, flags, mode);
    }
    return result;
}

int fstatat(int fd, const char *path, struct stat *buf, int flags)
{
    if (FCFS_PRELOAD_IS_MY_FD_MOUNTPOINT(fd, path)) {
        return fcfs_fstatat(fd, path, buf, flags);
    } else {
        return g_fcfs_preload_global_vars.fstatat(fd, path, buf, flags);
    }
}

ssize_t readlinkat(int fd, const char *path, char *buff, size_t size)
{
    if (FCFS_PRELOAD_IS_MY_FD_MOUNTPOINT(fd, path)) {
        return fcfs_readlinkat(fd, path, buff, size);
    } else {
        return g_fcfs_preload_global_vars.readlinkat(fd, path, buff, size);
    }
}

int mknodat(int fd, const char *path, mode_t mode, dev_t dev)
{
    if (FCFS_PRELOAD_IS_MY_FD_MOUNTPOINT(fd, path)) {
        return fcfs_mknodat(fd, path, mode, dev);
    } else {
        return g_fcfs_preload_global_vars.mknodat(fd, path, mode, dev);
    }
}

int mkfifoat(int fd, const char *path, mode_t mode)
{
    if (FCFS_PRELOAD_IS_MY_FD_MOUNTPOINT(fd, path)) {
        return fcfs_mkfifoat(fd, path, mode);
    } else {
        return g_fcfs_preload_global_vars.mkfifoat(fd, path, mode);
    }
}

int faccessat(int fd, const char *path, int mode, int flags)
{
    if (FCFS_PRELOAD_IS_MY_FD_MOUNTPOINT(fd, path)) {
        return fcfs_faccessat(fd, path, mode, flags);
    } else {
        return g_fcfs_preload_global_vars.faccessat(fd, path, mode, flags);
    }
}

int futimesat(int fd, const char *path, const struct timeval times[2])
{
    if (FCFS_PRELOAD_IS_MY_FD_MOUNTPOINT(fd, path)) {
        return fcfs_futimesat(fd, path, times);
    } else {
        return g_fcfs_preload_global_vars.futimesat(fd, path, times);
    }
}

int utimensat(int fd, const char *path, const struct timespec times[2], int flags)
{
    if (FCFS_PRELOAD_IS_MY_FD_MOUNTPOINT(fd, path)) {
        return fcfs_utimensat(fd, path, times, flags);
    } else {
        return g_fcfs_preload_global_vars.utimensat(fd, path, times, flags);
    }
}

int unlinkat(int fd, const char *path, int flags)
{
    if (FCFS_PRELOAD_IS_MY_FD_MOUNTPOINT(fd, path)) {
        return fcfs_unlinkat(fd, path, flags);
    } else {
        return g_fcfs_preload_global_vars.unlinkat(fd, path, flags);
    }
}

int mkdirat(int fd, const char *path, mode_t mode)
{
    if (FCFS_PRELOAD_IS_MY_FD_MOUNTPOINT(fd, path)) {
        return fcfs_mkdirat(fd, path, mode);
    } else {
        return g_fcfs_preload_global_vars.mkdirat(fd, path, mode);
    }
}

int fchownat(int fd, const char *path, uid_t owner, gid_t group, int flags)
{
    if (FCFS_PRELOAD_IS_MY_FD_MOUNTPOINT(fd, path)) {
        return fcfs_fchownat(fd, path, owner, group, flags);
    } else {
        return g_fcfs_preload_global_vars.fchownat(fd, path, owner, group, flags);
    }
}

int fchmodat(int fd, const char *path, mode_t mode, int flags)
{
    if (FCFS_PRELOAD_IS_MY_FD_MOUNTPOINT(fd, path)) {
        return fcfs_fchmodat(fd, path, mode, flags);
    } else {
        return g_fcfs_preload_global_vars.fchmodat(fd, path, mode, flags);
    }
}

int linkat(int fd1, const char *path1, int fd2, const char *path2, int flags)
{
    if (FCFS_PRELOAD_IS_MY_FD_MOUNTPOINT(fd1, path1) ||
            FCFS_PRELOAD_IS_MY_FD_MOUNTPOINT(fd2, path2))
    {
        return fcfs_linkat(fd1, path1, fd2, path2, flags);
    } else {
        return g_fcfs_preload_global_vars.linkat(fd1, path1, fd2, path2, flags);
    }
}

int renameat(int fd1, const char *path1, int fd2, const char *path2)
{
    if (FCFS_PRELOAD_IS_MY_FD_MOUNTPOINT(fd1, path1) ||
            FCFS_PRELOAD_IS_MY_FD_MOUNTPOINT(fd2, path2))
    {
        return fcfs_renameat(fd1, path1, fd2, path2);
    } else {
        return g_fcfs_preload_global_vars.renameat(fd1, path1, fd2, path2);
    }
}

int renameat2(int fd1, const char *path1, int fd2,
        const char *path2, unsigned int flags)
{
    if (FCFS_PRELOAD_IS_MY_FD_MOUNTPOINT(fd1, path1) ||
            FCFS_PRELOAD_IS_MY_FD_MOUNTPOINT(fd2, path2))
    {
        return fcfs_renameat2(fd1, path1, fd2, path2, flags);
    } else {
        return g_fcfs_preload_global_vars.renameat2(
                fd1, path1, fd2, path2, flags);
    }
}

static inline int do_scandirat(int fd, const char *path,
        struct dirent ***namelist, fcfs_dir_filter_func filter,
        fcfs_dir_compare_func compar)
{
    if (FCFS_PRELOAD_IS_MY_FD_MOUNTPOINT(fd, path)) {
        return fcfs_scandirat(fd, path, namelist, filter, compar);
    } else {
        return g_fcfs_preload_global_vars.scandirat(
                fd, path, namelist, filter, compar);
    }
}

int scandirat(int fd, const char *path, struct dirent ***namelist,
        fcfs_dir_filter_func filter, fcfs_dir_compare_func compar)
{
    return do_scandirat(fd, path, namelist, filter, compar);
}

int scandirat64(int fd, const char *path, struct dirent ***namelist,
        fcfs_dir_filter_func filter, fcfs_dir_compare_func compar)
{
    return do_scandirat(fd, path, namelist, filter, compar);
}

char *getcwd(char *buf, size_t size)
{
    if (g_fcfs_preload_global_vars.cwd_call_type ==
            fcfs_preload_call_fastcfs)
    {
        return fcfs_getcwd(buf, size);
    } else {
        return g_fcfs_preload_global_vars.getcwd(buf, size);
    }
}

char *getwd(char *buf)
{
    if (g_fcfs_preload_global_vars.cwd_call_type ==
            fcfs_preload_call_fastcfs)
    {
        return fcfs_getwd(buf);
    } else {
        return g_fcfs_preload_global_vars.getwd(buf);
    }
}

int closedir(DIR *dirp)
{
    FCFSPreloadDIRWrapper *wapper;
    int result;

    wapper = (FCFSPreloadDIRWrapper *)dirp;
    if (wapper->call_type == fcfs_preload_call_fastcfs) {
        result = fcfs_closedir((FCFSPosixAPIDIR *)wapper->dirp);
    } else {
        result = g_fcfs_preload_global_vars.closedir(wapper->dirp);
    }

    free(wapper);
    return result;
}

static inline struct dirent *do_readdir(DIR *dirp)
{
    FCFSPreloadDIRWrapper *wapper;

    wapper = (FCFSPreloadDIRWrapper *)dirp;
    if (wapper->call_type == fcfs_preload_call_fastcfs) {
        return fcfs_readdir((FCFSPosixAPIDIR *)wapper->dirp);
    } else {
        return g_fcfs_preload_global_vars.readdir(wapper->dirp);
    }
}

struct dirent *readdir(DIR *dirp)
{
    return do_readdir(dirp);
}

struct dirent64 *readdir64(DIR *dirp)
{
    return (struct dirent64 *)do_readdir(dirp);
}

static inline int do_readdir_r(DIR *dirp, struct dirent *entry,
        struct dirent **result)
{
    FCFSPreloadDIRWrapper *wapper;

    wapper = (FCFSPreloadDIRWrapper *)dirp;
    if (wapper->call_type == fcfs_preload_call_fastcfs) {
        return fcfs_readdir_r((FCFSPosixAPIDIR *)wapper->dirp, entry, result);
    } else {
        return g_fcfs_preload_global_vars.readdir_r(
                wapper->dirp, entry, result);
    }
}

int readdir_r(DIR *dirp, struct dirent *entry, struct dirent **result)
{
    return do_readdir_r(dirp, entry, result);
}

int readdir64_r(DIR *dirp, struct dirent64 *entry, struct dirent64 **result)
{
    return do_readdir_r(dirp, (struct dirent *)entry, (struct dirent **)result);
}

void seekdir(DIR *dirp, long loc)
{
    FCFSPreloadDIRWrapper *wapper;

    wapper = (FCFSPreloadDIRWrapper *)dirp;
    if (wapper->call_type == fcfs_preload_call_fastcfs) {
        return fcfs_seekdir((FCFSPosixAPIDIR *)wapper->dirp, loc);
    } else {
        return g_fcfs_preload_global_vars.seekdir(wapper->dirp, loc);
    }
}

long telldir(DIR *dirp)
{
    FCFSPreloadDIRWrapper *wapper;

    wapper = (FCFSPreloadDIRWrapper *)dirp;
    if (wapper->call_type == fcfs_preload_call_fastcfs) {
        return fcfs_telldir((FCFSPosixAPIDIR *)wapper->dirp);
    } else {
        return g_fcfs_preload_global_vars.telldir(wapper->dirp);
    }
}

void rewinddir(DIR *dirp)
{
    FCFSPreloadDIRWrapper *wapper;

    wapper = (FCFSPreloadDIRWrapper *)dirp;
    if (wapper->call_type == fcfs_preload_call_fastcfs) {
        return fcfs_rewinddir((FCFSPosixAPIDIR *)wapper->dirp);
    } else {
        return g_fcfs_preload_global_vars.rewinddir(wapper->dirp);
    }
}

int dirfd(DIR *dirp)
{
    FCFSPreloadDIRWrapper *wapper;

    wapper = (FCFSPreloadDIRWrapper *)dirp;
    if (wapper->call_type == fcfs_preload_call_fastcfs) {
        return fcfs_dirfd((FCFSPosixAPIDIR *)wapper->dirp);
    } else {
        return g_fcfs_preload_global_vars.dirfd(wapper->dirp);
    }
}
