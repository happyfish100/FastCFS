
#include "fastcfs/api/std/posix_api.h"
#include "global.h"
#include "com_fastken_fcfs_FCFSPosixAPI.h"

void JNICALL Java_com_fastken_fcfs_FCFSPosixAPI_init
  (JNIEnv *env, jobject obj, jstring poolname, jstring filename)
{
    const char *log_prefix_name = "papi";
    jboolean *isCopy = NULL;
    char *ns;
    char *config_filename;
    FCFSPosixAPIContext *ctx;
    int result;

    ctx = malloc(sizeof(FCFSPosixAPIContext));
    if (ctx == NULL) {
        fcfs_jni_throw_exception(env, "Out of Memory");
        return;
    }

    ns = (char *)((*env)->GetStringUTFChars(env, poolname, isCopy));
    config_filename = (char *)((*env)->GetStringUTFChars(
                env, filename, isCopy));
    result = fcfs_posix_api_init_start_ex(ctx,
            log_prefix_name, ns, config_filename);
    (*env)->ReleaseStringUTFChars(env, poolname, ns);
    (*env)->ReleaseStringUTFChars(env, filename, config_filename);

    if (result != 0) {
        free(ctx);
        fcfs_jni_throw_filesystem_exception(env, "/", result);
        return;
    }

    (*env)->CallVoidMethod(env, obj, g_fcfs_jni_global_vars.
            papi.setHandler, (long)ctx);
}

void JNICALL Java_com_fastken_fcfs_FCFSPosixAPI_destroy
  (JNIEnv *env, jobject obj)
{
    long handler;
    FCFSPosixAPIContext *ctx;

    handler = (*env)->CallLongMethod(env, obj,
            g_fcfs_jni_global_vars.papi.getHandler);
    if (handler == 0) {
        return;
    }

    ctx = (FCFSPosixAPIContext *)handler;
    fcfs_posix_api_stop_ex(ctx);
    fcfs_posix_api_destroy_ex(ctx);
    free(ctx);

    (*env)->CallVoidMethod(env, obj, g_fcfs_jni_global_vars.
            papi.setHandler, 0);
}

jobject JNICALL Java_com_fastken_fcfs_FCFSPosixAPI_opendir
  (JNIEnv *env, jobject obj, jstring jpath)
{
    long handler;
    jboolean *isCopy = NULL;
    const char *path;
    FCFSPosixAPIContext *ctx;
    DIR *dir;

    handler = (*env)->CallLongMethod(env, obj,
            g_fcfs_jni_global_vars.papi.getHandler);
    if (handler == 0) {
        fcfs_jni_throw_null_pointer_exception(env);
        return NULL;
    }

    ctx = (FCFSPosixAPIContext *)handler;
    path = (*env)->GetStringUTFChars(env, jpath, isCopy);
    if ((dir=fcfs_opendir_ex(ctx, path)) == NULL) {
        fcfs_jni_throw_filesystem_exception(env, path,
                errno != 0 ? errno : ENOENT);
        (*env)->ReleaseStringUTFChars(env, jpath, path);
        return NULL;
    }
    (*env)->ReleaseStringUTFChars(env, jpath, path);

    return (*env)->NewObject(env, g_fcfs_jni_global_vars.dir.clazz,
            g_fcfs_jni_global_vars.dir.constructor1, (long)dir);
}

jobject JNICALL Java_com_fastken_fcfs_FCFSPosixAPI_open
  (JNIEnv *env, jobject obj, jstring jpath, jint flags, jint mode)
{
    long handler;
    int fd;
    jboolean *isCopy = NULL;
    const char *path;
    FCFSPosixAPIContext *ctx;

    handler = (*env)->CallLongMethod(env, obj,
            g_fcfs_jni_global_vars.papi.getHandler);
    if (handler == 0) {
        fcfs_jni_throw_null_pointer_exception(env);
        return NULL;
    }

    ctx = (FCFSPosixAPIContext *)handler;
    path = (*env)->GetStringUTFChars(env, jpath, isCopy);
    if ((fd=fcfs_open_ex(ctx, path, flags, mode,
                    fcfs_papi_tpid_type_tid)) < 0)
    {
        fcfs_jni_throw_filesystem_exception(env, path,
                errno != 0 ? errno : ENOENT);
        (*env)->ReleaseStringUTFChars(env, jpath, path);
        return NULL;
    }
    (*env)->ReleaseStringUTFChars(env, jpath, path);

    return (*env)->NewObject(env, g_fcfs_jni_global_vars.file.clazz,
            g_fcfs_jni_global_vars.file.constructor1, fd);
}

