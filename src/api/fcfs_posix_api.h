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


#ifndef _FCFS_POSIX_API_H
#define _FCFS_POSIX_API_H

#include "fcfs_api.h"

typedef struct fcfs_posix_api_context {
    bool forward;
    string_t mountpoint;
    FCFSAPIOwnerInfo owner;
    FCFSAPIContext api_ctx;
} FCFSPosixAPIContext;

#ifdef __cplusplus
extern "C" {
#endif

    extern FCFSPosixAPIContext g_fcfs_papi_ctx;

    int fcfs_posix_api_init_ex1(FCFSPosixAPIContext *ctx, const char *ns,
            const char *config_filename, const char *fdir_section_name,
            const char *fs_section_name, const bool publish);

    static inline int fcfs_posix_api_init_ex(FCFSPosixAPIContext *ctx,
            const char *ns, const char *config_filename,
            const bool publish)
    {
        return fcfs_posix_api_init_ex1(ctx, ns, config_filename,
                FCFS_API_DEFAULT_FASTDIR_SECTION_NAME,
                FCFS_API_DEFAULT_FASTSTORE_SECTION_NAME,
                publish);
    }

    static inline int fcfs_posix_api_init(const char *ns,
            const char *config_filename, const bool publish)
    {
        return fcfs_posix_api_init_ex(&g_fcfs_papi_ctx,
                ns, config_filename, publish);
    }

    void fcfs_posix_api_destroy_ex(FCFSPosixAPIContext *ctx);

    int fcfs_posix_api_start_ex(FCFSPosixAPIContext *ctx);

    void fcfs_posix_api_terminate_ex(FCFSPosixAPIContext *ctx);

#ifdef __cplusplus
}
#endif

#endif
