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

#ifndef _FCFS_PRELOAD_TYPES_H
#define _FCFS_PRELOAD_TYPES_H

#include "fastcfs/api/std/posix_api.h"

#ifdef OS_LINUX
typedef struct __dirstream DIR;
#endif

typedef enum {
    fcfs_preload_call_system  = 0,
    fcfs_preload_call_fastcfs = 1
} FCFSPreloadCallType;

typedef struct fcfs_preload_dir_wrapper {
    FCFSPreloadCallType call_type;
    DIR *dirp;
} FCFSPreloadDIRWrapper;

typedef struct fcfs_preload_global_vars {
    bool inited;
    FCFSPreloadCallType cwd_call_type;

    struct {
        int (*unsetenv)(const char *name);

        int (*clearenv)(void);

        FILE *(*fopen)(const char *pathname, const char *mode);

        size_t (*fread)(void *ptr, size_t size, size_t nmemb, FILE *fp);

        size_t (*fwrite)(const void *ptr, size_t size, size_t nmemb, FILE *fp);

        int (*open)(const char *path, int flags, ...);

        int (*openat)(int fd, const char *path, int flags, ...);

        int (*creat)(const char *path, mode_t mode);

        int (*close)(int fd);

        int (*fsync)(int fd);

        int (*fdatasync)(int fd);

        ssize_t (*write)(int fd, const void *buff, size_t count);

        ssize_t (*pwrite)(int fd, const void *buff, size_t count, off_t offset);

        ssize_t (*writev)(int fd, const struct iovec *iov, int iovcnt);

        ssize_t (*pwritev)(int fd, const struct iovec *iov,
                int iovcnt, off_t offset);

        ssize_t (*read)(int fd, void *buff, size_t count);

        ssize_t (*__read_chk)(int fd, void *buff, size_t count, size_t size);

        ssize_t (*pread)(int fd, void *buff, size_t count, off_t offset);

        ssize_t (*readv)(int fd, const struct iovec *iov, int iovcnt);

        ssize_t (*preadv)(int fd, const struct iovec *iov,
                int iovcnt, off_t offset);

        off_t (*lseek)(int fd, off_t offset, int whence);

        int (*fallocate)(int fd, int mode, off_t offset, off_t length);

        int (*truncate)(const char *path, off_t length);

        int (*ftruncate)(int fd, off_t length);

        union {
            struct {
                int (*stat)(const char *path, struct stat *buf);

                int (*lstat)(const char *path, struct stat *buf);

                int (*fstat)(int fd, struct stat *buf);

                int (*fstatat)(int fd, const char *path,
                        struct stat *buf, int flags);
            };

            struct {
                int (*__xstat)(int ver, const char *path, struct stat *buf);

                int (*__lxstat)(int ver, const char *path, struct stat *buf);

                int (*__fxstat)(int ver, int fd, struct stat *buf);

                int (*__fxstatat)(int ver, int fd, const char *path,
                        struct stat *buf, int flags);
            };
        };

        int (*flock)(int fd, int operation);

        int (*fcntl)(int fd, int cmd, ...);

        int (*symlink)(const char *link, const char *path);

        int (*symlinkat)(const char *link, int fd, const char *path);

        int (*link)(const char *path1, const char *path2);

        int (*linkat)(int fd1, const char *path1, int fd2,
                const char *path2, int flags);

        ssize_t (*readlink)(const char *path, char *buff, size_t size);

        ssize_t (*readlinkat)(int fd, const char *path,
                char *buff, size_t size);

        int (*mknod)(const char *path, mode_t mode, dev_t dev);

        int (*mknodat)(int fd, const char *path, mode_t mode, dev_t dev);

        int (*mkfifo)(const char *path, mode_t mode);

        int (*mkfifoat)(int fd, const char *path, mode_t mode);

        int (*access)(const char *path, int mode);

        int (*faccessat)(int fd, const char *path, int mode, int flags);

        int (*euidaccess)(const char *path, int mode);

        int (*eaccess)(const char *path, int mode);

        int (*utime)(const char *path, const struct utimbuf *times);

        int (*utimes)(const char *path, const struct timeval times[2]);

        int (*futimes)(int fd, const struct timeval times[2]);

        int (*futimesat)(int fd, const char *path,
                const struct timeval times[2]);

        int (*futimens)(int fd, const struct timespec times[2]);

        int (*utimensat)(int fd, const char *path, const
                struct timespec times[2], int flags);

        int (*unlink)(const char *path);

        int (*unlinkat)(int fd, const char *path, int flags);

        int (*rename)(const char *path1, const char *path2);

        int (*renameat)(int fd1, const char *path1, int fd2, const char *path2);

        int (*renameat2)(int fd1, const char *path1, int fd2,
                const char *path2, unsigned int flags);

        int (*mkdir)(const char *path, mode_t mode);

        int (*mkdirat)(int fd, const char *path, mode_t mode);

        int (*rmdir)(const char *path);

        int (*chown)(const char *path, uid_t owner, gid_t group);

        int (*lchown)(const char *path, uid_t owner, gid_t group);

        int (*fchown)(int fd, uid_t owner, gid_t group);

        int (*fchownat)(int fd, const char *path,
                uid_t owner, gid_t group, int flags);

        int (*chmod)(const char *path, mode_t mode);

        int (*fchmod)(int fd, mode_t mode);

        int (*fchmodat)(int fd, const char *path, mode_t mode, int flags);

        int (*statvfs)(const char *path, struct statvfs *buf);

        int (*fstatvfs)(int fd, struct statvfs *buf);

        int (*setxattr)(const char *path, const char *name,
                const void *value, size_t size, int flags);

        int (*lsetxattr)(const char *path, const char *name,
                const void *value, size_t size, int flags);

        int (*fsetxattr)(int fd, const char *name,
                const void *value, size_t size, int flags);

        ssize_t (*getxattr)(const char *path, const char *name,
                void *value, size_t size);

        ssize_t (*lgetxattr)(const char *path, const char *name,
                void *value, size_t size);

        ssize_t (*fgetxattr)(int fd, const char *name,
                void *value, size_t size);

        ssize_t (*listxattr)(const char *path, char *list, size_t size);

        ssize_t (*llistxattr)(const char *path, char *list, size_t size);

        ssize_t (*flistxattr)(int fd, char *list, size_t size);

        int (*removexattr)(const char *path, const char *name);

        int (*lremovexattr)(const char *path, const char *name);

        int (*fremovexattr)(int fd, const char *name);

        int (*lockf)(int fd, int cmd, off_t len);

        int (*posix_fallocate)(int fd, off_t offset, off_t len);

        int (*posix_fadvise)(int fd, off_t offset, off_t len, int advice);

        int (*dprintf)(int fd, const char *format, ...);

        int (*vdprintf)(int fd, const char *format, va_list ap);

        DIR *(*opendir)(const char *path);

        DIR *(*fdopendir)(int fd);

        int (*closedir)(DIR *dirp);

        struct dirent *(*readdir)(DIR *dirp);

        int (*readdir_r)(DIR *dirp, struct dirent *entry,
                struct dirent **result);

        void (*seekdir)(DIR *dirp, long loc);

        long (*telldir)(DIR *dirp);

        void (*rewinddir)(DIR *dirp);

        int (*dirfd)(DIR *dirp);

        int (*scandir)(const char *path, struct dirent ***namelist,
                fcfs_dir_filter_func filter, fcfs_dir_compare_func compar);

        int (*scandirat)(int fd, const char *path, struct dirent ***namelist,
                fcfs_dir_filter_func filter, fcfs_dir_compare_func compar);

        int (*scandir64)(const char *path, struct dirent ***namelist,
                fcfs_dir_filter_func filter, fcfs_dir_compare_func compar);

        int (*scandirat64)(int fd, const char *path, struct dirent ***namelist,
                fcfs_dir_filter_func filter, fcfs_dir_compare_func compar);

        int (*chdir)(const char *path);

        int (*fchdir)(int fd);

        char *(*getcwd)(char *buf, size_t size);

        char *(*getwd)(char *buf);

        int (*chroot)(const char *path);

        int (*dup)(int fd);

        int (*dup2)(int fd1, int fd2);

        void *(*mmap)(void *addr, size_t length, int prot,
                int flags, int fd, off_t offset);

        int (*munmap)(void *addr, size_t length);
    };
} FCFSPreloadGlobalVars;

#endif
