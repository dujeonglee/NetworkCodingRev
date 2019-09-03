#include "tx.h"
#include "chk.h"
#include "encoding_decoding_macro.h"

using namespace NetworkCoding;

TransmissionBlock::TransmissionBlock(TransmissionSession *const p_session) : p_Session(p_session),
                                                                             m_BlockSize(p_session->m_BlockSize),
                                                                             m_TransmissionMode(p_session->m_TransmissionMode),
                                                                             m_BlockSequenceNumber(p_session->m_MaxBlockSequenceNumber.fetch_add(1, std::memory_order_relaxed)),
                                                                             m_RetransmissionRedundancy(p_session->m_RetransmissionRedundancy)
{
    /**
     * @brief 1. Initialize the member variables.
     */
    m_LargestOriginalPacketSize = 0;
    m_TransmissionCount = 0;
    m_AckedRank = 0;
    /**
     * @brief 2. Register the block sequence number to NetworkCoding::TransmissionSession::m_AckList.
     * @see NetworkCoding::TransmissionSession::m_AckList.
     */
    {
        std::unique_lock<std::mutex> acklock(p_Session->m_AckListLock);
        p_Session->m_AckList[(int32_t)m_BlockSequenceNumber] = 0;
    }
}

TransmissionBlock::~TransmissionBlock()
{
    /**
     * @brief 1. Update NetworkCoding::TransmissionSession::m_BytesUnacked.
     * @see NetworkCoding::TransmissionSession::m_BytesUnacked.
     */
    for (uint32_t i = 0; i < m_OriginalPacketBuffer.size(); i++)
    {
        Header::Data *const DataHeader = reinterpret_cast<Header::Data *>(m_OriginalPacketBuffer[i].get());
        if (p_Session->m_BytesUnacked >= ntohs(DataHeader->m_TotalSize))
        {
            p_Session->m_BytesUnacked.fetch_sub(ntohs(DataHeader->m_TotalSize));
        }
        else
        {
            std::cout << "CWND Error" << std::endl;
            p_Session->m_BytesUnacked = 0;
        }
    }
    /**
     * @brief 2. Clear NetworkCoding::TransmissionBlock::m_OriginalPacketBuffer.
     * @see NetworkCoding::TransmissionBlock::m_OriginalPacketBuffer.
     */
    m_OriginalPacketBuffer.clear();
}

bool TransmissionBlock::Send(uint8_t *const buffer, const uint16_t buffersize)
{
    /**
     * @brief 1. Check if session is in connected state.
     * @see NetworkCoding::TransmissionSession::m_IsConnected.
     */
    if (p_Session->m_IsConnected == false)
    {
        delete this;
        return false;
    }
    /**
     * @brief 2. Put buffer into the m_OriginalPacketBuffer.
     * @see NetworkCoding::TransmissionBlock::m_OriginalPacketBuffer.
     */
    try
    {
#if ENABLE_CRITICAL_EXCEPTIONS
        TEST_EXCEPTION(std::bad_alloc());
#endif
        m_OriginalPacketBuffer.emplace_back(std::unique_ptr<uint8_t[]>(new uint8_t[sizeof(Header::Data) + (m_BlockSize - 1) + buffersize]));
    }
    catch (const std::bad_alloc &ex)
    {
        EXCEPTION_PRINT;
        return false;
    }

    /**
     * @brief 3. Prepare header.
     * @see NetworkCoding::Header::Data.
     */
    Header::Data *const DataHeader = reinterpret_cast<Header::Data *>(m_OriginalPacketBuffer[m_TransmissionCount].get());
    DataHeader->m_Type = Header::Common::HeaderType::DATA;
    DataHeader->m_CheckSum = 0;
    DataHeader->m_TotalSize = htons(sizeof(Header::Data) + (m_BlockSize - 1) + buffersize);
    DataHeader->m_MinBlockSequenceNumber = htons(p_Session->m_MinBlockSequenceNumber);
    DataHeader->m_CurrentBlockSequenceNumber = htons(m_BlockSequenceNumber);
    DataHeader->m_MaxBlockSequenceNumber = htons(p_Session->m_MaxBlockSequenceNumber);
    DataHeader->m_ExpectedRank = m_TransmissionCount + 1;
    DataHeader->m_MaximumRank = m_BlockSize;
    DataHeader->m_Flags = Header::Data::FLAGS_ORIGINAL;
    DataHeader->m_TxCount = m_TransmissionCount + 1;
    if (DataHeader->m_ExpectedRank == m_BlockSize)
    {
        DataHeader->m_Flags |= Header::Data::FLAGS_END_OF_BLK;
    }
    DataHeader->m_PayloadSize = htons(buffersize);
    if (m_LargestOriginalPacketSize < buffersize)
    {
        m_LargestOriginalPacketSize = buffersize;
    }
    DataHeader->m_LastIndicator = 1;
    for (uint8_t i = 0; i < m_BlockSize; i++)
    {
        if (i != m_TransmissionCount)
        {
            DataHeader->m_Codes[i] = 0;
        }
        else
        {
            DataHeader->m_Codes[i] = 1;
        }
    }
    memcpy(DataHeader->m_Codes + m_BlockSize, buffer, buffersize);
    DataHeader->m_CheckSum = Checksum::get(m_OriginalPacketBuffer[m_TransmissionCount].get(), ntohs(DataHeader->m_TotalSize));
    /**
     * @brief 4. Enqueue the packet.
     * @see NetworkCoding::TransmissionSession::PushDataPacket.
     */
    p_Session->PushDataPacket(m_OriginalPacketBuffer[m_TransmissionCount++].get(), ntohs(DataHeader->m_TotalSize), true, this);
    /**
     * @brief 5. Clear NetworkCoding::TransmissionSession::p_TransmissionBlock so that NetworkCoding::TransmissionSession can arrange next block for transmission.
     * @see NetworkCoding::TransmissionSession::p_TransmissionBlock.
     */
    if (m_TransmissionCount == m_BlockSize)
    {
        p_Session->p_TransmissionBlock = nullptr;
    }
    return true;
}

