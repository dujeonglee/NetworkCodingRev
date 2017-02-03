#include "tx.h"

using namespace NetworkCoding;

////////////////////////////////////////////////////////////
/////////////// TransmissionBlock
/*
 * Create TransmissionBlock.
 */
TransmissionBlock::TransmissionBlock(TransmissionSession* const Session) : p_Session(Session){}

/*
 * Initialize Transmission Block.
 * 1. It copy session parameters.
 * 2. It increases m_MaxBlockSequenceNumber by 1 when intiailisation is successful.
 * 3. When we cannot allocate buffer space as much as p_Session->m_BlockSize,
 *    this function returns false and does not increase m_MaxBlockSequenceNumber.
 */
bool TransmissionBlock::Init()
{
    // 1. Acquire session lock.
    std::unique_lock< std::mutex > lock(p_Session->m_Lock);

    // 2. Copy session parameters.
    m_BlockSize = p_Session->m_BlockSize;
    m_TransmissionMode = p_Session->m_TransmissionMode;
    m_BlockSequenceNumber = p_Session->m_MaxBlockSequenceNumber++;
    p_Session->m_AckList[m_BlockSequenceNumber%(Parameter::MAX_CONCURRENCY*2)] = false;
    m_RetransmissionRedundancy = p_Session->m_RetransmissionRedundancy;
    m_RetransmissionInterval = p_Session->m_RetransmissionInterval;
    m_LargestOriginalPacketSize = 0;
    m_TransmissionCount = 0;

    // 3. Resize the packet buffer.
    if(m_OriginalPacketBuffer.size() != (size_t)m_BlockSize)
    {
        const u08 OldSize = (u08)m_OriginalPacketBuffer.size();
        try
        {
            m_OriginalPacketBuffer.resize((size_t)m_BlockSize);
        }
        catch(const std::bad_alloc& ex)
        {
            p_Session->m_BlockSize = OldSize;
            m_BlockSize = OldSize;
            m_OriginalPacketBuffer.resize(OldSize);
            std::cout<<ex.what();
        }
    }
    if(m_BlockSize != TransmissionSession::INVALID_BLOCK_SIZE)
    {
        // Even though memory allocation is failed, if buffer size is non-zero we can transmit packets.
        return true;
    }
    else
    {
        p_Session->m_AckList[m_BlockSequenceNumber%(Parameter::MAX_CONCURRENCY*2)] = true;
        p_Session->m_MaxBlockSequenceNumber--;
        return false;
    }
}

/*
 * Transmit a packet.

 */
u16 TransmissionBlock::Send(u32 IPv4, u16 Port, u08* buffer, u16 buffersize, bool reqack)
{
    std::unique_lock< std::mutex > lock(m_Lock);
    const u08 AckIndex = m_BlockSequenceNumber%(Parameter::MAX_CONCURRENCY*2);
    if(m_TransmissionCount < m_BlockSize)
    {
        // Original
        Header::Data* DataHeader = reinterpret_cast <Header::Data*>(m_OriginalPacketBuffer[m_TransmissionCount].buffer);
        DataHeader->m_Type = Header::Common::HeaderType::DATA;
        DataHeader->m_TotalSize = sizeof(Header::Data) + (m_BlockSize-1) + buffersize;
        DataHeader->m_MinBlockSequenceNumber = p_Session->m_MinBlockSequenceNumber.load();
        DataHeader->m_CurrentBlockSequenceNumber = m_BlockSequenceNumber;
        DataHeader->m_MaxBlockSequenceNumber = p_Session->m_MaxBlockSequenceNumber.load();
        DataHeader->m_ExpectedRank = m_TransmissionCount + 1;
        DataHeader->m_MaximumRank = m_BlockSize;
        DataHeader->m_AckAddress = (laddr)(&(p_Session->m_AckList[AckIndex]));
#ifdef ENVIRONMENT32
        DataHeader->m_Reserved = 0;
#endif
        DataHeader->m_Flags = Header::Data::FLAGS_ORIGINAL;
        if(m_TransmissionCount == (m_BlockSize - 1) || reqack)
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
        sendto(p_Session->c_Socket, m_OriginalPacketBuffer[m_TransmissionCount].buffer, DataHeader->m_TotalSize, 0, (sockaddr*)&RemoteAddress, sizeof(RemoteAddress));
        m_TransmissionCount++;
    }
    if(m_TransmissionCount == m_BlockSize || reqack)
    {
        // Start retransmission
        std::unique_lock< std::mutex > lock(p_Session->m_Lock);
        p_Session->m_RetransmissionThreadPool.enqueue([=](){Retransmission();});
        p_Session->m_CurrentTransmissionBlock = nullptr;
    }
    return buffersize;
}


