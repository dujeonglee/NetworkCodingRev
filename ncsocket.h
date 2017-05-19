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
    NCSocket(const u16 PORT, const long int RXTIMEOUT, const long int TXTIMEOUT, const std::function <void (u08* buffer, u16 length, const sockaddr_in * const sender_addr, const u32 sender_addr_len)> rx)
    {
        m_State = INIT_FAILURE;
        m_Socket = -1;
        m_Rx = nullptr;
        m_Tx = nullptr;
        m_RxThread = nullptr;
        m_Socket = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
        if(m_Socket == -1)
        {
            return;
        }
        const int opt = 1;
        if(setsockopt(m_Socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1)
        {
            close(m_Socket);
            m_Socket = -1;
            return;
        }
        const timeval tx_to = {TXTIMEOUT/1000, (TXTIMEOUT%1000)*1000};
        if(setsockopt(m_Socket, SOL_SOCKET, SO_SNDTIMEO, &tx_to, sizeof(tx_to)) == -1)
        {
            close(m_Socket);
            m_Socket = -1;
            return;
        }
        const timeval rx_to = {RXTIMEOUT/1000, (RXTIMEOUT%1000)*1000};
        if(setsockopt(m_Socket, SOL_SOCKET, SO_RCVTIMEO, &rx_to, sizeof(rx_to)) == -1)
        {
            close(m_Socket);
            m_Socket = -1;
            return;
        }

        sockaddr_in BindAddress = {0};
        BindAddress.sin_family = AF_INET;
        BindAddress.sin_addr.s_addr = INADDR_ANY;
        BindAddress.sin_port = PORT;
        if(bind(m_Socket, (sockaddr*)&BindAddress, sizeof(BindAddress)) == -1)
        {
            close(m_Socket);
            m_Socket = -1;
            return;
        }

        try
        {
#if ENABLE_CRITICAL_EXCEPTIONS
            TEST_EXCEPTION(std::bad_alloc());
#endif
            m_Rx = new Reception(m_Socket, rx);
        }
        catch(const std::bad_alloc& ex)
        {
            EXCEPTION_PRINT;
            close(m_Socket);
            m_Rx = nullptr;
            m_Socket = -1;
            return;
        }
        try
        {
#if ENABLE_CRITICAL_EXCEPTIONS
            TEST_EXCEPTION(std::bad_alloc());
#endif
            m_Tx = new Transmission(m_Socket);
        }
        catch(const std::bad_alloc& ex)
        {
            EXCEPTION_PRINT;
            delete m_Rx;
            close(m_Socket);
            m_Tx = nullptr;
            m_Rx = nullptr;
            m_Socket = -1;
            return;
        }

        m_RxThreadIsRunning = true;
        try
        {
#if ENABLE_CRITICAL_EXCEPTIONS
            TEST_EXCEPTION(std::bad_alloc());
#endif
            NCSocket const * self = this;
            m_RxThread = new std::thread([self, RXTIMEOUT](){
                u08 rxbuffer[Parameter::MAXIMUM_BUFFER_SIZE];
                while(self->m_RxThreadIsRunning)
                {
                    sockaddr_in sender_addr = {0,};
                    socklen_t sender_addr_length = sizeof(sockaddr_in);
                    fd_set ReadFD;
                    FD_ZERO(&ReadFD);
                    FD_SET(self->m_Socket, &ReadFD);
                    const int MaxFD = self->m_Socket;
                    timeval rx_to = {RXTIMEOUT/1000, (RXTIMEOUT%1000)*1000};
                    const int state = select(MaxFD + 1 , &ReadFD, NULL, NULL, &rx_to);
                    if(state == 1 && FD_ISSET(self->m_Socket, &ReadFD))
                    {
                        const int ret = recvfrom(self->m_Socket, rxbuffer, sizeof(rxbuffer), 0, (sockaddr*)&sender_addr, &sender_addr_length);
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
                                self->m_Rx->RxHandler(rxbuffer, (u16)ret, &sender_addr, sender_addr_length);
                            break;
                            case Header::Common::HeaderType::DATA_ACK:
                            case Header::Common::HeaderType::SYNC_ACK:
                            case Header::Common::HeaderType::PONG:
                                self->m_Tx->RxHandler(rxbuffer, (u16)ret, &sender_addr, sender_addr_length);
                            break;

                        }
                    }
                }
            });
        }
        catch(const std::bad_alloc& ex)
        {
            EXCEPTION_PRINT;
            delete m_Tx;
            delete m_Rx;
            close(m_Socket);
            m_RxThread = nullptr;
            m_Tx = nullptr;
            m_Rx = nullptr;
            m_Socket = -1;
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
        if(m_Tx)
        {
            delete m_Tx;
        }
        if(m_Rx)
        {
            delete m_Rx;
        }
        if(m_Socket != -1)
        {
            close(m_Socket);
        }
    }

    NCSocket& operator=(const NCSocket&) = delete;
    NCSocket& operator=(NCSocket&&) = delete;
private:
    enum STATE : unsigned char{
        INIT_FAILURE,
        INIT_SUCCESS
    };
    STATE m_State;
    int m_Socket;
    Reception* m_Rx;
    Transmission* m_Tx;
    std::thread* m_RxThread;
    bool m_RxThreadIsRunning;
public:
    bool Connect(u32 ip, u16 port, u32 timeout, Parameter::TRANSMISSION_MODE TransmissionMode, Parameter::BLOCK_SIZE BlockSize, u16 RetransmissionRedundancy)
    {
        if(m_State == INIT_FAILURE)
        {
            return false;
        }
        return m_Tx->Connect(ip, port, timeout, TransmissionMode, BlockSize, RetransmissionRedundancy);
    }

    void Disconnect(u32 ip, u16 port)
    {
        if(m_State == INIT_FAILURE)
        {
            return;
        }
        m_Tx->Disconnect(ip, port);
    }

    bool Send(u32 ip, u16 port, u08* buff, u16 size/*, bool reqack*/)
    {
        if(m_State == INIT_FAILURE)
        {
            return false;
        }
        return m_Tx->Send(ip, port, buff, size/*, reqack*/);
    }

    bool Flush(u32 ip, u16 port)
    {
        if(m_State == INIT_FAILURE)
        {
            return false;
        }
        return m_Tx->Flush(ip, port);
    }

    void WaitUntilTxIsCompleted(u32 ip, u16 port)
    {
        if(m_State == INIT_FAILURE)
        {
            return;
        }
        m_Tx->WaitUntilTxIsCompleted(ip, port);
    }
};

}

#endif
