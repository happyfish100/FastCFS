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
#include "fastcommon/shared_func.h"
#include "fastcommon/logger.h"
#include "fastcommon/locked_list.h"
#include "papi.h"
#include "capi.h"

static FCLockedList capi_opend_files;

#define FCFS_CAPI_MAGIC_NUMBER 1645431088

#ifdef OS_LINUX
#define FCFS_POS_OFFSET(pos) (pos)->__pos
#else
#define FCFS_POS_OFFSET(pos) *(pos)
#endif

#define FCFS_CAPI_CONVERT_FP_EX(fp, ...) \
    FCFSPosixCAPIFILE *file; \
    file = (FCFSPosixCAPIFILE *)fp; \
    if (file->magic != FCFS_CAPI_MAGIC_NUMBER) { \
        errno = EBADF; \
        return __VA_ARGS__; \
    }

#define FCFS_CAPI_CONVERT_FP(fp) \
    FCFS_CAPI_CONVERT_FP_EX(fp, -1)

#define FCFS_CAPI_CONVERT_FP_VOID(fp) \
    FCFS_CAPI_CONVERT_FP_EX(fp)

int fcfs_capi_init()
{
    return locked_list_init(&capi_opend_files);
}

void fcfs_capi_destroy()
{
    fcfs_fcloseall();
    locked_list_destroy(&capi_opend_files);
}

static FILE *alloc_file_handle(int fd)
{
    FCFSPosixCAPIFILE *file;
    int result;

    if ((file=fc_malloc(sizeof(FCFSPosixCAPIFILE))) == NULL) {
        errno = ENOMEM;
        return NULL;
    }

    if ((result=init_pthread_lock(&file->lock)) != 0) {
        free(file);
        errno = result;
        return NULL;
    }

    file->fd = fd;
    file->magic = FCFS_CAPI_MAGIC_NUMBER;
    file->error_no = 0;
    file->eof = 0;
    locked_list_add_tail(&file->dlink, &capi_opend_files);

    return (FILE *)file;
}

static inline int free_file_handle(FCFSPosixCAPIFILE *file)
{
    int result;

    locked_list_del(&file->dlink, &capi_opend_files);
    if (fcfs_close(file->fd) == 0) {
        result = 0;
    } else {
        result = EOF;
    }
    file->magic = 0;
    free(file);

    return result;
}

static int mode_to_flags(const char *mode, int *flags)
{
    const char *p;
    const char *end;
    int len;

    len = strlen(mode);
    if (len == 0) {
        return EINVAL;
    }

    p = mode;
    switch (*p++) {
        case 'r':
            if (*p == '+') {
                *flags = O_RDWR;
                p++;
            } else {
                *flags = O_RDONLY;
            }
            break;
        case 'w':
            if (*p == '+') {
                *flags = O_RDWR | O_CREAT | O_TRUNC;
                p++;
            } else {
                *flags = O_WRONLY | O_CREAT | O_TRUNC;
            }
            break;
        case 'a':
            if (*p == '+') {
                *flags = O_RDWR | O_CREAT | O_APPEND;
                p++;
            } else {
                *flags = O_WRONLY | O_CREAT | O_APPEND;
            }
            break;
        default:
            return EINVAL;
    }

    end = mode + len;
    while (p < end) {
        switch (*p) {
            case 'e':
                *flags |= O_CLOEXEC;
                break;
            case'x':
                *flags |= O_EXCL;
                break;
            default:
                break;
        }
        ++p;
    }

    return 0;
}

static int check_flags(const char *mode, const int flags, int *adding_flags)
{
    const char *p;
    const char *end;
    int rwflags;
    int len;

    len = strlen(mode);
    if (len == 0) {
        return EINVAL;
    }

    *adding_flags = 0;
    p = mode;
    switch (*p++) {
        case 'r':
            if (*p == '+') {
                rwflags = O_RDWR;
                p++;
            } else {
                rwflags = O_RDONLY;
            }
            break;
        case 'a':
            *adding_flags = O_APPEND;
        case 'w':
            if (*p == '+') {
                rwflags = O_RDWR;
                p++;
            } else {
                rwflags = O_WRONLY;
            }

            break;
        default:
            return EINVAL;
    }

    if ((flags & rwflags) != rwflags) {
        return EINVAL;
    }

    end = mode + len;
    while (p < end) {
        switch (*p) {
            case 'e':
                *adding_flags |= O_CLOEXEC;
                break;
            case'x':
                //just ignore
                break;
            default:
                break;
        }
        ++p;
    }

    return 0;
}

