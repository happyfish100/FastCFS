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


#ifndef _SERVER_GLOBAL_H
#define _SERVER_GLOBAL_H

#include "fastcommon/common_define.h"
#include "sf/sf_global.h"
#include "common/auth_global.h"
#include "server_types.h"

typedef struct server_global_vars {
    struct {
        string_t username;
        string_t secret_key;
    } admin;

    SFSlowLogContext slow_log;
} AuthServerGlobalVars;

#define SLOW_LOG_CFG            g_server_global_vars.slow_log.cfg
#define SLOW_LOG_CTX            g_server_global_vars.slow_log.ctx

#ifdef __cplusplus
extern "C" {
#endif

    extern AuthServerGlobalVars g_server_global_vars;

#ifdef __cplusplus
}
#endif

#endif
