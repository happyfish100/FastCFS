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

#define FCFS_PRELOAD_CALL_SYSTEM   1645611685
#define FCFS_PRELOAD_CALL_FASTCFS  1645611708

typedef struct fcfs_preload_dir_wrapper {
    int call_type;
    DIR *dirp;
} FCFSPreloadDIRWrapper;

typedef struct fcfs_preload_file_wrapper {
    int call_type;
    FILE *fp;
} FCFSPreloadFILEWrapper;

typedef struct fcfs_preload_global_vars {
    bool inited;
    int cwd_call_type;

    struct {
        int (*unsetenv)(const char *name);

        int (*clearenv)(void);

        struct {
            struct {
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

        int (*__xmknod)(int ver, const char *path,
                mode_t mode, dev_t *dev);

        int (*__xmknodat)(int ver, int fd, const char *path,
                mode_t mode, dev_t *dev);

        int (*mkfifo)(const char *path, mode_t mode);

        int (*mkfifoat)(int fd, const char *path, mode_t mode);

        int (*euidaccess)(const char *path, int mode);

        int (*eaccess)(const char *path, int mode);

        int (*futimes)(int fd, const struct timeval times[2]);

        int (*futimens)(int fd, const struct timespec times[2]);

        int (*statvfs)(const char *path, struct statvfs *buf);

        int (*fstatvfs)(int fd, struct statvfs *buf);

        int (*lockf)(int fd, int cmd, off_t len);

        int (*posix_fallocate)(int fd, off_t offset, off_t len);

        int (*posix_fadvise)(int fd, off_t offset, off_t len, int advice);

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

        char *(*getcwd)(char *buf, size_t size);

        char *(*getwd)(char *buf);

        int (*__uflow)(FILE *fp);
        int (*__overflow)(FILE *fp, int ch);
    };

    struct {
        FILE * (*fopen)(const char *path, const char *mode);

        FILE * (*fdopen)(int fd, const char *mode);

        FILE * (*freopen)(const char *path, const char *mode, FILE *fp);

        int (*fclose)(FILE *fp);

        int (*fcloseall)();

        void (*flockfile)(FILE *fp);

        int (*ftrylockfile)(FILE *fp);

        void (*funlockfile)(FILE *fp);

        int (*fseek)(FILE *fp, long offset, int whence);

        int (*fseeko)(FILE *fp, off_t offset, int whence);

        long (*ftell)(FILE *fp);

        off_t (*ftello)(FILE *fp);

        void (*rewind)(FILE *fp);

        int (*fgetpos)(FILE *fp, fpos_t *pos);

        int (*fsetpos)(FILE *fp, const fpos_t *pos);

        int (*fgetc_unlocked)(FILE *fp);

        int (*fputc_unlocked)(int c, FILE *fp);

        int (*getc_unlocked)(FILE *fp);

        int (*putc_unlocked)(int c, FILE *fp);

        void (*clearerr_unlocked)(FILE *fp);

        int (*feof_unlocked)(FILE *fp);

        int (*ferror_unlocked)(FILE *fp);

        int (*fileno_unlocked)(FILE *fp);

        int (*fflush_unlocked)(FILE *fp);

        size_t (*fread_unlocked)(void *buff, size_t size, size_t n, FILE *fp);

        size_t (*fwrite_unlocked)(const void *buff, size_t size, size_t n, FILE *fp);

        char * (*fgets_unlocked)(char *s, int size, FILE *fp);

        int (*fputs_unlocked)(const char *s, FILE *fp);

        void (*clearerr)(FILE *fp);

        int (*feof)(FILE *fp);

        int (*ferror)(FILE *fp);

        int (*fileno)(FILE *fp);

        int (*fgetc)(FILE *fp);

        char * (*fgets)(char *s, int size, FILE *fp);

        int (*getc)(FILE *fp);

        int (*ungetc)(int c, FILE *fp);

        int (*fputc)(int c, FILE *fp);

        int (*fputs)(const char *s, FILE *fp);

        int (*putc)(int c, FILE *fp);

        size_t (*fread)(void *buff, size_t size, size_t nmemb, FILE *fp);

        size_t (*fwrite)(const void *buff, size_t size, size_t nmemb, FILE *fp);

        int (*fprintf)(FILE *fp, const char *format, ...);

        int (*vfprintf)(FILE *fp, const char *format, va_list ap);

        int (*__fprintf_chk)(FILE *fp, int flag, const char *format, ...);

        int (*__vfprintf_chk)(FILE *fp, int flag, const char *format, va_list ap);

        ssize_t (*getdelim)(char **line, size_t *size, int delim, FILE *fp);

        ssize_t (*getline)(char **line, size_t *size, FILE *fp);

        int (*fscanf)(FILE *fp, const char *format, ...);

        int (*vfscanf)(FILE *fp, const char *format, va_list ap);

        int (*setvbuf)(FILE *fp, char *buf, int mode, size_t size);

        void (*setbuf)(FILE *fp, char *buf);

        void (*setbuffer)(FILE *fp, char *buf, size_t size);

        void (*setlinebuf)(FILE *fp);

        int (*fflush)(FILE *fp);

        ssize_t (*__libc_readline_unlocked)(FILE *fp,
                char *buffer, size_t size);
    };

} FCFSPreloadGlobalVars;

#endif
