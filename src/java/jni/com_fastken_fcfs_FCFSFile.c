
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

void JNICALL Java_com_fastken_fcfs_FCFSFile_sync
  (JNIEnv *env, jobject obj)
{
    FILE_OBJ_FETCH_FD();

    if (fcfs_fsync(fd) < 0) {
        throw_file_exception(env, fd, errno != 0 ? errno : EIO);
    }
}

void JNICALL Java_com_fastken_fcfs_FCFSFile_datasync
  (JNIEnv *env, jobject obj)
{
    FILE_OBJ_FETCH_FD();

    if (fcfs_fdatasync(fd) < 0) {
        throw_file_exception(env, fd, errno != 0 ? errno : EIO);
    }
}

#define FILE_CHECK_ARRAY_BOUNDS_EX(b, off, len, size, retval) \
    do { \
        size = (*env)->GetArrayLength(env, b); \
        if (off < 0 || off >= size) { \
            fcfs_jni_throw_out_of_bounds_exception(env, off); \
            return retval; \
        } \
        if (off + len > size) { \
            fcfs_jni_throw_out_of_bounds_exception(env, off + len); \
            return retval; \
        } \
    } while (0)

#define FILE_CHECK_ARRAY_BOUNDS(b, off, len, size)   \
        FILE_CHECK_ARRAY_BOUNDS_EX(b, off, len, size, )

void JNICALL Java_com_fastken_fcfs_FCFSFile_write
  (JNIEnv *env, jobject obj, jbyteArray bs, jint off, jint len)
{
    jbyte *ba;
    jsize size;

    FILE_OBJ_FETCH_FD();
    FILE_CHECK_ARRAY_BOUNDS(bs, off, len, size);
    ba = (*env)->GetByteArrayElements(env, bs, NULL);
    if (fcfs_write(fd, ba + off, len) < 0) {
        throw_file_exception(env, fd, errno != 0 ? errno : EIO);
    }
    (*env)->ReleaseByteArrayElements(env, bs, ba, 0);
}

jint JNICALL Java_com_fastken_fcfs_FCFSFile_read
  (JNIEnv *env, jobject obj, jbyteArray bs, jint off, jint len)
{
    jbyte *ba;
    jsize size;
    int bytes;

    FILE_OBJ_FETCH_FD(-1);
    FILE_CHECK_ARRAY_BOUNDS_EX(bs, off, len, size, -1);
    ba = (*env)->GetByteArrayElements(env, bs, NULL);
    if ((bytes=fcfs_read(fd, ba + off, len)) < 0) {
        throw_file_exception(env, fd, errno != 0 ? errno : EIO);
    }
    (*env)->ReleaseByteArrayElements(env, bs, ba, 0);
    return bytes;
}

jlong JNICALL Java_com_fastken_fcfs_FCFSFile_lseek
  (JNIEnv *env, jobject obj, jlong offset, jint whence)
{
    jlong position;

    FILE_OBJ_FETCH_FD(-1);
    if ((position=fcfs_lseek(fd, offset, whence)) < 0) {
        throw_file_exception(env, fd, errno != 0 ? errno : EIO);
    }
    return position;
}

void JNICALL Java_com_fastken_fcfs_FCFSFile_allocate
  (JNIEnv *env, jobject obj, jint mode, jlong offset, jlong length)
{
    FILE_OBJ_FETCH_FD();
    if (fcfs_fallocate(fd, mode, offset, length) < 0) {
        throw_file_exception(env, fd, errno != 0 ? errno : EIO);
    }
}

void JNICALL Java_com_fastken_fcfs_FCFSFile_truncate
  (JNIEnv *env, jobject obj, jlong length)
{
    FILE_OBJ_FETCH_FD();
    if (fcfs_ftruncate(fd, length) < 0) {
        throw_file_exception(env, fd, errno != 0 ? errno : EIO);
    }
}

