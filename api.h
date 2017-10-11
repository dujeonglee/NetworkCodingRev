#ifndef API_H_
#define API_H_

#include <stdint.h>

typedef void (*rxcallback)(uint8_t *const buffer, const uint16_t length, const void *const address, const uint32_t sender_addr_len);

typedef void *(*InitSocketFuncType)(const char *local_port, const uint32_t RxTimeout, const uint32_t TxTimeout, const rxcallback cb);
typedef bool (*ConnectFuncType)(void *const handle, const char *const ip, const char *const port, const uint32_t timeout, const uint8_t TransmissionMode, const uint8_t BlockSize, const uint16_t RetransmissionRedundancy);
typedef void (*DisconnectFuncType)(void *const handle, const char *const ip, const char *const port);
typedef bool (*SendFuncType)(void *const handle, const char *const ip, const char *const port, uint8_t *const buff, const uint16_t size);
typedef bool (*FlushFuncType)(void *const handle, const char *const ip, const char *const port);
typedef void (*WaitUntilTxIsCompletedFuncType)(void *const handle, const char *const ip, const char *const port);
typedef void (*FreeSocketFuncType)(void *const handle);

extern "C" {

void *InitSocket(const char *const local_port, const uint32_t RxTimeout, const uint32_t TxTimeout, const rxcallback cb);
bool Connect(void *const handle, const char *const ip, const char *const port, const uint32_t timeout, const uint8_t TransmissionMode, const uint8_t BlockSize, const uint16_t RetransmissionRedundancy);
void Disconnect(void *const handle, const char *const ip, const char *const port);
bool Send(void *const handle, const char *const ip, const char *const port, uint8_t *const buff, const uint16_t size);
bool Flush(void *const handle, const char *const ip, const char *const port);
void WaitUntilTxIsCompleted(void *const handle, const char *const ip, const char *const port);
void FreeSocket(void *const handle);
}

#endif
