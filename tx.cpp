#include "tx.h"
#include "encoding_decoding_macro.h"

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


const uint16_t TransmissionBlock::AckIndex()
{
    return m_BlockSequenceNumber%(Parameter::MAXIMUM_NUMBER_OF_CONCURRENT_RETRANSMISSION*2);
}

bool TransmissionBlock::Send(uint8_t* buffer, uint16_t buffersize)
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
        m_OriginalPacketBuffer.emplace_back(std::unique_ptr<uint8_t[]>(new uint8_t[sizeof(Header::Data) + (m_BlockSize - 1) + buffersize]));
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
    if(/*reqack || */DataHeader->m_ExpectedRank == m_BlockSize)
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
    for(uint8_t i = 0 ; i < m_BlockSize ; i++)
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
    sendto(p_Session->c_Socket, m_OriginalPacketBuffer[m_TransmissionCount++].get(), ntohs(DataHeader->m_TotalSize), 0, (sockaddr*)&p_Session->c_Addr.Addr, p_Session->c_Addr.AddrLength);

    if((m_TransmissionCount == m_BlockSize)/* || reqack == true*/)
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
    const uint8_t c_AckIndex = AckIndex();
    if(m_TransmissionMode == Parameter::BEST_EFFORT_TRANSMISSION_MODE)
    {
        if(m_TransmissionCount >= m_OriginalPacketBuffer.size() + m_RetransmissionRedundancy)
        {
            p_Session->m_AckList[c_AckIndex] = true;
        }
    }
    if(p_Session->m_AckList[c_AckIndex] == true)
    {
        for(uint16_t i = p_Session->m_MinBlockSequenceNumber ; i != p_Session->m_MaxBlockSequenceNumber ; i++)
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
        delete this;
        return;
    }
    {
        std::vector<uint8_t> RandomCoefficients;
        Header::Data* RemedyHeader = reinterpret_cast<Header::Data*>(m_RemedyPacketBuffer);
        if(m_OriginalPacketBuffer.size() == 1)
        {
            // No encoding if only one packet is in m_OriginalPacketBuffer.
            do
            {
                try
                {
                    TEST_EXCEPTION(std::bad_alloc());
                    RandomCoefficients.push_back(1);
                }
                catch(const std::bad_alloc& ex)
                {
                    EXCEPTION_PRINT;
                    std::this_thread::sleep_for(std::chrono::milliseconds(1));
                    continue;
                }
                break;
            }while(1);
        }
        else
        {
            for(uint8_t Coef = 0 ; Coef < m_OriginalPacketBuffer.size() ; Coef++)
            {
                do
                {
                    try
                    {
                        TEST_EXCEPTION(std::bad_alloc());
                        RandomCoefficients.push_back((rand()%255)+1);
                    }
                    catch(const std::bad_alloc& ex)
                    {
                        EXCEPTION_PRINT;
                        std::this_thread::sleep_for(std::chrono::milliseconds(1));
                        continue;
                    }
                    break;
                }while(1);
            }
        }

        memset(m_RemedyPacketBuffer, 0x0, sizeof(m_RemedyPacketBuffer));
        RemedyHeader->m_Type = Header::Common::HeaderType::DATA;
        RemedyHeader->m_TotalSize = htons(sizeof(Header::Data) + (m_BlockSize-1) + m_LargestOriginalPacketSize);
        RemedyHeader->m_MinBlockSequenceNumber = htons(p_Session->m_MinBlockSequenceNumber.load());
        RemedyHeader->m_CurrentBlockSequenceNumber = htons(m_BlockSequenceNumber);
        RemedyHeader->m_MaxBlockSequenceNumber = htons(p_Session->m_MaxBlockSequenceNumber.load());
        RemedyHeader->m_ExpectedRank = m_OriginalPacketBuffer.size();
        RemedyHeader->m_MaximumRank = m_BlockSize;
        RemedyHeader->m_Flags = Header::Data::FLAGS_END_OF_BLK;
        RemedyHeader->m_TxCount = ++m_TransmissionCount;
        for(uint8_t PacketIndex = 0 ; PacketIndex < m_OriginalPacketBuffer.size() ; PacketIndex++)
        {
            uint8_t* OriginalBuffer = reinterpret_cast<uint8_t*>(m_OriginalPacketBuffer[PacketIndex].get());
            Header::Data* OriginalHeader = reinterpret_cast<Header::Data*>(OriginalBuffer);
            const uint16_t length = ntohs(OriginalHeader->m_TotalSize);
#if 0
            for(uint16_t CodingOffset = Header::Data::OffSets::CodingOffset ; CodingOffset < length ; CodingOffset++)
            {
                m_RemedyPacketBuffer[CodingOffset] ^= FiniteField::instance()->mul(OriginalBuffer[CodingOffset], RandomCoefficients[PacketIndex]);
            }
#else
            uint16_t CodingOffset = Header::Data::OffSets::CodingOffset;
            while(CodingOffset < length)
            {
                /*if(length - CodingOffset > 1024)
                {
                    Encoding1024(m_RemedyPacketBuffer, OriginalBuffer, RandomCoefficients, CodingOffset, PacketIndex);
                }
                else if(length - CodingOffset > 512)
                {
                    Encoding512(m_RemedyPacketBuffer, OriginalBuffer, RandomCoefficients, CodingOffset, PacketIndex);
                }
                else if(length - CodingOffset > 256)
                {
                    Encoding256(m_RemedyPacketBuffer, OriginalBuffer, RandomCoefficients, CodingOffset, PacketIndex);
                }
                else */if(length - CodingOffset > 128)
                {
                    Encoding128(m_RemedyPacketBuffer, OriginalBuffer, RandomCoefficients, CodingOffset, PacketIndex);
                }
                else if(length - CodingOffset > 64)
                {
                    Encoding64(m_RemedyPacketBuffer, OriginalBuffer, RandomCoefficients, CodingOffset, PacketIndex);
                }
                else if(length - CodingOffset > 32)
                {
                    Encoding32(m_RemedyPacketBuffer, OriginalBuffer, RandomCoefficients, CodingOffset, PacketIndex);
                }
                else if(length - CodingOffset > 16)
                {
                    Encoding16(m_RemedyPacketBuffer, OriginalBuffer, RandomCoefficients, CodingOffset, PacketIndex);
                }
                else if(length - CodingOffset > 8)
                {
                    Encoding8(m_RemedyPacketBuffer, OriginalBuffer, RandomCoefficients, CodingOffset, PacketIndex);
                }
                else if(length - CodingOffset > 4)
                {
                    Encoding4(m_RemedyPacketBuffer, OriginalBuffer, RandomCoefficients, CodingOffset, PacketIndex);
                }
                else if(length - CodingOffset > 2)
                {
                    Encoding2(m_RemedyPacketBuffer, OriginalBuffer, RandomCoefficients, CodingOffset, PacketIndex);
                }
                else
                {
                    for(; CodingOffset < length ; CodingOffset++)
                    {
                        m_RemedyPacketBuffer[CodingOffset] ^= FiniteField::instance()->mul(OriginalBuffer[CodingOffset], RandomCoefficients[PacketIndex]);
                    }
                }
            }
#endif
        }
        sendto(p_Session->c_Socket, m_RemedyPacketBuffer, ntohs(RemedyHeader->m_TotalSize), 0, (sockaddr*)&p_Session->c_Addr.Addr, p_Session->c_Addr.AddrLength);
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
TransmissionSession::TransmissionSession(Transmission* const transmission, int32_t Socket, const DataStructures::AddressType Addr, Parameter::TRANSMISSION_MODE TransmissionMode, Parameter::BLOCK_SIZE BlockSize, uint16_t RetransmissionRedundancy): c_Transmission(transmission), c_Socket(Socket), c_Addr(Addr)
{
    m_TransmissionMode = TransmissionMode;
    m_BlockSize = BlockSize;
    m_RetransmissionRedundancy = RetransmissionRedundancy;
    m_RetransmissionInterval = Parameter::MINIMUM_RETRANSMISSION_INTERVAL;
    m_MinBlockSequenceNumber = 0;
    m_MaxBlockSequenceNumber = 0;
    p_TransmissionBlock = nullptr;
    for(uint32_t i = 0 ; i < (Parameter::MAXIMUM_NUMBER_OF_CONCURRENT_RETRANSMISSION*2) ; i++)
    {
        m_AckList[i] = true;
    }
    m_ConcurrentRetransmissions = 0;
    m_IsConnected = false;
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
void TransmissionSession::ChangeRetransmissionRedundancy(const uint16_t RetransmissionRedundancy)
{
    m_TaskQueue.Enqueue([this, RetransmissionRedundancy]()
    {
        m_RetransmissionRedundancy = RetransmissionRedundancy;
    });
}

/*OK*/
void TransmissionSession::ChangeSessionParameter(const Parameter::TRANSMISSION_MODE TransmissionMode, const Parameter::BLOCK_SIZE BlockSize, const uint16_t RetransmissionRedundancy)
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
            //self->c_Transmission->Disconnect(self->c_IPv4, self->c_Port);
        });
        DisconnectThread.detach();
        return;
    }

    sendto(c_Socket, reinterpret_cast<uint8_t*>(&ping), sizeof(Header::Ping), 0, (sockaddr*)&c_Addr.Addr, c_Addr.AddrLength);
    while(m_IsConnected && m_Timer.ScheduleTask(Parameter::PING_INTERVAL, [this](){
        while(m_IsConnected && m_TaskQueue.Enqueue([this](){
            SendPing();
        }, TransmissionSession::HIGH_PRIORITY)==false);
    })==false);
}

