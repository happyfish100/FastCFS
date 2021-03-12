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

#ifndef _AUTH_DAO_TYPES_H
#define _AUTH_DAO_TYPES_H

#include <limits.h>
#include "fastdir/client/fdir_client.h"
#include "common/auth_types.h"

#define AUTH_NAMESPACE_STR    "sys-auth"
#define AUTH_NAMESPACE_LEN    (sizeof(AUTH_NAMESPACE_STR) - 1)

#define AUTH_BASE_PATH        "/home"

typedef struct auth_full_path {
    char buff[PATH_MAX];
    FDIRDEntryFullName fullname;
} AuthFullPath;

#define AUTH_SET_USER_PATH(fp, username, subdir)  \
    do {  \
        FC_SET_STRING_EX(fp.fullname.ns, AUTH_NAMESPACE_STR, \
                AUTH_NAMESPACE_LEN); \
        fp.fullname.path.str = fp.buff;  \
        if (subdir != NULL) { \
            fp.fullname.path.len = sprintf(fp.fullname.path.str, \
                    AUTH_BASE_PATH"/%s/%s", (username)->str, subdir); \
        } else { \
            fp.fullname.path.len = sprintf(fp.fullname.path.str, \
                    AUTH_BASE_PATH"/%s", (username)->str);   \
        } \
    } while (0)

#define AUTH_SET_USER_HOME(fp, username)  \
    AUTH_SET_USER_PATH(fp, username, NULL)

#endif
