#include "rx.h"

using namespace NetworkCoding;

std::queue<ReceptionBlock*> ReceptionBlock::m_Pool;
ReceptionBlock *ReceptionBlock::GetFreeBlock()
{
    ReceptionBlock* freeblock;
    if(!m_Pool.empty())
    {
        freeblock = m_Pool.front();
        m_Pool.pop();
        freeblock->m_Rank = 0;
        return freeblock;
    }
    else
    {
        try
        {
            freeblock = new ReceptionBlock();
        }
        catch(const std::exception& ex)
        {
            return nullptr;
        }
        freeblock->m_Rank = 0;
        return freeblock;
    }
}

void ReceptionBlock::PutFreeBlock(ReceptionBlock* block)
{
    try
    {
        m_Pool.push(block);
    }
    catch(const std::exception& ex)
    {
        delete block;
    }
}

ReceptionSession::ReceptionSession()
{
    m_CurrentBlockSequenceNumber = 0;
}

const ReceptionSession::ActionType ReceptionSession::Action(const Header::Data* const s)
{
    for(u16 i=  0 ; i < Parameter::MAX_CONCURRENCY ; i++)
    {
        if(m_CurrentBlockSequenceNumber + i == s->m_CurrentBlockSequenceNumber)
        {
            ReceptionBlock** const block = m_Blocks.find(m_CurrentBlockSequenceNumber + i);
            if((*block)->m_FullRank == (*block)->m_Rank)
            {
                return ReceptionSession::ActionType::ACK;
            }
            return ReceptionSession::ActionType::QUEUE;
        }
    }
    return ReceptionSession::ActionType::ACK;
}

Reception::Reception(s32 Socket) : c_Socket(Socket)
{

}

Reception::~Reception()
{
    m_Sessions.perform_for_all_data([](ReceptionSession*& session ){delete session;});
    m_Sessions.clear();
}


void Reception::RxHandler(u08* buffer, u16 size, const sockaddr_in * const sender_addr, const u32 sender_addr_len)
{
    Header::Common* CommonHeader = reinterpret_cast< Header::Common* >(buffer);
    switch(CommonHeader->m_Type)
    {
        case Header::Common::HeaderType::DATA:
        {
            ReceptionSession** session;
            // 1. If there is no associated session, we drop the packet.
            const DataStructures::IPv4PortKey key = {sender_addr->sin_addr.s_addr, sender_addr->sin_port};
            session = m_Sessions.find(key);
            if(session == nullptr)
            {
                return;
            }

            const Header::Data* DataHeader = reinterpret_cast< Header::Data* >(buffer);
            // 1. Check if the block sequence number is valid.
            ReceptionBlock* block = nullptr;
            const ReceptionSession::ActionType ActionForThisPacket = (*session)->Action(DataHeader);
            switch(ActionForThisPacket)
            {
                case ReceptionSession::ActionType::QUEUE:
                {
                    ReceptionBlock** tmp = (*session)->m_Blocks.find(DataHeader->m_CurrentBlockSequenceNumber);
                    if(tmp == nullptr)
                    {
                        block = ReceptionBlock::GetFreeBlock();
                        if(block == nullptr)
                        {
                            return;
                        }
                        if((*session)->m_Blocks.insert(DataHeader->m_CurrentBlockSequenceNumber, block) == false)
                        {
                            ReceptionBlock::PutFreeBlock(block);
                            return;
                        }
                    }
                    else
                    {
                        block = (*tmp);
                    }
                    if(block)
                    {
                        if(DataHeader->m_ExpectedRank > block->m_Buffer.size())
                        {
                            try
                            {
                                block->m_Buffer.resize(DataHeader->m_ExpectedRank);
                            }
                            catch(const std::bad_alloc& ex)
                            {
                                std::cout<<ex.what();
                                return;
                            }
                        }
                        // 3. If original packet

                        // 4. If encoding packet
                            // 4.1 If it is independent packet then store the packet.
                            // 4.2 if all packets are collected to decode
                            // 4.3. forward the packets to local clients.
                        // Check next block is fully decoded.
                    }
                }
                break;

                case ReceptionSession::ActionType::ACK:
                {
                    // Send Ack;
                    Header::DataAck ack;
                    ack.m_Sequence = DataHeader->m_CurrentBlockSequenceNumber;
                    ack.m_AckAddress = DataHeader->m_AckAddress;
                    ack.m_Losses = DataHeader->m_TxCount; /* It is bidirectional packet loss including ack loss. */
                    sendto(c_Socket, &ack, sizeof(ack), 0, (sockaddr*)&sender_addr, sender_addr_len);
                    return;
                }
                break;
            }
        }
        break;

        case Header::Common::HeaderType::SYNC:
        {
            ReceptionSession** session;
            Header::Sync* SyncHeader = reinterpret_cast< Header::Sync* >(buffer);
            {// 1. If there is no associated session, we create new one.
                const DataStructures::IPv4PortKey key = {sender_addr->sin_addr.s_addr, sender_addr->sin_port};
                session = m_Sessions.find(key);
                if(session == nullptr)
                {
                    ReceptionSession* NewSession = nullptr;
                    try
                    {
                        NewSession = new ReceptionSession();
                    }
                    catch(const std::bad_alloc& ex)
                    {
                        std::cout<<ex.what();
                        return;
                    }
                    if(m_Sessions.insert(key, NewSession) == false)
                    {
                        delete NewSession;
                        return;
                    }
                    session = &NewSession;
                }
            }
            (*session)->m_Blocks.clear();
            (*session)->m_CurrentBlockSequenceNumber = SyncHeader->m_Sequence;
            sendto(c_Socket, buffer, sizeof(Header::Sync), 0, (sockaddr*)&sender_addr, sender_addr_len);
        }
        break;

        default:
        break;
    }
}
