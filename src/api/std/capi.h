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


#ifndef _FCFS_CAPI_H
#define _FCFS_CAPI_H

#include "api_types.h"

#define fcfs_fopen(path, mode) \
    fcfs_fopen_ex(&G_FCFS_PAPI_CTX, path, mode)

#define fcfs_fdopen(fd, mode) \
    fcfs_fdopen_ex(&G_FCFS_PAPI_CTX, fd, mode)

#define fcfs_freopen(path, mode, fp) \
    fcfs_freopen_ex(&G_FCFS_PAPI_CTX, path, mode, fp)

#ifdef __cplusplus
extern "C" {
#endif

    FILE *fcfs_fopen_ex(FCFSPosixAPIContext *ctx,
            const char *path, const char *mode);

    FILE *fcfs_fdopen_ex(FCFSPosixAPIContext *ctx, int fd, const char *mode);

    FILE *fcfs_freopen_ex(FCFSPosixAPIContext *ctx,
            const char *path, const char *mode, FILE *fp);

    int fcfs_fclose(FILE *fp);

    void fcfs_flockfile(FILE *fp);

    int fcfs_ftrylockfile(FILE *fp);

    void fcfs_funlockfile(FILE *fp);

    int fcfs_fseek(FILE *fp, long offset, int whence);

    int fcfs_fseeko(FILE *fp, off_t offset, int whence);

    long fcfs_ftell(FILE *fp);

    off_t fcfs_ftello(FILE *fp);

    void fcfs_rewind(FILE *fp);

    int fcfs_fgetpos(FILE *fp, fpos_t *pos);

    int fcfs_fsetpos(FILE *fp, const fpos_t *pos);

    /* unlocked */
    int fcfs_getc_unlocked(FILE *fp);

    int fcfs_putc_unlocked(int c, FILE *fp);

    void fcfs_clearerr_unlocked(FILE *fp);

    int fcfs_feof_unlocked(FILE *fp);

    int fcfs_ferror_unlocked(FILE *fp);

    int fcfs_fileno_unlocked(FILE *fp);

    int fcfs_fflush_unlocked(FILE *fp);

    int fcfs_fgetc_unlocked(FILE *fp);

    int fcfs_fputc_unlocked(int c, FILE *fp);

    size_t fcfs_fread_unlocked(void *ptr, size_t size, size_t n, FILE *fp);

    size_t fcfs_fwrite_unlocked(const void *ptr,
            size_t size, size_t n, FILE *fp);

    char *fcfs_fgets_unlocked(char *s, int n, FILE *fp);

    int fcfs_fputs_unlocked(const char *s, FILE *fp);



    void fcfs_clearerr(FILE *fp);

    int fcfs_feof(FILE *fp);

    int fcfs_ferror(FILE *fp);

    int fcfs_fileno(FILE *fp);

    int fcfs_fgetc(FILE *fp);

    char *fcfs_fgets(char *s, int size, FILE *fp);

    int fcfs_getc(FILE *fp);

    int fcfs_ungetc(int c, FILE *fp);

    int fcfs_fputc(int c, FILE *fp);

    int fcfs_fputs(const char *s, FILE *fp);

    int fcfs_putc(int c, FILE *fp);

    size_t fcfs_fread(void *ptr, size_t size, size_t nmemb, FILE *fp);

    size_t fcfs_fwrite(const void *ptr, size_t size, size_t nmemb, FILE *fp);

    ssize_t fcfs_getline(char **lineptr, size_t *n, FILE *fp);

    ssize_t fcfs_getdelim(char **lineptr, size_t *n, int delim, FILE *fp);

    int fcfs_fscanf(FILE *fp, const char *format, ...);

    int fcfs_vfscanf(FILE *fp, const char *format, va_list ap);

    int fcfs_setvbuf(FILE *fp, char *buf, int mode, size_t size);

    static inline void fcfs_setbuf(FILE *fp, char *buf)
    {
        fcfs_setvbuf(fp, buf, (buf != NULL ? _IOFBF : _IONBF), BUFSIZ);
    }

    static inline void fcfs_setbuffer(FILE *fp, char *buf, size_t size)
    {
        fcfs_setvbuf(fp, buf, (buf != NULL ? _IOFBF : _IONBF), size);
    }

    static inline void fcfs_setlinebuf_ex(FILE *fp)
    {
        fcfs_setvbuf(fp, NULL, _IOLBF, 0);
    }

    int fcfs_fflush(FILE *fp);

#ifdef __cplusplus
}
#endif

#endif
