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


#ifndef _FCFS_JNI_COMMON_H
#define _FCFS_JNI_COMMON_H

#include <jni.h>

#ifdef __cplusplus
extern "C" {
#endif
    void fcfs_jni_throw_exception(JNIEnv *env, const char *message);

    void fcfs_jni_throw_null_pointer_exception(JNIEnv *env);

    void fcfs_jni_throw_out_of_bounds_exception(JNIEnv *env, const int index);

    void fcfs_jni_throw_filesystem_exception(JNIEnv *env,
            const char *file, const int err_no);

    jobject fcfs_jni_convert_to_list(JNIEnv *env,
            const char *buff, const int length);

    int fcfs_jni_convert_open_flags(const int flags);

    int fcfs_jni_convert_setxattr_flags(const int flags);

#ifdef __cplusplus
}
#endif

#endif
