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

#define BLOCK_SIZE  512

typedef struct {
    int count;
    char **blocks;
} BlockArray;

static char *ns = "test";
static int64_t file_size;
static int write_buffer_size;
static int read_buffer_size;

static BlockArray barray;
const char *filename = "/test.dat";

static void usage(char *argv[])
{
    fprintf(stderr, "Usage: %s [-c config_filename=%s] "
            "-n [namespace=test]\n\t[-s file_size=64MB] "
            "[-r read_buffer_size=4KB] [-w write_buffer_size=4KB]\n\n",
            argv[0], FCFS_FUSE_DEFAULT_CONFIG_FILENAME);
}

static int check_create_root_path()
{
    int result;
    int64_t inode;
    FDIRClientOwnerModePair omp;

    if ((result=fcfs_api_lookup_inode_by_path("/", &inode)) != 0) {
        if (result == ENOENT) {
            FDIRDEntryFullName fullname;
            FDIRDEntryInfo dentry;

            FC_SET_STRING(fullname.ns, ns);
            FC_SET_STRING(fullname.path, "/");

            omp.mode = 0777 | S_IFDIR;
            omp.uid = geteuid();
            omp.gid = getegid();
            result = fdir_client_create_dentry(g_fcfs_api_ctx.contexts.fdir,
                    &fullname, &omp, &dentry);
        }
    }

    return result;
}

static int init()
{
    char *buff;
    char *p;
    char **pp;
    char **end;
    int count;
    int bytes;

    srand(time(NULL));
    count = (int64_t)rand() * 256LL / RAND_MAX;
    barray.count = 2;
    while (barray.count < count) {
        barray.count *= 2;
    }

    bytes = BLOCK_SIZE * barray.count;
    buff = (char *)fc_malloc(bytes);
    if (buff == NULL) {
        return ENOMEM;
    }

    barray.blocks = (char **)fc_malloc(sizeof(char *) * barray.count);
    if (barray.blocks == NULL) {
        return ENOMEM;
    }

    p = buff;
    end = barray.blocks + barray.count;
    for (pp=barray.blocks; pp<end; pp++) {
        memset(p, pp - barray.blocks, BLOCK_SIZE);
        *pp = p;
        p += BLOCK_SIZE;
    }
    
    printf("block count: %d\n", barray.count);
    return 0;
}

/*
static void *write_thread(void *arg)
{
    FCFSAPIFileContext fctx;
    FCFSAPIFileInfo fi;
    char *out_buff;
    int write_bytes;

    fctx.tid = getpid();
    fctx.omp.mode = 0755;
    fctx.omp.uid = geteuid();
    fctx.omp.gid = getegid();

    if ((result=fcfs_api_open(&fi, filename, O_CREAT |
                    O_WRONLY | O_TRUNC, &fctx)) != 0)
    {
        return result;
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

    return NULL;
}
*/

/*
static void *read_thread(void *arg)
{
    FCFSAPIFileContext fctx;
    FCFSAPIFileInfo fi;
    char *in_buff;
    int read_bytes;

    fctx.tid = getpid();
    fctx.omp.mode = 0755;
    fctx.omp.uid = geteuid();
    fctx.omp.gid = getegid();

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
}
*/

int main(int argc, char *argv[])
{
    const bool publish = false;
    const char *config_filename = FCFS_FUSE_DEFAULT_CONFIG_FILENAME;
	int ch;
	int result;
    char *endptr;

    file_size = 64 * 1024 * 1024;
    write_buffer_size = read_buffer_size = 4 * 1024;
    while ((ch=getopt(argc, argv, "hc:o:n:i:l:S:AT")) != -1) {
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
                break;
            case 's':
                file_size = strtol(optarg, &endptr, 10);
                if (file_size < 1024 * 1024) {
                    fprintf(stderr, "file size: %"PRId64" "
                            "is too small < 1MB", file_size);
                    return EINVAL;
                }
                break;
            case 'w':
                write_buffer_size = strtol(optarg, &endptr, 10);
                if (write_buffer_size <= 0) {
                    fprintf(stderr, "invalid write buffer size: %d <= 0",
                            write_buffer_size);
                    return EINVAL;
                }
                break;
            case 'r':
                read_buffer_size = strtol(optarg, &endptr, 10);
                if (read_buffer_size <= 0) {
                    fprintf(stderr, "invalid read buffer size: %d <= 0",
                            read_buffer_size);
                    return EINVAL;
                }
                break;
            default:
                usage(argv);
                return 1;
        }
    }

    log_init();
    //g_log_context.log_level = LOG_DEBUG;
   
    if ((result=init()) != 0) {
        return result;
    }

    if ((result=fcfs_api_pooled_init_with_auth(ns,
                    config_filename, publish)) != 0)
    {
        return result;
    }

    if ((result=check_create_root_path()) != 0) {
        return result;
    }

    if ((result=fcfs_api_start()) != 0) {
        return result;
    }

    fcfs_api_terminate();
    return 0;
}
