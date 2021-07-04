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
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include "fastcommon/shared_func.h"
#include "fastcommon/pthread_func.h"
#include "fastcommon/process_ctrl.h"
#include "fastcommon/logger.h"
#include "fastcommon/sockopt.h"
#include "fastcommon/sched_thread.h"
#include "sf/sf_global.h"
#include "sf/sf_func.h"
#include "sf/sf_nio.h"
#include "sf/sf_service.h"
#include "sf/sf_util.h"
#include "common/auth_proto.h"
#include "server_global.h"
#include "server_func.h"
#include "session_subscribe.h"
#include "cluster_relationship.h"
#include "common_handler.h"
#include "cluster_handler.h"
#include "service_handler.h"

static int setup_server_env(const char *config_filename);

static bool daemon_mode = true;
static const char *config_filename;
static char g_pid_filename[MAX_PATH_SIZE];

static int process_cmdline(int argc, char *argv[], bool *continue_flag)
{
    char *action;
    bool stop;
    int result;

    *continue_flag = false;
    if (argc < 2) {
        sf_usage(argv[0]);
        return 1;
    }

    config_filename = sf_parse_daemon_mode_and_action(argc, argv,
            &g_fcfs_auth_global_vars.version, &daemon_mode, &action);
    if (config_filename == NULL) {
        return 0;
    }

    log_init2();
    //log_set_time_precision(&g_log_context, LOG_TIME_PRECISION_USECOND);

    result = get_base_path_from_conf_file(config_filename,
            SF_G_BASE_PATH_STR, sizeof(SF_G_BASE_PATH_STR));
    if (result != 0) {
        log_destroy();
        return result;
    }

    snprintf(g_pid_filename, sizeof(g_pid_filename), 
             "%s/authd.pid", SF_G_BASE_PATH_STR);

    stop = false;
    result = process_action(g_pid_filename, action, &stop);
    if (result != 0) {
        if (result == EINVAL) {
            sf_usage(argv[0]);
        }
        log_destroy();
        return result;
    }

    if (stop) {
        log_destroy();
        return 0;
    }

    *continue_flag = true;
    return 0;
}

int main(int argc, char *argv[])
{
    pthread_t schedule_tid;
    int wait_count;
    int result;

    result = process_cmdline(argc, argv, (bool *)&SF_G_CONTINUE_FLAG);
    if (!SF_G_CONTINUE_FLAG) {
        return result;
    }

    sf_enable_exit_on_oom();
    srand(time(NULL));
    fast_mblock_manager_init();

    //sched_set_delay_params(300, 1024);
    do {
        if ((result=setup_server_env(config_filename)) != 0) {
            break;
        }

        if ((result=sf_startup_schedule(&schedule_tid)) != 0) {
            break;
        }

        if ((result=sf_add_slow_log_schedule(&g_server_global_vars.
                        slow_log)) != 0)
        {
            break;
        }

        if ((result=sf_socket_server()) != 0) {
            break;
        }
        if ((result=sf_socket_server_ex(&CLUSTER_SF_CTX)) != 0) {
            break;
        }

        if ((result=write_to_pid_file(g_pid_filename)) != 0) {
            break;
        }

        if ((result=service_handler_init()) != 0) {
            break;
        }

        if ((result=session_subscribe_init()) != 0) {
            break;
        }

        common_handler_init();
        //sched_print_all_entries();

        result = sf_service_init_ex(&CLUSTER_SF_CTX, "cluster",
                cluster_alloc_thread_extra_data,
                cluster_thread_loop_callback, NULL,
                sf_proto_set_body_length, cluster_deal_task,
                cluster_task_finish_cleanup, cluster_recv_timeout_callback,
                1000, sizeof(FCFSAuthProtoHeader), sizeof(AuthServerTaskArg));
        if (result != 0) {
            break;
        }
        sf_enable_thread_notify_ex(&CLUSTER_SF_CTX, true);
        sf_set_remove_from_ready_list_ex(&CLUSTER_SF_CTX, false);
        sf_accept_loop_ex(&CLUSTER_SF_CTX, false);

        result = sf_service_init_ex(&g_sf_context, "service",
                service_alloc_thread_extra_data, NULL,
                NULL, sf_proto_set_body_length, service_deal_task,
                service_task_finish_cleanup, NULL, 1000,
                sizeof(FCFSAuthProtoHeader), sizeof(AuthServerTaskArg));
        if (result != 0) {
            break;
        }
        sf_enable_thread_notify(true);
        sf_set_remove_from_ready_list(false);

        if ((result=cluster_relationship_init()) != 0) {
            break;
        }
    } while (0);

    if (result != 0) {
        lcrit("program exit abnomally");
        log_destroy();
        return result;
    }

    //sched_print_all_entries();
    sf_accept_loop();

    if (g_schedule_flag) {
        pthread_kill(schedule_tid, SIGINT);
    }

    wait_count = 0;
    while ((SF_G_ALIVE_THREAD_COUNT != 0) || g_schedule_flag) {
        fc_sleep_ms(10);
        if (++wait_count > 1000) {
            lwarning("waiting timeout, exit!");
            break;
        }
    }

    sf_service_destroy();
    delete_pid_file(g_pid_filename);
    logInfo("file: "__FILE__", line: %d, "
            "program exit normally.\n", __LINE__);
    log_destroy();
    return 0;
}

static int setup_server_env(const char *config_filename)
{
    int result;

    sf_set_current_time();
    if ((result=server_load_config(config_filename)) != 0) {
        return result;
    }

    if (daemon_mode) {
        daemon_init(false);
    }
    umask(0);

    result = sf_setup_signal_handler();

    log_set_cache(true);
    return result;
}
