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
#include <limits.h>
#include "fastcommon/pthread_func.h"
#include "fastcommon/logger.h"
#include "sf/sf_proto.h"
#include "sf/sf_global.h"
#include "auth_func.h"
#include "client_global.h"
#include "fcfs_auth_client.h"
#include "session_regenerate.h"

typedef struct synced_session_array {
    bool inited;
    pthread_mutex_t lock;
} SessionRegenerateContext;

static SessionRegenerateContext session_ctx;

static int session_regenerate_do()
{
    char old_session_id[FCFS_AUTH_SESSION_ID_LEN];
    int result;

    memcpy(old_session_id, g_fcfs_auth_client_vars.client_ctx.
            session.id, FCFS_AUTH_SESSION_ID_LEN);

    PTHREAD_MUTEX_LOCK(&session_ctx.lock);
    if (memcmp(old_session_id, g_fcfs_auth_client_vars.client_ctx.
                session.id, FCFS_AUTH_SESSION_ID_LEN) == 0)
    {
        result = fcfs_auth_client_user_login_ex(&g_fcfs_auth_client_vars.
                client_ctx, &g_fcfs_auth_client_vars.client_ctx.auth_cfg.
                username, &g_fcfs_auth_client_vars.client_ctx.auth_cfg.
                passwd, g_fcfs_auth_client_vars.client_ctx.session.poolname,
                FCFS_AUTH_SESSION_FLAGS_PUBLISH);
    } else {
        result = 0;
    }
    PTHREAD_MUTEX_UNLOCK(&session_ctx.lock);

    return result;
}

static int fcfs_auth_error_handler(const int errnum)
{
    if (errnum == SF_SESSION_ERROR_NOT_EXIST) {
        if (session_regenerate_do() == EPERM) {
            return EPERM;
        } else {
            return SF_RETRIABLE_ERROR_NO_SERVER;
        }
    } else {
        return errnum;
    }
}

int session_regenerate_init()
{
    int result;

    if (session_ctx.inited) {
        return 0;
    }

    if ((result=init_pthread_lock(&session_ctx.lock)) != 0) {
        return result;
    }

    sf_set_error_handler(fcfs_auth_error_handler);
    session_ctx.inited = true;
    return 0;
}
