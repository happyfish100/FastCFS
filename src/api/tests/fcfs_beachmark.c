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
#include "fastcommon/fc_atomic.h"
#include "fastcfs/api/std/posix_api.h"

typedef enum {
    fcfs_beachmark_mode_read,
    fcfs_beachmark_mode_write,
    fcfs_beachmark_mode_randread,
    fcfs_beachmark_mode_randwrite
} BeachmarkMode;

typedef struct {
    int index;
    volatile int64_t success_count;
} BeachmarkThreadInfo;

static struct {
    const char *config_filename;
    char *ns;
    int64_t file_size;
    struct {
        BeachmarkMode val;
        const char *str;
    } mode;
    int buffer_size;
    int thread_count;
    int runtime;
    string_t filename_prefix;
    bool is_fcfs_input;
} cfg = {FCFS_FUSE_DEFAULT_CONFIG_FILENAME,
    "fs", 1 * 1024 * 1024 * 1024, {fcfs_beachmark_mode_read,
        "read"}, 4 * 1024, 1, 60, {NULL, 0}, false};

static struct {
    volatile int ready_count;
    volatile int running_count;
    volatile char ready_flag;
    volatile char continue_flag;
    time_t start_time;
    struct {
        int cur;
        int avg;
        int max;
    } iops;

    struct {
        char cur[32];
        char avg[32];
        char max[32];
    } iops_buff;

    BeachmarkThreadInfo *threads;
    BeachmarkThreadInfo *tend;
} st = {0, 0, 0, 1};

static inline void usage(char *argv[])
{
    fprintf(stderr, "Usage: %s [-c config_filename=%s] "
            "[-n namespace=fs] [-b buffer_size=4KB] "
            "[-s file_size=1G] [-m mode=read] "
            "[-T threads=1] [-t runtime=60] <-f filename_prefix>\n"
            "\t mode value list: read, write, randrand, randwrite\n\n",
            argv[0], FCFS_FUSE_DEFAULT_CONFIG_FILENAME);
}

static inline int open_file(const char *filename, int flags)
{
    if (cfg.is_fcfs_input) {
        return fcfs_open(filename, flags);
    } else {
        return open(filename, flags);
    }
}

static inline void close_file(int fd)
{
    if (cfg.is_fcfs_input) {
        fcfs_close(fd);
    } else {
        close(fd);
    }
}

static int create_file(const char *filename, const int64_t start_offset)
{
#define BUFFER_SIZE  (4 * 1024 * 1024)
    const int flags = O_WRONLY | O_CREAT;
    int result;
    int bytes;
    int fd;
    int len;
    int64_t start_time_us;
    char *buff;
    char time_buff[32];
    unsigned char *p;
    unsigned char *end;
    int64_t remain;

    if (cfg.is_fcfs_input) {
        fd = fcfs_open(filename, flags, 0755);
    } else {
        fd = open(filename, flags, 0755);
    }
    if (fd < 0) {
        result = errno != 0 ? errno : ENOENT;
        logError("file: "__FILE__", line: %d, "
                "open file %s to write fail, errno: %d, error info: %s",
                __LINE__, filename, result, strerror(result));
        return result;
    }

    start_time_us = get_current_time_us();
    printf("creating file: %s ...\n", filename);
    if ((buff=fc_malloc(BUFFER_SIZE)) == NULL) {
        close_file(fd);
        return ENOMEM;
    }

    end = (unsigned char *)buff + BUFFER_SIZE;
    for (p=(unsigned char *)buff; p<end; p++) {
        *p = ((int64_t)rand() * 255) / (int64_t)RAND_MAX;
    }

    result = 0;
    remain = cfg.file_size - start_offset;
    while (remain > 0) {
        len = remain > BUFFER_SIZE ? BUFFER_SIZE : remain;
        if (cfg.is_fcfs_input) {
            bytes = fcfs_write(fd, buff, len);
        } else {
            bytes = write(fd, buff, len);
        }
        if (bytes != len) {
            result = errno != 0 ? errno : EIO;
            logError("file: "__FILE__", line: %d, "
                    "write to file %s fail, errno: %d, error info: %s",
                    __LINE__, filename, result, strerror(result));
            break;
        }

        remain -= len;
    }

    if (result == 0) {
        long_to_comma_str((get_current_time_us() -
                    start_time_us) / 1000, time_buff);
        printf("create file: %s successfully, time used: %s ms\n",
                filename, time_buff);
    }

    free(buff);
    close_file(fd);
    return result;
}

