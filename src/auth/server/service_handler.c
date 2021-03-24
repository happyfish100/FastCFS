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

//service_handler.c

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <limits.h>
#include <fcntl.h>
#include "fastcommon/logger.h"
#include "fastcommon/sockopt.h"
#include "fastcommon/shared_func.h"
#include "fastcommon/pthread_func.h"
#include "fastcommon/sched_thread.h"
#include "fastcommon/ioevent_loop.h"
#include "sf/sf_util.h"
#include "sf/sf_func.h"
#include "sf/sf_nio.h"
#include "sf/sf_service.h"
#include "sf/sf_global.h"
#include "common/auth_proto.h"
#include "common/auth_func.h"
#include "db/dao/dao.h"
#include "db/auth_db.h"
#include "server_global.h"
#include "server_func.h"
#include "common_handler.h"
#include "service_handler.h"

typedef int (*deal_task_func)(struct fast_task_info *task);

int service_handler_init()
{
    return auth_db_init();
}

int service_handler_destroy()
{
    return 0;
}

void service_task_finish_cleanup(struct fast_task_info *task)
{
    sf_task_finish_clean_up(task);
}

static int service_deal_user_login(struct fast_task_info *task)
{
    FCFSAuthProtoUserLoginReq *req;
    FCFSAuthProtoUserLoginResp *resp;
    const FCFSAuthUserInfo *user;
    string_t username;
    string_t passwd;
    int64_t session_id;
    int result;

    if ((result=server_check_min_body_length(sizeof(
                        FCFSAuthProtoUserLoginReq) + 1)) != 0)
    {
        return result;
    }

    req = (FCFSAuthProtoUserLoginReq *)REQUEST.body;
    FC_SET_STRING_EX(username, req->up_pair.username.str,
            req->up_pair.username.len);
    FC_SET_STRING_EX(passwd, req->up_pair.passwd,
            FCFS_AUTH_PASSWD_LEN);
    if ((result=server_expect_body_length(
                    sizeof(FCFSAuthProtoUserLoginReq)
                    + username.len)) != 0)
    {
        return result;
    }

    if (!((user=adb_user_get(SERVER_CTX, &username)) != NULL &&
            fc_string_equal(&user->passwd, &passwd)))
    {
        RESPONSE.error.length = sprintf(RESPONSE.error.message,
                "user login fail, username or password not correct");
        return EPERM;
    }

    //TODO
    session_id = 123456;

    resp = (FCFSAuthProtoUserLoginResp *)REQUEST.body;
    long2buff(session_id, resp->session_id);
    RESPONSE.header.body_len = sizeof(FCFSAuthProtoUserLoginResp);
    RESPONSE.header.cmd = FCFS_AUTH_SERVICE_PROTO_USER_LOGIN_RESP;
    TASK_ARG->context.common.response_done = true;
    return 0;
}

static int service_deal_user_create(struct fast_task_info *task)
{
    FCFSAuthProtoUserCreateReq *req;
    FCFSAuthUserInfo user;
    int result;

    if ((result=server_check_body_length(sizeof(FCFSAuthProtoUserCreateReq)
                    + 1, sizeof(FCFSAuthProtoUserCreateReq) + NAME_MAX)) != 0)
    {
        return result;
    }

    req = (FCFSAuthProtoUserCreateReq *)REQUEST.body;
    FC_SET_STRING_EX(user.name, req->up_pair.username.str,
            req->up_pair.username.len);
    FC_SET_STRING_EX(user.passwd, req->up_pair.passwd,
            FCFS_AUTH_PASSWD_LEN);
    user.priv = buff2long(req->priv);
    if ((result=server_expect_body_length(
                    sizeof(FCFSAuthProtoUserCreateReq)
                    + user.name.len)) != 0)
    {
        return result;
    }

    return adb_user_create(SERVER_CTX, &user);
}

