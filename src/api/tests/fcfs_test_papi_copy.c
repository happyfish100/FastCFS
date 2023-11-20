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
#include <sys/uio.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "fastcommon/logger.h"
#include "fastcfs/api/std/posix_api.h"

const char *config_filename = FCFS_FUSE_DEFAULT_CONFIG_FILENAME;
static char *ns = "fs";
static int buffer_size = 128 * 1024;
static int slice_size = 4 * 1024;
static char *input_filename;
static char *fs_filename;
static bool use_iov = false;  //use readv & writev

static void usage(char *argv[])
{
    fprintf(stderr, "Usage: %s [-c config_filename=%s] "
            "[-n namespace=fs] [-b buffer_size=128KB] "
            "[-V use readv & writev] [-s slice_size=4K] "
            "<input_local_filename> <fs_filename>\n\n",
            argv[0], FCFS_FUSE_DEFAULT_CONFIG_FILENAME);
}

static int alloc_iovec_array(iovec_array_t *array, char *buff)
{
    struct iovec *iob;
    struct iovec *last;
    char *p;

    array->count = array->alloc = (buffer_size +
            (slice_size - 1)) / slice_size;
    array->iovs = fc_malloc(sizeof(struct iovec) * array->alloc);
    if (array->iovs == NULL) {
        return ENOMEM;
    }

    p = buff;
    last = array->iovs + (array->alloc - 1);
    for (iob=array->iovs; iob<last; iob++) {
        iob->iov_base = p;
        iob->iov_len = slice_size;
        p += iob->iov_len;
    }

    last->iov_base = p;
    last->iov_len = (buff + buffer_size) - p;

    return 0;
}

