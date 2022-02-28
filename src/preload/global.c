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

#include <dlfcn.h>
#include "global.h"

FCFSPreloadGlobalVars g_fcfs_preload_global_vars;

static inline void *dlsym_one(const char *fname, const bool required)
{
    int log_level;
    void *func;

    if ((func=dlsym(RTLD_NEXT, fname)) == NULL) {
        log_level = (required ? LOG_ERR : LOG_DEBUG);
        log_it_ex(&g_log_context, log_level, "file: "__FILE__", line: %d, "
                "function %s not exist!", __LINE__, fname);
    }
    return func;
}

static inline void *dlsym_two(const char *fname1,
        const char *fname2, const bool required)
{
    int log_level;
    void *func;

    if ((func=dlsym(RTLD_NEXT, fname1)) != NULL) {
        return func;
    }

    if ((func=dlsym(RTLD_NEXT, fname2)) == NULL) {
        log_level = (required ? LOG_ERR : LOG_DEBUG);
        log_it_ex(&g_log_context, log_level, "file: "__FILE__", line: %d, "
                "function %s | %s not exist!", __LINE__, fname1, fname2);
    }

    return func;
}

static int dlsym_papi()
{
    bool required;

    g_fcfs_preload_global_vars.unsetenv = dlsym_one("unsetenv", true);
    g_fcfs_preload_global_vars.clearenv = dlsym_one("clearenv", true);

    g_fcfs_preload_global_vars.fstatat = dlsym_one("fstatat", false);
    required = (g_fcfs_preload_global_vars.fstatat == NULL);
    g_fcfs_preload_global_vars.__xstat = dlsym_two(
            "__xstat", "__xstat64", required);
    g_fcfs_preload_global_vars.__lxstat = dlsym_two(
            "__lxstat", "__lxstat64", required);
    g_fcfs_preload_global_vars.__fxstat = dlsym_two(
            "__fxstat", "__fxstat64", required);
    g_fcfs_preload_global_vars.__fxstatat = dlsym_two(
            "__fxstatat", "__fxstatat64", required);

    g_fcfs_preload_global_vars.__xmknod = dlsym_one("__xmknod", false);
    g_fcfs_preload_global_vars.__xmknodat = dlsym_one("__xmknodat", false);

    g_fcfs_preload_global_vars.mkfifo = dlsym_one("mkfifo", true);
    g_fcfs_preload_global_vars.mkfifoat = dlsym_one("mkfifoat", false);
    g_fcfs_preload_global_vars.euidaccess = dlsym_one("euidaccess", false);
    g_fcfs_preload_global_vars.eaccess = dlsym_one("eaccess", false);
    g_fcfs_preload_global_vars.futimes = dlsym_one("futimes", true);
    g_fcfs_preload_global_vars.futimens = dlsym_one("futimens", true);
    g_fcfs_preload_global_vars.statvfs = dlsym_two(
            "statvfs", "statvfs64", true);
    g_fcfs_preload_global_vars.fstatvfs = dlsym_two(
            "fstatvfs", "fstatvfs64", true);

    g_fcfs_preload_global_vars.lockf = dlsym_two("lockf", "lockf64", true);
    g_fcfs_preload_global_vars.posix_fallocate = dlsym_two(
            "posix_fallocate", "posix_fallocate64", true);
    g_fcfs_preload_global_vars.posix_fadvise = dlsym_two(
            "posix_fadvise", "posix_fadvise64", true);
    g_fcfs_preload_global_vars.vdprintf = dlsym_one("vdprintf", true);

    g_fcfs_preload_global_vars.opendir = dlsym_one("opendir", true);
    g_fcfs_preload_global_vars.fdopendir = dlsym_one("fdopendir", true);
    g_fcfs_preload_global_vars.closedir = dlsym_one("closedir", true);
    g_fcfs_preload_global_vars.readdir = dlsym_two(
            "readdir", "readdir64", true);
    g_fcfs_preload_global_vars.readdir_r = dlsym_two(
            "readdir_r", "readdir64_r", true);
    g_fcfs_preload_global_vars.seekdir = dlsym_one("seekdir", true);
    g_fcfs_preload_global_vars.telldir = dlsym_one("telldir", true);
    g_fcfs_preload_global_vars.rewinddir = dlsym_one("rewinddir", true);
    g_fcfs_preload_global_vars.dirfd = dlsym_one("dirfd", true);
    g_fcfs_preload_global_vars.scandir = dlsym_one("scandir", true);
    g_fcfs_preload_global_vars.scandirat = dlsym_one("scandirat", false);
    g_fcfs_preload_global_vars.getcwd = dlsym_one("getcwd", true);
    g_fcfs_preload_global_vars.getwd = dlsym_one("getwd", true);

    return 0;
}

