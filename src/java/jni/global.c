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

void JNICALL Java_com_fastken_fcfs_FCFSPosixAPI_doInit
  (JNIEnv *env, jclass clazz)
{
    g_fcfs_jni_global_vars.papi.setHandler = (*env)->GetMethodID(
            env, clazz, "setHandler", "(J)V");
    g_fcfs_jni_global_vars.papi.getHandler = (*env)->GetMethodID(
            env, clazz, "getHandler", "()J");
}

void JNICALL Java_com_fastken_fcfs_FCFSPosixAPI_00024DIR_doInit
  (JNIEnv *env, jclass clazz)
{
    g_fcfs_jni_global_vars.dir.setHandler = (*env)->GetMethodID(
            env, clazz, "setHandler", "(J)V");
    g_fcfs_jni_global_vars.dir.getHandler = (*env)->GetMethodID(
            env, clazz, "getHandler", "()J");

    fprintf(stderr, "setHandler: %p\n", g_fcfs_jni_global_vars.dir.setHandler);
    fprintf(stderr, "getHandler: %p\n", g_fcfs_jni_global_vars.dir.getHandler);
}

void JNICALL Java_com_fastken_fcfs_FCFSPosixAPI_00024DIREntry_doInit
  (JNIEnv *env, jclass clazz)
{
    g_fcfs_jni_global_vars.dirent.clazz = clazz;
    g_fcfs_jni_global_vars.dirent.constructor2 = (*env)->GetMethodID(
            env, clazz, "<init>", "(JLjava/lang/String;)V");

    fprintf(stderr, "line: %d, clazz: %p\n", __LINE__, g_fcfs_jni_global_vars.dirent.clazz);
    fprintf(stderr, "constructor2: %p\n", g_fcfs_jni_global_vars.dirent.constructor2);
}

void JNICALL Java_com_fastken_fcfs_FCFSPosixAPI_00024Buffer_doInit
  (JNIEnv *env, jclass clazz)
{
    g_fcfs_jni_global_vars.buffer.clazz = clazz;
    g_fcfs_jni_global_vars.buffer.constructor1 = (*env)->GetMethodID(
            env, clazz, "<init>", "([B)V");
    g_fcfs_jni_global_vars.buffer.constructor3 = (*env)->GetMethodID(
            env, clazz, "<init>", "([BII)V");
    g_fcfs_jni_global_vars.buffer.setLength = (*env)->GetMethodID(
            env, clazz, "setLength", "(I)V");
    g_fcfs_jni_global_vars.buffer.getBuff = (*env)->GetMethodID(
            env, clazz, "getBuff", "()[B");
    g_fcfs_jni_global_vars.buffer.getOffset = (*env)->GetMethodID(
            env, clazz, "getOffset", "()I");
    g_fcfs_jni_global_vars.buffer.getLength = (*env)->GetMethodID(
            env, clazz, "getLength", "()I");

    fprintf(stderr, "line: %d, clazz: %p\n", __LINE__, g_fcfs_jni_global_vars.buffer.clazz);
    fprintf(stderr, "constructor1: %p, constructor3: %p, "
            "setLength: %p, getBuff: %p, getOffset: %p, getLength: %p\n",
            g_fcfs_jni_global_vars.buffer.constructor1,
            g_fcfs_jni_global_vars.buffer.constructor3,
            g_fcfs_jni_global_vars.buffer.setLength,
            g_fcfs_jni_global_vars.buffer.getBuff,
            g_fcfs_jni_global_vars.buffer.getOffset,
            g_fcfs_jni_global_vars.buffer.getLength);
}

#define INIT_RESULT_METHODS(env, clazz, type, methods) \
    do { \
        methods.clazz = clazz;  \
        methods.constructor2 = (*env)->GetMethodID(  \
                env, clazz, "<init>", "("type"I)V"); \
        methods.getValue = (*env)->GetMethodID(      \
                env, clazz, "getValue", "()"type""); \
        methods.setValue = (*env)->GetMethodID(      \
                env, clazz, "setValue", "("type")V");\
        methods.getErrno = (*env)->GetMethodID(  \
                env, clazz, "getErrno", "()I");  \
        methods.setErrno = (*env)->GetMethodID(  \
                env, clazz, "setErrno", "(I)V"); \
    } while (0)


void JNICALL Java_com_fastken_fcfs_FCFSPosixAPI_00024StringResult_doInit
  (JNIEnv *env, jclass clazz)
{
    INIT_RESULT_METHODS(env, clazz, "Ljava/lang/String;",
            g_fcfs_jni_global_vars.sresult);
}

void JNICALL Java_com_fastken_fcfs_FCFSPosixAPI_00024IntResult_doInit
  (JNIEnv *env, jclass clazz)
{
    INIT_RESULT_METHODS(env, clazz, "I", g_fcfs_jni_global_vars.iresult);
}

void JNICALL Java_com_fastken_fcfs_FCFSPosixAPI_00024LongResult_doInit
  (JNIEnv *env, jclass clazz)
{
    INIT_RESULT_METHODS(env, clazz, "J", g_fcfs_jni_global_vars.lresult);
}

void JNICALL Java_com_fastken_fcfs_FCFSPosixAPI_00024FileStat_doInit
  (JNIEnv *env, jclass clazz)
{
    g_fcfs_jni_global_vars.fstat.clazz = clazz;
    g_fcfs_jni_global_vars.fstat.constructor10 = (*env)->GetMethodID(
            env, clazz, "<init>", "(JIIIIIJJJJ)V");

    fprintf(stderr, "line: %d, clazz: %p\n", __LINE__, g_fcfs_jni_global_vars.fstat.clazz);
    fprintf(stderr, "constructor10: %p\n", g_fcfs_jni_global_vars.fstat.constructor10);
}

void JNICALL Java_com_fastken_fcfs_FCFSPosixAPI_00024VFSStat_doInit
  (JNIEnv *env, jclass clazz)
{
    g_fcfs_jni_global_vars.vfsstat.clazz = clazz;
    g_fcfs_jni_global_vars.vfsstat.constructor6 = (*env)->GetMethodID(
            env, clazz, "<init>", "(JJJJJJ)V");

    fprintf(stderr, "line: %d, clazz: %p\n", __LINE__, g_fcfs_jni_global_vars.vfsstat.clazz);
    fprintf(stderr, "constructor6: %p\n", g_fcfs_jni_global_vars.vfsstat.constructor6);
}