void TransmissionBlock::Retransmission()
{
    const u08 AckIndex = m_BlockSequenceNumber%(Parameter::MAX_CONCURRENCY*2);

    sockaddr_in RemoteAddress = {0};
    RemoteAddress.sin_family = AF_INET;
    RemoteAddress.sin_addr.s_addr = p_Session->c_IPv4;
    RemoteAddress.sin_port = p_Session->c_Port;

    clock_t LastTransmissionTime = clock();
    for( u16 TxCnt = 0 ;
        (TxCnt < m_RetransmissionRedundancy) && (p_Session->m_AckList[AckIndex] == false) ;
        (p_Session->m_TransmissionMode == TransmissionSession::TRANSMISSION_MODE::RELIABLE_TRANSMISSION_MODE ? TxCnt = 0 : TxCnt++))
    {
        if(p_Session->m_IsConnected.load() == false)
        {
            p_Session->m_TransmissionBlockPool.push_back(this);
            p_Session->m_Condition.notify_one();
            return;
        }
        if((clock() - LastTransmissionTime) / (CLOCKS_PER_SEC) * 1000 > m_RetransmissionInterval)
        {
            if(m_TransmissionCount == 1)
            {
                Header::Data* DataHeader = reinterpret_cast<Header::Data*>(m_OriginalPacketBuffer[0].buffer);
                DataHeader->m_TotalSize = sizeof(Header::Data) + (m_BlockSize-1) + m_LargestOriginalPacketSize;
                DataHeader->m_MinBlockSequenceNumber = p_Session->m_MinBlockSequenceNumber.load();
                DataHeader->m_CurrentBlockSequenceNumber = m_BlockSequenceNumber;
                DataHeader->m_MaxBlockSequenceNumber = p_Session->m_MaxBlockSequenceNumber.load();
                DataHeader->m_ExpectedRank = m_BlockSize;
                DataHeader->m_MaximumRank = m_BlockSize;
                DataHeader->m_AckAddress = (laddr)(&(p_Session->m_AckList[AckIndex]));
#ifdef ENVIRONMENT32
                DataHeader->m_Reserved = 0;
#endif
                DataHeader->m_Flags = Header::Data::FLAGS_ORIGINAL | Header::Data::FLAGS_END_OF_BLK;
                sendto(p_Session->c_Socket, m_OriginalPacketBuffer[0].buffer, DataHeader->m_TotalSize, 0, (sockaddr*)&RemoteAddress, sizeof(RemoteAddress));
            }
            else
            {
                std::vector<u08> RandomCoefficients;
                Header::Data* RemedyHeader = reinterpret_cast<Header::Data*>(m_RemedyPacketBuffer.buffer);

                LastTransmissionTime = clock();
                for(u08 Coef = 0 ; Coef < m_BlockSize ; Coef++)
                {
                    RandomCoefficients.push_back((rand()%255)+1);
                }

                RandomCoefficients.resize(m_BlockSize, (rand()%255)+1);
                RemedyHeader->m_TotalSize = sizeof(Header::Data) + (m_BlockSize-1) + m_LargestOriginalPacketSize;
                RemedyHeader->m_MinBlockSequenceNumber = p_Session->m_MinBlockSequenceNumber.load();
                RemedyHeader->m_CurrentBlockSequenceNumber = m_BlockSequenceNumber;
                RemedyHeader->m_MaxBlockSequenceNumber = p_Session->m_MaxBlockSequenceNumber.load();
                RemedyHeader->m_ExpectedRank = m_BlockSize;
                RemedyHeader->m_MaximumRank = m_BlockSize;
                RemedyHeader->m_AckAddress = (laddr)(&(p_Session->m_AckList[AckIndex]));
#ifdef ENVIRONMENT32
                RemedyHeader->m_Reserved = 0;
#endif
                RemedyHeader->m_Flags = Header::Data::FLAGS_END_OF_BLK;

                u08* RemedyBuffer = reinterpret_cast<u08*>(m_RemedyPacketBuffer.buffer);
                for(unsigned char CodingOffset = Header::Data::CODING_OFFSET ; CodingOffset < m_LargestOriginalPacketSize ; CodingOffset++)
                {
                    RemedyBuffer[CodingOffset] = 0;
                    for(unsigned char PacketIndex = 0 ; PacketIndex < m_BlockSize ; PacketIndex++)
                    {
                        u08* OriginalBuffer = reinterpret_cast<u08*>(m_OriginalPacketBuffer[PacketIndex].buffer);
                        RemedyBuffer[CodingOffset] ^= FiniteField::instance()->mul(OriginalBuffer[CodingOffset], RandomCoefficients[PacketIndex]);
                    }
                }
                sendto(p_Session->c_Socket, m_RemedyPacketBuffer.buffer, RemedyHeader->m_TotalSize, 0, (sockaddr*)&RemoteAddress, sizeof(RemoteAddress));
            }
            LastTransmissionTime = clock();
        }
    }

    {
        std::unique_lock<std::mutex> lock(p_Session->m_Lock);
        // Update minimum sequence number that awaiting the ack from client.
        // Caution: Overflow should be dealt carefully.
        for(u16 i = p_Session->m_MinBlockSequenceNumber ; i != p_Session->m_MaxBlockSequenceNumber ; i++)
        {
            if(p_Session->m_AckList[i%(Parameter::MAX_CONCURRENCY*2)] == true)
            {
                p_Session->m_MinBlockSequenceNumber++;
            }
            else
            {
                break;
            }
        }
        p_Session->m_TransmissionBlockPool.push_back(this);
        p_Session->m_Condition.notify_one();
    }
}