static int dlsym_capi()
{
    g_fcfs_preload_global_vars.fopen = dlsym_two("fopen", "fopen64", true);
    g_fcfs_preload_global_vars.fdopen = dlsym_one("fdopen", true);
    g_fcfs_preload_global_vars.freopen = dlsym_two("freopen", "freopen64", true);
    g_fcfs_preload_global_vars.fclose = dlsym_one("fclose", true);
    g_fcfs_preload_global_vars.fcloseall = dlsym_one("fcloseall", true);
    g_fcfs_preload_global_vars.flockfile = dlsym_one("flockfile", true);
    g_fcfs_preload_global_vars.ftrylockfile = dlsym_one("ftrylockfile", true);
    g_fcfs_preload_global_vars.funlockfile = dlsym_one("funlockfile", true);
    g_fcfs_preload_global_vars.fseek = dlsym_one("fseek", true);
    g_fcfs_preload_global_vars.fseeko = dlsym_two("fseeko", "fseeko64", true);
    g_fcfs_preload_global_vars.ftell = dlsym_one("ftell", true);
    g_fcfs_preload_global_vars.ftello = dlsym_two("ftello", "ftello64", true);
    g_fcfs_preload_global_vars.rewind = dlsym_one("rewind", true);
    g_fcfs_preload_global_vars.fgetpos = dlsym_one("fgetpos", true);
    g_fcfs_preload_global_vars.fsetpos = dlsym_one("fsetpos", true);
    g_fcfs_preload_global_vars.fgetc_unlocked = dlsym_one("fgetc_unlocked", true);
    g_fcfs_preload_global_vars.fputc_unlocked = dlsym_one("fputc_unlocked", true);
    g_fcfs_preload_global_vars.getc_unlocked = dlsym_one("getc_unlocked", true);
    g_fcfs_preload_global_vars.putc_unlocked = dlsym_one("putc_unlocked", true);
    g_fcfs_preload_global_vars.clearerr_unlocked = dlsym_one("clearerr_unlocked", true);
    g_fcfs_preload_global_vars.feof_unlocked = dlsym_one("feof_unlocked", true);
    g_fcfs_preload_global_vars.ferror_unlocked = dlsym_one("ferror_unlocked", true);
    g_fcfs_preload_global_vars.fileno_unlocked = dlsym_one("fileno_unlocked", true);
    g_fcfs_preload_global_vars.fflush_unlocked = dlsym_one("fflush_unlocked", true);
    g_fcfs_preload_global_vars.fread_unlocked = dlsym_one("fread_unlocked", true);
    g_fcfs_preload_global_vars.fwrite_unlocked = dlsym_one("fwrite_unlocked", true);
    g_fcfs_preload_global_vars.fgets_unlocked = dlsym_one("fgets_unlocked", true);
    g_fcfs_preload_global_vars.fputs_unlocked = dlsym_one("fputs_unlocked", true);
    g_fcfs_preload_global_vars.clearerr = dlsym_one("clearerr", true);
    g_fcfs_preload_global_vars.feof = dlsym_one("feof", true);
    g_fcfs_preload_global_vars.ferror = dlsym_one("ferror", true);
    g_fcfs_preload_global_vars.fileno = dlsym_one("fileno", true);
    g_fcfs_preload_global_vars.fgetc = dlsym_one("fgetc", true);
    g_fcfs_preload_global_vars.fgets = dlsym_one("fgets", true);
    g_fcfs_preload_global_vars.getc = dlsym_one("getc", true);
    g_fcfs_preload_global_vars.ungetc = dlsym_one("ungetc", true);
    g_fcfs_preload_global_vars.fputc = dlsym_one("fputc", true);
    g_fcfs_preload_global_vars.fputs = dlsym_one("fputs", true);
    g_fcfs_preload_global_vars.putc = dlsym_one("putc", true);
    g_fcfs_preload_global_vars.fread = dlsym_one("fread", true);
    g_fcfs_preload_global_vars.fwrite = dlsym_one("fwrite", true);
    g_fcfs_preload_global_vars.fprintf = dlsym_one("fprintf", true);
    g_fcfs_preload_global_vars.vfprintf = dlsym_one("vfprintf", true);
    g_fcfs_preload_global_vars.__fprintf_chk =
        dlsym_one("__fprintf_chk", false);
    g_fcfs_preload_global_vars.__vfprintf_chk =
        dlsym_one("__vfprintf_chk", false);
    g_fcfs_preload_global_vars.getdelim = dlsym_one("getdelim", true);
    g_fcfs_preload_global_vars.getline = dlsym_one("getline", true);
    g_fcfs_preload_global_vars.fscanf = dlsym_one("fscanf", true);
    g_fcfs_preload_global_vars.vfscanf = dlsym_one("vfscanf", true);
    g_fcfs_preload_global_vars.setvbuf = dlsym_one("setvbuf", true);
    g_fcfs_preload_global_vars.setbuf = dlsym_one("setbuf", true);
    g_fcfs_preload_global_vars.setbuffer = dlsym_one("setbuffer", true);
    g_fcfs_preload_global_vars.setlinebuf = dlsym_one("setlinebuf", true);
    g_fcfs_preload_global_vars.fflush = dlsym_one("fflush", true);

    return 0;
}

static inline int dlsym_all()
{
    int result;

    if ((result=dlsym_papi()) != 0) {
        return result;
    }

    return dlsym_capi();
}

int fcfs_preload_global_init()
{
    return dlsym_all();
}
