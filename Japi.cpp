#include "Japi.h"
#include "api.h"
#include "common.h"
#include <iostream>
//#include <stdlib.h>

/*
 * Class:     Japi
 * Method:    InitSocket
 * Signature: (Ljava/lang/String;II)J
 */
JNIEXPORT jlong JNICALL
Java_Japi_InitSocket(JNIEnv *env, jobject obj, jstring port, jint rxTimeout, jint txTimeout)
{
    std::cout << __FUNCTION__ << std::endl;
    const char *nativePort = env->GetStringUTFChars(port, 0);

    const long handle = (long)InitSocket(nativePort, static_cast<int>(rxTimeout), static_cast<int>(txTimeout), nullptr);

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
    std::cout << __FUNCTION__ << std::endl;
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
* Class:     Japi
* Method:    Disconnect
* Signature: (JLjava/lang/String;Ljava/lang/String;)V
*/
JNIEXPORT void JNICALL
Java_Japi_Disconnect(JNIEnv *env, jobject obj, jlong handle, jstring ip, jstring port)
{
    std::cout << __FUNCTION__ << std::endl;
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
* Class:     Japi
* Method:    Send
* Signature: (JLjava/lang/String;Ljava/lang/String;[BI)Z
*/
JNIEXPORT jboolean JNICALL
Java_Japi_Send(JNIEnv *env, jobject obj, jlong handle, jstring ip, jstring port, jbyteArray buffer, jint size)
{
    std::cout << __FUNCTION__ << std::endl;
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
* Class:     Japi
* Method:    Flush
* Signature: (JLjava/lang/String;Ljava/lang/String;)Z
*/
JNIEXPORT jboolean JNICALL
Java_Japi_Flush(JNIEnv *env, jobject obj, jlong handle, jstring ip, jstring port)
{
    std::cout << __FUNCTION__ << std::endl;
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
Java_Japi_WaitUntilTxIsCompleted(JNIEnv *env, jobject obj, jlong handle, jstring ip, jstring port)
{
    std::cout << __FUNCTION__ << std::endl;
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
 * Class:     Japi
 * Method:    Receive
 * Signature: (J[B[Ljava/lang/String;)I
 */
JNIEXPORT jint JNICALL 
Java_Japi_Receive(JNIEnv *env, jobject obj, jlong handle, jbyteArray buffer, jobjectArray sender)
{
    std::cout << __FUNCTION__ << std::endl;
    uint8_t local_buffer[1500];
    uint16_t local_buffer_length = sizeof(local_buffer);
    NetworkCoding::DataStructures::AddressType addr;
    addr.AddrLength = sizeof(addr);

    const bool result = Receive(
        reinterpret_cast<void *>(handle),
        local_buffer,
        &local_buffer_length,
        &addr.Addr,
        &addr.AddrLength);
    if (!result)
    {
        return static_cast<jint>(0);
    }
    env->SetByteArrayRegion(buffer, 0, local_buffer_length, reinterpret_cast<jbyte *>(local_buffer));
    if (addr.AddrLength == sizeof(addr.Addr.IPv4))
    {
        char str[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &(addr.Addr.IPv4), str, INET_ADDRSTRLEN);
        env->SetObjectArrayElement(sender, 0, env->NewStringUTF(str));
        env->SetObjectArrayElement(
            sender, 
            1, 
            env->NewStringUTF(std::to_string(ntohs(addr.Addr.IPv4.sin_port)).c_str())
        );
    }
    else if (addr.AddrLength == sizeof(addr.Addr.IPv6))
    {
        char str[INET6_ADDRSTRLEN];
        inet_ntop(AF_INET6, &(addr.Addr.IPv6), str, INET6_ADDRSTRLEN);
        env->SetObjectArrayElement(sender, 0, env->NewStringUTF(str));
        env->SetObjectArrayElement(
            sender, 
            1, 
            env->NewStringUTF(std::to_string(ntohs(addr.Addr.IPv6.sin6_port)).c_str())
        );
    }
    else
    {
    }

    return static_cast<jint>(local_buffer_length);
}

/*
* Class:     Japi
* Method:    FreeSocket
* Signature: (J)V
*/
JNIEXPORT void JNICALL
Java_Japi_FreeSocket(JNIEnv *env, jobject obj, jlong handle)
{
    std::cout << __FUNCTION__ << std::endl;
    FreeSocket(reinterpret_cast<void *>(handle));
}
