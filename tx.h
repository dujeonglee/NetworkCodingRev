#ifndef TX_H
#define TX_H
#include "common.h"

namespace NetworkCoding
{
// Class declarations for transmission
class TransmissionBlock;
class TransmissionSession;
class Transmission;

/**
 * @brief The TransmissionBlock class
 * Maintain
 */
class TransmissionBlock
{
public:
    TransmissionSession* const p_Session;
    u08 m_BlockSize;
    u08 m_TransmissionMode;
    u16 m_BlockSequenceNumber;
    u16 m_RetransmissionRedundancy;
    u16 m_RetransmissionInterval;
    u16 m_LargestOriginalPacketSize;
    u08 m_TransmissionCount;
    std::mutex m_Lock;

    Others::PacketBuffer m_RemedyPacketBuffer;
    std::vector< Others::PacketBuffer > m_OriginalPacketBuffer;

    TransmissionBlock(TransmissionSession* const);
    TransmissionBlock() = delete;
    TransmissionBlock(const TransmissionBlock&) = delete;
    TransmissionBlock(TransmissionSession&&) = delete;
    ~TransmissionBlock() = default;

    TransmissionBlock& operator=(const TransmissionBlock&) = delete;
    TransmissionBlock& operator=(TransmissionBlock&&) = delete;

    void Init();
    void Deinit();
    u16 Send(u32 IPv4, u16 Port, u08* buffer, u16 buffersize, bool reqack);
    void Retransmission();
};

class TransmissionSession
{
public:
    const s32 c_Socket;
    const u32 c_IPv4;
    const u16 c_Port;
    volatile bool m_IsConnected;
    std::atomic<u16> m_MinBlockSequenceNumber;
    std::atomic<u16> m_MaxBlockSequenceNumber;
    volatile std::atomic<bool> m_AckList[Parameter::MAX_CONCURRENCY*2];
    u08 m_Concurrency;
    u16 m_RetransmissionRedundancy;
    u16 m_RetransmissionInterval;
    enum TRANSMISSION_MODE: u08
    {
        RELIABLE_TRANSMISSION_MODE = 0,
        BEST_EFFORT_TRANSMISSION_MODE
    };
    u08 m_TransmissionMode;
    enum BLOCK_SIZE: u08
    {
        INVALID_BLOCK_SIZE = 0,
        BLOCK_SIZE_02 = 2,
        BLOCK_SIZE_04 = 4,
        BLOCK_SIZE_08 = 8,
        BLOCK_SIZE_16 = 16,
        BLOCK_SIZE_32 = 32,
        BLOCK_SIZE_64 = 64
    };
    u08 m_BlockSize;
    ThreadPool m_RetransmissionThreadPool;
    std::vector< TransmissionBlock* > m_TransmissionBlockPool;
    std::atomic< TransmissionBlock* > m_CurrentTransmissionBlock;

    std::mutex m_Lock;
    std::condition_variable m_Condition;

    TransmissionSession(s32 Socket, u32 IPv4, u16 Port, u08 Concurrency, TRANSMISSION_MODE TransmissionMode, BLOCK_SIZE BlockSize, u16 RetransmissionRedundancy, u16 RetransmissionInterval);
    TransmissionSession() = delete;
    TransmissionSession(const TransmissionSession&) = delete;
    TransmissionSession(TransmissionSession&&) = delete;
    ~TransmissionSession();
    TransmissionSession& operator=(const TransmissionSession&) = delete;
    TransmissionSession& operator=(TransmissionSession&&) = delete;

    void ChangeConcurrency(const u08 Concurrency);
    void ChangeTransmissionMode(const TRANSMISSION_MODE TransmissionMode);
    void ChangeBlockSize(const BLOCK_SIZE BlockSize);
    void ChangeRetransmissionRedundancy(const u16 RetransmissionRedundancy);
    void ChangeRetransmissionInterval(const u16 RetransmissionInterval);
    void ChangeSessionParameter(const u08 Concurrency, const TRANSMISSION_MODE TransmissionMode, const BLOCK_SIZE BlockSize, const u16 RetransmissionRedundancy, const u16 RetransmissionInterval);
};

class Transmission
{
private:
    const s32 c_Socket;
    avltree< Others::IPv4PortKey , TransmissionSession* > m_Sessions;
    std::mutex m_Lock;
public:
    Transmission(s32 Socket);
    ~Transmission();
public:
    bool Connect(u32 IPv4, u16 Port, u08 Concurrency, TransmissionSession::TRANSMISSION_MODE TransmissionMode, TransmissionSession::BLOCK_SIZE BlockSize, u16 RetransmissionRedundancy, u16 RetransmissionInterval);
    u16 Send(u32 IPv4, u16 Port, u08* buffer, u16 buffersize, bool reqack);
public:
    void RxHandler(u08* buffer, u16 size, const sockaddr_in * const sender_addr, const u32 sender_addr_len);
};

}


#endif // TX_H