static int check_create_file(const char *filename)
{
    struct stat stbuf;
    int ret;
    int result;

    if (cfg.is_fcfs_input) {
        ret = fcfs_stat(filename, &stbuf);
    } else {
        ret = stat(filename, &stbuf);
    }

    if (ret != 0) {
        result = errno != 0 ? errno : EIO;
        if (result == ENOENT) {
            stbuf.st_size = 0;
        } else {
            logError("file: "__FILE__", line: %d, "
                    "stat file %s fail, errno: %d, error info: %s",
                    __LINE__, filename, result, strerror(result));
            return result;
        }
    }

    if (stbuf.st_size == cfg.file_size) {
        return 0;
    } else if (stbuf.st_size > cfg.file_size) {
        if (cfg.is_fcfs_input) {
            ret = fcfs_truncate(filename, cfg.file_size);
        } else {
            ret = truncate(filename, cfg.file_size);
        }

        if (ret == 0) {
            return 0;
        } else {
            result = errno != 0 ? errno : EIO;
            logError("file: "__FILE__", line: %d, "
                    "truncate file %s fail, errno: %d, error info: %s",
                    __LINE__, filename, result, strerror(result));
            return result;
        }
    }

    return create_file(filename, stbuf.st_size);
}

static int thread_run(BeachmarkThreadInfo *thread)
{
    int result;
    int flags;
    int fd;
    bool is_read;
    bool is_sequence;
    char *buff;
    int64_t blocks;
    int64_t offset;
    int size;
    int bytes;
    char *filename;

    buff = (char *)fc_malloc(cfg.buffer_size);
    if (buff == NULL) {
        return ENOMEM;
    }

    filename = fc_malloc(cfg.filename_prefix.len + 8);
    if (filename == NULL) {
        return ENOMEM;
    }
    sprintf(filename, "%s.%d", cfg.filename_prefix.str, thread->index);
    if ((result=check_create_file(filename)) != 0) {
        return result;
    }

    switch (cfg.mode.val) {
        case fcfs_beachmark_mode_read:
        case fcfs_beachmark_mode_randread:
            is_read = true;
            flags = O_RDONLY;
            break;
        case fcfs_beachmark_mode_write:
        case fcfs_beachmark_mode_randwrite:
            is_read = false;
            flags = O_WRONLY;
            break;
        default:
            return EINVAL;
    }

    is_sequence = (cfg.mode.val == fcfs_beachmark_mode_read ||
            cfg.mode.val == fcfs_beachmark_mode_write);
    if ((fd=open_file(filename, flags)) < 0) {
        result = errno != 0 ? errno : ENOENT;
        logError("file: "__FILE__", line: %d, "
                "open file %s to read fail, errno: %d, error info: %s",
                __LINE__, filename, result, strerror(result));
        return result;
    }

    __sync_add_and_fetch(&st.ready_count, 1);
    while (FC_ATOMIC_GET(st.continue_flag) &&
            !FC_ATOMIC_GET(st.ready_flag))
    {
        fc_sleep_ms(10);
    }

    offset = 0;
    size = cfg.buffer_size;
    blocks = cfg.file_size / cfg.buffer_size;
    while (FC_ATOMIC_GET(st.continue_flag)) {
        switch (cfg.mode.val) {
            case fcfs_beachmark_mode_read:
            case fcfs_beachmark_mode_write:
                size = cfg.file_size - offset;
                if (size <= 0) {
                    offset = 0;
                    size = cfg.buffer_size;
                } else if (size > cfg.buffer_size) {
                    size = cfg.buffer_size;
                }
                break;
            case fcfs_beachmark_mode_randread:
            case fcfs_beachmark_mode_randwrite:
                offset = (((int64_t)rand() * blocks) / (int64_t)RAND_MAX)
                    * cfg.buffer_size;
                break;
            default:
                break;
        }

        if (is_read) {
            if (cfg.is_fcfs_input) {
                bytes = fcfs_pread(fd, buff, size, offset);
            } else {
                bytes = pread(fd, buff, size, offset);
            }
        } else {
            if (cfg.is_fcfs_input) {
                bytes = fcfs_pwrite(fd, buff, size, offset);
            } else {
                bytes = pwrite(fd, buff, size, offset);
            }
        }
        if (bytes != size) {
            result = errno != 0 ? errno : EIO;
            logError("file: "__FILE__", line: %d, "
                    "read from file %s fail, errno: %d, error info: %s",
                    __LINE__, filename, result, strerror(result));
            return result;
        }

        FC_ATOMIC_INC(thread->success_count);
        if (is_sequence) {
            offset += bytes;
        }
    }

    close_file(fd);
    return 0;
}

