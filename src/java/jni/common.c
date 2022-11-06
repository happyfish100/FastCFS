/*
 * Copyright (c) 2022 YuQing <384681@qq.com>
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

#include <errno.h>
#include <sys/types.h>
#include <sys/xattr.h>
#include "fastcommon/common_define.h"
#include "com_fastken_fcfs_FCFSConstants.h"
#include "common.h"

void fcfs_jni_throw_exception(JNIEnv *env, const char *message)
{
    jclass clazz;
    clazz = (*env)->FindClass(env, "java/lang/Exception");
    if (clazz == NULL) {
        return;
    }

    (*env)->ThrowNew(env, clazz, message);
}

void fcfs_jni_throw_null_pointer_exception(JNIEnv *env)
{
    jclass clazz;
    clazz = (*env)->FindClass(env, "java/lang/NullPointerException");
    if (clazz == NULL) {
        return;
    }

    (*env)->ThrowNew(env, clazz, "null pointer");
}

void fcfs_jni_throw_out_of_bounds_exception(JNIEnv *env, const int index)
{
    jclass clazz;
    jmethodID constructor1;

    clazz = (*env)->FindClass(env, "java/lang/ArrayIndexOutOfBoundsException");
    if (clazz == NULL) {
        return;
    }

    constructor1 = (*env)->GetMethodID(env, clazz, "<init>", "(I)V");
    (*env)->Throw(env, (*env)->NewObject(env, clazz, constructor1, index));
}

void fcfs_jni_throw_filesystem_exception(JNIEnv *env,
        const char *file, const int err_no)
{
    const char *cname;
    bool need_error;
    jclass clazz;

    switch (err_no) {
        case ENOENT:
            cname = "java/nio/file/NoSuchFileException";
            need_error = false;
            break;
        case EEXIST:
            cname = "java/nio/file/FileAlreadyExistsException";
            need_error = false;
            break;
        case EPERM:
            cname = "java/nio/file/AccessDeniedException";
            need_error = false;
            break;
        case ENOTEMPTY:
            cname = "java/nio/file/DirectoryNotEmptyException";
            need_error = false;
            break;
        case ELOOP:
            cname = "java/nio/file/FileSystemLoopException";
            need_error = false;
            break;
        case ENOTDIR:
            cname = "java/nio/file/NotDirectoryException";
            need_error = false;
            break;
        case ENOLINK:
            cname = "java/nio/file/NotLinkException";
            need_error = false;
            break;
        default:
            cname = "java/nio/file/FileSystemException";
            need_error = true;
            break;
    }

    clazz = (*env)->FindClass(env, cname);
    if (clazz == NULL) {
        return;
    }

    if (need_error) {
        jmethodID constructor3;
        jobject obj;

        constructor3 = (*env)->GetMethodID(env, clazz, "<init>",
                "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;)V");
        obj = (*env)->NewObject(env, clazz, constructor3,
                (*env)->NewStringUTF(env, file), NULL,
                (*env)->NewStringUTF(env, strerror(err_no)));
        (*env)->Throw(env, obj);
    } else {
        (*env)->ThrowNew(env, clazz, file);
    }
}

jobject fcfs_jni_convert_to_list(JNIEnv *env,
        const char *buff, const int length)
{
    jclass lclazz;
    jobject lobj;
    jmethodID lconstructor;
    jmethodID ladd;
    const char *name;
    const char *end;

    lclazz = (*env)->FindClass(env, "java/util/ArrayList");
    lconstructor = (*env)->GetMethodID(env, lclazz, "<init>", "()V");
    lobj = (*env)->NewObject(env, lclazz, lconstructor);
    if (length == 0) {
        return lobj;
    }

    ladd = (*env)->GetMethodID(env, lclazz,
            "add", "(Ljava/lang/Object;)Z");
    name = buff;
    end = buff + length;
    do {
        (*env)->CallBooleanMethod(env, lobj, ladd,
                (*env)->NewStringUTF(env, name));
        name += strlen(name) + 1;
    } while (name < end);

    return lobj;
}

int fcfs_jni_convert_open_flags(const int flags)
{
    int new_flags;

    new_flags = 0;
    if ((flags & com_fastken_fcfs_FCFSConstants_O_WRONLY)) {
        new_flags |= O_WRONLY;
    }
    if ((flags & com_fastken_fcfs_FCFSConstants_O_RDWR)) {
        new_flags |= O_RDWR;
    }
    if ((flags & com_fastken_fcfs_FCFSConstants_O_CREAT)) {
        new_flags |= O_CREAT;
    }
    if ((flags & com_fastken_fcfs_FCFSConstants_O_EXCL)) {
        new_flags |= O_EXCL;
    }
    if ((flags & com_fastken_fcfs_FCFSConstants_O_TRUNC)) {
        new_flags |= O_TRUNC;
    }
    if ((flags & com_fastken_fcfs_FCFSConstants_O_APPEND)) {
        new_flags |= O_APPEND;
    }
    if ((flags & com_fastken_fcfs_FCFSConstants_O_NOFOLLOW)) {
        new_flags |= O_NOFOLLOW;
    }
    if ((flags & com_fastken_fcfs_FCFSConstants_O_CLOEXEC)) {
        new_flags |= O_CLOEXEC;
    }

    return new_flags;
}

int fcfs_jni_convert_setxattr_flags(const int flags)
{
    int new_flags;

    new_flags = 0;
    if ((flags & com_fastken_fcfs_FCFSConstants_XATTR_CREATE)) {
        new_flags |= XATTR_CREATE;
    }
    if ((flags & com_fastken_fcfs_FCFSConstants_XATTR_REPLACE)) {
        new_flags |= XATTR_REPLACE;
    }

    return new_flags;
}