static inline int do_fdopen(FCFSPosixAPIContext *ctx,
        const char *path, const char *mode)
{
    int flags;
    int result;

    if ((result=mode_to_flags(mode, &flags)) != 0) {
        errno = result;
        return -1;
    }

    return fcfs_file_open(ctx, path, flags, 0666, fcfs_papi_tpid_type_pid);
}

void fcfs_flockfile(FILE *fp)
{
    FCFS_CAPI_CONVERT_FP_VOID(fp);
    PTHREAD_MUTEX_LOCK(&file->lock);
}

int fcfs_ftrylockfile(FILE *fp)
{
    int result;
    FCFS_CAPI_CONVERT_FP_EX(fp, -1);
    if ((result=pthread_mutex_trylock(&file->lock)) != 0) {
        errno = result;
        return -1;
    }
    return 0;
}

void fcfs_funlockfile(FILE *fp)
{
    FCFS_CAPI_CONVERT_FP_VOID(fp);
    PTHREAD_MUTEX_UNLOCK(&file->lock);
}

FILE *fcfs_fopen_ex(FCFSPosixAPIContext *ctx,
        const char *path, const char *mode)
{
    int fd;

    if ((fd=do_fdopen(ctx, path, mode)) < 0) {
        return NULL;
    }

    return alloc_file_handle(fd);
}

FILE *fcfs_fdopen_ex(FCFSPosixAPIContext *ctx, int fd, const char *mode)
{
    int flags;
    int adding_flags;
    int result;

    if ((flags=fcfs_fcntl(fd, F_GETFL)) < 0) {
        return NULL;
    }

    if ((result=check_flags(mode, flags, &adding_flags)) != 0) {
        errno = result;
        return NULL;
    }

    if (adding_flags != 0 && (flags & adding_flags) != adding_flags) {
        if (fcfs_fcntl(fd, F_SETFL, (flags | adding_flags)) != 0) {
            return NULL;
        }
    }

    return alloc_file_handle(fd);
}

FILE *fcfs_freopen_ex(FCFSPosixAPIContext *ctx,
        const char *path, const char *mode, FILE *fp)
{
    int fd;
    FCFSPosixAPIFileInfo *finfo;

    FCFS_CAPI_CONVERT_FP_EX(fp, NULL);
    if (path == NULL) {
        if ((finfo=fcfs_get_file_handle(file->fd)) == NULL) {
            errno = EBADF;
            return NULL;
        }

        path = finfo->filename.str;
    }

    if ((fd=do_fdopen(ctx, path, mode)) < 0) {
        return NULL;
    }

    fcfs_close(file->fd);
    file->fd = fd;
    file->eof = 0;
    file->error_no = 0;
    return (FILE *)file;
}

int fcfs_fclose(FILE *fp)
{
    FCFS_CAPI_CONVERT_FP_EX(fp, EOF);
    return free_file_handle(file);
}

int fcfs_fcloseall()
{
    int result;
    int r;
    FCFSPosixCAPIFILE *file;

    result = 0;
    while (1) {
        locked_list_first_entry(&capi_opend_files,
                FCFSPosixCAPIFILE, dlink, file);
        if (file == NULL) {
            break;
        }

        if ((r=free_file_handle(file)) != 0) {
            result = r;
        }
    }

    return result;
}

int fcfs_setvbuf(FILE *fp, char *buf, int mode, size_t size)
{
    switch (mode) {
        case _IOFBF:
            break;
        case _IOLBF:
            break;
        case _IONBF:
            break;
        default:
            break;
    }

    //alloc_file_buffer(FCFSPosixCAPIFILE *file, const int size);
    return 0;
}

int fcfs_fseek(FILE *fp, long offset, int whence)
{
    FCFS_CAPI_CONVERT_FP(fp);
    return fcfs_lseek(file->fd, offset, whence) >= 0 ? 0 : -1;
}

int fcfs_fseeko(FILE *fp, off_t offset, int whence)
{
    FCFS_CAPI_CONVERT_FP(fp);
    return fcfs_lseek(file->fd, offset, whence) >= 0 ? 0 : -1;
}