const bool TransmissionBlock::Retransmission()
{
    /**
     * @brief 1. If NetworkCoding::TransmissionBlock::m_TransmissionMode is NetworkCoding::Parameter::BEST_EFFORT_TRANSMISSION_MODE and
     * we already transmitted NetworkCoding::TransmissionBlock::m_RetransmissionRedundancy remedy packets then
     * we complete this block regardless of ack from receiver.
     */
    if (m_TransmissionMode == Parameter::BEST_EFFORT_TRANSMISSION_MODE)
    {
        if (m_TransmissionCount >= m_OriginalPacketBuffer.size() + m_RetransmissionRedundancy)
        {
            std::unique_lock<std::mutex> acklock(p_Session->m_AckListLock);
            p_Session->m_AckList.erase(m_BlockSequenceNumber);
        }
    }
    /**
     * @brief 2. Update NetworkCoding::TransmissionSession::m_MinBlockSequenceNumber with
     * the minimum sequence number in NetworkCoding::TransmissionSession::m_AckList.
     * @warning ~TransmissionBlock require m_AckListLock.
     * Therefore, we must release m_AckListLock before calling dtor.
     */
    {
        std::unique_lock<std::mutex> acklock(p_Session->m_AckListLock);
        if (p_Session->m_AckList.find(m_BlockSequenceNumber) == p_Session->m_AckList.end())
        {
            for (uint16_t i = p_Session->m_MinBlockSequenceNumber; i != p_Session->m_MaxBlockSequenceNumber; i++)
            {
                if (p_Session->m_AckList.find(i) == p_Session->m_AckList.end())
                {
                    p_Session->m_MinBlockSequenceNumber++;
                }
                else
                {
                    break;
                }
            }
            acklock.unlock();
            delete this;
            return false;
        }
    }
    /**
     * @brief 3. Check if session is in connected state.
     */
    if (p_Session->m_IsConnected == false)
    {
        delete this;
        return false;
    }
    {
        /**
         * @brief 4. Generate random coefficients.
         * If m_OriginalPacketBuffer.size() == 1 we do not encode the packet.
         * Otherwise, we generates m_OriginalPacketBuffer.size() many random coefficients.
         */
        std::vector<uint8_t> RandomCoefficients;
        Header::Data *RemedyHeader = reinterpret_cast<Header::Data *>(m_RemedyPacketBuffer);
        if (m_OriginalPacketBuffer.size() == 1)
        {
            do
            {
                try
                {
                    TEST_EXCEPTION(std::bad_alloc());
                    RandomCoefficients.push_back(1);
                }
                catch (const std::bad_alloc &ex)
                {
                    EXCEPTION_PRINT;
                    std::this_thread::sleep_for(std::chrono::milliseconds(1));
                    continue;
                }
                break;
            } while (1);
        }
        else
        {
            for (uint8_t Coef = 0; Coef < m_OriginalPacketBuffer.size(); Coef++)
            {
                do
                {
                    try
                    {
                        TEST_EXCEPTION(std::bad_alloc());
                        RandomCoefficients.push_back((rand() % 255) + 1);
                    }
                    catch (const std::bad_alloc &ex)
                    {
                        EXCEPTION_PRINT;
                        std::this_thread::sleep_for(std::chrono::milliseconds(1));
                        continue;
                    }
                    break;
                } while (1);
            }
        }

        /**
         * @brief 5. Prepare header.
         * @see NetworkCoding::Header::Data.
         */
        memset(m_RemedyPacketBuffer, 0x0, sizeof(m_RemedyPacketBuffer));
        RemedyHeader->m_Type = Header::Common::HeaderType::DATA;
        RemedyHeader->m_CheckSum = 0;
        RemedyHeader->m_TotalSize = htons(sizeof(Header::Data) + (m_BlockSize - 1) + m_LargestOriginalPacketSize);
        RemedyHeader->m_MinBlockSequenceNumber = htons(p_Session->m_MinBlockSequenceNumber.load());
        RemedyHeader->m_CurrentBlockSequenceNumber = htons(m_BlockSequenceNumber);
        RemedyHeader->m_MaxBlockSequenceNumber = htons(p_Session->m_MaxBlockSequenceNumber.load());
        RemedyHeader->m_ExpectedRank = m_OriginalPacketBuffer.size();
        RemedyHeader->m_MaximumRank = m_BlockSize;
        RemedyHeader->m_Flags = Header::Data::FLAGS_END_OF_BLK;
        RemedyHeader->m_TxCount = ++m_TransmissionCount;
        RemedyHeader->m_CheckSum = 0;
        /**
         * @brief 6. Encoding a remedy packet.
         */
        for (uint8_t PacketIndex = 0; PacketIndex < m_OriginalPacketBuffer.size(); PacketIndex++)
        {
            uint8_t *OriginalBuffer = reinterpret_cast<uint8_t *>(m_OriginalPacketBuffer[PacketIndex].get());
            Header::Data *OriginalHeader = reinterpret_cast<Header::Data *>(OriginalBuffer);
            const uint16_t length = ntohs(OriginalHeader->m_TotalSize);
            uint16_t CodingOffset = Header::Data::OffSets::CodingOffset;
            while (CodingOffset < length)
            {
                if (length - CodingOffset > 512)
                {
                    EncodingPacket<512>::Run(m_RemedyPacketBuffer, OriginalBuffer, RandomCoefficients, CodingOffset, PacketIndex);
                }
                else if (length - CodingOffset > 256)
                {
                    EncodingPacket<256>::Run(m_RemedyPacketBuffer, OriginalBuffer, RandomCoefficients, CodingOffset, PacketIndex);
                }
                else if (length - CodingOffset > 128)
                {
                    EncodingPacket<128>::Run(m_RemedyPacketBuffer, OriginalBuffer, RandomCoefficients, CodingOffset, PacketIndex);
                }
                else if (length - CodingOffset > 64)
                {
                    EncodingPacket<64>::Run(m_RemedyPacketBuffer, OriginalBuffer, RandomCoefficients, CodingOffset, PacketIndex);
                }
                else if (length - CodingOffset > 32)
                {
                    EncodingPacket<32>::Run(m_RemedyPacketBuffer, OriginalBuffer, RandomCoefficients, CodingOffset, PacketIndex);
                }
                else if (length - CodingOffset > 16)
                {
                    EncodingPacket<16>::Run(m_RemedyPacketBuffer, OriginalBuffer, RandomCoefficients, CodingOffset, PacketIndex);
                }
                else if (length - CodingOffset > 8)
                {
                    EncodingPacket<8>::Run(m_RemedyPacketBuffer, OriginalBuffer, RandomCoefficients, CodingOffset, PacketIndex);
                }
                else if (length - CodingOffset > 4)
                {
                    EncodingPacket<4>::Run(m_RemedyPacketBuffer, OriginalBuffer, RandomCoefficients, CodingOffset, PacketIndex);
                }
                else if (length - CodingOffset > 2)
                {
                    EncodingPacket<2>::Run(m_RemedyPacketBuffer, OriginalBuffer, RandomCoefficients, CodingOffset, PacketIndex);
                }
                else
                {
                    for (; CodingOffset < length;)
                    {
                        EncodingPacket<1>::Run(m_RemedyPacketBuffer, OriginalBuffer, RandomCoefficients, CodingOffset, PacketIndex);
                    }
                }
            }
        }
        RemedyHeader->m_CheckSum = Checksum::get(m_RemedyPacketBuffer, ntohs(RemedyHeader->m_TotalSize));
        /**
         * @brief 7. Enqueue the remedy packet.
         * @see  NetworkCoding::TransmissionSession::PushDataPacket.
         */
        p_Session->PushDataPacket(m_RemedyPacketBuffer, ntohs(RemedyHeader->m_TotalSize), false, this);
    }
    return true;
}