////////////////////////////////////////////////////////////
/////////////// TransmissionSession
/*OK*/
TransmissionSession::TransmissionSession(s32 Socket, u32 IPv4, u16 Port, u08 Concurrency, TRANSMISSION_MODE TransmissionMode, BLOCK_SIZE BlockSize, u16 RetransmissionRedundancy, u16 RetransmissionInterval): c_Socket(Socket),c_IPv4(IPv4), c_Port(Port)
{
    m_MinBlockSequenceNumber = 0;
    m_MaxBlockSequenceNumber = 0;
    m_CurrentTransmissionBlock = nullptr;
    for(u32 i = 0 ; i < (Parameter::MAX_CONCURRENCY*2) ; i++)
    {
        m_AckList[i] = true;
    }
    m_RetransmissionThreadPool.resize(Parameter::MAX_CONCURRENCY);
    ChangeSessionParameter(Concurrency, TransmissionMode, BlockSize, RetransmissionRedundancy, RetransmissionInterval);
}


/*OK*/
TransmissionSession::~TransmissionSession()
{
    m_RetransmissionThreadPool.destroy();
    for(size_t i = 0 ; i < m_TransmissionBlockPool.size() ; i++)
    {
        delete m_TransmissionBlockPool[i];
    }
    m_TransmissionBlockPool.clear();
}

