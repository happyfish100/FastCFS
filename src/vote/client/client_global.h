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


#ifndef _FCFS_VOTE_CLIENT_GLOBAL_H
#define _FCFS_VOTE_CLIENT_GLOBAL_H

#include "vote_global.h"
#include "client_types.h"

typedef struct fcfs_vote_client_global_vars {
    FCFSVoteClientContext client_ctx;
} FCFSVoteClientGlobalVars;

#define fcfs_vote_client_init_full_ctx_ex(vote, client_ctx) \
    (vote)->ctx = client_ctx

#define fcfs_vote_client_init_full_ctx(vote) \
    fcfs_vote_client_init_full_ctx_ex(vote, &g_fcfs_vote_client_vars.client_ctx)

#ifdef __cplusplus
extern "C" {
#endif

    extern FCFSVoteClientGlobalVars g_fcfs_vote_client_vars;

#ifdef __cplusplus
}
#endif

#endif
