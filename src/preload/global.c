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

static int dlsym_all()
{
    g_fcfs_preload_global_vars.unsetenv = dlsym_one("unsetenv", true);
    g_fcfs_preload_global_vars.clearenv = dlsym_one("clearenv", true);
    g_fcfs_preload_global_vars.fopen = dlsym_two("fopen", "fopen64", true);
    g_fcfs_preload_global_vars.fread = dlsym_one("fread", true);
    g_fcfs_preload_global_vars.fwrite = dlsym_one("fwrite", true);
    g_fcfs_preload_global_vars.open = dlsym_two("open", "open64", true);
    g_fcfs_preload_global_vars.openat = dlsym_two("openat", "openat64", true);
    g_fcfs_preload_global_vars.creat = dlsym_two("creat", "creat64", true);
    g_fcfs_preload_global_vars.close = dlsym_one("close", true);
    g_fcfs_preload_global_vars.fsync = dlsym_one("fsync", true);
    g_fcfs_preload_global_vars.fdatasync = dlsym_one("fdatasync", true);
    g_fcfs_preload_global_vars.write = dlsym_one("write", true);
    g_fcfs_preload_global_vars.pwrite = dlsym_two("pwrite", "pwrite64", true);
    g_fcfs_preload_global_vars.writev = dlsym_one("writev", true);
    g_fcfs_preload_global_vars.pwritev = dlsym_two(
            "pwritev", "pwritev64", false);
    g_fcfs_preload_global_vars.read = dlsym_one("read", true);
    g_fcfs_preload_global_vars.__read_chk = dlsym_one("__read_chk", true);
    g_fcfs_preload_global_vars.pread = dlsym_two("pread", "pread64", true);
    g_fcfs_preload_global_vars.readv = dlsym_one("readv", true);
    g_fcfs_preload_global_vars.preadv = dlsym_two("preadv", "preadv64", false);
    g_fcfs_preload_global_vars.lseek = dlsym_two("lseek", "lseek64", true);
    g_fcfs_preload_global_vars.fallocate = dlsym_two(
            "fallocate", "fallocate64", false);
    g_fcfs_preload_global_vars.truncate = dlsym_two(
            "truncate", "truncate64", true);
    g_fcfs_preload_global_vars.ftruncate = dlsym_two(
            "ftruncate", "ftruncate64", true);

    g_fcfs_preload_global_vars.stat = dlsym_one("stat", false);
    if (g_fcfs_preload_global_vars.stat != NULL) {
        g_fcfs_preload_global_vars.lstat = dlsym_one("lstat", true);
        g_fcfs_preload_global_vars.fstat = dlsym_one("fstat", true);
        g_fcfs_preload_global_vars.fstatat = dlsym_one("fstatat", true);
    } else {
        g_fcfs_preload_global_vars.__xstat = dlsym_two(
                "__xstat", "__xstat64", true);
        g_fcfs_preload_global_vars.__lxstat = dlsym_two(
                "__lxstat", "__lxstat64", true);
        g_fcfs_preload_global_vars.__fxstat = dlsym_two(
                "__fxstat", "__fxstat64", true);
        g_fcfs_preload_global_vars.__fxstatat = dlsym_two(
                "__fxstatat", "__fxstatat64", true);
    }

    g_fcfs_preload_global_vars.flock = dlsym_two("flock", "flock64", true);
    g_fcfs_preload_global_vars.fcntl = dlsym_two("fcntl", "fcntl64", true);
    g_fcfs_preload_global_vars.symlink = dlsym_one("symlink", true);
    g_fcfs_preload_global_vars.symlinkat = dlsym_one("symlinkat", true);
    g_fcfs_preload_global_vars.link = dlsym_one("link", true);
    g_fcfs_preload_global_vars.linkat = dlsym_one("linkat", true);
    g_fcfs_preload_global_vars.readlink = dlsym_one("readlink", true);
    g_fcfs_preload_global_vars.readlinkat = dlsym_one("readlinkat", true);
    g_fcfs_preload_global_vars.mknod = dlsym_two("mknod", "__xmknod", true);
    g_fcfs_preload_global_vars.mknodat = dlsym_two(
            "mknodat", "__xmknodat", false);
    g_fcfs_preload_global_vars.mkfifo = dlsym_one("mkfifo", true);
    g_fcfs_preload_global_vars.mkfifoat = dlsym_one("mkfifoat", false);
    g_fcfs_preload_global_vars.access = dlsym_one("access", true);
    g_fcfs_preload_global_vars.faccessat = dlsym_one("faccessat", true);
    g_fcfs_preload_global_vars.euidaccess = dlsym_one("euidaccess", false);
    g_fcfs_preload_global_vars.eaccess = dlsym_one("eaccess", false);
    g_fcfs_preload_global_vars.utime = dlsym_one("utime", true);
    g_fcfs_preload_global_vars.utimes = dlsym_one("utimes", true);
    g_fcfs_preload_global_vars.futimes = dlsym_one("futimes", true);
    g_fcfs_preload_global_vars.futimesat = dlsym_one("futimesat", false);
    g_fcfs_preload_global_vars.futimens = dlsym_one("futimens", true);
    g_fcfs_preload_global_vars.utimensat = dlsym_one("utimensat", true);
    g_fcfs_preload_global_vars.unlink = dlsym_one("unlink", true);
    g_fcfs_preload_global_vars.unlinkat = dlsym_one("unlinkat", true);
    g_fcfs_preload_global_vars.rename = dlsym_one("rename", true);
    g_fcfs_preload_global_vars.renameat = dlsym_one("renameat", true);
    g_fcfs_preload_global_vars.renameat2 = dlsym_two(
            "renameat2", "renameatx_np", true);
    g_fcfs_preload_global_vars.mkdir = dlsym_one("mkdir", true);
    g_fcfs_preload_global_vars.mkdirat = dlsym_one("mkdirat", true);
    g_fcfs_preload_global_vars.rmdir = dlsym_one("rmdir", true);
    g_fcfs_preload_global_vars.chown = dlsym_one("chown", true);
    g_fcfs_preload_global_vars.lchown = dlsym_one("lchown", true);
    g_fcfs_preload_global_vars.fchown = dlsym_one("fchown", true);
    g_fcfs_preload_global_vars.fchownat = dlsym_one("fchownat", true);
    g_fcfs_preload_global_vars.chmod = dlsym_one("chmod", true);
    g_fcfs_preload_global_vars.fchmod = dlsym_one("fchmod", true);
    g_fcfs_preload_global_vars.fchmodat = dlsym_one("fchmodat", true);
    g_fcfs_preload_global_vars.statvfs = dlsym_two(
            "statvfs", "statvfs64", true);
    g_fcfs_preload_global_vars.fstatvfs = dlsym_two(
            "fstatvfs", "fstatvfs64", true);
    g_fcfs_preload_global_vars.setxattr = dlsym_one("setxattr", true);
    g_fcfs_preload_global_vars.lsetxattr = dlsym_one("lsetxattr", false);
    g_fcfs_preload_global_vars.fsetxattr = dlsym_one("fsetxattr", true);
    g_fcfs_preload_global_vars.getxattr = dlsym_one("getxattr", true);
    g_fcfs_preload_global_vars.lgetxattr = dlsym_one("lgetxattr", false);
    g_fcfs_preload_global_vars.fgetxattr = dlsym_one("fgetxattr", true);
    g_fcfs_preload_global_vars.listxattr = dlsym_one("listxattr", true);
    g_fcfs_preload_global_vars.llistxattr = dlsym_one("llistxattr", false);
    g_fcfs_preload_global_vars.flistxattr = dlsym_one("flistxattr", true);
    g_fcfs_preload_global_vars.removexattr = dlsym_one("removexattr", true);
    g_fcfs_preload_global_vars.lremovexattr = dlsym_one("lremovexattr", false);
    g_fcfs_preload_global_vars.fremovexattr = dlsym_one("fremovexattr", true);

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
    g_fcfs_preload_global_vars.chdir = dlsym_one("chdir", true);
    g_fcfs_preload_global_vars.fchdir = dlsym_one("fchdir", true);
    g_fcfs_preload_global_vars.getcwd = dlsym_one("getcwd", true);
    g_fcfs_preload_global_vars.getwd = dlsym_one("getwd", true);
    g_fcfs_preload_global_vars.chroot = dlsym_one("chroot", true);
    g_fcfs_preload_global_vars.dup = dlsym_one("dup", true);
    g_fcfs_preload_global_vars.dup2 = dlsym_one("dup2", true);
    g_fcfs_preload_global_vars.mmap = dlsym_one("mmap", true);
    g_fcfs_preload_global_vars.munmap = dlsym_one("munmap", true);

    return 0;
}

int fcfs_preload_global_init()
{
    return dlsym_all();
}
