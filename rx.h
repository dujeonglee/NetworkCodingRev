#ifndef RX_H
#define RX_H
#include "common.h"
#include "ThreadPool.h"

namespace NetworkCoding
{
class ReceptionBlock;
class ReceptionSession;
class Reception;

class ReceptionBlock
{
    friend class ReceptionSession;
    friend class Reception;
private:
    Reception* const c_Reception;
    ReceptionSession* const c_Session;
    const uint16_t m_BlockSequenceNumber;
    std::vector< std::unique_ptr< uint8_t[] > > m_DecodedPacketBuffer;
    std::vector< std::unique_ptr< uint8_t[] > > m_EncodedPacketBuffer;
    std::vector< std::unique_ptr< uint8_t[] > > m_DecodingMatrix;
    bool m_DecodingReady;
    const uint8_t FindMaximumRank(Header::Data* hdr = nullptr);
    const bool FindEndOfBlock(Header::Data* hdr = nullptr);
    enum ReceiveAction
    {
        DROP = 0,
        ENQUEUE_AND_DECODING,
        DECODING,
    };

    ReceiveAction FindAction(uint8_t* buffer, uint16_t length);
    bool Decoding();
public:
    ReceptionBlock() = delete;
    ReceptionBlock(const ReceptionBlock&) = delete;
    ReceptionBlock(ReceptionBlock&&) = delete;
    ReceptionBlock& operator=(const ReceptionBlock&) = delete;
    ReceptionBlock& operator=(ReceptionBlock&&) = delete;

    ReceptionBlock(Reception* const reception, ReceptionSession* const session, const uint16_t BlockSequenceNumber);
    ~ReceptionBlock();
    void Receive(uint8_t* buffer, uint16_t length, const sockaddr* const sender_addr, const uint32_t sender_addr_len);
};

class ReceptionSession
{
    friend class ReceptionBlock;
    friend class Reception;
private:
    Reception* const c_Reception;
    AVLTree< uint16_t, ReceptionBlock* > m_Blocks;
    ThreadPool<1, 1> m_RxTaskQueue;
    uint16_t m_SequenceNumberForService;
    uint16_t m_MinSequenceNumberAwaitingAck;
    uint16_t m_MaxSequenceNumberAwaitingAck;
    const DataStructures::AddressType m_SenderAddress;
public:
    ReceptionSession(const ReceptionSession&) = delete;
    ReceptionSession(ReceptionSession&&) = delete;
    ReceptionSession& operator=(const ReceptionSession&) = delete;
    ReceptionSession& operator=(ReceptionSession&&) = delete;

    ReceptionSession(Reception * const Session, const DataStructures::AddressType addr);
    ~ReceptionSession();
    void Receive(uint8_t* buffer, uint16_t length, const sockaddr* const sender_addr, const uint32_t sender_addr_len);
};

class Reception
{
    friend class ReceptionBlock;
    friend class ReceptionSession;
private:
    const int32_t c_Socket;
    AVLTree< DataStructures::SessionKey, ReceptionSession* > m_Sessions;
public:
    const std::function <void (uint8_t* buffer, uint16_t length, const sockaddr* const sender_addr, const uint32_t sender_addr_len)> m_RxCallback;
    Reception(int32_t Socket, std::function <void (uint8_t* buffer, uint16_t length, const sockaddr* const sender_addr, const uint32_t sender_addr_len)> rx);
    ~Reception();
public:
    void RxHandler(uint8_t* buffer, uint16_t size, const sockaddr* const sender_addr, const uint32_t sender_addr_len);
};
}

#endif
