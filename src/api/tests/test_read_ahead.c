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
static int64_t write_buffer_size;
static int64_t read_buffer_size;
static int read_thread_count;
static volatile int running_read_threads;
static volatile int can_read = 0;

static BlockArray barray;
const char *filename = "/test.dat";

static void usage(char *argv[])
{
    fprintf(stderr, "Usage: %s [-c config_filename=%s] "
            "-n [namespace=test]\n\t[-s file_size=64MB] "
            "[-r read_buffer_size=4KB] [-w write_buffer_size=4KB]\n"
            "\t[-R read_thread_count=1]\n\n", argv[0],
            FCFS_FUSE_DEFAULT_CONFIG_FILENAME);
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

static void fill_buffer(char *buff, const int length,
        const int64_t offset)
{
    int64_t block_index;
    int count;
    int i;
    char *block;
    char *p;

    block_index = offset / BLOCK_SIZE;
    count = length / BLOCK_SIZE;
    p = buff;
    for (i=0; i<count; i++) {
        block = barray.blocks[(block_index + i) % barray.count];
        memcpy(p, block, BLOCK_SIZE);
        p += BLOCK_SIZE;
    }
}

static void *write_thread(void *arg)
{
    FCFSAPIFileContext fctx;
    FCFSAPIFileInfo fi;
    char *out_buff;
    long thread_index;
    int length;
    int write_bytes;
    int result;
    int64_t remain;

    thread_index = (long)arg;
    fctx.tid = getpid() + thread_index;
    fctx.omp.mode = 0755;
    fctx.omp.uid = geteuid();
    fctx.omp.gid = getegid();
    if ((result=fcfs_api_open(&fi, filename, O_CREAT |
                    O_WRONLY | O_TRUNC, &fctx)) != 0)
    {
        __sync_bool_compare_and_swap(&can_read, 0, -1);
        return NULL;
    }

    __sync_bool_compare_and_swap(&can_read, 0, 1);

    out_buff = (char *)fc_malloc(write_buffer_size);
    if (out_buff == NULL) {
        fcfs_api_close(&fi);
        return NULL;
    }

    remain = file_size;
    while (remain > 0) {
        fill_buffer(out_buff, write_buffer_size, file_size - remain);
        length = FC_MIN(remain, write_buffer_size);
        result = fcfs_api_write(&fi, out_buff, length, &write_bytes);
        if (result != 0) {
            logError("file: "__FILE__", line: %d, "
                    "write to file %s fail, offset: %"PRId64", length: %d, "
                    "write_bytes: %d, errno: %d, error info: %s",
                    __LINE__, filename, file_size - remain, length,
                    write_bytes, result, STRERROR(result));
            break;
        }

        remain -= write_bytes;
    }

    fcfs_api_close(&fi);
    free(out_buff);
    return NULL;
}

static int read_test(const int thread_index)
{
    FCFSAPIFileContext fctx;
    FCFSAPIFileInfo fi;
    char *expect_buff;
    char *in_buff;
    int wait_count;
    int retry_count;
    int length;
    int read_bytes;
    int result;
    int fail_count;
    int64_t remain;

    fctx.tid = getpid() + thread_index;
    fctx.omp.mode = 0755;
    fctx.omp.uid = geteuid();
    fctx.omp.gid = getegid();
    if ((result=fcfs_api_open(&fi, filename, O_RDONLY, &fctx)) != 0) {
        return result;
    }

    in_buff = (char *)fc_malloc(read_buffer_size * 2);
    if (in_buff == NULL) {
        fcfs_api_close(&fi);
        return ENOMEM;
    }
    expect_buff = in_buff + read_buffer_size;

    fail_count = 0;
    wait_count = retry_count = 0;
    remain = file_size;
    while (remain > 0) {
        length = FC_MIN(remain, read_buffer_size);
        result = fcfs_api_pread(&fi, in_buff, length,
                file_size - remain, &read_bytes);
        if (result != 0) {
            logError("file: "__FILE__", line: %d, thread index: %d, "
                    "read from file %s fail, offset: %"PRId64", length: %d, "
                    "read_bytes: %d, errno: %d, error info: %s", __LINE__,
                    thread_index, filename, file_size - remain, length,
                    read_bytes, result, STRERROR(result));
            break;
        }

        if (read_bytes != length) {
            ++wait_count;
            fc_sleep_ms(1);
            continue;
        }

        fill_buffer(expect_buff, read_buffer_size, file_size - remain);
        result = memcmp(in_buff, expect_buff, length);
        if (result != 0) {
            if (fail_count++ < 10) {
                fc_sleep_ms(1);
                ++retry_count;
                continue;
            }

            logError("file: "__FILE__", line: %d, thread index: %d, "
                    "file offset: %"PRId64", read and expect buffer "
                    "compare result: %d != 0", __LINE__, thread_index,
                    file_size - remain, result);
            break;
        }

        if (fail_count > 0) {
            fail_count = 0;
        }
        remain -= read_bytes;
    }

    fcfs_api_close(&fi);
    free(in_buff);

    if (wait_count > 0 || retry_count > 0) {
        logInfo("file: "__FILE__", line: %d, "
                "thread index: %d, wait file content ready count: %d, "
                "retry count: %d", __LINE__, thread_index, wait_count,
                retry_count);
    }

    return result;
}

static void *read_thread(void *arg)
{
    int thread_index;

    thread_index = (long)arg;
    if (read_test(thread_index) == 0) {
        logInfo("file: "__FILE__", line: %d,  thread index: %d, "
                "read test successfully!", __LINE__, thread_index);
    }

    FC_ATOMIC_DEC(read_thread_count);
    return NULL;
}

int main(int argc, char *argv[])
{
    const bool publish = false;
    const char *config_filename = FCFS_FUSE_DEFAULT_CONFIG_FILENAME;
	int ch;
    int i;
	int result;
    int status;
    long thread_index;
    pthread_t wtid;
    pthread_t rtid;

    read_thread_count = 1;
    file_size = 64 * 1024 * 1024;
    write_buffer_size = read_buffer_size = 4 * 1024;
    while ((ch=getopt(argc, argv, "hc:n:s:w:r:R:")) != -1) {
        switch (ch) {
            case 'h':
                usage(argv);
                return 0;
            case 'c':
                config_filename = optarg;
                break;
            case 'n':
                ns = optarg;
                break;
            case 's':
                if ((result=parse_bytes(optarg, 1, &file_size)) != 0) {
                    return EINVAL;
                }
                if (file_size < 1024 * 1024) {
                    fprintf(stderr, "file size: %"PRId64" "
                            "is too small < 1MB", file_size);
                    return EINVAL;
                }
                file_size = MEM_ALIGN_CEIL(file_size, 1024 * 1024);
                break;
            case 'w':
                if ((result=parse_bytes(optarg, 1, &write_buffer_size)) != 0) {
                    return EINVAL;
                }
                write_buffer_size = MEM_ALIGN_CEIL(
                        write_buffer_size, 1024);
                break;
            case 'r':
                if ((result=parse_bytes(optarg, 1, &read_buffer_size)) != 0) {
                    return EINVAL;
                }
                read_buffer_size = MEM_ALIGN_CEIL(
                        read_buffer_size, 1024);
                break;
            case 'R':
                read_thread_count = strtol(optarg, NULL, 10);
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

    thread_index = 1;
    if ((result=pthread_create(&wtid, NULL, write_thread,
                    (void *)(thread_index++))) != 0)
    {
        return result;
    }

    while ((status=FC_ATOMIC_GET(can_read)) == 0) {
        fc_sleep_ms(10);
    }

    if (status < 0) {
        return EIO;
    }

    running_read_threads = read_thread_count;
    for (i=0; i<read_thread_count; i++) {
        if ((result=pthread_create(&rtid, NULL, read_thread,
                        (void *)(thread_index++))) != 0)
        {
            return result;
        }
    }
    pthread_join(wtid, NULL);

    while (FC_ATOMIC_GET(read_thread_count) != 0) {
        fc_sleep_ms(10);
    }

    fcfs_api_terminate();
    return 0;
}