TransmissionSession::TransmissionSession(Transmission *const transmission, const int32_t Socket, const DataTypes::Address Addr, const Parameter::TRANSMISSION_MODE TransmissionMode, const Parameter::BLOCK_SIZE BlockSize, const uint16_t RetransmissionRedundancy) : c_Transmission(transmission), c_Socket(Socket), c_Addr(Addr)
{
    /**
     * @brief Initialize all member variables.
     */
    m_TransmissionMode = TransmissionMode;
    m_BlockSize = BlockSize;
    m_RoundTripTime = Parameter::MINIMUM_RETRANSMISSION_INTERVAL;
    m_RetransmissionRedundancy = RetransmissionRedundancy;
    m_MinBlockSequenceNumber = 0;
    m_MaxBlockSequenceNumber = 0;
    p_TransmissionBlock = nullptr;
    m_BytesUnacked = 0;
    m_CongestionWindow = Parameter::MINIMUM_CONGESTION_WINDOW_SIZE;

    m_IsConnected = false;
}

TransmissionSession::~TransmissionSession()
{
    /**
     * @brief 1. Set m_IsConnected as disconnected, i.e. false.
     */
    m_IsConnected = false;
    /**
     * @brief 2. Stop m_Timer.
     */
    m_Timer.Stop();
    /**
     * @brief 3. Clear m_TxQueue.
     */
    while (m_TxQueue.size())
    {
        if (std::get<2>(m_TxQueue.front()) == false)
        {
            delete[] std::get<0>(m_TxQueue.front());
        }
        m_TxQueue.pop();
    }
}

