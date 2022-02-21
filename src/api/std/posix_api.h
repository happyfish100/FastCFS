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

#include <unistd.h>
#include <sys/types.h>
#include "fastcommon/shared_func.h"
#include "api_types.h"
#include "fd_manager.h"
#include "papi.h"
#include "capi.h"

#ifdef __cplusplus
extern "C" {
#endif

    extern FCFSPosixAPIGlobalVars g_fcfs_papi_global_vars;

    int fcfs_posix_api_init_ex1(FCFSPosixAPIContext *ctx, const char *ns,
            const char *config_filename, const char *fdir_section_name,
            const char *fs_section_name, const bool publish);

    static inline int fcfs_posix_api_init_ex(FCFSPosixAPIContext *ctx,
            const char *ns, const char *config_filename)
    {
        const bool publish = true;
        return fcfs_posix_api_init_ex1(ctx, ns, config_filename,
                FCFS_API_DEFAULT_FASTDIR_SECTION_NAME,
                FCFS_API_DEFAULT_FASTSTORE_SECTION_NAME,
                publish);
    }

    static inline int fcfs_posix_api_init(const char *ns,
            const char *config_filename)
    {
        return fcfs_posix_api_init_ex(&g_fcfs_papi_global_vars.ctx,
                ns, config_filename);
    }

    static inline int fcfs_posix_api_start_ex(FCFSPosixAPIContext *ctx)
    {
        return fcfs_api_start_ex(&ctx->api_ctx);
    }

    static inline void fcfs_posix_api_terminate_ex(FCFSPosixAPIContext *ctx)
    {
        fcfs_api_terminate_ex(&ctx->api_ctx);
    }

    void fcfs_posix_api_destroy_ex(FCFSPosixAPIContext *ctx);

    static inline int fcfs_posix_api_start()
    {
        return fcfs_posix_api_start_ex(&g_fcfs_papi_global_vars.ctx);
    }

    static inline void fcfs_posix_api_terminate()
    {
        fcfs_posix_api_terminate_ex(&g_fcfs_papi_global_vars.ctx);
    }

    static inline void fcfs_posix_api_destroy()
    {
        fcfs_posix_api_destroy_ex(&g_fcfs_papi_global_vars.ctx);
    }

    static inline pid_t fcfs_posix_api_getpid()
    {
        return getpid();
    }

    static inline pid_t fcfs_posix_api_gettid(
            const FCFSPosixAPITPIDType tpid_type)
    {
        if (tpid_type == fcfs_papi_tpid_type_pid) {
            return getpid();
        } else {
            return fc_gettid();
        }
    }

    static inline void fcfs_posix_api_set_omp(FDIRClientOwnerModePair *omp,
            const FCFSPosixAPIContext *pctx, const mode_t mode)
    {
        omp->mode = (mode & (~fc_get_umask()));
        if (pctx->owner.type == fcfs_api_owner_type_fixed) {
            omp->uid = pctx->owner.uid;
            omp->gid = pctx->owner.gid;
        } else {
            omp->uid = geteuid();
            omp->gid = getegid();
        }
    }

    static inline void fcfs_posix_api_set_fctx(FCFSAPIFileContext *fctx,
            const FCFSPosixAPIContext *pctx, const mode_t mode,
            const FCFSPosixAPITPIDType tpid_type)
    {
        fcfs_posix_api_set_omp(&fctx->omp, pctx, mode);
        fctx->tid = fcfs_posix_api_gettid(tpid_type);
    }

#define FCFS_API_IS_MY_MOUNTPOINT_EX(ctx, path) \
    (strlen(path) > (ctx)->mountpoint.len && \
     ((ctx)->mountpoint.len == 0 || \
      memcmp(path, (ctx)->mountpoint.str, \
          (ctx)->mountpoint.len) == 0))

#define FCFS_API_IS_MY_MOUNTPOINT(path) \
    FCFS_API_IS_MY_MOUNTPOINT_EX(&g_fcfs_papi_global_vars.ctx, path)

#define FCFS_API_CHECK_PATH_MOUNTPOINT_EX(file, line, ctx, path, func, retval) \
    do { \
        if (!FCFS_API_IS_MY_MOUNTPOINT_EX(ctx, path)) \
        { \
            logError("file: %s, line: %d, "  \
                    "%s path: %s is not the FastCFS mountpoint!", \
                    file, line, func, path); \
            errno = EOPNOTSUPP; \
            return retval; \
        } \
    } while (0)

#define FCFS_API_CHECK_PATH_MOUNTPOINT(ctx, path, func) \
    FCFS_API_CHECK_PATH_MOUNTPOINT_EX(__FILE__, __LINE__, ctx, path, func, -1)

#ifdef __cplusplus
}
#endif

#endif
