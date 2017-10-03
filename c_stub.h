#ifndef C_STUB_H_
#define C_STUB_H_

#include "ncsocket.h"
using namespace NetworkCoding;

typedef void(*rxcallback)(uint8_t* buffer, uint16_t length, const sockaddr* const sender_addr, const uint32_t sender_addr_len);

extern "C" {

void *InitSocket(const char* local_port, const long int RxTimeout, const long int TxTimeout, const rxcallback cb);
bool Connect(void* const handle, const std::string ip, const std::string port, uint32_t timeout, Parameter::TRANSMISSION_MODE TransmissionMode, Parameter::BLOCK_SIZE BlockSize, uint16_t RetransmissionRedundancy);
void Disconnect(void* const handle, const std::string ip, const std::string port);
bool Send(void* const handle, const std::string ip, const std::string port, uint8_t* buff, uint16_t size);
bool Flush(void* const handle, const std::string ip, const std::string port);
void WaitUntilTxIsCompleted(void* const handle, const std::string ip, const std::string port);

}

#endif
