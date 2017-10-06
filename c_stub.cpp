#include "c_stub.h"

#include "ncsocket.h"
using namespace NetworkCoding;

void *InitSocket(const char *const local_port, const long int RxTimeout, const long int TxTimeout, const rxcallback cb)
{
    return new NCSocket(std::string(local_port), RxTimeout, TxTimeout, cb);
}

bool Connect(void *const handle, const char *const ip, const char *const port, uint32_t timeout, uint8_t TransmissionMode, uint8_t BlockSize, uint16_t RetransmissionRedundancy)
{
    return ((NCSocket *)handle)->Connect(std::string(ip), std::string(port), timeout, (Parameter::TRANSMISSION_MODE)TransmissionMode, (Parameter::BLOCK_SIZE)BlockSize, RetransmissionRedundancy);
}

void Disconnect(void *const handle, const char *const ip, const char *const port)
{
    return ((NCSocket *)handle)->Disconnect(std::string(ip), std::string(port));
}

bool Send(void *const handle, const char *const ip, const char *const port, uint8_t *buff, uint16_t size)
{
    return ((NCSocket *)handle)->Send(std::string(ip), std::string(port), buff, size);
}

bool Flush(void *const handle, const char *const ip, const char *const port)
{
    return ((NCSocket *)handle)->Flush(std::string(ip), std::string(port));
}

void WaitUntilTxIsCompleted(void *const handle, const char *const ip, const char *const port)
{
    return ((NCSocket *)handle)->WaitUntilTxIsCompleted(std::string(ip), std::string(port));
}

void FreeSocket(void *handle)
{
    delete ((NCSocket *)handle);
}
