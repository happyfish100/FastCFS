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


#include <sys/stat.h>
#include "fastcommon/shared_func.h"
#include "fastcommon/logger.h"
#include "func.h"

FCFSAuthDAOVariables g_auth_dao_vars = {
    {AUTH_NAMESPACE_STR, AUTH_NAMESPACE_LEN},
    {{0, 0, 0700 | S_IFDIR}, {0, 0, 0700 | S_IFREG}},
    {
        {AUTH_XTTR_NAME_PASSWD_STR, AUTH_XTTR_NAME_PASSWD_LEN},
        {AUTH_XTTR_NAME_PRIV_STR, AUTH_XTTR_NAME_PRIV_LEN},
        {AUTH_XTTR_NAME_STATUS_STR, AUTH_XTTR_NAME_STATUS_LEN},
        {AUTH_XTTR_NAME_QUOTA_STR, AUTH_XTTR_NAME_QUOTA_LEN},
        {AUTH_XTTR_NAME_FDIR_STR, AUTH_XTTR_NAME_FDIR_LEN},
        {AUTH_XTTR_NAME_FSTORE_STR, AUTH_XTTR_NAME_FSTORE_LEN},
        {AUTH_XTTR_NAME_POOL_AUTO_ID_STR, AUTH_XTTR_NAME_POOL_AUTO_ID_LEN}
    }
};
