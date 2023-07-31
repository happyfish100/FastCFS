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

typedef struct {
    int index;
    volatile int64_t success_count;
} BeachmarkThreadInfo;

static struct {
    const char *config_filename;
    char *ns;
    int buffer_size;
    int thread_count;
    int runtime;
    string_t filename_prefix;
    bool is_fcfs_input;
} cfg = {FCFS_FUSE_DEFAULT_CONFIG_FILENAME,
    "fs", 4 * 1024, 1, 60, {NULL, 0}, false};

static struct {
    volatile int running_count;
    volatile bool continue_flag;
    time_t start_time;
    BeachmarkThreadInfo *threads;
    BeachmarkThreadInfo *tend;
} st = {0, true};

static void usage(char *argv[])
{
    fprintf(stderr, "Usage: %s [-c config_filename=%s] "
            "[-n namespace=fs] [-b buffer_size=4KB] "
            "[-T threads=1] [-t runtime=60] "
            "<-f filename_prefix>\n\n", argv[0],
            FCFS_FUSE_DEFAULT_CONFIG_FILENAME);
}

static int thread_run(BeachmarkThreadInfo *thread)
{
    int result;
    int fd;
    char *buff;
    int read_bytes;
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

    if (cfg.is_fcfs_input) {
        fd = fcfs_open(filename, O_RDONLY);
    } else {
        fd = open(filename, O_RDONLY);
    }
    if (fd < 0) {
        result = errno != 0 ? errno : ENOENT;
        logError("file: "__FILE__", line: %d, "
                "open file %s to read fail, "
                "errno: %d, error info: %s",
                __LINE__, filename,
                result, strerror(result));
        return result;
    }

    while (st.continue_flag) {
        if (cfg.is_fcfs_input) {
            read_bytes = fcfs_read(fd, buff, cfg.buffer_size);
        } else {
            read_bytes = read(fd, buff, cfg.buffer_size);
        }
        if (read_bytes < 0) {
            result = errno != 0 ? errno : ENOENT;
            logError("file: "__FILE__", line: %d, "
                    "read from file %s fail, errno: %d, error info: %s",
                    __LINE__, filename, result, strerror(result));
            return result;
        }

        if (read_bytes == cfg.buffer_size) {
            thread->success_count++;
        } else {
            if (cfg.is_fcfs_input) {
               if (fcfs_lseek(fd, 0, SEEK_SET) < 0) {
                   result = errno != 0 ? errno : ENOENT;
               } else {
                   result = 0;
               }
            } else {
                if (lseek(fd, 0, SEEK_SET) < 0) {
                    result = errno != 0 ? errno : ENOENT;
                } else {
                    result = 0;
                }
            }
                
            if (result != 0) {
                logError("file: "__FILE__", line: %d, "
                        "lseek file %s fail, errno: %d, error info: %s",
                        __LINE__, filename, result, strerror(result));
                return result;
            }
        }
    }

    if (cfg.is_fcfs_input) {
        fcfs_close(fd);
    } else {
        close(fd);
    }
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
    int64_t total_count;

    total_count = 0;
    for (thread=st.threads; thread<st.tend; thread++) {
        total_count += thread->success_count;
    }
    printf("running threads: %d, IOPS: %d\n", FC_ATOMIC_GET(
                st.running_count), (int)(total_count /
                    (current_time - st.start_time)));
}

static void sigQuitHandler(int sig)
{
    if (st.continue_flag) {
        st.continue_flag = false;
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

    st.start_time = time(NULL);
    end_time = st.start_time + cfg.runtime;
    do {
        sleep(1);
        current_time = time(NULL);
        output(current_time);
    } while (st.continue_flag && FC_ATOMIC_GET(st.running_count) > 0 &&
            current_time < end_time);

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
    while ((ch=getopt(argc, argv, "hc:n:b:T:t:f:")) != -1) {
        switch (ch) {
            case 'h':
                usage(argv);
                return 0;
            case 'c':
                cfg.config_filename = optarg;
                break;
            case 'n':
                cfg.ns = optarg;
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

    return beachmark();
}
