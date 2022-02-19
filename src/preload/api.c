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
#include <dlfcn.h>
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

    fprintf(stderr, "file: "__FILE__", line: %d, inited: %d, "
            "constructor\n", __LINE__, g_fcfs_preload_global_vars.inited);

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


static inline void *fcfs_dlsym1(const char *fname)
{
    void *func;

    if ((func=dlsym(RTLD_NEXT, fname)) == NULL) {
        fprintf(stderr, "file: "__FILE__", line: %d, "
                "function %s not exist!", __LINE__, fname);
    }
    return func;
}

static inline void *fcfs_dlsym2(const char *fname1, const char *fname2)
{
    void *func;

    if ((func=dlsym(RTLD_NEXT, fname1)) != NULL) {
        return func;
    }

    if ((func=dlsym(RTLD_NEXT, fname2)) == NULL) {
        fprintf(stderr, "file: "__FILE__", line: %d, "
                "function %s | %s not exist!", __LINE__,
                fname1, fname2);
    }
    return func;
}

static inline int do_open(const char *path, int flags, int mode)
{
    int result;

    if (FCFS_PRELOAD_IS_MY_MOUNTPOINT(path)) {
        result = fcfs_open(path, flags, mode); 
    } else {
        if (g_fcfs_preload_global_vars.open != NULL) {
            result = g_fcfs_preload_global_vars.open(path, flags, mode);
        } else {
            result = syscall(SYS_open, path, flags, mode);
        }
    }

    return result;
}

int _open_(const char *path, int flags, ...)
{
    va_list ap;
    int mode;

    va_start(ap, flags);
    mode = va_arg(ap, int);
    va_end(ap);

    fprintf(stderr, "func: %s, path: %s, mode: %o\n", __FUNCTION__, path, mode);
    return do_open(path, flags, mode);
}

int open64(const char *path, int flags, ...)
{
    va_list ap;
    int mode;

    va_start(ap, flags);
    mode = va_arg(ap, int);
    va_end(ap);

    fprintf(stderr, "func: %s, path: %s, mode: %o\n", __FUNCTION__, path, mode);
    return do_open(path, flags, mode);
}

int __open(const char *path, int flags, int mode)
{
    fprintf(stderr, "func: %s, path: %s, mode: %o\n", __FUNCTION__, path, mode);
    return do_open(path, flags, mode);
}

static inline int do_creat(const char *path, mode_t mode)
{
    if (FCFS_PRELOAD_IS_MY_MOUNTPOINT(path)) {
        return fcfs_creat(path, mode);
    } else {
        if (g_fcfs_preload_global_vars.creat != NULL) {
            return g_fcfs_preload_global_vars.creat(path, mode);
        } else {
            return syscall(SYS_creat, path, mode);
        }
    }
}

int _creat_(const char *path, mode_t mode)
{
    return do_creat(path, mode);
}

int creat64(const char *path, mode_t mode)
{
    return do_creat(path, mode);
}

static inline int do_truncate(const char *path, off_t length)
{
    if (FCFS_PRELOAD_IS_MY_MOUNTPOINT(path)) {
        return fcfs_truncate(path, length);
    } else {
        if (g_fcfs_preload_global_vars.truncate != NULL) {
            return g_fcfs_preload_global_vars.truncate(path, length);
        } else {
            return syscall(SYS_truncate, path, length);
        }
    }
}

int _truncate_(const char *path, off_t length)
{
    return do_truncate(path, length);
}

int truncate64(const char *path, off_t length)
{
    return do_truncate(path, length);
}

int lstat(const char *path, struct stat *buf)
{
    fprintf(stderr, "func: %s, path: %s\n", __FUNCTION__, path);

    if (FCFS_PRELOAD_IS_MY_MOUNTPOINT(path)) {
        return fcfs_lstat(path, buf);
    } else {
        if (g_fcfs_preload_global_vars.lstat != NULL) {
            return g_fcfs_preload_global_vars.lstat(path, buf);
        } else {
            return syscall(SYS_lstat, path, buf);
        }
    }
}

static inline int do_lxstat(int ver, const char *path, struct stat *buf)
{
    fprintf(stderr, "func: %s, ver: %d, path: %s\n", __FUNCTION__, ver, path);

    if (FCFS_PRELOAD_IS_MY_MOUNTPOINT(path)) {
        return fcfs_lstat(path, buf);
    } else {
        if (g_fcfs_preload_global_vars.__lxstat != NULL) {
            return g_fcfs_preload_global_vars.__lxstat(ver, path, buf);
        } else {
            return syscall(SYS_lstat, path, buf);
        }
    }
}

int __lxstat_(int ver, const char *path, struct stat *buf)
{
    return do_lxstat(ver, path, buf);
}

int __lxstat64(int ver, const char *path, struct stat *buf)
{
    return do_lxstat(ver, path, buf);
}

int stat(const char *path, struct stat *buf)
{
    fprintf(stderr, "func: %s, path: %s\n", __FUNCTION__, path);
    if (FCFS_PRELOAD_IS_MY_MOUNTPOINT(path)) {
        return fcfs_stat(path, buf);
    } else {
        if (g_fcfs_preload_global_vars.stat != NULL) {
            return g_fcfs_preload_global_vars.stat(path, buf);
        } else {
            return syscall(SYS_stat, path, buf);
        }
    }
}

static inline int do_xstat(int ver, const char *path, struct stat *buf)
{
    fprintf(stderr, "func: %s, ver: %d, path: %s\n", __FUNCTION__, ver, path);
    if (FCFS_PRELOAD_IS_MY_MOUNTPOINT(path)) {
        return fcfs_stat(path, buf);
    } else {
        if (g_fcfs_preload_global_vars.__xstat != NULL) {
            return g_fcfs_preload_global_vars.__xstat(ver, path, buf);
        } else {
            return syscall(SYS_stat, path, buf);
        }
    }
}

int __xstat_(int ver, const char *path, struct stat *buf)
{
    int result;
    result = do_xstat(ver, path, buf);
    fprintf(stderr, "func: %s, result: %d\n", __FUNCTION__, result);
    return result;
}

int __xstat64(int ver, const char *path, struct stat *buf)
{
    int result;
    result = do_xstat(ver, path, buf);
    fprintf(stderr, "func: %s, result: %d\n", __FUNCTION__, result);
    return result;
}

int link(const char *path1, const char *path2)
{
    if (FCFS_PRELOAD_IS_MY_MOUNTPOINT(path1) ||
            FCFS_PRELOAD_IS_MY_MOUNTPOINT(path2))
    {
        return fcfs_link(path1, path2);
    } else {
        if (g_fcfs_preload_global_vars.link != NULL) {
            return g_fcfs_preload_global_vars.link(path1, path2);
        } else {
            return syscall(SYS_link, path1, path2);
        }
    }
}

int symlink(const char *link, const char *path)
{
    if (FCFS_PRELOAD_IS_MY_MOUNTPOINT(path)) {
        return fcfs_symlink(link, path);
    } else {
        if (g_fcfs_preload_global_vars.symlink != NULL) {
            return g_fcfs_preload_global_vars.symlink(link, path);
        } else {
            return syscall(SYS_symlink, link, path);
        }
    }
}

