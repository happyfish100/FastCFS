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


#ifndef _FCFS_JNI_GLOBAL_H
#define _FCFS_JNI_GLOBAL_H

#include <jni.h>

typedef struct {
    jclass clazz;
    jmethodID constructor2;
    jmethodID getValue;
    jmethodID setValue;
    jmethodID getErrno;
    jmethodID setErrno;
} FCFSJNIResultMethods;

typedef struct {
    struct {
        jmethodID setHandler;
        jmethodID getHandler;
    } papi;

    struct {
        jmethodID setHandler;
        jmethodID getHandler;
    } dir;

    struct {
        jclass clazz;
        jmethodID constructor1;
        jmethodID constructor3;
        jmethodID setLength;
        jmethodID getBuff;
        jmethodID getOffset;
        jmethodID getLength;
    } buffer;

    FCFSJNIResultMethods sresult;
    FCFSJNIResultMethods iresult;
    FCFSJNIResultMethods lresult;

    struct {
        jclass clazz;
        jmethodID constructor2;
    } dirent;

    struct {
        jclass clazz;
        jmethodID constructor10;
    } fstat;

    struct {
        jclass clazz;
        jmethodID constructor6;
    } vfsstat;

} FCFSJNIGlobalVars;

#ifdef __cplusplus
extern "C" {
#endif

extern FCFSJNIGlobalVars g_fcfs_jni_global_vars;

/*
 * Class:     com_fastken_fcfs_FCFSPosixAPI
 * Method:    doInit
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_com_fastken_fcfs_FCFSPosixAPI_doInit
  (JNIEnv *, jclass);

/*
 * Class:     com_fastken_fcfs_FCFSPosixAPI_DIR
 * Method:    doInit
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_com_fastken_fcfs_FCFSPosixAPI_00024DIR_doInit
  (JNIEnv *, jclass);

/*
 * Class:     com_fastken_fcfs_FCFSPosixAPI_DIREntry
 * Method:    doInit
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_com_fastken_fcfs_FCFSPosixAPI_00024DIREntry_doInit
  (JNIEnv *, jclass);

/*
 * Class:     com_fastken_fcfs_FCFSPosixAPI_Buffer
 * Method:    doInit
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_com_fastken_fcfs_FCFSPosixAPI_00024Buffer_doInit
  (JNIEnv *, jclass);

/*
 * Class:     com_fastken_fcfs_FCFSPosixAPI_IntResult
 * Method:    doInit
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_com_fastken_fcfs_FCFSPosixAPI_00024IntResult_doInit
  (JNIEnv *, jclass);

/*
 * Class:     com_fastken_fcfs_FCFSPosixAPI_LongResult
 * Method:    doInit
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_com_fastken_fcfs_FCFSPosixAPI_00024LongResult_doInit
  (JNIEnv *, jclass);

/*
 * Class:     com_fastken_fcfs_FCFSPosixAPI_StringResult
 * Method:    doInit
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_com_fastken_fcfs_FCFSPosixAPI_00024StringResult_doInit
  (JNIEnv *, jclass);

/*
 * Class:     com_fastken_fcfs_FCFSPosixAPI_FileStat
 * Method:    doInit
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_com_fastken_fcfs_FCFSPosixAPI_00024FileStat_doInit
  (JNIEnv *, jclass);

/*
 * Class:     com_fastken_fcfs_FCFSPosixAPI_VFSStat
 * Method:    doInit
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_com_fastken_fcfs_FCFSPosixAPI_00024VFSStat_doInit
  (JNIEnv *, jclass);

#ifdef __cplusplus
}
#endif

#endif
