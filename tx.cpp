#include "tx.h"

using namespace NetworkCoding;

////////////////////////////////////////////////////////////
/////////////// TransmissionBlock
/*
 * Create TransmissionBlock.
 */
TransmissionBlock::TransmissionBlock(TransmissionSession *p_session) :
    p_Session(p_session),
    m_BlockSize(p_session->m_BlockSize),
    m_TransmissionMode(p_session->m_TransmissionMode),
    m_BlockSequenceNumber(p_session->m_MaxBlockSequenceNumber.fetch_add(1, std::memory_order_relaxed)),
    m_RetransmissionRedundancy(p_session->m_RetransmissionRedundancy),
    m_RetransmissionInterval(p_session->m_RetransmissionInterval)
{
    p_Session->m_ConcurrentRetransmissions++;
    m_LargestOriginalPacketSize = 0;
    m_TransmissionCount = 0;
    p_Session->m_AckList[AckIndex()] = false;
}

TransmissionBlock::~TransmissionBlock()
{
    p_Session->m_AckList[AckIndex()] = true;
    m_OriginalPacketBuffer.clear();
    p_Session->m_ConcurrentRetransmissions--;
}


const u16 TransmissionBlock::AckIndex()
{
    return m_BlockSequenceNumber%(Parameter::MAXIMUM_NUMBER_OF_CONCURRENT_RETRANSMISSION*2);
}

const bool TransmissionBlock::IsAcked()
{
    return p_Session->m_AckList[AckIndex()];
}

bool TransmissionBlock::Send(u08* buffer, u16 buffersize, bool reqack)
{
    if(p_Session->m_IsConnected == false)
    {
        return false;
    }
    // 2. Allocate memory for packet buffer.
    try
    {
        m_OriginalPacketBuffer.emplace_back(std::unique_ptr<u08[]>(new u08[sizeof(Header::Data) + (m_BlockSize - 1) + buffersize]));
    }
    catch(const std::bad_alloc& ex)
    {
        std::cout<<ex.what()<<std::endl;
        return false;
    }

    // Fill up the fields
    Header::Data* const DataHeader = reinterpret_cast <Header::Data*>(m_OriginalPacketBuffer[m_TransmissionCount].get());
    DataHeader->m_Type = Header::Common::HeaderType::DATA;
    DataHeader->m_TotalSize = sizeof(Header::Data) + (m_BlockSize - 1) + buffersize;
    DataHeader->m_MinBlockSequenceNumber = p_Session->m_MinBlockSequenceNumber;
    DataHeader->m_CurrentBlockSequenceNumber = m_BlockSequenceNumber;
    DataHeader->m_MaxBlockSequenceNumber = p_Session->m_MaxBlockSequenceNumber;
    DataHeader->m_ExpectedRank = m_TransmissionCount + 1;
    DataHeader->m_MaximumRank = m_BlockSize;
    DataHeader->m_AckAddress = (laddr)(&(p_Session->m_AckList[AckIndex()]));
#ifdef ENVIRONMENT32
    DataHeader->m_Reserved = 0;
#endif
    DataHeader->m_Flags = Header::Data::FLAGS_ORIGINAL;
    DataHeader->m_TxCount = m_TransmissionCount + 1;
    if(reqack)
    {
        //Note: Header::Data::FLAGS_END_OF_BLK asks ack from the client
        DataHeader->m_Flags |= Header::Data::FLAGS_END_OF_BLK;
    }
    DataHeader->m_PayloadSize = buffersize;
    if(m_LargestOriginalPacketSize < buffersize)
    {
        m_LargestOriginalPacketSize = buffersize;
    }
    DataHeader->m_LastIndicator = 1;/* This field is reserved to support fragmentation */
    for(u08 i = 0 ; i < m_BlockSize ; i++)
    {
        if(i != m_TransmissionCount)
        {
            DataHeader->m_Codes[i] = 0;
        }
        else
        {
            DataHeader->m_Codes[i] = 1;
        }
    }
    memcpy(DataHeader->m_Codes + m_BlockSize, buffer, buffersize);
    sockaddr_in RemoteAddress = {0};
    RemoteAddress.sin_family = AF_INET;
    RemoteAddress.sin_addr.s_addr = p_Session->c_IPv4;
    RemoteAddress.sin_port = p_Session->c_Port;
    sendto(p_Session->c_Socket, m_OriginalPacketBuffer[m_TransmissionCount++].get(), DataHeader->m_TotalSize, 0, (sockaddr*)&RemoteAddress, sizeof(RemoteAddress));

    if((m_TransmissionCount == m_BlockSize) || reqack == true)
    {
        p_Session->p_TransmissionBlock = nullptr;
        p_Session->m_Timer.ScheduleTask(m_RetransmissionInterval, [this](){
            p_Session->m_TaskQueue.Enqueue([this](){
                Retransmission();
            }, (p_Session->m_MinBlockSequenceNumber == m_BlockSequenceNumber?TransmissionSession::MIDDLE_PRIORITY:TransmissionSession::LOW_PRIORITY));
        });
    }
    return true;
}