void TransmissionSession::ChangeTransmissionMode(const Parameter::TRANSMISSION_MODE TransmissionMode)
{
    /**
     * @brief Session parameter updates are serialized using m_Timer.
     */
    m_Timer.ImmediateTask(
        [this, TransmissionMode]() -> void {
            m_TransmissionMode = TransmissionMode;
        },
        TransmissionSession::HIGH_PRIORITY);
}

void TransmissionSession::ChangeBlockSize(const Parameter::BLOCK_SIZE BlockSize)
{
    /**
     * @brief Session parameter updates are serialized using m_Timer.
     */
    m_Timer.ImmediateTask(
        [this, BlockSize]() -> void {
            m_BlockSize = BlockSize;
        },
        TransmissionSession::HIGH_PRIORITY);
}

void TransmissionSession::ChangeRetransmissionRedundancy(const uint16_t RetransmissionRedundancy)
{
    /**
     * @brief Session parameter updates are serialized using m_Timer.
     */
    m_Timer.ImmediateTask(
        [this, RetransmissionRedundancy]() -> void {
            m_RetransmissionRedundancy = RetransmissionRedundancy;
        },
        TransmissionSession::HIGH_PRIORITY);
}

void TransmissionSession::ChangeSessionParameter(const Parameter::TRANSMISSION_MODE TransmissionMode, const Parameter::BLOCK_SIZE BlockSize, const uint16_t RetransmissionRedundancy)
{
    /**
     * @brief Session parameter updates are serialized using m_Timer.
     */
    m_Timer.ImmediateTask(
        [this, TransmissionMode, BlockSize, RetransmissionRedundancy]() -> void {
            m_TransmissionMode = TransmissionMode;
            m_BlockSize = BlockSize;
            m_RetransmissionRedundancy = RetransmissionRedundancy;
        },
        TransmissionSession::HIGH_PRIORITY);
}

const bool TransmissionSession::SendPing()
{
    Header::Ping ping;
    ping.m_Type = Header::Data::HeaderType::PING;
    ping.m_CheckSum = 0;
    ping.m_PingTime = CLOCK::now().time_since_epoch().count();

    /**
     * @brief 1. Check last pong time. If we have not received pong from remote host more than CONNECTION_TIMEOUT then we trigger disconnection for this session and stop sending ping.
     */
    CLOCK::time_point const CurrentTime(CLOCK::duration(ping.m_PingTime));
    CLOCK::time_point const LastPongRecvTime(CLOCK::duration(m_LastPongTime.load()));
    std::chrono::duration<double> TimeSinceLastPongTime = std::chrono::duration_cast<std::chrono::duration<double>>(CurrentTime - LastPongRecvTime);
    if (TimeSinceLastPongTime.count() > Parameter::CONNECTION_TIMEOUT)
    {
        std::cout << "Client is disconnected. No response for " << TimeSinceLastPongTime.count() << " sec." << std::endl;
        c_Transmission->Disconnect(c_Addr);
        return false;
    }
    /**
     * @brief 2. Send ping packet to remote host.
     */
    ping.m_CheckSum = Checksum::get(reinterpret_cast<uint8_t *>(&ping), sizeof(Header::Ping));
    sendto(c_Socket, reinterpret_cast<uint8_t *>(&ping), sizeof(Header::Ping), 0, (sockaddr *)&c_Addr.Addr, c_Addr.AddrLength);
    /**
     * @brief 3. Schedule next ping transmission.
     */
    return true;
}

