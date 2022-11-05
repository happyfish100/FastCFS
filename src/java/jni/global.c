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
#include "fastcommon/common_define.h"
#include "global.h"

FCFSJNIGlobalVars g_fcfs_jni_global_vars = {{NULL, NULL}};

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

void JNICALL Java_com_fastken_fcfs_FCFSPosixAPI_doInit
  (JNIEnv *env, jclass clazz)
{
    g_fcfs_jni_global_vars.papi.setHandler = (*env)->GetMethodID(
            env, clazz, "setHandler", "(J)V");
    g_fcfs_jni_global_vars.papi.getHandler = (*env)->GetMethodID(
            env, clazz, "getHandler", "()J");

    g_fcfs_jni_global_vars.dir.clazz = (*env)->FindClass(env,
            "com/fastken/fcfs/FCFSDirectory");
    g_fcfs_jni_global_vars.dir.clazz = (*env)->NewGlobalRef(env,
            g_fcfs_jni_global_vars.dir.clazz);

    g_fcfs_jni_global_vars.file.clazz = (*env)->FindClass(env,
            "com/fastken/fcfs/FCFSFile");
    g_fcfs_jni_global_vars.file.clazz = (*env)->NewGlobalRef(env,
            g_fcfs_jni_global_vars.file.clazz);

    g_fcfs_jni_global_vars.statvfs.clazz = (*env)->FindClass(env,
            "com/fastken/fcfs/FCFSVFSStat");
    g_fcfs_jni_global_vars.statvfs.clazz = (*env)->NewGlobalRef(env,
            g_fcfs_jni_global_vars.statvfs.clazz);
}

void JNICALL Java_com_fastken_fcfs_FCFSDirectory_doInit
  (JNIEnv *env, jclass clazz)
{
    g_fcfs_jni_global_vars.dir.constructor1 = (*env)->GetMethodID(
            env, clazz, "<init>", "(J)V");
    g_fcfs_jni_global_vars.dir.setHandler = (*env)->GetMethodID(
            env, clazz, "setHandler", "(J)V");
    g_fcfs_jni_global_vars.dir.getHandler = (*env)->GetMethodID(
            env, clazz, "getHandler", "()J");

    g_fcfs_jni_global_vars.dirent.clazz = (*env)->FindClass(env,
            "com/fastken/fcfs/FCFSDirectory$Entry");
    g_fcfs_jni_global_vars.dirent.clazz = (*env)->NewGlobalRef(env,
            g_fcfs_jni_global_vars.dirent.clazz);
}

void JNICALL Java_com_fastken_fcfs_FCFSDirectory_00024Entry_doInit
  (JNIEnv *env, jclass clazz)
{
    g_fcfs_jni_global_vars.dirent.constructor2 = (*env)->GetMethodID(
            env, clazz, "<init>", "(JLjava/lang/String;)V");
}

void JNICALL Java_com_fastken_fcfs_FCFSFile_doInit
  (JNIEnv *env, jclass clazz)
{
    g_fcfs_jni_global_vars.file.constructor1 = (*env)->GetMethodID(
            env, clazz, "<init>", "(I)V");
    g_fcfs_jni_global_vars.file.setFD = (*env)->GetMethodID(
            env, clazz, "setFD", "(I)V");
    g_fcfs_jni_global_vars.file.getFD = (*env)->GetMethodID(
            env, clazz, "getFD", "()I");

    g_fcfs_jni_global_vars.fstat.clazz = (*env)->FindClass(env,
            "com/fastken/fcfs/FCFSFileStat");
    g_fcfs_jni_global_vars.fstat.clazz = (*env)->NewGlobalRef(env,
            g_fcfs_jni_global_vars.fstat.clazz);
}

void JNICALL Java_com_fastken_fcfs_FCFSFileStat_doInit
  (JNIEnv *env, jclass clazz)
{
    g_fcfs_jni_global_vars.fstat.constructor10 = (*env)->GetMethodID(
            env, clazz, "<init>", "(JIIIIIJJJJ)V");
}

void JNICALL Java_com_fastken_fcfs_FCFSVFSStat_doInit
  (JNIEnv *env, jclass clazz)
{
    g_fcfs_jni_global_vars.statvfs.constructor6 = (*env)->GetMethodID(
            env, clazz, "<init>", "(JJJJJJ)V");
}
