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


#ifndef _FCFS_AUTH_SERVER_FUNC_H
#define _FCFS_AUTH_SERVER_FUNC_H

#include "server_types.h"

#ifdef __cplusplus
extern "C" {
#endif

int server_load_config(const char *filename);

#define server_expect_body_length(task, expect_body_len) \
    sf_server_expect_body_length(&RESPONSE, REQUEST.header.body_len, \
            expect_body_len)

#define server_check_min_body_length(task, min_body_length) \
    sf_server_check_min_body_length(&RESPONSE, REQUEST.header.body_len, \
            min_body_length)

#define server_check_max_body_length(task, max_body_length) \
    sf_server_check_max_body_length(&RESPONSE, REQUEST.header.body_len, \
            max_body_length)

#define server_check_body_length(task, min_body_length, max_body_length) \
    sf_server_check_body_length(&RESPONSE, REQUEST.header.body_len, \
            min_body_length, max_body_length)

#ifdef __cplusplus
}
#endif

#endif
