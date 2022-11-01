
#include "fastcfs/api/std/posix_api.h"
#include "global.h"
#include "com_fastken_fcfs_FCFSPosixAPI.h"

static void throws_exception(JNIEnv *env, const char *message)
{
    jclass exClass;
    exClass = (*env)->FindClass(env, "java/lang/Exception");
    if (exClass == NULL) {
        return;
    }

    (*env)->ThrowNew(env, exClass, message);
}

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
        throws_exception(env, "Out of Memory");
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
        throws_exception(env, strerror(result));
        return;
    }

    fprintf(stderr, "line: %d, ctx: %p\n", __LINE__, ctx);

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
    fprintf(stderr, "line: %d, ctx: %p\n", __LINE__, ctx);

    fcfs_posix_api_stop_ex(ctx);
    fcfs_posix_api_destroy_ex(ctx);
    free(ctx);

    (*env)->CallVoidMethod(env, obj, g_fcfs_jni_global_vars.
            papi.setHandler, 0);
}
