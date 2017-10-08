#include "api.h"

#include "ncsocket.h"
using namespace NetworkCoding;

void *InitSocket(const char *const local_port, const long int RxTimeout, const long int TxTimeout, const rxcallback cb)
{
    return new NCSocket(std::string(local_port), RxTimeout, TxTimeout, cb);
}

bool Connect(void *const handle, const char *const ip, const char *const port, const uint32_t timeout, const uint8_t TransmissionMode, const uint8_t BlockSize, const uint16_t RetransmissionRedundancy)
{
    if (TransmissionMode != Parameter::TRANSMISSION_MODE::RELIABLE_TRANSMISSION_MODE &&
        TransmissionMode != Parameter::TRANSMISSION_MODE::BEST_EFFORT_TRANSMISSION_MODE)
    {
        return false;
    }
    if (BlockSize != Parameter::BLOCK_SIZE::BLOCK_SIZE_04 &&
        BlockSize != Parameter::BLOCK_SIZE::BLOCK_SIZE_08 &&
        BlockSize != Parameter::BLOCK_SIZE::BLOCK_SIZE_12 &&
        BlockSize != Parameter::BLOCK_SIZE::BLOCK_SIZE_16 &&
        BlockSize != Parameter::BLOCK_SIZE::BLOCK_SIZE_20 &&
        BlockSize != Parameter::BLOCK_SIZE::BLOCK_SIZE_24 &&
        BlockSize != Parameter::BLOCK_SIZE::BLOCK_SIZE_28 &&
        BlockSize != Parameter::BLOCK_SIZE::BLOCK_SIZE_32 &&
        BlockSize != Parameter::BLOCK_SIZE::BLOCK_SIZE_64 &&
        BlockSize != Parameter::BLOCK_SIZE::BLOCK_SIZE_128)
    {
        return false;
    }
    return ((NCSocket *)handle)->Connect(std::string(ip), std::string(port), timeout, (Parameter::TRANSMISSION_MODE)TransmissionMode, (Parameter::BLOCK_SIZE)BlockSize, RetransmissionRedundancy);
}

void Disconnect(void *const handle, const char *const ip, const char *const port)
{
    return ((NCSocket *)handle)->Disconnect(std::string(ip), std::string(port));
}

bool Send(void *const handle, const char *const ip, const char *const port, uint8_t *const buff, const uint16_t size)
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
