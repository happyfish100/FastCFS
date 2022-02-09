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


#ifndef _FCFS_PRELOAD_API_H
#define _FCFS_PRELOAD_API_H

#include "types.h"

#define FCFS_PRELOAD_IS_MY_MOUNTPOINT(path) \
    FCFS_API_IS_MY_MOUNTPOINT(path)

#define FCFS_PRELOAD_IS_MY_FD_MOUNTPOINT(fd, path) \
    (FCFS_PAPI_IS_MY_FD(fd) || FCFS_PRELOAD_IS_MY_MOUNTPOINT(path))

#ifdef __cplusplus
extern "C" {
#endif

int closedir(DIR *dirp);

struct dirent *readdir(DIR *dirp);

struct dirent64 *readdir64(DIR *dirp);

int readdir_r(DIR *dirp, struct dirent *entry, struct dirent **result);

int readdir64_r(DIR *dirp, struct dirent64 *entry, struct dirent64 **result);

void seekdir(DIR *dirp, long loc);

long telldir(DIR *dirp);

void rewinddir(DIR *dirp);

int dirfd(DIR *dirp);

int close(int fd);

int fsync(int fd);

int fdatasync(int fd);

ssize_t write(int fd, const void *buff, size_t count);

ssize_t pwrite(int fd, const void *buff, size_t count, off_t offset);

ssize_t writev(int fd, const struct iovec *iov, int iovcnt);

ssize_t pwritev(int fd, const struct iovec *iov, int iovcnt, off_t offset);

ssize_t read(int fd, void *buff, size_t count);

ssize_t pread(int fd, void *buff, size_t count, off_t offset);

ssize_t readv(int fd, const struct iovec *iov, int iovcnt);

ssize_t preadv(int fd, const struct iovec *iov, int iovcnt, off_t offset);

off_t lseek(int fd, off_t offset, int whence);

int fallocate(int fd, int mode, off_t offset, off_t length);

int ftruncate(int fd, off_t length);

int fstat(int fd, struct stat *buf);

int flock(int fd, int operation);

int fcntl(int fd, int cmd, ...);

int futimes(int fd, const struct timeval times[2]);

int futimens(int fd, const struct timespec times[2]);

int fchown(int fd, uid_t owner, gid_t group);

int fchmod(int fd, mode_t mode);

int fsetxattr(int fd, const char *name, const void *value,
        size_t size, int flags);

ssize_t fgetxattr(int fd, const char *name, void *value, size_t size);

ssize_t flistxattr(int fd, char *list, size_t size);

int fremovexattr(int fd, const char *name);

int fchdir(int fd);

int fstatvfs(int fd, struct statvfs *buf);

int dup(int fd);

int dup2(int fd1, int fd2);

void *mmap(void *addr, size_t length, int prot,
        int flags, int fd, off_t offset);

DIR *fdopendir(int fd);

int symlinkat(const char *link, int fd, const char *path);

int openat(int fd, const char *path, int flags, ...);

int fstatat(int fd, const char *path, struct stat *buf, int flags);

ssize_t readlinkat(int fd, const char *path, char *buff, size_t size);

int mknodat(int fd, const char *path, mode_t mode, dev_t dev);

int mkfifoat(int fd, const char *path, mode_t mode);

int faccessat(int fd, const char *path, int mode, int flags);

int futimesat(int fd, const char *path, const struct timeval times[2]);

int utimensat(int fd, const char *path, const
        struct timespec times[2], int flags);

int unlinkat(int fd, const char *path, int flags);

int mkdirat(int fd, const char *path, mode_t mode);

int fchownat(int fd, const char *path, uid_t owner, gid_t group, int flags);

int fchmodat(int fd, const char *path, mode_t mode, int flags);

int linkat(int fd1, const char *path1, int fd2, const char *path2, int flags);

int renameat(int fd1, const char *path1, int fd2, const char *path2);

int renameat2(int fd1, const char *path1, int fd2,
        const char *path2, unsigned int flags);

int scandirat(int fd, const char *path, struct dirent ***namelist,
        fcfs_dir_filter_func filter, fcfs_dir_compare_func compar);

int scandirat64(int fd, const char *path, struct dirent ***namelist,
        fcfs_dir_filter_func filter, fcfs_dir_compare_func compar);

char *getcwd(char *buf, size_t size);

char *getwd(char *buf);

int open(const char *path, int flags, ...);

int creat(const char *path, mode_t mode);

int truncate(const char *path, off_t length);

int lstat(const char *path, struct stat *buf);

int stat(const char *path, struct stat *buf);

int link(const char *path1, const char *path2);

int symlink(const char *link, const char *path);

ssize_t readlink(const char *path, char *buff, size_t size);

int mknod(const char *path, mode_t mode, dev_t dev);

int mkfifo(const char *path, mode_t mode);

int access(const char *path, int mode);

int utime(const char *path, const struct utimbuf *times);

int utimes(const char *path, const struct timeval times[2]);

int unlink(const char *path);

int rename(const char *path1, const char *path2);

int mkdir(const char *path, mode_t mode);

int rmdir(const char *path);

int chown(const char *path, uid_t owner, gid_t group);

int lchown(const char *path, uid_t owner, gid_t group);

int chmod(const char *path, mode_t mode);

int statvfs(const char *path, struct statvfs *buf);

int setxattr(const char *path, const char *name,
        const void *value, size_t size, int flags);

int lsetxattr(const char *path, const char *name,
        const void *value, size_t size, int flags);

ssize_t getxattr(const char *path, const char *name,
        void *value, size_t size);

ssize_t lgetxattr(const char *path, const char *name,
        void *value, size_t size);

ssize_t listxattr(const char *path, char *list, size_t size);

ssize_t llistxattr(const char *path, char *list, size_t size);

int removexattr(const char *path, const char *name);

int lremovexattr(const char *path, const char *name);

int chdir(const char *path);

int chroot(const char *path);

DIR *opendir(const char *path);

int scandir(const char *path, struct dirent ***namelist,
        fcfs_dir_filter_func filter, fcfs_dir_compare_func compar);

int scandir64(const char *path, struct dirent ***namelist,
        fcfs_dir_filter_func filter, fcfs_dir_compare_func compar);

#ifdef __cplusplus
}
#endif

#endif