#ifndef _NCSOCKET_H_
#define _NCSOCKET_H_

#include "tx.h"
#include "rx.h"
#include "SingleShotTimer.h"
#include <sys/select.h>

namespace NetworkCoding
{
class NCSocket
{
  public:
    NCSocket() = delete;
    NCSocket(const NCSocket &) = delete;
    NCSocket(NCSocket &&) = delete;
    NCSocket(const std::string PORT, const uint32_t RXTIMEOUT, const uint32_t TXTIMEOUT,
             const std::function<void(uint8_t *const buffer, const uint16_t length, const sockaddr *const sender_addr, const uint32_t sender_addr_len)> rx)
    {
        m_State = INIT_FAILURE;
        m_Socket[IPVERSION_4] = -1;
        m_Socket[IPVERSION_6] = -1;
        m_Rx[IPVERSION_4] = nullptr;
        m_Rx[IPVERSION_6] = nullptr;
        m_Tx[IPVERSION_4] = nullptr;
        m_Tx[IPVERSION_6] = nullptr;

        { // Open sockets for IPv4 and IPv6.
            addrinfo hints;
            addrinfo *ret = nullptr;

            memset(&hints, 0x00, sizeof(hints));
            hints.ai_flags = AI_PASSIVE;
            hints.ai_family = AF_UNSPEC;    // Accept IPv4 and IPv6
            hints.ai_socktype = SOCK_DGRAM; // UDP
            if (0 != getaddrinfo(nullptr, PORT.c_str(), &hints, &ret))
            {
                exit(-1);
            }
            for (addrinfo *iter = ret; iter != nullptr; iter = iter->ai_next)
            {
                if (iter->ai_family == AF_INET)
                {
                    m_Socket[IPVERSION_4] = socket(iter->ai_family, iter->ai_socktype, iter->ai_protocol);
                    if (m_Socket[IPVERSION_4] == -1)
                    {
                        return;
                    }
                    const int opt = 1;
                    if (setsockopt(m_Socket[IPVERSION_4], SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1)
                    {
                        close(m_Socket[IPVERSION_4]);
                        m_Socket[IPVERSION_4] = -1;
                        return;
                    }
                    const timeval tx_to = {TXTIMEOUT / 1000, (TXTIMEOUT % 1000) * 1000};
                    if (setsockopt(m_Socket[IPVERSION_4], SOL_SOCKET, SO_SNDTIMEO, &tx_to, sizeof(tx_to)) == -1)
                    {
                        close(m_Socket[IPVERSION_4]);
                        m_Socket[IPVERSION_4] = -1;
                        return;
                    }
                    if (bind(m_Socket[IPVERSION_4], (sockaddr *)iter->ai_addr, iter->ai_addrlen) == -1)
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
                    catch (const std::bad_alloc &ex)
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
                    catch (const std::bad_alloc &ex)
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
                else if (iter->ai_family == AF_INET6)
                {
                    m_Socket[IPVERSION_6] = socket(iter->ai_family, iter->ai_socktype, iter->ai_protocol);
                    if (m_Socket[IPVERSION_6] == -1)
                    {
                        return;
                    }
                    const int opt = 1;
                    if (setsockopt(m_Socket[IPVERSION_6], SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1)
                    {
                        close(m_Socket[IPVERSION_6]);
                        m_Socket[IPVERSION_6] = -1;
                        return;
                    }
                    const timeval tx_to = {TXTIMEOUT / 1000, (TXTIMEOUT % 1000) * 1000};
                    if (setsockopt(m_Socket[IPVERSION_6], SOL_SOCKET, SO_SNDTIMEO, &tx_to, sizeof(tx_to)) == -1)
                    {
                        close(m_Socket[IPVERSION_6]);
                        m_Socket[IPVERSION_6] = -1;
                        return;
                    }
                    setsockopt(m_Socket[IPVERSION_6], IPPROTO_IPV6, IPV6_V6ONLY, (char *)&opt, sizeof(opt));
                    if (bind(m_Socket[IPVERSION_6], (sockaddr *)iter->ai_addr, iter->ai_addrlen) == -1)
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
                    catch (const std::bad_alloc &ex)
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
                    catch (const std::bad_alloc &ex)
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
            if (ret)
            {
                freeaddrinfo(ret);
            }
        }
        NCSocket const *self = this;
        m_RxTask.PeriodicTask(0, [self, RXTIMEOUT]() {
            fd_set ReadFD;
            FD_ZERO(&ReadFD);
            if (self->m_Socket[IPVERSION_4] != -1)
            {
                FD_SET(self->m_Socket[IPVERSION_4], &ReadFD);
            }
            if (self->m_Socket[IPVERSION_6] != -1)
            {
                FD_SET(self->m_Socket[IPVERSION_6], &ReadFD);
            }
            const int MaxFD = (self->m_Socket[IPVERSION_4] > self->m_Socket[IPVERSION_6] ? self->m_Socket[IPVERSION_4] : self->m_Socket[IPVERSION_6]);
            timeval rx_to = {RXTIMEOUT / 1000, (RXTIMEOUT % 1000) * 1000};
            uint8_t rxbuffer[Parameter::MAXIMUM_BUFFER_SIZE];

            fd_set AllFD = ReadFD;
            const int state = select(MaxFD + 1, &AllFD, NULL, NULL, &rx_to);
            if (state && FD_ISSET(self->m_Socket[IPVERSION_4], &AllFD))
            {
                sockaddr_in sender_addr = {
                    0,
                };
                socklen_t sender_addr_length = sizeof(sockaddr_in);
                const int ret = recvfrom(self->m_Socket[IPVERSION_4], rxbuffer, sizeof(rxbuffer), 0, (sockaddr *)&sender_addr, &sender_addr_length);
                if (ret > 0)
                {
                    TEST_DROP;
                    switch (reinterpret_cast<Header::Common *>(rxbuffer)->m_Type)
                    {
                    case Header::Common::HeaderType::DATA:
                    case Header::Common::HeaderType::SYNC:
                    case Header::Common::HeaderType::PING:
                        self->m_Rx[IPVERSION_4]->RxHandler(rxbuffer, (uint16_t)ret, (sockaddr *)&sender_addr, sender_addr_length);
                        break;
                    case Header::Common::HeaderType::DATA_ACK:
                    case Header::Common::HeaderType::SYNC_ACK:
                    case Header::Common::HeaderType::PONG:
                        self->m_Tx[IPVERSION_4]->RxHandler(rxbuffer, (uint16_t)ret, (sockaddr *)&sender_addr, sender_addr_length);
                        break;
                    }
                }
            }
            if (state && FD_ISSET(self->m_Socket[IPVERSION_6], &AllFD))
            {
                sockaddr_in6 sender_addr = {
                    0,
                };
                socklen_t sender_addr_length = sizeof(sockaddr_in6);
                const int ret = recvfrom(self->m_Socket[IPVERSION_6], rxbuffer, sizeof(rxbuffer), 0, (sockaddr *)&sender_addr, &sender_addr_length);
                if (ret > 0)
                {
                    TEST_DROP;
                    switch (reinterpret_cast<Header::Common *>(rxbuffer)->m_Type)
                    {
                    case Header::Common::HeaderType::DATA:
                    case Header::Common::HeaderType::SYNC:
                    case Header::Common::HeaderType::PING:
                        self->m_Rx[IPVERSION_6]->RxHandler(rxbuffer, (uint16_t)ret, (sockaddr *)&sender_addr, sender_addr_length);
                        break;
                    case Header::Common::HeaderType::DATA_ACK:
                    case Header::Common::HeaderType::SYNC_ACK:
                    case Header::Common::HeaderType::PONG:
                        self->m_Tx[IPVERSION_6]->RxHandler(rxbuffer, (uint16_t)ret, (sockaddr *)&sender_addr, sender_addr_length);
                        break;
                    }
                }
            }
            return true;
        });
        m_State = INIT_SUCCESS;
    }

    ~NCSocket()
    {
        m_RxTask.Stop();
        if (m_Tx[IPVERSION_4])
        {
            delete m_Tx[IPVERSION_4];
            m_Tx[IPVERSION_4] = nullptr;
        }
        if (m_Tx[IPVERSION_6])
        {
            delete m_Tx[IPVERSION_6];
            m_Tx[IPVERSION_6] = nullptr;
        }
        if (m_Rx[IPVERSION_4])
        {
            delete m_Rx[IPVERSION_4];
            m_Rx[IPVERSION_4] = nullptr;
        }
        if (m_Rx[IPVERSION_6])
        {
            delete m_Rx[IPVERSION_6];
            m_Rx[IPVERSION_6] = nullptr;
        }
        if (m_Socket[IPVERSION_4] != -1)
        {
            close(m_Socket[IPVERSION_4]);
            m_Socket[IPVERSION_4] = -1;
        }
        if (m_Socket[IPVERSION_6] != -1)
        {
            close(m_Socket[IPVERSION_6]);
            m_Socket[IPVERSION_6] = -1;
        }
    }

    NCSocket &operator=(const NCSocket &) = delete;
    NCSocket &operator=(NCSocket &&) = delete;

  private:
    enum STATE : unsigned char
    {
        INIT_FAILURE,
        INIT_SUCCESS
    };
    enum IPVERSION : unsigned char
    {
        IPVERSION_4 = 0,
        IPVERSION_6,
        IPVERSIONS
    };
    STATE m_State;
    int m_Socket[IPVERSIONS];
    Reception *m_Rx[IPVERSIONS];
    Transmission *m_Tx[IPVERSIONS];
    SingleShotTimer<1, 1> m_RxTask;

  public:
    bool Connect(const std::string ip, const std::string port, const uint32_t timeout, const Parameter::TRANSMISSION_MODE TransmissionMode, const Parameter::BLOCK_SIZE BlockSize, const uint16_t RetransmissionRedundancy)
    {
        if (m_State == INIT_FAILURE)
        {
            return false;
        }
        const DataTypes::Address Addr = DataTypes::GetAddress(ip, port);
        if (Addr.AddrLength == sizeof(sockaddr_in) && m_Tx[IPVERSION_4] != nullptr)
        {
            return m_Tx[IPVERSION_4]->Connect(Addr, timeout, TransmissionMode, BlockSize, RetransmissionRedundancy);
        }
        else if (Addr.AddrLength == sizeof(sockaddr_in6) && m_Tx[IPVERSION_6] != nullptr)
        {
            return m_Tx[IPVERSION_6]->Connect(Addr, timeout, TransmissionMode, BlockSize, RetransmissionRedundancy);
        }
        return false;
    }

    void Disconnect(const std::string ip, const std::string port)
    {
        if (m_State == INIT_FAILURE)
        {
            return;
        }
        const DataTypes::Address Addr = DataTypes::GetAddress(ip, port);
        if (Addr.AddrLength == sizeof(sockaddr_in) && m_Tx[IPVERSION_4] != nullptr)
        {
            m_Tx[IPVERSION_4]->Disconnect(Addr);
        }
        else if (Addr.AddrLength == sizeof(sockaddr_in6) && m_Tx[IPVERSION_6] != nullptr)
        {
            m_Tx[IPVERSION_6]->Disconnect(Addr);
        }
    }

    bool Send(const std::string ip, const std::string port, uint8_t *const buff, const uint16_t size)
    {
        if (m_State == INIT_FAILURE)
        {
            return false;
        }
        const DataTypes::Address Addr = DataTypes::GetAddress(ip, port);
        if (Addr.AddrLength == sizeof(sockaddr_in) && m_Tx[IPVERSION_4] != nullptr)
        {
            return m_Tx[IPVERSION_4]->Send(Addr, buff, size);
        }
        else if (Addr.AddrLength == sizeof(sockaddr_in6) && m_Tx[IPVERSION_6] != nullptr)
        {
            return m_Tx[IPVERSION_6]->Send(Addr, buff, size);
        }
        return false;
    }

    bool Flush(const std::string ip, const std::string port)
    {
        if (m_State == INIT_FAILURE)
        {
            return false;
        }
        const DataTypes::Address Addr = DataTypes::GetAddress(ip, port);
        if (Addr.AddrLength == sizeof(sockaddr_in) && m_Tx[IPVERSION_4] != nullptr)
        {
            return m_Tx[IPVERSION_4]->Flush(Addr);
        }
        else if (Addr.AddrLength == sizeof(sockaddr_in6) && m_Tx[IPVERSION_6] != nullptr)
        {
            return m_Tx[IPVERSION_6]->Flush(Addr);
        }
        return false;
    }

    void WaitUntilTxIsCompleted(const std::string ip, const std::string port)
    {
        if (m_State == INIT_FAILURE)
        {
            return;
        }
        const DataTypes::Address Addr = DataTypes::GetAddress(ip, port);
        if (Addr.AddrLength == sizeof(sockaddr_in) && m_Tx[IPVERSION_4] != nullptr)
        {
            m_Tx[IPVERSION_4]->Flush(Addr);
            m_Tx[IPVERSION_4]->WaitUntilTxIsCompleted(Addr);
        }
        else if (Addr.AddrLength == sizeof(sockaddr_in6) && m_Tx[IPVERSION_6] != nullptr)
        {
            m_Tx[IPVERSION_6]->Flush(Addr);
            m_Tx[IPVERSION_6]->WaitUntilTxIsCompleted(Addr);
        }
    }

    bool Receive(uint8_t *const buffer, uint16_t *const length, sockaddr *const sender_addr, uint32_t *const sender_addr_len, uint32_t timeout)
    {
        if (m_State == INIT_FAILURE)
        {
            return false;
        }
        if (m_Rx[IPVERSION_4])
        {
            if (true == m_Rx[IPVERSION_4]->Receive(buffer, length, sender_addr, sender_addr_len, timeout))
            {
                return true;
            }
        }
        if (m_Rx[IPVERSION_6])
        {
            if (true == m_Rx[IPVERSION_6]->Receive(buffer, length, sender_addr, sender_addr_len, timeout))
            {
                return true;
            }
        }
        return false;
    }
};
}

#endif
