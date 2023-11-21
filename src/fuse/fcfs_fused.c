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
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <getopt.h>
#include "fastcommon/logger.h"
#include "fastcommon/process_ctrl.h"
#include "fastcommon/sched_thread.h"
#include "sf/sf_global.h"
#include "sf/sf_service.h"
#include "sf/sf_func.h"
#include "sf/sf_util.h"
#include "sf/idempotency/client/client_channel.h"
#include "sf/idempotency/client/receipt_handler.h"
#include "global.h"
#include "fuse_wrapper.h"

//#define FDIR_MBLOCK_CHECK  1

#ifdef FDIR_MBLOCK_CHECK
static int setup_mblock_stat_task();
#endif

static struct fuse_session *fuse_instance;

static int setup_server_env(const char *config_filename);
static struct fuse_session *create_fuse_session(char *argv0,
        struct fuse_lowlevel_ops *ops);
static void fuse_exit_handler(int sig);

static bool daemon_mode = true;
static bool need_delete_pid_file = false;
static const char *config_filename;
static char g_pid_filename[MAX_PATH_SIZE];

static int parse_cmd_options(int argc, char *argv[])
{
    int ch;
    const struct option longopts[] = {
        {"user", required_argument, NULL, 'u'},
        {"key",  required_argument, NULL, 'k'},
        {"namespace",  required_argument, NULL, 'n'},
        {"mountpoint", required_argument, NULL, 'm'},
        {"base-path",  required_argument, NULL, 'b'},
        SF_COMMON_LONG_OPTIONS,
        {NULL, 0, NULL, 0}
    };

    while ((ch = getopt_long(argc, argv, SF_COMMON_OPT_STRING"u:k:n:m:b:",
                    longopts, NULL)) != -1)
    {
        switch (ch) {
            case 'u':
                FC_SET_STRING(g_fcfs_auth_client_vars.client_ctx.
                        auth_cfg.username, optarg);
                break;
            case 'k':
                FC_SET_STRING(g_fcfs_auth_client_vars.client_ctx.
                        auth_cfg.secret_key_filename, optarg);
                break;
            case 'n':
                g_fuse_global_vars.nsmp.ns = optarg;
                break;
            case 'm':
                g_fuse_global_vars.nsmp.mountpoint = optarg;
                break;
            case 'b':
                sf_set_global_base_path(optarg);
                break;
            case '?':
                return EINVAL;
            default:
                break;
        }
    }

    return 0;
}

#define OPTION_NAME_USER_STR "user"
#define OPTION_NAME_USER_LEN (sizeof(OPTION_NAME_USER_STR) - 1)

#define OPTION_NAME_KEY_STR  "key"
#define OPTION_NAME_KEY_LEN  (sizeof(OPTION_NAME_KEY_STR) - 1)

#define OPTION_NAME_NAMESPACE_STR  "namespace"
#define OPTION_NAME_NAMESPACE_LEN  (sizeof(OPTION_NAME_NAMESPACE_STR) - 1)

#define OPTION_NAME_MOUNTPOINT_STR "mountpoint"
#define OPTION_NAME_MOUNTPOINT_LEN (sizeof(OPTION_NAME_MOUNTPOINT_STR) - 1)

#define OPTION_NAME_BASE_PATH_STR "base-path"
#define OPTION_NAME_BASE_PATH_LEN (sizeof(OPTION_NAME_BASE_PATH_STR) - 1)

