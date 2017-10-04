#ifndef C_STUB_H_
#define C_STUB_H_

#include <string>
#include <cstdint>

#include <sys/types.h>
#include <sys/socket.h>

typedef void(*rxcallback)(uint8_t* buffer, uint16_t length, const sockaddr* const sender_addr, const uint32_t sender_addr_len);

typedef void* (*InitSocketFuncType)(const char* local_port, const long int RxTimeout, const long int TxTimeout, const rxcallback cb);
typedef bool (*ConnectFuncType)(void* const handle, const std::string ip, const std::string port, uint32_t timeout, uint8_t TransmissionMode, uint8_t BlockSize, uint16_t RetransmissionRedundancy);
typedef void (*DisconnectFuncType)(void* const handle, const std::string ip, const std::string port);
typedef bool (*SendFuncType)(void* const handle, const std::string ip, const std::string port, uint8_t* buff, uint16_t size);
typedef bool (*FlushFuncType)(void* const handle, const std::string ip, const std::string port);
typedef void (*WaitUntilTxIsCompletedFuncType)(void* const handle, const std::string ip, const std::string port);
typedef void (*FreeSocketFuncType)(void* const handle);

extern "C" {

void *InitSocket(const char* local_port, const long int RxTimeout, const long int TxTimeout, const rxcallback cb);
bool Connect(void* const handle, const std::string ip, const std::string port, uint32_t timeout, uint8_t TransmissionMode, uint8_t BlockSize, uint16_t RetransmissionRedundancy);
void Disconnect(void* const handle, const std::string ip, const std::string port);
bool Send(void* const handle, const std::string ip, const std::string port, uint8_t* buff, uint16_t size);
bool Flush(void* const handle, const std::string ip, const std::string port);
void WaitUntilTxIsCompleted(void* const handle, const std::string ip, const std::string port);
void FreeSocket(void* handle);

}

#endif