static int service_deal_user_list(struct fast_task_info *task)
{
    FCFSAuthUserArray array;
    const FCFSAuthUserInfo *user;
    const FCFSAuthUserInfo *end;
    string_t username;
    FCFSAuthProtoUserListReq *req;
    FCFSAuthProtoListRespHeader *resp_header;
    FCFSAuthProtoUserListRespBodyPart *body_part;
    char *p;
    int result;

    if ((result=server_check_body_length(0, NAME_MAX)) != 0) {
        return result;
    }

    fcfs_auth_user_init_array(&array);
    if (REQUEST.header.body_len > 0) {
        req = (FCFSAuthProtoUserListReq *)REQUEST.body;
        username.len = REQUEST.header.body_len;
        username.str = req->username;
        if ((user=adb_user_get(SERVER_CTX, &username)) == NULL) {
            RESPONSE.error.length = sprintf(RESPONSE.error.message,
                    "username:%.*s not exist", username.len, username.str);
            fcfs_auth_user_free_array(&array);
            return ENOENT;
        }
        array.users[0] = *user;
        array.count = 1;
    } else if ((result=adb_user_list(SERVER_CTX, &array)) != 0) {
        fcfs_auth_user_free_array(&array);
        return result;
    }

    resp_header = (FCFSAuthProtoListRespHeader *)REQUEST.body;
    p = (char *)(resp_header + 1);
    end = array.users + array.count;
    for (user=array.users; user<end; user++) {
        body_part = (FCFSAuthProtoUserListRespBodyPart *)p;
        long2buff(user->priv, body_part->priv);
        body_part->username.len = user->name.len;
        memcpy(body_part->username.str, user->name.str, user->name.len);
        p += sizeof(FCFSAuthProtoUserListRespBodyPart) + user->name.len;
    }
    int2buff(array.count, resp_header->count);
    RESPONSE.header.body_len = p - REQUEST.body;
    RESPONSE.header.cmd = FCFS_AUTH_SERVICE_PROTO_USER_LIST_RESP;
    TASK_ARG->context.common.response_done = true;

    fcfs_auth_user_free_array(&array);
    return 0;
}

static int service_deal_user_grant(struct fast_task_info *task)
{
    FCFSAuthProtoUserGrantReq *req;
    string_t username;
    int64_t priv;
    int result;

    if ((result=server_check_body_length(sizeof(FCFSAuthProtoUserGrantReq)
                    + 1, sizeof(FCFSAuthProtoUserGrantReq) + NAME_MAX)) != 0)
    {
        return result;
    }

    req = (FCFSAuthProtoUserGrantReq *)REQUEST.body;
    FC_SET_STRING_EX(username, req->username.str, req->username.len);
    priv = buff2long(req->priv);
    if ((result=server_expect_body_length(
                    sizeof(FCFSAuthProtoUserGrantReq)
                    + username.len)) != 0)
    {
        return result;
    }

    return adb_user_update_priv(SERVER_CTX, &username, priv);
}

static int service_deal_user_remove(struct fast_task_info *task)
{
    FCFSAuthProtoUserRemoveReq *req;
    string_t username;
    int result;

    if ((result=server_check_body_length(sizeof(FCFSAuthProtoUserRemoveReq)
                    + 1, sizeof(FCFSAuthProtoUserRemoveReq) + NAME_MAX)) != 0)
    {
        return result;
    }

    req = (FCFSAuthProtoUserRemoveReq *)REQUEST.body;
    FC_SET_STRING_EX(username, req->username.str, req->username.len);
    if ((result=server_expect_body_length(
                    sizeof(FCFSAuthProtoUserRemoveReq)
                    + username.len)) != 0)
    {
        return result;
    }

    return adb_user_remove(SERVER_CTX, &username);
}

static int service_deal_spool_create(struct fast_task_info *task)
{
    FCFSAuthProtoSPoolCreateReq *req;
    string_t username;
    FCFSAuthStoragePoolInfo spool;
    int result;

    if ((result=server_check_body_length(sizeof(FCFSAuthProtoSPoolCreateReq)
                    + 1, sizeof(FCFSAuthProtoSPoolCreateReq) + NAME_MAX)) != 0)
    {
        return result;
    }

    req = (FCFSAuthProtoSPoolCreateReq *)REQUEST.body;
    FC_SET_STRING_EX(spool.name, req->poolname.str, req->poolname.len);
    spool.quota = buff2long(req->quota);
    if ((result=server_expect_body_length(
                    sizeof(FCFSAuthProtoSPoolCreateReq)
                    + spool.name.len)) != 0)
    {
        return result;
    }

    //TODO
    FC_SET_STRING_EX(username, "admin", sizeof("admin") - 1);
    return adb_spool_create(SERVER_CTX, &username, &spool);
}