void TransmissionSession::ProcessPong(const uint16_t Rtt)
{
    /**
     * @brief Update m_RoundTripTime, which is used for retransmission interval.
     *        It is serialized using m_Timer.
     */
    m_Timer.ImmediateTask(
        [this, Rtt]() -> void {
            if (Rtt < Parameter::MINIMUM_RETRANSMISSION_INTERVAL)
            {
                m_RoundTripTime = (m_RoundTripTime + Parameter::MINIMUM_RETRANSMISSION_INTERVAL) / 2;
            }
            else
            {
                m_RoundTripTime = (m_RoundTripTime + Rtt) / 2;
            }
        },
        TransmissionSession::HIGH_PRIORITY);
}

void TransmissionSession::ProcessDataAck(const uint8_t Rank, const uint8_t MaxRank, const uint8_t Loss, const uint16_t Sequence)
{
    /**
     * @brief Proccess ack. It is serialized by m_Timer.
     * @todo Retransmission can be further optimized by using loss rate.
     *       E.g., we can immediately transmit encoded packets corersponding to loss ratio, i.e. (Block size * loss possibility) without waiting the ack.
     */
    m_Timer.ImmediateTask(
        [this, Rank, MaxRank, Loss, Sequence]() -> void {
            {
                std::map<int32_t, int16_t>::iterator it;
                std::unique_lock<std::mutex> acklock(m_AckListLock);
                it = m_AckList.find((int32_t)ntohs(Sequence));
                if (it != m_AckList.end())
                {
                    it->second = Rank;
                }
                if (it->second >= MaxRank)
                {
                    m_AckList.erase(it);
                }
            }
        },
        TransmissionSession::HIGH_PRIORITY);
}

void TransmissionSession::ProcessSyncAck(const uint16_t sequence)
{
    /**
     * @brief Process sync ack. Both end hosts synchronize their starting TransmissionBlock's sequence number.
     */
    m_Timer.ImmediateTask(
        [this, sequence]() -> void {
            if (sequence == m_MaxBlockSequenceNumber)
            {
                m_IsConnected = true;
            }
            m_Timer.PeriodicTask(
                0,
                [this]() -> const bool {
                    PopDataPacket();
                    return m_IsConnected;
                },
                TransmissionSession::LOW_PRIORITY);
        },
        TransmissionSession::HIGH_PRIORITY);
}

void TransmissionSession::PushDataPacket(uint8_t *const buffer, const uint16_t size, bool orig, TransmissionBlock *const block)
{
    /**
     * @brief 1. Increament m_BytesUnacked by packet size.
     */
    if (orig)
    {
        m_BytesUnacked.fetch_add(size);
    }
    /**
     * @brief 2. Enqueue the packet on m_TxQueue.
     */
    m_Timer.ImmediateTask(
        [this, buffer, size, orig, block]() -> void {
            if (orig)
            {
                m_TxQueue.push(std::make_tuple(buffer, size, orig, block));
            }
            else
            {
                uint8_t *cpy = new uint8_t[size];
                memcpy(cpy, buffer, size);
                m_TxQueue.push(std::make_tuple(cpy, size, orig, block));
            }
        },
        TransmissionSession::LOW_PRIORITY);
}

