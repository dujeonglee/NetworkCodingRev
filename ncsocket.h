#ifndef _NCSOCKET_H_
#define _NCSOCKET_H_

#include "tx.h"
#include "rx.h"
#include <sys/select.h>

namespace NetworkCoding
{
class NCSocket{
public:
    NCSocket() = delete;
    NCSocket(const NCSocket&) = delete;
    NCSocket(NCSocket&&) = delete;
    NCSocket(const uint16_t PORT, const long int RXTIMEOUT, const long int TXTIMEOUT, const std::function <void (uint8_t* buffer, uint16_t length, const sockaddr* const sender_addr, const uint32_t sender_addr_len)> rx)
    {
        m_State = INIT_FAILURE;
        m_Socket[IPVERSION_4] = -1;
        m_Socket[IPVERSION_6] = -1;
        m_Rx[IPVERSION_4] = nullptr;
        m_Rx[IPVERSION_6] = nullptr;
        m_Tx[IPVERSION_4] = nullptr;
        m_Tx[IPVERSION_6] = nullptr;
        m_RxThread = nullptr;

        {
            addrinfo hints;
            addrinfo *ret = nullptr;

            m_Socket[IPVERSION_4] = -1;
            m_Socket[IPVERSION_6] = -1;
            
            memset(&hints, 0x00, sizeof(hints));
            hints.ai_flags = AI_PASSIVE;
            hints.ai_family = AF_UNSPEC;    // Accept IPv4 and IPv6 
            hints.ai_socktype = SOCK_DGRAM; // UDP
            if(0 != getaddrinfo(nullptr, std::to_string(PORT).c_str(), &hints, &ret)) 
            { 
                exit(-1); 
            }
            for(addrinfo *iter = ret; iter != nullptr; iter = iter->ai_next) 
            {
                if(iter->ai_family == AF_INET)
                {
                    m_Socket[IPVERSION_4] = socket(iter->ai_family, iter->ai_socktype, iter->ai_protocol);
                    if(m_Socket[IPVERSION_4] == -1)
                    {
                        return;
                    }
                    const int opt = 1;
                    if(setsockopt(m_Socket[IPVERSION_4], SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1)
                    {
                        close(m_Socket[IPVERSION_4]);
                        m_Socket[IPVERSION_4] = -1;
                        return;
                    }
                    const timeval tx_to = {TXTIMEOUT/1000, (TXTIMEOUT%1000)*1000};
                    if(setsockopt(m_Socket[IPVERSION_4], SOL_SOCKET, SO_SNDTIMEO, &tx_to, sizeof(tx_to)) == -1)
                    {
                        close(m_Socket[IPVERSION_4]);
                        m_Socket[IPVERSION_4] = -1;
                        return;
                    }
                    const timeval rx_to = {RXTIMEOUT/1000, (RXTIMEOUT%1000)*1000};
                    if(setsockopt(m_Socket[IPVERSION_4], SOL_SOCKET, SO_RCVTIMEO, &rx_to, sizeof(rx_to)) == -1)
                    {
                        close(m_Socket[IPVERSION_4]);
                        m_Socket[IPVERSION_4] = -1;
                        return;
                    }

                    if(bind(m_Socket[IPVERSION_4], (sockaddr*)iter->ai_addr, iter->ai_addrlen) == -1)
                    {
                        close(m_Socket[IPVERSION_4]);
                        m_Socket[IPVERSION_4] = -1;
                        return;
                    }
                    try
                    {
#if ENABLE_CRITICAL_EXCEPTIONS
                        TEST_EXCEPTION(std::bad_alloc());
#endif
                        m_Rx[IPVERSION_4] = new Reception(m_Socket[IPVERSION_4], rx);
                    }
                    catch(const std::bad_alloc& ex)
                    {
                        EXCEPTION_PRINT;
                        close(m_Socket[IPVERSION_4]);
                        m_Socket[IPVERSION_4] = -1;
                        m_Rx[IPVERSION_4] = nullptr;
                        return;
                    }
                    try
                    {
#if ENABLE_CRITICAL_EXCEPTIONS
                        TEST_EXCEPTION(std::bad_alloc());
#endif
                        m_Tx[IPVERSION_4] = new Transmission(m_Socket[IPVERSION_4]);
                    }
                    catch(const std::bad_alloc& ex)
                    {
                        EXCEPTION_PRINT;
                        delete m_Rx[IPVERSION_4];
                        close(m_Socket[IPVERSION_4]);
                        m_Socket[IPVERSION_4] = -1;
                        m_Tx[IPVERSION_4] = nullptr;
                        m_Rx[IPVERSION_4] = nullptr;
                        return;
                    }
                }
                else if(iter->ai_family == AF_INET6)
                {
                    m_Socket[IPVERSION_6] = socket(iter->ai_family, iter->ai_socktype, iter->ai_protocol);
                    if(m_Socket[IPVERSION_6] == -1)
                    {
                        return;
                    }
                    const int opt = 1;
                    if(setsockopt(m_Socket[IPVERSION_6], SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1)
                    {
                        close(m_Socket[IPVERSION_6]);
                        m_Socket[IPVERSION_6] = -1;
                        return;
                    }
                    const timeval tx_to = {TXTIMEOUT/1000, (TXTIMEOUT%1000)*1000};
                    if(setsockopt(m_Socket[IPVERSION_6], SOL_SOCKET, SO_SNDTIMEO, &tx_to, sizeof(tx_to)) == -1)
                    {
                        close(m_Socket[IPVERSION_6]);
                        m_Socket[IPVERSION_6] = -1;
                        return;
                    }
                    const timeval rx_to = {RXTIMEOUT/1000, (RXTIMEOUT%1000)*1000};
                    if(setsockopt(m_Socket[IPVERSION_6], SOL_SOCKET, SO_RCVTIMEO, &rx_to, sizeof(rx_to)) == -1)
                    {
                        close(m_Socket[IPVERSION_6]);
                        m_Socket[IPVERSION_6] = -1;
                        return;
                    }
                    setsockopt(m_Socket[IPVERSION_6], IPPROTO_IPV6, IPV6_V6ONLY, (char *)&opt, sizeof(opt)); 
                    if(bind(m_Socket[IPVERSION_6], (sockaddr*)iter->ai_addr, iter->ai_addrlen) == -1)
                    {
                        close(m_Socket[IPVERSION_6]);
                        m_Socket[IPVERSION_6] = -1;
                        return;
                    }
                    try
                    {
#if ENABLE_CRITICAL_EXCEPTIONS
                        TEST_EXCEPTION(std::bad_alloc());
#endif
                        m_Rx[IPVERSION_6] = new Reception(m_Socket[IPVERSION_6], rx);
                    }
                    catch(const std::bad_alloc& ex)
                    {
                        EXCEPTION_PRINT;
                        close(m_Socket[IPVERSION_6]);
                        m_Socket[IPVERSION_6] = -1;
                        m_Rx[IPVERSION_6] = nullptr;
                        return;
                    }
                    try
                    {
#if ENABLE_CRITICAL_EXCEPTIONS
                        TEST_EXCEPTION(std::bad_alloc());
#endif
                        m_Tx[IPVERSION_6] = new Transmission(m_Socket[IPVERSION_6]);
                    }
                    catch(const std::bad_alloc& ex)
                    {
                        EXCEPTION_PRINT;
                        delete m_Rx[IPVERSION_6];
                        close(m_Socket[IPVERSION_6]);
                        m_Socket[IPVERSION_6] = -1;
                        m_Tx[IPVERSION_6] = nullptr;
                        m_Rx[IPVERSION_6] = nullptr;
                        return;
                    }
                }
            }
            if(ret)
            {
                freeaddrinfo(ret);
            }
        }
        m_RxThreadIsRunning = true;
        try
        {
#if ENABLE_CRITICAL_EXCEPTIONS
            TEST_EXCEPTION(std::bad_alloc());
#endif
            NCSocket const * self = this;
            m_RxThread = new std::thread([self, RXTIMEOUT](){
                fd_set ReadFD;
                FD_ZERO(&ReadFD);
                if(self->m_Socket[IPVERSION_4] != -1)
                {
                    FD_SET(self->m_Socket[IPVERSION_4], &ReadFD);
                }
                if(self->m_Socket[IPVERSION_6] != -1)
                {
                    FD_SET(self->m_Socket[IPVERSION_6], &ReadFD);
                }
                const int MaxFD = (self->m_Socket[IPVERSION_4] > self->m_Socket[IPVERSION_6]?self->m_Socket[IPVERSION_4]:self->m_Socket[IPVERSION_6]);
                timeval rx_to = {RXTIMEOUT/1000, (RXTIMEOUT%1000)*1000};
                uint8_t rxbuffer[Parameter::MAXIMUM_BUFFER_SIZE];
                while(self->m_RxThreadIsRunning)
                {
                    fd_set AllFD = ReadFD;
                    const int state = select(MaxFD + 1 , &AllFD, NULL, NULL, &rx_to);
                    if(state && FD_ISSET(self->m_Socket[IPVERSION_4], &AllFD))
                    {
                        sockaddr_in sender_addr = {0,};
                        socklen_t sender_addr_length = sizeof(sockaddr_in);
                        const int ret = recvfrom(self->m_Socket[IPVERSION_4], rxbuffer, sizeof(rxbuffer), 0, (sockaddr*)&sender_addr, &sender_addr_length);
                        if(ret <= 0)
                        {
                            continue;
                        }
                        TEST_DROP;
                        switch(reinterpret_cast<Header::Common*>(rxbuffer)->m_Type)
                        {
                            case Header::Common::HeaderType::DATA:
                            case Header::Common::HeaderType::SYNC:
                            case Header::Common::HeaderType::PING:
                                self->m_Rx[IPVERSION_4]->RxHandler(rxbuffer, (uint16_t)ret, (sockaddr*)&sender_addr, sender_addr_length);
                            break;
                            case Header::Common::HeaderType::DATA_ACK:
                            case Header::Common::HeaderType::SYNC_ACK:
                            case Header::Common::HeaderType::PONG:
                                self->m_Tx[IPVERSION_4]->RxHandler(rxbuffer, (uint16_t)ret, (sockaddr*)&sender_addr, sender_addr_length);
                            break;

                        }
                    }
                    if(state && FD_ISSET(self->m_Socket[IPVERSION_6], &AllFD))
                    {
                        sockaddr_in6 sender_addr = {0,};
                        socklen_t sender_addr_length = sizeof(sockaddr_in6);
                        const int ret = recvfrom(self->m_Socket[IPVERSION_6], rxbuffer, sizeof(rxbuffer), 0, (sockaddr*)&sender_addr, &sender_addr_length);
                        if(ret <= 0)
                        {
                            continue;
                        }
                        TEST_DROP;
                        switch(reinterpret_cast<Header::Common*>(rxbuffer)->m_Type)
                        {
                            case Header::Common::HeaderType::DATA:
                            case Header::Common::HeaderType::SYNC:
                            case Header::Common::HeaderType::PING:
                                self->m_Rx[IPVERSION_6]->RxHandler(rxbuffer, (uint16_t)ret, (sockaddr*)&sender_addr, sender_addr_length);
                            break;
                            case Header::Common::HeaderType::DATA_ACK:
                            case Header::Common::HeaderType::SYNC_ACK:
                            case Header::Common::HeaderType::PONG:
                                self->m_Tx[IPVERSION_6]->RxHandler(rxbuffer, (uint16_t)ret, (sockaddr*)&sender_addr, sender_addr_length);
                            break;

                        }
                    }
                }
            });
        }
        catch(const std::bad_alloc& ex)
        {
            EXCEPTION_PRINT;
            if(m_Tx[IPVERSION_4])
            {
                delete m_Tx[IPVERSION_4];
                m_Tx[IPVERSION_4] = nullptr;
            }
            if(m_Tx[IPVERSION_6])
            {
                delete m_Tx[IPVERSION_6];
                m_Tx[IPVERSION_6] = nullptr;
            }
            if(m_Rx[IPVERSION_4])
            {
                delete m_Rx[IPVERSION_4];
                m_Rx[IPVERSION_4] = nullptr;
            }
            if(m_Rx[IPVERSION_6])
            {
                delete m_Rx[IPVERSION_6];
                m_Rx[IPVERSION_6] = nullptr;
            }
            if(m_Socket[IPVERSION_4] != -1)
            {
                close(m_Socket[IPVERSION_4]);
                m_Socket[IPVERSION_4] = -1;

            }
            if(m_Socket[IPVERSION_6] != -1)
            {
                close(m_Socket[IPVERSION_6]);
                m_Socket[IPVERSION_6] = -1;
            }
            m_RxThread = nullptr;
            return;
        }
        m_State = INIT_SUCCESS;
    }

    ~NCSocket()
    {
        m_RxThreadIsRunning = false;
        if(m_RxThread)
        {
            if(m_RxThread->joinable())
            {
                m_RxThread->join();
            }
            delete m_RxThread;
        }
        if(m_Tx[IPVERSION_4])
        {
            delete m_Tx[IPVERSION_4];
        }
        if(m_Tx[IPVERSION_6])
        {
            delete m_Tx[IPVERSION_6];
        }
        if(m_Rx[IPVERSION_4])
        {
            delete m_Rx[IPVERSION_4];
        }
        if(m_Rx[IPVERSION_6])
        {
            delete m_Rx[IPVERSION_6];
        }
        if(m_Socket[IPVERSION_4] != -1)
        {
            close(m_Socket[IPVERSION_4]);
            m_Socket[IPVERSION_4] = -1;
        }
        if(m_Socket[IPVERSION_6] != -1)
        {
            close(m_Socket[IPVERSION_6]);
            m_Socket[IPVERSION_6] = -1;
        }
    }

    NCSocket& operator=(const NCSocket&) = delete;
    NCSocket& operator=(NCSocket&&) = delete;
private:
    enum STATE : unsigned char{
        INIT_FAILURE,
        INIT_SUCCESS
    };
    enum IPVERSION: unsigned char{
        IPVERSION_4 = 0,
        IPVERSION_6
    };
    STATE m_State;
    int m_Socket[2];
    Reception* m_Rx[2];
    Transmission* m_Tx[2];
    std::thread* m_RxThread;
    bool m_RxThreadIsRunning;
public:
    bool Connect(const std::string ip, const std::string port, uint32_t timeout, Parameter::TRANSMISSION_MODE TransmissionMode, Parameter::BLOCK_SIZE BlockSize, uint16_t RetransmissionRedundancy)
    {
        if(m_State == INIT_FAILURE)
        {
            return false;
        }
        const DataStructures::AddressType Addr = DataStructures::GetAddressType(ip, port);
        if(Addr.AddrLength == sizeof(sockaddr_in))
        {
            return m_Tx[IPVERSION_4]->Connect(Addr, timeout, TransmissionMode, BlockSize, RetransmissionRedundancy);
        }
        else if(Addr.AddrLength == sizeof(sockaddr_in6))
        {
            return m_Tx[IPVERSION_6]->Connect(Addr, timeout, TransmissionMode, BlockSize, RetransmissionRedundancy);
        }
        return false;
    }

    void Disconnect(const std::string ip, const std::string port)
    {
        if(m_State == INIT_FAILURE)
        {
            return;
        }
        const DataStructures::AddressType Addr = DataStructures::GetAddressType(ip, port);
        if(Addr.AddrLength == sizeof(sockaddr_in))
        {
            m_Tx[IPVERSION_4]->Disconnect(Addr);
        }
        else
        {
            m_Tx[IPVERSION_6]->Disconnect(Addr);
        }
    }

    bool Send(const std::string ip, const std::string port, uint8_t* buff, uint16_t size/*, bool reqack*/)
    {
        if(m_State == INIT_FAILURE)
        {
            return false;
        }
        const DataStructures::AddressType Addr = DataStructures::GetAddressType(ip, port);
        if(Addr.AddrLength == sizeof(sockaddr_in))
        {
            return m_Tx[IPVERSION_4]->Send(DataStructures::GetAddressType(ip, port), buff, size/*, reqack*/);
        }
        else
        {
            return m_Tx[IPVERSION_6]->Send(DataStructures::GetAddressType(ip, port), buff, size/*, reqack*/);
        }
    }

    bool Flush(const std::string ip, const std::string port)
    {
        if(m_State == INIT_FAILURE)
        {
            return false;
        }
        const DataStructures::AddressType Addr = DataStructures::GetAddressType(ip, port);
        if(Addr.AddrLength == sizeof(sockaddr_in))
        {
            return m_Tx[IPVERSION_4]->Flush(DataStructures::GetAddressType(ip, port));
        }
        else
        {
            return m_Tx[IPVERSION_6]->Flush(DataStructures::GetAddressType(ip, port));
        }
    }

    void WaitUntilTxIsCompleted(const std::string ip, const std::string port)
    {
        if(m_State == INIT_FAILURE)
        {
            return;
        }
        const DataStructures::AddressType Addr = DataStructures::GetAddressType(ip, port);
        if(Addr.AddrLength == sizeof(sockaddr_in))
        {
            m_Tx[IPVERSION_4]->Flush(DataStructures::GetAddressType(ip, port));
            m_Tx[IPVERSION_4]->WaitUntilTxIsCompleted(DataStructures::GetAddressType(ip, port));
        }
        else
        {
            m_Tx[IPVERSION_6]->Flush(DataStructures::GetAddressType(ip, port));
            m_Tx[IPVERSION_6]->WaitUntilTxIsCompleted(DataStructures::GetAddressType(ip, port));
        }
    }
};

}

#endif
