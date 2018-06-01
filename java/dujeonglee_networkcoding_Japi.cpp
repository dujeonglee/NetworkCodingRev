#include "dujeonglee_networkcoding_Japi.h"
#include "api.h"
#include "common.h"
#include <stdint.h>
#include <iostream>

/*
 * Class:     dujeonglee_networkcoding_Japi
 * Method:    InitSocket
 * Signature: (Ljava/lang/String;II)J
 */
JNIEXPORT jlong JNICALL
Java_dujeonglee_networkcoding_Japi_initSocket(JNIEnv *env, jobject obj, jstring port, jint rxTimeout, jint txTimeout)
{
    const char *nativePort = env->GetStringUTFChars(port, 0);

    const long handle = (long)InitSocket(nativePort, static_cast<int>(rxTimeout), static_cast<int>(txTimeout), nullptr);

    env->ReleaseStringUTFChars(port, nativePort);
    return static_cast<jlong>(handle);
}

/*
 * Class:     dujeonglee_networkcoding_Japi
 * Method:    Connect
 * Signature: (JLjava/lang/String;Ljava/lang/String;IIII)Z
 */
JNIEXPORT jboolean JNICALL
Java_dujeonglee_networkcoding_Japi_connect(JNIEnv *env, jobject obj, jlong handle, jstring ip, jstring port, jint timeout, jint transmissionMode, jint blockSize, jint retransmissionRedundancy)
{
    const char *nativeIp = env->GetStringUTFChars(ip, 0);
    const char *nativePort = env->GetStringUTFChars(port, 0);

    const bool result = Connect(
        reinterpret_cast<void *>(handle),
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
 * Class:     dujeonglee_networkcoding_Japi
 * Method:    Disconnect
 * Signature: (JLjava/lang/String;Ljava/lang/String;)V
 */
JNIEXPORT void JNICALL
Java_dujeonglee_networkcoding_Japi_disconnect(JNIEnv *env, jobject obj, jlong handle, jstring ip, jstring port)
{
    const char *nativeIp = env->GetStringUTFChars(ip, 0);
    const char *nativePort = env->GetStringUTFChars(port, 0);

    Disconnect(
        reinterpret_cast<void *>(handle),
        nativeIp,
        nativePort);

    env->ReleaseStringUTFChars(ip, nativeIp);
    env->ReleaseStringUTFChars(port, nativePort);
}

/*
 * Class:     dujeonglee_networkcoding_Japi
 * Method:    Send
 * Signature: (JLjava/lang/String;Ljava/lang/String;[BI)Z
 */
JNIEXPORT jboolean JNICALL
Java_dujeonglee_networkcoding_Japi_send(JNIEnv *env, jobject obj, jlong handle, jstring ip, jstring port, jbyteArray buffer, jint size)
{
    const char *nativeIp = env->GetStringUTFChars(ip, 0);
    const char *nativePort = env->GetStringUTFChars(port, 0);
    jbyte *nativeBuffer = env->GetByteArrayElements(buffer, nullptr);

    const bool result = Send(
        reinterpret_cast<void *>(handle),
        nativeIp,
        nativePort,
        reinterpret_cast<uint8_t *>(reinterpret_cast<char *>(nativeBuffer)),
        static_cast<uint16_t>(size));

    env->ReleaseStringUTFChars(ip, nativeIp);
    env->ReleaseStringUTFChars(port, nativePort);
    env->ReleaseByteArrayElements(buffer, nativeBuffer, JNI_ABORT);
    return static_cast<jboolean>(result);
}

/*
 * Class:     dujeonglee_networkcoding_Japi
 * Method:    Flush
 * Signature: (JLjava/lang/String;Ljava/lang/String;)Z
 */
JNIEXPORT jboolean JNICALL
Java_dujeonglee_networkcoding_Japi_flush(JNIEnv *env, jobject obj, jlong handle, jstring ip, jstring port)
{
    const char *nativeIp = env->GetStringUTFChars(ip, 0);
    const char *nativePort = env->GetStringUTFChars(port, 0);

    const bool result = Flush(
        reinterpret_cast<void *>(handle),
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
Java_dujeonglee_networkcoding_Japi_waitUntilTxIsCompleted(JNIEnv *env, jobject obj, jlong handle, jstring ip, jstring port)
{
    const char *nativeIp = env->GetStringUTFChars(ip, 0);
    const char *nativePort = env->GetStringUTFChars(port, 0);

    WaitUntilTxIsCompleted(
        reinterpret_cast<void *>(handle),
        nativeIp,
        nativePort);

    env->ReleaseStringUTFChars(ip, nativeIp);
    env->ReleaseStringUTFChars(port, nativePort);
}

/*
 * Class:     dujeonglee_networkcoding_Japi
 * Method:    Receive
 * Signature: (J[BI[Ljava/lang/String;II)I
 */
JNIEXPORT jint JNICALL Java_dujeonglee_networkcoding_Japi_receive(JNIEnv *env, jobject obj, jlong handle, jbyteArray buffer, jint bufferSize, jobjectArray senderInfo, jint senderInfoSize, jint timeout)
{
    uint8_t local_buffer[1500] = {0};
    uint16_t local_buffer_length = sizeof(local_buffer);
    NetworkCoding::DataTypes::Address addr;
    addr.AddrLength = sizeof(addr);

    const bool result = Receive(
        reinterpret_cast<void *>(handle),
        local_buffer,
        &local_buffer_length,
        &addr.Addr,
        &addr.AddrLength,
        static_cast<uint32_t>(timeout));
    if (!result)
    {
        return static_cast<jint>(0);
    }
    if((int)local_buffer_length > static_cast<int>(bufferSize))
    {
        return static_cast<jint>(0);
    }
    env->SetByteArrayRegion(buffer, 0, local_buffer_length, reinterpret_cast<jbyte *>(local_buffer));
    if (addr.AddrLength == sizeof(addr.Addr.IPv4))
    {
        char str[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &(addr.Addr.IPv4), str, INET_ADDRSTRLEN);
        if(static_cast<int>(senderInfoSize) > 0)
        {
            env->SetObjectArrayElement(senderInfo, 0, env->NewStringUTF(str));
        }
        if(static_cast<int>(senderInfoSize) > 1)
        {
            env->SetObjectArrayElement(senderInfo, 1, env->NewStringUTF(std::to_string(ntohs(addr.Addr.IPv4.sin_port)).c_str()));
        }
    }
    else if (addr.AddrLength == sizeof(addr.Addr.IPv6))
    {
        char str[INET6_ADDRSTRLEN];
        inet_ntop(AF_INET6, &(addr.Addr.IPv6), str, INET6_ADDRSTRLEN);
        if(static_cast<int>(senderInfoSize) > 0)
        {
            env->SetObjectArrayElement(senderInfo, 0, env->NewStringUTF(str));
        }
        if(static_cast<int>(senderInfoSize) > 1)
        {
            env->SetObjectArrayElement(senderInfo, 1, env->NewStringUTF(std::to_string(ntohs(addr.Addr.IPv4.sin_port)).c_str()));
        }
    }
    else
    {
        if(static_cast<int>(senderInfoSize) > 0)
        {
            env->SetObjectArrayElement(senderInfo, 0, env->NewStringUTF("Unknown Address"));
        }
        if(static_cast<int>(senderInfoSize) > 1)
        {
            env->SetObjectArrayElement(senderInfo, 1, env->NewStringUTF("Unknown Port"));
        }
    }
    return static_cast<jint>(local_buffer_length);
}

/*
 * Class:     dujeonglee_networkcoding_Japi
 * Method:    FreeSocket
 * Signature: (J)V
 */
JNIEXPORT void JNICALL
Java_dujeonglee_networkcoding_Japi_freeSocket(JNIEnv *env, jobject obj, jlong handle)
{
    FreeSocket(reinterpret_cast<void *>(handle));
}
