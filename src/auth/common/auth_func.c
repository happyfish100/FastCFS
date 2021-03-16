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

#include "fastcommon/system_info.h"
#include "fastcommon/sched_thread.h"
#include "fastcommon/md5.h"
#include "auth_func.h"

void fcfs_auth_generate_passwd(unsigned char passwd[16])
{
    struct {
        time_t current_time;
        pid_t pid;
        int random;
#if defined(OS_LINUX) || defined(OS_FREEBSD)
        struct fast_sysinfo si;
#endif
        int hash_codes[4];
    } input;

    input.current_time = get_current_time();
    input.pid = getpid();
    input.random = rand();

#if defined(OS_LINUX) || defined(OS_FREEBSD)
    get_sysinfo(&input.si);
#endif

    logInfo("procs: %d, pid: %d, random: %d", input.si.procs, input.pid, input.random);

    INIT_HASH_CODES4(input.hash_codes);
    CALC_HASH_CODES4(&input, sizeof(input), input.hash_codes);
    FINISH_HASH_CODES4(input.hash_codes);

    my_md5_buffer((char *)&input, sizeof(input), passwd);
}