void TransmissionSession::UpdateRetransmissionInterval(const uint16_t rtt)
{
    m_TaskQueue.Enqueue([this, rtt]()
    {
        m_RetransmissionInterval = (m_RetransmissionInterval + rtt)/2;
    });
}

////////////////////////////////////////////////////////////
/////////////// Transmission
/* OK */
Transmission::Transmission(int32_t Socket) : c_Socket(Socket){}

/* OK */
Transmission::~Transmission()
{
    m_Sessions.DoSomethingOnAllData([](TransmissionSession* &session ){delete session;});
    m_Sessions.Clear();
}

/* OK */
bool Transmission::Connect(const DataStructures::AddressType Addr, uint32_t ConnectionTimeout, Parameter::TRANSMISSION_MODE TransmissionMode, Parameter::BLOCK_SIZE BlockSize, uint16_t RetransmissionRedundancy)
{
    TransmissionSession* newsession = nullptr;
    {
        std::unique_lock< std::mutex > lock(m_Lock);
        const DataStructures::SessionKey key = DataStructures::GetSessionKey((sockaddr*)&Addr.Addr, Addr.AddrLength);
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
                newsession = new TransmissionSession(this, c_Socket, Addr, TransmissionMode, BlockSize, RetransmissionRedundancy);
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

    if(sendto(c_Socket, (uint8_t*)&SyncPacket, sizeof(SyncPacket), 0, (sockaddr*)(&newsession->c_Addr.Addr), newsession->c_Addr.AddrLength) != sizeof(SyncPacket))
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
bool Transmission::Send(const DataStructures::AddressType Addr, uint8_t* buffer, uint16_t buffersize)
{
    TransmissionSession* p_session = nullptr;
    {
        std::unique_lock< std::mutex > lock(m_Lock);
        const DataStructures::SessionKey key = DataStructures::GetSessionKey((sockaddr*)&Addr.Addr, Addr.AddrLength);
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
    while(p_session->m_ConcurrentRetransmissions >= Parameter::MAXIMUM_NUMBER_OF_CONCURRENT_RETRANSMISSION ||
          (uint16_t)(p_session->m_MaxBlockSequenceNumber - p_session->m_MinBlockSequenceNumber) >= (uint16_t)Parameter::MAXIMUM_NUMBER_OF_CONCURRENT_RETRANSMISSION)
    {
        if(!p_session->m_IsConnected)
        {
            return false;
        }
        std::this_thread::sleep_for (std::chrono::milliseconds(1));
    }
    const bool TransmissionIsScheduled = p_session->m_TaskQueue.Enqueue([buffer, buffersize/*, reqack*/, p_session, &TransmissionIsCompleted, &TransmissionResult](){
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
        if(block->Send(buffer, buffersize/*, reqack*/) == false)
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

bool Transmission::Flush(const DataStructures::AddressType Addr)
{
    TransmissionSession* p_session = nullptr;
    {
        std::unique_lock< std::mutex > lock(m_Lock);
        const DataStructures::SessionKey key = DataStructures::GetSessionKey((sockaddr*)&Addr.Addr, Addr.AddrLength);
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
    return p_session->m_TaskQueue.Enqueue([p_session](){
        // 1. Get Transmission Block
        if(p_session->p_TransmissionBlock)
        {
            p_session->p_TransmissionBlock->Retransmission();
            p_session->p_TransmissionBlock = nullptr;
        }
    }, TransmissionSession::LOW_PRIORITY);
}

void Transmission::WaitUntilTxIsCompleted(const DataStructures::AddressType Addr)
{
    TransmissionSession* p_session = nullptr;
    {
        std::unique_lock< std::mutex > lock(m_Lock);
        const DataStructures::SessionKey key = DataStructures::GetSessionKey((sockaddr*)&Addr.Addr, Addr.AddrLength);
        TransmissionSession** const pp_session = m_Sessions.GetPtr(key);
        if(pp_session)
        {
            p_session = (*pp_session);
        }
    }
    if(p_session == nullptr)
    {
        return;
    }
    if(p_session->m_IsConnected == false)
    {
        return;
    }
	while(p_session->m_ConcurrentRetransmissions > 0)
	{
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}
}

bool Transmission::Disconnect(const DataStructures::AddressType Addr)
{
    const DataStructures::SessionKey key = DataStructures::GetSessionKey((sockaddr*)&Addr.Addr, Addr.AddrLength);
    TransmissionSession** pp_session = nullptr;
    {
        std::unique_lock< std::mutex > lock(m_Lock);

        pp_session = m_Sessions.GetPtr(key);
        if(pp_session == nullptr)
        {
            return false;
        }
    }
    do
    {
        (*pp_session)->m_IsConnected = false;
        for(auto i = 0 ; i < Parameter::MAXIMUM_NUMBER_OF_CONCURRENT_RETRANSMISSION*2 ; i++)
        {
            (*pp_session)->m_AckList[i] = true;
        }
    }while(0 < (*pp_session)->m_ConcurrentRetransmissions);
    m_Sessions.Remove(key, [](TransmissionSession*&session){
        session->m_Timer.Stop();
        session->m_TaskQueue.Stop();
        delete session;
    });
    return true;
}

/* OK */
void Transmission::RxHandler(uint8_t* buffer, uint16_t size, const sockaddr* const sender_addr, const uint32_t sender_addr_len)
{
    Header::Common* CommonHeader = reinterpret_cast< Header::Common* >(buffer);
    switch(CommonHeader->m_Type)
    {
        case Header::Common::HeaderType::DATA_ACK:
        {
            const Header::DataAck* Ack = reinterpret_cast< Header::DataAck* >(buffer);
            const DataStructures::SessionKey key = DataStructures::GetSessionKey(sender_addr, sender_addr_len);
            std::unique_lock< std::mutex > lock(m_Lock);
            TransmissionSession** const pp_session = m_Sessions.GetPtr(key);
            if(pp_session)
            {
                TransmissionSession* const SessionAddress = (*pp_session);
                uint16_t Sequence = ntohs(Ack->m_Sequence);
                uint8_t Loss = Ack->m_Losses;
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
            const DataStructures::SessionKey key = DataStructures::GetSessionKey(sender_addr, sender_addr_len);
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
            const DataStructures::SessionKey key = DataStructures::GetSessionKey(sender_addr, sender_addr_len);
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
                    (*pp_session)->UpdateRetransmissionInterval((uint16_t)(rtt.count()*1000.));
                }
            }
        }
        break;

        default:
        break;
    }
}
