#include "rx.h"

using namespace NetworkCoding;
/* All RX operation is run on a single thread, which means we do not need locks. */
std::queue<ReceptionBlock*> ReceptionBlock::m_Pool;
ReceptionBlock *ReceptionBlock::GetFreeBlock()
{
    ReceptionBlock* freeblock;
    if(!m_Pool.empty())
    {
        freeblock = m_Pool.front();
        m_Pool.pop();
        freeblock->m_Rank = 0;
        freeblock->m_Acknowledged = 0;
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
        freeblock->m_Acknowledged = 0;
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
    s32 modifier = 0;
    if(m_MinBlockSequence > m_MaxBlockSequence)
    {
        while(!((m_MinBlockSequence + modifier) < (m_MaxBlockSequence + modifier)) &&
              !((m_MaxBlockSequence < 0xffff - Parameter::MAX_CONCURRENCY)))
        {
            modifier--;
        }
    }
    else
    {
        while(!(m_MinBlockSequence > Parameter::MAX_CONCURRENCY))
        {
            modifier++;
        }
    }

    const u16 MODIFIED_MIN_SEQ = m_MinBlockSequence + modifier;
    const u16 MODIFIED_CUR_SEQ = s->m_CurrentBlockSequenceNumber + modifier;
    const u16 MODIFIED_MAX_SEQ = m_MaxBlockSequence + modifier;
    if(MODIFIED_CUR_SEQ < MODIFIED_MIN_SEQ)
    {
        if(MODIFIED_CUR_SEQ >= (m_MaxBlockSequence - Parameter::MAX_CONCURRENCY))
        {
            return ReceptionSession::ACK;
        }
        else
        {
            return ReceptionSession::DISCARD;
        }
    }
    else if(MODIFIED_CUR_SEQ >= MODIFIED_MIN_SEQ && MODIFIED_CUR_SEQ <= MODIFIED_MAX_SEQ)
    {
        return ReceptionSession::QUEUE_ON_EXISTING_BLOCK;
    }
    else
    {
        if(MODIFIED_CUR_SEQ <= (m_MinBlockSequence + Parameter::MAX_CONCURRENCY))
        {
            return ReceptionSession::QUEUE_ON_NEW_BLOCK;
        }
        else
        {
            return ReceptionSession::DISCARD;
        }
    }
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
            {// 1. If there is no associated session, we create new one.
                const Others::IPv4PortKey key = {sender_addr->sin_addr.s_addr, sender_addr->sin_port};
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

            {
                Header::Data* DataHeader = reinterpret_cast< Header::Data* >(buffer);
                // 1. Check if the block sequence number is valid.
                ReceptionBlock* block = nullptr;
                const ReceptionSession::ActionType ActionForThisPacket = (*session)->Action(DataHeader);
                switch(ActionForThisPacket)
                {
                    case ReceptionSession::ActionType::QUEUE_ON_NEW_BLOCK:
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
                    break;

                    case ReceptionSession::ActionType::QUEUE_ON_EXISTING_BLOCK:
                    {
                        ReceptionBlock** tmp = (*session)->m_Blocks.find(DataHeader->m_CurrentBlockSequenceNumber);
                        if(tmp == nullptr)
                        {
                            return;
                        }
                        block = (*tmp);
                    }
                    break;

                    case ReceptionSession::ActionType::ACK:
                    case ReceptionSession::ActionType::DISCARD:
                    {
                        // Send Ack;
                        return;
                    }
                    break;
                }
                if(block)
                {
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
                        if(DataHeader->m_CurrentBlockSequenceNumber == (*session)->m_MinBlockSequence && DataHeader->m_ExpectedRank == block->m_Rank)
                        {
                            // Rx callback
                        }
                        if((DataHeader->m_ExpectedRank == block->m_Rank) && (DataHeader->m_Flags & Header::Data::DataHeaderFlag::FLAGS_END_OF_BLK))
                        {
                            Header::DataAck ack;
                            ack.m_Sequence = DataHeader->m_CurrentBlockSequenceNumber;
                            ack.m_AckAddress = DataHeader->m_AckAddress;
                            ack.m_Losses = 0;
                            sendto(c_Socket, &ack, sizeof(ack), 0, (sockaddr*)&sender_addr, sender_addr_len);
                            block->m_Acknowledged = true;
                            (*session)->m_MinBlockSequence++;
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
        }
        break;

        case Header::Common::HeaderType::SYNC:
        Header::Sync* SyncHeader = reinterpret_cast< Header::Sync* >(buffer);
        ReceptionSession** session;
        {// 1. If there is no associated session, we create new one.
            const Others::IPv4PortKey key = {sender_addr->sin_addr.s_addr, sender_addr->sin_port};
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
        (*session)->m_MinBlockSequence = SyncHeader->m_Sequence;
        (*session)->m_MaxBlockSequence = (*session)->m_MinBlockSequence + Parameter::MAX_CONCURRENCY;
        sendto(c_Socket, buffer, sizeof(Header::Sync), 0, (sockaddr*)&sender_addr, sender_addr_len);
        break;

        default:
        break;
    }
}