static int service_deal_spool_list(struct fast_task_info *task)
{
    FCFSAuthStoragePoolArray array;
    const FCFSAuthStoragePoolInfo *spool;
    const FCFSAuthStoragePoolInfo *end;
    string_t username;
    string_t poolname;
    FCFSAuthProtoSPoolListReq *req;
    FCFSAuthProtoListRespHeader *resp_header;
    FCFSAuthProtoSPoolListRespBodyPart *body_part;
    char *p;
    int result;

    if ((result=server_check_body_length(sizeof(FCFSAuthProtoSPoolListReq),
                    sizeof(FCFSAuthProtoSPoolListReq) + NAME_MAX * 2)) != 0)
    {
        return result;
    }

    req = (FCFSAuthProtoSPoolListReq *)REQUEST.body;
    if (req->username.len == 0) {
        //TODO
        FC_SET_STRING_EX(username, "admin", sizeof("admin") - 1);
    } else {
        username.len = req->username.len;
        username.str = req->username.str;
    }

    if ((result=server_expect_body_length(sizeof(FCFSAuthProtoSPoolListReq)
                    + req->username.len + req->poolname.len)) != 0)
    {
        return result;
    }

    fcfs_auth_spool_init_array(&array);
    if (req->poolname.len > 0) {
        poolname.len = req->poolname.len;
        poolname.str = req->username.str + req->username.len +
            sizeof(FCFSAuthProtoNameInfo);
        if ((spool=adb_spool_get(SERVER_CTX, &username, &poolname)) == NULL) {
            RESPONSE.error.length = sprintf(RESPONSE.error.message,
                    "user: %.*s, pool: %.*s not exist", username.len,
                    username.str, poolname.len, poolname.str);
            fcfs_auth_spool_free_array(&array);
            return ENOENT;
        }

        array.spools[0] = *spool;
        array.count = 1;
    } else if ((result=adb_spool_list(SERVER_CTX, &username, &array)) != 0) {
        fcfs_auth_spool_free_array(&array);
        return result;
    }

    resp_header = (FCFSAuthProtoListRespHeader *)REQUEST.body;
    p = (char *)(resp_header + 1);
    end = array.spools + array.count;
    for (spool=array.spools; spool<end; spool++) {
        body_part = (FCFSAuthProtoSPoolListRespBodyPart *)p;
        long2buff(spool->quota, body_part->quota);
        body_part->poolname.len = spool->name.len;
        memcpy(body_part->poolname.str, spool->name.str, spool->name.len);
        p += sizeof(FCFSAuthProtoSPoolListRespBodyPart) + spool->name.len;
    }
    int2buff(array.count, resp_header->count);
    RESPONSE.header.body_len = p - REQUEST.body;
    RESPONSE.header.cmd = FCFS_AUTH_SERVICE_PROTO_SPOOL_LIST_RESP;
    TASK_ARG->context.common.response_done = true;

    fcfs_auth_spool_free_array(&array);
    return 0;
}

static int service_deal_spool_remove(struct fast_task_info *task)
{
    FCFSAuthProtoSPoolRemoveReq *req;
    string_t username;
    string_t poolname;
    int result;

    if ((result=server_check_body_length(sizeof(FCFSAuthProtoSPoolRemoveReq)
                    + 1, sizeof(FCFSAuthProtoSPoolRemoveReq) + NAME_MAX)) != 0)
    {
        return result;
    }

    req = (FCFSAuthProtoSPoolRemoveReq *)REQUEST.body;
    FC_SET_STRING_EX(poolname, req->poolname.str, req->poolname.len);
    if ((result=server_expect_body_length(
                    sizeof(FCFSAuthProtoSPoolRemoveReq)
                    + poolname.len)) != 0)
    {
        return result;
    }

    //TODO
    FC_SET_STRING_EX(username, "admin", sizeof("admin") - 1);
    return adb_spool_remove(SERVER_CTX, &username, &poolname);
}

static int service_deal_spool_set_quota(struct fast_task_info *task)
{
    FCFSAuthProtoSPoolSetQuotaReq *req;
    string_t username;
    string_t poolname;
    int64_t quota;
    int result;

    if ((result=server_check_body_length(sizeof(FCFSAuthProtoSPoolSetQuotaReq)
                    + 1, sizeof(FCFSAuthProtoSPoolSetQuotaReq) + NAME_MAX)) != 0)
    {
        return result;
    }

    req = (FCFSAuthProtoSPoolSetQuotaReq *)REQUEST.body;
    FC_SET_STRING_EX(poolname, req->poolname.str, req->poolname.len);
    if ((result=server_expect_body_length(
                    sizeof(FCFSAuthProtoSPoolSetQuotaReq)
                    + poolname.len)) != 0)
    {
        return result;
    }

    quota = buff2long(req->quota);
    //TODO
    FC_SET_STRING_EX(username, "admin", sizeof("admin") - 1);
    return adb_spool_set_quota(SERVER_CTX, &username, &poolname, quota);
}

