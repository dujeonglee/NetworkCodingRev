#include "rx.h"
#include <cstdlib>

#define ASCENDING_ORDER(a,b,c)        ((u16)(c - a) >= (u16)(c - b))

using namespace NetworkCoding;

void PRINT(Header::Data* data)
{
    printf("[Type %hhu][TotalSize %hu][MinSeq. %hu][CurSeq. %hu][MaxSeq. %hu][Exp.Rank %hhu][Max.Rank %hhu][Addr %ld][Flags %hhx][TxCnt %hhu][Payload %hu][LastInd. %hhu]",
            data->m_Type,
            data->m_TotalSize,
            data->m_MinBlockSequenceNumber,
            data->m_CurrentBlockSequenceNumber,
            data->m_MaxBlockSequenceNumber,
            data->m_ExpectedRank,
            data->m_MaximumRank,
            data->m_AckAddress,
            data->m_Flags,
            data->m_TxCount,
            data->m_PayloadSize,
            data->m_LastIndicator);
    printf("[Code ");
    for(u08 i = 0 ; i < data->m_MaximumRank ; i++)
    {
        printf(" %3hhu ", data->m_Codes[i]);
    }
    printf("]\n");
}

bool ReceptionBlock::IsInnovative(u08* buffer, u16 length)
{
    const u08 OLD_RANK = m_DecodedPacketBuffer.size() + m_EncodedPacketBuffer.size();
    const u08 EXPECTED_RANK = OLD_RANK+1;
    u08 MAX_RANK = 0;
    std::vector<Header::Data*> Matrix;
    if(OLD_RANK == 0)
    {
        // First packet is always innovative.
        return true;
    }
    else if(m_DecodedPacketBuffer.size() == 0
            && reinterpret_cast <Header::Data*>(buffer)->m_Flags & Header::Data::DataHeaderFlag::FLAGS_ORIGINAL)
    {
        // When there is only original packet in the buffer, and the received packet is Decoded packet,
        // we can guarnatee this packet is always innovative.
        return true;
    }
    else if(OLD_RANK == reinterpret_cast<Header::Data*>(buffer)->m_ExpectedRank)
    {
        return false;
    }
    // 1. Allcate Decoding Matrix
    try
    {
        Matrix.resize(EXPECTED_RANK, nullptr);
    }
    catch(const std::bad_alloc& ex)
    {
        std::cout<<ex.what()<<std::endl;
        return false;
    }
    u08 idx = 0;
    u08 decoded_idx = 0;
    u08 encoded_idx = 0;
    u08 rxpacket_idx = 0;
    for(u08 i = 0 ; i < EXPECTED_RANK ; )
    {
        if(decoded_idx < m_DecodedPacketBuffer.size() && reinterpret_cast<Header::Data*>(m_DecodedPacketBuffer[decoded_idx].get())->m_Codes[idx])
        {
            if(MAX_RANK < reinterpret_cast<Header::Data*>(m_DecodedPacketBuffer[decoded_idx++].get())->m_ExpectedRank)
            {
                MAX_RANK = reinterpret_cast<Header::Data*>(m_DecodedPacketBuffer[decoded_idx++].get())->m_ExpectedRank;
            }
            Matrix[i++] = reinterpret_cast<Header::Data*>(m_DecodedPacketBuffer[decoded_idx++].get());
        }
        else if(encoded_idx < m_EncodedPacketBuffer.size() && reinterpret_cast<Header::Data*>(m_EncodedPacketBuffer[encoded_idx].get())->m_Codes[idx])
        {
            if(MAX_RANK < reinterpret_cast<Header::Data*>(m_EncodedPacketBuffer[encoded_idx++].get())->m_ExpectedRank)
            {
                MAX_RANK = reinterpret_cast<Header::Data*>(m_EncodedPacketBuffer[encoded_idx++].get())->m_ExpectedRank;
            }
            Matrix[i++] = reinterpret_cast<Header::Data*>(m_EncodedPacketBuffer[encoded_idx++].get());
        }
        else if(rxpacket_idx < 1 && reinterpret_cast<Header::Data*>(buffer)->m_Codes[idx])
        {
            if(MAX_RANK < reinterpret_cast<Header::Data*>(buffer)->m_ExpectedRank)
            {
                MAX_RANK = reinterpret_cast<Header::Data*>(buffer)->m_ExpectedRank;
            }
            Matrix[i++] = reinterpret_cast<Header::Data*>(buffer);
            rxpacket_idx++;
        }
        if(decoded_idx == m_DecodedPacketBuffer.size() &&
                encoded_idx == m_EncodedPacketBuffer.size() &&
                rxpacket_idx == 1 &&
                i < EXPECTED_RANK)
        {
            exit(-1);
            return false;
        }
        idx++;
    }
    for(u08 i = 0 ; i < EXPECTED_RANK ; )
    {
        while(Matrix[i]->m_Codes[idx] == 0)
        {
            idx++;
        }
        // 1. Make I-matrix
        if(Matrix[i]->m_Codes[idx] != 1)
        {
            const u08 MUL = FiniteField::instance()->inv(Matrix[i]->m_Codes[idx]);
            for(u08 ii = 0 ; ii < MAX_RANK ; ii++)
            {
                Matrix[i]->m_Codes[ii] = FiniteField::instance()->mul(MUL, Matrix[i]->m_Codes[ii]);
            }
        }
        // 2. Elimination
        for(u08 ii = 0 ; ii < EXPECTED_RANK ; ii++)
        {
            if(ii == i)
            {
                continue;
            }
            if(Matrix[ii]->m_Codes[idx] > 0)
            {
                for(u08 iii = 0 ; iii < )
            }
        }
    }
    return true;
}