uint16_t TransmissionSession::PopDataPacket()
{
    /**
     * @brief 1. Dequeue a packet from m_TxQueue.
     */
    if (m_TxQueue.size() == 0)
    {
        return 0;
    }
    Header::Data *const DataHeader = reinterpret_cast<Header::Data *>(std::get<0>(m_TxQueue.front()));
    const uint16_t sequence = ntohs(DataHeader->m_CurrentBlockSequenceNumber);
    const uint8_t expected_rank = DataHeader->m_ExpectedRank;
    /**
     * @brief 2. Transmit the packet.
     */
    const int ret = sendto(c_Socket, std::get<0>(m_TxQueue.front()), std::get<1>(m_TxQueue.front()), 0, (sockaddr *)&c_Addr.Addr, c_Addr.AddrLength);
    if (ret < 0)
    {
        return 0;
    }
    else
    {
        /**
         * @brief 3. Schedule next retransmission if the packet is the last original packet or network coding packet.
         */
        if (DataHeader->m_Flags & Header::Data::FLAGS_END_OF_BLK)
        {
            TransmissionBlock *const block = std::get<3>(m_TxQueue.front());
            m_Timer.ScheduleTaskNoExcept(m_RoundTripTime,
                                         [block]() -> void {
                                             block->Retransmission();
                                         },
                                         (m_MinBlockSequenceNumber == block->m_BlockSequenceNumber ? TransmissionSession::MIDDLE_PRIORITY : TransmissionSession::LOW_PRIORITY));
        }
        /**
         * @brief 4. Free the packet.
         */
        if (std::get<2>(m_TxQueue.front()) == false)
        {
            delete[] std::get<0>(m_TxQueue.front());
        }
        m_TxQueue.pop();
        /**
         * @brief 5. Schedule ack timeout.
         */
        m_Timer.ScheduleTaskNoExcept(m_RoundTripTime,
                                     [this, sequence, expected_rank]() -> void {
                                         std::unique_lock<std::mutex> lock(m_AckListLock);
                                         std::map<int32_t, int16_t>::iterator it;
                                         it = m_AckList.find(sequence);
                                         if (it == m_AckList.end())
                                         {
                                             if (m_CongestionWindow * 2 > Parameter::MAXIMUM_CONGESTION_WINDOW_SIZE)
                                             {
                                                 m_CongestionWindow = Parameter::MAXIMUM_CONGESTION_WINDOW_SIZE;
                                             }
                                             else
                                             {
                                                 m_CongestionWindow = m_CongestionWindow * 2;
                                             }
                                         }
                                         else if (it->second >= expected_rank)
                                         {
                                             if (m_CongestionWindow * 2 > Parameter::MAXIMUM_CONGESTION_WINDOW_SIZE)
                                             {
                                                 m_CongestionWindow = Parameter::MAXIMUM_CONGESTION_WINDOW_SIZE;
                                             }
                                             else
                                             {
                                                 m_CongestionWindow = m_CongestionWindow * 2;
                                             }
                                         }
                                         else
                                         {
                                             m_CongestionWindow = m_CongestionWindow / 2;
                                             if (m_CongestionWindow < Parameter::MINIMUM_CONGESTION_WINDOW_SIZE)
                                             {
                                                 m_CongestionWindow = Parameter::MINIMUM_CONGESTION_WINDOW_SIZE;
                                             }
                                         }
                                     },
                                     TransmissionSession::LOW_PRIORITY);
        return (uint16_t)ret;
    }
}

Transmission::Transmission(const int32_t Socket) : c_Socket(Socket) {}

Transmission::~Transmission()
{
    /**
     * @brief Free TransmissionSession objects
     */
    m_Sessions.DoSomethingOnAllData([](TransmissionSession *&session) -> void { delete session; });
    m_Sessions.Clear();
}

bool Transmission::Connect(const DataTypes::Address Addr, uint32_t ConnectionTimeout, const Parameter::TRANSMISSION_MODE TransmissionMode, const Parameter::BLOCK_SIZE BlockSize, const uint16_t RetransmissionRedundancy)
{
    /**
     * @brief 1. Create session.
     */
    TransmissionSession *newsession = nullptr;
    {
        std::unique_lock<std::mutex> lock(m_Lock);
        const DataTypes::SessionKey key = DataTypes::GetSessionKey((sockaddr *)&Addr.Addr, Addr.AddrLength);
        TransmissionSession **const session = m_Sessions.GetPtr(key);
        if (session != nullptr)
        {
            // Session already has been established.
            newsession = (*session);
        }
        else
        {
            // Session is not established yet.
            try
            {
                TEST_EXCEPTION(std::bad_alloc());
                newsession = new TransmissionSession(this, c_Socket, Addr, TransmissionMode, BlockSize, RetransmissionRedundancy);
            }
            catch (const std::bad_alloc &ex)
            {
                EXCEPTION_PRINT;
                return false;
            }
            if (m_Sessions.Insert(key, newsession) == false)
            {
                return false;
            }
            if (newsession->m_BlockSize == Parameter::BLOCK_SIZE::INVALID_BLOCK_SIZE)
            {
                delete newsession;
                m_Sessions.Remove(key);
                return false;
            }
        }
    }
    /**
     * @brief 2. Trasnmit Sync packet to remote host.
     */
    Header::Sync SyncPacket;
    SyncPacket.m_Type = Header::Common::HeaderType::SYNC;
    SyncPacket.m_CheckSum = 0;
    SyncPacket.m_Sequence = htons(newsession->m_MaxBlockSequenceNumber);

    SyncPacket.m_CheckSum = Checksum::get(reinterpret_cast<uint8_t *>(&SyncPacket), sizeof(SyncPacket));
    if (sendto(c_Socket, (uint8_t *)&SyncPacket, sizeof(SyncPacket), 0, (sockaddr *)(&newsession->c_Addr.Addr), newsession->c_Addr.AddrLength) != sizeof(SyncPacket))
    {
        return false;
    }

    /**
     * @brief 3. Wait for sync ack from remote host for "ConnectionTimeout" ms. 
     */
    while (!newsession->m_IsConnected && ConnectionTimeout > 0)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        ConnectionTimeout--;
    }
    if (ConnectionTimeout == 0)
    {
        // Sync ack has not been received from remote host.
        return false;
    }
    /**
     * @brief 4. Start periodically send ping with interval of PING_INTERVAL.
     */
    newsession->m_LastPongTime = CLOCK::now().time_since_epoch().count();
    newsession->m_Timer.PeriodicTask(
        Parameter::PING_INTERVAL,
        [newsession]() -> const bool {
            return newsession->SendPing();
        },
        TransmissionSession::HIGH_PRIORITY);
    return true;
}