void TransmissionBlock::Retransmission()
{
    if(p_Session->m_IsConnected == false)
    {
        return;
    }
    const u08 c_AckIndex = AckIndex();
    if(m_TransmissionMode == Parameter::BEST_EFFORT_TRANSMISSION_MODE)
    {
        if(m_TransmissionCount >= m_OriginalPacketBuffer.size() + m_RetransmissionRedundancy)
        {
            p_Session->m_AckList[c_AckIndex] = true;
        }
    }
    if(IsAcked() == true)
    {
        for(u16 i = p_Session->m_MinBlockSequenceNumber ; i != p_Session->m_MaxBlockSequenceNumber ; i++)
        {
            if(p_Session->m_AckList[i%(Parameter::MAXIMUM_NUMBER_OF_CONCURRENT_RETRANSMISSION*2)] == true)
            {
                p_Session->m_MinBlockSequenceNumber++;
            }
            else
            {
                break;
            }
        }
        delete this;
        return;
    }
    sockaddr_in RemoteAddress = {0};
    RemoteAddress.sin_family = AF_INET;
    RemoteAddress.sin_addr.s_addr = p_Session->c_IPv4;
    RemoteAddress.sin_port = p_Session->c_Port;
    if(m_OriginalPacketBuffer.size() == 1)
    {
        Header::Data* DataHeader = reinterpret_cast<Header::Data*>(m_OriginalPacketBuffer[0].get());
        DataHeader->m_Type = Header::Common::HeaderType::DATA;
        /* Must be the same *///DataHeader->m_TotalSize = sizeof(Header::Data) + (m_BlockSize-1) + m_LargestOriginalPacketSize;
        DataHeader->m_MinBlockSequenceNumber = p_Session->m_MinBlockSequenceNumber.load();
        /* Must be the same *///DataHeader->m_CurrentBlockSequenceNumber = m_BlockSequenceNumber;
        DataHeader->m_MaxBlockSequenceNumber = p_Session->m_MaxBlockSequenceNumber.load();
        /* Must be the same *///DataHeader->m_ExpectedRank = m_BlockSize;
        /* Must be the same *///DataHeader->m_MaximumRank = m_BlockSize;
        /* Must be the same *///DataHeader->m_AckAddress = (laddr)(&(p_Session->m_AckList[c_AckIndex]));
#ifdef ENVIRONMENT32
        DataHeader->m_Reserved = 0;
#endif
        DataHeader->m_Flags = Header::Data::FLAGS_ORIGINAL | Header::Data::FLAGS_END_OF_BLK;
        DataHeader->m_TxCount = m_TransmissionCount++;
        sendto(p_Session->c_Socket, m_OriginalPacketBuffer[0].get(), DataHeader->m_TotalSize, 0, (sockaddr*)&RemoteAddress, sizeof(RemoteAddress));
    }
    else
    {
        std::vector<u08> RandomCoefficients;
        Header::Data* RemedyHeader = reinterpret_cast<Header::Data*>(m_RemedyPacketBuffer);

        for(u08 Coef = 0 ; Coef < m_BlockSize ; Coef++)
        {
            RandomCoefficients.push_back((rand()%255)+1);
        }

        RandomCoefficients.resize(m_BlockSize, (rand()%255)+1);
        RemedyHeader->m_Type = Header::Common::HeaderType::DATA;
        RemedyHeader->m_TotalSize = sizeof(Header::Data) + (m_BlockSize-1) + m_LargestOriginalPacketSize;
        RemedyHeader->m_MinBlockSequenceNumber = p_Session->m_MinBlockSequenceNumber.load();
        RemedyHeader->m_CurrentBlockSequenceNumber = m_BlockSequenceNumber;
        RemedyHeader->m_MaxBlockSequenceNumber = p_Session->m_MaxBlockSequenceNumber.load();
        RemedyHeader->m_ExpectedRank = m_OriginalPacketBuffer.size();
        RemedyHeader->m_MaximumRank = m_OriginalPacketBuffer.size();
        RemedyHeader->m_AckAddress = (laddr)(&(p_Session->m_AckList[c_AckIndex]));
#ifdef ENVIRONMENT32
        RemedyHeader->m_Reserved = 0;
#endif
        RemedyHeader->m_Flags = Header::Data::FLAGS_END_OF_BLK;
        RemedyHeader->m_TxCount = m_TransmissionCount++;
        for(u16 CodingOffset = Header::Data::OffSets::CodingOffset ;
            CodingOffset < m_LargestOriginalPacketSize ;
            CodingOffset++)
        {
            m_RemedyPacketBuffer[CodingOffset] = 0;
            for(u08 PacketIndex = 0 ; PacketIndex < m_BlockSize ; PacketIndex++)
            {
                u08* OriginalBuffer = reinterpret_cast<u08*>(m_OriginalPacketBuffer[PacketIndex].get());
                m_RemedyPacketBuffer[CodingOffset] ^= FiniteField::instance()->mul(OriginalBuffer[CodingOffset], RandomCoefficients[PacketIndex]);
            }
        }
        sendto(p_Session->c_Socket, m_RemedyPacketBuffer, RemedyHeader->m_TotalSize, 0, (sockaddr*)&RemoteAddress, sizeof(RemoteAddress));
    }
    p_Session->m_Timer.ScheduleTask(m_RetransmissionInterval, [this](){
        p_Session->m_TaskQueue.Enqueue([this](){
            Retransmission();
        }, (p_Session->m_MinBlockSequenceNumber == m_BlockSequenceNumber?TransmissionSession::MIDDLE_PRIORITY:TransmissionSession::LOW_PRIORITY));
    });
}