static int copy_file()
{
#define FIXED_BUFFEER_SIZE  (128 * 1024)
    const char *log_prefix_name = NULL;
	int result;
    int src_fd;
    int dst_fd;
    bool is_fcfs_input;
    char fixed_buff[FIXED_BUFFEER_SIZE];
    iovec_array_t src_buffers;
    iovec_array_t dst_buffers;
    char *buff;
    char new_filename[PATH_MAX];
    char new_fs_filename[PATH_MAX];
    int filename_len;
    int read_bytes;
    int write_bytes;
    int current_write;
    int count;

    if ((result=fcfs_posix_api_init(log_prefix_name,
                    ns, config_filename)) != 0)
    {
        return result;
    }
    if ((result=fcfs_posix_api_start()) != 0) {
        return result;
    }

    fcfs_posix_api_log_configs();

    is_fcfs_input = FCFS_API_IS_MY_MOUNTPOINT(input_filename);
    if (is_fcfs_input) {
        src_fd = fcfs_open(input_filename, O_RDONLY);
    } else {
        src_fd = open(input_filename, O_RDONLY);
    }
    if (src_fd < 0) {
        result = errno != 0 ? errno : ENOENT;
        logError("file: "__FILE__", line: %d, "
                "open file %s to read fail, "
                "errno: %d, error info: %s",
                __LINE__, input_filename,
                result, strerror(result));
        return result;
    }

    if (fs_filename[strlen(fs_filename) - 1] == '/') {
        const char *filename;
        filename = strrchr(input_filename, '/');
        if (filename != NULL) {
            filename++;  //skip "/"
        } else {
            filename = input_filename;
        }
        filename_len = snprintf(new_filename, sizeof(new_filename),
                "%s%s", fs_filename, filename);
    } else {
        filename_len = snprintf(new_filename, sizeof(new_filename),
                "%s%s", (*fs_filename != '/' ? "/" : ""), fs_filename);
    }

    if (!(filename_len > g_fcfs_papi_global_vars.ctx.mountpoint.len &&
                memcmp(new_filename, g_fcfs_papi_global_vars.ctx.mountpoint.str,
                    g_fcfs_papi_global_vars.ctx.mountpoint.len) == 0))
    {
        snprintf(new_fs_filename, sizeof(new_fs_filename), "%s%s",
                g_fcfs_papi_global_vars.ctx.mountpoint.str, new_filename);
    } else {
        snprintf(new_fs_filename, sizeof(new_fs_filename), "%s", new_filename);
    }

    if ((dst_fd=fcfs_open(new_fs_filename, O_CREAT | O_WRONLY, 0755)) < 0) {
        result = errno != 0 ? errno : EIO;
        logError("file: "__FILE__", line: %d, "
                "open file %s fail, errno: %d, error info: %s",
                __LINE__, fs_filename, result, STRERROR(result));
        return result;
    }

    logInfo("is_fcfs_input: %d, input fd: %d, output fd: %d",
            is_fcfs_input, src_fd, dst_fd);

    if (buffer_size <= FIXED_BUFFEER_SIZE) {
        buff = fixed_buff;
    } else {
        buff = (char *)fc_malloc(buffer_size);
        if (buff == NULL) {
            return ENOMEM;
        }
    }

    if (use_iov) {
        if ((result=alloc_iovec_array(&src_buffers, buff)) != 0) {
            return result;
        }

        if ((result=alloc_iovec_array(&dst_buffers, buff)) != 0) {
            return result;
        }
    }

    count = 0;
    write_bytes = 0;
    while (1) {
        if (is_fcfs_input) {
            if (use_iov) {
                read_bytes = fcfs_readv(src_fd, src_buffers.iovs,
                        src_buffers.count);
            } else {
                read_bytes = fcfs_read(src_fd, buff, buffer_size);
            }
        } else {
            if (use_iov) {
                read_bytes = readv(src_fd, src_buffers.iovs,
                        src_buffers.count);
            } else {
                read_bytes = read(src_fd, buff, buffer_size);
            }
        }
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

        if (use_iov) {
            int bytes;
            int remain;
            struct iovec *iob;
            struct iovec *last;

            if (read_bytes == buffer_size) {
                dst_buffers.count = src_buffers.count;
            } else {
                iob = src_buffers.iovs;
                bytes = iob->iov_len;
                while (bytes < read_bytes) {
                    ++iob;
                    bytes += iob->iov_len;
                }

                dst_buffers.count = (iob - src_buffers.iovs + 1);
                last = dst_buffers.iovs + (dst_buffers.count - 1);
                remain = bytes - read_bytes;
                last->iov_len -= remain;

                logInfo("read_bytes: %d, bytes: %d, count: %d, last length: %d",
                        read_bytes, bytes, dst_buffers.count, (int)last->iov_len);
            }

            current_write = fcfs_writev(dst_fd, dst_buffers.iovs,
                    dst_buffers.count);
            if (read_bytes < buffer_size) {
                /* restore the last iov */
                (dst_buffers.iovs + (dst_buffers.count - 1))->iov_len =
                    (src_buffers.iovs + (dst_buffers.count - 1))->iov_len;
            }
        } else {
            current_write = fcfs_write(dst_fd, buff, read_bytes);
        }
        if (current_write != read_bytes) {
            result = errno != 0 ? errno : EIO;
            logError("file: "__FILE__", line: %d, "
                    "write to file %s fail, "
                    "errno: %d, error info: %s",
                    __LINE__, fs_filename,
                    result, STRERROR(result));
            return result;
        }

        if (++count % 10 == 0) {
            fcfs_fsync(dst_fd);
        }

        write_bytes += current_write;
    }

    if (is_fcfs_input) {
        fcfs_close(src_fd);
    } else {
        close(src_fd);
    }

    fcfs_fsync(dst_fd);
    fcfs_close(dst_fd);

    fcfs_posix_api_stop();
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

    while ((ch=getopt(argc, argv, "hc:n:b:s:V")) != -1) {
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
            case 'b':
                if ((result=parse_bytes(optarg, 1, &bytes)) != 0) {
                    usage(argv);
                    return result;
                }
                if (bytes < 4 * 1024) {
                    buffer_size = 4 * 1024;
                } else {
                    buffer_size = bytes;
                }
                break;
            case 's':
                if ((result=parse_bytes(optarg, 1, &bytes)) != 0) {
                    usage(argv);
                    return result;
                }
                if (bytes < 1 * 1024) {
                    slice_size = 1 * 1024;
                } else {
                    slice_size = bytes;
                }
                break;
            case 'V':
                use_iov = true;
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

    log_try_init();
    //g_log_context.log_level = LOG_DEBUG;

    if (use_iov) {
        logInfo("use writev & readv, slice size: %d", slice_size);
    }

    input_filename = argv[optind];
    fs_filename = argv[optind + 1];
    if (strlen(fs_filename) == 0) {
        usage(argv);
        return EINVAL;
    }

    return copy_file();
}
