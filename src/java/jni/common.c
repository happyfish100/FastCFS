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
#include "common.h"

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