jstring JNICALL Java_com_fastken_fcfs_FCFSPosixAPI_getcwd
  (JNIEnv *env, jobject obj)
{
    long handler;
    char path[PATH_MAX];
    FCFSPosixAPIContext *ctx;

    handler = (*env)->CallLongMethod(env, obj,
            g_fcfs_jni_global_vars.papi.getHandler);
    if (handler == 0) {
        fcfs_jni_throw_null_pointer_exception(env);
        return NULL;
    }

    ctx = (FCFSPosixAPIContext *)handler;
    if (fcfs_getcwd_ex(ctx, path, sizeof(path)) == NULL) {
        fcfs_jni_throw_filesystem_exception(env, "/",
                errno != 0 ? errno : ENAMETOOLONG);
        return NULL;
    }

    return (*env)->NewStringUTF(env, path);
}

jobject JNICALL Java_com_fastken_fcfs_FCFSPosixAPI_stat
    (JNIEnv *env, jobject obj, jstring jpath, jboolean followlink)
{
    long handler;
    int ret;
    jboolean *isCopy = NULL;
    const char *path;
    FCFSPosixAPIContext *ctx;
    struct stat stat;

    handler = (*env)->CallLongMethod(env, obj,
            g_fcfs_jni_global_vars.papi.getHandler);
    if (handler == 0) {
        fcfs_jni_throw_null_pointer_exception(env);
        return NULL;
    }

    ctx = (FCFSPosixAPIContext *)handler;
    path = (*env)->GetStringUTFChars(env, jpath, isCopy);
    if (followlink) {
        ret = fcfs_stat_ex(ctx, path, &stat);
    }  else {
        ret = fcfs_lstat_ex(ctx, path, &stat);
    }
    if (ret != 0) {
        fcfs_jni_throw_filesystem_exception(env, path,
                errno != 0 ? errno : ENOENT);
        (*env)->ReleaseStringUTFChars(env, jpath, path);
        return NULL;
    }
    (*env)->ReleaseStringUTFChars(env, jpath, path);

    return (*env)->NewObject(env, g_fcfs_jni_global_vars.fstat.clazz,
            g_fcfs_jni_global_vars.fstat.constructor10, stat.st_ino,
            stat.st_mode, stat.st_nlink, stat.st_uid, stat.st_gid,
            stat.st_rdev, stat.st_size, (jlong)stat.st_atime * 1000LL,
            (jlong)stat.st_mtime * 1000LL, (jlong)stat.st_ctime * 1000LL);
}

jstring JNICALL Java_com_fastken_fcfs_FCFSPosixAPI_readlink
  (JNIEnv *env, jobject obj, jstring jpath)
{
    long handler;
    jboolean *isCopy = NULL;
    const char *path;
    char buff[PATH_MAX];
    FCFSPosixAPIContext *ctx;

    handler = (*env)->CallLongMethod(env, obj,
            g_fcfs_jni_global_vars.papi.getHandler);
    if (handler == 0) {
        fcfs_jni_throw_null_pointer_exception(env);
        return NULL;
    }

    ctx = (FCFSPosixAPIContext *)handler;
    path = (*env)->GetStringUTFChars(env, jpath, isCopy);
    if (fcfs_readlink_ex(ctx, path, buff, sizeof(buff)) < 0) {
        fcfs_jni_throw_filesystem_exception(env, path,
                errno != 0 ? errno : ENOENT);
        (*env)->ReleaseStringUTFChars(env, jpath, path);
        return NULL;
    }
    (*env)->ReleaseStringUTFChars(env, jpath, path);

    return (*env)->NewStringUTF(env, buff);
}
