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

#include "sf/idempotency/client/rpc_wrapper.h"
#include "../common/auth_func.h"
#include "fcfs_auth_for_server.h"

int fcfs_auth_for_server_check_priv(FCFSAuthClientContext *client_ctx,
        SFRequestInfo *request, SFResponseInfo *response,
        const FCFSAuthValidatePriviledgeType priv_type,
        const int64_t the_priv)
{
    int result;
    bool validate;
    ServerSessionIdInfo session;
    string_t session_id;

    if ((result=sf_server_check_min_body_length(response, request->
                    header.body_len, FCFS_AUTH_SESSION_ID_LEN)) != 0)
    {
        return result;
    }
    session.id = buff2long(request->body);

    if (session.fields.publish) {
        switch (priv_type) {
            case fcfs_auth_validate_priv_type_user:
                result = server_session_user_priv_granted(
                        session.id, the_priv);
                break;
            case fcfs_auth_validate_priv_type_pool_fdir:
                result = server_session_fdir_priv_granted(
                        session.id, the_priv);
                break;
            default:
                result = server_session_fstore_priv_granted(
                        session.id, the_priv);
                break;
        }

        if (result == SF_SESSION_ERROR_NOT_EXIST) {
            validate = (g_current_time - session.fields.ts <=
                    g_server_session_cfg.validate_within_fresh_seconds);
        } else {
            validate = false;
        }
    } else {
        validate = true;
    }

    if (validate) {
        const int64_t pool_id = 0;
        FC_SET_STRING_EX(session_id, request->body, FCFS_AUTH_SESSION_ID_LEN);
        result = fcfs_auth_client_session_validate(client_ctx,
                &session_id, &g_server_session_cfg.validate_key,
                priv_type, pool_id, the_priv);
    }

    /*
    logInfo("check priv cmd: %d, session: %"PRId64", validate: %d, "
            "error no: %d", request->header.cmd, session.id, validate, result);
            */
    if (result != 0) {
        return result;
    }

    request->body += FCFS_AUTH_SESSION_ID_LEN;
    request->header.body_len -= FCFS_AUTH_SESSION_ID_LEN;
    return 0;
}