ReceptionBlock::ReceptionBlock(Reception * const reception, ReceptionSession * const session, const u16 BlockSequenceNumber):c_Reception(reception), c_Session(session), m_BlockSequenceNumber(BlockSequenceNumber)
{
    m_DecodedPacketBuffer.clear();
    m_EncodedPacketBuffer.clear();
    m_ServiceMask[0] = 0;
    m_ServiceMask[1] = 0;
    m_ServiceMask[2] = 0;
    m_ServiceMask[3] = 0;
}

ReceptionBlock::~ReceptionBlock()
{
    m_DecodedPacketBuffer.clear();
    m_EncodedPacketBuffer.clear();
}

void ReceptionBlock::Receive(u08 *buffer, u16 length, const sockaddr_in * const sender_addr, const u32 sender_addr_len)
{
    Header::Data* const DataHeader = reinterpret_cast <Header::Data*>(buffer);
    if((DataHeader->m_Flags & Header::Data::DataHeaderFlag::FLAGS_END_OF_BLK) &&
            (m_DecodedPacketBuffer.size() + m_EncodedPacketBuffer.size()) == DataHeader->m_ExpectedRank)
    {
        Header::DataAck ack;
        ack.m_Type = Header::Common::HeaderType::DATA_ACK;
        ack.m_Sequence = DataHeader->m_CurrentBlockSequenceNumber;
        ack.m_AckAddress = DataHeader->m_AckAddress;
#ifdef ENVIRONMENT32
        ack.m_Reserved = 0; // 32bit machine uses the first 4 bytes and the remaining memory shall be set to 0. This field should be used after casting to laddr.
#endif
        ack.m_Losses = DataHeader->m_TxCount - DataHeader->m_ExpectedRank;
        sendto(c_Reception->c_Socket, (u08*)&ack, sizeof(ack), 0, (sockaddr*)sender_addr, sender_addr_len);
        return;
    }
    if(IsInnovative(buffer, length) == false)
    {
        return;
    }
    if(DataHeader->m_Flags & Header::Data::DataHeaderFlag::FLAGS_ORIGINAL)
    {
        try
        {
            m_DecodedPacketBuffer.emplace_back(std::unique_ptr<u08[]>(new u08[length]));
        }
        catch(const std::bad_alloc& ex)
        {
            std::cout<<ex.what()<<std::endl;
            return;
        }
        memcpy(m_DecodedPacketBuffer.back().get(), buffer, length);
    }
    else
    {
        try
        {
            m_EncodedPacketBuffer.emplace_back(std::unique_ptr<u08[]>(new u08[length]));
        }
        catch(const std::bad_alloc& ex)
        {
            std::cout<<ex.what()<<std::endl;
            return;
        }
        memcpy(m_EncodedPacketBuffer.back().get(), buffer, length);
    }
    if(DataHeader->m_Flags & Header::Data::DataHeaderFlag::FLAGS_ORIGINAL)
    {
        if(c_Session->m_MinSequenceNumber == DataHeader->m_CurrentBlockSequenceNumber &&
                DataHeader->m_ExpectedRank == m_DecodedPacketBuffer.size())
        {
            reinterpret_cast <Header::Data*>(m_DecodedPacketBuffer.back().get())->m_Flags |= Header::Data::DataHeaderFlag::FLAGS_CONSUMED;
            const u32 buffer_index = m_DecodedPacketBuffer.size();
            const sockaddr_in addr = (*sender_addr);
            const u32 addr_length = sizeof(addr);
            c_Session->m_RxTaskQueue.Enqueue([this, buffer_index, length, addr, addr_length](){
                c_Reception->m_RxCallback(m_DecodedPacketBuffer[buffer_index].get(), length, &addr, addr_length);
            });
        }
    }
    if((DataHeader->m_ExpectedRank == (m_DecodedPacketBuffer.size() + m_EncodedPacketBuffer.size())) &&
            (DataHeader->m_Flags & Header::Data::DataHeaderFlag::FLAGS_END_OF_BLK))
    {
        // Decoding.
        if(m_EncodedPacketBuffer.size())
        {
            printf("Decoding...%u %u\n", m_DecodedPacketBuffer.size(), m_EncodedPacketBuffer.size());
        }
        else
        {
            printf("No Decoding!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
        }
        for(u08 i = 0 ; i < m_DecodedPacketBuffer.size() ; i++)
        {
            //PRINT(reinterpret_cast <Header::Data*>(m_DecodedPacketBuffer[i].get()));
        }
        for(u08 i = 0 ; i < m_EncodedPacketBuffer.size() ; i++)
        {
            //PRINT(reinterpret_cast <Header::Data*>(m_EncodedPacketBuffer[i].get()));
        }
        Header::DataAck ack;
        ack.m_Type = Header::Common::HeaderType::DATA_ACK;
        ack.m_Sequence = DataHeader->m_CurrentBlockSequenceNumber;
        ack.m_AckAddress = DataHeader->m_AckAddress;
#ifdef ENVIRONMENT32
        ack.m_Reserved = 0; // 32bit machine uses the first 4 bytes and the remaining memory shall be set to 0. This field should be used after casting to laddr.
#endif
        ack.m_Losses = DataHeader->m_TxCount - DataHeader->m_ExpectedRank;
        sendto(c_Reception->c_Socket, (u08*)&ack, sizeof(ack), 0, (sockaddr*)sender_addr, sender_addr_len);
    }
}


ReceptionSession::ReceptionSession(Reception * const Session):c_Reception(Session)
{
    m_MinSequenceNumber = 0;
    m_MaxSequenceNumber = 0;
}

ReceptionSession::~ReceptionSession()
{
    m_Blocks.DoSomethingOnAllData([](ReceptionBlock* &block){delete block;});
}

void ReceptionSession::Receive(u08* buffer, u16 length, const sockaddr_in * const sender_addr, const u32 sender_addr_len)
{
    Header::Data* const DataHeader = reinterpret_cast <Header::Data*>(buffer);
    // update min and max sequence.
    if(ASCENDING_ORDER(m_MinSequenceNumber, DataHeader->m_MinBlockSequenceNumber, m_MaxSequenceNumber))
    {
        for(; m_MinSequenceNumber!=DataHeader->m_MinBlockSequenceNumber ; m_MinSequenceNumber++)
        {
            // This must be changed not to delete Block until it is successfully delivered to application.
            m_Blocks.Remove(m_MinSequenceNumber, [](ReceptionBlock* &data){delete data;});
        }
        m_MinSequenceNumber = DataHeader->m_MinBlockSequenceNumber;
    }
    if(ASCENDING_ORDER(m_MinSequenceNumber, m_MaxSequenceNumber, DataHeader->m_MaxBlockSequenceNumber))
    {
        m_MaxSequenceNumber = DataHeader->m_MaxBlockSequenceNumber;
    }
    if(!ASCENDING_ORDER(m_MinSequenceNumber, DataHeader->m_CurrentBlockSequenceNumber, (m_MaxSequenceNumber+(Parameter::MAXIMUM_NUMBER_OF_CONCURRENT_RETRANSMISSION - (u16)(m_MaxSequenceNumber - m_MinSequenceNumber)))))
    {
        // If the sequence is less than min seq send ack and return.
        Header::DataAck ack;
        ack.m_Type = Header::Common::HeaderType::DATA_ACK;
        ack.m_Sequence = DataHeader->m_CurrentBlockSequenceNumber;
        ack.m_AckAddress = DataHeader->m_AckAddress;
#ifdef ENVIRONMENT32
        ack.m_Reserved = 0; // 32bit machine uses the first 4 bytes and the remaining memory shall be set to 0. This field should be used after casting to laddr.
#endif
        ack.m_Losses = DataHeader->m_TxCount - DataHeader->m_ExpectedRank;
        sendto(c_Reception->c_Socket, (u08*)&ack, sizeof(ack), 0, (sockaddr*)sender_addr, sender_addr_len);
        return;
    }

    ReceptionBlock** const pp_Block = m_Blocks.GetPtr(DataHeader->m_CurrentBlockSequenceNumber);
    ReceptionBlock* p_Block = nullptr;
    if(pp_Block == nullptr)
    {
        try
        {
            p_Block = new ReceptionBlock(c_Reception, this, DataHeader->m_CurrentBlockSequenceNumber);
        }
        catch(const std::bad_alloc& ex)
        {
            std::cout<<ex.what()<<std::endl;
            return;
        }
        if(m_Blocks.Insert(DataHeader->m_CurrentBlockSequenceNumber, p_Block) == false)
        {
            delete p_Block;
            return;
        }
    }
    else
    {
        p_Block = (*pp_Block);
    }
    p_Block->Receive(buffer, length, sender_addr, sender_addr_len);
}

Reception::Reception(s32 Socket, std::function<void(u08* buffer, u16 length, const sockaddr_in * const sender_addr, const u32 sender_addr_len)> rx) : c_Socket(Socket), m_RxCallback(rx){}

Reception::~Reception()
{
    m_Sessions.DoSomethingOnAllData([](ReceptionSession* &session ){delete session;});
    m_Sessions.Clear();
}

void Reception::RxHandler(u08* buffer, u16 size, const sockaddr_in * const sender_addr, const u32 sender_addr_len)
{
    Header::Common* CommonHeader = reinterpret_cast< Header::Common* >(buffer);
    switch(CommonHeader->m_Type)
    {
    case Header::Common::HeaderType::DATA:
    {
        if((std::rand()%2) == 0)
        {
            return;
        }
        //PRINT((Header::Data*)buffer);
        const DataStructures::IPv4PortKey key = {sender_addr->sin_addr.s_addr, sender_addr->sin_port};
        ReceptionSession** const pp_Session = m_Sessions.GetPtr(key);
        ReceptionSession* p_Session = nullptr;
        if(pp_Session == nullptr)
        {
            try
            {
                p_Session = new ReceptionSession(this);
            }
            catch(const std::bad_alloc& ex)
            {
                std::cout<<ex.what()<<std::endl;
                return;
            }
            if(m_Sessions.Insert(key, p_Session) == false)
            {
                delete p_Session;
                return;
            }
        }
        else
        {
            p_Session = (*pp_Session);
        }
        p_Session->Receive(buffer, size, sender_addr, sender_addr_len);
    }
        break;

    case Header::Common::HeaderType::SYNC:
    {
        // create Rx Session.
        Header::Sync* const sync = reinterpret_cast< Header::Sync* >(buffer);
        sync->m_Type = Header::Common::HeaderType::SYNC_ACK;
        sendto(c_Socket, buffer, size, 0, (sockaddr*)sender_addr, sender_addr_len);
    }
        break;
    default:
        break;
    }

}