ssize_t readlink(const char *path, char *buff, size_t size)
{
    if (FCFS_PRELOAD_IS_MY_MOUNTPOINT(path)) {
        return fcfs_readlink(path, buff, size);
    } else {
        if (g_fcfs_preload_global_vars.readlink != NULL) {
            return g_fcfs_preload_global_vars.readlink(path, buff, size);
        } else {
            return syscall(SYS_readlink, path, buff, size);
        }
    }
}

static inline int do_mknod(const char *path, mode_t mode, dev_t dev)
{
    if (FCFS_PRELOAD_IS_MY_MOUNTPOINT(path)) {
        return fcfs_mknod(path, mode, dev);
    } else {
        if (g_fcfs_preload_global_vars.mknod != NULL) {
            return g_fcfs_preload_global_vars.mknod(path, mode, dev);
        } else {
            return syscall(SYS_mknod, path, mode, dev);
        }
    }
}

int mknod(const char *path, mode_t mode, dev_t dev)
{
    return do_mknod(path, mode, dev);
}

int __xmknod(const char *path, mode_t mode, dev_t dev)
{
    return do_mknod(path, mode, dev);
}

int mkfifo(const char *path, mode_t mode)
{
    if (FCFS_PRELOAD_IS_MY_MOUNTPOINT(path)) {
        return fcfs_mkfifo(path, mode);
    } else {
        if (g_fcfs_preload_global_vars.mkfifo == NULL) {
            g_fcfs_preload_global_vars.mkfifo = fcfs_dlsym1("mkfifo");
        }
        return g_fcfs_preload_global_vars.mkfifo(path, mode);
    }
}

