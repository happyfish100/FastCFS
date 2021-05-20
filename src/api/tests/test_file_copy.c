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

const char *config_filename = FCFS_FUSE_DEFAULT_CONFIG_FILENAME;
static char *ns = "fs";
static int buffer_size = 128 * 1024;
static char *input_filename;
static char *fs_filename;

static void usage(char *argv[])
{
    fprintf(stderr, "Usage: %s [-c config_filename=%s] "
            "[-n namespace=fs] [-b buffer_size=128KB] "
            "<input_local_filename> <fs_filename>\n\n",
            argv[0], FCFS_FUSE_DEFAULT_CONFIG_FILENAME);
}

static int copy_file()
{
    const bool publish = false;
#define FIXED_BUFFEER_SIZE  (128 * 1024)
    FCFSAPIFileContext fctx;
	int result;
    int fd;
    FCFSAPIFileInfo fi;
    char fixed_buff[FIXED_BUFFEER_SIZE];
    char async_report_config[256];
    char write_combine_config[512];
    char *buff;
    char new_fs_filename[PATH_MAX];
    int read_bytes;
    int write_bytes;
    int current_write;

    if ((fd=open(input_filename, O_RDONLY)) < 0) {
        result = errno != 0 ? errno : ENOENT;
        logError("file: "__FILE__", line: %d, "
                "open file %s to read fail, "
                "errno: %d, error info: %s",
                __LINE__, input_filename,
                result, strerror(result));
        return result;
    }

    if ((result=fcfs_api_pooled_init_with_auth(ns,
                    config_filename, publish)) != 0)
    {
        return result;
    }
    if ((result=fcfs_api_start()) != 0) {
        return result;
    }


    fcfs_api_async_report_config_to_string_ex(&g_fcfs_api_ctx,
            async_report_config, sizeof(async_report_config));
    fdir_client_log_config_ex(g_fcfs_api_ctx.contexts.fdir,
            async_report_config, true);

    fs_api_config_to_string(write_combine_config,
            sizeof(write_combine_config));
    fs_client_log_config_ex(g_fcfs_api_ctx.contexts.fsapi->fs,
            write_combine_config, true);

    if (fs_filename[strlen(fs_filename) - 1] == '/') {
        const char *filename;
        filename = strrchr(input_filename, '/');
        if (filename != NULL) {
            filename++;  //skip "/"
        } else {
            filename = input_filename;
        }
        snprintf(new_fs_filename, sizeof(new_fs_filename),
                "%s%s", fs_filename, filename);
    } else {
        snprintf(new_fs_filename, sizeof(new_fs_filename),
                "%s", fs_filename);
    }

    fctx.tid = getpid();
    fctx.omp.mode = 0755;
    fctx.omp.uid = geteuid();
    fctx.omp.gid = getegid();
    if ((result=fcfs_api_open(&fi, new_fs_filename,
                    O_CREAT | O_WRONLY, &fctx)) != 0)
    {
        return result;
    }

    if (buffer_size <= FIXED_BUFFEER_SIZE) {
        buff = fixed_buff;
    } else {
        buff = (char *)fc_malloc(buffer_size);
        if (buff == NULL) {
            return ENOMEM;
        }
    }

    write_bytes = 0;
    while (1) {
        read_bytes = read(fd, buff, buffer_size);
        if (read_bytes == 0) {
            break;
        } else if (read_bytes < 0) {
            result = errno != 0 ? errno : ENOENT;
            logError("file: "__FILE__", line: %d, "
                    "read from file %s fail, "
                    "errno: %d, error info: %s",
                    __LINE__, input_filename,
                    result, strerror(result));
            return result;
        }

        result = fcfs_api_write(&fi, buff, read_bytes, &current_write);
        if (result != 0) {
            logError("file: "__FILE__", line: %d, "
                    "write to file %s fail, "
                    "errno: %d, error info: %s",
                    __LINE__, fs_filename,
                    result, STRERROR(result));
            return result;
        }

        write_bytes += current_write;
    }
    fcfs_api_close(&fi);

    fcfs_api_terminate();
    return 0;
}

int main(int argc, char *argv[])
{
    int result;
	int ch;
    int64_t bytes;

    if (argc < 3) {
        usage(argv);
        return 1;
    }

    while ((ch=getopt(argc, argv, "hc:n:b:")) != -1) {
        switch (ch) {
            case 'h':
                usage(argv);
                break;
            case 'c':
                config_filename = optarg;
                break;
            case 'n':
                ns = optarg;
                break;
            case 'b':
                if ((result=parse_bytes(optarg, 1, &bytes)) != 0) {
                    usage(argv);
                    return result;
                }
                buffer_size = bytes;
                break;
            default:
                usage(argv);
                return 1;
        }
    }

    if (optind + 1 >= argc) {
        usage(argv);
        return 1;
    }

    log_init();
    //g_log_context.log_level = LOG_DEBUG;

    input_filename = argv[optind];
    fs_filename = argv[optind + 1];
    if (strlen(fs_filename) == 0) {
        usage(argv);
        return EINVAL;
    }

    return copy_file();
}