bool Transmission::Send(const DataTypes::Address Addr, uint8_t *buffer, uint16_t buffersize)
{
    /**
     * @brief 1. Find TransmissionSession object corresponding to given Addr.
     */
    TransmissionSession *p_session = nullptr;
    {
        std::unique_lock<std::mutex> lock(m_Lock);
        const DataTypes::SessionKey key = DataTypes::GetSessionKey((sockaddr *)&Addr.Addr, Addr.AddrLength);
        TransmissionSession **const pp_session = m_Sessions.GetPtr(key);
        if (pp_session)
        {
            p_session = (*pp_session);
        }
    }
    if (p_session == nullptr)
    {
        return false;
    }
    if (p_session->m_IsConnected == false)
    {
        return false;
    }

    std::atomic<bool> TransmissionIsCompleted(false);
    std::atomic<bool> TransmissionResult(false);
    /**
     * @brief 2. Wait until there is availble transmission slot, i.e., m_BytesUnacked + buffersize > m_CongestionWindow.
     */
    while (p_session->m_BytesUnacked + buffersize > p_session->m_CongestionWindow)
    {
        if (!p_session->m_IsConnected)
        {
            return false;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    /**
     * @brief 3. Schedule transmission, i.e., TransmissionBlock::Send, on tx_Timer.
     */
    const uint32_t TransmissionIsScheduled = p_session->m_Timer.ImmediateTask(
        [buffer, buffersize, p_session, &TransmissionIsCompleted, &TransmissionResult]() -> void {
            // 1. Get Transmission Block
            if (p_session->p_TransmissionBlock == nullptr)
            {
                try
                {
#if ENABLE_CRITICAL_EXCEPTIONS
                    TEST_EXCEPTION(std::bad_alloc());
#endif
                    p_session->p_TransmissionBlock = new TransmissionBlock(p_session);
                }
                catch (const std::bad_alloc &ex)
                {
                    EXCEPTION_PRINT;
                    TransmissionResult = false;
                    TransmissionIsCompleted = true;
                    return;
                }
            }
            TransmissionBlock *const block = p_session->p_TransmissionBlock;
            if (block->Send(buffer, buffersize) == false)
            {
                TransmissionResult = false;
                TransmissionIsCompleted = true;
                return;
            }
            TransmissionResult = true;
            TransmissionIsCompleted = true;
        },
        TransmissionSession::MIDDLE_PRIORITY);

    if (TransmissionIsScheduled == INVALID_TASK_ID)
    {
        TransmissionResult = false;
        TransmissionIsCompleted = true;
    }
    while (!TransmissionIsCompleted)
    {
        // Yield CPU resource.
        std::this_thread::sleep_for(std::chrono::milliseconds(0));
    }
    return TransmissionResult;
}

bool Transmission::Flush(const DataTypes::Address Addr)
{
    TransmissionSession *p_session = nullptr;
    {
        std::unique_lock<std::mutex> lock(m_Lock);
        const DataTypes::SessionKey key = DataTypes::GetSessionKey((sockaddr *)&Addr.Addr, Addr.AddrLength);
        TransmissionSession **const pp_session = m_Sessions.GetPtr(key);
        if (pp_session)
        {
            p_session = (*pp_session);
        }
    }
    if (p_session == nullptr)
    {
        return false;
    }
    if (p_session->m_IsConnected == false)
    {
        return false;
    }
    const uint32_t TaskID = p_session->m_Timer.ImmediateTaskNoExcept(
        [p_session]() -> void {
            // 1. Get Transmission Block
            if (p_session->p_TransmissionBlock)
            {
                TransmissionBlock *const block = p_session->p_TransmissionBlock;
                block->Retransmission();
                p_session->p_TransmissionBlock = nullptr;
            }
        },
        TransmissionSession::HIGH_PRIORITY);
    return IMMEDIATE_TASK_ID == TaskID;
}

void Transmission::WaitUntilTxIsCompleted(const DataTypes::Address Addr)
{
    TransmissionSession *p_session = nullptr;
    {
        std::unique_lock<std::mutex> lock(m_Lock);
        const DataTypes::SessionKey key = DataTypes::GetSessionKey((sockaddr *)&Addr.Addr, Addr.AddrLength);
        TransmissionSession **const pp_session = m_Sessions.GetPtr(key);
        if (pp_session)
        {
            p_session = (*pp_session);
        }
    }
    if (p_session == nullptr)
    {
        return;
    }
    if (p_session->m_IsConnected == false)
    {
        return;
    }
    while (p_session->m_IsConnected && p_session->m_BytesUnacked > 0)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}

void Transmission::Disconnect(const DataTypes::Address Addr)
{
    const DataTypes::SessionKey key = DataTypes::GetSessionKey((sockaddr *)&Addr.Addr, Addr.AddrLength);
    TransmissionSession **pp_session = nullptr;
    {
        std::unique_lock<std::mutex> lock(m_Lock);

        pp_session = m_Sessions.GetPtr(key);
        if (pp_session == nullptr)
        {
            return;
        }
    }
    (*pp_session)->m_IsConnected = false;
    while (0 < (*pp_session)->m_BytesUnacked)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    m_Sessions.Remove(key,
                      [](TransmissionSession *&session) -> void {
                          delete session;
                      });
}

/* OK */
void Transmission::RxHandler(uint8_t *const buffer, const uint16_t size, const sockaddr *const sender_addr, const uint32_t sender_addr_len)
{
    Header::Common *CommonHeader = reinterpret_cast<Header::Common *>(buffer);
    if (Checksum::get(buffer, size) != 0x0)
    {
        return;
    }
    switch (CommonHeader->m_Type)
    {
    case Header::Common::HeaderType::DATA_ACK:
    {
        const Header::DataAck *Ack = reinterpret_cast<Header::DataAck *>(buffer);
        const DataTypes::SessionKey key = DataTypes::GetSessionKey(sender_addr, sender_addr_len);
        std::unique_lock<std::mutex> lock(m_Lock);
        TransmissionSession **const pp_session = m_Sessions.GetPtr(key);
        if (pp_session)
        {
            (*pp_session)->ProcessDataAck(Ack->m_Rank, Ack->m_MaxRank, Ack->m_Losses, Ack->m_BlockSequenceNumber);
        }
    }
    break;

    case Header::Common::HeaderType::SYNC_ACK:
    {
        const Header::Sync *sync = reinterpret_cast<Header::Sync *>(buffer);
        const DataTypes::SessionKey key = DataTypes::GetSessionKey(sender_addr, sender_addr_len);
        std::unique_lock<std::mutex> lock(m_Lock);
        TransmissionSession **const pp_session = m_Sessions.GetPtr(key);
        if (pp_session)
        {
            (*pp_session)->ProcessSyncAck(ntohs(sync->m_Sequence));
        }
    }
    break;

    case Header::Common::PONG:
    {
        const Header::Pong *pong = reinterpret_cast<Header::Pong *>(buffer);
        const DataTypes::SessionKey key = DataTypes::GetSessionKey(sender_addr, sender_addr_len);
        std::unique_lock<std::mutex> lock(m_Lock);
        TransmissionSession **const pp_session = m_Sessions.GetPtr(key);
        if (pp_session)
        {
            CLOCK::time_point const SentTime(CLOCK::duration(pong->m_PingTime));
            (*pp_session)->m_LastPongTime = CLOCK::now().time_since_epoch().count();
            CLOCK::time_point const RecvTime(CLOCK::duration((*pp_session)->m_LastPongTime));
            std::chrono::duration<double> rtt = std::chrono::duration_cast<std::chrono::duration<double>>(RecvTime - SentTime);
            (*pp_session)->ProcessPong((uint16_t)(rtt.count() * 1000.));
        }
    }
    break;

    default:
        break;
    }
}