long fcfs_ftell(FILE *fp)
{
    FCFS_CAPI_CONVERT_FP(fp);
    return fcfs_ltell(file->fd);
}

off_t fcfs_ftello(FILE *fp)
{
    FCFS_CAPI_CONVERT_FP(fp);
    return fcfs_ltell(file->fd);
}

void fcfs_rewind(FILE *fp)
{
    FCFS_CAPI_CONVERT_FP_VOID(fp);
    fcfs_lseek(file->fd, 0L, SEEK_SET);
}

int fcfs_fgetpos(FILE *fp, fpos_t *pos)
{
    FCFS_CAPI_CONVERT_FP(fp);
    FCFS_POS_OFFSET(pos) = fcfs_ltell(file->fd);
    return FCFS_POS_OFFSET(pos) >= 0 ? 0 : -1;
}

int fcfs_fsetpos(FILE *fp, const fpos_t *pos)
{
    FCFS_CAPI_CONVERT_FP(fp);
    return fcfs_lseek(file->fd, FCFS_POS_OFFSET(pos), SEEK_SET) >= 0 ? 0 : -1;
}

#define SET_FILE_ERROR(file, bytes) \
    do { \
        if (bytes < 0) { \
            file->error_no = (errno != 0 ? errno : EIO); \
        } else { \
            file->eof = 1; \
        } \
    } while (0)

static inline int do_fgetc(FILE *fp, const bool need_lock)
{
    int bytes;
    int c;
    unsigned char buff[8];

    FCFS_CAPI_CONVERT_FP_EX(fp, EOF);
    if (need_lock) {
        PTHREAD_MUTEX_LOCK(&file->lock);
    }

    bytes = fcfs_read(file->fd, buff, 1);
    if (bytes > 0) {
        c = *buff;
    } else {
        SET_FILE_ERROR(file, bytes);
        c = EOF;
    }

    if (need_lock) {
        PTHREAD_MUTEX_UNLOCK(&file->lock);
    }
    return c;
}

static inline int do_fputc(int c, FILE *fp, const bool need_lock)
{
    int bytes;
    char ch;

    FCFS_CAPI_CONVERT_FP_EX(fp, EOF);
    if (need_lock) {
        PTHREAD_MUTEX_LOCK(&file->lock);
    }

    ch = c;
    bytes = fcfs_write(file->fd, &ch, 1);
    if (bytes <= 0) {
        file->error_no = (errno != 0 ? errno : EIO);
        ch = EOF;
    }

    if (need_lock) {
        PTHREAD_MUTEX_UNLOCK(&file->lock);
    }
    return ch;
}

static inline void do_clearerr(FILE *fp, const bool need_lock)
{
    FCFS_CAPI_CONVERT_FP_VOID(fp);
    if (need_lock) {
        PTHREAD_MUTEX_LOCK(&file->lock);
    }

    file->error_no = 0;
    file->eof = 0;

    if (need_lock) {
        PTHREAD_MUTEX_UNLOCK(&file->lock);
    }
}

static inline int do_feof(FILE *fp, const bool need_lock)
{
    int eof;

    FCFS_CAPI_CONVERT_FP(fp);
    if (need_lock) {
        PTHREAD_MUTEX_LOCK(&file->lock);
    }
    eof = file->eof;
    if (need_lock) {
        PTHREAD_MUTEX_UNLOCK(&file->lock);
    }

    return eof;
}

static inline int do_ferror(FILE *fp, const bool need_lock)
{
    int error_no;

    FCFS_CAPI_CONVERT_FP(fp);
    if (need_lock) {
        PTHREAD_MUTEX_LOCK(&file->lock);
    }
    error_no = file->error_no;
    if (need_lock) {
        PTHREAD_MUTEX_UNLOCK(&file->lock);
    }

    return error_no;
}

static inline int do_fileno(FILE *fp, const bool need_lock)
{
    int fd;

    FCFS_CAPI_CONVERT_FP(fp);
    if (need_lock) {
        PTHREAD_MUTEX_LOCK(&file->lock);
    }
    fd = file->fd;
    if (need_lock) {
        PTHREAD_MUTEX_UNLOCK(&file->lock);
    }

    return fd;
}