int access(const char *path, int mode)
{
    fprintf(stderr, "func: %s, path: %s, mode: %o\n", __FUNCTION__, path, mode);
    if (FCFS_PRELOAD_IS_MY_MOUNTPOINT(path)) {
        return fcfs_access(path, mode);
    } else {
        if (g_fcfs_preload_global_vars.access != NULL) {
            return g_fcfs_preload_global_vars.access(path, mode);
        } else {
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
        if (g_fcfs_preload_global_vars.utimes != NULL) {
            return g_fcfs_preload_global_vars.utimes(path, times);
        } else {
            return syscall(SYS_utimes, path, times);
        }
    }
}

int unlink(const char *path)
{
    if (FCFS_PRELOAD_IS_MY_MOUNTPOINT(path)) {
        return fcfs_unlink(path);
    } else {
        if (g_fcfs_preload_global_vars.unlink != NULL) {
            return g_fcfs_preload_global_vars.unlink(path);
        } else {
            return syscall(SYS_unlink, path);
        }
    }
}

int rename(const char *path1, const char *path2)
{
    if (FCFS_PRELOAD_IS_MY_MOUNTPOINT(path1) ||
            FCFS_PRELOAD_IS_MY_MOUNTPOINT(path2))
    {
        return fcfs_rename(path1, path2);
    } else {
        if (g_fcfs_preload_global_vars.rename != NULL) {
            return g_fcfs_preload_global_vars.rename(path1, path2);
        } else {
            return syscall(SYS_rename, path1, path2);
        }
    }
}

int mkdir(const char *path, mode_t mode)
{
    if (FCFS_PRELOAD_IS_MY_MOUNTPOINT(path)) {
        return fcfs_mkdir(path, mode);
    } else {
        if (g_fcfs_preload_global_vars.mkdir != NULL) {
            return g_fcfs_preload_global_vars.mkdir(path, mode);
        } else {
            return syscall(SYS_mkdir, path, mode);
        }
    }
}

int rmdir(const char *path)
{
    if (FCFS_PRELOAD_IS_MY_MOUNTPOINT(path)) {
        return fcfs_rmdir(path);
    } else {
        if (g_fcfs_preload_global_vars.rmdir != NULL) {
            return g_fcfs_preload_global_vars.rmdir(path);
        } else {
            return syscall(SYS_rmdir, path);
        }
    }
}

int chown(const char *path, uid_t owner, gid_t group)
{
    if (FCFS_PRELOAD_IS_MY_MOUNTPOINT(path)) {
        return fcfs_chown(path, owner, group);
    } else {
        if (g_fcfs_preload_global_vars.chown != NULL) {
            return g_fcfs_preload_global_vars.chown(path, owner, group);
        } else {
            return syscall(SYS_chown, path, owner, group);
        }
    }
}

int lchown(const char *path, uid_t owner, gid_t group)
{
    if (FCFS_PRELOAD_IS_MY_MOUNTPOINT(path)) {
        return fcfs_lchown(path, owner, group);
    } else {
        if (g_fcfs_preload_global_vars.lchown != NULL) {
            return g_fcfs_preload_global_vars.lchown(path, owner, group);
        } else {
            return syscall(SYS_lchown, path, owner, group);
        }
    }
}

int chmod(const char *path, mode_t mode)
{
    if (FCFS_PRELOAD_IS_MY_MOUNTPOINT(path)) {
        return fcfs_chmod(path, mode);
    } else {
        if (g_fcfs_preload_global_vars.chmod != NULL) {
            return g_fcfs_preload_global_vars.chmod(path, mode);
        } else {
            return syscall(SYS_chmod, path, mode);
        }
    }
}

int statvfs(const char *path, struct statvfs *buf)
{
    if (FCFS_PRELOAD_IS_MY_MOUNTPOINT(path)) {
        return fcfs_statvfs(path, buf);
    } else {
        if (g_fcfs_preload_global_vars.statvfs == NULL) {
            g_fcfs_preload_global_vars.statvfs = fcfs_dlsym1("statvfs");
        }
        return g_fcfs_preload_global_vars.statvfs(path, buf);
    }
}

int setxattr(const char *path, const char *name,
        const void *value, size_t size, int flags)
{
    if (FCFS_PRELOAD_IS_MY_MOUNTPOINT(path)) {
        return fcfs_setxattr(path, name, value, size, flags);
    } else {
        if (g_fcfs_preload_global_vars.setxattr != NULL) {
            return g_fcfs_preload_global_vars.setxattr(
                    path, name, value, size, flags);
        } else {
            return syscall(SYS_setxattr, path, name, value, size, flags);
        }
    }
}

int lsetxattr(const char *path, const char *name,
        const void *value, size_t size, int flags)
{
    if (FCFS_PRELOAD_IS_MY_MOUNTPOINT(path)) {
        return fcfs_lsetxattr(path, name, value, size, flags);
    } else {
        if (g_fcfs_preload_global_vars.lsetxattr != NULL) {
            return g_fcfs_preload_global_vars.lsetxattr(
                    path, name, value, size, flags);
        } else {
            return syscall(SYS_lsetxattr, path, name, value, size, flags);
        }
    }
}

ssize_t getxattr(const char *path, const char *name, void *value, size_t size)
{
    if (FCFS_PRELOAD_IS_MY_MOUNTPOINT(path)) {
        return fcfs_getxattr(path, name, value, size);
    } else {
        if (g_fcfs_preload_global_vars.getxattr != NULL) {
            return g_fcfs_preload_global_vars.getxattr(path, name, value, size);
        } else {
            return syscall(SYS_getxattr, path, name, value, size);
        }
    }
}

ssize_t lgetxattr(const char *path, const char *name, void *value, size_t size)
{
    if (FCFS_PRELOAD_IS_MY_MOUNTPOINT(path)) {
        return fcfs_lgetxattr(path, name, value, size);
    } else {
        if (g_fcfs_preload_global_vars.lgetxattr != NULL) {
            return g_fcfs_preload_global_vars.lgetxattr(path, name, value, size);
        } else {
            return syscall(SYS_lgetxattr, path, name, value, size);
        }
    }
}

ssize_t listxattr(const char *path, char *list, size_t size)
{
    if (FCFS_PRELOAD_IS_MY_MOUNTPOINT(path)) {
        return fcfs_listxattr(path, list, size);
    } else {
        if (g_fcfs_preload_global_vars.listxattr != NULL) {
            return g_fcfs_preload_global_vars.listxattr(path, list, size);
        } else {
            return syscall(SYS_listxattr, path, list, size);
        }
    }
}

ssize_t llistxattr(const char *path, char *list, size_t size)
{
    if (FCFS_PRELOAD_IS_MY_MOUNTPOINT(path)) {
        return fcfs_llistxattr(path, list, size);
    } else {
        if (g_fcfs_preload_global_vars.llistxattr != NULL) {
            return g_fcfs_preload_global_vars.llistxattr(path, list, size);
        } else {
            return syscall(SYS_llistxattr, path, list, size);
        }
    }
}

int removexattr(const char *path, const char *name)
{
    if (FCFS_PRELOAD_IS_MY_MOUNTPOINT(path)) {
        return fcfs_removexattr(path, name);
    } else {
        if (g_fcfs_preload_global_vars.removexattr != NULL) {
            return g_fcfs_preload_global_vars.removexattr(path, name);
        } else {
            return syscall(SYS_removexattr, path, name);
        }
    }
}

int lremovexattr(const char *path, const char *name)
{
    if (FCFS_PRELOAD_IS_MY_MOUNTPOINT(path)) {
        return fcfs_lremovexattr(path, name);
    } else {
        if (g_fcfs_preload_global_vars.lremovexattr != NULL) {
            return g_fcfs_preload_global_vars.lremovexattr(path, name);
        } else {
            return syscall(SYS_lremovexattr, path, name);
        }
    }
}

int chdir(const char *path)
{
    if (FCFS_PRELOAD_IS_MY_MOUNTPOINT(path)) {
        return fcfs_chdir(path);
    } else {
        if (g_fcfs_preload_global_vars.chdir != NULL) {
            return g_fcfs_preload_global_vars.chdir(path);
        } else {
            return syscall(SYS_chdir, path);
        }
    }
}

int chroot(const char *path)
{
    if (FCFS_PRELOAD_IS_MY_MOUNTPOINT(path)) {
        return fcfs_chroot(path);
    } else {
        if (g_fcfs_preload_global_vars.chroot != NULL) {
            return g_fcfs_preload_global_vars.chroot(path);
        } else {
            return syscall(SYS_chroot, path);
        }
    }
}

DIR *opendir(const char *path)
{
    FCFSPreloadDIRWrapper *wapper;
    DIR *dirp;
    FCFSPreloadCallType call_type;

    fprintf(stderr, "func: %s, path: %s\n", __FUNCTION__, path);

    if (FCFS_PRELOAD_IS_MY_MOUNTPOINT(path)) {
        call_type = fcfs_preload_call_fastcfs;
        dirp = (DIR *)fcfs_opendir(path);
    } else {
        call_type = fcfs_preload_call_system;
        if (g_fcfs_preload_global_vars.opendir == NULL) {
            g_fcfs_preload_global_vars.opendir = fcfs_dlsym1("opendir");
        }
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
        if (g_fcfs_preload_global_vars.scandir == NULL) {
            g_fcfs_preload_global_vars.scandir = fcfs_dlsym1("scandir");
        }
        return g_fcfs_preload_global_vars.scandir(path, namelist, filter, compar);
    }
}

int _scandir_(const char *path, struct dirent ***namelist,
        fcfs_dir_filter_func filter, fcfs_dir_compare_func compar)
{
    return do_scandir(path, namelist, filter, compar);
}

int scandir64(const char * path, struct dirent64 ***namelist,
        int (*filter) (const struct dirent64 *),
        int (*compar) (const struct dirent64 **,
            const struct dirent64 **))
{
    return do_scandir(path, (struct dirent ***)namelist,
            (fcfs_dir_filter_func)filter,
            (fcfs_dir_compare_func)compar);
}

int close(int fd)
{
    if (FCFS_PAPI_IS_MY_FD(fd)) {
        return fcfs_close(fd);
    } else {
        if (g_fcfs_preload_global_vars.close != NULL) {
            return g_fcfs_preload_global_vars.close(fd);
        } else {
            return syscall(SYS_close, fd);
        }
    }
}

int fsync(int fd)
{
    if (FCFS_PAPI_IS_MY_FD(fd)) {
        return fcfs_fsync(fd);
    } else {
        if (g_fcfs_preload_global_vars.fsync != NULL) {
            return g_fcfs_preload_global_vars.fsync(fd);
        } else {
            return syscall(SYS_fsync, fd);
        }
    }
}

int fdatasync(int fd)
{
    if (FCFS_PAPI_IS_MY_FD(fd)) {
        return fcfs_fdatasync(fd);
    } else {
        if (g_fcfs_preload_global_vars.fdatasync != NULL) {
            return g_fcfs_preload_global_vars.fdatasync(fd);
        } else {
            return syscall(SYS_fdatasync, fd);
        }
    }
}

ssize_t write(int fd, const void *buff, size_t count)
{
    if (FCFS_PAPI_IS_MY_FD(fd)) {
        return fcfs_write(fd, buff, count);
    } else {
        if (g_fcfs_preload_global_vars.write != NULL) {
            return g_fcfs_preload_global_vars.write(fd, buff, count);
        } else {
            return syscall(SYS_write, fd, buff, count);
        }
    }
}

static inline ssize_t do_pwrite(int fd, const void *buff,
        size_t count, off_t offset)
{
    if (FCFS_PAPI_IS_MY_FD(fd)) {
        return fcfs_pwrite(fd, buff, count, offset);
    } else {
        if (g_fcfs_preload_global_vars.pwrite == NULL) {
            g_fcfs_preload_global_vars.pwrite = fcfs_dlsym1("pwrite");
        }
        return g_fcfs_preload_global_vars.pwrite(fd, buff, count, offset);
    }
}

ssize_t _pwrite_(int fd, const void *buff, size_t count, off_t offset)
{
    return do_pwrite(fd, buff, count, offset);
}

ssize_t pwrite64(int fd, const void *buff, size_t count, off_t offset)
{
    return do_pwrite(fd, buff, count, offset);
}

ssize_t writev(int fd, const struct iovec *iov, int iovcnt)
{
    if (FCFS_PAPI_IS_MY_FD(fd)) {
        return fcfs_writev(fd, iov, iovcnt);
    } else {
        if (g_fcfs_preload_global_vars.writev != NULL) {
            return g_fcfs_preload_global_vars.writev(fd, iov, iovcnt);
        } else {
            return syscall(SYS_writev, fd, iov, iovcnt);
        }
    }
}

static inline ssize_t do_pwritev(int fd, const struct iovec *iov,
        int iovcnt, off_t offset)
{
    if (FCFS_PAPI_IS_MY_FD(fd)) {
        return fcfs_pwritev(fd, iov, iovcnt, offset);
    } else {
        if (g_fcfs_preload_global_vars.pwritev == NULL) {
            g_fcfs_preload_global_vars.pwritev = fcfs_dlsym1("pwritev");
        }
        return g_fcfs_preload_global_vars.pwritev(fd, iov, iovcnt, offset);
    }
}

ssize_t _pwritev_(int fd, const struct iovec *iov, int iovcnt, off_t offset)
{
    return do_pwritev(fd, iov, iovcnt, offset);
}

ssize_t pwritev64(int fd, const struct iovec *iov, int iovcnt, off_t offset)
{
    return do_pwritev(fd, iov, iovcnt, offset);
}

ssize_t read(int fd, void *buff, size_t count)
{
    fprintf(stderr, "func: %s, fd: %d\n", __FUNCTION__, fd);
    if (FCFS_PAPI_IS_MY_FD(fd)) {
        return fcfs_read(fd, buff, count);
    } else {
        if (g_fcfs_preload_global_vars.read != NULL) {
            return g_fcfs_preload_global_vars.read(fd, buff, count);
        } else {
            return syscall(SYS_read, fd, buff, count);
        }
    }
}

ssize_t __read_chk(int fd, void *buff, size_t count, size_t size)
{
    fprintf(stderr, "func: %s, fd: %d\n", __FUNCTION__, fd);
    if (FCFS_PAPI_IS_MY_FD(fd)) {
        return fcfs_read(fd, buff, count);
    } else {
        if (g_fcfs_preload_global_vars.__read_chk == NULL) {
            g_fcfs_preload_global_vars.__read_chk = fcfs_dlsym1("__read_chk");
        }
        return g_fcfs_preload_global_vars.__read_chk(fd, buff, count, size);
    }
}

static inline ssize_t do_pread(int fd, void *buff, size_t count, off_t offset)
{
    fprintf(stderr, "func: %s, fd: %d\n", __FUNCTION__, fd);
    if (FCFS_PAPI_IS_MY_FD(fd)) {
        return fcfs_pread(fd, buff, count, offset);
    } else {
        if (g_fcfs_preload_global_vars.pread == NULL) {
            g_fcfs_preload_global_vars.pread = fcfs_dlsym1("pread");
        }
        return g_fcfs_preload_global_vars.pread(fd, buff, count, offset);
    }
}

ssize_t _pread_(int fd, void *buff, size_t count, off_t offset)
{
    return do_pread(fd, buff, count, offset);
}

ssize_t pread64(int fd, void *buff, size_t count, off_t offset)
{
    return do_pread(fd, buff, count, offset);
}

ssize_t readv(int fd, const struct iovec *iov, int iovcnt)
{
    fprintf(stderr, "func: %s, fd: %d\n", __FUNCTION__, fd);
    if (FCFS_PAPI_IS_MY_FD(fd)) {
        return fcfs_readv(fd, iov, iovcnt);
    } else {
        if (g_fcfs_preload_global_vars.readv != NULL) {
            return g_fcfs_preload_global_vars.readv(fd, iov, iovcnt);
        } else {
            return syscall(SYS_readv, fd, iov, iovcnt);
        }
    }
}

static inline ssize_t do_preadv(int fd, const struct iovec *iov,
        int iovcnt, off_t offset)
{
    if (FCFS_PAPI_IS_MY_FD(fd)) {
        return fcfs_preadv(fd, iov, iovcnt, offset);
    } else {
        if (g_fcfs_preload_global_vars.preadv == NULL) {
            g_fcfs_preload_global_vars.preadv = fcfs_dlsym1("preadv");
        }
        return g_fcfs_preload_global_vars.preadv(fd, iov, iovcnt, offset);
    }
}

ssize_t _preadv_(int fd, const struct iovec *iov, int iovcnt, off_t offset)
{
    fprintf(stderr, "func: %s, fd: %d\n", __FUNCTION__, fd);
    return do_preadv(fd, iov, iovcnt, offset);
}

ssize_t preadv64(int fd, const struct iovec *iov, int iovcnt, off_t offset)
{
    fprintf(stderr, "func: %s, fd: %d\n", __FUNCTION__, fd);
    return do_preadv(fd, iov, iovcnt, offset);
}

static inline off_t do_lseek(int fd, off_t offset, int whence)
{
    if (FCFS_PAPI_IS_MY_FD(fd)) {
        return fcfs_lseek(fd, offset, whence);
    } else {
        if (g_fcfs_preload_global_vars.lseek != NULL) {
            return g_fcfs_preload_global_vars.lseek(fd, offset, whence);
        } else {
            return syscall(SYS_lseek, fd, offset, whence);
        }
    }
}

off_t _lseek_(int fd, off_t offset, int whence)
{
    fprintf(stderr, "func: %s, fd: %d\n", __FUNCTION__, fd);
    return do_lseek(fd, offset, whence);
}

off_t lseek64(int fd, off_t offset, int whence)
{
    fprintf(stderr, "func: %s, fd: %d\n", __FUNCTION__, fd);
    return do_lseek(fd, offset, whence);
}

static inline int do_fallocate(int fd, int mode, off_t offset, off_t length)
{
    if (FCFS_PAPI_IS_MY_FD(fd)) {
        return fcfs_fallocate(fd, mode, offset, length);
    } else {
        if (g_fcfs_preload_global_vars.fallocate == NULL) {
            g_fcfs_preload_global_vars.fallocate = fcfs_dlsym1("fallocate");
        }
        return g_fcfs_preload_global_vars.fallocate(fd, mode, offset, length);
    }
}

int _fallocate_(int fd, int mode, off_t offset, off_t length)
{
    return do_fallocate(fd, mode, offset, length);
}

int fallocate64(int fd, int mode, off_t offset, off_t length)
{
    return do_fallocate(fd, mode, offset, length);
}

static inline int do_ftruncate(int fd, off_t length)
{
    if (FCFS_PAPI_IS_MY_FD(fd)) {
        return fcfs_ftruncate(fd, length);
    } else {
        if (g_fcfs_preload_global_vars.ftruncate != NULL) {
            return g_fcfs_preload_global_vars.ftruncate(fd, length);
        } else {
            return syscall(SYS_ftruncate, fd, length);
        }
    }
}

int _ftruncate_(int fd, off_t length)
{
    return do_ftruncate(fd, length);
}

int ftruncate64(int fd, off_t length)
{
    return do_ftruncate(fd, length);
}

int fstat(int fd, struct stat *buf)
{
    fprintf(stderr, "func: %s, fd: %d\n", __FUNCTION__, fd);
    if (FCFS_PAPI_IS_MY_FD(fd)) {
        return fcfs_fstat(fd, buf);
    } else {
        if (g_fcfs_preload_global_vars.fstat != NULL) {
            return g_fcfs_preload_global_vars.fstat(fd, buf);
        } else {
            return syscall(SYS_fstat, fd, buf);
        }
    }
}

static inline int do_fxstat(int ver, int fd, struct stat *buf)
{
    if (FCFS_PAPI_IS_MY_FD(fd)) {
        return fcfs_fstat(fd, buf);
    } else {
        if (g_fcfs_preload_global_vars.__fxstat != NULL) {
            return g_fcfs_preload_global_vars.__fxstat(ver, fd, buf);
        } else {
            return syscall(SYS_fstat, fd, buf);
        }
    }
}

int __fxstat_(int ver, int fd, struct stat *buf)
{
    fprintf(stderr, "func: %s, ver: %d, fd: %d\n", __FUNCTION__, ver, fd);
    return do_fxstat(ver, fd, buf);
}

int __fxstat64(int ver, int fd, struct stat *buf)
{
    fprintf(stderr, "func: %s, ver: %d, fd: %d\n", __FUNCTION__, ver, fd);
    return do_fxstat(ver, fd, buf);
}

int flock(int fd, int operation)
{
    fprintf(stderr, "func: %s, fd: %d, operation: %d\n", __FUNCTION__, fd, operation);
    if (FCFS_PAPI_IS_MY_FD(fd)) {
        return fcfs_flock(fd, operation);
    } else {
        if (g_fcfs_preload_global_vars.flock != NULL) {
            return g_fcfs_preload_global_vars.flock(fd, operation);
        } else {
            return syscall(SYS_flock, fd, operation);
        }
    }
}

static int do_fcntl(int fd, int cmd, void *arg)
{
    struct flock *lock;
    int flags;

    switch (cmd) {
        case F_GETLK:
        case F_SETLK:
        case F_SETLKW:
            lock = (struct flock *)arg;
            if (FCFS_PAPI_IS_MY_FD(fd)) {
                return fcfs_fcntl(fd, cmd, lock); 
            } else {
                if (g_fcfs_preload_global_vars.fcntl != NULL) {
                    return g_fcfs_preload_global_vars.fcntl(fd, cmd, lock);
                } else {
                    return syscall(SYS_fcntl, fd, cmd, lock);
                }
            }
        default:
            flags = (long)arg;
            if (FCFS_PAPI_IS_MY_FD(fd)) {
                return fcfs_fcntl(fd, cmd, flags); 
            } else {
                if (g_fcfs_preload_global_vars.fcntl != NULL) {
                    return g_fcfs_preload_global_vars.fcntl(fd, cmd, flags);
                } else {
                    return syscall(SYS_fcntl, fd, cmd, flags);
                }
            }
    }
}

int _fcntl_(int fd, int cmd, ...)
{
    va_list ap;
    void *arg;

    fprintf(stderr, "func: %s, fd: %d, cmd: %d\n", __FUNCTION__, fd, cmd);
    va_start(ap, cmd);
    arg = va_arg(ap, void *);
    va_end(ap);

    return do_fcntl(fd, cmd, arg);
}

int fcntl64(int fd, int cmd, ...)
{
    va_list ap;
    void *arg;

    fprintf(stderr, "func: %s, fd: %d, cmd: %d\n", __FUNCTION__, fd, cmd);
    va_start(ap, cmd);
    arg = va_arg(ap, void *);
    va_end(ap);

    return do_fcntl(fd, cmd, arg);
}

int futimes(int fd, const struct timeval times[2])
{
    if (FCFS_PAPI_IS_MY_FD(fd)) {
        return fcfs_futimes(fd, times);
    } else {
        if (g_fcfs_preload_global_vars.futimes == NULL) {
            g_fcfs_preload_global_vars.futimes = fcfs_dlsym1("futimes");
        }
        return g_fcfs_preload_global_vars.futimes(fd, times);
    }
}

int futimens(int fd, const struct timespec times[2])
{
    if (FCFS_PAPI_IS_MY_FD(fd)) {
        return fcfs_futimens(fd, times);
    } else {
        if (g_fcfs_preload_global_vars.futimens == NULL) {
            g_fcfs_preload_global_vars.futimens = fcfs_dlsym1("futimens");
        }
        return g_fcfs_preload_global_vars.futimens(fd, times);
    }
}

int fchown(int fd, uid_t owner, gid_t group)
{
    if (FCFS_PAPI_IS_MY_FD(fd)) {
        return fcfs_fchown(fd, owner, group);
    } else {
        if (g_fcfs_preload_global_vars.fchown != NULL) {
            return g_fcfs_preload_global_vars.fchown(fd, owner, group);
        } else {
            return syscall(SYS_fchown, fd, owner, group);
        }
    }
}

int fchmod(int fd, mode_t mode)
{
    if (FCFS_PAPI_IS_MY_FD(fd)) {
        return fcfs_fchmod(fd, mode);
    } else {
        if (g_fcfs_preload_global_vars.fchmod != NULL) {
            return g_fcfs_preload_global_vars.fchmod(fd, mode);
        } else {
            return syscall(SYS_fchmod, fd, mode);
        }
    }
}

int fsetxattr(int fd, const char *name, const
        void *value, size_t size, int flags)
{
    if (FCFS_PAPI_IS_MY_FD(fd)) {
        return fcfs_fsetxattr(fd, name, value, size, flags);
    } else {
        if (g_fcfs_preload_global_vars.fsetxattr != NULL) {
            return g_fcfs_preload_global_vars.fsetxattr(
                    fd, name, value, size, flags);
        } else {
            return syscall(SYS_fsetxattr, fd, name, value, size, flags);
        }
    }
}

ssize_t fgetxattr(int fd, const char *name, void *value, size_t size)
{
    if (FCFS_PAPI_IS_MY_FD(fd)) {
        return fcfs_fgetxattr(fd, name, value, size);
    } else {
        if (g_fcfs_preload_global_vars.fgetxattr != NULL) {
            return g_fcfs_preload_global_vars.fgetxattr(fd, name, value, size);
        } else {
            return syscall(SYS_fgetxattr, fd, name, value, size);
        }
    }
}

ssize_t flistxattr(int fd, char *list, size_t size)
{
    if (FCFS_PAPI_IS_MY_FD(fd)) {
        return fcfs_flistxattr(fd, list, size);
    } else {
        if (g_fcfs_preload_global_vars.flistxattr != NULL) {
            return g_fcfs_preload_global_vars.flistxattr(fd, list, size);
        } else {
            return syscall(SYS_flistxattr, fd, list, size);
        }
    }
}

int fremovexattr(int fd, const char *name)
{
    if (FCFS_PAPI_IS_MY_FD(fd)) {
        return fcfs_fremovexattr(fd, name);
    } else {
        if (g_fcfs_preload_global_vars.fremovexattr != NULL) {
            return g_fcfs_preload_global_vars.fremovexattr(fd, name);
        } else {
            return syscall(SYS_fremovexattr, fd, name);
        }
    }
}

int fchdir(int fd)
{
    if (FCFS_PAPI_IS_MY_FD(fd)) {
        return fcfs_fchdir(fd);
    } else {
        if (g_fcfs_preload_global_vars.fchdir != NULL) {
            return g_fcfs_preload_global_vars.fchdir(fd);
        } else {
            return syscall(SYS_fchdir, fd);
        }
    }
}

int fstatvfs(int fd, struct statvfs *buf)
{
    if (FCFS_PAPI_IS_MY_FD(fd)) {
        return fcfs_fstatvfs(fd, buf);
    } else {
        if (g_fcfs_preload_global_vars.fstatvfs == NULL) {
            g_fcfs_preload_global_vars.fstatvfs = fcfs_dlsym1("fstatvfs");
        }
        return g_fcfs_preload_global_vars.fstatvfs(fd, buf);
    }
}

int dup(int fd)
{
    if (FCFS_PAPI_IS_MY_FD(fd)) {
        return fcfs_dup(fd);
    } else {
        if (g_fcfs_preload_global_vars.dup != NULL) {
            return g_fcfs_preload_global_vars.dup(fd);
        } else {
            return syscall(SYS_dup, fd);
        }
    }
}

int dup2(int fd1, int fd2)
{
    if (FCFS_PAPI_IS_MY_FD(fd1) || FCFS_PAPI_IS_MY_FD(fd2)) {
        return fcfs_dup2(fd1, fd2);
    } else {
        if (g_fcfs_preload_global_vars.dup2 != NULL) {
            return g_fcfs_preload_global_vars.dup2(fd1, fd2);
        } else {
            return syscall(SYS_dup2, fd1, fd2);
        }
    }
}

static inline void *do_mmap(void *addr, size_t length, int prot,
        int flags, int fd, off_t offset)
{
    if (FCFS_PAPI_IS_MY_FD(fd)) {
        return fcfs_mmap(addr, length, prot, flags, fd, offset);
    } else {
        if (g_fcfs_preload_global_vars.mmap == NULL) {
            g_fcfs_preload_global_vars.mmap = fcfs_dlsym1("mmap");
        }
        return g_fcfs_preload_global_vars.mmap(addr,
                length, prot, flags, fd, offset);
    }
}

void *_mmap_(void *addr, size_t length, int prot,
        int flags, int fd, off_t offset)
{
    return do_mmap(addr, length, prot, flags, fd, offset);
}

void *mmap64(void *addr, size_t length, int prot,
        int flags, int fd, off_t offset)
{
    return do_mmap(addr, length, prot, flags, fd, offset);
}

DIR *fdopendir(int fd)
{
    FCFSPreloadDIRWrapper *wapper;
    DIR *dirp;
    FCFSPreloadCallType call_type;

    fprintf(stderr, "func: %s, fd: %d\n", __FUNCTION__, fd);
    if (FCFS_PAPI_IS_MY_FD(fd)) {
        call_type = fcfs_preload_call_fastcfs;
        dirp = (DIR *)fcfs_fdopendir(fd);
    } else {
        call_type = fcfs_preload_call_system;
        if (g_fcfs_preload_global_vars.fdopendir == NULL) {
            g_fcfs_preload_global_vars.fdopendir = fcfs_dlsym1("fdopendir");
        }
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
        if (g_fcfs_preload_global_vars.symlinkat != NULL) {
            return g_fcfs_preload_global_vars.symlinkat(link, fd, path);
        } else {
            return syscall(SYS_symlinkat, link, fd, path);
        }
    }
}

static inline int do_openat(int fd, const char *path, int flags, int mode)
{
    if (FCFS_PRELOAD_IS_MY_FD_MOUNTPOINT(fd, path)) {
        return fcfs_openat(fd, path, flags, mode);
    } else {
        if (g_fcfs_preload_global_vars.openat != NULL) {
            return g_fcfs_preload_global_vars.openat(fd, path, flags, mode);
        } else {
            return syscall(SYS_openat, fd, path, flags, mode);
        }
    }
}

int _openat_(int fd, const char *path, int flags, ...)
{
    va_list ap;
    int mode;

    va_start(ap, flags);
    mode = va_arg(ap, int);
    va_end(ap);

    fprintf(stderr, "func: %s, path: %s, mode: %o\n", __FUNCTION__, path, mode);
    return do_openat(fd, path, flags, mode);
}

int openat64(int fd, const char *path, int flags, ...)
{
    va_list ap;
    int mode;

    va_start(ap, flags);
    mode = va_arg(ap, int);
    va_end(ap);

    fprintf(stderr, "func: %s, path: %s, mode: %o\n", __FUNCTION__, path, mode);
    return do_openat(fd, path, flags, mode);
}

int fstatat(int fd, const char *path, struct stat *buf, int flags)
{
    fprintf(stderr, "func: %s, fd: %d, path: %s\n", __FUNCTION__, fd, path);
    if (FCFS_PRELOAD_IS_MY_FD_MOUNTPOINT(fd, path)) {
        return fcfs_fstatat(fd, path, buf, flags);
    } else {
        if (g_fcfs_preload_global_vars.fstatat == NULL) {
            g_fcfs_preload_global_vars.fstatat = fcfs_dlsym1("fstatat");
        }
        return g_fcfs_preload_global_vars.fstatat(fd, path, buf, flags);
    }
}

static inline int do_fxstatat(int ver, int fd,
        const char *path, struct stat *buf, int flags)
{
    fprintf(stderr, "line: %d, func: %s, fd: %d, path: %s\n", __LINE__, __FUNCTION__, fd, path);
    if (FCFS_PRELOAD_IS_MY_FD_MOUNTPOINT(fd, path)) {
        return fcfs_fstatat(fd, path, buf, flags);
    } else {
        if (g_fcfs_preload_global_vars.__fxstatat == NULL) {
            g_fcfs_preload_global_vars.__fxstatat = fcfs_dlsym1("__fxstatat");
        }
        int result;
        result = g_fcfs_preload_global_vars.__fxstatat(ver, fd, path, buf, flags);
        fprintf(stderr, "result: %d\n", result);
        return result;
    }
}

int __fxstatat_(int ver, int fd, const char *path, struct stat *buf, int flags)
{
    return do_fxstatat(ver, fd, path, buf, flags);
}

int __fxstatat64(int ver, int fd, const char *path, struct stat *buf, int flags)
{
    return do_fxstatat(ver, fd, path, buf, flags);
}

ssize_t readlinkat(int fd, const char *path, char *buff, size_t size)
{
    if (FCFS_PRELOAD_IS_MY_FD_MOUNTPOINT(fd, path)) {
        return fcfs_readlinkat(fd, path, buff, size);
    } else {
        if (g_fcfs_preload_global_vars.readlinkat != NULL) {
            return g_fcfs_preload_global_vars.readlinkat(fd, path, buff, size);
        } else {
            return syscall(SYS_readlinkat, fd, path, buff, size);
        }
    }
}

static inline int do_mknodat(int fd, const char *path, mode_t mode, dev_t dev)
{
    if (FCFS_PRELOAD_IS_MY_FD_MOUNTPOINT(fd, path)) {
        return fcfs_mknodat(fd, path, mode, dev);
    } else {
        if (g_fcfs_preload_global_vars.mknodat != NULL) {
            return g_fcfs_preload_global_vars.mknodat(fd, path, mode, dev);
        } else {
            return syscall(SYS_mknodat, fd, path, mode, dev);
        }
    }
}

int mknodat(int fd, const char *path, mode_t mode, dev_t dev)
{
    return do_mknodat(fd, path, mode, dev);
}

int __xmknodat(int fd, const char *path, mode_t mode, dev_t dev)
{
    return do_mknodat(fd, path, mode, dev);
}

int mkfifoat(int fd, const char *path, mode_t mode)
{
    if (FCFS_PRELOAD_IS_MY_FD_MOUNTPOINT(fd, path)) {
        return fcfs_mkfifoat(fd, path, mode);
    } else {
        if (g_fcfs_preload_global_vars.mkfifoat == NULL) {
            g_fcfs_preload_global_vars.mkfifoat = fcfs_dlsym1("mkfifoat");
        }
        return g_fcfs_preload_global_vars.mkfifoat(fd, path, mode);
    }
}

int faccessat(int fd, const char *path, int mode, int flags)
{
    fprintf(stderr, "func: %s, path: %s, mode: %o\n", __FUNCTION__, path, mode);
    if (FCFS_PRELOAD_IS_MY_FD_MOUNTPOINT(fd, path)) {
        return fcfs_faccessat(fd, path, mode, flags);
    } else {
        if (g_fcfs_preload_global_vars.faccessat != NULL) {
            return g_fcfs_preload_global_vars.faccessat(fd, path, mode, flags);
        } else {
            return syscall(SYS_faccessat, fd, path, mode, flags);
        }
    }
}

int futimesat(int fd, const char *path, const struct timeval times[2])
{
    if (FCFS_PRELOAD_IS_MY_FD_MOUNTPOINT(fd, path)) {
        return fcfs_futimesat(fd, path, times);
    } else {
        if (g_fcfs_preload_global_vars.futimesat != NULL) {
            return g_fcfs_preload_global_vars.futimesat(fd, path, times);
        } else {
            return syscall(SYS_futimesat, fd, path, times);
        }
    }
}

int utimensat(int fd, const char *path, const struct timespec times[2], int flags)
{
    if (FCFS_PRELOAD_IS_MY_FD_MOUNTPOINT(fd, path)) {
        return fcfs_utimensat(fd, path, times, flags);
    } else {
        if (g_fcfs_preload_global_vars.utimensat != NULL) {
            return g_fcfs_preload_global_vars.utimensat(fd, path, times, flags);
        } else {
            return syscall(SYS_utimensat, fd, path, times, flags);
        }
    }
}

int unlinkat(int fd, const char *path, int flags)
{
    if (FCFS_PRELOAD_IS_MY_FD_MOUNTPOINT(fd, path)) {
        return fcfs_unlinkat(fd, path, flags);
    } else {
        if (g_fcfs_preload_global_vars.unlinkat != NULL) {
            return g_fcfs_preload_global_vars.unlinkat(fd, path, flags);
        } else {
            return syscall(SYS_unlinkat, fd, path, flags);
        }
    }
}

int mkdirat(int fd, const char *path, mode_t mode)
{
    if (FCFS_PRELOAD_IS_MY_FD_MOUNTPOINT(fd, path)) {
        return fcfs_mkdirat(fd, path, mode);
    } else {
        if (g_fcfs_preload_global_vars.futimesat != NULL) {
            return g_fcfs_preload_global_vars.mkdirat(fd, path, mode);
        } else {
            return syscall(SYS_futimesat, fd, path, mode);
        }
    }
}

int fchownat(int fd, const char *path, uid_t owner, gid_t group, int flags)
{
    if (FCFS_PRELOAD_IS_MY_FD_MOUNTPOINT(fd, path)) {
        return fcfs_fchownat(fd, path, owner, group, flags);
    } else {
        if (g_fcfs_preload_global_vars.fchownat != NULL) {
            return g_fcfs_preload_global_vars.fchownat(
                    fd, path, owner, group, flags);
        } else {
            return syscall(SYS_fchownat, fd, path, owner, group, flags);
        }
    }
}

int fchmodat(int fd, const char *path, mode_t mode, int flags)
{
    if (FCFS_PRELOAD_IS_MY_FD_MOUNTPOINT(fd, path)) {
        return fcfs_fchmodat(fd, path, mode, flags);
    } else {
        if (g_fcfs_preload_global_vars.fchmodat != NULL) {
            return g_fcfs_preload_global_vars.fchmodat(fd, path, mode, flags);
        } else {
            return syscall(SYS_fchmodat, fd, path, mode, flags);
        }
    }
}

int linkat(int fd1, const char *path1, int fd2, const char *path2, int flags)
{
    if (FCFS_PRELOAD_IS_MY_FD_MOUNTPOINT(fd1, path1) ||
            FCFS_PRELOAD_IS_MY_FD_MOUNTPOINT(fd2, path2))
    {
        return fcfs_linkat(fd1, path1, fd2, path2, flags);
    } else {
        if (g_fcfs_preload_global_vars.linkat != NULL) {
            return g_fcfs_preload_global_vars.linkat(
                    fd1, path1, fd2, path2, flags);
        } else {
            return syscall(SYS_linkat, fd1, path1, fd2, path2, flags);
        }
    }
}

int renameat(int fd1, const char *path1, int fd2, const char *path2)
{
    if (FCFS_PRELOAD_IS_MY_FD_MOUNTPOINT(fd1, path1) ||
            FCFS_PRELOAD_IS_MY_FD_MOUNTPOINT(fd2, path2))
    {
        return fcfs_renameat(fd1, path1, fd2, path2);
    } else {
        if (g_fcfs_preload_global_vars.renameat != NULL) {
            return g_fcfs_preload_global_vars.renameat(fd1, path1, fd2, path2);
        } else {
            return syscall(SYS_renameat, fd1, path1, fd2, path2);
        }
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
        if (g_fcfs_preload_global_vars.renameat2 != NULL) {
            return g_fcfs_preload_global_vars.renameat2(
                    fd1, path1, fd2, path2, flags);
        } else {
            return syscall(SYS_renameat2, fd1, path1, fd2, path2, flags);
        }
    }
}

static inline int do_scandirat(int fd, const char *path,
        struct dirent ***namelist, fcfs_dir_filter_func filter,
        fcfs_dir_compare_func compar)
{
    if (FCFS_PRELOAD_IS_MY_FD_MOUNTPOINT(fd, path)) {
        return fcfs_scandirat(fd, path, namelist, filter, compar);
    } else {
        if (g_fcfs_preload_global_vars.scandirat == NULL) {
            g_fcfs_preload_global_vars.scandirat = fcfs_dlsym1("scandirat");
        }
        return g_fcfs_preload_global_vars.scandirat(
                fd, path, namelist, filter, compar);
    }
}

int _scandirat_(int fd, const char *path, struct dirent ***namelist,
        fcfs_dir_filter_func filter, fcfs_dir_compare_func compar)
{
    return do_scandirat(fd, path, namelist, filter, compar);
}

int scandirat64(int fd, const char *path, struct dirent64 ***namelist,
        int (*filter) (const struct dirent64 *),
        int (*compar) (const struct dirent64 **,
            const struct dirent64 **))
{
    return do_scandirat(fd, path, (struct dirent ***)namelist,
            (fcfs_dir_filter_func)filter,
            (fcfs_dir_compare_func)compar);
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
        result = fcfs_closedir(wapper->dirp);
    } else {
        if (g_fcfs_preload_global_vars.closedir == NULL) {
            g_fcfs_preload_global_vars.closedir = fcfs_dlsym1("closedir");
        }
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
        return fcfs_readdir(wapper->dirp);
    } else {
        if (g_fcfs_preload_global_vars.readdir == NULL) {
            g_fcfs_preload_global_vars.readdir = fcfs_dlsym1("readdir");
        }
        return g_fcfs_preload_global_vars.readdir(wapper->dirp);
    }
}