int service_deal_task(struct fast_task_info *task, const int stage)
{
    int result;

    /*
    logInfo("file: "__FILE__", line: %d, "
            "task: %p, sock: %d, nio stage: %d, continue: %d, "
            "cmd: %d (%s)", __LINE__, task, task->event.fd, stage,
            stage == SF_NIO_STAGE_CONTINUE,
            ((FDIRProtoHeader *)task->data)->cmd,
            fdir_get_cmd_caption(((FDIRProtoHeader *)task->data)->cmd));
            */

    if (stage == SF_NIO_STAGE_CONTINUE) {
        if (task->continue_callback != NULL) {
            result = task->continue_callback(task);
        } else {
            result = RESPONSE_STATUS;
            if (result == TASK_STATUS_CONTINUE) {
                logError("file: "__FILE__", line: %d, "
                        "unexpect status: %d", __LINE__, result);
                result = EBUSY;
            }
        }
    } else {
        sf_proto_init_task_context(task, &TASK_CTX.common);
        switch (REQUEST.header.cmd) {
            case SF_PROTO_ACTIVE_TEST_REQ:
                RESPONSE.header.cmd = SF_PROTO_ACTIVE_TEST_RESP;
                result = sf_proto_deal_active_test(task, &REQUEST, &RESPONSE);
                break;
            case FCFS_AUTH_SERVICE_PROTO_USER_LOGIN_REQ:
                result = service_deal_user_login(task);
                break;
            case FCFS_AUTH_SERVICE_PROTO_USER_CREATE_REQ:
                RESPONSE.header.cmd = FCFS_AUTH_SERVICE_PROTO_USER_CREATE_RESP;
                result = service_deal_user_create(task);
                break;
            case FCFS_AUTH_SERVICE_PROTO_USER_LIST_REQ:
                result = service_deal_user_list(task);
                break;
            case FCFS_AUTH_SERVICE_PROTO_USER_GRANT_REQ:
                RESPONSE.header.cmd = FCFS_AUTH_SERVICE_PROTO_USER_GRANT_RESP;
                result = service_deal_user_grant(task);
                break;
            case FCFS_AUTH_SERVICE_PROTO_USER_REMOVE_REQ:
                RESPONSE.header.cmd = FCFS_AUTH_SERVICE_PROTO_USER_REMOVE_RESP;
                result = service_deal_user_remove(task);
                break;
            case FCFS_AUTH_SERVICE_PROTO_SPOOL_CREATE_REQ:
                RESPONSE.header.cmd = FCFS_AUTH_SERVICE_PROTO_SPOOL_CREATE_RESP;
                result = service_deal_spool_create(task);
                break;
            case FCFS_AUTH_SERVICE_PROTO_SPOOL_LIST_REQ:
                result = service_deal_spool_list(task);
                break;
            case FCFS_AUTH_SERVICE_PROTO_SPOOL_REMOVE_REQ:
                RESPONSE.header.cmd = FCFS_AUTH_SERVICE_PROTO_SPOOL_REMOVE_RESP;
                result = service_deal_spool_remove(task);
                break;
            case FCFS_AUTH_SERVICE_PROTO_SPOOL_SET_QUOTA_REQ:
                RESPONSE.header.cmd = FCFS_AUTH_SERVICE_PROTO_SPOOL_SET_QUOTA_RESP;
                result = service_deal_spool_set_quota(task);
                break;
            default:
                RESPONSE.error.length = sprintf(
                        RESPONSE.error.message,
                        "unkown cmd: %d", REQUEST.header.cmd);
                result = -EINVAL;
                break;
        }
    }

    if (result == TASK_STATUS_CONTINUE) {
        return 0;
    } else {
        RESPONSE_STATUS = result;
        return sf_proto_deal_task_done(task, &TASK_CTX.common);
    }
}

void *service_alloc_thread_extra_data(const int thread_index)
{
    int alloc_size;
    AuthServerContext *server_context;

    alloc_size = sizeof(AuthServerContext) + dao_get_context_size();
    server_context = (AuthServerContext *)fc_malloc(alloc_size);
    if (server_context == NULL) {
        return NULL;
    }

    memset(server_context, 0, alloc_size);
    server_context->dao_ctx = (void *)(server_context + 1);
    if (dao_init_context(server_context->dao_ctx) != 0) {
        sf_terminate_myself();
        return NULL;
    }

    return server_context;
}