/*OK*/
void TransmissionSession::ChangeConcurrency(const u08 NewConcurrency)
{
    std::unique_lock< std::mutex > lock(m_Lock);
    const u08 OldConcurrency = m_Concurrency;
    if(NewConcurrency == 0 || NewConcurrency == m_Concurrency || NewConcurrency > Parameter::MAX_CONCURRENCY)
    {
        return;
    }
    if(NewConcurrency < OldConcurrency)
    {
        for(u08 i = NewConcurrency ; i < OldConcurrency ; i++)
        {
            delete m_TransmissionBlockPool[i];
        }
        m_TransmissionBlockPool.resize(NewConcurrency);
    }
    else
    {
        try
        {
            m_TransmissionBlockPool.resize(NewConcurrency, nullptr);
        }
        catch(const std::bad_alloc& ex)
        {
            std::cout<<ex.what();
        }
        for(u08 i = OldConcurrency-1 ; i < NewConcurrency ; i++)
        {
            try
            {
                m_TransmissionBlockPool[i] = new TransmissionBlock(this);
            }
            catch (const std::bad_alloc& ex)
            {
                m_Concurrency = i+1;
                std::cout<<ex.what();
            }
        }
    }
    m_Concurrency = NewConcurrency;
}

/*OK*/
void TransmissionSession::ChangeTransmissionMode(const TRANSMISSION_MODE TransmissionMode)
{
    std::unique_lock< std::mutex > lock(m_Lock);
    m_TransmissionMode = TransmissionMode;
}

/*OK*/
void TransmissionSession::ChangeBlockSize(const BLOCK_SIZE BlockSize)
{
    std::unique_lock< std::mutex > lock(m_Lock);
    m_BlockSize = BlockSize;
}

/*OK*/
void TransmissionSession::ChangeRetransmissionRedundancy(const u16 RetransmissionRedundancy)
{
    std::unique_lock< std::mutex > lock(m_Lock);
    m_RetransmissionRedundancy = RetransmissionRedundancy;
}

/*OK*/
void TransmissionSession::ChangeRetransmissionInterval(const u16 RetransmissionInterval)
{
    std::unique_lock< std::mutex > lock(m_Lock);
    m_RetransmissionInterval = RetransmissionInterval;
}

/*OK*/
void TransmissionSession::ChangeSessionParameter(const u08 Concurrency, const TRANSMISSION_MODE TransmissionMode, const BLOCK_SIZE BlockSize, const u16 RetransmissionRedundancy, const u16 RetransmissionInterval)
{
    ChangeConcurrency(Concurrency);
    ChangeTransmissionMode(TransmissionMode);
    ChangeBlockSize(BlockSize);
    ChangeRetransmissionRedundancy(RetransmissionRedundancy);
    ChangeRetransmissionInterval(RetransmissionInterval);
}

////////////////////////////////////////////////////////////
/////////////// Transmission
Transmission::Transmission(s32 Socket) : c_Socket(Socket){}

Transmission::~Transmission()
{
    m_Sessions.perform_for_all_data([](TransmissionSession*& session ){delete session;});
    m_Sessions.clear();
}

