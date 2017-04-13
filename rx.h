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
    const u16 m_BlockSequenceNumber;
    std::vector< std::unique_ptr< u08[] > > m_DecodedPacketBuffer;
    std::vector< std::unique_ptr< u08[] > > m_EncodedPacketBuffer;
    std::vector< std::unique_ptr< u08[] > > m_DecodingMatrix;
    bool m_DecodingReady;
    const u08 FindMaximumRank(Header::Data* hdr = nullptr);
    const bool FindEndOfBlock(Header::Data* hdr = nullptr);
    enum ReceiveAction
    {
        DROP = 0,
        ENQUEUE_AND_DECODING,
        DECODING,
    };

    ReceiveAction FindAction(u08* buffer, u16 length);
    bool Decoding();
public:
    ReceptionBlock() = delete;
    ReceptionBlock(const ReceptionBlock&) = delete;
    ReceptionBlock(ReceptionBlock&&) = delete;
    ReceptionBlock& operator=(const ReceptionBlock&) = delete;
    ReceptionBlock& operator=(ReceptionBlock&&) = delete;

    ReceptionBlock(Reception* const reception, ReceptionSession* const session, const u16 BlockSequenceNumber);
    ~ReceptionBlock();
    void Receive(u08* buffer, u16 length, const sockaddr_in * const sender_addr, const u32 sender_addr_len);
};

class ReceptionSession
{
    friend class ReceptionBlock;
    friend class Reception;
private:
    Reception* const c_Reception;
    AVLTree< u16, ReceptionBlock* > m_Blocks;
    ThreadPool<1, 1> m_RxTaskQueue;
    u16 m_SequenceNumberForService;
    u16 m_MinSequenceNumberAwaitingAck;
    u16 m_MaxSequenceNumberAwaitingAck;
public:
    ReceptionSession(const ReceptionSession&) = delete;
    ReceptionSession(ReceptionSession&&) = delete;
    ReceptionSession& operator=(const ReceptionSession&) = delete;
    ReceptionSession& operator=(ReceptionSession&&) = delete;

    ReceptionSession(Reception * const Session);
    ~ReceptionSession();
    void Receive(u08* buffer, u16 length, const sockaddr_in * const sender_addr, const u32 sender_addr_len);
};

class Reception
{
    friend class ReceptionBlock;
    friend class ReceptionSession;
private:
    const s32 c_Socket;
    AVLTree< DataStructures::IPv4PortKey, ReceptionSession* > m_Sessions;
public:
    const std::function <void (u08* buffer, u16 length, const sockaddr_in * const sender_addr, const u32 sender_addr_len)> m_RxCallback;
    Reception(s32 Socket, std::function <void (u08* buffer, u16 length, const sockaddr_in * const sender_addr, const u32 sender_addr_len)> rx);
    ~Reception();
public:
    void RxHandler(u08* buffer, u16 size, const sockaddr_in * const sender_addr, const u32 sender_addr_len);
};
}

#endif