static void *thread_entrance(void *arg)
{
    BeachmarkThreadInfo *thread;

    thread = arg;
    __sync_add_and_fetch(&st.running_count, 1);
    thread_run(thread);
    __sync_sub_and_fetch(&st.running_count, 1);
    return NULL;
}

static void output(const time_t current_time)
{
    BeachmarkThreadInfo *thread;
    static time_t last_time = 0;
    static int64_t last_count = 0;
    int64_t total_count;
    int time_distance;

    if (last_time == 0) {
        last_time = st.start_time;
        printf("\n");
    }

    total_count = 0;
    for (thread=st.threads; thread<st.tend; thread++) {
        total_count += FC_ATOMIC_GET(thread->success_count);
    }

    time_distance = current_time - last_time;
    if (time_distance > 0) {
        st.iops.cur = (total_count - last_count) / time_distance;
        if (st.iops.cur > st.iops.max) {
            st.iops.max = st.iops.cur;
        }
        st.iops.avg = total_count / (current_time - st.start_time);
        long_to_comma_str(st.iops.cur, st.iops_buff.cur);
        long_to_comma_str(st.iops.avg, st.iops_buff.avg);
        printf("running time: %4d seconds, %s IOPS {current: %s, avg: %s}\n",
                (int)(current_time - st.start_time), cfg.mode.str,
                st.iops_buff.cur, st.iops_buff.avg);
        last_time = current_time;
        last_count = total_count;
    }
}

static void sigQuitHandler(int sig)
{
    if (FC_ATOMIC_GET(st.continue_flag)) {
        FC_ATOMIC_SET(st.continue_flag, 0);
        printf("file: "__FILE__", line: %d, "
                "catch signal %d, program exiting...\n",
                __LINE__, sig);
    }
}

static int setup_signal_handler()
{
    struct sigaction act;

    memset(&act, 0, sizeof(act));
    sigemptyset(&act.sa_mask);
    act.sa_handler = sigQuitHandler;
    if(sigaction(SIGINT, &act, NULL) < 0 ||
            sigaction(SIGTERM, &act, NULL) < 0 ||
            sigaction(SIGQUIT, &act, NULL) < 0)
    {
        fprintf(stderr, "file: "__FILE__", line: %d, "
                "call sigaction fail, errno: %d, error info: %s\n",
                __LINE__, errno, strerror(errno));
        return errno != 0 ? errno : EBUSY;
    }

    return 0;
}