////////////////////////////////////////////////////////////
/////////////// TransmissionSession
/*OK*/
TransmissionSession::TransmissionSession(s32 Socket, u32 IPv4, u16 Port, Parameter::TRANSMISSION_MODE TransmissionMode, Parameter::BLOCK_SIZE BlockSize, u16 RetransmissionRedundancy, u16 RetransmissionInterval): c_Socket(Socket),c_IPv4(IPv4), c_Port(Port)
{
    m_TransmissionMode = TransmissionMode;
    m_BlockSize = BlockSize;
    m_RetransmissionRedundancy = RetransmissionRedundancy;
    m_RetransmissionInterval = RetransmissionInterval;
    m_MinBlockSequenceNumber = 0;
    m_MaxBlockSequenceNumber = 0;
    p_TransmissionBlock = nullptr;
    for(u32 i = 0 ; i < (Parameter::MAXIMUM_NUMBER_OF_CONCURRENT_RETRANSMISSION*2) ; i++)
    {
        m_AckList[i] = true;
    }
    m_ConcurrentRetransmissions = 0;
}


/*OK*/
TransmissionSession::~TransmissionSession()
{
    m_Timer.Stop();
    m_TaskQueue.Stop();
}

/*OK*/
void TransmissionSession::ChangeTransmissionMode(const Parameter::TRANSMISSION_MODE TransmissionMode)
{
    m_TaskQueue.Enqueue([this, TransmissionMode]()
    {
        m_TransmissionMode = TransmissionMode;
    });
}

/*OK*/
void TransmissionSession::ChangeBlockSize(const Parameter::BLOCK_SIZE BlockSize)
{
    m_TaskQueue.Enqueue([this, BlockSize]()
    {
        m_BlockSize = BlockSize;
    });
}