jboolean JNICALL Java_com_fastken_fcfs_FCFSFile_lock
  (JNIEnv *env, jobject obj, jlong position, jlong length,
   jboolean shared, jboolean blocked)
{
    struct flock lock;
    int result;

    FILE_OBJ_FETCH_FD(false);
    lock.l_type = (shared ? F_RDLCK : F_WRLCK);
    lock.l_whence = SEEK_SET;
    lock.l_start = position;
    lock.l_len = length;
    lock.l_pid = getpid();
    if (fcfs_fcntl(fd, (blocked ? F_SETLKW : F_SETLK), &lock) < 0) {
        result = errno != 0 ? errno : EIO;
        if (blocked || result != EWOULDBLOCK) {
            throw_file_exception(env, fd, result);
        }
        return false;
    }
    return true;
}

void JNICALL Java_com_fastken_fcfs_FCFSFile_unlock
  (JNIEnv *env, jobject obj, jlong position, jlong length)
{
    struct flock lock;

    FILE_OBJ_FETCH_FD();
    lock.l_type = F_UNLCK;
    lock.l_whence = SEEK_SET;
    lock.l_start = position;
    lock.l_len = length;
    lock.l_pid = getpid();
    if (fcfs_fcntl(fd, F_SETLK, &lock) < 0) {
        throw_file_exception(env, fd, errno != 0 ? errno : EIO);
    }
}

void JNICALL Java_com_fastken_fcfs_FCFSFile_utimes
  (JNIEnv *env, jobject obj, jlong atime, jlong mtime)
{
    struct timeval times[2];

    FILE_OBJ_FETCH_FD();
    times[0].tv_sec = atime / 1000;
    times[0].tv_usec = (atime % 1000) * 1000;
    times[1].tv_sec = mtime / 1000;
    times[1].tv_usec = (mtime % 1000) * 1000;
    if (fcfs_futimes(fd, times) < 0) {
        throw_file_exception(env, fd, errno != 0 ? errno : EIO);
    }
}

void JNICALL Java_com_fastken_fcfs_FCFSFile_chown
  (JNIEnv *env, jobject obj, jint owner, jint group)
{
    FILE_OBJ_FETCH_FD();
    if (fcfs_fchown(fd, owner, group) < 0) {
        throw_file_exception(env, fd, errno != 0 ? errno : EIO);
    }
}

void JNICALL Java_com_fastken_fcfs_FCFSFile_chmod
  (JNIEnv *env, jobject obj, jint mode)
{
    FILE_OBJ_FETCH_FD();
    if (fcfs_fchmod(fd, mode) < 0) {
        throw_file_exception(env, fd, errno != 0 ? errno : EIO);
    }
}

void JNICALL Java_com_fastken_fcfs_FCFSFile_setxattr
  (JNIEnv *env, jobject obj, jstring jname, jbyteArray bs,
   jint off, jint len, jint flags)
{
    const char *name;
    jbyte *ba;
    jsize size;

    FILE_OBJ_FETCH_FD();
    FILE_CHECK_ARRAY_BOUNDS(bs, off, len, size);
    ba = (*env)->GetByteArrayElements(env, bs, NULL);
    name = (*env)->GetStringUTFChars(env, jname, NULL);
    if (fcfs_fsetxattr(fd, name, ba + off, len, flags) < 0) {
        throw_file_exception(env, fd, errno != 0 ? errno : EIO);
    }
    (*env)->ReleaseStringUTFChars(env, jname, name);
    (*env)->ReleaseByteArrayElements(env, bs, ba, 0);
}

void JNICALL Java_com_fastken_fcfs_FCFSFile_removexattr
  (JNIEnv *env, jobject obj, jstring jname)
{
    const char *name;

    FILE_OBJ_FETCH_FD();
    name = (*env)->GetStringUTFChars(env, jname, NULL);
    if (fcfs_fremovexattr(fd, name) < 0) {
        throw_file_exception(env, fd, errno != 0 ? errno : EIO);
    }
    (*env)->ReleaseStringUTFChars(env, jname, name);
}

void JNICALL Java_com_fastken_fcfs_FCFSFile_chdir
  (JNIEnv *env, jobject obj)
{
    FILE_OBJ_FETCH_FD();
    if (fcfs_fchdir(fd) < 0) {
        throw_file_exception(env, fd, errno != 0 ? errno : EIO);
    }
}
