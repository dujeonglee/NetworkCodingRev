#include "c_stub.h"

#include "ncsocket.h"

void *InitSocket(const char* local_port, const long int RxTimeout, const long int TxTimeout, const rxcallback cb)
{
    return new NCSocket(std::string(local_port), RxTimeout, TxTimeout, cb);
}

bool Connect(void* const handle, const std::string ip, const std::string port, uint32_t timeout, Parameter::TRANSMISSION_MODE TransmissionMode, Parameter::BLOCK_SIZE BlockSize, uint16_t RetransmissionRedundancy)
{
    return ((NCSocket*)handle)->Connect(ip, port, timeout, TransmissionMode, BlockSize, RetransmissionRedundancy);
}

void Disconnect(void* const handle, const std::string ip, const std::string port)
{
    return ((NCSocket*)handle)->Disconnect(ip, port);    
}

bool Send(void* const handle, const std::string ip, const std::string port, uint8_t* buff, uint16_t size/*, bool reqack*/)
{
    return ((NCSocket*)handle)->Send(ip, port, buff, size);    
}

bool Flush(void* const handle, const std::string ip, const std::string port)
{
    return ((NCSocket*)handle)->Flush(ip, port);    
}

void WaitUntilTxIsCompleted(void* const handle, const std::string ip, const std::string port)
{
    return ((NCSocket*)handle)->WaitUntilTxIsCompleted(ip, port);    
}
