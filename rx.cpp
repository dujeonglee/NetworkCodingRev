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
        freeblock->m_Delivered = 0;
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
        freeblock->m_Delivered = 0;
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
    m_MinBlockSequence = 0;
    m_MaxBlockSequence = Parameter::MAX_CONCURRENCY;
}

ReceptionSession::ActionType ReceptionSession::Action(Header::Data* s)
{
    // Update Minimum Block Sequence Number
    for(u16 i=  0 ; i < Parameter::MAX_CONCURRENCY ; i++)
    {
        if(m_MinBlockSequenceNumber+i == s->m_MinBlockSequenceNumber)
        {
            m_MinBlockSequenceNumber = s->m_MinBlockSequenceNumber;
            break;
        }
    }
    bool Enqueue = false;
    for(u16 i=  0 ; i < Parameter::MAX_CONCURRENCY ; i++)
    {
        if(m_MinBlockSequenceNumber+i == s->m_CurrentBlockSequenceNumber)
        {
            Enqueue = true;
            break;
        }
    }

    return (Enqueue?ReceptionSession::ActionType::QUEUE :
                    ReceptionSession::ActionType::ACK);
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
                        if(DataHeader->m_Flags & Header::Data::DataHeaderFlag::FLAGS_ORIGINAL)
                        {
                            if(block->m_Buffer.size() < DataHeader->m_ExpectedRank)
                            {
                                try
                                {
                                    block->m_Buffer.resize(DataHeader->m_ExpectedRank);
                                }
                                catch(const std::exception& ex)
                                {
                                    std::cout<<ex.what();
                                    return;
                                }
                            }
                            memcpy(block->m_Buffer[DataHeader->m_ExpectedRank].buffer, buffer, size);
                            block->m_Rank++;
                            if(DataHeader->m_CurrentBlockSequenceNumber == (*session)->m_MinBlockSequenceNumber &&
                                    DataHeader->m_ExpectedRank == block->m_Rank)
                            {
                                block->m_Delivered++;
                                // Rx callback
                            }
                            if((DataHeader->m_ExpectedRank == block->m_Rank) &&
                                    (DataHeader->m_Flags & Header::Data::DataHeaderFlag::FLAGS_END_OF_BLK))
                            {
                                Header::DataAck ack;
                                ack.m_Sequence = DataHeader->m_CurrentBlockSequenceNumber;
                                ack.m_AckAddress = DataHeader->m_AckAddress;
                                ack.m_Losses = 0;
                                sendto(c_Socket, &ack, sizeof(ack), 0, (sockaddr*)&sender_addr, sender_addr_len);
                            }
                        }
                        // 4. If encoding packet
                        else
                        {
                            // 4.1 If it is independent packet then store the packet.
                            // 4.2 if all packets are collected to decode
                            // 4.3. forward the packets to local clients.
                        }
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
                    ack.m_Losses = 0;
                    sendto(c_Socket, &ack, sizeof(ack), 0, (sockaddr*)&sender_addr, sender_addr_len);
                    block->m_Acknowledged = true;
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
            (*session)->m_MinBlockSequenceNumber = SyncHeader->m_Sequence;
            sendto(c_Socket, buffer, sizeof(Header::Sync), 0, (sockaddr*)&sender_addr, sender_addr_len);
        }
        break;

        default:
        break;
    }
}
