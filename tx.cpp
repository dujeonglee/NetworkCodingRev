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
    m_RetransmissionRedundancy(p_session->m_RetransmissionRedundancy)
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
        delete this;
        return false;
    }
    // 2. Allocate memory for packet buffer.
    try
    {
#if ENABLE_CRITICAL_EXCEPTIONS
        TEST_EXCEPTION(std::bad_alloc());
#endif
        m_OriginalPacketBuffer.emplace_back(std::unique_ptr<u08[]>(new u08[sizeof(Header::Data) + (m_BlockSize - 1) + buffersize]));
    }
    catch(const std::bad_alloc& ex)
    {
        EXCEPTION_PRINT;
        return false;
    }

    // Fill up the fields
    Header::Data* const DataHeader = reinterpret_cast <Header::Data*>(m_OriginalPacketBuffer[m_TransmissionCount].get());
    DataHeader->m_Type = Header::Common::HeaderType::DATA;
    DataHeader->m_TotalSize = htons(sizeof(Header::Data) + (m_BlockSize - 1) + buffersize);
    DataHeader->m_MinBlockSequenceNumber = htons(p_Session->m_MinBlockSequenceNumber);
    DataHeader->m_CurrentBlockSequenceNumber = htons(m_BlockSequenceNumber);
    DataHeader->m_MaxBlockSequenceNumber = htons(p_Session->m_MaxBlockSequenceNumber);
    DataHeader->m_ExpectedRank = m_TransmissionCount + 1;
    DataHeader->m_MaximumRank = m_BlockSize;
    DataHeader->m_Flags = Header::Data::FLAGS_ORIGINAL;
    DataHeader->m_TxCount = m_TransmissionCount + 1;
    if(reqack || DataHeader->m_ExpectedRank == m_BlockSize)
    {
        //Note: Header::Data::FLAGS_END_OF_BLK asks ack from the client
        DataHeader->m_Flags |= Header::Data::FLAGS_END_OF_BLK;
    }
    DataHeader->m_PayloadSize = htons(buffersize);
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
    sendto(p_Session->c_Socket, m_OriginalPacketBuffer[m_TransmissionCount++].get(), ntohs(DataHeader->m_TotalSize), 0, (sockaddr*)&RemoteAddress, sizeof(RemoteAddress));

    if((m_TransmissionCount == m_BlockSize) || reqack == true)
    {
        p_Session->p_TransmissionBlock = nullptr;
        while(p_Session->m_IsConnected && p_Session->m_Timer.ScheduleTask(p_Session->m_RetransmissionInterval, [this](){
            const auto Priority = (p_Session->m_MinBlockSequenceNumber == m_BlockSequenceNumber?TransmissionSession::MIDDLE_PRIORITY:TransmissionSession::LOW_PRIORITY);
            while(p_Session->m_IsConnected && p_Session->m_TaskQueue.Enqueue([this](){
                Retransmission();
            }, Priority)==false);
        })==false);
    }
    return true;
}

void TransmissionBlock::Retransmission()
{
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
    if(p_Session->m_IsConnected == false)
    {
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
        DataHeader->m_MinBlockSequenceNumber = htons(p_Session->m_MinBlockSequenceNumber);
        /* Must be the same *///DataHeader->m_CurrentBlockSequenceNumber = m_BlockSequenceNumber;
        DataHeader->m_MaxBlockSequenceNumber = htons(p_Session->m_MaxBlockSequenceNumber);
        /* Must be the same *///DataHeader->m_ExpectedRank = m_BlockSize;
        /* Must be the same *///DataHeader->m_MaximumRank = m_BlockSize;
        DataHeader->m_Flags = Header::Data::FLAGS_ORIGINAL | Header::Data::FLAGS_END_OF_BLK;
        DataHeader->m_TxCount = ++m_TransmissionCount;
        sendto(p_Session->c_Socket, m_OriginalPacketBuffer[0].get(), ntohs(DataHeader->m_TotalSize), 0, (sockaddr*)&RemoteAddress, sizeof(RemoteAddress));
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
        RemedyHeader->m_TotalSize = htons(sizeof(Header::Data) + (m_BlockSize-1) + m_LargestOriginalPacketSize);
        RemedyHeader->m_MinBlockSequenceNumber = htons(p_Session->m_MinBlockSequenceNumber.load());
        RemedyHeader->m_CurrentBlockSequenceNumber = htons(m_BlockSequenceNumber);
        RemedyHeader->m_MaxBlockSequenceNumber = htons(p_Session->m_MaxBlockSequenceNumber.load());
        RemedyHeader->m_ExpectedRank = m_OriginalPacketBuffer.size();
        RemedyHeader->m_MaximumRank = m_BlockSize;
        RemedyHeader->m_Flags = Header::Data::FLAGS_END_OF_BLK;
        RemedyHeader->m_TxCount = ++m_TransmissionCount;
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
        sendto(p_Session->c_Socket, m_RemedyPacketBuffer, ntohs(RemedyHeader->m_TotalSize), 0, (sockaddr*)&RemoteAddress, sizeof(RemoteAddress));
    }
    while(p_Session->m_IsConnected && p_Session->m_Timer.ScheduleTask(p_Session->m_RetransmissionInterval, [this](){
        const auto Priority = (p_Session->m_MinBlockSequenceNumber == m_BlockSequenceNumber?TransmissionSession::MIDDLE_PRIORITY:TransmissionSession::LOW_PRIORITY);
        while(p_Session->m_IsConnected && p_Session->m_TaskQueue.Enqueue([this](){
            Retransmission();
        }, Priority)==false);
    })==false);
}

