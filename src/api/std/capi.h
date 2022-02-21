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
