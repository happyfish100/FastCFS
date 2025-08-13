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

#define AUTH_BASE_PATH_STR    "/home"
#define AUTH_BASE_PATH_LEN    (sizeof(AUTH_BASE_PATH_STR) - 1)

typedef struct auth_full_path {
    char buff[PATH_MAX];
    FDIRDEntryFullName fullname;
} AuthFullPath;

#define AUTH_SET_GRANTED_POOL_PATH(fp, username, granted_id)  \
    do {  \
        char *p;  \
        FC_SET_STRING_EX(fp.fullname.ns, AUTH_NAMESPACE_STR, \
                AUTH_NAMESPACE_LEN); \
        fp.fullname.path.str = fp.buff;  \
        p = fp.buff; \
        memcpy(p, AUTH_BASE_PATH_STR, AUTH_BASE_PATH_LEN);  \
        p += AUTH_BASE_PATH_LEN;  \
        *p++ = '/';  \
        memcpy(p, (username)->str, (username)->len);  \
        p += (username)->len;  \
        *p++ = '/';  \
        memcpy(p, AUTH_DIR_NAME_GRANTED_STR, AUTH_DIR_NAME_GRANTED_LEN);  \
        p += AUTH_DIR_NAME_GRANTED_LEN;  \
        *p++ = '/';  \
        p += fc_itoa(granted_id, p); \
        *p = '\0';  \
        fp.fullname.path.len = p - fp.buff;  \
    } while (0)

#define AUTH_SET_USER_PATH2(fp, username, subdir1_str, subdir1_len, subdir2)  \
    do {  \
        char *p;  \
        FC_SET_STRING_EX(fp.fullname.ns, AUTH_NAMESPACE_STR, \
                AUTH_NAMESPACE_LEN); \
        fp.fullname.path.str = fp.buff;  \
        p = fp.buff; \
        memcpy(p, AUTH_BASE_PATH_STR, AUTH_BASE_PATH_LEN);  \
        p += AUTH_BASE_PATH_LEN;  \
        *p++ = '/';  \
        memcpy(p, (username)->str, (username)->len);  \
        p += (username)->len;  \
        *p++ = '/';  \
        memcpy(p, subdir1_str, subdir1_len);  \
        p += subdir1_len;  \
        *p++ = '/';  \
        memcpy(p, (subdir2).str, (subdir2).len);  \
        p += (subdir2).len;  \
        *p = '\0';  \
        fp.fullname.path.len = p - fp.buff;  \
    } while (0)

#define AUTH_SET_USER_PATH1(fp, username, subdir1_str, subdir1_len)  \
    do {  \
        char *p;  \
        FC_SET_STRING_EX(fp.fullname.ns, AUTH_NAMESPACE_STR,  \
                AUTH_NAMESPACE_LEN); \
        fp.fullname.path.str = fp.buff;  \
        p = fp.buff; \
        memcpy(p, AUTH_BASE_PATH_STR, AUTH_BASE_PATH_LEN);  \
        p += AUTH_BASE_PATH_LEN;  \
        *p++ = '/';  \
        memcpy(p, (username)->str, (username)->len);  \
        p += (username)->len;  \
        *p++ = '/';  \
        memcpy(p, subdir1_str, subdir1_len);  \
        p += subdir1_len;  \
        *p = '\0';  \
        fp.fullname.path.len = p - fp.buff;  \
    } while (0)

#define AUTH_SET_USER_HOME(fp, username)  \
    do {  \
        char *p;  \
        FC_SET_STRING_EX(fp.fullname.ns, AUTH_NAMESPACE_STR, \
                AUTH_NAMESPACE_LEN); \
        fp.fullname.path.str = fp.buff;  \
        p = fp.buff; \
        memcpy(p, AUTH_BASE_PATH_STR, AUTH_BASE_PATH_LEN);  \
        p += AUTH_BASE_PATH_LEN;  \
        *p++ = '/';  \
        memcpy(p, (username)->str, (username)->len);  \
        p += (username)->len;  \
        *p = '\0';  \
        fp.fullname.path.len = p - fp.buff;  \
    } while (0)


#define AUTH_SET_BASE_PATH(fp)  \
    do { \
        FC_SET_STRING_EX(fp.fullname.ns, AUTH_NAMESPACE_STR, \
                AUTH_NAMESPACE_LEN); \
        FC_SET_STRING_EX(fp.fullname.path, AUTH_BASE_PATH_STR, \
                AUTH_BASE_PATH_LEN); \
    } while (0)

#define AUTH_SET_PATH_OPER_FNAME(path, fp) \
    FDIR_SET_OPERATOR(path.oper, 0, 0, 0, NULL); \
    path.fullname = fp.fullname

#define AUTH_SET_OPER_INODE_PAIR(oino, _inode) \
    FDIR_SET_OPERATOR(oino.oper, 0, 0, 0, NULL); \
    oino.inode = _inode

#endif