struct dirent *_readdir_(DIR *dirp)
{
    fprintf(stderr, "func: %s, line: %d\n", __FUNCTION__, __LINE__);
    return do_readdir(dirp);
}

struct dirent64 *readdir64(DIR *dirp)
{
    fprintf(stderr, "func: %s, line: %d\n", __FUNCTION__, __LINE__);
    return (struct dirent64 *)do_readdir(dirp);
}

static inline int do_readdir_r(DIR *dirp, struct dirent *entry,
        struct dirent **result)
{
    FCFSPreloadDIRWrapper *wapper;

    wapper = (FCFSPreloadDIRWrapper *)dirp;

    if (wapper->call_type == fcfs_preload_call_fastcfs) {
        return fcfs_readdir_r(wapper->dirp, entry, result);
    } else {
        if (g_fcfs_preload_global_vars.readdir_r == NULL) {
            g_fcfs_preload_global_vars.readdir_r = fcfs_dlsym1("readdir_r");
        }
        return g_fcfs_preload_global_vars.readdir_r(
                wapper->dirp, entry, result);
    }
}

int _readdir_r_(DIR *dirp, struct dirent *entry, struct dirent **result)
{
    fprintf(stderr, "func: %s, line: %d\n", __FUNCTION__, __LINE__);
    return do_readdir_r(dirp, entry, result);
}