/*OK*/
void TransmissionSession::ChangeRetransmissionRedundancy(const u16 RetransmissionRedundancy)
{
    m_TaskQueue.Enqueue([this, RetransmissionRedundancy]()
    {
        m_RetransmissionRedundancy = RetransmissionRedundancy;
    });
}

/*OK*/
void TransmissionSession::ChangeRetransmissionInterval(const u16 RetransmissionInterval)
{
    m_TaskQueue.Enqueue([this, RetransmissionInterval]()
    {
        m_RetransmissionInterval = RetransmissionInterval;
    });
}

/*OK*/
void TransmissionSession::ChangeSessionParameter(const Parameter::TRANSMISSION_MODE TransmissionMode, const Parameter::BLOCK_SIZE BlockSize, const u16 RetransmissionRedundancy, const u16 RetransmissionInterval)
{
    m_TaskQueue.Enqueue([this, TransmissionMode, BlockSize, RetransmissionRedundancy, RetransmissionInterval]()
    {
        m_TransmissionMode = TransmissionMode;
        m_BlockSize = BlockSize;
        m_RetransmissionRedundancy = RetransmissionRedundancy;
        m_RetransmissionInterval = RetransmissionInterval;
    });
}

////////////////////////////////////////////////////////////
/////////////// Transmission
/* OK */
Transmission::Transmission(s32 Socket) : c_Socket(Socket){}

/* OK */
Transmission::~Transmission()
{
    m_Sessions.DoSomethingOnAllData([](TransmissionSession* &session ){delete session;});
    m_Sessions.Clear();
}

/* OK */
bool Transmission::Connect(u32 IPv4, u16 Port, u32 ConnectionTimeout, Parameter::TRANSMISSION_MODE TransmissionMode, Parameter::BLOCK_SIZE BlockSize, u16 RetransmissionRedundancy, u16 RetransmissionInterval)
{
    std::unique_lock< std::mutex > lock(m_Lock);

    const DataStructures::IPv4PortKey key = {IPv4, Port};
    TransmissionSession** const session = m_Sessions.GetPtr(key);
    TransmissionSession* newsession = nullptr;
    if(session != nullptr)
    {
        newsession = (*session);
    }
    else
    {
        try
        {
            newsession = new TransmissionSession(c_Socket, IPv4, Port, TransmissionMode, BlockSize, RetransmissionRedundancy, RetransmissionInterval);
        }
        catch(const std::bad_alloc& ex)
        {
            std::cout<<ex.what();
            return false;
        }
        if(m_Sessions.Insert(key, newsession) == false)
        {
            return false;
        }
        if(newsession->m_BlockSize == Parameter::BLOCK_SIZE::INVALID_BLOCK_SIZE)
        {
            delete newsession;
            m_Sessions.Remove(key);
            return false;
        }
    }
    Header::Sync SyncPacket;
    SyncPacket.m_Type = Header::Common::HeaderType::SYNC;
    SyncPacket.m_Sequence = newsession->m_MaxBlockSequenceNumber;
    SyncPacket.m_SessionAddress = (laddr)newsession;
#ifdef ENVIRONMENT32
    SyncPacket.m_Reserved = 0;
#endif

    sockaddr_in RemoteAddress = {0};
    RemoteAddress.sin_family = AF_INET;
    RemoteAddress.sin_addr.s_addr = newsession->c_IPv4;
    RemoteAddress.sin_port = newsession->c_Port;
    if(sendto(c_Socket, (u08*)&SyncPacket, sizeof(SyncPacket), 0, (sockaddr*)&RemoteAddress, sizeof(RemoteAddress)) != sizeof(SyncPacket))
    {
        return false;
    }
    while(!newsession->m_IsConnected && ConnectionTimeout > 0)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        ConnectionTimeout--;
    }
    return ConnectionTimeout > 0;
}