static inline int do_fflush(FILE *fp, const bool need_lock)
{
    FCFS_CAPI_CONVERT_FP_EX(fp, EOF);
    return 0;
}

static inline size_t do_fread(void *buff, size_t size,
        size_t n, FILE *fp, const bool need_lock)
{
    size_t count;

    FCFS_CAPI_CONVERT_FP_EX(fp, EOF);
    if (need_lock) {
        PTHREAD_MUTEX_LOCK(&file->lock);
    }

    count = fcfs_file_read(file->fd, buff, size, n);
    if (count != n) {
        if (count < 0) {
            file->error_no = (errno != 0 ? errno : EIO);
            count = 0;
        } else if (count == 0) {
            file->eof = 1;
        }
    }

    if (need_lock) {
        PTHREAD_MUTEX_UNLOCK(&file->lock);
    }
    return count;
}

static inline size_t do_fwrite(const void *buff, size_t size,
        size_t n, FILE *fp, const bool need_lock)
{
    size_t count;

    FCFS_CAPI_CONVERT_FP_EX(fp, EOF);
    if (need_lock) {
        PTHREAD_MUTEX_LOCK(&file->lock);
    }

    count = fcfs_file_write(file->fd, buff, size, n);
    if (count != n) {
        if (count < 0) {
            file->error_no = (errno != 0 ? errno : EIO);
            count = 0;
        } else if (count == 0) {
            file->error_no = (errno != 0 ? errno : EIO);
        }
    }

    if (need_lock) {
        PTHREAD_MUTEX_UNLOCK(&file->lock);
    }
    return count;
}

static inline int do_fputs(const char *s, FILE *fp, const bool need_lock)
{
    int len;
    int bytes;

    FCFS_CAPI_CONVERT_FP_EX(fp, EOF);
    if (need_lock) {
        PTHREAD_MUTEX_LOCK(&file->lock);
    }

    len = strlen(s);
    if (len == 0) {
        bytes = 0;
    } else {
        bytes = fcfs_write(file->fd, s, len);
        if (bytes <= 0) {
            file->error_no = (errno != 0 ? errno : EIO);
            bytes = EOF;
        }
    }

    if (need_lock) {
        PTHREAD_MUTEX_UNLOCK(&file->lock);
    }
    return bytes;
}

static inline char *do_fgets(char *s, int size,
        FILE *fp, const bool need_lock)
{
    size_t bytes;

    FCFS_CAPI_CONVERT_FP_EX(fp, NULL);
    if (need_lock) {
        PTHREAD_MUTEX_LOCK(&file->lock);
    }
    bytes = fcfs_file_gets(file->fd, s, size);
    if (bytes < 0) {
        file->error_no = (errno != 0 ? errno : EIO);
    } else if (bytes == 0) {
        file->eof = 1;
    }
    if (need_lock) {
        PTHREAD_MUTEX_UNLOCK(&file->lock);
    }

    return (bytes > 0 ? s : NULL);
}

int fcfs_fgetc_unlocked(FILE *fp)
{
    const bool need_lock = false;
    return do_fgetc(fp, need_lock);
}

int fcfs_fputc_unlocked(int c, FILE *fp)
{
    const bool need_lock = false;
    return do_fputc(c, fp, need_lock);
}

void fcfs_clearerr_unlocked(FILE *fp)
{
    const bool need_lock = false;
    return do_clearerr(fp, need_lock);
}

int fcfs_feof_unlocked(FILE *fp)
{
    const bool need_lock = false;
    return do_feof(fp, need_lock);
}

int fcfs_ferror_unlocked(FILE *fp)
{
    const bool need_lock = false;
    return do_ferror(fp, need_lock);
}

int fcfs_fileno_unlocked(FILE *fp)
{
    const bool need_lock = false;
    return do_fileno(fp, need_lock);
}

int fcfs_fflush_unlocked(FILE *fp)
{
    const bool need_lock = false;
    return do_fflush(fp, need_lock);
}

size_t fcfs_fread_unlocked(void *buff,
        size_t size, size_t n, FILE *fp)
{
    const bool need_lock = false;
    return do_fread(buff, size, n, fp, need_lock);
}

size_t fcfs_fwrite_unlocked(const void *buff,
        size_t size, size_t n, FILE *fp)
{
    const bool need_lock = false;
    return do_fwrite(buff, size, n, fp, need_lock);
}

