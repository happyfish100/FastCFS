
#include "fastcfs/api/std/posix_api.h"
#include "global.h"
#include "common.h"
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

#define PAPI_SET_CTX_AND_PATH_EX(retval, path)  \
    long handler;  \
    jboolean *isCopy = NULL; \
    const char *path; \
    FCFSPosixAPIContext *ctx; \
    \
    handler = (*env)->CallLongMethod(env, obj,       \
            g_fcfs_jni_global_vars.papi.getHandler); \
    if (handler == 0) {  \
        fcfs_jni_throw_null_pointer_exception(env); \
        return retval; \
    } \
    \
    ctx = (FCFSPosixAPIContext *)handler; \
    path = (*env)->GetStringUTFChars(env, j##path, isCopy)

#define PAPI_SET_CTX_AND_PATH(retval)  \
    PAPI_SET_CTX_AND_PATH_EX(retval, path)

jobject JNICALL Java_com_fastken_fcfs_FCFSPosixAPI_opendir
  (JNIEnv *env, jobject obj, jstring jpath)
{
    DIR *dir;

    PAPI_SET_CTX_AND_PATH(NULL);
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
    int fd;

    PAPI_SET_CTX_AND_PATH(NULL);
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
    int ret;
    struct stat stat;

    PAPI_SET_CTX_AND_PATH(NULL);
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
    char buff[PATH_MAX];

    PAPI_SET_CTX_AND_PATH(NULL);
    if (fcfs_readlink_ex(ctx, path, buff, sizeof(buff)) < 0) {
        fcfs_jni_throw_filesystem_exception(env, path,
                errno != 0 ? errno : ENOENT);
        (*env)->ReleaseStringUTFChars(env, jpath, path);
        return NULL;
    }
    (*env)->ReleaseStringUTFChars(env, jpath, path);

    return (*env)->NewStringUTF(env, buff);
}

jobject JNICALL Java_com_fastken_fcfs_FCFSPosixAPI_statvfs
  (JNIEnv *env, jobject obj, jstring jpath)
{
    struct statvfs stat;
    SFSpaceStat space;

    PAPI_SET_CTX_AND_PATH(NULL);
    if (fcfs_statvfs_ex(ctx, path, &stat) != 0) {
        fcfs_jni_throw_filesystem_exception(env, path,
                errno != 0 ? errno : ENOENT);
        (*env)->ReleaseStringUTFChars(env, jpath, path);
        return NULL;
    }
    (*env)->ReleaseStringUTFChars(env, jpath, path);

    space.total = (int64_t)stat.f_blocks * stat.f_bsize;
    space.avail = (int64_t)stat.f_bavail * stat.f_bsize;
    space.used = space.total - space.avail;
    return (*env)->NewObject(env, g_fcfs_jni_global_vars.statvfs.clazz,
            g_fcfs_jni_global_vars.statvfs.constructor6, space.total,
            space.avail, space.used, stat.f_files, stat.f_favail,
            stat.f_files - stat.f_favail);
}

jbyteArray JNICALL Java_com_fastken_fcfs_FCFSPosixAPI_getxattr
  (JNIEnv *env, jobject obj, jstring jpath, jstring jname, jboolean followlink)
{
    const char *name;
    char holder[4 * 1024];
    char *buff;
    int result;
    int size;
    int length;
    jbyteArray value;

    PAPI_SET_CTX_AND_PATH(NULL);
    name = (*env)->GetStringUTFChars(env, jname, isCopy);

    buff = holder;
    size = sizeof(holder);
    if (followlink) {
        length = fcfs_getxattr_ex(ctx, path, name, buff, size);
    }  else {
        length = fcfs_lgetxattr_ex(ctx, path, name, buff, size);
    }
    if (length < 0) {
        result = errno != 0 ? errno : ENOENT;
        if (result == EOVERFLOW) {
            if (followlink) {
                size = fcfs_getxattr_ex(ctx, path, name, buff, 0);
            }  else {
                size = fcfs_lgetxattr_ex(ctx, path, name, buff, 0);
            }
            if (size < 0) {
                result = errno != 0 ? errno : ENOENT;
            } else {
                if ((buff=malloc(size)) == NULL) {
                    result = ENOMEM;
                } else {
                    if (followlink) {
                        length = fcfs_getxattr_ex(ctx, path, name, buff, size);
                    }  else {
                        length = fcfs_lgetxattr_ex(ctx, path, name, buff, size);
                    }
                    if (length < 0) {
                        result = errno != 0 ? errno : ENOENT;
                    } else {
                        result = 0;
                    }
                }
            }
        }

        if (result != 0) {
            fcfs_jni_throw_filesystem_exception(env, path, result);
            (*env)->ReleaseStringUTFChars(env, jpath, path);
            (*env)->ReleaseStringUTFChars(env, jname, name);
            return NULL;
        }
    }

    (*env)->ReleaseStringUTFChars(env, jpath, path);
    (*env)->ReleaseStringUTFChars(env, jname, name);
    value = (*env)->NewByteArray(env, length);
    (*env)->SetByteArrayRegion(env, value, 0, length,
            (const jbyte *)buff);
    if (buff != holder) {
        free(buff);
    }

    return value;
}

jobject JNICALL Java_com_fastken_fcfs_FCFSPosixAPI_listxattr
  (JNIEnv *env, jobject obj, jstring jpath, jboolean followlink)
{
    char holder[4 * 1024];
    char *buff;
    int result;
    int size;
    int length;
    jobject list_obj;

    PAPI_SET_CTX_AND_PATH(NULL);
    buff = holder;
    size = sizeof(holder);
    if (followlink) {
        length = fcfs_listxattr_ex(ctx, path, buff, size);
    }  else {
        length = fcfs_llistxattr_ex(ctx, path, buff, size);
    }
    if (length < 0) {
        result = errno != 0 ? errno : ENOENT;
        if (result == EOVERFLOW) {
            if (followlink) {
                size = fcfs_listxattr_ex(ctx, path, buff, 0);
            }  else {
                size = fcfs_llistxattr_ex(ctx, path, buff, 0);
            }
            if (size < 0) {
                result = errno != 0 ? errno : ENOENT;
            } else {
                if ((buff=malloc(size)) == NULL) {
                    result = ENOMEM;
                } else {
                    if (followlink) {
                        length = fcfs_listxattr_ex(ctx, path, buff, size);
                    }  else {
                        length = fcfs_llistxattr_ex(ctx, path, buff, size);
                    }
                    if (length < 0) {
                        result = errno != 0 ? errno : ENOENT;
                    } else {
                        result = 0;
                    }
                }
            }
        }

        if (result != 0) {
            fcfs_jni_throw_filesystem_exception(env, path, result);
            (*env)->ReleaseStringUTFChars(env, jpath, path);
            return NULL;
        }
    }
    (*env)->ReleaseStringUTFChars(env, jpath, path);
    list_obj = fcfs_jni_convert_to_list(env, buff, length);
    if (buff != holder) {
        free(buff);
    }

    return list_obj;
}

void JNICALL Java_com_fastken_fcfs_FCFSPosixAPI_truncate
  (JNIEnv *env, jobject obj, jstring jpath, jlong length)
{
    PAPI_SET_CTX_AND_PATH();

    if (fcfs_truncate_ex(ctx, path, length) < 0) {
        fcfs_jni_throw_filesystem_exception(env,
                path, errno != 0 ? errno : EIO);
    }
    (*env)->ReleaseStringUTFChars(env, jpath, path);
}

void JNICALL Java_com_fastken_fcfs_FCFSPosixAPI_link
  (JNIEnv *env, jobject obj, jstring jpath1, jstring jpath2)
{
    const char *path2;

    PAPI_SET_CTX_AND_PATH_EX(, path1);
    path2 = (*env)->GetStringUTFChars(env, jpath2, isCopy);
    if (fcfs_link_ex(ctx, path1, path2) < 0) {
        fcfs_jni_throw_filesystem_exception(env,
                path1, errno != 0 ? errno : EIO);
    }
    (*env)->ReleaseStringUTFChars(env, jpath1, path1);
    (*env)->ReleaseStringUTFChars(env, jpath2, path2);
}

void JNICALL Java_com_fastken_fcfs_FCFSPosixAPI_symlink
  (JNIEnv *env, jobject obj, jstring jlink, jstring jpath)
{
    const char *path;

    PAPI_SET_CTX_AND_PATH_EX(, link);
    path = (*env)->GetStringUTFChars(env, jpath, isCopy);
    if (fcfs_symlink_ex(ctx, link, path) < 0) {
        fcfs_jni_throw_filesystem_exception(env,
                link, errno != 0 ? errno : EIO);
    }
    (*env)->ReleaseStringUTFChars(env, jlink, link);
    (*env)->ReleaseStringUTFChars(env, jpath, path);
}

void JNICALL Java_com_fastken_fcfs_FCFSPosixAPI_rename
  (JNIEnv *env, jobject obj, jstring jpath1, jstring jpath2)
{
    const char *path2;

    PAPI_SET_CTX_AND_PATH_EX(, path1);
    path2 = (*env)->GetStringUTFChars(env, jpath2, isCopy);
    if (fcfs_rename_ex(ctx, path1, path2) < 0) {
        fcfs_jni_throw_filesystem_exception(env,
                path1, errno != 0 ? errno : EIO);
    }
    (*env)->ReleaseStringUTFChars(env, jpath1, path1);
    (*env)->ReleaseStringUTFChars(env, jpath2, path2);
}
