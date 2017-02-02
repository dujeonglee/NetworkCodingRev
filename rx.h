#ifndef RX_H
#define RX_H
#include "common.h"
namespace NetworkCoding
{
class ReceptionBlock;
class ReceptionSession;
class Reception;

// 1. Acknowleged.
// 2. Consumed.
class ReceptionBlock
{
private:
    static std::queue<ReceptionBlock*> m_Pool;
public:
    static ReceptionBlock* GetFreeBlock();
    static void PutFreeBlock(ReceptionBlock*);
public:
    u08 m_Rank;
    u08 m_Acknowledged;
    std::vector < Others::PacketBuffer > m_Buffer;
};

class ReceptionSession
{
public:
    enum ActionType: u08
    {
        QUEUE_ON_NEW_BLOCK,
        QUEUE_ON_EXISTING_BLOCK,
        ACK,
        DISCARD
    };

    s32 m_MinBlockSequence;
    s32 m_MaxBlockSequence;
    avltree< u16, ReceptionBlock* > m_Blocks;
    ReceptionSession();
    ActionType Action(Header::Data* s);
};

class Reception
{
private:
    const s32 c_Socket;
public:
    Reception(s32 Socket);
    ~Reception();
public:
    std::mutex m_Lock;
    avltree<Others::IPv4PortKey, ReceptionSession*> m_Sessions;
public:
    void RxHandler(u08* buffer, u16 size, const sockaddr_in * const sender_addr, const u32 sender_addr_len);
};

}
#endif