int fcfs_fputs_unlocked(const char *s, FILE *fp)
{
    const bool need_lock = false;
    return do_fputs(s, fp, need_lock);
}

char *fcfs_fgets_unlocked(char *s, int size, FILE *fp)
{
    const bool need_lock = false;
    return do_fgets(s, size, fp, need_lock);
}

int fcfs_fgetc(FILE *fp)
{
    const bool need_lock = true;
    return do_fgetc(fp, need_lock);
}

int fcfs_fputc(int c, FILE *fp)
{
    const bool need_lock = true;
    return do_fputc(c, fp, need_lock);
}

void fcfs_clearerr(FILE *fp)
{
    const bool need_lock = true;
    return do_clearerr(fp, need_lock);
}

int fcfs_feof(FILE *fp)
{
    const bool need_lock = true;
    return do_feof(fp, need_lock);
}

int fcfs_ferror(FILE *fp)
{
    const bool need_lock = true;
    return do_ferror(fp, need_lock);
}

int fcfs_fileno(FILE *fp)
{
    const bool need_lock = true;
    return do_fileno(fp, need_lock);
}

int fcfs_fflush(FILE *fp)
{
    const bool need_lock = true;
    return do_fflush(fp, need_lock);
}

size_t fcfs_fread(void *buff,
        size_t size, size_t n, FILE *fp)
{
    const bool need_lock = true;
    return do_fread(buff, size, n, fp, need_lock);
}

size_t fcfs_fwrite(const void *buff,
        size_t size, size_t n, FILE *fp)
{
    const bool need_lock = true;
    return do_fwrite(buff, size, n, fp, need_lock);
}

int fcfs_fputs(const char *s, FILE *fp)
{
    const bool need_lock = true;
    return do_fputs(s, fp, need_lock);
}

char *fcfs_fgets(char *s, int size, FILE *fp)
{
    const bool need_lock = true;
    return do_fgets(s, size, fp, need_lock);
}

int fcfs_ungetc(int c, FILE *fp)
{
    FCFS_CAPI_CONVERT_FP_EX(fp, EOF);

    PTHREAD_MUTEX_LOCK(&file->lock);
    if (c == EOF) {
        file->error_no = EINVAL;
    } else {
        if (fcfs_lseek(file->fd, -1, SEEK_CUR) < 0) {
            file->error_no = (errno != 0 ? errno : EIO);
            c = EOF;
        }
    }
    PTHREAD_MUTEX_UNLOCK(&file->lock);

    return c;
}

int fcfs_fprintf(FILE *fp, const char *format, ...)
{
    va_list ap;
    int bytes;

    FCFS_CAPI_CONVERT_FP(fp);

    va_start(ap, format);
    bytes = fcfs_vdprintf(file->fd, format, ap);
    va_end(ap);
    return bytes;
}

int fcfs_vfprintf(FILE *fp, const char *format, va_list ap)
{
    FCFS_CAPI_CONVERT_FP(fp);
    return fcfs_vdprintf(file->fd, format, ap);
}

ssize_t fcfs_getdelim(char **line, size_t *size, int delim, FILE *fp)
{
    FCFS_CAPI_CONVERT_FP(fp);
    return fcfs_file_getdelim(file->fd, line, size, delim);
}

static inline ssize_t do_readline(FILE *fp, char *buff,
        size_t size, const bool need_lock)
{
    size_t bytes;

    FCFS_CAPI_CONVERT_FP(fp);
    if (need_lock) {
        PTHREAD_MUTEX_LOCK(&file->lock);
    }
    bytes = fcfs_file_readline(file->fd, buff, size);
    if (bytes < 0) {
        file->error_no = (errno != 0 ? errno : EIO);
    } else if (bytes == 0) {
        file->eof = 1;
    }
    if (need_lock) {
        PTHREAD_MUTEX_UNLOCK(&file->lock);
    }

    return bytes;
}

ssize_t fcfs_readline(FILE *fp, char *buff, size_t size)
{
    const bool need_lock = true;
    return do_readline(fp, buff, size, need_lock);
}

ssize_t fcfs_readline_unlocked(FILE *fp, char *buff, size_t size)
{
    const bool need_lock = false;
    return do_readline(fp, buff, size, need_lock);
}
