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
#include "fastcfs/fcfs_api.h"

static void usage(char *argv[])
{
    fprintf(stderr, "Usage: %s [-c config_filename] "
            "-n [namespace=fs] <input_local_filename> "
            "<fs_filename>\n\n", argv[0]);
}

const char *config_filename = "/etc/fastcfs/fuse.conf";
static char *ns = "fs";
static char *input_filename;
static char *fs_filename;

static int copy_file()
{
#define BUFFEER_SIZE  (128 * 1024)
    FDIRClientOwnerModePair omp;
	int result;
    int fd;
    FSAPIFileInfo fi;
    char buff[BUFFEER_SIZE];
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

    if ((result=fcfs_api_pooled_init(ns, config_filename)) != 0) {
        return result;
    }

    omp.mode = 0755;
    omp.uid = geteuid();
    omp.gid = getegid();
    if ((result=fsapi_open(&fi, fs_filename,
                    O_CREAT | O_WRONLY, &omp)) != 0)
    {
        return result;
    }

    write_bytes = 0;
    while (1) {
        read_bytes = read(fd, buff, BUFFEER_SIZE);
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

        result = fsapi_write(&fi, buff, read_bytes, &current_write);
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
    fsapi_close(&fi);

    return 0;
}

int main(int argc, char *argv[])
{
	int ch;

    if (argc < 3) {
        usage(argv);
        return 1;
    }

    while ((ch=getopt(argc, argv, "hc:n:")) != -1) {
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

    return copy_file();
}
