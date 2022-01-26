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


#ifndef _FCFS_PAPI_DIR_H
#define _FCFS_PAPI_DIR_H

#include "api_types.h"

#ifdef __cplusplus
extern "C" {
#endif

    int fcfs_mkdir(const char *path, mode_t mode);

    int fcfs_mkdirat(int fd, const char *path, mode_t mode);

#ifdef __cplusplus
}
#endif

#endif