/* OK */
bool Transmission::Send(u32 IPv4, u16 Port, u08* buffer, u16 buffersize, bool reqack)
{
    TransmissionSession* p_session = nullptr;
    {
        std::unique_lock< std::mutex > lock(m_Lock);
        const DataStructures::IPv4PortKey key = {IPv4, Port};
        TransmissionSession** const pp_session = m_Sessions.GetPtr(key);
        if(pp_session)
        {
            p_session = (*pp_session);
        }
    }
    if(p_session == nullptr)
    {
        return false;
    }
    if(p_session->m_IsConnected == false)
    {
        return false;
    }

    std::atomic<bool> TransmissionIsCompleted(false);
    std::atomic<bool> TransmissionResult(false);
    while(p_session->m_ConcurrentRetransmissions >= Parameter::MAXIMUM_NUMBER_OF_CONCURRENT_RETRANSMISSION);
    const bool TransmissionIsScheduled = p_session->m_TaskQueue.Enqueue([buffer, buffersize, reqack, p_session, &TransmissionIsCompleted, &TransmissionResult](){
        // 1. Get Transmission Block
        if(p_session->p_TransmissionBlock == nullptr)
        {
            try
            {
                p_session->p_TransmissionBlock = new TransmissionBlock(p_session);
            }
            catch(const std::bad_alloc& ex)
            {
                TransmissionResult = false;
                TransmissionIsCompleted = true;
                return;
            }
        }
        TransmissionBlock* const block = p_session->p_TransmissionBlock;
        if(block->Send(buffer, buffersize, reqack) == false)
        {
            TransmissionResult = false;
            TransmissionIsCompleted = true;
            return;
        }
        TransmissionResult = true;
        TransmissionIsCompleted = true;
    }, TransmissionSession::MIDDLE_PRIORITY);

    if(TransmissionIsScheduled == false)
    {
        TransmissionResult = false;
        TransmissionIsCompleted = true;
    }
    while(!TransmissionIsCompleted)
    {
        // Yield CPU resource.
        std::this_thread::sleep_for (std::chrono::milliseconds(0));
    }
    return TransmissionResult;
}

bool Transmission::Disconnect(u32 IPv4, u16 Port)
{
    std::unique_lock< std::mutex > lock(m_Lock);

    const DataStructures::IPv4PortKey key = {IPv4, Port};
    TransmissionSession** const pp_session = m_Sessions.GetPtr(key);
    if(pp_session == nullptr)
    {
        return false;
    }
    TransmissionSession* const p_session = (*pp_session);
    p_session->m_TaskQueue.Enqueue([p_session](){
        p_session->m_Timer.Stop();
        p_session->m_TaskQueue.Stop();
        p_session->m_IsConnected = false;
    }, TransmissionSession::HIGH_PRIORITY);
    return true;
}

/* OK */
void Transmission::RxHandler(u08* buffer, u16 size, const sockaddr_in * const sender_addr, const u32 sender_addr_len)
{
    Header::Common* CommonHeader = reinterpret_cast< Header::Common* >(buffer);
    switch(CommonHeader->m_Type)
    {
        case Header::Common::HeaderType::DATA_ACK:
        {
            const Header::DataAck* Ack = reinterpret_cast< Header::DataAck* >(buffer);
            const DataStructures::IPv4PortKey key = {sender_addr->sin_addr.s_addr, sender_addr->sin_port};
            std::unique_lock< std::mutex > lock(m_Lock);
            TransmissionSession** const pp_session = m_Sessions.GetPtr(key);
            if(pp_session)
            {
                std::atomic<bool>* const AckAddress = reinterpret_cast< std::atomic<bool>* >(Ack->m_AckAddress);
                (*pp_session)->m_TaskQueue.Enqueue([AckAddress](){
                    (*AckAddress) = true;
                }, TransmissionSession::HIGH_PRIORITY);
            }
        }
        break;

        case Header::Common::HeaderType::SYNC_ACK:
        {
            const Header::Sync* sync = reinterpret_cast< Header::Sync* >(buffer);
            TransmissionSession* const session = (TransmissionSession*)sync->m_SessionAddress;
            if(sync->m_Sequence == session->m_MaxBlockSequenceNumber)
            {
                session->m_IsConnected = true;
            }
        }
        break;
        default:
        break;
    }
}
