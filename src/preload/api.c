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

static int counter = 0;
#define FCFS_LOG_DEBUG(format, ...)  \
    if (g_fcfs_preload_global_vars.inited && g_log_context. \
            log_level >= LOG_DEBUG) logDebug(format, ##__VA_ARGS__)

//            log_level >= LOG_DEBUG) fprintf(stderr, format, ##__VA_ARGS__)


__attribute__ ((constructor)) static void preload_global_init(void)
{
    int result;
    pid_t pid;
    char *ns = "fs";
    char *config_filename;

    config_filename = getenv("FCFS_PRELOAD_CONFIG_FILENAME");
    if (config_filename == NULL) {
        config_filename = FCFS_FUSE_DEFAULT_CONFIG_FILENAME;
    }

    log_init();
    pid = getpid();
    fprintf(stderr, "pid: %d, file: "__FILE__", line: %d, inited: %d, "
            "constructor, config_filename: %s\n", pid, __LINE__,
            g_fcfs_preload_global_vars.inited, config_filename);

    if ((result=fcfs_preload_global_init()) != 0) {
        return;
    }
    
#ifdef FCFS_PRELOAD_WITH_CAPI
    if ((result=fcfs_capi_init()) != 0) {
        return;
    }
#endif

    FCFS_LOG_DEBUG("file: "__FILE__", line: %d, pid: %d, "
            "constructor\n", __LINE__, pid);

    log_set_fd_flags(&g_log_context, O_CLOEXEC);
    if ((result=fcfs_posix_api_init("fcfs_preload",
                    ns, config_filename)) != 0)
    {
        return;
    }

    FCFS_LOG_DEBUG("pid: %d, file: "__FILE__", line: %d, "
            "constructor\n", pid, __LINE__);
    if ((result=fcfs_posix_api_start()) != 0) {
        return;
    }

    g_fcfs_preload_global_vars.cwd_call_type =
        FCFS_PRELOAD_CALL_SYSTEM;
    g_fcfs_preload_global_vars.inited = true;

    FCFS_LOG_DEBUG("pid: %d, file: "__FILE__", line: %d, inited: %d, "
            "log_fd: %d\n", pid, __LINE__, g_fcfs_preload_global_vars.inited,
            g_log_context.log_fd);

    fprintf(stderr, "pid: %d, parent pid: %d, base path: %s, "
            "log_level: %d, log_fd: %d\n", getpid(), getppid(),
            SF_G_BASE_PATH_STR, g_log_context.log_level,
            g_log_context.log_fd);
}

__attribute__ ((destructor)) static void preload_global_destroy(void)
{
    FCFS_LOG_DEBUG("pid: %d, file: "__FILE__", line: %d, "
            "destructor\n", getpid(), __LINE__);
    if (g_fcfs_preload_global_vars.inited) {
        fcfs_posix_api_stop();
    }
}

static inline void *fcfs_dlsym1(const char *fname)
{
    void *func;

    if ((func=dlsym(RTLD_NEXT, fname)) == NULL) {
        FCFS_LOG_DEBUG("file: "__FILE__", line: %d, "
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
        FCFS_LOG_DEBUG("file: "__FILE__", line: %d, "
                "function %s | %s not exist!", __LINE__,
                fname1, fname2);
    }
    return func;
}

/*
long syscall(long number, ...)
{
    static long (*syscall_func)(long number, ...) = NULL;
    va_list ap;
    long arg1;
    long arg2;
    long arg3;
    long arg4;
    long arg5;
    long arg6;

    va_start(ap, number);
    arg1 = va_arg(ap, long);
    arg2 = va_arg(ap, long);
    arg3 = va_arg(ap, long);
    arg4 = va_arg(ap, long);
    arg5 = va_arg(ap, long);
    arg6 = va_arg(ap, long);
    va_end(ap);

    FCFS_LOG_DEBUG("func: %s, line: %d, number: %ld, "
            "arg1: %ld, arg2: %ld, arg3: %ld, arg4: %ld, arg5: %ld, arg6: %ld\n",
            __FUNCTION__, __LINE__, number, arg1, arg2, arg3, arg4, arg5, arg6);

    if (syscall_func == NULL) {
       syscall_func = fcfs_dlsym1("syscall");
    }

    return syscall_func(number, arg1, arg2, arg3, arg4, arg5, arg6);
}
*/

static inline int do_open(const char *path, int flags, int mode)
{
    int fd;

    if (FCFS_PRELOAD_IS_MY_MOUNTPOINT(path)) {
        fd = fcfs_open(path, flags, mode);
    } else {
        fd = syscall(SYS_open, path, flags, mode);
    }

    FCFS_LOG_DEBUG("%d. @func: %s, path: %s, mode: %o, fd: %d\n",
            ++counter, __FUNCTION__, path, mode, fd);
    return fd;
}

int _open_(const char *path, int flags, ...)
{
    va_list ap;
    int mode;

    va_start(ap, flags);
    mode = va_arg(ap, int);
    va_end(ap);

    FCFS_LOG_DEBUG("#func: %s, path: %s, mode: %o\n", __FUNCTION__, path, mode);
    return do_open(path, flags, mode);
}

int __open_nocancel(const char *path, int flags, ...)
{
    va_list ap;
    int mode;

    va_start(ap, flags);
    mode = va_arg(ap, int);
    va_end(ap);

    FCFS_LOG_DEBUG("#func: %s, path: %s, mode: %o\n", __FUNCTION__, path, mode);
    return do_open(path, flags, mode);
}

int open64(const char *path, int flags, ...)
{
    va_list ap;
    int mode;

    va_start(ap, flags);
    mode = va_arg(ap, int);
    va_end(ap);

    FCFS_LOG_DEBUG("#func: %s, path: %s, mode: %o\n", __FUNCTION__, path, mode);
    return do_open(path, flags, mode);
}

int __open(const char *path, int flags, int mode)
{
    FCFS_LOG_DEBUG("#func: %s, path: %s, mode: %o\n", __FUNCTION__, path, mode);
    return do_open(path, flags, mode);
}

int __open_2_(const char *path, int flags)
{
    FCFS_LOG_DEBUG("#func: %s, path: %s\n", __FUNCTION__, path);
    return do_open(path, flags, 0666);
}

int __open64_2_(const char *path, int flags)
{
    FCFS_LOG_DEBUG("#func: %s, path: %s\n", __FUNCTION__, path);
    return do_open(path, flags, 0666);
}

static inline int do_creat(const char *path, mode_t mode)
{
    if (FCFS_PRELOAD_IS_MY_MOUNTPOINT(path)) {
        return fcfs_creat(path, mode);
    } else {
        return syscall(SYS_creat, path, mode);
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
        return syscall(SYS_truncate, path, length);
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
    FCFS_LOG_DEBUG("%d. func: %s, line: %d\n", ++counter, __FUNCTION__, __LINE__);

    if (FCFS_PRELOAD_IS_MY_MOUNTPOINT(path)) {
        return fcfs_lstat(path, buf);
    } else {
        return syscall(SYS_lstat, path, buf);
    }
}

static inline int do_lxstat(int ver, const char *path, struct stat *buf)
{
    FCFS_LOG_DEBUG("%d. func: %s, ver: %d, path: %s\n", ++counter, __FUNCTION__, ver, path);

    if (FCFS_PRELOAD_IS_MY_MOUNTPOINT(path)) {
        return fcfs_lstat(path, buf);
    } else {
        if (g_fcfs_preload_global_vars.__lxstat == NULL) {
            g_fcfs_preload_global_vars.__lxstat = fcfs_dlsym1("__lxstat");
        }
        return g_fcfs_preload_global_vars.__lxstat(ver, path, buf);
    }
}

int __lxstat_(int ver, const char *path, struct stat *buf)
{
    return do_lxstat(ver, path, buf);
}

int __lxstat64(int ver, const char *path, struct stat64 *buf)
{
    return do_lxstat(ver, path, (struct stat *)buf);
}

int stat(const char *path, struct stat *buf)
{
    FCFS_LOG_DEBUG("%d. func: %s, line: %d, path: %s\n", ++counter, __FUNCTION__, __LINE__, path);
    if (FCFS_PRELOAD_IS_MY_MOUNTPOINT(path)) {
        return fcfs_stat(path, buf);
    } else {
        return syscall(SYS_stat, path, buf);
    }
}

static inline int do_xstat(int ver, const char *path, struct stat *buf)
{
    FCFS_LOG_DEBUG("%d. func: %s, ver: %d, path: %s\n", ++counter, __FUNCTION__, ver, path);
    if (FCFS_PRELOAD_IS_MY_MOUNTPOINT(path)) {
        return fcfs_stat(path, buf);
    } else {
        if (g_fcfs_preload_global_vars.__xstat == NULL) {
            g_fcfs_preload_global_vars.__xstat = fcfs_dlsym1("__xstat");
        }
        return g_fcfs_preload_global_vars.__xstat(ver, path, buf);
    }
}

int __xstat_(int ver, const char *path, struct stat *buf)
{
    return do_xstat(ver, path, buf);
}

int __xstat64(int ver, const char *path, struct stat64 *buf)
{
    return do_xstat(ver, path, (struct stat *)buf);
}

int link(const char *path1, const char *path2)
{
    if (FCFS_PRELOAD_IS_MY_MOUNTPOINT(path1) ||
            FCFS_PRELOAD_IS_MY_MOUNTPOINT(path2))
    {
        return fcfs_link(path1, path2);
    } else {
        return syscall(SYS_link, path1, path2);
    }
}

int symlink(const char *link, const char *path)
{
    if (FCFS_PRELOAD_IS_MY_MOUNTPOINT(path)) {
        return fcfs_symlink(link, path);
    } else {
        return syscall(SYS_symlink, link, path);
    }
}

ssize_t readlink(const char *path, char *buff, size_t size)
{
    if (FCFS_PRELOAD_IS_MY_MOUNTPOINT(path)) {
        return fcfs_readlink(path, buff, size);
    } else {
        return syscall(SYS_readlink, path, buff, size);
    }
}

int mknod(const char *path, mode_t mode, dev_t dev)
{
    if (FCFS_PRELOAD_IS_MY_MOUNTPOINT(path)) {
        return fcfs_mknod(path, mode, dev);
    } else {
        return syscall(SYS_mknod, path, mode, dev);
    }
}

int __xmknod(int ver, const char *path, mode_t mode, dev_t *dev)
{
    if (FCFS_PRELOAD_IS_MY_MOUNTPOINT(path)) {
        return fcfs_mknod(path, mode, *dev);
    } else {
        if (g_fcfs_preload_global_vars.__xmknod == NULL) {
            g_fcfs_preload_global_vars.__xmknod = fcfs_dlsym1("__xmknod");
        }
        return g_fcfs_preload_global_vars.__xmknod(ver, path, mode, dev);
    }
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
    FCFS_LOG_DEBUG("%d. func: %s, path: %s, mode: %o\n",
            ++counter, __FUNCTION__, path, mode);
    if (FCFS_PRELOAD_IS_MY_MOUNTPOINT(path)) {
        return fcfs_access(path, mode);
    } else {
        return syscall(SYS_access, path, mode);
    }
}

int eaccess(const char *path, int mode)
{
    FCFS_LOG_DEBUG("func: %s, path: %s, mode: %o\n", __FUNCTION__, path, mode);
    if (FCFS_PRELOAD_IS_MY_MOUNTPOINT(path)) {
        return fcfs_eaccess(path, mode);
    } else {
        if (g_fcfs_preload_global_vars.eaccess == NULL) {
            g_fcfs_preload_global_vars.eaccess = fcfs_dlsym1("eaccess");
        }
        return g_fcfs_preload_global_vars.eaccess(path, mode);
    }
}

int euidaccess(const char *path, int mode)
{
    FCFS_LOG_DEBUG("func: %s, path: %s, mode: %o\n", __FUNCTION__, path, mode);
    if (FCFS_PRELOAD_IS_MY_MOUNTPOINT(path)) {
        return fcfs_euidaccess(path, mode);
    } else {
        if (g_fcfs_preload_global_vars.euidaccess == NULL) {
            g_fcfs_preload_global_vars.euidaccess = fcfs_dlsym1("euidaccess");
        }
        return g_fcfs_preload_global_vars.euidaccess(path, mode);
    }
}

int utime(const char *path, const struct utimbuf *times)
{
    if (FCFS_PRELOAD_IS_MY_MOUNTPOINT(path)) {
        return fcfs_utime(path, times);
    } else {
        return syscall(SYS_utime, path, times);
    }
}

int utimes(const char *path, const struct timeval times[2])
{
    if (FCFS_PRELOAD_IS_MY_MOUNTPOINT(path)) {
        return fcfs_utimes(path, times);
    } else {
        return syscall(SYS_utimes, path, times);
    }
}

int unlink(const char *path)
{
    if (FCFS_PRELOAD_IS_MY_MOUNTPOINT(path)) {
        return fcfs_unlink(path);
    } else {
        return syscall(SYS_unlink, path);
    }
}

int rename(const char *path1, const char *path2)
{
    if (FCFS_PRELOAD_IS_MY_MOUNTPOINT(path1) ||
            FCFS_PRELOAD_IS_MY_MOUNTPOINT(path2))
    {
        return fcfs_rename(path1, path2);
    } else {
        return syscall(SYS_rename, path1, path2);
    }
}

int mkdir(const char *path, mode_t mode)
{
    if (FCFS_PRELOAD_IS_MY_MOUNTPOINT(path)) {
        return fcfs_mkdir(path, mode);
    } else {
        return syscall(SYS_mkdir, path, mode);
    }
}

int rmdir(const char *path)
{
    if (FCFS_PRELOAD_IS_MY_MOUNTPOINT(path)) {
        return fcfs_rmdir(path);
    } else {
        return syscall(SYS_rmdir, path);
    }
}

int chown(const char *path, uid_t owner, gid_t group)
{
    if (FCFS_PRELOAD_IS_MY_MOUNTPOINT(path)) {
        return fcfs_chown(path, owner, group);
    } else {
        return syscall(SYS_chown, path, owner, group);
    }
}

int lchown(const char *path, uid_t owner, gid_t group)
{
    if (FCFS_PRELOAD_IS_MY_MOUNTPOINT(path)) {
        return fcfs_lchown(path, owner, group);
    } else {
        return syscall(SYS_lchown, path, owner, group);
    }
}

int chmod(const char *path, mode_t mode)
{
    if (FCFS_PRELOAD_IS_MY_MOUNTPOINT(path)) {
        return fcfs_chmod(path, mode);
    } else {
        return syscall(SYS_chmod, path, mode);
    }
}

static inline int do_statvfs(const char *path, struct statvfs *buf)
{
    if (FCFS_PRELOAD_IS_MY_MOUNTPOINT(path)) {
        return fcfs_statvfs(path, buf);
    } else {
        if (g_fcfs_preload_global_vars.statvfs == NULL) {
            g_fcfs_preload_global_vars.statvfs = fcfs_dlsym2(
                    "statvfs", "statvfs64");
        }
        return g_fcfs_preload_global_vars.statvfs(path, buf);
    }
}

int _statvfs_(const char *path, struct statvfs *buf)
{
    return do_statvfs(path, buf);
}

int statvfs64(const char *path, struct statvfs64 *buf)
{
    return do_statvfs(path, (struct statvfs *)buf);
}

int setxattr(const char *path, const char *name,
        const void *value, size_t size, int flags)
{
    if (FCFS_PRELOAD_IS_MY_MOUNTPOINT(path)) {
        return fcfs_setxattr(path, name, value, size, flags);
    } else {
        return syscall(SYS_setxattr, path, name, value, size, flags);
    }
}

int lsetxattr(const char *path, const char *name,
        const void *value, size_t size, int flags)
{
    if (FCFS_PRELOAD_IS_MY_MOUNTPOINT(path)) {
        return fcfs_lsetxattr(path, name, value, size, flags);
    } else {
        return syscall(SYS_lsetxattr, path, name, value, size, flags);
    }
}

ssize_t getxattr(const char *path, const char *name, void *value, size_t size)
{
    if (FCFS_PRELOAD_IS_MY_MOUNTPOINT(path)) {
        return fcfs_getxattr(path, name, value, size);
    } else {
        return syscall(SYS_getxattr, path, name, value, size);
    }
}

ssize_t lgetxattr(const char *path, const char *name, void *value, size_t size)
{
    if (FCFS_PRELOAD_IS_MY_MOUNTPOINT(path)) {
        return fcfs_lgetxattr(path, name, value, size);
    } else {
        return syscall(SYS_lgetxattr, path, name, value, size);
    }
}

ssize_t listxattr(const char *path, char *list, size_t size)
{
    if (FCFS_PRELOAD_IS_MY_MOUNTPOINT(path)) {
        return fcfs_listxattr(path, list, size);
    } else {
        return syscall(SYS_listxattr, path, list, size);
    }
}

ssize_t llistxattr(const char *path, char *list, size_t size)
{
    if (FCFS_PRELOAD_IS_MY_MOUNTPOINT(path)) {
        return fcfs_llistxattr(path, list, size);
    } else {
        return syscall(SYS_llistxattr, path, list, size);
    }
}

int removexattr(const char *path, const char *name)
{
    if (FCFS_PRELOAD_IS_MY_MOUNTPOINT(path)) {
        return fcfs_removexattr(path, name);
    } else {
        return syscall(SYS_removexattr, path, name);
    }
}

int lremovexattr(const char *path, const char *name)
{
    if (FCFS_PRELOAD_IS_MY_MOUNTPOINT(path)) {
        return fcfs_lremovexattr(path, name);
    } else {
        return syscall(SYS_lremovexattr, path, name);
    }
}

int chdir(const char *path)
{
    int result;

    FCFS_LOG_DEBUG("%d. func: %s, line: %d, path: %s\n",
            ++counter, __FUNCTION__, __LINE__, path);

    if (FCFS_PRELOAD_IS_MY_MOUNTPOINT(path)) {
        if ((result=fcfs_chdir(path)) == 0) {
            g_fcfs_preload_global_vars.cwd_call_type =
                FCFS_PRELOAD_CALL_FASTCFS;
        }
    } else {
        if ((result=syscall(SYS_chdir, path)) == 0) {
            g_fcfs_preload_global_vars.cwd_call_type =
                FCFS_PRELOAD_CALL_SYSTEM;
        }
    }

    return result;
}

int chroot(const char *path)
{
    if (FCFS_PRELOAD_IS_MY_MOUNTPOINT(path)) {
        return fcfs_chroot(path);
    } else {
        return syscall(SYS_chroot, path);
    }
}

DIR *opendir(const char *path)
{
    FCFSPreloadDIRWrapper *wapper;
    DIR *dirp;
    int call_type;

    FCFS_LOG_DEBUG("%d. func: %s, path: %s\n", ++counter, __FUNCTION__, path);

    if (FCFS_PRELOAD_IS_MY_MOUNTPOINT(path)) {
        call_type = FCFS_PRELOAD_CALL_FASTCFS;
        dirp = (DIR *)fcfs_opendir(path);
    } else {
        call_type = FCFS_PRELOAD_CALL_SYSTEM;
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
    FCFS_LOG_DEBUG("%d. pid: %d, func: %s, line: %d, fd: %d\n", ++counter,
            getpid(), __FUNCTION__, __LINE__, fd);

    if (FCFS_PAPI_IS_MY_FD(fd)) {
        return fcfs_close(fd);
    } else {
        return syscall(SYS_close, fd);
    }
}

int fsync(int fd)
{
    if (FCFS_PAPI_IS_MY_FD(fd)) {
        return fcfs_fsync(fd);
    } else {
        return syscall(SYS_fsync, fd);
    }
}

int fdatasync(int fd)
{
    if (FCFS_PAPI_IS_MY_FD(fd)) {
        return fcfs_fdatasync(fd);
    } else {
        return syscall(SYS_fdatasync, fd);
    }
}

ssize_t write(int fd, const void *buff, size_t count)
{
    if (FCFS_PAPI_IS_MY_FD(fd)) {
        return fcfs_write(fd, buff, count);
    } else {
        return syscall(SYS_write, fd, buff, count);
    }
}

static inline ssize_t do_pwrite(int fd, const void *buff,
        size_t count, off_t offset)
{
    if (FCFS_PAPI_IS_MY_FD(fd)) {
        return fcfs_pwrite(fd, buff, count, offset);
    } else {
        return syscall(SYS_pwrite64, fd, buff, count, offset);
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
        return syscall(SYS_writev, fd, iov, iovcnt);
    }
}

static inline ssize_t do_pwritev(int fd, const struct iovec *iov,
        int iovcnt, off_t offset)
{
    if (FCFS_PAPI_IS_MY_FD(fd)) {
        return fcfs_pwritev(fd, iov, iovcnt, offset);
    } else {
        return syscall(SYS_writev, fd, iov, iovcnt, offset);
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
    FCFS_LOG_DEBUG("%d. line: %d, func: %s, fd: %d, count: %d\n",
            ++counter, __LINE__, __FUNCTION__, fd, (int)count);
    if (FCFS_PAPI_IS_MY_FD(fd)) {
        return fcfs_read(fd, buff, count);
    } else {
        return syscall(SYS_read, fd, buff, count);
    }
}

ssize_t __read_chk(int fd, void *buff, size_t count, size_t size)
{
    /*
    fprintf(stderr, "==== line: %d, func: %s, fd: %d, count: %d, size: %d ====\n",
            __LINE__, __FUNCTION__, fd, (int)count, (int)size);
            */
    return read(fd, buff, size > 0 ? FC_MIN(count, size) : count);
}

ssize_t readahead(int fd, off64_t offset, size_t count)
{
    FCFS_LOG_DEBUG("func: %s, fd: %d\n", __FUNCTION__, fd);
    if (FCFS_PAPI_IS_MY_FD(fd)) {
        return fcfs_readahead(fd, offset, count);
    } else {
        return syscall(SYS_readahead, fd, offset, count);
    }
}

static inline ssize_t do_pread(int fd, void *buff, size_t count, off_t offset)
{
    FCFS_LOG_DEBUG("%d. func: %s, fd: %d\n", ++counter, __FUNCTION__, fd);
    if (FCFS_PAPI_IS_MY_FD(fd)) {
        return fcfs_pread(fd, buff, count, offset);
    } else {
        return syscall(SYS_pread64, fd, buff, count, offset);
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

ssize_t __pread_chk(int fd, void *buff, size_t count,
        off_t offset, size_t size)
{
    /*
    fprintf(stderr, "==== line: %d, func: %s, fd: %d, count: %d, size: %d ====\n",
            __LINE__, __FUNCTION__, fd, (int)count, (int)size);
            */
    return do_pread(fd, buff, size > 0 ? FC_MIN(count, size) : count, offset);
}

ssize_t __pread64_chk(int fd, void *buff, size_t count,
        off_t offset, size_t size)
{
    /*
    fprintf(stderr, "==== line: %d, func: %s, fd: %d, count: %d, size: %d ====\n",
            __LINE__, __FUNCTION__, fd, (int)count, (int)size);
            */
    return do_pread(fd, buff, size > 0 ? FC_MIN(count, size) : count, offset);
}

ssize_t readv(int fd, const struct iovec *iov, int iovcnt)
{
    FCFS_LOG_DEBUG("%d. func: %s, fd: %d\n", ++counter, __FUNCTION__, fd);
    if (FCFS_PAPI_IS_MY_FD(fd)) {
        return fcfs_readv(fd, iov, iovcnt);
    } else {
        return syscall(SYS_readv, fd, iov, iovcnt);
    }
}

static inline ssize_t do_preadv(int fd, const struct iovec *iov,
        int iovcnt, off_t offset)
{
    if (FCFS_PAPI_IS_MY_FD(fd)) {
        return fcfs_preadv(fd, iov, iovcnt, offset);
    } else {
        return syscall(SYS_preadv, fd, iov, iovcnt, offset);
    }
}

ssize_t _preadv_(int fd, const struct iovec *iov, int iovcnt, off_t offset)
{
    FCFS_LOG_DEBUG("%d. func: %s, fd: %d\n", ++counter, __FUNCTION__, fd);
    return do_preadv(fd, iov, iovcnt, offset);
}

ssize_t preadv64(int fd, const struct iovec *iov, int iovcnt, off_t offset)
{
    FCFS_LOG_DEBUG("%d. func: %s, fd: %d\n", ++counter, __FUNCTION__, fd);
    return do_preadv(fd, iov, iovcnt, offset);
}

static inline off_t do_lseek(int fd, off_t offset, int whence)
{
    if (FCFS_PAPI_IS_MY_FD(fd)) {
        return fcfs_lseek(fd, offset, whence);
    } else {
        return syscall(SYS_lseek, fd, offset, whence);
    }
}

off_t _lseek_(int fd, off_t offset, int whence)
{
    FCFS_LOG_DEBUG("%d. func: %s, fd: %d\n", ++counter, __FUNCTION__, fd);
    return do_lseek(fd, offset, whence);
}

off_t __lseek(int fd, off_t offset, int whence)
{
    FCFS_LOG_DEBUG("%d. func: %s, fd: %d\n", ++counter, __FUNCTION__, fd);
    return do_lseek(fd, offset, whence);
}

off_t lseek64(int fd, off_t offset, int whence)
{
    FCFS_LOG_DEBUG("%d. func: %s, fd: %d\n", ++counter, __FUNCTION__, fd);
    return do_lseek(fd, offset, whence);
}

static inline int do_fallocate(int fd, int mode, off_t offset, off_t length)
{
    if (FCFS_PAPI_IS_MY_FD(fd)) {
        return fcfs_fallocate(fd, mode, offset, length);
    } else {
        return syscall(SYS_fallocate, fd, mode, offset, length);
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
        return syscall(SYS_ftruncate, fd, length);
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
    FCFS_LOG_DEBUG("%d. func: %s, fd: %d\n", ++counter, __FUNCTION__, fd);
    if (FCFS_PAPI_IS_MY_FD(fd)) {
        return fcfs_fstat(fd, buf);
    } else {
        return syscall(SYS_fstat, fd, buf);
    }
}

static inline int do_fxstat(int ver, int fd, struct stat *buf)
{
    FCFS_LOG_DEBUG("%d. func: %s, ver: %d, fd: %d\n", ++counter, __FUNCTION__, ver, fd);
    if (FCFS_PAPI_IS_MY_FD(fd)) {
        return fcfs_fstat(fd, buf);
    } else {
        if (g_fcfs_preload_global_vars.__fxstat == NULL) {
            g_fcfs_preload_global_vars.__fxstat = fcfs_dlsym1("__fxstat");
        }
        return g_fcfs_preload_global_vars.__fxstat(ver, fd, buf);
    }
}

int __fxstat_(int ver, int fd, struct stat *buf)
{
    FCFS_LOG_DEBUG("func: %s, ver: %d, fd: %d\n", __FUNCTION__, ver, fd);
    return do_fxstat(ver, fd, buf);
}

int __fxstat64(int ver, int fd, struct stat64 *buf)
{
    FCFS_LOG_DEBUG("func: %s, ver: %d, fd: %d\n", __FUNCTION__, ver, fd);
    return do_fxstat(ver, fd, (struct stat *)buf);
}

int flock(int fd, int operation)
{
    FCFS_LOG_DEBUG("func: %s, fd: %d, operation: %d\n", __FUNCTION__, fd, operation);
    if (FCFS_PAPI_IS_MY_FD(fd)) {
        return fcfs_flock(fd, operation);
    } else {
        return syscall(SYS_flock, fd, operation);
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
                return syscall(SYS_fcntl, fd, cmd, lock);
            }
        default:
            flags = (long)arg;

            FCFS_LOG_DEBUG("func: %s, fd: %d, cmd: %d, flags: %d\n",
                    __FUNCTION__, fd, cmd, flags);

            if (FCFS_PAPI_IS_MY_FD(fd)) {
                return fcfs_fcntl(fd, cmd, flags); 
            } else {
                return syscall(SYS_fcntl, fd, cmd, flags);
            }
    }
}

int _fcntl_(int fd, int cmd, ...)
{
    va_list ap;
    void *arg;

    FCFS_LOG_DEBUG("%d. func: %s, fd: %d, cmd: %d\n", ++counter, __FUNCTION__, fd, cmd);
    va_start(ap, cmd);
    arg = va_arg(ap, void *);
    va_end(ap);

    return do_fcntl(fd, cmd, arg);
}

int fcntl64(int fd, int cmd, ...)
{
    va_list ap;
    void *arg;

    FCFS_LOG_DEBUG("%d. func: %s, fd: %d, cmd: %d\n", ++counter, __FUNCTION__, fd, cmd);
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
        return syscall(SYS_fchown, fd, owner, group);
    }
}

int fchmod(int fd, mode_t mode)
{
    if (FCFS_PAPI_IS_MY_FD(fd)) {
        return fcfs_fchmod(fd, mode);
    } else {
        return syscall(SYS_fchmod, fd, mode);
    }
}

int fsetxattr(int fd, const char *name, const
        void *value, size_t size, int flags)
{
    if (FCFS_PAPI_IS_MY_FD(fd)) {
        return fcfs_fsetxattr(fd, name, value, size, flags);
    } else {
        return syscall(SYS_fsetxattr, fd, name, value, size, flags);
    }
}

ssize_t fgetxattr(int fd, const char *name, void *value, size_t size)
{
    if (FCFS_PAPI_IS_MY_FD(fd)) {
        return fcfs_fgetxattr(fd, name, value, size);
    } else {
        return syscall(SYS_fgetxattr, fd, name, value, size);
    }
}

ssize_t flistxattr(int fd, char *list, size_t size)
{
    if (FCFS_PAPI_IS_MY_FD(fd)) {
        return fcfs_flistxattr(fd, list, size);
    } else {
        return syscall(SYS_flistxattr, fd, list, size);
    }
}

int fremovexattr(int fd, const char *name)
{
    if (FCFS_PAPI_IS_MY_FD(fd)) {
        return fcfs_fremovexattr(fd, name);
    } else {
        return syscall(SYS_fremovexattr, fd, name);
    }
}

int fchdir(int fd)
{
    int result;

    if (FCFS_PAPI_IS_MY_FD(fd)) {
        if ((result=fcfs_fchdir(fd)) == 0) {
            g_fcfs_preload_global_vars.cwd_call_type =
                FCFS_PRELOAD_CALL_FASTCFS;
        }
    } else {
        if ((result=syscall(SYS_fchdir, fd)) == 0) {
            g_fcfs_preload_global_vars.cwd_call_type =
                FCFS_PRELOAD_CALL_SYSTEM;
        }
    }

    return result;
}

static inline int do_fstatvfs(int fd, struct statvfs *buf)
{
    if (FCFS_PAPI_IS_MY_FD(fd)) {
        return fcfs_fstatvfs(fd, buf);
    } else {
        if (g_fcfs_preload_global_vars.fstatvfs == NULL) {
            g_fcfs_preload_global_vars.fstatvfs = fcfs_dlsym2(
                    "fstatvfs", "fstatvfs64");
        }
        return g_fcfs_preload_global_vars.fstatvfs(fd, buf);
    }
}

int _fstatvfs_(int fd, struct statvfs *buf)
{
    return do_fstatvfs(fd, buf);
}

int fstatvfs64(int fd, struct statvfs64 *buf)
{
    return do_fstatvfs(fd, (struct statvfs *)buf);
}

int dup(int fd)
{
    FCFS_LOG_DEBUG("#func: %s, fd: %d\n", __FUNCTION__, fd);
    if (FCFS_PAPI_IS_MY_FD(fd)) {
        return fcfs_dup(fd);
    } else {
        return syscall(SYS_dup, fd);
    }
}

int dup2(int fd1, int fd2)
{
    FCFS_LOG_DEBUG("#func: %s, fd1: %d, fd2: %d\n", __FUNCTION__, fd1, fd2);
    if (FCFS_PAPI_IS_MY_FD(fd1) && FCFS_PAPI_IS_MY_FD(fd2)) {
        return fcfs_dup2(fd1, fd2);
    } else {
        return syscall(SYS_dup2, fd1, fd2);
    }
}

static inline void *do_mmap(void *addr, size_t length, int prot,
        int flags, int fd, off_t offset)
{
    FCFS_LOG_DEBUG("func: %s, fd: %d, offset: %"PRId64"\n",
            __FUNCTION__, fd, (int64_t)offset);
    if (FCFS_PAPI_IS_MY_FD(fd)) {
        return fcfs_mmap(addr, length, prot, flags, fd, offset);
    } else {
        return (void *)syscall(SYS_mmap, addr, length,
                prot, flags, fd, offset);
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
    int call_type;

    FCFS_LOG_DEBUG("%d. func: %s, fd: %d\n", ++counter, __FUNCTION__, fd);
    if (FCFS_PAPI_IS_MY_FD(fd)) {
        call_type = FCFS_PRELOAD_CALL_FASTCFS;
        dirp = (DIR *)fcfs_fdopendir(fd);
    } else {
        call_type = FCFS_PRELOAD_CALL_SYSTEM;
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
        return syscall(SYS_symlinkat, link, fd, path);
    }
}

static inline int do_openat(int fd, const char *path, int flags, int mode)
{
    FCFS_LOG_DEBUG("func: %s, path: %s, mode: %o\n", __FUNCTION__, path, mode);
    if (FCFS_PRELOAD_IS_MY_FD_MOUNTPOINT(fd, path)) {
        return fcfs_openat(fd, path, flags, mode);
    } else {
        return syscall(SYS_openat, fd, path, flags, mode);
    }
}

int _openat_(int fd, const char *path, int flags, ...)
{
    va_list ap;
    int mode;

    va_start(ap, flags);
    mode = va_arg(ap, int);
    va_end(ap);

    FCFS_LOG_DEBUG("func: %s, path: %s, mode: %o\n", __FUNCTION__, path, mode);
    return do_openat(fd, path, flags, mode);
}

int openat64(int fd, const char *path, int flags, ...)
{
    va_list ap;
    int mode;

    va_start(ap, flags);
    mode = va_arg(ap, int);
    va_end(ap);

    FCFS_LOG_DEBUG("func: %s, path: %s, mode: %o\n", __FUNCTION__, path, mode);
    return do_openat(fd, path, flags, mode);
}

int __openat_2_(int fd, const char *path, int flags)
{
    FCFS_LOG_DEBUG("func: %s, path: %s\n", __FUNCTION__, path);
    return do_openat(fd, path, flags, 0666);
}

int __openat64_2(int fd, const char *path, int flags)
{
    FCFS_LOG_DEBUG("func: %s, path: %s\n", __FUNCTION__, path);
    return do_openat(fd, path, flags, 0666);
}

int fstatat(int fd, const char *path, struct stat *buf, int flags)
{
    FCFS_LOG_DEBUG("func: %s, fd: %d, path: %s\n", __FUNCTION__, fd, path);
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
    FCFS_LOG_DEBUG("%d. line: %d, func: %s, fd: %d, path: %s\n", ++counter, __LINE__, __FUNCTION__, fd, path);
    if (FCFS_PRELOAD_IS_MY_FD_MOUNTPOINT(fd, path)) {
        return fcfs_fstatat(fd, path, buf, flags);
    } else {
        if (g_fcfs_preload_global_vars.__fxstatat == NULL) {
            g_fcfs_preload_global_vars.__fxstatat = fcfs_dlsym1("__fxstatat");
        }
        return g_fcfs_preload_global_vars.__fxstatat(ver, fd, path, buf, flags);
    }
}

int __fxstatat_(int ver, int fd, const char *path,
        struct stat *buf, int flags)
{
    return do_fxstatat(ver, fd, path, buf, flags);
}

int __fxstatat64(int ver, int fd, const char *path,
        struct stat64 *buf, int flags)
{
    return do_fxstatat(ver, fd, path, (struct stat *)buf, flags);
}

ssize_t readlinkat(int fd, const char *path, char *buff, size_t size)
{
    if (FCFS_PRELOAD_IS_MY_FD_MOUNTPOINT(fd, path)) {
        return fcfs_readlinkat(fd, path, buff, size);
    } else {
        return syscall(SYS_readlinkat, fd, path, buff, size);
    }
}

int mknodat(int fd, const char *path, mode_t mode, dev_t dev)
{
    if (FCFS_PRELOAD_IS_MY_FD_MOUNTPOINT(fd, path)) {
        return fcfs_mknodat(fd, path, mode, dev);
    } else {
        return syscall(SYS_mknodat, fd, path, mode, dev);
    }
}

int __xmknodat(int ver, int fd, const char *path, mode_t mode, dev_t *dev)
{
    if (FCFS_PRELOAD_IS_MY_FD_MOUNTPOINT(fd, path)) {
        return fcfs_mknodat(fd, path, mode, *dev);
    } else {
        if (g_fcfs_preload_global_vars.__xmknodat == NULL) {
            g_fcfs_preload_global_vars.__xmknodat = fcfs_dlsym1("__xmknodat");
        }
        return g_fcfs_preload_global_vars.__xmknodat(ver, fd, path, mode, dev);
    }
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
    FCFS_LOG_DEBUG("%d. func: %s, path: %s, mode: %o\n", ++counter, __FUNCTION__, path, mode);
    if (FCFS_PRELOAD_IS_MY_FD_MOUNTPOINT(fd, path)) {
        return fcfs_faccessat(fd, path, mode, flags);
    } else {
        return syscall(SYS_faccessat, fd, path, mode, flags);
    }
}

int futimesat(int fd, const char *path, const struct timeval times[2])
{
    if (FCFS_PRELOAD_IS_MY_FD_MOUNTPOINT(fd, path)) {
        return fcfs_futimesat(fd, path, times);
    } else {
        return syscall(SYS_futimesat, fd, path, times);
    }
}

int utimensat(int fd, const char *path, const struct timespec times[2], int flags)
{
    if (FCFS_PRELOAD_IS_MY_FD_MOUNTPOINT(fd, path)) {
        return fcfs_utimensat(fd, path, times, flags);
    } else {
        return syscall(SYS_utimensat, fd, path, times, flags);
    }
}

int unlinkat(int fd, const char *path, int flags)
{
    if (FCFS_PRELOAD_IS_MY_FD_MOUNTPOINT(fd, path)) {
        return fcfs_unlinkat(fd, path, flags);
    } else {
        return syscall(SYS_unlinkat, fd, path, flags);
    }
}

int mkdirat(int fd, const char *path, mode_t mode)
{
    if (FCFS_PRELOAD_IS_MY_FD_MOUNTPOINT(fd, path)) {
        return fcfs_mkdirat(fd, path, mode);
    } else {
        return syscall(SYS_futimesat, fd, path, mode);
    }
}

int fchownat(int fd, const char *path, uid_t owner, gid_t group, int flags)
{
    if (FCFS_PRELOAD_IS_MY_FD_MOUNTPOINT(fd, path)) {
        return fcfs_fchownat(fd, path, owner, group, flags);
    } else {
        return syscall(SYS_fchownat, fd, path, owner, group, flags);
    }
}

int fchmodat(int fd, const char *path, mode_t mode, int flags)
{
    if (FCFS_PRELOAD_IS_MY_FD_MOUNTPOINT(fd, path)) {
        return fcfs_fchmodat(fd, path, mode, flags);
    } else {
        return syscall(SYS_fchmodat, fd, path, mode, flags);
    }
}

int linkat(int fd1, const char *path1, int fd2, const char *path2, int flags)
{
    if (FCFS_PRELOAD_IS_MY_FD_MOUNTPOINT(fd1, path1) ||
            FCFS_PRELOAD_IS_MY_FD_MOUNTPOINT(fd2, path2))
    {
        return fcfs_linkat(fd1, path1, fd2, path2, flags);
    } else {
        return syscall(SYS_linkat, fd1, path1, fd2, path2, flags);
    }
}

int renameat(int fd1, const char *path1, int fd2, const char *path2)
{
    if (FCFS_PRELOAD_IS_MY_FD_MOUNTPOINT(fd1, path1) ||
            FCFS_PRELOAD_IS_MY_FD_MOUNTPOINT(fd2, path2))
    {
        return fcfs_renameat(fd1, path1, fd2, path2);
    } else {
        return syscall(SYS_renameat, fd1, path1, fd2, path2);
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
        return syscall(SYS_renameat2, fd1, path1, fd2, path2, flags);
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
    FCFS_LOG_DEBUG("%d. func: %s, line: %d, size: %d\n",
            ++counter, __FUNCTION__, __LINE__, (int)size);
    if (g_fcfs_preload_global_vars.cwd_call_type ==
            FCFS_PRELOAD_CALL_FASTCFS)
    {
        return fcfs_getcwd(buf, size);
    } else {
        return g_fcfs_preload_global_vars.getcwd(buf, size);
    }
}

char *getwd(char *buf)
{
    if (g_fcfs_preload_global_vars.cwd_call_type ==
            FCFS_PRELOAD_CALL_FASTCFS)
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

    FCFS_LOG_DEBUG("%d. func: %s, line: %d\n", ++counter, __FUNCTION__, __LINE__);
    wapper = (FCFSPreloadDIRWrapper *)dirp;
    if (wapper->call_type == FCFS_PRELOAD_CALL_FASTCFS) {
        result = fcfs_closedir(wapper->dirp);
    } else if (wapper->call_type == FCFS_PRELOAD_CALL_SYSTEM) {
        if (g_fcfs_preload_global_vars.closedir == NULL) {
            g_fcfs_preload_global_vars.closedir = fcfs_dlsym1("closedir");
        }
        result = g_fcfs_preload_global_vars.closedir(wapper->dirp);
    } else {
        errno = EBADF;
        return -1;
    }

    free(wapper);
    return result;
}

static inline struct dirent *do_readdir(DIR *dirp)
{
    FCFSPreloadDIRWrapper *wapper;

    wapper = (FCFSPreloadDIRWrapper *)dirp;
    if (wapper->call_type == FCFS_PRELOAD_CALL_FASTCFS) {
        return fcfs_readdir(wapper->dirp);
    } else if (wapper->call_type == FCFS_PRELOAD_CALL_SYSTEM) {
        if (g_fcfs_preload_global_vars.readdir == NULL) {
            g_fcfs_preload_global_vars.readdir = fcfs_dlsym1("readdir");
        }
        return g_fcfs_preload_global_vars.readdir(wapper->dirp);
    } else {
        errno = EBADF;
        return NULL;
    }
}

struct dirent *_readdir_(DIR *dirp)
{
    FCFS_LOG_DEBUG("%d. func: %s, line: %d\n", ++counter, __FUNCTION__, __LINE__);
    return do_readdir(dirp);
}

struct dirent64 *readdir64(DIR *dirp)
{
    FCFS_LOG_DEBUG("%d. func: %s, line: %d\n", ++counter, __FUNCTION__, __LINE__);
    return (struct dirent64 *)do_readdir(dirp);
}

static inline int do_readdir_r(DIR *dirp, struct dirent *entry,
        struct dirent **result)
{
    FCFSPreloadDIRWrapper *wapper;

    wapper = (FCFSPreloadDIRWrapper *)dirp;

    if (wapper->call_type == FCFS_PRELOAD_CALL_FASTCFS) {
        return fcfs_readdir_r(wapper->dirp, entry, result);
    } else if (wapper->call_type == FCFS_PRELOAD_CALL_SYSTEM) {
        if (g_fcfs_preload_global_vars.readdir_r == NULL) {
            g_fcfs_preload_global_vars.readdir_r = fcfs_dlsym1("readdir_r");
        }
        return g_fcfs_preload_global_vars.readdir_r(
                wapper->dirp, entry, result);
    } else {
        return EBADF;
    }
}

int _readdir_r_(DIR *dirp, struct dirent *entry, struct dirent **result)
{
    FCFS_LOG_DEBUG("%d. func: %s, line: %d\n", ++counter, __FUNCTION__, __LINE__);
    return do_readdir_r(dirp, entry, result);
}

int readdir64_r(DIR *dirp, struct dirent64 *entry, struct dirent64 **result)
{
    FCFS_LOG_DEBUG("%d. func: %s, line: %d\n", ++counter, __FUNCTION__, __LINE__);
    return do_readdir_r(dirp, (struct dirent *)entry, (struct dirent **)result);
}

void seekdir(DIR *dirp, long loc)
{
    FCFSPreloadDIRWrapper *wapper;

    wapper = (FCFSPreloadDIRWrapper *)dirp;
    if (wapper->call_type == FCFS_PRELOAD_CALL_FASTCFS) {
        return fcfs_seekdir(wapper->dirp, loc);
    } else if (wapper->call_type == FCFS_PRELOAD_CALL_SYSTEM) {
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
    if (wapper->call_type == FCFS_PRELOAD_CALL_FASTCFS) {
        return fcfs_telldir(wapper->dirp);
    } else if (wapper->call_type == FCFS_PRELOAD_CALL_SYSTEM) {
        if (g_fcfs_preload_global_vars.telldir == NULL) {
            g_fcfs_preload_global_vars.telldir = fcfs_dlsym1("telldir");
        }
        return g_fcfs_preload_global_vars.telldir(wapper->dirp);
    } else {
        errno = EBADF;
        return -1;
    }
}

void rewinddir(DIR *dirp)
{
    FCFSPreloadDIRWrapper *wapper;

    wapper = (FCFSPreloadDIRWrapper *)dirp;
    if (wapper->call_type == FCFS_PRELOAD_CALL_FASTCFS) {
        return fcfs_rewinddir(wapper->dirp);
    } else if (wapper->call_type == FCFS_PRELOAD_CALL_SYSTEM) {
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
    if (wapper->call_type == FCFS_PRELOAD_CALL_FASTCFS) {
        return fcfs_dirfd(wapper->dirp);
    } else if (wapper->call_type == FCFS_PRELOAD_CALL_SYSTEM) {
        if (g_fcfs_preload_global_vars.dirfd == NULL) {
            g_fcfs_preload_global_vars.dirfd = fcfs_dlsym1("dirfd");
        }
        return g_fcfs_preload_global_vars.dirfd(wapper->dirp);
    } else {
        errno = EBADF;
        return -1;
    }
}

int vdprintf(int fd, const char *format, va_list ap)
{
    if (FCFS_PAPI_IS_MY_FD(fd)) {
        return fcfs_vdprintf(fd, format, ap);
    } else {
        if (g_fcfs_preload_global_vars.vdprintf == NULL) {
            g_fcfs_preload_global_vars.vdprintf = fcfs_dlsym1("vdprintf");
        }
        return g_fcfs_preload_global_vars.vdprintf(fd, format, ap);
    }
}

int dprintf(int fd, const char *format, ...)
{
    va_list ap;
    int bytes;

    va_start(ap, format);
    bytes = vdprintf(fd, format, ap);
    va_end(ap);
    return bytes;
}

static inline int do_lockf(int fd, int cmd, off_t len)
{
    if (FCFS_PAPI_IS_MY_FD(fd)) {
        return fcfs_lockf(fd, cmd, len);
    } else {
        if (g_fcfs_preload_global_vars.lockf == NULL) {
            g_fcfs_preload_global_vars.lockf = fcfs_dlsym2(
                    "lockf", "lockf64");
        }
        return g_fcfs_preload_global_vars.lockf(fd, cmd, len);
    }
}

int _lockf_(int fd, int cmd, off_t len)
{
    return do_lockf(fd, cmd, len);
}

int lockf64(int fd, int cmd, off_t len)
{
    return do_lockf(fd, cmd, len);
}

static inline int do_posix_fallocate(int fd, off_t offset, off_t len)
{
    if (FCFS_PAPI_IS_MY_FD(fd)) {
        return fcfs_posix_fallocate(fd, offset, len);
    } else {
        if (g_fcfs_preload_global_vars.posix_fallocate == NULL) {
            g_fcfs_preload_global_vars.posix_fallocate = fcfs_dlsym2(
                    "posix_fallocate", "posix_fallocate64");
        }
        return g_fcfs_preload_global_vars.posix_fallocate(fd, offset, len);
    }
}

int _posix_fallocate_(int fd, off_t offset, off_t len)
{
    return do_posix_fallocate(fd, offset, len);
}

int posix_fallocate64(int fd, off_t offset, off_t len)
{
    return do_posix_fallocate(fd, offset, len);
}

int _posix_fadvise_(int fd, off_t offset, off_t len, int advice)
{
    if (FCFS_PAPI_IS_MY_FD(fd)) {
        return fcfs_posix_fadvise(fd, offset, len, advice);
    } else {
        if (g_fcfs_preload_global_vars.posix_fadvise == NULL) {
            g_fcfs_preload_global_vars.posix_fadvise = fcfs_dlsym2(
                    "posix_fadvise", "posix_fadvise64");
        }
        return g_fcfs_preload_global_vars.posix_fadvise(
                fd, offset, len, advice);
    }
}

int posix_fadvise64(int fd, off_t offset, off_t len, int advice)
{
    return _posix_fadvise_(fd, offset, len, advice);
}

int unsetenv(const char *name)
{
    FCFS_LOG_DEBUG("pid: %d, func: %s, line: %d, name: %s\n",
            getpid(), __FUNCTION__, __LINE__, name);

    if (g_fcfs_preload_global_vars.unsetenv == NULL) {
        g_fcfs_preload_global_vars.unsetenv = fcfs_dlsym1("unsetenv");
    }
    return g_fcfs_preload_global_vars.unsetenv(name);
}

int clearenv(void)
{
    FCFS_LOG_DEBUG("pid: %d, func: %s, line: %d\n",
            getpid(), __FUNCTION__, __LINE__);

    if (g_fcfs_preload_global_vars.clearenv == NULL) {
        g_fcfs_preload_global_vars.clearenv = fcfs_dlsym1("clearenv");
    }
    return g_fcfs_preload_global_vars.clearenv();
}

#ifdef FCFS_PRELOAD_WITH_CAPI
static inline FILE *do_fopen(const char *path, const char *mode)
{
    FCFSPreloadFILEWrapper *wapper;
    FILE *fp;
    int call_type;

    if (FCFS_PRELOAD_IS_MY_MOUNTPOINT(path)) {
        call_type = FCFS_PRELOAD_CALL_FASTCFS;
        fp = fcfs_fopen(path, mode);
    } else {
        call_type = FCFS_PRELOAD_CALL_SYSTEM;
        if (g_fcfs_preload_global_vars.fopen == NULL) {
            g_fcfs_preload_global_vars.fopen = fcfs_dlsym2("fopen", "fopen64");
        }
        fp = g_fcfs_preload_global_vars.fopen(path, mode);

        FCFS_LOG_DEBUG("func: %s, line: %d, fp ========== %p\n",  __FUNCTION__, __LINE__, fp);
    }

    FCFS_LOG_DEBUG("func: %s, line: %d, fp: %p\n",  __FUNCTION__, __LINE__, fp);

    if (fp != NULL) {
        wapper = fc_malloc(sizeof(FCFSPreloadFILEWrapper));
        wapper->call_type = call_type;
        wapper->fp = fp;
        return (FILE *)wapper;
    } else {
        return NULL;
    }
}

FILE *_fopen_(const char *path, const char *mode)
{
    FCFS_LOG_DEBUG("pid: %d, func: %s, line: %d, path: %s, mode: %s, inited: %d\n",
            getpid(), __FUNCTION__, __LINE__, path, mode, g_fcfs_preload_global_vars.inited);

    return do_fopen(path, mode);
}

FILE *fopen64(const char *path, const char *mode)
{
    FCFS_LOG_DEBUG("pid: %d, func: %s, line: %d, path: %s, mode: %s\n",
            getpid(), __FUNCTION__, __LINE__, path, mode);
    return do_fopen(path, mode);
}

FILE *_IO_fdopen(int fd, const char *mode)
{
    FCFSPreloadFILEWrapper *wapper;
    FILE *fp;
    int call_type;

    FCFS_LOG_DEBUG("====== func: %s, line: %d, fd: %d, mode: %s\n",
            __FUNCTION__, __LINE__, fd, mode);

    if (FCFS_PAPI_IS_MY_FD(fd)) {
        call_type = FCFS_PRELOAD_CALL_FASTCFS;
        fp = fcfs_fdopen(fd, mode);
    } else {
        call_type = FCFS_PRELOAD_CALL_SYSTEM;
        if (g_fcfs_preload_global_vars.fdopen == NULL) {
            g_fcfs_preload_global_vars.fdopen = fcfs_dlsym1("fdopen");
        }
        fp = g_fcfs_preload_global_vars.fdopen(fd, mode);
    }

    if (fp != NULL) {
        wapper = fc_malloc(sizeof(FCFSPreloadFILEWrapper));
        wapper->call_type = call_type;
        wapper->fp = fp;
        return (FILE *)wapper;
    } else {
        return NULL;
    }
}

FILE *fdopen(int fd, const char *mode)
{
    return _IO_fdopen(fd, mode);
}

FILE *_freopen_(const char *path, const char *mode, FILE *fp)
{
    FCFSPreloadFILEWrapper *wapper;

    wapper = (FCFSPreloadFILEWrapper *)fp;
    FCFS_LOG_DEBUG("====== func: %s, line: %d, wapper: %p, fp: %p\n",
            __FUNCTION__, __LINE__, wapper, wapper->fp);
    if (wapper->call_type == FCFS_PRELOAD_CALL_FASTCFS) {
        fp = fcfs_freopen(path, mode, wapper->fp);
    } else if (wapper->call_type == FCFS_PRELOAD_CALL_SYSTEM) {
        if (g_fcfs_preload_global_vars.freopen == NULL) {
            g_fcfs_preload_global_vars.freopen = fcfs_dlsym1("freopen");
        }
        fp = g_fcfs_preload_global_vars.freopen(path, mode, wapper->fp);
    } else {
        errno = EBADF;
        return NULL;
    }

    if (fp != NULL) {
        wapper->fp = fp;
        return (FILE *)wapper;
    } else {
        return NULL;
    }
}

FILE *freopen64(const char *path, const char *mode, FILE *fp)
{
    return _freopen_(path, mode, fp);
}

#define CHECK_DEAL_STD_STREAM(funcname, ...) \
    if (fp == stdin || fp == stderr || fp == stdout) {  \
        if (g_fcfs_preload_global_vars.funcname == NULL) { \
            g_fcfs_preload_global_vars.funcname = fcfs_dlsym1(#funcname); \
        } \
        return g_fcfs_preload_global_vars.funcname(__VA_ARGS__); \
    }

#define CHECK_DEAL_STDIO_VOID(funcname, ...) \
    if (fp == stdin || fp == stderr || fp == stdout) {  \
        if (g_fcfs_preload_global_vars.funcname == NULL) { \
            g_fcfs_preload_global_vars.funcname = fcfs_dlsym1(#funcname); \
        } \
        g_fcfs_preload_global_vars.funcname(__VA_ARGS__); \
    }

int fclose(FILE *fp)
{
    FCFSPreloadFILEWrapper *wapper;

    CHECK_DEAL_STD_STREAM(fclose, fp);
    wapper = (FCFSPreloadFILEWrapper *)fp;

    FCFS_LOG_DEBUG("func: %s, line: %d, wapper: %p, fp: %p\n",
            __FUNCTION__, __LINE__, wapper, wapper->fp);

    if (wapper->call_type == FCFS_PRELOAD_CALL_FASTCFS) {
        return fcfs_fclose(wapper->fp);
    } else if (wapper->call_type == FCFS_PRELOAD_CALL_SYSTEM) {
        if (g_fcfs_preload_global_vars.fclose == NULL) {
            g_fcfs_preload_global_vars.fclose = fcfs_dlsym1("fclose");
        }
        return g_fcfs_preload_global_vars.fclose(wapper->fp);
    } else {
        errno = EBADF;
        return EOF;
    }
}

int fcloseall()
{
    int r1;
    int r2;

    FCFS_LOG_DEBUG("func: %s, line: %d\n", __FUNCTION__, __LINE__);

    r1 = fcfs_fcloseall();
    if (g_fcfs_preload_global_vars.fcloseall == NULL) {
        g_fcfs_preload_global_vars.fcloseall = fcfs_dlsym1("fcloseall");
    }
    r2 = g_fcfs_preload_global_vars.fcloseall();
    return (r1 == 0 ? r2 : r1);
}

void flockfile(FILE *fp)
{
    FCFSPreloadFILEWrapper *wapper;

    CHECK_DEAL_STDIO_VOID(flockfile, fp);
    wapper = (FCFSPreloadFILEWrapper *)fp;
    if (wapper->call_type == FCFS_PRELOAD_CALL_FASTCFS) {
        fcfs_flockfile(wapper->fp);
    } else if (wapper->call_type == FCFS_PRELOAD_CALL_SYSTEM) {
        if (g_fcfs_preload_global_vars.flockfile == NULL) {
            g_fcfs_preload_global_vars.flockfile = fcfs_dlsym1("flockfile");
        }
        g_fcfs_preload_global_vars.flockfile(wapper->fp);
    }
}

int ftrylockfile(FILE *fp)
{
    FCFSPreloadFILEWrapper *wapper;

    CHECK_DEAL_STD_STREAM(ftrylockfile, fp);
    wapper = (FCFSPreloadFILEWrapper *)fp;
    if (wapper->call_type == FCFS_PRELOAD_CALL_FASTCFS) {
        return fcfs_ftrylockfile(wapper->fp);
    } else if (wapper->call_type == FCFS_PRELOAD_CALL_SYSTEM) {
        if (g_fcfs_preload_global_vars.ftrylockfile == NULL) {
            g_fcfs_preload_global_vars.ftrylockfile = fcfs_dlsym1("ftrylockfile");
        }
        return g_fcfs_preload_global_vars.ftrylockfile(wapper->fp);
    } else {
        errno = EBADF;
        return EOF;
    }
}

void funlockfile(FILE *fp)
{
    FCFSPreloadFILEWrapper *wapper;

    CHECK_DEAL_STDIO_VOID(funlockfile, fp);
    wapper = (FCFSPreloadFILEWrapper *)fp;
    if (wapper->call_type == FCFS_PRELOAD_CALL_FASTCFS) {
        fcfs_funlockfile(wapper->fp);
    } else if (wapper->call_type == FCFS_PRELOAD_CALL_SYSTEM) {
        if (g_fcfs_preload_global_vars.funlockfile == NULL) {
            g_fcfs_preload_global_vars.funlockfile = fcfs_dlsym1("funlockfile");
        }
        g_fcfs_preload_global_vars.funlockfile(wapper->fp);
    }
}

int fseek(FILE *fp, long offset, int whence)
{
    FCFSPreloadFILEWrapper *wapper;

    CHECK_DEAL_STD_STREAM(fseek, fp, offset, whence);
    wapper = (FCFSPreloadFILEWrapper *)fp;
    FCFS_LOG_DEBUG("func: %s, line: %d, wapper: %p, fp: %p\n",
            __FUNCTION__, __LINE__, wapper, wapper->fp);
    if (wapper->call_type == FCFS_PRELOAD_CALL_FASTCFS) {
        return fcfs_fseek(wapper->fp, offset, whence);
    } else if (wapper->call_type == FCFS_PRELOAD_CALL_SYSTEM) {
        if (g_fcfs_preload_global_vars.fseek == NULL) {
            g_fcfs_preload_global_vars.fseek = fcfs_dlsym1("fseek");
        }
        return g_fcfs_preload_global_vars.fseek(wapper->fp, offset, whence);
    } else {
        errno = EBADF;
        return EOF;
    }
}

int _fseeko_(FILE *fp, off_t offset, int whence)
{
    FCFSPreloadFILEWrapper *wapper;

    CHECK_DEAL_STD_STREAM(fseeko, fp, offset, whence);

    wapper = (FCFSPreloadFILEWrapper *)fp;
    FCFS_LOG_DEBUG("func: %s, line: %d, wapper: %p, fp: %p\n",
            __FUNCTION__, __LINE__, wapper, wapper->fp);
    if (wapper->call_type == FCFS_PRELOAD_CALL_FASTCFS) {
        return fcfs_fseeko(wapper->fp, offset, whence);
    } else if (wapper->call_type == FCFS_PRELOAD_CALL_SYSTEM) {
        if (g_fcfs_preload_global_vars.fseeko == NULL) {
            g_fcfs_preload_global_vars.fseeko = fcfs_dlsym1("fseeko");
        }
        return g_fcfs_preload_global_vars.fseeko(wapper->fp, offset, whence);
    } else {
        errno = EBADF;
        return EOF;
    }
}

int fseeko64(FILE *fp, off_t offset, int whence)
{
    FCFS_LOG_DEBUG("func: %s, line: %d\n", __FUNCTION__, __LINE__);
    return _fseeko_(fp, offset, whence);
}

int __fseeko64(FILE *fp, off_t offset, int whence)
{
    FCFS_LOG_DEBUG("func: %s, line: %d\n", __FUNCTION__, __LINE__);
    return _fseeko_(fp, offset, whence);
}

long ftell(FILE *fp)
{
    FCFSPreloadFILEWrapper *wapper;

    CHECK_DEAL_STD_STREAM(ftell, fp);

    wapper = (FCFSPreloadFILEWrapper *)fp;
    FCFS_LOG_DEBUG("func: %s, line: %d, wapper: %p, fp: %p\n",
            __FUNCTION__, __LINE__, wapper, wapper->fp);
    if (wapper->call_type == FCFS_PRELOAD_CALL_FASTCFS) {
        return fcfs_ftell(wapper->fp);
    } else if (wapper->call_type == FCFS_PRELOAD_CALL_SYSTEM) {
        if (g_fcfs_preload_global_vars.ftell == NULL) {
            g_fcfs_preload_global_vars.ftell = fcfs_dlsym1("ftell");
        }
        return g_fcfs_preload_global_vars.ftell(wapper->fp);
    } else {
        errno = EBADF;
        return EOF;
    }
}

long _IO_ftell(FILE *fp)
{
    return ftell(fp);
}

static inline off_t do_ftello(FILE *fp)
{
    FCFSPreloadFILEWrapper *wapper;

    CHECK_DEAL_STD_STREAM(ftello, fp);
    FCFS_LOG_DEBUG("func: %s, line: %d\n", __FUNCTION__, __LINE__);
    wapper = (FCFSPreloadFILEWrapper *)fp;
    if (wapper->call_type == FCFS_PRELOAD_CALL_FASTCFS) {
        return fcfs_ftello(wapper->fp);
    } else if (wapper->call_type == FCFS_PRELOAD_CALL_SYSTEM) {
        if (g_fcfs_preload_global_vars.ftello == NULL) {
            g_fcfs_preload_global_vars.ftello = fcfs_dlsym1("ftello");
        }
        return g_fcfs_preload_global_vars.ftello(wapper->fp);
    } else {
        errno = EBADF;
        return EOF;
    }
}

off_t _ftello_(FILE *fp)
{
    FCFS_LOG_DEBUG("func: %s, line: %d\n", __FUNCTION__, __LINE__);
    return do_ftello(fp);
}

off_t ftello64(FILE *fp)
{
    FCFS_LOG_DEBUG("func: %s, line: %d\n", __FUNCTION__, __LINE__);
    return do_ftello(fp);
}

off_t __ftello64(FILE *fp)
{
    FCFS_LOG_DEBUG("func: %s, line: %d\n", __FUNCTION__, __LINE__);
    return do_ftello(fp);
}

void rewind(FILE *fp)
{
    FCFSPreloadFILEWrapper *wapper;

    CHECK_DEAL_STDIO_VOID(rewind, fp);

    FCFS_LOG_DEBUG("func: %s, line: %d\n", __FUNCTION__, __LINE__);
    wapper = (FCFSPreloadFILEWrapper *)fp;
    if (wapper->call_type == FCFS_PRELOAD_CALL_FASTCFS) {
        fcfs_rewind(wapper->fp);
    } else if (wapper->call_type == FCFS_PRELOAD_CALL_SYSTEM) {
        if (g_fcfs_preload_global_vars.rewind == NULL) {
            g_fcfs_preload_global_vars.rewind = fcfs_dlsym1("rewind");
        }
        g_fcfs_preload_global_vars.rewind(wapper->fp);
    }
}

int _fgetpos_(FILE *fp, fpos_t *pos)
{
    FCFSPreloadFILEWrapper *wapper;

    CHECK_DEAL_STD_STREAM(fgetpos, fp, pos);
    wapper = (FCFSPreloadFILEWrapper *)fp;
    FCFS_LOG_DEBUG("func: %s, line: %d, wapper: %p, fp: %p\n",
            __FUNCTION__, __LINE__, wapper, wapper->fp);
    if (wapper->call_type == FCFS_PRELOAD_CALL_FASTCFS) {
        return fcfs_fgetpos(wapper->fp, pos);
    } else if (wapper->call_type == FCFS_PRELOAD_CALL_SYSTEM) {
        if (g_fcfs_preload_global_vars.fgetpos == NULL) {
            g_fcfs_preload_global_vars.fgetpos = fcfs_dlsym1("fgetpos");
        }
        return g_fcfs_preload_global_vars.fgetpos(wapper->fp, pos);
    } else {
        errno = EBADF;
        return EOF;
    }
}

int fgetpos64(FILE *fp, fpos_t *pos)
{
    return _fgetpos_(fp, pos);
}

int _IO_fgetpos(FILE *fp, fpos_t *pos)
{
    return _fgetpos_(fp, pos);
}

int _IO_fgetpos64(FILE *fp, fpos_t *pos)
{
    return _fgetpos_(fp, pos);
}

int _fsetpos_(FILE *fp, const fpos_t *pos)
{
    FCFSPreloadFILEWrapper *wapper;

    CHECK_DEAL_STD_STREAM(fsetpos, fp, pos);
    wapper = (FCFSPreloadFILEWrapper *)fp;
    FCFS_LOG_DEBUG("func: %s, line: %d, wapper: %p, fp: %p\n",
            __FUNCTION__, __LINE__, wapper, wapper->fp);
    if (wapper->call_type == FCFS_PRELOAD_CALL_FASTCFS) {
        return fcfs_fsetpos(wapper->fp, pos);
    } else if (wapper->call_type == FCFS_PRELOAD_CALL_SYSTEM) {
        if (g_fcfs_preload_global_vars.fsetpos == NULL) {
            g_fcfs_preload_global_vars.fsetpos = fcfs_dlsym1("fsetpos");
        }
        return g_fcfs_preload_global_vars.fsetpos(wapper->fp, pos);
    } else {
        errno = EBADF;
        return EOF;
    }
}

int fsetpos64(FILE *fp, const fpos_t *pos)
{
    return _fsetpos_(fp, pos);
}

int _IO_fsetpos(FILE *fp, const fpos_t *pos)
{
    return _fsetpos_(fp, pos);
}

int _IO_fsetpos64(FILE *fp, const fpos_t *pos)
{
    return _fsetpos_(fp, pos);
}

int fgetc_unlocked(FILE *fp)
{
    FCFSPreloadFILEWrapper *wapper;

    CHECK_DEAL_STD_STREAM(fgetc_unlocked, fp);
    wapper = (FCFSPreloadFILEWrapper *)fp;
    FCFS_LOG_DEBUG("func: %s, line: %d, wapper: %p, fp: %p\n",
            __FUNCTION__, __LINE__, wapper, wapper->fp);
    if (wapper->call_type == FCFS_PRELOAD_CALL_FASTCFS) {
        return fcfs_fgetc_unlocked(wapper->fp);
    } else if (wapper->call_type == FCFS_PRELOAD_CALL_SYSTEM) {
        if (g_fcfs_preload_global_vars.fgetc_unlocked == NULL) {
            g_fcfs_preload_global_vars.fgetc_unlocked = fcfs_dlsym1("fgetc_unlocked");
        }
        return g_fcfs_preload_global_vars.fgetc_unlocked(wapper->fp);
    } else {
        errno = EBADF;
        return EOF;
    }
}

int fputc_unlocked(int c, FILE *fp)
{
    FCFSPreloadFILEWrapper *wapper;

    CHECK_DEAL_STD_STREAM(fputc_unlocked, c, fp);
    wapper = (FCFSPreloadFILEWrapper *)fp;
    if (wapper->call_type == FCFS_PRELOAD_CALL_FASTCFS) {
        return fcfs_fputc_unlocked(c, wapper->fp);
    } else if (wapper->call_type == FCFS_PRELOAD_CALL_SYSTEM) {
        if (g_fcfs_preload_global_vars.fputc_unlocked == NULL) {
            g_fcfs_preload_global_vars.fputc_unlocked = fcfs_dlsym1("fputc_unlocked");
        }
        return g_fcfs_preload_global_vars.fputc_unlocked(c, wapper->fp);
    } else {
        errno = EBADF;
        return EOF;
    }
}

int getc_unlocked(FILE *fp)
{
    FCFSPreloadFILEWrapper *wapper;

    CHECK_DEAL_STD_STREAM(getc_unlocked, fp);
    wapper = (FCFSPreloadFILEWrapper *)fp;
    if (wapper->call_type == FCFS_PRELOAD_CALL_FASTCFS) {
        return fcfs_getc_unlocked(wapper->fp);
    } else if (wapper->call_type == FCFS_PRELOAD_CALL_SYSTEM) {
        if (g_fcfs_preload_global_vars.getc_unlocked == NULL) {
            g_fcfs_preload_global_vars.getc_unlocked = fcfs_dlsym1("getc_unlocked");
        }
        return g_fcfs_preload_global_vars.getc_unlocked(wapper->fp);
    } else {
        errno = EBADF;
        return EOF;
    }
}

int putc_unlocked(int c, FILE *fp)
{
    FCFSPreloadFILEWrapper *wapper;

    CHECK_DEAL_STD_STREAM(putc_unlocked, c, fp);
    wapper = (FCFSPreloadFILEWrapper *)fp;
    if (wapper->call_type == FCFS_PRELOAD_CALL_FASTCFS) {
        return fcfs_putc_unlocked(c, wapper->fp);
    } else if (wapper->call_type == FCFS_PRELOAD_CALL_SYSTEM) {
        if (g_fcfs_preload_global_vars.putc_unlocked == NULL) {
            g_fcfs_preload_global_vars.putc_unlocked = fcfs_dlsym1("putc_unlocked");
        }
        return g_fcfs_preload_global_vars.putc_unlocked(c, wapper->fp);
    } else {
        errno = EBADF;
        return EOF;
    }
}

void clearerr_unlocked(FILE *fp)
{
    FCFSPreloadFILEWrapper *wapper;

    CHECK_DEAL_STDIO_VOID(clearerr_unlocked, fp);
    wapper = (FCFSPreloadFILEWrapper *)fp;
    if (wapper->call_type == FCFS_PRELOAD_CALL_FASTCFS) {
        fcfs_clearerr_unlocked(wapper->fp);
    } else if (wapper->call_type == FCFS_PRELOAD_CALL_SYSTEM) {
        if (g_fcfs_preload_global_vars.clearerr_unlocked == NULL) {
            g_fcfs_preload_global_vars.clearerr_unlocked = fcfs_dlsym1("clearerr_unlocked");
        }
        g_fcfs_preload_global_vars.clearerr_unlocked(wapper->fp);
    }
}

int feof_unlocked(FILE *fp)
{
    FCFSPreloadFILEWrapper *wapper;

    CHECK_DEAL_STD_STREAM(feof_unlocked, fp);
    wapper = (FCFSPreloadFILEWrapper *)fp;
    FCFS_LOG_DEBUG("func: %s, line: %d", __FUNCTION__, __LINE__);
    if (wapper->call_type == FCFS_PRELOAD_CALL_FASTCFS) {
        return fcfs_feof_unlocked(wapper->fp);
    } else if (wapper->call_type == FCFS_PRELOAD_CALL_SYSTEM) {
        if (g_fcfs_preload_global_vars.feof_unlocked == NULL) {
            g_fcfs_preload_global_vars.feof_unlocked = fcfs_dlsym1("feof_unlocked");
        }
        return g_fcfs_preload_global_vars.feof_unlocked(wapper->fp);
    } else {
        errno = EBADF;
        return EOF;
    }
}

#ifdef _IO_feof_unlocked
#undef _IO_feof_unlocked
#endif

int _IO_feof_unlocked(FILE *fp)
{
    return feof_unlocked(fp);
}

int ferror_unlocked(FILE *fp)
{
    FCFSPreloadFILEWrapper *wapper;

    CHECK_DEAL_STD_STREAM(ferror_unlocked, fp);
    wapper = (FCFSPreloadFILEWrapper *)fp;
    FCFS_LOG_DEBUG("func: %s, line: %d", __FUNCTION__, __LINE__);
    if (wapper->call_type == FCFS_PRELOAD_CALL_FASTCFS) {
        return fcfs_ferror_unlocked(wapper->fp);
    } else if (wapper->call_type == FCFS_PRELOAD_CALL_SYSTEM) {
        if (g_fcfs_preload_global_vars.ferror_unlocked == NULL) {
            g_fcfs_preload_global_vars.ferror_unlocked = fcfs_dlsym1("ferror_unlocked");
        }
        return g_fcfs_preload_global_vars.ferror_unlocked(wapper->fp);
    } else {
        errno = EBADF;
        return EOF;
    }
}

#ifdef _IO_ferror_unlocked
#undef _IO_ferror_unlocked
#endif

int _IO_ferror_unlocked(FILE *fp)
{
    return ferror_unlocked(fp);
}

int fileno_unlocked(FILE *fp)
{
    FCFSPreloadFILEWrapper *wapper;

    CHECK_DEAL_STD_STREAM(fileno_unlocked, fp);
    wapper = (FCFSPreloadFILEWrapper *)fp;
    FCFS_LOG_DEBUG("func: %s, line: %d", __FUNCTION__, __LINE__);
    if (wapper->call_type == FCFS_PRELOAD_CALL_FASTCFS) {
        return fcfs_fileno_unlocked(wapper->fp);
    } else if (wapper->call_type == FCFS_PRELOAD_CALL_SYSTEM) {
        if (g_fcfs_preload_global_vars.fileno_unlocked == NULL) {
            g_fcfs_preload_global_vars.fileno_unlocked =
                fcfs_dlsym1("fileno_unlocked");
        }
        return g_fcfs_preload_global_vars.fileno_unlocked(wapper->fp);
    } else {
        errno = EBADF;
        return EOF;
    }
}

int fflush_unlocked(FILE *fp)
{
    FCFSPreloadFILEWrapper *wapper;

    if (fp == NULL)  {
        return g_fcfs_preload_global_vars.fflush_unlocked(fp);
    }

    CHECK_DEAL_STD_STREAM(fflush_unlocked, fp);
    wapper = (FCFSPreloadFILEWrapper *)fp;
    if (wapper->call_type == FCFS_PRELOAD_CALL_FASTCFS) {
        return fcfs_fflush_unlocked(wapper->fp);
    } else if (wapper->call_type == FCFS_PRELOAD_CALL_SYSTEM) {
        if (g_fcfs_preload_global_vars.fflush_unlocked == NULL) {
            g_fcfs_preload_global_vars.fflush_unlocked = fcfs_dlsym1("fflush_unlocked");
        }
        return g_fcfs_preload_global_vars.fflush_unlocked(wapper->fp);
    } else {
        errno = EBADF;
        return EOF;
    }
}

size_t fread_unlocked(void *buff, size_t size, size_t n, FILE *fp)
{
    FCFSPreloadFILEWrapper *wapper;

    CHECK_DEAL_STD_STREAM(fread_unlocked, buff, size, n, fp);

    FCFS_LOG_DEBUG("func: %s, line: %d, size: %d, nmemb: %d\n", __FUNCTION__,
            __LINE__, (int)size, (int)n);

    wapper = (FCFSPreloadFILEWrapper *)fp;
    if (wapper->call_type == FCFS_PRELOAD_CALL_FASTCFS) {
        return fcfs_fread_unlocked(buff, size, n, wapper->fp);
    } else if (wapper->call_type == FCFS_PRELOAD_CALL_SYSTEM) {
        if (g_fcfs_preload_global_vars.fread_unlocked == NULL) {
            g_fcfs_preload_global_vars.fread_unlocked = fcfs_dlsym1("fread_unlocked");
        }
        //return g_fcfs_preload_global_vars.fread_unlocked(buff, size, n, wapper->fp);
        int bytes = g_fcfs_preload_global_vars.fread_unlocked(buff, size, n, wapper->fp);

        FCFS_LOG_DEBUG("func: %s, line: %d, size: %d, nmemb: %d, bytes: %d\n",
                __FUNCTION__, __LINE__, (int)size, (int)n, bytes);
        return bytes;
    } else {
        errno = EBADF;
        return EOF;
    }
}

size_t fwrite_unlocked(const void *buff, size_t size, size_t n, FILE *fp)
{
    FCFSPreloadFILEWrapper *wapper;

    CHECK_DEAL_STD_STREAM(fwrite_unlocked, buff, size, n, fp);
    wapper = (FCFSPreloadFILEWrapper *)fp;
    FCFS_LOG_DEBUG("func: %s, line: %d, wapper: %p, fp: %p\n",
            __FUNCTION__, __LINE__, wapper, wapper->fp);
    if (wapper->call_type == FCFS_PRELOAD_CALL_FASTCFS) {
        return fcfs_fwrite_unlocked(buff, size, n, wapper->fp);
    } else if (wapper->call_type == FCFS_PRELOAD_CALL_SYSTEM) {
        if (g_fcfs_preload_global_vars.fwrite_unlocked == NULL) {
            g_fcfs_preload_global_vars.fwrite_unlocked = fcfs_dlsym1("fwrite_unlocked");
        }
        return g_fcfs_preload_global_vars.fwrite_unlocked(buff, size, n, wapper->fp);
    } else {
        errno = EBADF;
        return EOF;
    }
}

char *fgets_unlocked(char *s, int size, FILE *fp)
{
    FCFSPreloadFILEWrapper *wapper;

    CHECK_DEAL_STD_STREAM(fgets_unlocked, s, size, fp);

    wapper = (FCFSPreloadFILEWrapper *)fp;
    FCFS_LOG_DEBUG("func: %s, line: %d, wapper: %p, fp: %p\n",
            __FUNCTION__, __LINE__, wapper, wapper->fp);
    if (wapper->call_type == FCFS_PRELOAD_CALL_FASTCFS) {
        return fcfs_fgets_unlocked(s, size, wapper->fp);
    } else if (wapper->call_type == FCFS_PRELOAD_CALL_SYSTEM) {
        if (g_fcfs_preload_global_vars.fgets_unlocked == NULL) {
            g_fcfs_preload_global_vars.fgets_unlocked = fcfs_dlsym1("fgets_unlocked");
        }
        return g_fcfs_preload_global_vars.fgets_unlocked(s, size, wapper->fp);
    } else {
        errno = EBADF;
        return NULL;
    }
}

ssize_t __libc_readline_unlocked (FILE *fp, char *buff, size_t size)
{
    FCFSPreloadFILEWrapper *wapper;

    CHECK_DEAL_STD_STREAM(__libc_readline_unlocked, fp, buff, size);
    wapper = (FCFSPreloadFILEWrapper *)fp;
    FCFS_LOG_DEBUG("func: %s, line: %d, wapper: %p, fp: %p\n",
            __FUNCTION__, __LINE__, wapper, wapper->fp);
    if (wapper->call_type == FCFS_PRELOAD_CALL_FASTCFS) {
        return fcfs_readline_unlocked(wapper->fp, buff, size);
    } else if (wapper->call_type == FCFS_PRELOAD_CALL_SYSTEM) {
        if (g_fcfs_preload_global_vars.__libc_readline_unlocked == NULL) {
            g_fcfs_preload_global_vars.__libc_readline_unlocked =
                fcfs_dlsym1("__libc_readline_unlocked");
        }
        return g_fcfs_preload_global_vars.__libc_readline_unlocked(
                wapper->fp, buff, size);
    } else {
        errno = EBADF;
        return -1;
    }
}

int fputs_unlocked(const char *s, FILE *fp)
{
    FCFSPreloadFILEWrapper *wapper;

    CHECK_DEAL_STD_STREAM(fputs_unlocked, s, fp);
    wapper = (FCFSPreloadFILEWrapper *)fp;
    if (wapper->call_type == FCFS_PRELOAD_CALL_FASTCFS) {
        return fcfs_fputs_unlocked(s, wapper->fp);
    } else if (wapper->call_type == FCFS_PRELOAD_CALL_SYSTEM) {
        if (g_fcfs_preload_global_vars.fputs_unlocked == NULL) {
            g_fcfs_preload_global_vars.fputs_unlocked = fcfs_dlsym1("fputs_unlocked");
        }
        return g_fcfs_preload_global_vars.fputs_unlocked(s, wapper->fp);
    } else {
        errno = EBADF;
        return EOF;
    }
}

void clearerr(FILE *fp)
{
    FCFSPreloadFILEWrapper *wapper;

    CHECK_DEAL_STDIO_VOID(clearerr, fp);
    wapper = (FCFSPreloadFILEWrapper *)fp;
    if (wapper->call_type == FCFS_PRELOAD_CALL_FASTCFS) {
        fcfs_clearerr(wapper->fp);
    } else if (wapper->call_type == FCFS_PRELOAD_CALL_SYSTEM) {
        if (g_fcfs_preload_global_vars.clearerr == NULL) {
            g_fcfs_preload_global_vars.clearerr = fcfs_dlsym1("clearerr");
        }
        g_fcfs_preload_global_vars.clearerr(wapper->fp);
    }
}

int _IO_feof(FILE *fp)
{
    FCFSPreloadFILEWrapper *wapper;

    CHECK_DEAL_STD_STREAM(feof, fp);

    wapper = (FCFSPreloadFILEWrapper *)fp;
    FCFS_LOG_DEBUG("func: %s, line: %d, wapper: %p, fp: %p\n",
            __FUNCTION__, __LINE__, wapper, wapper->fp);
    if (wapper->call_type == FCFS_PRELOAD_CALL_FASTCFS) {
        return fcfs_feof(wapper->fp);
    } else if (wapper->call_type == FCFS_PRELOAD_CALL_SYSTEM) {
        if (g_fcfs_preload_global_vars.feof == NULL) {
            g_fcfs_preload_global_vars.feof = fcfs_dlsym1("feof");
        }
        return g_fcfs_preload_global_vars.feof(wapper->fp);
    } else {
        errno = EBADF;
        return EOF;
    }
}

int _feof_(FILE *fp)
{
    return _IO_feof(fp);
}

int _ferror_(FILE *fp)
{
    FCFSPreloadFILEWrapper *wapper;

    CHECK_DEAL_STD_STREAM(feof, fp);
    wapper = (FCFSPreloadFILEWrapper *)fp;
    FCFS_LOG_DEBUG("func: %s, line: %d, wapper: %p, fp: %p\n",
            __FUNCTION__, __LINE__, wapper, wapper->fp);
    if (wapper->call_type == FCFS_PRELOAD_CALL_FASTCFS) {
        return fcfs_ferror(wapper->fp);
    } else if (wapper->call_type == FCFS_PRELOAD_CALL_SYSTEM) {
        if (g_fcfs_preload_global_vars.ferror == NULL) {
            g_fcfs_preload_global_vars.ferror = fcfs_dlsym1("ferror");
        }
        return g_fcfs_preload_global_vars.ferror(wapper->fp);
    } else {
        errno = EBADF;
        return EOF;
    }
}

int _IO_ferror(FILE *fp)
{
    return _ferror_(fp);
}

int fileno(FILE *fp)
{
    FCFSPreloadFILEWrapper *wapper;

    CHECK_DEAL_STD_STREAM(fileno, fp);
    wapper = (FCFSPreloadFILEWrapper *)fp;
    FCFS_LOG_DEBUG("func: %s, line: %d, wapper: %p, fp: %p\n",
            __FUNCTION__, __LINE__, wapper, wapper->fp);
    if (wapper->call_type == FCFS_PRELOAD_CALL_FASTCFS) {
        return fcfs_fileno(wapper->fp);
    } else if (wapper->call_type == FCFS_PRELOAD_CALL_SYSTEM) {
        if (g_fcfs_preload_global_vars.fileno == NULL) {
            g_fcfs_preload_global_vars.fileno = fcfs_dlsym1("fileno");
        }
        return g_fcfs_preload_global_vars.fileno(wapper->fp);
    } else {
        errno = EBADF;
        return EOF;
    }
}

int fgetc(FILE *fp)
{
    FCFSPreloadFILEWrapper *wapper;

    CHECK_DEAL_STD_STREAM(fgetc, fp);
    wapper = (FCFSPreloadFILEWrapper *)fp;
    FCFS_LOG_DEBUG("func: %s, line: %d, wapper: %p, fp: %p\n",
            __FUNCTION__, __LINE__, wapper, wapper->fp);
    if (wapper->call_type == FCFS_PRELOAD_CALL_FASTCFS) {
        return fcfs_fgetc(wapper->fp);
    } else if (wapper->call_type == FCFS_PRELOAD_CALL_SYSTEM) {
        if (g_fcfs_preload_global_vars.fgetc == NULL) {
            g_fcfs_preload_global_vars.fgetc = fcfs_dlsym1("fgetc");
        }
        return g_fcfs_preload_global_vars.fgetc(wapper->fp);
    } else {
        errno = EBADF;
        return EOF;
    }
}

char *fgets(char *s, int size, FILE *fp)
{
    FCFSPreloadFILEWrapper *wapper;

    CHECK_DEAL_STD_STREAM(fgets, s, size, fp);
    wapper = (FCFSPreloadFILEWrapper *)fp;
    FCFS_LOG_DEBUG("func: %s, line: %d, wapper: %p, fp: %p\n",
            __FUNCTION__, __LINE__, wapper, wapper->fp);
    if (wapper->call_type == FCFS_PRELOAD_CALL_FASTCFS) {
        return fcfs_fgets(s, size, wapper->fp);
    } else if (wapper->call_type == FCFS_PRELOAD_CALL_SYSTEM) {
        if (g_fcfs_preload_global_vars.fgets == NULL) {
            g_fcfs_preload_global_vars.fgets = fcfs_dlsym1("fgets");
        }
        return g_fcfs_preload_global_vars.fgets(s, size, wapper->fp);
    } else {
        errno = EBADF;
        return NULL;
    }
}

#ifdef getc
#undef getc
#endif

int getc(FILE *fp)
{
    FCFSPreloadFILEWrapper *wapper;

    CHECK_DEAL_STD_STREAM(getc, fp);
    wapper = (FCFSPreloadFILEWrapper *)fp;
    FCFS_LOG_DEBUG("func: %s, line: %d, wapper: %p, fp: %p\n",
            __FUNCTION__, __LINE__, wapper, wapper->fp);
    if (wapper->call_type == FCFS_PRELOAD_CALL_FASTCFS) {
        return fcfs_getc(wapper->fp);
    } else if (wapper->call_type == FCFS_PRELOAD_CALL_SYSTEM) {
        if (g_fcfs_preload_global_vars.getc == NULL) {
            g_fcfs_preload_global_vars.getc = fcfs_dlsym1("getc");
        }
        return g_fcfs_preload_global_vars.getc(wapper->fp);
    } else {
        errno = EBADF;
        return EOF;
    }
}

int ungetc(int c, FILE *fp)
{
    FCFSPreloadFILEWrapper *wapper;

    CHECK_DEAL_STD_STREAM(ungetc, c, fp);

    wapper = (FCFSPreloadFILEWrapper *)fp;
    if (wapper->call_type == FCFS_PRELOAD_CALL_FASTCFS) {
        return fcfs_ungetc(c, wapper->fp);
    } else if (wapper->call_type == FCFS_PRELOAD_CALL_SYSTEM) {
        if (g_fcfs_preload_global_vars.ungetc == NULL) {
            g_fcfs_preload_global_vars.ungetc = fcfs_dlsym1("ungetc");
        }
        return g_fcfs_preload_global_vars.ungetc(c, wapper->fp);
    } else {
        errno = EBADF;
        return EOF;
    }
}

int fputc(int c, FILE *fp)
{
    FCFSPreloadFILEWrapper *wapper;

    CHECK_DEAL_STD_STREAM(fputc, c, fp);
    wapper = (FCFSPreloadFILEWrapper *)fp;
    if (wapper->call_type == FCFS_PRELOAD_CALL_FASTCFS) {
        return fcfs_fputc(c, wapper->fp);
    } else if (wapper->call_type == FCFS_PRELOAD_CALL_SYSTEM) {
        if (g_fcfs_preload_global_vars.fputc == NULL) {
            g_fcfs_preload_global_vars.fputc = fcfs_dlsym1("fputc");
        }
        return g_fcfs_preload_global_vars.fputc(c, wapper->fp);
    } else {
        errno = EBADF;
        return EOF;
    }
}

int fputs(const char *s, FILE *fp)
{
    FCFSPreloadFILEWrapper *wapper;

    CHECK_DEAL_STD_STREAM(fputs, s, fp);
    wapper = (FCFSPreloadFILEWrapper *)fp;
    if (wapper->call_type == FCFS_PRELOAD_CALL_FASTCFS) {
        return fcfs_fputs(s, wapper->fp);
    } else if (wapper->call_type == FCFS_PRELOAD_CALL_SYSTEM) {
        if (g_fcfs_preload_global_vars.fputs == NULL) {
            g_fcfs_preload_global_vars.fputs = fcfs_dlsym1("fputs");
        }
        return g_fcfs_preload_global_vars.fputs(s, wapper->fp);
    } else {
        errno = EBADF;
        return EOF;
    }
}

#ifdef putc
#undef putc
#endif

int putc(int c, FILE *fp)
{
    FCFSPreloadFILEWrapper *wapper;

    CHECK_DEAL_STD_STREAM(putc, c, fp);
    wapper = (FCFSPreloadFILEWrapper *)fp;
    if (wapper->call_type == FCFS_PRELOAD_CALL_FASTCFS) {
        return fcfs_putc(c, wapper->fp);
    } else if (wapper->call_type == FCFS_PRELOAD_CALL_SYSTEM) {
        if (g_fcfs_preload_global_vars.putc == NULL) {
            g_fcfs_preload_global_vars.putc = fcfs_dlsym1("putc");
        }
        return g_fcfs_preload_global_vars.putc(c, wapper->fp);
    } else {
        errno = EBADF;
        return EOF;
    }
}

size_t fread(void *buff, size_t size, size_t nmemb, FILE *fp)
{
    FCFSPreloadFILEWrapper *wapper;

    CHECK_DEAL_STD_STREAM(fread, buff, size, nmemb, fp);

    FCFS_LOG_DEBUG("func: %s, line: %d, size: %d, nmemb: %d\n", __FUNCTION__,
            __LINE__, (int)size, (int)nmemb);

    wapper = (FCFSPreloadFILEWrapper *)fp;
    if (wapper->call_type == FCFS_PRELOAD_CALL_FASTCFS) {
        return fcfs_fread(buff, size, nmemb, wapper->fp);
    } else if (wapper->call_type == FCFS_PRELOAD_CALL_SYSTEM) {
        if (g_fcfs_preload_global_vars.fread == NULL) {
            g_fcfs_preload_global_vars.fread = fcfs_dlsym1("fread");
        }
        return g_fcfs_preload_global_vars.fread(buff, size, nmemb, wapper->fp);
    } else {
        errno = EBADF;
        return EOF;
    }
}

size_t fwrite(const void *buff, size_t size, size_t nmemb, FILE *fp)
{
    FCFSPreloadFILEWrapper *wapper;

    CHECK_DEAL_STD_STREAM(fwrite, buff, size, nmemb, fp);

    wapper = (FCFSPreloadFILEWrapper *)fp;
    if (wapper->call_type == FCFS_PRELOAD_CALL_FASTCFS) {
        return fcfs_fwrite(buff, size, nmemb, wapper->fp);
    } else if (wapper->call_type == FCFS_PRELOAD_CALL_SYSTEM) {
        if (g_fcfs_preload_global_vars.fwrite == NULL) {
            g_fcfs_preload_global_vars.fwrite = fcfs_dlsym1("fwrite");
        }
        return g_fcfs_preload_global_vars.fwrite(buff, size, nmemb, wapper->fp);
    } else {
        errno = EBADF;
        return EOF;
    }
}

int vfprintf(FILE *fp, const char *format, va_list ap)
{
    FCFSPreloadFILEWrapper *wapper;

    CHECK_DEAL_STD_STREAM(vfprintf, fp, format, ap);
    wapper = (FCFSPreloadFILEWrapper *)fp;
    if (wapper->call_type == FCFS_PRELOAD_CALL_FASTCFS) {
        return fcfs_vfprintf(wapper->fp, format, ap);
    } else if (wapper->call_type == FCFS_PRELOAD_CALL_SYSTEM) {
        if (g_fcfs_preload_global_vars.vfprintf == NULL) {
            g_fcfs_preload_global_vars.vfprintf = fcfs_dlsym1("vfprintf");
        }
        return g_fcfs_preload_global_vars.vfprintf(wapper->fp, format, ap);
    } else {
        errno = EBADF;
        return EOF;
    }
}

int fprintf(FILE *fp, const char *format, ...)
{
    va_list ap;
    int result;

    va_start(ap, format);
    result = vfprintf(fp, format, ap);
    va_end(ap);

    return result;
}

int __vfprintf_chk(FILE *fp, int flag, const char *format, va_list ap)
{
    FCFSPreloadFILEWrapper *wapper;

    CHECK_DEAL_STD_STREAM(__vfprintf_chk, fp, flag, format, ap);
    wapper = (FCFSPreloadFILEWrapper *)fp;
    if (wapper->call_type == FCFS_PRELOAD_CALL_FASTCFS) {
        return fcfs_vfprintf(wapper->fp, format, ap);
    } else if (wapper->call_type == FCFS_PRELOAD_CALL_SYSTEM) {
        if (g_fcfs_preload_global_vars.__vfprintf_chk == NULL) {
            g_fcfs_preload_global_vars.__vfprintf_chk =
                fcfs_dlsym1("__vfprintf_chk");
        }
        return g_fcfs_preload_global_vars.__vfprintf_chk(
                wapper->fp, flag, format, ap);
    } else {
        errno = EBADF;
        return EOF;
    }
}

int __fprintf_chk(FILE *fp, int flag, const char *format, ...)
{
    va_list ap;
    int result;

    va_start(ap, format);
    result = __vfprintf_chk(fp, flag, format, ap);
    va_end(ap);

    return result;
}

ssize_t getdelim(char **line, size_t *size, int delim, FILE *fp)
{
    FCFSPreloadFILEWrapper *wapper;

    CHECK_DEAL_STD_STREAM(getdelim, line, size, delim, fp);

    wapper = (FCFSPreloadFILEWrapper *)fp;
    FCFS_LOG_DEBUG("func: %s, line: %d, wapper: %p, fp: %p\n",
            __FUNCTION__, __LINE__, wapper, wapper->fp);
    if (wapper->call_type == FCFS_PRELOAD_CALL_FASTCFS) {
        return fcfs_getdelim(line, size, delim, wapper->fp);
    } else if (wapper->call_type == FCFS_PRELOAD_CALL_SYSTEM) {
        if (g_fcfs_preload_global_vars.getdelim == NULL) {
            g_fcfs_preload_global_vars.getdelim = fcfs_dlsym1("getdelim");
        }
        return g_fcfs_preload_global_vars.getdelim(line, size, delim, wapper->fp);
    } else {
        errno = EBADF;
        return EOF;
    }
}

ssize_t __getdelim(char **line, size_t *size, int delim, FILE *fp)
{
    FCFS_LOG_DEBUG("func: %s, line: %d\n", __FUNCTION__, __LINE__);
    return getdelim(line, size, delim, fp);
}

ssize_t getline(char **line, size_t *size, FILE *fp)
{
    FCFSPreloadFILEWrapper *wapper;

    wapper = (FCFSPreloadFILEWrapper *)fp;
    FCFS_LOG_DEBUG("func: %s, line: %d, wapper: %p, fp: %p\n",
            __FUNCTION__, __LINE__, wapper, wapper->fp);
    if (wapper->call_type == FCFS_PRELOAD_CALL_FASTCFS) {
        return fcfs_getline(line, size, wapper->fp);
    } else if (wapper->call_type == FCFS_PRELOAD_CALL_SYSTEM) {
        if (g_fcfs_preload_global_vars.getline == NULL) {
            g_fcfs_preload_global_vars.getline = fcfs_dlsym1("getline");
        }
        return g_fcfs_preload_global_vars.getline(line, size, wapper->fp);
    } else {
        errno = EBADF;
        return EOF;
    }
}

ssize_t _IO_getline(char **line, size_t *size, FILE *fp)
{
    return getline(line, size, fp);
}

int vfscanf(FILE *fp, const char *format, va_list ap)
{
    FCFSPreloadFILEWrapper *wapper;

    CHECK_DEAL_STD_STREAM(vfscanf, fp, format, ap);

    wapper = (FCFSPreloadFILEWrapper *)fp;
    if (wapper->call_type == FCFS_PRELOAD_CALL_FASTCFS) {
        //TODO
        errno = EOPNOTSUPP;
        return EOF;
        //return fcfs_vfscanf(wapper->fp, format, ap);
    } else if (wapper->call_type == FCFS_PRELOAD_CALL_SYSTEM) {
        if (g_fcfs_preload_global_vars.vfscanf == NULL) {
            g_fcfs_preload_global_vars.vfscanf = fcfs_dlsym1("vfscanf");
        }
        return g_fcfs_preload_global_vars.vfscanf(wapper->fp, format, ap);
    } else {
        errno = EBADF;
        return EOF;
    }
}

int fscanf(FILE *fp, const char *format, ...)
{
    va_list ap;
    int count;

    va_start(ap, format);
    count = vfscanf(fp, format, ap);
    va_end(ap);

    return count;
}

int setvbuf(FILE *fp, char *buf, int mode, size_t size)
{
    FCFSPreloadFILEWrapper *wapper;

    CHECK_DEAL_STD_STREAM(setvbuf, fp, buf, mode, size);
    wapper = (FCFSPreloadFILEWrapper *)fp;
    if (wapper->call_type == FCFS_PRELOAD_CALL_FASTCFS) {
        return fcfs_setvbuf(wapper->fp, buf, mode, size);
    } else if (wapper->call_type == FCFS_PRELOAD_CALL_SYSTEM) {
        if (g_fcfs_preload_global_vars.setvbuf == NULL) {
            g_fcfs_preload_global_vars.setvbuf = fcfs_dlsym1("setvbuf");
        }
        return g_fcfs_preload_global_vars.setvbuf(wapper->fp, buf, mode, size);
    } else {
        errno = EBADF;
        return EOF;
    }
}


int _IO_setvbuf(FILE *fp, char *buf, int mode, size_t size)
{
    return setvbuf(fp, buf, mode, size);
}

void setbuf(FILE *fp, char *buf)
{
    FCFSPreloadFILEWrapper *wapper;

    CHECK_DEAL_STD_STREAM(setbuf, fp, buf);
    wapper = (FCFSPreloadFILEWrapper *)fp;
    if (wapper->call_type == FCFS_PRELOAD_CALL_FASTCFS) {
        fcfs_setbuf(wapper->fp, buf);
    } else if (wapper->call_type == FCFS_PRELOAD_CALL_SYSTEM) {
        if (g_fcfs_preload_global_vars.setbuf == NULL) {
            g_fcfs_preload_global_vars.setbuf = fcfs_dlsym1("setbuf");
        }
        g_fcfs_preload_global_vars.setbuf(wapper->fp, buf);
    }
}

void setbuffer(FILE *fp, char *buf, size_t size)
{
    FCFSPreloadFILEWrapper *wapper;

    CHECK_DEAL_STD_STREAM(setbuffer, fp, buf, size);
    wapper = (FCFSPreloadFILEWrapper *)fp;
    if (wapper->call_type == FCFS_PRELOAD_CALL_FASTCFS) {
        fcfs_setbuffer(wapper->fp, buf, size);
    } else if (wapper->call_type == FCFS_PRELOAD_CALL_SYSTEM) {
        if (g_fcfs_preload_global_vars.setbuffer == NULL) {
            g_fcfs_preload_global_vars.setbuffer = fcfs_dlsym1("setbuffer");
        }
        g_fcfs_preload_global_vars.setbuffer(wapper->fp, buf, size);
    }
}

void _IO_setbuffer(FILE *fp, char *buf, size_t size)
{
    setbuffer(fp, buf, size);
}

void setlinebuf(FILE *fp)
{
    FCFSPreloadFILEWrapper *wapper;

    CHECK_DEAL_STD_STREAM(setlinebuf, fp);
    wapper = (FCFSPreloadFILEWrapper *)fp;
    if (wapper->call_type == FCFS_PRELOAD_CALL_FASTCFS) {
        fcfs_setlinebuf(wapper->fp);
    } else if (wapper->call_type == FCFS_PRELOAD_CALL_SYSTEM) {
        if (g_fcfs_preload_global_vars.setlinebuf == NULL) {
            g_fcfs_preload_global_vars.setlinebuf = fcfs_dlsym1("setlinebuf");
        }
        g_fcfs_preload_global_vars.setlinebuf(wapper->fp);
    }
}

int fflush(FILE *fp)
{
    FCFSPreloadFILEWrapper *wapper;

    if (fp == NULL)  {
        return g_fcfs_preload_global_vars.fflush(fp);
    }

    CHECK_DEAL_STD_STREAM(fflush, fp);
    wapper = (FCFSPreloadFILEWrapper *)fp;
    if (wapper->call_type == FCFS_PRELOAD_CALL_FASTCFS) {
        return fcfs_fflush(wapper->fp);
    } else if (wapper->call_type == FCFS_PRELOAD_CALL_SYSTEM) {
        if (g_fcfs_preload_global_vars.fflush == NULL) {
            g_fcfs_preload_global_vars.fflush = fcfs_dlsym1("fflush");
        }
        return g_fcfs_preload_global_vars.fflush(wapper->fp);
    } else {
        errno = EBADF;
        return EOF;
    }
}

int _IO_fflush(FILE *fp)
{
    return fflush(fp);
}

int __uflow(FILE *fp)
{
    FCFSPreloadFILEWrapper *wapper;

    CHECK_DEAL_STD_STREAM(__uflow, fp);
    wapper = (FCFSPreloadFILEWrapper *)fp;
    if (wapper->call_type == FCFS_PRELOAD_CALL_FASTCFS) {
        return 0;
    } else if (wapper->call_type == FCFS_PRELOAD_CALL_SYSTEM) {
        if (g_fcfs_preload_global_vars.__uflow == NULL) {
            g_fcfs_preload_global_vars.__uflow = fcfs_dlsym1("__uflow");
        }
        return g_fcfs_preload_global_vars.__uflow(wapper->fp);
    } else {
        errno = EBADF;
        return EOF;
    }
}

int __overflow(FILE *fp, int ch)
{
    FCFSPreloadFILEWrapper *wapper;

    CHECK_DEAL_STD_STREAM(__overflow, fp, ch);
    wapper = (FCFSPreloadFILEWrapper *)fp;
    if (wapper->call_type == FCFS_PRELOAD_CALL_FASTCFS) {
        return 0;
    } else if (wapper->call_type == FCFS_PRELOAD_CALL_SYSTEM) {
        if (g_fcfs_preload_global_vars.__overflow == NULL) {
            g_fcfs_preload_global_vars.__overflow = fcfs_dlsym1("__overflow");
        }
        return g_fcfs_preload_global_vars.__overflow(wapper->fp, ch);
    } else {
        errno = EBADF;
        return EOF;
    }
}
#endif
