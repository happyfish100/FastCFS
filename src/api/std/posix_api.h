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

    /** FastCFS POSIX API init
     * parameters:
     *   ctx: the POSIX API context
     *   log_prefix_name: the prefix name for log filename, NULL for stderr
     *   ns: the namespace/poolname of FastDIR
     *   config_filename: the config filename, eg. /etc/fastcfs/fcfs/fuse.conf
     *   fdir_section_name: the section name of FastDIR
     *   fs_section_name: the section name of FastStore
     *   publish: if publish the session, this parameter is valid when auth enabled
     * return: error no, 0 for success, != 0 fail
    */
    int fcfs_posix_api_init_ex1(FCFSPosixAPIContext *ctx,
            const char *log_prefix_name, const char *ns,
            const char *config_filename, const char *fdir_section_name,
            const char *fs_section_name, const bool publish);

    /** FastCFS POSIX API init with default section names
     * parameters:
     *   ctx: the POSIX API context
     *   log_prefix_name: the prefix name for log filename, NULL for stderr
     *   ns: the namespace/poolname of FastDIR
     *   config_filename: the config filename, eg. /etc/fastcfs/fcfs/fuse.conf
     * return: error no, 0 for success, != 0 fail
    */
    static inline int fcfs_posix_api_init_ex(FCFSPosixAPIContext *ctx,
            const char *log_prefix_name, const char *ns,
            const char *config_filename)
    {
        const bool publish = true;
        return fcfs_posix_api_init_ex1(ctx, log_prefix_name, ns,
                config_filename, FCFS_API_DEFAULT_FASTDIR_SECTION_NAME,
                FCFS_API_DEFAULT_FASTSTORE_SECTION_NAME, publish);
    }

    /** FastCFS POSIX API init with the global context
     * parameters:
     *   log_prefix_name: the prefix name for log filename, NULL for stderr
     *   ns: the namespace/poolname of FastDIR
     *   config_filename: the config filename, eg. /etc/fastcfs/fcfs/fuse.conf
     * return: error no, 0 for success, != 0 fail
    */
    static inline int fcfs_posix_api_init(const char *log_prefix_name,
            const char *ns, const char *config_filename)
    {
        return fcfs_posix_api_init_ex(&g_fcfs_papi_global_vars.ctx,
                log_prefix_name, ns, config_filename);
    }

    /** log configs of FastCFS POSIX API
     * parameters:
     *   ctx: the POSIX API context
     *   fdir_section_name: the section name of FastDIR
     *   fs_section_name: the section name of FastStore
     * return: none
    */
    void fcfs_posix_api_log_configs_ex(FCFSPosixAPIContext *ctx,
            const char *fdir_section_name, const char *fs_section_name);

    /** log configs of FastCFS POSIX API with the global context
     *  and default section names
     *
     * return: none
    */
    static inline void fcfs_posix_api_log_configs()
    {
        fcfs_posix_api_log_configs_ex(&g_fcfs_papi_global_vars.ctx,
                FCFS_API_DEFAULT_FASTDIR_SECTION_NAME,
                FCFS_API_DEFAULT_FASTSTORE_SECTION_NAME);
    }

    /** FastCFS POSIX API start (create the background threads)
     * parameters:
     *   ctx: the POSIX API context
     * return: error no, 0 for success, != 0 fail
    */
    static inline int fcfs_posix_api_start_ex(FCFSPosixAPIContext *ctx)
    {
        return fcfs_api_start_ex(&ctx->api_ctx, &ctx->owner);
    }

    /** FastCFS POSIX API init and start
     * parameters:
     *   ctx: the POSIX API context
     *   log_prefix_name: the prefix name for log filename, NULL for stderr
     *   ns: the namespace/poolname of FastDIR
     *   config_filename: the config filename, eg. /etc/fastcfs/fcfs/fuse.conf
     * return: error no, 0 for success, != 0 fail
    */
    static inline int fcfs_posix_api_init_start_ex(
            FCFSPosixAPIContext *ctx, const char *log_prefix_name,
            const char *ns, const char *config_filename)
    {
        int result;

        if ((result=fcfs_posix_api_init_ex(ctx, log_prefix_name,
                        ns, config_filename)) != 0)
        {
            return result;
        }
        return fcfs_api_start_ex(&ctx->api_ctx, &ctx->owner);
    }

    /** FastCFS POSIX API stop the background threads
     * parameters:
     *   ctx: the POSIX API context
     * return: none
    */
    static inline void fcfs_posix_api_stop_ex(FCFSPosixAPIContext *ctx)
    {
        fcfs_api_terminate_ex(&ctx->api_ctx);
    }

    /** FastCFS POSIX API destroy
     * parameters:
     *   ctx: the POSIX API context
     * return: none
    */
    void fcfs_posix_api_destroy_ex(FCFSPosixAPIContext *ctx);


    /** FastCFS POSIX API start (create the background threads)
     *
     * return: error no, 0 for success, != 0 fail
    */
    static inline int fcfs_posix_api_start()
    {
        return fcfs_posix_api_start_ex(&g_fcfs_papi_global_vars.ctx);
    }

    /** FastCFS POSIX API init and start with the global context
     * parameters:
     *   log_prefix_name: the prefix name for log filename, NULL for stderr
     *   ns: the namespace/poolname of FastDIR
     *   config_filename: the config filename, eg. /etc/fastcfs/fcfs/fuse.conf
     * return: error no, 0 for success, != 0 fail
    */
    static inline int fcfs_posix_api_init_start(const char *log_prefix_name,
            const char *ns, const char *config_filename)
    {
        return fcfs_posix_api_init_start_ex(&g_fcfs_papi_global_vars.ctx,
                log_prefix_name, ns, config_filename);
    }

    /** FastCFS POSIX API stop the background threads with the global context
     *
     * return: none
    */
    static inline void fcfs_posix_api_stop()
    {
        fcfs_posix_api_stop_ex(&g_fcfs_papi_global_vars.ctx);
    }

    /** FastCFS POSIX API destroy with the global context
     *
     * return: none
    */
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