int readdir64_r(DIR *dirp, struct dirent64 *entry, struct dirent64 **result)
{
    fprintf(stderr, "func: %s, line: %d\n", __FUNCTION__, __LINE__);
    return do_readdir_r(dirp, (struct dirent *)entry, (struct dirent **)result);
}

void seekdir(DIR *dirp, long loc)
{
    FCFSPreloadDIRWrapper *wapper;

    wapper = (FCFSPreloadDIRWrapper *)dirp;
    if (wapper->call_type == fcfs_preload_call_fastcfs) {
        return fcfs_seekdir(wapper->dirp, loc);
    } else {
        if (g_fcfs_preload_global_vars.seekdir == NULL) {
            g_fcfs_preload_global_vars.seekdir = fcfs_dlsym1("seekdir");
        }
        return g_fcfs_preload_global_vars.seekdir(wapper->dirp, loc);
    }
}

long telldir(DIR *dirp)
{
    FCFSPreloadDIRWrapper *wapper;

    wapper = (FCFSPreloadDIRWrapper *)dirp;
    if (wapper->call_type == fcfs_preload_call_fastcfs) {
        return fcfs_telldir(wapper->dirp);
    } else {
        if (g_fcfs_preload_global_vars.telldir == NULL) {
            g_fcfs_preload_global_vars.telldir = fcfs_dlsym1("telldir");
        }
        return g_fcfs_preload_global_vars.telldir(wapper->dirp);
    }
}