bool Transmission::Connect(u32 IPv4, u16 Port, u08 Concurrency, TransmissionSession::TRANSMISSION_MODE TransmissionMode, TransmissionSession::BLOCK_SIZE BlockSize, u16 RetransmissionRedundancy, u16 RetransmissionInterval)
{
    std::unique_lock< std::mutex > lock(m_Lock);

    const DataStructures::IPv4PortKey key = {IPv4, Port};
    TransmissionSession** session = m_Sessions.find(key);
    TransmissionSession* newsession = nullptr;
    if(session != nullptr)
    {
        newsession = (*session);
    }
    else
    {
        try
        {
            newsession = new TransmissionSession(c_Socket, IPv4, Port, Concurrency, TransmissionMode, BlockSize, RetransmissionRedundancy, RetransmissionInterval);
        }
        catch(const std::bad_alloc& ex)
        {
            std::cout<<ex.what();
            return false;
        }
        try
        {
            m_Sessions.insert(key, newsession);
        }
        catch(const std::bad_alloc& ex)
        {
            delete newsession;
            std::cout<<ex.what();
            return false;
        }
        if(newsession->m_BlockSize == 0 || newsession->m_Concurrency == 0)
        {
            delete newsession;
            m_Sessions.remove(key);
            return false;
        }
    }
    {
        Header::Sync sync;
        sockaddr_in RemoteAddress = {0};

        newsession->m_MinBlockSequenceNumber = newsession->m_MaxBlockSequenceNumber.load();
        newsession->m_IsConnected = false;
        /* Connect function abort all on-going transmissions. Wait until all retransmission tasks are finished. */
        /* Receiver must purge all packets in the buffer when receiving SYNC message. */
        while(newsession->m_RetransmissionThreadPool.tasks() > 0);

        sync.m_Type = Header::Common::HeaderType::SYNC;
        sync.m_Sequence = newsession->m_MaxBlockSequenceNumber;
        sync.m_SessionAddress = (laddr)newsession;
#ifdef ENVIRONMENT32
        sync.m_Reserved = 0;
#endif
        RemoteAddress.sin_family = AF_INET;
        RemoteAddress.sin_addr.s_addr = IPv4;
        RemoteAddress.sin_port = Port;
        for(u08 i = 0 ; i < 3 ; i++)
        {
            sendto(newsession->c_Socket, (char*)&sync, sizeof(sync), 0, (sockaddr*)&RemoteAddress, sizeof(RemoteAddress));
            clock_t SentTime = clock();
            do
            {
                if(newsession->m_IsConnected == true)
                {
                    return true;
                }
            }while((clock() - SentTime)*1000/CLOCKS_PER_SEC < 500 /*500 ms*/);
        }
    }
    return false;
}

u16 Transmission::Send(u32 IPv4, u16 Port, u08* buffer, u16 buffersize, bool reqack)
{
    TransmissionSession** session = nullptr;
    TransmissionBlock* TxBlock = nullptr;
    {// 1. Find session with ip address and port
        std::unique_lock< std::mutex > lock(m_Lock);
        const DataStructures::IPv4PortKey key = {IPv4, Port};
        session = m_Sessions.find(key);
        if(session == nullptr)
        {
            return 0;
        }
        if((*session)->m_IsConnected == false)
        {
            return 0;
        }
    }

    {// 2. Get a transmission block from a block pool.
        std::unique_lock< std::mutex > lock((*session)->m_Lock);

        if((*session)->m_CurrentTransmissionBlock == nullptr)
        {
            while((*session)->m_TransmissionBlockPool.size() == 0)
            {
                (*session)->m_Condition.wait(lock);
            }
            (*session)->m_CurrentTransmissionBlock = (*session)->m_TransmissionBlockPool.back();
            (*session)->m_TransmissionBlockPool.pop_back();
            if((*session)->m_CurrentTransmissionBlock->Init() == false)
            {
                // Init is failed. One cannot send packet as not eneough buffer. Send is failed.
                (*session)->m_TransmissionBlockPool.push_back((*session)->m_CurrentTransmissionBlock);
                (*session)->m_CurrentTransmissionBlock = nullptr;
                return 0;
            }
        }
        TxBlock = (*session)->m_CurrentTransmissionBlock;
    }
    // TxBlock must not be nullptr;
    return TxBlock->Send(IPv4, Port, buffer, buffersize, reqack);
}


void Transmission::RxHandler(u08* buffer, u16 size, const sockaddr_in * const sender_addr, const u32 sender_addr_len)
{
    Header::Common* CommonHeader = reinterpret_cast< Header::Common* >(buffer);
    switch(CommonHeader->m_Type)
    {
        case Header::Common::HeaderType::DATA_ACK:
        {
            const Header::DataAck* Ack = reinterpret_cast< Header::DataAck* >(buffer);
            std::atomic<bool>* const AckAddress = reinterpret_cast< std::atomic<bool>* >(Ack->m_AckAddress);
            (*AckAddress) = true;
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
