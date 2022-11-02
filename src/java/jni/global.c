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

void JNICALL Java_com_fastken_fcfs_FCFSPosixAPI_doInit
  (JNIEnv *env, jclass clazz)
{
    g_fcfs_jni_global_vars.papi.setHandler = (*env)->GetMethodID(
            env, clazz, "setHandler", "(J)V");
    g_fcfs_jni_global_vars.papi.getHandler = (*env)->GetMethodID(
            env, clazz, "getHandler", "()J");

    g_fcfs_jni_global_vars.dir.clazz = (*env)->FindClass(env,
            "com/fastken/fcfs/FCFSDirectory");
    fprintf(stderr, "line: %d, Directory clazz: %p\n", __LINE__,
            g_fcfs_jni_global_vars.dir.clazz);

    g_fcfs_jni_global_vars.dir.clazz = (*env)->NewGlobalRef(env,
            g_fcfs_jni_global_vars.dir.clazz);

    fprintf(stderr, "line: %d, Directory clazz: %p\n", __LINE__,
            g_fcfs_jni_global_vars.dir.clazz);
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

    fprintf(stderr, "line: %d, constructor1: %p\n", __LINE__,
            g_fcfs_jni_global_vars.dir.constructor1);
    fprintf(stderr, "setHandler: %p, getHandler: %p\n",
            g_fcfs_jni_global_vars.dir.setHandler,
            g_fcfs_jni_global_vars.dir.getHandler);

    g_fcfs_jni_global_vars.dirent.clazz = (*env)->FindClass(env,
            "com/fastken/fcfs/FCFSDirectory$Entry");
    g_fcfs_jni_global_vars.dirent.clazz = (*env)->NewGlobalRef(env,
            g_fcfs_jni_global_vars.dirent.clazz);

    fprintf(stderr, "line: %d, Directory.Entry clazz: %p\n", __LINE__, g_fcfs_jni_global_vars.dirent.clazz);
}

void JNICALL Java_com_fastken_fcfs_FCFSDirectory_00024Entry_doInit
  (JNIEnv *env, jclass clazz)
{
    g_fcfs_jni_global_vars.dirent.constructor2 = (*env)->GetMethodID(
            env, clazz, "<init>", "(JLjava/lang/String;)V");

    fprintf(stderr, "Directory.Entry clazz: %p, constructor2: %p\n",
            clazz, g_fcfs_jni_global_vars.dirent.constructor2);
}

void JNICALL Java_com_fastken_fcfs_FCFSFile_doInit
  (JNIEnv *env, jclass clazz)
{
    g_fcfs_jni_global_vars.file.clazz = clazz;
    g_fcfs_jni_global_vars.file.constructor1 = (*env)->GetMethodID(
            env, clazz, "<init>", "(I)V");
    g_fcfs_jni_global_vars.file.setFD = (*env)->GetMethodID(
            env, clazz, "setFD", "(I)V");
    g_fcfs_jni_global_vars.file.getFD = (*env)->GetMethodID(
            env, clazz, "getFD", "()I");

    fprintf(stderr, "constructor1: %p\n", g_fcfs_jni_global_vars.file.constructor1);
    fprintf(stderr, "setFD: %p, getFD: %p\n",
            g_fcfs_jni_global_vars.file.setFD,
            g_fcfs_jni_global_vars.file.getFD);
}

void JNICALL Java_com_fastken_fcfs_FCFSFileStat_doInit
  (JNIEnv *env, jclass clazz)
{
    g_fcfs_jni_global_vars.fstat.clazz = clazz;
    g_fcfs_jni_global_vars.fstat.constructor10 = (*env)->GetMethodID(
            env, clazz, "<init>", "(JIIIIIJJJJ)V");

    fprintf(stderr, "line: %d, clazz: %p\n", __LINE__, g_fcfs_jni_global_vars.fstat.clazz);
    fprintf(stderr, "constructor10: %p\n", g_fcfs_jni_global_vars.fstat.constructor10);
}

void JNICALL Java_com_fastken_fcfs_FCFSVFSStat_doInit
  (JNIEnv *env, jclass clazz)
{
    g_fcfs_jni_global_vars.vfsstat.clazz = clazz;
    g_fcfs_jni_global_vars.vfsstat.constructor6 = (*env)->GetMethodID(
            env, clazz, "<init>", "(JJJJJJ)V");

    fprintf(stderr, "line: %d, clazz: %p\n", __LINE__, g_fcfs_jni_global_vars.vfsstat.clazz);
    fprintf(stderr, "constructor6: %p\n", g_fcfs_jni_global_vars.vfsstat.constructor6);
}
