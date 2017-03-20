#ifndef _NCSOCKET_H_
#define _NCSOCKET_H_

#include "tx.h"

namespace NetworkCoding
{
template <u16 PORT, u32 RXTIMEOUT, u32 TXTIMEOUT>
class NCSocket{
public:
    NCSocket() = delete;
    NCSocket(const NCSocket&) = delete;
    NCSocket(NCSocket&&) = delete;
    NCSocket(std::function <void (unsigned char* buffer, unsigned int length, sockaddr_in addr)> rx)
    {
        m_State = INIT_FAILURE;
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
        const timeval tx_to = {TXTIMEOUT/1000, TXTIMEOUT%1000};
        if(setsockopt(m_Socket, SOL_SOCKET, SO_SNDTIMEO, &tx_to, sizeof(tx_to)) == -1)
        {
            close(m_Socket);
            m_Socket = -1;
            return;
        }
        const timeval rx_to = {RXTIMEOUT/1000, RXTIMEOUT%1000};
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
            m_Tx = new Transmission(m_Socket);
        }
        catch(const std::bad_alloc& ex)
        {
            std::cout<<ex.what()<<std::endl;
            close(m_Socket);
            m_Socket = -1;
            return;
        }

        m_RxThreadIsRunning = true;
        try
        {
            m_RxThread = new std::thread([this](){
                u08 rxbuffer[Parameter::MAXIMUM_BUFFER_SIZE];
                while(m_RxThreadIsRunning)
                {
                    sockaddr_in sender_addr = {0,};
                    int sender_addr_length = sizeof(sockaddr_in);
                    const int ret = recvfrom(m_Socket, rxbuffer, sizeof(rxbuffer), 0, (sockaddr*)&sender_addr, &sender_addr_length);
                    if(ret <= 0)
                    {
                        continue;
                    }
                    m_Tx->RxHandler(rxbuffer, (u16)ret, &sender_addr, sender_addr_length);
                }
            });
        }
        catch(const std::bad_alloc& ex)
        {
            std::cout<<ex.what()<<std::endl;
            close(m_Socket);
            m_Socket = -1;
            delete m_Tx;
            return;
        }
        m_RxCallback = rx;
        m_State = INIT_SUCCESS;
    }

    ~NCSocket()
    {
        m_RxThreadIsRunning = false;
        m_RxThread->join();
        delete m_Tx;
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
    Transmission* m_Tx;
    std::thread* m_RxThread;
    bool m_RxThreadIsRunning;
    std::function <void (u08* buffer, u16 length, sockaddr_in addr)> m_RxCallback;
public:
    bool OpenSession(unsigned int ip, unsigned short int port, Parameter::TRANSMISSION_MODE TransmissionMode, Parameter::BLOCK_SIZE BlockSize, u16 RetransmissionRedundancy, u16 RetransmissionInterval)
    {
        if(m_State == INIT_FAILURE)
        {
            return false;
        }
        return m_Tx->Connect(ip, port, TransmissionMode, BlockSize, RetransmissionRedundancy, RetransmissionInterval);
    }

    void CloseSession(unsigned int ip, unsigned short int port)
    {
        if(m_State == INIT_FAILURE)
        {
            return;
        }
    }

    bool Send(u32 ip, u16 port, u08* buff, u16 size, bool reqack)
    {
        if(m_State == INIT_FAILURE)
        {
            return false;
        }
        return m_Tx->Send(ip, port, buff, size, reqack);
    }
};

}

#endif
