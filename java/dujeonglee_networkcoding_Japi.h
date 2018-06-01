/* DO NOT EDIT THIS FILE - it is machine generated */
#include <jni.h>
/* Header for class dujeonglee_networkcoding_Japi */

#ifndef _Included_dujeonglee_networkcoding_Japi
#define _Included_dujeonglee_networkcoding_Japi
#ifdef __cplusplus
extern "C" {
#endif
/*
 * Class:     dujeonglee_networkcoding_Japi
 * Method:    InitSocket
 * Signature: (Ljava/lang/String;II)J
 */
JNIEXPORT jlong JNICALL Java_dujeonglee_networkcoding_Japi_initSocket(JNIEnv *, jobject, jstring, jint, jint);

/*
 * Class:     dujeonglee_networkcoding_Japi
 * Method:    Connect
 * Signature: (JLjava/lang/String;Ljava/lang/String;IIII)Z
 */
JNIEXPORT jboolean JNICALL Java_dujeonglee_networkcoding_Japi_connect(JNIEnv *, jobject, jlong, jstring, jstring, jint, jint, jint, jint);

/*
 * Class:     dujeonglee_networkcoding_Japi
 * Method:    Disconnect
 * Signature: (JLjava/lang/String;Ljava/lang/String;)V
 */
JNIEXPORT void JNICALL Java_dujeonglee_networkcoding_Japi_disconnect(JNIEnv *, jobject, jlong, jstring, jstring);

/*
 * Class:     dujeonglee_networkcoding_Japi
 * Method:    Send
 * Signature: (JLjava/lang/String;Ljava/lang/String;[BI)Z
 */
JNIEXPORT jboolean JNICALL Java_dujeonglee_networkcoding_Japi_send(JNIEnv *, jobject, jlong, jstring, jstring, jbyteArray, jint);

/*
 * Class:     dujeonglee_networkcoding_Japi
 * Method:    Flush
 * Signature: (JLjava/lang/String;Ljava/lang/String;)Z
 */
JNIEXPORT jboolean JNICALL Java_dujeonglee_networkcoding_Japi_flush(JNIEnv *, jobject, jlong, jstring, jstring);

/*
 * Class:     dujeonglee_networkcoding_Japi
 * Method:    WaitUntilTxIsCompleted
 * Signature: (JLjava/lang/String;Ljava/lang/String;)V
 */
JNIEXPORT void JNICALL Java_dujeonglee_networkcoding_Japi_waitUntilTxIsCompleted(JNIEnv *, jobject, jlong, jstring, jstring);

/*
 * Class:     dujeonglee_networkcoding_Japi
 * Method:    Receive
 * Signature: (J[BI[Ljava/lang/String;II)I
 */
JNIEXPORT jint JNICALL Java_dujeonglee_networkcoding_Japi_receive(JNIEnv *, jobject, jlong, jbyteArray, jint, jobjectArray, jint, jint);

/*
 * Class:     dujeonglee_networkcoding_Japi
 * Method:    FreeSocket
 * Signature: (J)V
 */
JNIEXPORT void JNICALL Java_dujeonglee_networkcoding_Japi_freeSocket(JNIEnv *, jobject, jlong);

#ifdef __cplusplus
}
#endif
#endif
