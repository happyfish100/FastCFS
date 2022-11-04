/* DO NOT EDIT THIS FILE - it is machine generated */
#include <jni.h>
/* Header for class com_fastken_fcfs_FCFSDirectory */

#ifndef _Included_com_fastken_fcfs_FCFSDirectory
#define _Included_com_fastken_fcfs_FCFSDirectory
#ifdef __cplusplus
extern "C" {
#endif
/*
 * Class:     com_fastken_fcfs_FCFSDirectory
 * Method:    doInit
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_com_fastken_fcfs_FCFSDirectory_doInit
  (JNIEnv *, jclass);

/*
 * Class:     com_fastken_fcfs_FCFSDirectory
 * Method:    next
 * Signature: ()Lcom/fastken/fcfs/FCFSDirectory/Entry;
 */
JNIEXPORT jobject JNICALL Java_com_fastken_fcfs_FCFSDirectory_next
  (JNIEnv *, jobject);

/*
 * Class:     com_fastken_fcfs_FCFSDirectory
 * Method:    seek
 * Signature: (J)V
 */
JNIEXPORT void JNICALL Java_com_fastken_fcfs_FCFSDirectory_seek
  (JNIEnv *, jobject, jlong);

/*
 * Class:     com_fastken_fcfs_FCFSDirectory
 * Method:    tell
 * Signature: ()J
 */
JNIEXPORT jlong JNICALL Java_com_fastken_fcfs_FCFSDirectory_tell
  (JNIEnv *, jobject);

/*
 * Class:     com_fastken_fcfs_FCFSDirectory
 * Method:    rewind
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_com_fastken_fcfs_FCFSDirectory_rewind
  (JNIEnv *, jobject);

/*
 * Class:     com_fastken_fcfs_FCFSDirectory
 * Method:    close
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_com_fastken_fcfs_FCFSDirectory_close
  (JNIEnv *, jobject);

#ifdef __cplusplus
}
#endif
#endif