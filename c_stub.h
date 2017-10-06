#ifndef C_STUB_H_
#define C_STUB_H_

#include <string>
#include <cstdint>
#include "common.h"

typedef void (*rxcallback)(uint8_t *const buffer, const uint16_t length, const void *const sockaddr_type, const uint32_t sender_addr_len);

typedef void *(*InitSocketFuncType)(const char *local_port, const long int RxTimeout, const long int TxTimeout, const rxcallback cb);
typedef bool (*ConnectFuncType)(void *const handle, const char *const ip, const char *const port, uint32_t timeout, uint8_t TransmissionMode, uint8_t BlockSize, uint16_t RetransmissionRedundancy);
typedef void (*DisconnectFuncType)(void *const handle, const char *const ip, const char *const port);
typedef bool (*SendFuncType)(void *const handle, const char *const ip, const char *const port, uint8_t *buff, uint16_t size);
typedef bool (*FlushFuncType)(void *const handle, const char *const ip, const char *const port);
typedef void (*WaitUntilTxIsCompletedFuncType)(void *const handle, const char *const ip, const char *const port);
typedef void (*FreeSocketFuncType)(void *const handle);

extern "C" {

void *InitSocket(const char *const local_port, const long int RxTimeout, const long int TxTimeout, const rxcallback cb);
bool Connect(void *const handle, const char *const ip, const char *const port, uint32_t timeout, uint8_t TransmissionMode, uint8_t BlockSize, uint16_t RetransmissionRedundancy);
void Disconnect(void *const handle, const char *const ip, const char *const port);
bool Send(void *const handle, const char *const ip, const char *const port, uint8_t *buff, uint16_t size);
bool Flush(void *const handle, const char *const ip, const char *const port);
void WaitUntilTxIsCompleted(void *const handle, const char *const ip, const char *const port);
void FreeSocket(void *const handle);
}

#endif
