#include "fastcfs/api/std/posix_api.h"
#include "global.h"
#include "common.h"
#include "com_fastken_fcfs_FCFSDirectory.h"

jobject JNICALL Java_com_fastken_fcfs_FCFSDirectory_next
  (JNIEnv *env, jobject obj)
{
    long handler;
    DIR *dir;
    struct dirent *dirent;
    jstring name;

    handler = (*env)->CallLongMethod(env, obj,
            g_fcfs_jni_global_vars.dir.getHandler);
    if (handler == 0) {
        fcfs_jni_throw_null_pointer_exception(env);
        return NULL;
    }

    dir = (DIR *)handler;
    if ((dirent=fcfs_readdir(dir)) == NULL) {
        return NULL;
    }

    name = (*env)->NewStringUTF(env, dirent->d_name);
    return (*env)->NewObject(env, g_fcfs_jni_global_vars.dirent.clazz,
            g_fcfs_jni_global_vars.dirent.constructor2, dirent->d_ino, name);
}

void JNICALL Java_com_fastken_fcfs_FCFSDirectory_seek
  (JNIEnv *env, jobject obj, jlong loc)
{
    long handler;
    DIR *dir;

    handler = (*env)->CallLongMethod(env, obj,
            g_fcfs_jni_global_vars.dir.getHandler);
    if (handler == 0) {
        fcfs_jni_throw_null_pointer_exception(env);
        return;
    }

    dir = (DIR *)handler;
    fcfs_seekdir(dir, loc);
}

jlong JNICALL Java_com_fastken_fcfs_FCFSDirectory_tell
  (JNIEnv *env, jobject obj)
{
    long handler;
    DIR *dir;

    handler = (*env)->CallLongMethod(env, obj,
            g_fcfs_jni_global_vars.dir.getHandler);
    if (handler == 0) {
        fcfs_jni_throw_null_pointer_exception(env);
        return -1;
    }

    dir = (DIR *)handler;
    return fcfs_telldir(dir);
}

void JNICALL Java_com_fastken_fcfs_FCFSDirectory_rewind
  (JNIEnv *env, jobject obj)
{
    long handler;
    DIR *dir;

    handler = (*env)->CallLongMethod(env, obj,
            g_fcfs_jni_global_vars.dir.getHandler);
    if (handler == 0) {
        fcfs_jni_throw_null_pointer_exception(env);
        return;
    }

    dir = (DIR *)handler;
    fcfs_rewinddir(dir);
}

void JNICALL Java_com_fastken_fcfs_FCFSDirectory_close
  (JNIEnv *env, jobject obj)
{
    long handler;
    DIR *dir;

    handler = (*env)->CallLongMethod(env, obj,
            g_fcfs_jni_global_vars.dir.getHandler);
    if (handler == 0) {
        fcfs_jni_throw_null_pointer_exception(env);
        return;
    }

    dir = (DIR *)handler;
    fcfs_closedir(dir);
    (*env)->CallVoidMethod(env, obj, g_fcfs_jni_global_vars.
            dir.setHandler, 0);
}