void rewinddir(DIR *dirp)
{
    FCFSPreloadDIRWrapper *wapper;

    wapper = (FCFSPreloadDIRWrapper *)dirp;
    if (wapper->call_type == fcfs_preload_call_fastcfs) {
        return fcfs_rewinddir(wapper->dirp);
    } else {
        if (g_fcfs_preload_global_vars.rewinddir == NULL) {
            g_fcfs_preload_global_vars.rewinddir = fcfs_dlsym1("rewinddir");
        }
        return g_fcfs_preload_global_vars.rewinddir(wapper->dirp);
    }
}

int dirfd(DIR *dirp)
{
    FCFSPreloadDIRWrapper *wapper;

    wapper = (FCFSPreloadDIRWrapper *)dirp;
    if (wapper->call_type == fcfs_preload_call_fastcfs) {
        return fcfs_dirfd(wapper->dirp);
    } else {
        if (g_fcfs_preload_global_vars.dirfd == NULL) {
            g_fcfs_preload_global_vars.dirfd = fcfs_dlsym1("dirfd");
        }
        return g_fcfs_preload_global_vars.dirfd(wapper->dirp);
    }
}

int unsetenv(const char *name)
{
    fprintf(stderr, "pid: %d, func: %s, line: %d, name: %s\n",
            getpid(), __FUNCTION__, __LINE__, name);

    if (g_fcfs_preload_global_vars.unsetenv == NULL) {
        g_fcfs_preload_global_vars.unsetenv = fcfs_dlsym1("unsetenv");
    }
    return g_fcfs_preload_global_vars.unsetenv(name);
}