static int beachmark()
{
    const char *log_prefix_name = NULL;
	int result;
    int count;
    int bytes;
    long i;
    time_t current_time;
    time_t end_time;
    pthread_t *tids;
    void **args;
    char buffer_size_prompt[32];
    BeachmarkThreadInfo *thread;

    if ((result=fcfs_posix_api_init(log_prefix_name,
                    cfg.ns, cfg.config_filename)) != 0)
    {
        return result;
    }
    if ((result=fcfs_posix_api_start()) != 0) {
        return result;
    }

    if ((result=setup_signal_handler()) != 0) {
        return result;
    }

    fcfs_posix_api_log_configs();
    cfg.is_fcfs_input = FCFS_API_IS_MY_MOUNTPOINT(cfg.filename_prefix.str);

    bytes = sizeof(BeachmarkThreadInfo) * cfg.thread_count;
    st.threads = fc_malloc(bytes);
    if (st.threads == NULL) {
        return ENOMEM;
    }
    memset(st.threads, 0, bytes);
    st.tend = st.threads + cfg.thread_count;

    tids = fc_malloc(sizeof(pthread_t) * cfg.thread_count);
    if (tids == NULL) {
        return ENOMEM;
    }

    args = fc_malloc(sizeof(void *) * cfg.thread_count);
    if (args == NULL) {
        return ENOMEM;
    }

    for (i=0, thread=st.threads; i<cfg.thread_count; i++, thread++) {
        thread->index = i;
        args[i] = thread;
    }

    count = cfg.thread_count;
    if ((result=create_work_threads(&count, thread_entrance,
                    args, tids, 256 * 1024)) != 0)
    {
        return result;
    }

    if (cfg.buffer_size % 1024 != 0) {
        long_to_comma_str(cfg.buffer_size, buffer_size_prompt);
    } else {
        sprintf(buffer_size_prompt, "%d KB", cfg.buffer_size / 1024);
    }
    printf("\nthreads: %d, mode: %s, file size: %"PRId64" MB, "
            "buffer size: %s\n\n", cfg.thread_count, cfg.mode.str,
            cfg.file_size / (1024 * 1024), buffer_size_prompt);

    fc_sleep_ms(100);
    while (FC_ATOMIC_GET(st.continue_flag) &&
            FC_ATOMIC_GET(st.ready_count) <
            FC_ATOMIC_GET(st.running_count))
    {
        fc_sleep_ms(10);
    }
    FC_ATOMIC_SET(st.ready_flag, 1);

    st.start_time = time(NULL);
    end_time = st.start_time + cfg.runtime;
    do {
        sleep(1);
        current_time = time(NULL);
        output(current_time);
    } while (FC_ATOMIC_GET(st.continue_flag) &&
            FC_ATOMIC_GET(st.running_count) > 0 &&
            current_time < end_time);

    long_to_comma_str(st.iops.avg, st.iops_buff.avg);
    long_to_comma_str(st.iops.max, st.iops_buff.max);
    printf("\nrunning time: %4d seconds, %s IOPS {avg: %s, max: %s}\n",
            (int)(current_time - st.start_time), cfg.mode.str,
            st.iops_buff.avg, st.iops_buff.max);

    FC_ATOMIC_SET(st.continue_flag, 0);
    while (FC_ATOMIC_GET(st.running_count) > 0) {
        sleep(1);
    }

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

    log_try_init();
    while ((ch=getopt(argc, argv, "hc:m:n:b:T:t:f:s:")) != -1) {
        switch (ch) {
            case 'h':
                usage(argv);
                return 0;
            case 'c':
                cfg.config_filename = optarg;
                break;
            case 'm':
                if (strcasecmp(optarg, "read") == 0) {
                    cfg.mode.val = fcfs_beachmark_mode_read;
                    cfg.mode.str = "read";
                } else if (strcasecmp(optarg, "write") == 0) {
                    cfg.mode.val = fcfs_beachmark_mode_write;
                    cfg.mode.str = "write";
                } else if (strcasecmp(optarg, "randread") == 0) {
                    cfg.mode.val = fcfs_beachmark_mode_randread;
                    cfg.mode.str = "randread";
                } else if (strcasecmp(optarg, "randwrite") == 0) {
                    cfg.mode.val = fcfs_beachmark_mode_randwrite;
                    cfg.mode.str = "randwrite";
                } else {
                    logError("file: "__FILE__", line: %d, "
                            "invalid mode: %s!", __LINE__, optarg);
                    usage(argv);
                    return EINVAL;
                }
                break;
            case 'n':
                cfg.ns = optarg;
                break;
            case 's':
                if ((result=parse_bytes(optarg, 1, &cfg.file_size)) != 0) {
                    usage(argv);
                    return result;
                }
                break;
            case 'T':
                cfg.thread_count = strtol(optarg, NULL, 10);
                if (cfg.thread_count <= 0) {
                    logError("file: "__FILE__", line: %d, "
                            "invalid thread count: %d which <= 0",
                            __LINE__, cfg.thread_count);
                    return EINVAL;
                }
                break;
            case 't':
                cfg.runtime = strtol(optarg, NULL, 10);
                if (cfg.runtime <= 0) {
                    logError("file: "__FILE__", line: %d, "
                            "invalid runtime: %d which <= 0",
                            __LINE__, cfg.runtime);
                    return EINVAL;
                }
                break;
            case 'b':
                if ((result=parse_bytes(optarg, 1, &bytes)) != 0) {
                    usage(argv);
                    return result;
                }
                if (bytes <= 0) {
                    logError("file: "__FILE__", line: %d, "
                            "invalid buffer size: %"PRId64" which <= 0",
                            __LINE__, bytes);
                    return EINVAL;
                }
                cfg.buffer_size = bytes;
                break;
            case 'f':
                FC_SET_STRING(cfg.filename_prefix, optarg);
                break;
            default:
                usage(argv);
                return 1;
        }
    }

    if (cfg.filename_prefix.str == NULL) {
        fprintf(stderr, "expect parameter -f filename_prefix\n\n");
        usage(argv);
        return EINVAL;
    }

    if (cfg.file_size < cfg.buffer_size) {
        logError("file: "__FILE__", line: %d, "
                "invalid file size: %"PRId64" which < buffer size: %d",
                __LINE__, cfg.file_size, cfg.buffer_size);
        return EINVAL;
    }

    srand(time(NULL));
    return beachmark();
}