static int process_cmdline(int argc, char *argv[], bool *continue_flag)
{
    const SFCMDOption other_options[] = {
        {{OPTION_NAME_USER_STR, OPTION_NAME_USER_LEN},
            'u', true, "-u | --user: the username"},
        {{OPTION_NAME_KEY_STR, OPTION_NAME_KEY_LEN},
            'k', true, "-k | --key: the secret key filename"},
        {{OPTION_NAME_NAMESPACE_STR, OPTION_NAME_NAMESPACE_LEN},
            'n', true, "-n | --namespace: the FastDIR namespace"},
        {{OPTION_NAME_MOUNTPOINT_STR, OPTION_NAME_MOUNTPOINT_LEN},
            'm', true, "-m | --mountpoint: the mountpoint"},
        {{OPTION_NAME_BASE_PATH_STR, OPTION_NAME_BASE_PATH_LEN},
            'b', true, "-b | --base-path: the base path"},
        {{NULL, 0}, 0, false, NULL}
    };
    char *action;
    bool stop;
    int result;

    *continue_flag = false;
    if (argc < 2) {
        sf_usage_ex(argv[0], other_options);
        return 1;
    }

    config_filename = sf_parse_daemon_mode_and_action_ex(
            argc, argv, &g_fcfs_global_vars.version,
            &daemon_mode, &action, "start", other_options);
    if (config_filename == NULL) {
        return 0;
    }

    log_init2();
    //log_set_time_precision(&g_log_context, LOG_TIME_PRECISION_MSECOND);

    if ((result=parse_cmd_options(argc, argv)) != 0) {
        return result;
    }

    if (!SF_G_BASE_PATH_INITED) {
        result = sf_get_base_path_from_conf_file(config_filename);
        if (result != 0) {
            log_destroy();
            return result;
        }
        SF_G_BASE_PATH_INITED = true;
    }

    snprintf(g_pid_filename, sizeof(g_pid_filename),
            "%s/fused.pid", SF_G_BASE_PATH_STR);

    stop = false;
    result = process_action(g_pid_filename, action, &stop);
    if (result != 0) {
        if (result == EINVAL) {
            sf_usage_ex(argv[0], other_options);
        }
        log_destroy();
        return result;
    }

    if (stop) {
        log_destroy();
        return 0;
    }

    *continue_flag = true;
    return result;
}

int main(int argc, char *argv[])
{
    int result;
    int wait_count;
    pthread_t schedule_tid;
    struct fuse_lowlevel_ops fuse_operations;
    struct fuse_session *se;

    result = process_cmdline(argc, argv, (bool *)&SF_G_CONTINUE_FLAG);
    if (!SF_G_CONTINUE_FLAG) {
        return result;
    }

#ifdef FDIR_MBLOCK_CHECK
    fast_mblock_manager_init();
#endif

    sf_enable_exit_on_oom();

    do {
        if ((result=setup_server_env(config_filename)) != 0) {
            break;
        }

        if ((result=fs_fuse_wrapper_init(&fuse_operations)) != 0) {
            break;
        }

        if ((result=sf_startup_schedule(&schedule_tid)) != 0) {
            break;
        }

        if ((se=create_fuse_session(argv[0], &fuse_operations)) == NULL) {
            result = ENOMEM;
            break;
        }

        fuse_instance = se;
        sf_set_sig_quit_handler(fuse_exit_handler);

#ifdef FDIR_MBLOCK_CHECK
    setup_mblock_stat_task();
#endif

        if ((result=fuse_session_mount(se, g_fuse_global_vars.
                        nsmp.mountpoint)) != 0)
        {
            break;
        }

        /* Block until ctrl+c or fusermount -u */
        if (g_fuse_global_vars.singlethread) {
            result = fuse_session_loop(se);
        } else {
            struct {
#if FUSE_VERSION < FUSE_MAKE_VERSION(3, 12)
                struct fuse_loop_config holder;
#endif
                struct fuse_loop_config *ptr;
            } fuse_config;

#if FUSE_VERSION < FUSE_MAKE_VERSION(3, 12)
            fuse_config.ptr = &fuse_config.holder;
            fuse_config.holder.clone_fd = g_fuse_global_vars.clone_fd;
            fuse_config.holder.max_idle_threads =
                g_fuse_global_vars.max_idle_threads;
#else
            fuse_config.ptr = fuse_loop_cfg_create();
            fuse_loop_cfg_set_clone_fd(fuse_config.ptr,
                    g_fuse_global_vars.clone_fd);
            if (g_fuse_global_vars.max_idle_threads <
                    g_fuse_global_vars.max_threads)
            {
                fuse_loop_cfg_set_idle_threads(fuse_config.ptr,
                        g_fuse_global_vars.max_idle_threads);
            }
            fuse_loop_cfg_set_max_threads(fuse_config.ptr,
                    g_fuse_global_vars.max_threads);
#endif
            result = fuse_session_loop_mt(se, fuse_config.ptr);
        }

        fuse_session_unmount(se);
        fcfs_api_terminate();
    } while (0);

    if (g_schedule_flag) {
        pthread_kill(schedule_tid, SIGINT);
    }

    wait_count = 0;
    while (g_schedule_flag) {
        fc_sleep_ms(100);
        if (++wait_count > 100) {
            lwarning("waiting timeout, exit!");
            break;
        }
    }

    if (result == 0) {
        logInfo("file: "__FILE__", line: %d, "
                "program exit normally.\n", __LINE__);
    } else {
        logCrit("file: "__FILE__", line: %d, "
                "program exit abnormally with errno: %d!\n",
                __LINE__, result);
    }

    if (need_delete_pid_file) {
        delete_pid_file(g_pid_filename);
    }
    log_destroy();

	return result < 0 ? 1 : result;
}

