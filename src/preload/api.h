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

#ifdef fread_unlocked
#undef fread_unlocked
#endif

#ifdef fwrite_unlocked
#undef fwrite_unlocked
#endif

#ifdef __cplusplus
extern "C" {
#endif

int closedir(DIR *dirp);

struct dirent *_readdir_(DIR *dirp) __asm__ ("" "readdir");

struct dirent64 *readdir64(DIR *dirp);

int _readdir_r_(DIR *dirp, struct dirent *entry, struct dirent **result)
     __asm__ ("" "readdir_r");

int readdir64_r(DIR *dirp, struct dirent64 *entry, struct dirent64 **result);

void seekdir(DIR *dirp, long loc);

long telldir(DIR *dirp);

void rewinddir(DIR *dirp);

int dirfd(DIR *dirp);

int close(int fd);

int fsync(int fd);

int fdatasync(int fd);

ssize_t write(int fd, const void *buff, size_t count);

ssize_t _pwrite_(int fd, const void *buff, size_t count, off_t offset)
    __asm__ ("" "pwrite");

ssize_t pwrite64(int fd, const void *buff, size_t count, off_t offset);

ssize_t writev(int fd, const struct iovec *iov, int iovcnt);

ssize_t _pwritev_(int fd, const struct iovec *iov, int iovcnt, off_t offset)
    __asm__ ("" "pwritev");

ssize_t pwritev64(int fd, const struct iovec *iov, int iovcnt, off_t offset);

ssize_t read(int fd, void *buff, size_t count);

ssize_t __read_chk(int fd, void *buff, size_t count, size_t size);

ssize_t _pread_(int fd, void *buff, size_t count, off_t offset)
    __asm__ ("" "pread");

ssize_t pread64(int fd, void *buff, size_t count, off_t offset);

ssize_t __pread_chk(int fd, void *buff, size_t count,
        off_t offset, size_t size);

ssize_t __pread64_chk(int fd, void *buff, size_t count,
        off_t offset, size_t size);

ssize_t readv(int fd, const struct iovec *iov, int iovcnt);

ssize_t _preadv_(int fd, const struct iovec *iov, int iovcnt, off_t offset)
    __asm__ ("" "preadv");

ssize_t preadv64(int fd, const struct iovec *iov, int iovcnt, off_t offset);

ssize_t readahead(int fd, off64_t offset, size_t count);

off_t _lseek_(int fd, off_t offset, int whence) __asm__ ("" "lseek");

off_t __lseek(int fd, off_t offset, int whence);

off_t lseek64(int fd, off_t offset, int whence);

int _fallocate_(int fd, int mode, off_t offset, off_t length)
    __asm__ ("" "fallocate");

int fallocate64(int fd, int mode, off_t offset, off_t length);

int _ftruncate_(int fd, off_t length) __asm__ ("" "ftruncate");

int ftruncate64(int fd, off_t length);

int __fxstat_(int ver, int fd, struct stat *buf) __asm__ ("" "__fxstat");

int __fxstat64(int ver, int fd, struct stat64 *buf);

int fstat(int fd, struct stat *buf);

int flock(int fd, int operation);

int _fcntl_(int fd, int cmd, ...)  __asm__ ("" "fcntl");

int fcntl64(int fd, int cmd, ...);

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

int _fstatvfs_(int fd, struct statvfs *buf) __asm__ ("" "fstatvfs");

int fstatvfs64(int fd, struct statvfs64 *buf);

int dup(int fd);

int dup2(int fd1, int fd2);

void *_mmap_(void *addr, size_t length, int prot, int flags,
        int fd, off_t offset) __asm__ ("" "mmap");

void *mmap64(void *addr, size_t length, int prot,
        int flags, int fd, off_t offset);

DIR *fdopendir(int fd);

int symlinkat(const char *link, int fd, const char *path);

int _openat_(int fd, const char *path, int flags, ...) __asm__ ("" "openat");

int openat64(int fd, const char *path, int flags, ...);

int __openat_2_(int fd, const char *path, int flags) __asm__ ("" "__openat_2");

int __openat64_2(int fd, const char *path, int flags);

int __fxstatat_(int ver, int fd, const char *path, struct stat *buf,
        int flags) __asm__ ("" "__fxstatat");

int __fxstatat64(int ver, int fd, const char *path,
        struct stat64 *buf, int flags);

int fstatat(int fd, const char *path, struct stat *buf, int flags);

ssize_t readlinkat(int fd, const char *path, char *buff, size_t size);

int __xmknodat(int ver, int fd, const char *path, mode_t mode, dev_t *dev);

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

int _scandirat_(int fd, const char *path, struct dirent ***namelist,
        fcfs_dir_filter_func filter, fcfs_dir_compare_func compar)
        __asm__ ("" "scandirat");

int scandirat64(int fd, const char *path, struct dirent64 ***namelist,
        int (*filter) (const struct dirent64 *),
        int (*compar) (const struct dirent64 **,
            const struct dirent64 **));

char *getcwd(char *buf, size_t size);

char *getwd(char *buf);

int _open_(const char *path, int flags, ...) __asm__ ("" "open");

int open64(const char *path, int flags, ...);

int __open(const char *path, int flags, int mode);

int __open_2_(const char *path, int flags) __asm__ ("" "__open_2");

int __open64_2_(const char *path, int flags);

int _creat_(const char *path, mode_t mode) __asm__ ("" "creat");

int creat64(const char *path, mode_t mode);

int _truncate_(const char *path, off_t length) __asm__ ("" "truncate");

int truncate64(const char *path, off_t length);

int __lxstat_(int ver, const char *path, struct stat *buf)
    __asm__ ("" "__lxstat");

int __lxstat64(int ver, const char *path, struct stat64 *buf);

int lstat(const char *path, struct stat *buf);

int __xstat_(int ver, const char *path, struct stat *buf)
     __asm__ ("" "__xstat");

int __xstat64(int ver, const char *path, struct stat64 *buf);

int stat(const char *path, struct stat *buf);

int link(const char *path1, const char *path2);

int symlink(const char *link, const char *path);

ssize_t readlink(const char *path, char *buff, size_t size);

int __xmknod(int ver, const char *path, mode_t mode, dev_t *dev);

int mknod(const char *path, mode_t mode, dev_t dev);

int mkfifo(const char *path, mode_t mode);

int access(const char *path, int mode);

int euidaccess(const char *path, int mode);

int eaccess(const char *path, int mode);

int utime(const char *path, const struct utimbuf *times);

int utimes(const char *path, const struct timeval times[2]);

int unlink(const char *path);

int rename(const char *path1, const char *path2);

int mkdir(const char *path, mode_t mode);

int rmdir(const char *path);

int chown(const char *path, uid_t owner, gid_t group);

int lchown(const char *path, uid_t owner, gid_t group);

int chmod(const char *path, mode_t mode);

int _statvfs_(const char *path, struct statvfs *buf) __asm__ ("" "statvfs");

int statvfs64(const char *path, struct statvfs64 *buf);

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

int unsetenv(const char *name);

int clearenv(void);

DIR *opendir(const char *path);

int _scandir_(const char *path, struct dirent ***namelist,
        fcfs_dir_filter_func filter, fcfs_dir_compare_func compar)
    __asm__ ("" "scandir");

int scandir64(const char *path, struct dirent64 ***namelist,
        int (*filter) (const struct dirent64 *),
        int (*compar) (const struct dirent64 **,
            const struct dirent64 **));

int dprintf(int fd, const char *format, ...);

int vdprintf(int fd, const char *format, va_list ap);

int _lockf_(int fd, int cmd, off_t len) __asm__ ("" "lockf");

int lockf64(int fd, int cmd, off_t len);

int _posix_fallocate_(int fd, off_t offset, off_t len)
    __asm__ ("" "posix_fallocate");

int posix_fallocate64(int fd, off_t offset, off_t len);

int _posix_fadvise_(int fd, off_t offset, off_t len, int advice)
    __asm__ ("" "posix_fadvise");

int posix_fadvise64(int fd, off_t offset, off_t len, int advice);

FILE *_fopen_(const char *path, const char *mode) __asm__ ("" "fopen");

FILE *fopen64(const char *path, const char *mode);

FILE *fdopen(int fd, const char *mode);

FILE *_freopen_(const char *path, const char *mode, FILE *fp)
    __asm__ ("" "freopen");

FILE *freopen64(const char *path, const char *mode, FILE *fp);

int fclose(FILE *fp);

int fcloseall();

void flockfile(FILE *fp);

int ftrylockfile(FILE *fp);

void funlockfile(FILE *fp);

int fseek(FILE *fp, long offset, int whence);

int _fseeko_(FILE *fp, off_t offset, int whence) __asm__ ("" "fseeko");

int fseeko64(FILE *fp, off_t offset, int whence);

long ftell(FILE *fp);

off_t _ftello_(FILE *fp) __asm__ ("" "ftello");

off_t ftello64(FILE *fp);

void rewind(FILE *fp);

int _fgetpos_(FILE *fp, fpos_t *pos) __asm__ ("" "fgetpos");

int fgetpos64(FILE *fp, fpos_t *pos);

int _fsetpos_(FILE *fp, const fpos_t *pos) __asm__ ("" "fsetpos");

int fsetpos64(FILE *fp, const fpos_t *pos);

int fgetc_unlocked(FILE *fp);

int fputc_unlocked(int c, FILE *fp);

int getc_unlocked(FILE *fp);

int putc_unlocked(int c, FILE *fp);

void clearerr_unlocked(FILE *fp);

int feof_unlocked(FILE *fp);

int ferror_unlocked(FILE *fp);

int fileno_unlocked(FILE *fp);

int fflush_unlocked(FILE *fp);

size_t fread_unlocked(void *buff, size_t size, size_t n, FILE *fp);

size_t fwrite_unlocked(const void *buff, size_t size, size_t n, FILE *fp);

char *fgets_unlocked(char *s, int size, FILE *fp);

int fputs_unlocked(const char *s, FILE *fp);

void clearerr(FILE *fp);

int _feof_(FILE *fp) __asm__ ("" "feof");

int _ferror_(FILE *fp) __asm__ ("" "ferror");

int fileno(FILE *fp);

int fgetc(FILE *fp);

char *fgets(char *s, int size, FILE *fp);

int getc(FILE *fp);

int ungetc(int c, FILE *fp);

int fputc(int c, FILE *fp);

int fputs(const char *s, FILE *fp);

int putc(int c, FILE *fp);

size_t fread(void *buff, size_t size, size_t nmemb, FILE *fp);

size_t fwrite(const void *buff, size_t size, size_t nmemb, FILE *fp);

int fprintf(FILE *fp, const char *format, ...);

int vfprintf(FILE *fp, const char *format, va_list ap);

int __fprintf_chk (FILE *fp, int flag, const char *format, ...);

int __vfprintf_chk (FILE *fp, int flag, const char *format, va_list ap);

ssize_t getdelim(char **line, size_t *size, int delim, FILE *fp);

ssize_t getline(char **line, size_t *size, FILE *fp);

int fscanf(FILE *fp, const char *format, ...);

int vfscanf(FILE *fp, const char *format, va_list ap);

int setvbuf(FILE *fp, char *buf, int mode, size_t size);

void setbuf(FILE *fp, char *buf);

void setbuffer(FILE *fp, char *buf, size_t size);

void setlinebuf(FILE *fp);

int fflush(FILE *fp);

long syscall(long number, ...);


#ifdef __cplusplus
}
#endif

#endif