int clearenv(void)
{
    fprintf(stderr, "pid: %d, func: %s, line: %d\n",
            getpid(), __FUNCTION__, __LINE__);

    if (g_fcfs_preload_global_vars.clearenv == NULL) {
        g_fcfs_preload_global_vars.clearenv = fcfs_dlsym1("clearenv");
    }
    return g_fcfs_preload_global_vars.clearenv();
}

static inline FILE *do_fopen(const char *pathname, const char *mode)
{
    if (g_fcfs_preload_global_vars.fopen == NULL) {
        g_fcfs_preload_global_vars.fopen = fcfs_dlsym1("fopen");
    }
    return g_fcfs_preload_global_vars.fopen(pathname, mode);
}

FILE *_fopen_(const char *pathname, const char *mode)
{
    fprintf(stderr, "pid: %d, func: %s, line: %d, pathname: %s\n",
            getpid(), __FUNCTION__, __LINE__, pathname);
    return do_fopen(pathname, mode);
}

FILE *fopen64(const char *pathname, const char *mode)
{
    fprintf(stderr, "pid: %d, func: %s, line: %d, pathname: %s\n",
            getpid(), __FUNCTION__, __LINE__, pathname);
    return do_fopen(pathname, mode);
}

size_t fread(void *ptr, size_t size, size_t nmemb, FILE *fp)
{
    fprintf(stderr, "pid: %d, func: %s, line: %d, size: %d, nmemb: %d, fp: %p\n",
            getpid(), __FUNCTION__, __LINE__, (int)size, (int)nmemb, fp);

    if (g_fcfs_preload_global_vars.fread == NULL) {
        g_fcfs_preload_global_vars.fread = fcfs_dlsym1("fread");
    }
    return g_fcfs_preload_global_vars.fread(ptr, size, nmemb, fp);
}

size_t fread_unlocked(void *ptr, size_t size, size_t nmemb, FILE *fp)
{
    fprintf(stderr, "pid: %d, func: %s, line: %d, size: %d, nmemb: %d, fp: %p\n",
            getpid(), __FUNCTION__, __LINE__, (int)size, (int)nmemb, fp);

    if (g_fcfs_preload_global_vars.fread == NULL) {
        g_fcfs_preload_global_vars.fread = fcfs_dlsym1("fread");
    }
    return g_fcfs_preload_global_vars.fread(ptr, size, nmemb, fp);
}

size_t fwrite(const void *ptr, size_t size, size_t nmemb, FILE *fp)
{
    fprintf(stderr, "pid: %d, func: %s, line: %d, size: %d, nmemb: %d, fp: %p\n",
            getpid(), __FUNCTION__, __LINE__, (int)size, (int)nmemb, fp);

    if (g_fcfs_preload_global_vars.fwrite == NULL) {
        g_fcfs_preload_global_vars.fwrite = fcfs_dlsym1("fwrite");
    }
    return g_fcfs_preload_global_vars.fwrite(ptr, size, nmemb, fp);
}
