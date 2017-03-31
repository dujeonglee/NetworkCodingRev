#ifndef TX_H
#define TX_H
#include "common.h"

namespace NetworkCoding
{
// Class declarations for transmission
class TransmissionBlock;
class TransmissionSession;
class Transmission;

class TransmissionBlock
{
public:
    TransmissionSession* const p_Session;
    const u08 m_BlockSize;
    const u08 m_TransmissionMode;
    const u16 m_BlockSequenceNumber;
    const u16 m_RetransmissionRedundancy;
    const u16 m_RetransmissionInterval;
    u16 m_LargestOriginalPacketSize;
    u08 m_TransmissionCount;

    u08 m_RemedyPacketBuffer[Parameter::MAXIMUM_BUFFER_SIZE];
    std::vector< std::unique_ptr< u08[] > > m_OriginalPacketBuffer;

    TransmissionBlock() = delete;
    TransmissionBlock(const TransmissionBlock&) = delete;
    TransmissionBlock(TransmissionSession&&) = delete;
    TransmissionBlock(TransmissionSession* p_session);
    ~TransmissionBlock();

    TransmissionBlock& operator=(const TransmissionBlock&) = delete;
    TransmissionBlock& operator=(TransmissionBlock&&) = delete;

    const u16 AckIndex();
    const bool IsAcked();
    bool Send(u08 *buffer, u16 buffersize, bool reqack);
    void Retransmission();
};

class TransmissionSession
{
public:
    const s32 c_Socket;
    const u32 c_IPv4;
    const u16 c_Port;
    std::atomic<bool> m_IsConnected;
    std::atomic<u16> m_MinBlockSequenceNumber;
    std::atomic<u16> m_MaxBlockSequenceNumber;
    std::atomic<bool> m_AckList[Parameter::MAXIMUM_NUMBER_OF_CONCURRENT_RETRANSMISSION*2];
    u16 m_RetransmissionRedundancy;
    u16 m_RetransmissionInterval;
    u08 m_TransmissionMode;
    u08 m_BlockSize;
    SingleShotTimer<1, 1> m_Timer;
    ThreadPool<2, 1> m_TaskQueue;
    TransmissionBlock* p_TransmissionBlock;
    std::atomic<unsigned long> m_ConcurrentRetransmissions;

    TransmissionSession(s32 Socket, u32 IPv4, u16 Port,
                        Parameter::TRANSMISSION_MODE TransmissionMode = Parameter::TRANSMISSION_MODE::RELIABLE_TRANSMISSION_MODE,
                        Parameter::BLOCK_SIZE BlockSize = Parameter::BLOCK_SIZE::BLOCK_SIZE_04,
                        u16 RetransmissionRedundancy = 0, u16 RetransmissionInterval = 10);
    TransmissionSession() = delete;
    TransmissionSession(const TransmissionSession&) = delete;
    TransmissionSession(TransmissionSession&&) = delete;
    ~TransmissionSession();
    TransmissionSession& operator=(const TransmissionSession&) = delete;
    TransmissionSession& operator=(TransmissionSession&&) = delete;

    void ChangeTransmissionMode(const Parameter::TRANSMISSION_MODE TransmissionMode);
    void ChangeBlockSize(const Parameter::BLOCK_SIZE BlockSize);
    void ChangeRetransmissionRedundancy(const u16 RetransmissionRedundancy);
    void ChangeRetransmissionInterval(const u16 RetransmissionInterval);
    void ChangeSessionParameter(const Parameter::TRANSMISSION_MODE TransmissionMode, const Parameter::BLOCK_SIZE BlockSize, const u16 RetransmissionRedundancy, const u16 RetransmissionInterval);
};

class Transmission
{
private:
    const s32 c_Socket;
    AVLTree< DataStructures::IPv4PortKey , TransmissionSession* > m_Sessions;
    std::mutex m_Lock;
public:
    Transmission(s32 Socket);
    ~Transmission();
public:
    bool Connect(u32 IPv4, u16 Port, u32 ConnectionTimeout, Parameter::TRANSMISSION_MODE TransmissionMode, Parameter::BLOCK_SIZE BlockSize, u16 RetransmissionRedundancy, u16 RetransmissionInterval);
    bool Send(u32 IPv4, u16 Port, u08* buffer, u16 buffersize, bool reqack);
    bool Disconnect(u32 IPv4, u16 Port);
public:
    void RxHandler(u08* buffer, u16 size, const sockaddr_in * const sender_addr, const u32 sender_addr_len);
};

}


#endif // TX_H
