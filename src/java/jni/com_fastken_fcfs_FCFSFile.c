
#include "fastcfs/api/std/posix_api.h"
#include "global.h"
#include "common.h"
#include "com_fastken_fcfs_FCFSFile.h"

void JNICALL Java_com_fastken_fcfs_FCFSFile_close
  (JNIEnv *env, jobject obj)
{
    int fd;

    fd = (*env)->CallIntMethod(env, obj, g_fcfs_jni_global_vars.file.getFD);
    if (fd < 0) {
        return;
    }

    fcfs_close(fd);
    (*env)->CallVoidMethod(env, obj, g_fcfs_jni_global_vars.file.setFD, -1);
}

#define FILE_OBJ_FETCH_FD(retval)  \
    int fd;  \
    \
    fd = (*env)->CallIntMethod(env, obj, g_fcfs_jni_global_vars.file.getFD); \
    if (fd < 0) {  \
        fcfs_jni_throw_null_pointer_exception(env); \
        return retval; \
    } \

static inline void throw_file_exception(JNIEnv *env,
        const int fd, const int err_no)
{
    FCFSPosixAPIFileInfo *handle;

    if ((handle=fcfs_fd_manager_get(fd)) == NULL) {
        fcfs_jni_throw_exception(env, strerror(err_no));
    } else {
        fcfs_jni_throw_filesystem_exception(env, handle->filename.str, err_no);
    }
}

jobject JNICALL Java_com_fastken_fcfs_FCFSFile_stat
  (JNIEnv *env, jobject obj)
{
    struct stat stat;

    FILE_OBJ_FETCH_FD(NULL);
    if (fcfs_fstat(fd, &stat) != 0) {
        throw_file_exception(env, fd, errno != 0 ? errno : ENOENT);
        return NULL;
    }

    return (*env)->NewObject(env, g_fcfs_jni_global_vars.fstat.clazz,
            g_fcfs_jni_global_vars.fstat.constructor10, stat.st_ino,
            stat.st_mode, stat.st_nlink, stat.st_uid, stat.st_gid,
            stat.st_rdev, stat.st_size, (jlong)stat.st_atime * 1000LL,
            (jlong)stat.st_mtime * 1000LL, (jlong)stat.st_ctime * 1000LL);
}

jobject JNICALL Java_com_fastken_fcfs_FCFSFile_statvfs
  (JNIEnv *env, jobject obj)
{
    struct statvfs stat;
    SFSpaceStat space;

    FILE_OBJ_FETCH_FD(NULL);

    if (fcfs_fstatvfs(fd, &stat) != 0) {
        throw_file_exception(env, fd, errno != 0 ? errno : ENOENT);
        return NULL;
    }

    space.total = (int64_t)stat.f_blocks * stat.f_bsize;
    space.avail = (int64_t)stat.f_bavail * stat.f_bsize;
    space.used = space.total - space.avail;
    return (*env)->NewObject(env, g_fcfs_jni_global_vars.statvfs.clazz,
            g_fcfs_jni_global_vars.statvfs.constructor6, space.total,
            space.avail, space.used, stat.f_files, stat.f_favail,
            stat.f_files - stat.f_favail);
}

jbyteArray JNICALL Java_com_fastken_fcfs_FCFSFile_getxattr
  (JNIEnv *env, jobject obj, jstring jname)
{
    jboolean *isCopy = NULL;
    const char *name;
    char holder[4 * 1024];
    char *buff;
    int result;
    int size;
    int length;
    jbyteArray value;

    FILE_OBJ_FETCH_FD(NULL);
    name = (*env)->GetStringUTFChars(env, jname, isCopy);

    buff = holder;
    size = sizeof(holder);
    if ((length=fcfs_fgetxattr(fd, name, buff, size)) < 0) {
        result = errno != 0 ? errno : ENOENT;
        if (result == EOVERFLOW) {
            if ((size=fcfs_fgetxattr(fd, name, buff, 0)) < 0) {
                result = errno != 0 ? errno : ENOENT;
            } else {
                if ((buff=malloc(size)) == NULL) {
                    result = ENOMEM;
                } else {
                    if ((length=fcfs_fgetxattr(fd, name, buff, size)) < 0) {
                        result = errno != 0 ? errno : ENOENT;
                    } else {
                        result = 0;
                    }
                }
            }
        }

        if (result != 0) {
            throw_file_exception(env, fd, result);
            (*env)->ReleaseStringUTFChars(env, jname, name);
            return NULL;
        }
    }

    (*env)->ReleaseStringUTFChars(env, jname, name);
    value = (*env)->NewByteArray(env, length);
    (*env)->SetByteArrayRegion(env, value, 0, length,
            (const jbyte *)buff);
    if (buff != holder) {
        free(buff);
    }

    return value;
}

jobject JNICALL Java_com_fastken_fcfs_FCFSFile_listxattr
  (JNIEnv *env, jobject obj)
{
    char holder[4 * 1024];
    char *buff;
    int result;
    int size;
    int length;
    jobject list_obj;

    FILE_OBJ_FETCH_FD(NULL);
    buff = holder;
    size = sizeof(holder);
    if ((length=fcfs_flistxattr(fd, buff, size)) < 0) {
        result = errno != 0 ? errno : ENOENT;
        if (result == EOVERFLOW) {
            if ((size=fcfs_flistxattr(fd, buff, 0)) < 0) {
                result = errno != 0 ? errno : ENOENT;
            } else {
                if ((buff=malloc(size)) == NULL) {
                    result = ENOMEM;
                } else {
                    if ((length=fcfs_flistxattr(fd, buff, size)) < 0) {
                        result = errno != 0 ? errno : ENOENT;
                    } else {
                        result = 0;
                    }
                }
            }
        }

        if (result != 0) {
            throw_file_exception(env, fd, result);
            return NULL;
        }
    }

    list_obj = fcfs_jni_convert_to_list(env, buff, length);
    if (buff != holder) {
        free(buff);
    }

    return list_obj;
}
