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
#include "fastcommon/connection_pool.h"
#include "sf/sf_proto.h"
#include "fastcfs/api/fcfs_api.h"

static void usage(char *argv[])
{
    fprintf(stderr, "Usage: %s [-c config_file=%s] [poolname]\n",
            argv[0], FCFS_FUSE_DEFAULT_CONFIG_FILENAME);
}

int main(int argc, char *argv[])
{
    const bool publish = false;
    const char *config_filename = FCFS_FUSE_DEFAULT_CONFIG_FILENAME;
    struct statvfs stbuf;
    const char *poolname;
    char *ns_str;
    char ns_holder[NAME_MAX];
    IniContext ini_context;
    int64_t total;
    int64_t used;
    int64_t avail;
    int ch;
	int result;

    log_init();

    while ((ch=getopt(argc, argv, "c:h")) != -1) {
        switch (ch) {
            case 'h':
                usage(argv);
                return 0;
            case 'c':
                config_filename = optarg;
                break;
            default:
                usage(argv);
                return 1;
        }
    }

    if (argc - optind >= 1) {
        poolname = argv[optind];
    } else {
        if ((result=iniLoadFromFile(config_filename, &ini_context)) != 0) {
            logError("file: "__FILE__", line: %d, "
                    "load conf file \"%s\" fail, ret code: %d",
                    __LINE__, config_filename, result);
            return result;
        }
        ns_str = iniGetStrValue(FCFS_API_DEFAULT_FASTDIR_SECTION_NAME,
                "namespace", &ini_context);
        if (ns_str == NULL || *ns_str == '\0') {
            logError("file: "__FILE__", line: %d, "
                    "config file: %s, section: %s, item: namespace "
                    "not exist or is empty", __LINE__, config_filename,
                    FCFS_API_DEFAULT_FASTDIR_SECTION_NAME);
            return ENOENT;
        }

        snprintf(ns_holder, sizeof(ns_holder), "%s", ns_str);
        poolname = ns_holder;
        iniFreeContext(&ini_context);
    }

    if ((result=fcfs_api_init_with_auth(poolname,
                    config_filename, publish)) != 0)
    {
        return result;
    }

    if ((result=fcfs_api_statvfs("/", &stbuf)) == 0) {
        total = stbuf.f_blocks * stbuf.f_frsize;
        used = (stbuf.f_blocks - stbuf.f_bfree) * stbuf.f_frsize;
        avail = stbuf.f_bavail  * stbuf.f_frsize;
        printf("{\"total\": %"PRId64", \"used\": %"PRId64", "
                "\"avail\": %"PRId64"}\n", total, used, avail);
    }

    return result;
}
