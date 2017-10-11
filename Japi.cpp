#include "Japi.h"
#include "api.h"
#include <iostream>

void RxCallback(uint8_t *const buffer, const uint16_t length, const void *const address, const uint32_t sender_addr_len)
{
    // To-Do
    // Call Java callback.
    /*
    jclass activityClass = env->GetObjectClass(obj);
    jmethodID getTextureCountMethodID = env->GetMethodID(activityClass,
                                                    "getTextureCount", "()I");
    if (getTextureCountMethodID == 0)
    {
        LOG("Function getTextureCount() not found.");
        return;
    }
    textureCount = env->CallIntMethod(obj, getTextureCountMethodID);
    */
}
/*
 * Class:     Japi
 * Method:    InitSocket
 * Signature: (Ljava/lang/String;II)J
 */
JNIEXPORT jlong JNICALL 
Java_Japi_InitSocket(JNIEnv *env, jobject obj, jstring port, jint rxTimeout, jint txTimeout)
{
    const char *nativePort = env->GetStringUTFChars(port, 0);
    const long handle = (long)InitSocket(nativePort, static_cast<int>(rxTimeout), static_cast<int>(txTimeout), RxCallback);
    env->ReleaseStringUTFChars(port, nativePort);
    return static_cast<jlong>(handle);
}

/*
* Class:     Japi
* Method:    Connect
* Signature: (JLjava/lang/String;Ljava/lang/String;IIII)Z
*/
JNIEXPORT jboolean JNICALL
Java_Japi_Connect(JNIEnv *env, jobject obj, jlong handle, jstring ip, jstring port, jint timeout, jint transmissionMode, jint blockSize, jint retransmissionRedundancy)
{
    std::cout<<__FUNCTION__<<std::endl;
    const char *nativeIp = env->GetStringUTFChars(ip, 0);
    const char *nativePort = env->GetStringUTFChars(port, 0);
    const bool result = Connect(
        reinterpret_cast<void*>(handle), 
        nativeIp, 
        nativePort, 
        static_cast<int>(timeout), 
        static_cast<int>(transmissionMode),
        static_cast<int>(blockSize),
        static_cast<int>(retransmissionRedundancy));
    env->ReleaseStringUTFChars(ip, nativeIp);
    env->ReleaseStringUTFChars(port, nativePort);
    return static_cast<jboolean>(result);
}

/*
* Class:     Japi
* Method:    Disconnect
* Signature: (JLjava/lang/String;Ljava/lang/String;)V
*/
JNIEXPORT void JNICALL
Java_Japi_Disconnect(JNIEnv *env, jobject obj, jlong handle, jstring ip, jstring port)
{
    std::cout<<__FUNCTION__<<std::endl;
    const char *nativeIp = env->GetStringUTFChars(ip, 0);
    const char *nativePort = env->GetStringUTFChars(port, 0);
    Disconnect(
        reinterpret_cast<void*>(handle), 
        nativeIp, 
        nativePort);
    env->ReleaseStringUTFChars(ip, nativeIp);
    env->ReleaseStringUTFChars(port, nativePort);
}

/*
* Class:     Japi
* Method:    Send
* Signature: (JLjava/lang/String;Ljava/lang/String;[BI)Z
*/
JNIEXPORT jboolean JNICALL
Java_Japi_Send(JNIEnv *env, jobject obj, jlong handle, jstring ip, jstring port, jbyteArray buffer, jint size)
{
    std::cout<<__FUNCTION__<<std::endl;
    const char *nativeIp = env->GetStringUTFChars(ip, 0);
    const char *nativePort = env->GetStringUTFChars(port, 0);
    jbyte* nativeBuffer = env->GetByteArrayElements(buffer, nullptr);
    const bool result = Send(
        reinterpret_cast<void*>(handle),
        nativeIp,
        nativePort,
        reinterpret_cast<uint8_t*>(reinterpret_cast<char*>(nativeBuffer)),
        static_cast<uint16_t>(size)
    );
    env->ReleaseStringUTFChars(ip, nativeIp);
    env->ReleaseStringUTFChars(port, nativePort);
    env->ReleaseByteArrayElements(buffer, nativeBuffer, JNI_ABORT);
    return static_cast<jboolean>(result);
}

/*
* Class:     Japi
* Method:    Flush
* Signature: (JLjava/lang/String;Ljava/lang/String;)Z
*/
JNIEXPORT jboolean JNICALL
Java_Japi_Flush(JNIEnv *env, jobject obj, jlong handle, jstring ip, jstring port)
{
    std::cout<<__FUNCTION__<<std::endl;
    const char *nativeIp = env->GetStringUTFChars(ip, 0);
    const char *nativePort = env->GetStringUTFChars(port, 0);
    const bool result = Flush(
        reinterpret_cast<void*>(handle), 
        nativeIp, 
        nativePort);
    env->ReleaseStringUTFChars(ip, nativeIp);
    env->ReleaseStringUTFChars(port, nativePort);
    return static_cast<jboolean>(result);
}

/*
* Class:     Japi
* Method:    WaitUntilTxIsCompleted
* Signature: (JLjava/lang/String;Ljava/lang/String;)V
*/
JNIEXPORT void JNICALL
Java_Japi_WaitUntilTxIsCompleted(JNIEnv *env, jobject obj, jlong handle, jstring ip, jstring port)
{
    std::cout<<__FUNCTION__<<std::endl;
    const char *nativeIp = env->GetStringUTFChars(ip, 0);
    const char *nativePort = env->GetStringUTFChars(port, 0);
    WaitUntilTxIsCompleted(
        reinterpret_cast<void*>(handle), 
        nativeIp, 
        nativePort);
    env->ReleaseStringUTFChars(ip, nativeIp);
    env->ReleaseStringUTFChars(port, nativePort);
}

/*
* Class:     Japi
* Method:    FreeSocket
* Signature: (J)V
*/
JNIEXPORT void JNICALL
Java_Japi_FreeSocket(JNIEnv *env, jobject obj, jlong handle)
{
    std::cout<<__FUNCTION__<<std::endl;
    FreeSocket(reinterpret_cast<void*>(handle)); 
}