static int setup_server_env(const char *config_filename)
{
    int result;

    if ((result=sf_global_init("fcfs_fused")) != 0) {
        return result;
    }
    if (daemon_mode) {
        daemon_init(false);
    }

    if ((result=fcfs_fuse_global_init(config_filename)) != 0) {
        return result;
    }
    umask(0);

    log_set_use_file_write_lock(true);
    if ((result=log_reopen()) != 0) {
        if (result == EAGAIN || result == EACCES) {
            logCrit("file: "__FILE__", line: %d, "
                    "the process already running, please kill "
                    "the old process then try again!", __LINE__);
        }
        return result;
    }

    if ((result=sf_setup_signal_handler()) != 0) {
        return result;
    }

    if ((result=write_to_pid_file(g_pid_filename)) != 0) {
        return result;
    }

    log_set_cache(true);
    need_delete_pid_file = true;
    return fcfs_api_start_ex(&g_fcfs_api_ctx);
}

static struct fuse_session *create_fuse_session(char *argv0,
        struct fuse_lowlevel_ops *ops)
{
    struct fuse_args args;
    char *argv[16];
    int argc;

    argc = 0;
    argv[argc++] = argv0;

    if (g_log_context.log_level == LOG_DEBUG) {
        argv[argc++] = "-d";
    }

    if (g_fuse_global_vars.auto_unmount) {
        argv[argc++] = "-o";
        argv[argc++] = "auto_unmount";
    }

    if (g_fuse_global_vars.allow_others == allow_root) {
        argv[argc++] = "-o";
        argv[argc++] = "allow_root";
    } else if (g_fuse_global_vars.allow_others == allow_all) {
        argv[argc++] = "-o";
        argv[argc++] = "allow_other";
    }

    if (g_fuse_global_vars.writeback_cache) {
        argv[argc++] = "-o";
        argv[argc++] = "writeback_cache";

        argv[argc++] = "-o";
        argv[argc++] = "time_gran=1000000000";
    }

    if (g_fuse_global_vars.read_only) {
        argv[argc++] = "-o";
        argv[argc++] = "ro";
    }

    args.argc = argc;
    args.argv = argv;
    args.allocated = 0;
    if ((g_fuse_cinfo_opts=fuse_parse_conn_info_opts(&args)) == NULL) {
        logError("file: "__FILE__", line: %d, "
                "fuse_parse_conn_info_opts fail!", __LINE__);
        return NULL;
    }

    return fuse_session_new(&args, ops, sizeof(*ops), NULL);
}

static void fuse_exit_handler(int sig)
{
    if (fuse_instance != NULL) {
        fuse_session_exit(fuse_instance);
        fuse_instance = NULL;
    }
}

#ifdef FDIR_MBLOCK_CHECK
static int mblock_stat_task_func(void *args)
{
    fast_mblock_manager_stat_print_ex(false, FAST_MBLOCK_ORDER_BY_ELEMENT_SIZE);
    return 0;
}

static int setup_mblock_stat_task()
{
    ScheduleEntry schedule_entry;
    ScheduleArray schedule_array;

    INIT_SCHEDULE_ENTRY(schedule_entry, sched_generate_next_id(),
            0, 0, 0, 60, mblock_stat_task_func, NULL);
    schedule_entry.new_thread = true;

    schedule_array.count = 1;
    schedule_array.entries = &schedule_entry;
    return sched_add_entries(&schedule_array);
}
#endif
