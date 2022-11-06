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
    struct {
        jmethodID setHandler;
        jmethodID getHandler;
    } papi;

    struct {
        jclass clazz;
        jmethodID constructor1;
        jmethodID setHandler;
        jmethodID getHandler;
    } dir;

    struct {
        jclass clazz;
        jmethodID constructor1;
        jmethodID setFD;
        jmethodID getFD;
    } file;

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
    } statvfs;
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
 * Class:     com_fastken_fcfs_FCFSDirectory
 * Method:    doInit
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_com_fastken_fcfs_FCFSDirectory_doInit
  (JNIEnv *, jclass);

/*
 * Class:     com_fastken_fcfs_FCFSDirectory_Entry
 * Method:    doInit
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_com_fastken_fcfs_FCFSDirectory_00024Entry_doInit
  (JNIEnv *, jclass);

/*
 * Class:     com_fastken_fcfs_FCFSFile
 * Method:    doInit
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_com_fastken_fcfs_FCFSFile_doInit
  (JNIEnv *, jclass);

/*
 * Class:     com_fastken_fcfs_FCFSFileStat
 * Method:    doInit
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_com_fastken_fcfs_FCFSFileStat_doInit
  (JNIEnv *, jclass);

/*
 * Class:     com_fastken_fcfs_FCFSVFSStat
 * Method:    doInit
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_com_fastken_fcfs_FCFSVFSStat_doInit
  (JNIEnv *, jclass);

#ifdef __cplusplus
}
#endif

#endif
