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

#ifndef _FCFS_VOTE_TYPES_H
#define _FCFS_VOTE_TYPES_H

#include "fastcommon/common_define.h"
#include "sf/sf_types.h"

#define FCFS_VOTE_DEFAULT_CLUSTER_PORT  41011
#define FCFS_VOTE_DEFAULT_SERVICE_PORT  41012

#define FCFS_VOTE_SERVICE_ID_FAUTH    'A'
#define FCFS_VOTE_SERVICE_ID_FDIR     'D'
#define FCFS_VOTE_SERVICE_ID_FSTORE   'S'

#ifdef __cplusplus
extern "C" {
#endif

    static inline const char *fcfs_vote_get_service_name(const int service_id)
    {
        switch (service_id) {
            case FCFS_VOTE_SERVICE_ID_FAUTH:
                return "fauth";
            case FCFS_VOTE_SERVICE_ID_FDIR:
                return "fdir";
            case FCFS_VOTE_SERVICE_ID_FSTORE:
                return "fstore";
            default:
                return "unkown";
        }
    }

#ifdef __cplusplus
}
#endif

#endif
