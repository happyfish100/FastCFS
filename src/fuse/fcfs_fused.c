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

static struct fuse_session *fuse_instance;

static int setup_server_env(const char *config_filename);
static struct fuse_session *create_fuse_session(char *argv0,
        struct fuse_lowlevel_ops *ops);
static void fuse_exit_handler(int sig);

static bool daemon_mode = true;
static const char *config_filename;
static char g_pid_filename[MAX_PATH_SIZE];

static void parse_cmd_options(int argc, char *argv[])
{
    int ch;
    struct option longopts[] = {
        {"user", required_argument, NULL, 'u'},
        {"key",  required_argument, NULL, 'k'},
        {"namespace",  required_argument, NULL, 'n'},
        {"mountpoint", required_argument, NULL, 'm'},
        {NULL, 0, NULL, 0}
    };

    while ((ch = getopt_long(argc, argv, "u:k:n:m:",
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
                g_fuse_global_vars.ns = optarg;
                break;
            case 'm':
                g_fuse_global_vars.mountpoint = optarg;
                break;
            default:
                //just ignore other options
                break;
        }
    }
}

static int process_cmdline(int argc, char *argv[], bool *continue_flag)
{
    char *strs[4];
    str_ptr_array_t options;
    char *action;
    bool stop;
    int result;

    options.strs = strs;
    options.count = 0;
    strs[options.count++] = "-u | --user: the username";
    strs[options.count++] = "-k | --key: the secret key filename";
    strs[options.count++] = "-n | --namespace: the FastDIR namespace "
        "or the storage pool name";
    strs[options.count++] = "-m | --mountpoint: the mountpoint";

    *continue_flag = false;
    if (argc < 2) {
        sf_usage_ex1(argv[0], &options);
        return 1;
    }

    config_filename = sf_parse_daemon_mode_and_action_ex1(
            argc, argv, &g_fcfs_global_vars.version,
            &daemon_mode, &action, "start", &options);
    if (config_filename == NULL) {
        return 0;
    }

    log_init2();
    //log_set_time_precision(&g_log_context, LOG_TIME_PRECISION_MSECOND);

    result = get_base_path_from_conf_file(config_filename,
            SF_G_BASE_PATH, sizeof(SF_G_BASE_PATH));
    if (result != 0) {
        log_destroy();
        return result;
    }

    snprintf(g_pid_filename, sizeof(g_pid_filename),
            "%s/fused.pid", SF_G_BASE_PATH);

    stop = false;
    result = process_action(g_pid_filename, action, &stop);
    if (result != 0) {
        if (result == EINVAL) {
            sf_usage_ex1(argv[0], &options);
        }
        log_destroy();
        return result;
    }

    if (stop) {
        log_destroy();
        return 0;
    }

    parse_cmd_options(argc, argv);
    *continue_flag = true;
    return 0;
}

int main(int argc, char *argv[])
{
    pthread_t schedule_tid;
    int wait_count;
    struct fuse_lowlevel_ops fuse_operations;
    struct fuse_session *se;
    int result;

    result = process_cmdline(argc, argv, (bool *)&SF_G_CONTINUE_FLAG);
    if (!SF_G_CONTINUE_FLAG) {
        return result;
    }

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

        if ((result=fuse_session_mount(se, g_fuse_global_vars.
                        mountpoint)) != 0)
        {
            break;
        }

        if ((result=write_to_pid_file(g_pid_filename)) != 0) {
            break;
        }

        /* Block until ctrl+c or fusermount -u */
        if (g_fuse_global_vars.singlethread) {
            result = fuse_session_loop(se);
        } else {
            struct fuse_loop_config fuse_config;
            fuse_config.clone_fd = g_fuse_global_vars.clone_fd;
            fuse_config.max_idle_threads = g_fuse_global_vars.max_idle_threads;
            result = fuse_session_loop_mt(se, &fuse_config);
        }

        fuse_session_unmount(se);
        fcfs_api_terminate();
        delete_pid_file(g_pid_filename);
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
    log_destroy();

	return result < 0 ? 1 : result;
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

            FC_SET_STRING(fullname.ns, g_fuse_global_vars.ns);
            FC_SET_STRING(fullname.path, "/");

            FCFS_FUSE_SET_OMP(omp, (0777 | S_IFDIR), geteuid(), getegid());
            result = fdir_client_create_dentry(g_fcfs_api_ctx.contexts.fdir,
                    &fullname, &omp, &dentry);
        }
    }

    return result;
}

static int setup_server_env(const char *config_filename)
{
    int result;

    sf_set_current_time();
    if ((result=fcfs_fuse_global_init(config_filename)) != 0) {
        return result;
    }

    if (daemon_mode) {
        daemon_init(false);
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
    log_set_cache(true);

    if (g_fdir_client_vars.client_ctx.idempotency_enabled ||
            g_fs_client_vars.client_ctx.idempotency_enabled)
    {
        if ((result=receipt_handler_init()) != 0) {
            return result;
        }
    }

    if ((result=check_create_root_path()) != 0) {
        return result;
    }

    return fcfs_api_start();
}

static struct fuse_session *create_fuse_session(char *argv0,
        struct fuse_lowlevel_ops *ops)
{
	struct fuse_args args;
    char *argv[8];
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

    args.argc = argc;
    args.argv = argv;
    args.allocated = 0;
    return fuse_session_new(&args, ops, sizeof(*ops), NULL);
}

static void fuse_exit_handler(int sig)
{
    if (fuse_instance != NULL) {
        fuse_session_exit(fuse_instance);
        fuse_instance = NULL;
    }
}