////////////////////////////////////////////////////////////
/////////////// TransmissionSession
/*OK*/
TransmissionSession::TransmissionSession(Transmission* const transmission, s32 Socket, u32 IPv4, u16 Port, Parameter::TRANSMISSION_MODE TransmissionMode, Parameter::BLOCK_SIZE BlockSize, u16 RetransmissionRedundancy): c_Transmission(transmission), c_Socket(Socket),c_IPv4(IPv4), c_Port(Port)
{
    m_TransmissionMode = TransmissionMode;
    m_BlockSize = BlockSize;
    m_RetransmissionRedundancy = RetransmissionRedundancy;
    m_RetransmissionInterval = Parameter::MINIMUM_RETRANSMISSION_INTERVAL;
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
void TransmissionSession::ChangeSessionParameter(const Parameter::TRANSMISSION_MODE TransmissionMode, const Parameter::BLOCK_SIZE BlockSize, const u16 RetransmissionRedundancy)
{
    m_TaskQueue.Enqueue([this, TransmissionMode, BlockSize, RetransmissionRedundancy]()
    {
        m_TransmissionMode = TransmissionMode;
        m_BlockSize = BlockSize;
        m_RetransmissionRedundancy = RetransmissionRedundancy;
    });
}

void TransmissionSession::SendPing()
{
    Header::Ping ping;
    ping.m_Type = Header::Data::HeaderType::PING;
    ping.m_PingTime = CLOCK::now().time_since_epoch().count();

    CLOCK::time_point const CurrentTime(CLOCK::duration(ping.m_PingTime));
    CLOCK::time_point const LastPongRecvTime(CLOCK::duration(m_LastPongTime.load()));
    std::chrono::duration<double> TimeSinceLastPongTime = std::chrono::duration_cast<std::chrono::duration<double>> (CurrentTime - LastPongRecvTime);
    if(TimeSinceLastPongTime.count() > Parameter::CONNECTION_TIMEOUT)
    {
        TransmissionSession const * self = this;
        std::cout<<"Pong Timeout...Client is disconnected.["<<TimeSinceLastPongTime.count()<<"]"<<std::endl;
        std::thread DisconnectThread = std::thread([self](){
            self->c_Transmission->Disconnect(self->c_IPv4, self->c_Port);
        });
        DisconnectThread.detach();
        return;
    }

    sockaddr_in RemoteAddress = {0};
    RemoteAddress.sin_family = AF_INET;
    RemoteAddress.sin_addr.s_addr = c_IPv4;
    RemoteAddress.sin_port = c_Port;
    sendto(c_Socket, reinterpret_cast<u08*>(&ping), sizeof(Header::Ping), 0, (sockaddr*)&RemoteAddress, sizeof(RemoteAddress));
    while(m_IsConnected && m_Timer.ScheduleTask(Parameter::PING_INTERVAL, [this](){
        while(m_IsConnected && m_TaskQueue.Enqueue([this](){
            SendPing();
        }, TransmissionSession::HIGH_PRIORITY)==false);
    })==false);
}

void TransmissionSession::UpdateRetransmissionInterval(const u16 rtt)
{
    m_TaskQueue.Enqueue([this, rtt]()
    {
        m_RetransmissionInterval = (m_RetransmissionInterval + rtt)/2;
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
bool Transmission::Connect(u32 IPv4, u16 Port, u32 ConnectionTimeout, Parameter::TRANSMISSION_MODE TransmissionMode, Parameter::BLOCK_SIZE BlockSize, u16 RetransmissionRedundancy)
{
    TransmissionSession* newsession = nullptr;
    {
        std::unique_lock< std::mutex > lock(m_Lock);
        const DataStructures::IPv4PortKey key = {IPv4, Port};
        TransmissionSession** const session = m_Sessions.GetPtr(key);
        if(session != nullptr)
        {
            newsession = (*session);
        }
        else
        {
            try
            {
                TEST_EXCEPTION(std::bad_alloc());
                newsession = new TransmissionSession(this, c_Socket, IPv4, Port, TransmissionMode, BlockSize, RetransmissionRedundancy);
            }
            catch(const std::bad_alloc& ex)
            {
                EXCEPTION_PRINT;
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
    }
    Header::Sync SyncPacket;
    SyncPacket.m_Type = Header::Common::HeaderType::SYNC;
    SyncPacket.m_Sequence = htons(newsession->m_MaxBlockSequenceNumber);

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
    if(ConnectionTimeout == 0)
    {
        return false;
    }
    newsession->m_LastPongTime = CLOCK::now().time_since_epoch().count();
    while(newsession->m_IsConnected && newsession->m_Timer.ScheduleTask(0, [newsession](){
        while(newsession->m_IsConnected && newsession->m_TaskQueue.Enqueue([newsession](){
            newsession->SendPing();
        }, TransmissionSession::HIGH_PRIORITY)==false);
    })==false);
    return true;
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
    while((u16)(p_session->m_MaxBlockSequenceNumber - p_session->m_MinBlockSequenceNumber) >= Parameter::MAXIMUM_NUMBER_OF_CONCURRENT_RETRANSMISSION)
    {
        if(!p_session->m_IsConnected)
        {
            return false;
        }
        std::this_thread::sleep_for (std::chrono::milliseconds(1));
    }
    const bool TransmissionIsScheduled = p_session->m_TaskQueue.Enqueue([buffer, buffersize, reqack, p_session, &TransmissionIsCompleted, &TransmissionResult](){
        // 1. Get Transmission Block
        if(p_session->p_TransmissionBlock == nullptr)
        {
            try
            {
#if ENABLE_CRITICAL_EXCEPTIONS
                TEST_EXCEPTION(std::bad_alloc());
#endif
                p_session->p_TransmissionBlock = new TransmissionBlock(p_session);
            }
            catch(const std::bad_alloc& ex)
            {
                EXCEPTION_PRINT;
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
    (*pp_session)->m_IsConnected = false;
    for(auto i = 0 ; i < Parameter::MAXIMUM_NUMBER_OF_CONCURRENT_RETRANSMISSION*2 ; i++)
    {
        (*pp_session)->m_AckList[i] = true;
    }
    m_Sessions.Remove(key, [](TransmissionSession*&session){
        session->m_Timer.Stop();
        session->m_TaskQueue.Stop();
        delete session;
    });
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
                TransmissionSession* const SessionAddress = (*pp_session);
                u16 Sequence = ntohs(Ack->m_Sequence);
                u08 Loss = Ack->m_Losses;
                (*pp_session)->m_TaskQueue.Enqueue([SessionAddress,Sequence,Loss](){
                    if(SessionAddress->m_AckList[Sequence%(Parameter::MAXIMUM_NUMBER_OF_CONCURRENT_RETRANSMISSION*2)] == true)
                    {
                        return;
                    }
                    SessionAddress->m_AckList[Sequence%(Parameter::MAXIMUM_NUMBER_OF_CONCURRENT_RETRANSMISSION*2)] = true;
                    /* To-Do */
                    // Update the Avg. loss rate.
                }, TransmissionSession::HIGH_PRIORITY);
            }
        }
        break;

        case Header::Common::HeaderType::SYNC_ACK:
        {
            const Header::Sync* sync = reinterpret_cast< Header::Sync* >(buffer);
            const DataStructures::IPv4PortKey key = {sender_addr->sin_addr.s_addr, sender_addr->sin_port};
            std::unique_lock< std::mutex > lock(m_Lock);
            TransmissionSession** const pp_session = m_Sessions.GetPtr(key);
            if(pp_session)
            {
                if(ntohs(sync->m_Sequence) == (*pp_session)->m_MaxBlockSequenceNumber)
                {
                    (*pp_session)->m_IsConnected = true;
                }
            }
        }
        break;

        case Header::Common::PONG:
        {
            const Header::Pong* pong = reinterpret_cast< Header::Pong* >(buffer);
            const DataStructures::IPv4PortKey key = {sender_addr->sin_addr.s_addr, sender_addr->sin_port};
            std::unique_lock< std::mutex > lock(m_Lock);
            TransmissionSession** const pp_session = m_Sessions.GetPtr(key);
            if(pp_session)
            {
                CLOCK::time_point const SentTime(CLOCK::duration(pong->m_PingTime));
                (*pp_session)->m_LastPongTime = CLOCK::now().time_since_epoch().count();
                CLOCK::time_point const RecvTime(CLOCK::duration((*pp_session)->m_LastPongTime));
                std::chrono::duration<double> rtt = std::chrono::duration_cast<std::chrono::duration<double>>(RecvTime - SentTime);
                if(rtt.count() * 1000 < Parameter::MINIMUM_RETRANSMISSION_INTERVAL)
                {
                    (*pp_session)->UpdateRetransmissionInterval(Parameter::MINIMUM_RETRANSMISSION_INTERVAL);
                }
                else
                {
                    (*pp_session)->UpdateRetransmissionInterval((u16)(rtt.count()*1000.));
                }
            }
        }
        break;

        default:
        break;
    }
}
