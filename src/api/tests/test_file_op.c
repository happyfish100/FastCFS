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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "fastcommon/logger.h"
#include "fastcfs/api/fcfs_api.h"

static void usage(char *argv[])
{
    fprintf(stderr, "Usage: %s [-c config_filename=%s] "
            "-n [namespace=fs] \n\t[-o offset=0] [-l length=0 for auto] "
            "[-A append mode] \n\t[-T truncate mode] [-S set file size = -1] "
            "-i <input_filename> <filename>\n\n", argv[0],
            FCFS_FUSE_DEFAULT_CONFIG_FILENAME);
}

int main(int argc, char *argv[])
{
    const bool publish = false;
    const char *config_filename = FCFS_FUSE_DEFAULT_CONFIG_FILENAME;
    FCFSAPIFileContext fctx;
	int ch;
	int result;
    int open_flags;
    int64_t offset = 0;
    int64_t file_size;
    int64_t file_size_to_set;
    int length = 0;
    FCFSAPIFileInfo fi;
    char *ns = "fs";
    char *input_filename = NULL;
    char *filename;
    char *endptr;
    char *out_buff;
    char *in_buff;
    int write_bytes;
    int read_bytes;

    if (argc < 2) {
        usage(argv);
        return 1;
    }

    fctx.tid = getpid();
    fctx.omp.mode = 0755;
    fctx.omp.uid = geteuid();
    fctx.omp.gid = getegid();
    open_flags = 0;
    file_size_to_set = -1;
    while ((ch=getopt(argc, argv, "hc:o:n:i:l:S:AT")) != -1) {
        switch (ch) {
            case 'h':
                usage(argv);
                break;
            case 'c':
                config_filename = optarg;
                break;
            case 'i':
                input_filename = optarg;
                break;
            case 'n':
                ns = optarg;
                break;
            case 'o':
                offset = strtol(optarg, &endptr, 10);
                break;
            case 'l':
                length = strtol(optarg, &endptr, 10);
                break;
            case 'S':
                file_size_to_set = strtol(optarg, &endptr, 10);
                break;
            case 'A':
                open_flags |= O_APPEND;
                break;
            case 'T':
                open_flags |= O_TRUNC;
                break;
            default:
                usage(argv);
                return 1;
        }
    }

    if (input_filename == NULL) {
        fprintf(stderr, "expect input filename\n");
        usage(argv);
        return 1;
    }

    if (optind >= argc) {
        fprintf(stderr, "expect filename\n");
        usage(argv);
        return 1;
    }

    log_init();
    //g_log_context.log_level = LOG_DEBUG;

    filename = argv[optind];
    if ((result=getFileContent(input_filename, &out_buff, &file_size)) != 0) {
        return result;
    }
    if (file_size == 0) {
        logError("file: "__FILE__", line: %d, "
                "empty file: %s", __LINE__, input_filename);
        return ENOENT;
    }
    if (length == 0 || length > file_size) {
        length = file_size;
    }

    if ((result=fcfs_api_pooled_init_with_auth(ns,
                    config_filename, publish)) != 0)
    {
        return result;
    }

    /*
    {
        struct statvfs stbuf;
        if (fcfs_api_statvfs(filename, &stbuf) == 0) {
            printf("%s fs id: %ld, total: %"PRId64" MB, free: %"PRId64" MB, "
                    "avail: %"PRId64" MB, f_namemax: %ld, f_flag: %ld\n", filename,
                    stbuf.f_fsid, (int64_t)(stbuf.f_blocks * stbuf.f_frsize) / (1024 * 1024),
                    (int64_t)(stbuf.f_bfree * stbuf.f_frsize) / (1024 * 1024),
                    (int64_t)(stbuf.f_bavail * stbuf.f_frsize) / (1024 * 1024),
                    stbuf.f_namemax, stbuf.f_flag);
        }
    }
    */

    if ((result=fcfs_api_start()) != 0) {
        return result;
    }

    if ((result=fcfs_api_open(&fi, filename, O_CREAT |
                    O_WRONLY | open_flags, &fctx)) != 0)
    {
        return result;
    }

    if (file_size_to_set >= 0) {
        if ((result=fcfs_api_ftruncate_ex(&fi, file_size_to_set,
                        fctx.tid)) != 0)
        {
            return result;
        }
    }

    if (offset == 0) {
        result = fcfs_api_write(&fi, out_buff, length, &write_bytes);
    } else {
        result = fcfs_api_pwrite(&fi, out_buff, length, offset, &write_bytes);
    }
    if (result != 0) {
        logError("file: "__FILE__", line: %d, "
                "write to file %s fail, offset: %"PRId64", length: %d, "
                "write_bytes: %d, errno: %d, error info: %s",
                __LINE__, filename, offset, length, write_bytes,
                result, STRERROR(result));
        return result;
    }
    fcfs_api_close(&fi);

    if ((result=fcfs_api_open(&fi, filename, O_RDONLY, &fctx)) != 0) {
        return result;
    }
    in_buff = (char *)malloc(length);
    if (in_buff == NULL) {
        logError("file: "__FILE__", line: %d, "
                "malloc %d bytes fail", __LINE__, length);
        return ENOMEM;
    }

    if ((result=fcfs_api_lseek(&fi, offset, SEEK_SET)) != 0) {
        return result;
    }

    if (offset == 0) {
        result = fcfs_api_read(&fi, in_buff, length, &read_bytes);
    } else {
        //result = fcfs_api_pread(&fi, in_buff, length, offset, &read_bytes);
        result = fcfs_api_read(&fi, in_buff, length, &read_bytes);
    }

    if (result != 0) {
        logError("file: "__FILE__", line: %d, "
                "read from file %s fail, offset: %"PRId64", length: %d, "
                "read_bytes: %d, errno: %d, error info: %s",
                __LINE__, filename, offset, length, read_bytes,
                result, STRERROR(result));
        return result;
    }

    if (read_bytes != length) {
        logError("file: "__FILE__", line: %d, "
                "read bytes: %d != slice length: %d",
                __LINE__, read_bytes, length);
        return EINVAL;
    }

    result = memcmp(in_buff, out_buff, length);
    if (result != 0) {
        printf("read and write buffer compare result: %d != 0\n", result);
        return EINVAL;
    }

    fcfs_api_close(&fi);
    fcfs_api_terminate();
    return 0;
}
