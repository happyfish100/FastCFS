
#include "fastcfs/api/fcfs_api.h"
#include "global.h"
#include "com_fastken_fcfs_FCFSPosixAPI.h"

void JNICALL Java_com_fastken_fcfs_FCFSPosixAPI_init
  (JNIEnv *env, jobject obj, jstring filename)
{
    jboolean *isCopy = NULL;
    char *config_filename;
    long handler = 123456;

    config_filename = (char *)((*env)->GetStringUTFChars(env, filename, isCopy));

    (*env)->CallVoidMethod(env, obj, g_fcfs_jni_global_vars.papi.setHandler, handler);

    fprintf(stderr, "config_filename: %s(%d)\n", config_filename, (int)strlen(config_filename));

    (*env)->ReleaseStringUTFChars(env, filename, config_filename);
}

void JNICALL Java_com_fastken_fcfs_FCFSPosixAPI_destroy
  (JNIEnv *env, jobject obj)
{
    long handler;
    handler = (*env)->CallLongMethod(env, obj, g_fcfs_jni_global_vars.papi.getHandler);
    fprintf(stderr, "line: %d, handler: %ld\n", __LINE__, handler);
}
