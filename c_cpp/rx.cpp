#include "rx.h"
#include "chk.h"
#include "encoding_decoding_macro.h"
#include <cstdlib>

#define STRICTLY_ASCENDING_ORDER(a, b, c) ((uint16_t)((uint16_t)(c) - (uint16_t)(a)) > (uint16_t)((uint16_t)(c) - (uint16_t)(b)))

using namespace NetworkCoding;

void PRINT(Header::Data *data)
{
    printf("[Type %hhu][TotalSize %hu][MinSeq. %hu][CurSeq. %hu][MaxSeq. %hu][Exp.Rank %hhu][Max.Rank %hhu][Flags %hhx][TxCnt %hhu][Payload %hu][LastInd. %hhu]",
           data->m_Type,
           ntohs(data->m_TotalSize),
           ntohs(data->m_MinBlockSequenceNumber),
           ntohs(data->m_CurrentBlockSequenceNumber),
           ntohs(data->m_MaxBlockSequenceNumber),
           data->m_ExpectedRank,
           data->m_MaximumRank,
           data->m_Flags,
           data->m_TxCount,
           ntohs(data->m_PayloadSize),
           data->m_LastIndicator);
    printf("[Code ");
    for (uint8_t i = 0; i < data->m_MaximumRank; i++)
    {
        printf(" %3hhu ", data->m_Codes[i]);
    }
    printf("]\n");
    for (uint16_t i = 0; i < ntohs(data->m_PayloadSize); i++)
    {
        std::cout << (data->m_Codes + (data->m_MaximumRank - 1))[i];
    }
    std::cout << std::endl;
}

const uint8_t ReceptionBlock::FindMaximumRank(const Header::Data *const hdr)
{
    uint8_t MaximumRank = 0;
    if (m_EncodedPacketBuffer.size())
    {
        return reinterpret_cast<Header::Data *>(m_EncodedPacketBuffer[0].get())->m_ExpectedRank;
    }
    if (hdr && hdr->m_Flags & Header::Data::DataHeaderFlag::FLAGS_END_OF_BLK)
    {
        return hdr->m_ExpectedRank;
    }
    for (uint8_t i = 0; i < m_DecodedPacketBuffer.size(); i++)
    {
        if (reinterpret_cast<Header::Data *>(m_DecodedPacketBuffer[i].get())->m_Flags & Header::Data::DataHeaderFlag::FLAGS_END_OF_BLK)
        {
            return reinterpret_cast<Header::Data *>(m_DecodedPacketBuffer[i].get())->m_ExpectedRank;
        }
        else if (MaximumRank < reinterpret_cast<Header::Data *>(m_DecodedPacketBuffer[i].get())->m_ExpectedRank)
        {
            MaximumRank = reinterpret_cast<Header::Data *>(m_DecodedPacketBuffer[i].get())->m_ExpectedRank;
        }
    }
    if (hdr && MaximumRank < hdr->m_ExpectedRank)
    {
        MaximumRank = hdr->m_ExpectedRank;
    }
    return MaximumRank;
}

const bool ReceptionBlock::FindEndOfBlock(const Header::Data *const hdr)
{
    if (m_EncodedPacketBuffer.size())
    {
        return true;
    }
    if (hdr && hdr->m_Flags & Header::Data::DataHeaderFlag::FLAGS_END_OF_BLK)
    {
        return true;
    }
    for (uint8_t i = 0; i < m_DecodedPacketBuffer.size(); i++)
    {
        if (reinterpret_cast<Header::Data *>(m_DecodedPacketBuffer[i].get())->m_Flags & Header::Data::DataHeaderFlag::FLAGS_END_OF_BLK)
        {
            return true;
        }
    }
    return false;
}

ReceptionBlock::ReceiveAction ReceptionBlock::FindAction(const uint8_t *const buffer, const uint16_t length)
{
    const uint8_t OLD_RANK = m_DecodedPacketBuffer.size() + m_EncodedPacketBuffer.size();
    const uint8_t MAX_RANK = FindMaximumRank(reinterpret_cast<const Header::Data *const>(buffer));
    const bool MAKE_DECODING_MATRIX = (OLD_RANK + 1 == MAX_RANK && FindEndOfBlock(reinterpret_cast<const Header::Data *const>(buffer)));

    std::vector<std::unique_ptr<uint8_t[]>> EncodingMatrix;
    if (OLD_RANK == MAX_RANK)
    {
        return DECODING;
    }
    if (MAKE_DECODING_MATRIX)
    {
        for (uint8_t row = 0; row < MAX_RANK; row++)
        {
            try
            {
                TEST_EXCEPTION(std::bad_alloc());
                m_DecodingMatrix.emplace_back(std::unique_ptr<uint8_t[]>(new uint8_t[MAX_RANK]));
            }
            catch (const std::bad_alloc &ex)
            {
                EXCEPTION_PRINT;
                m_DecodingMatrix.clear();
                return DROP;
            }
            memset(m_DecodingMatrix.back().get(), 0x0, MAX_RANK);
            m_DecodingMatrix.back().get()[row] = 0x01;
        }
    }
    // 1. Allcate Encoding Matrix
    for (uint8_t row = 0; row < MAX_RANK; row++)
    {
        try
        {
            TEST_EXCEPTION(std::bad_alloc());
            EncodingMatrix.emplace_back(std::unique_ptr<uint8_t[]>(new uint8_t[MAX_RANK]));
            memset(EncodingMatrix.back().get(), 0x0, MAX_RANK);
        }
        catch (const std::bad_alloc &ex)
        {
            EXCEPTION_PRINT;
            m_DecodingMatrix.clear();
            return DROP;
        }
    }
    // 2. Fill-in Encoding Matrix
    {
        uint8_t DecodedPktIdx = 0;
        uint8_t EncodedPktIdx = 0;
        uint8_t RxPkt = 0;
        for (uint8_t row = 0; row < MAX_RANK; row++)
        {
            if (DecodedPktIdx < m_DecodedPacketBuffer.size() &&
                reinterpret_cast<Header::Data *>(m_DecodedPacketBuffer[DecodedPktIdx].get())->m_Codes[row])
            {
                memcpy(EncodingMatrix[row].get(), reinterpret_cast<Header::Data *>(m_DecodedPacketBuffer[DecodedPktIdx++].get())->m_Codes, MAX_RANK);
            }
            else if (EncodedPktIdx < m_EncodedPacketBuffer.size() && reinterpret_cast<Header::Data *>(m_EncodedPacketBuffer[EncodedPktIdx].get())->m_Codes[row])
            {
                memcpy(EncodingMatrix[row].get(), reinterpret_cast<Header::Data *>(m_EncodedPacketBuffer[EncodedPktIdx++].get())->m_Codes, MAX_RANK);
            }
            else if (RxPkt < 1 && reinterpret_cast<const Header::Data *const>(buffer)->m_Codes[row])
            {
                memcpy(EncodingMatrix[row].get(), reinterpret_cast<const Header::Data *const>(buffer)->m_Codes, MAX_RANK);
                RxPkt++;
            }
        }
    }
    // 3. Elimination
    for (uint8_t row = 0; row < MAX_RANK; row++)
    {
        if (EncodingMatrix[row].get()[row] == 0)
        {
            continue;
        }
        const uint8_t MUL = FiniteField::instance()->inv(EncodingMatrix[row].get()[row]);
        for (uint8_t col = 0; col < MAX_RANK; col++)
        {
            EncodingMatrix[row].get()[col] = FiniteField::instance()->mul(EncodingMatrix[row].get()[col], MUL);
            if (MAKE_DECODING_MATRIX)
            {
                m_DecodingMatrix[row].get()[col] = FiniteField::instance()->mul(m_DecodingMatrix[row].get()[col], MUL);
            }
        }

        for (uint8_t elimination_row = row + 1; elimination_row < MAX_RANK; elimination_row++)
        {
            if (EncodingMatrix[elimination_row].get()[row] == 0)
            {
                continue;
            }
            const uint8_t MUL2 = FiniteField::instance()->inv(EncodingMatrix[elimination_row].get()[row]);
            for (uint8_t j = 0; j < MAX_RANK; j++)
            {
                EncodingMatrix[elimination_row].get()[j] = FiniteField::instance()->mul(EncodingMatrix[elimination_row].get()[j], MUL2);
                EncodingMatrix[elimination_row].get()[j] ^= EncodingMatrix[row].get()[j];
                if (MAKE_DECODING_MATRIX)
                {
                    m_DecodingMatrix[elimination_row].get()[j] = FiniteField::instance()->mul(m_DecodingMatrix[elimination_row].get()[j], MUL2);
                    m_DecodingMatrix[elimination_row].get()[j] ^= m_DecodingMatrix[row].get()[j];
                }
            }
        }
    }
    uint8_t RANK = 0;
    for (uint8_t i = 0; i < MAX_RANK; i++)
    {
        if (EncodingMatrix[i].get()[i] == 1)
        {
            RANK++;
        }
    }
    if (MAKE_DECODING_MATRIX)
    {
        for (int16_t col = MAX_RANK - 1; col > -1; col--)
        {
            for (int16_t row = 0; row < col; row++)
            {
                if (EncodingMatrix[row].get()[col] == 0)
                {
                    continue;
                }
                const uint8_t MUL = EncodingMatrix[row].get()[col];
                for (uint8_t j = 0; j < MAX_RANK; j++)
                {
                    EncodingMatrix[row].get()[j] ^= FiniteField::instance()->mul(EncodingMatrix[col].get()[j], MUL);
                    if (MAKE_DECODING_MATRIX)
                    {
                        m_DecodingMatrix[row].get()[j] ^= FiniteField::instance()->mul(m_DecodingMatrix[col].get()[j], MUL);
                    }
                }
            }
        }
    }
    if (RANK == (OLD_RANK + 1))
    {
        return ENQUEUE_AND_DECODING;
    }
    if (MAKE_DECODING_MATRIX && m_DecodingMatrix.size())
    {
        m_DecodingMatrix.clear();
    }
    return DROP;
}

bool ReceptionBlock::Decoding()
{
    std::vector<std::unique_ptr<uint8_t[]>> DecodeOut;
    const uint8_t MAX_RANK = FindMaximumRank();
    uint8_t EncodedPktIdx = 0;
    if (MAX_RANK != m_DecodedPacketBuffer.size() + m_EncodedPacketBuffer.size())
    {
        return false;
    }
    for (uint8_t row = 0; row < MAX_RANK; row++)
    {
        if (row < m_DecodedPacketBuffer.size() &&
            reinterpret_cast<Header::Data *>(m_DecodedPacketBuffer[row].get())->m_Codes[row] > 0)
        {
            continue;
        }
        do
        {
            try
            {
                TEST_EXCEPTION(std::bad_alloc());
                m_DecodedPacketBuffer.emplace(m_DecodedPacketBuffer.begin() + row, std::unique_ptr<uint8_t[]>(m_EncodedPacketBuffer[EncodedPktIdx++].release()));
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
    m_EncodedPacketBuffer.clear();
    for (uint8_t row = 0; row < MAX_RANK; row++)
    {
        Header::Data *const pkt = reinterpret_cast<Header::Data *>(m_DecodedPacketBuffer[row].get());
        if (pkt->m_Flags & Header::Data::DataHeaderFlag::FLAGS_ORIGINAL)
        {
            do
            {
                try
                {
                    TEST_EXCEPTION(std::bad_alloc());
                    DecodeOut.emplace_back(std::unique_ptr<uint8_t[]>(nullptr));
                }
                catch (const std::bad_alloc &ex)
                {
                    EXCEPTION_PRINT;
                    std::this_thread::sleep_for(std::chrono::milliseconds(1));
                    continue;
                }
                break;
            } while (1);
            continue;
        }
        do
        {
            try
            {
                TEST_EXCEPTION(std::bad_alloc());
                DecodeOut.emplace_back(std::unique_ptr<uint8_t[]>(new uint8_t[ntohs(pkt->m_TotalSize)]));
            }
            catch (std::bad_alloc &ex)
            {
                EXCEPTION_PRINT;
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
                continue;
            }
            break;
        } while (1);

        memset(DecodeOut.back().get(), 0x0, ntohs(pkt->m_TotalSize));
        for (uint32_t decodingposition = 0; decodingposition < Header::Data::CodingOffset; decodingposition++)
        {
            DecodeOut.back().get()[decodingposition] = m_DecodedPacketBuffer[row].get()[decodingposition];
        }
        for (uint8_t i = 0; i < MAX_RANK; i++)
        {
            const uint16_t length = (ntohs(reinterpret_cast<Header::Data *>(m_DecodedPacketBuffer[i].get())->m_TotalSize) < ntohs(pkt->m_TotalSize) ? ntohs(reinterpret_cast<Header::Data *>(m_DecodedPacketBuffer[i].get())->m_TotalSize) : ntohs(pkt->m_TotalSize));
            uint32_t decodingposition = Header::Data::CodingOffset;
            while (decodingposition < length)
            {
                if (length - decodingposition > 128)
                {
                    Decoding128(DecodeOut.back().get(), m_DecodedPacketBuffer, m_DecodingMatrix, decodingposition, i, row);
                }
                else if (length - decodingposition > 64)
                {
                    Decoding64(DecodeOut.back().get(), m_DecodedPacketBuffer, m_DecodingMatrix, decodingposition, i, row);
                }
                else if (length - decodingposition > 32)
                {
                    Decoding32(DecodeOut.back().get(), m_DecodedPacketBuffer, m_DecodingMatrix, decodingposition, i, row);
                }
                else if (length - decodingposition > 16)
                {
                    Decoding16(DecodeOut.back().get(), m_DecodedPacketBuffer, m_DecodingMatrix, decodingposition, i, row);
                }
                else if (length - decodingposition > 8)
                {
                    Decoding8(DecodeOut.back().get(), m_DecodedPacketBuffer, m_DecodingMatrix, decodingposition, i, row);
                }
                else if (length - decodingposition > 4)
                {
                    Decoding4(DecodeOut.back().get(), m_DecodedPacketBuffer, m_DecodingMatrix, decodingposition, i, row);
                }
                else if (length - decodingposition > 2)
                {
                    Decoding2(DecodeOut.back().get(), m_DecodedPacketBuffer, m_DecodingMatrix, decodingposition, i, row);
                }
                else
                {
                    for (; decodingposition < length; decodingposition++)
                    {
                        DecodeOut.back().get()[decodingposition] ^= FiniteField::instance()->mul(m_DecodingMatrix[row].get()[i], m_DecodedPacketBuffer[i].get()[decodingposition]);
                    }
                }
            }
        }
        if (reinterpret_cast<Header::Data *>(DecodeOut.back().get())->m_Codes[row] != 1)
        {
            std::cout << "Decoding Error\n";
            exit(-1);
        }
    }
    for (uint8_t i = 0; i < DecodeOut.size(); i++)
    {
        if (DecodeOut[i].get() == nullptr)
        {
            DecodeOut[i].reset(m_DecodedPacketBuffer[i].release());
        }
    }
    for (uint8_t i = 0; i < DecodeOut.size(); i++)
    {
        uint8_t *pkt = DecodeOut[i].release();
        if (!(reinterpret_cast<Header::Data *>(pkt)->m_Flags & Header::Data::DataHeaderFlag::FLAGS_CONSUMED))
        {
            bool result = false;
            do
            {
                result = c_Session->m_RxTaskQueue.Enqueue(
                    [this, pkt]() -> void {
                        if (c_Reception->m_RxCallback)
                        {
                            c_Reception->m_RxCallback(pkt + sizeof(Header::Data) + reinterpret_cast<Header::Data *>(pkt)->m_MaximumRank - 1, ntohs(reinterpret_cast<Header::Data *>(pkt)->m_PayloadSize), (sockaddr *)&c_Session->m_SenderAddress.Addr, c_Session->m_SenderAddress.AddrLength);
                            delete[] pkt;
                        }
                        else
                        {
                            std::unique_lock<std::mutex> Lock(c_Reception->m_PacketQueueLock);
                            do
                            {
                                try
                                {
                                    TEST_EXCEPTION(std::bad_alloc());
                                    c_Reception->m_PacketQueue.push_back(std::tuple<DataTypes::Address, uint8_t *>(c_Session->m_SenderAddress, pkt));
                                    break;
                                }
                                catch (std::bad_alloc &ex)
                                {
                                    EXCEPTION_PRINT;
                                    std::this_thread::sleep_for(std::chrono::milliseconds(1));
                                }
                            } while (1);
                            c_Reception->m_Condition.notify_one();
                        }
                    });
            } while (result == false);
        }
        else
        {
            delete[] pkt;
        }
    }
    return true;
}

ReceptionBlock::ReceptionBlock(Reception *const reception, ReceptionSession *const session, const uint16_t BlockSequenceNumber) : c_Reception(reception), c_Session(session), m_BlockSequenceNumber(BlockSequenceNumber)
{
    m_DecodedPacketBuffer.clear();
    m_EncodedPacketBuffer.clear();
    m_DecodingReady = false;
}

ReceptionBlock::~ReceptionBlock()
{
    m_DecodedPacketBuffer.clear();
    m_EncodedPacketBuffer.clear();
}

void ReceptionBlock::Receive(uint8_t *const buffer, const uint16_t length, const sockaddr *const sender_addr, const uint32_t sender_addr_len)
{
    Header::Data *const DataHeader = reinterpret_cast<Header::Data *>(buffer);
    if (m_DecodingReady)
    {
        Header::DataAck ack;
        ack.m_Type = Header::Common::HeaderType::DATA_ACK;
        ack.m_CheckSum = 0;
        ack.m_Sequence = DataHeader->m_CurrentBlockSequenceNumber;
        ack.m_Losses = DataHeader->m_TxCount - DataHeader->m_ExpectedRank;
        ack.m_CheckSum = Checksum::get(reinterpret_cast<uint8_t *>(&ack), sizeof(ack));
        sendto(c_Reception->c_Socket, (uint8_t *)&ack, sizeof(ack), 0, (sockaddr *)sender_addr, sender_addr_len);
        return;
    }
    switch (FindAction(buffer, length))
    {
    case DROP:
        return;
        break;
    case ENQUEUE_AND_DECODING:
        if (DataHeader->m_Flags & Header::Data::DataHeaderFlag::FLAGS_ORIGINAL)
        {
            try
            {
                TEST_EXCEPTION(std::bad_alloc());
                m_DecodedPacketBuffer.emplace_back(std::unique_ptr<uint8_t[]>(new uint8_t[length]));
            }
            catch (const std::bad_alloc &ex)
            {
                EXCEPTION_PRINT;
                m_DecodingMatrix.clear();
                return;
            }
            memcpy(m_DecodedPacketBuffer.back().get(), buffer, length);
            if (c_Session->m_SequenceNumberForService == ntohs(DataHeader->m_CurrentBlockSequenceNumber) &&
                DataHeader->m_ExpectedRank == m_DecodedPacketBuffer.size())
            {
                do
                {
                    uint8_t *pkt;
                    try
                    {
                        TEST_EXCEPTION(std::bad_alloc());
                        bool result = false;
                        pkt = new uint8_t[length];
                        memcpy(pkt, buffer, length);
                        do
                        {
                            result = c_Session->m_RxTaskQueue.Enqueue(
                                [this, pkt]() -> void {
                                    if (c_Reception->m_RxCallback)
                                    {
                                        c_Reception->m_RxCallback(pkt + sizeof(Header::Data) + reinterpret_cast<Header::Data *>(pkt)->m_MaximumRank - 1, ntohs(reinterpret_cast<Header::Data *>(pkt)->m_PayloadSize), (sockaddr *)&c_Session->m_SenderAddress.Addr, c_Session->m_SenderAddress.AddrLength);
                                        delete[] pkt;
                                    }
                                    else
                                    {
                                        std::unique_lock<std::mutex> Lock(c_Reception->m_PacketQueueLock);
                                        do
                                        {
                                            try
                                            {
                                                TEST_EXCEPTION(std::bad_alloc());
                                                c_Reception->m_PacketQueue.push_back(std::tuple<DataTypes::Address, uint8_t *>(c_Session->m_SenderAddress, pkt));
                                                break;
                                            }
                                            catch (std::bad_alloc &ex)
                                            {
                                                EXCEPTION_PRINT;
                                                std::this_thread::sleep_for(std::chrono::milliseconds(1));
                                            }
                                        } while (1);
                                        c_Reception->m_Condition.notify_one();
                                    }
                                });
                        } while (result == false);
                        reinterpret_cast<Header::Data *>(m_DecodedPacketBuffer.back().get())->m_Flags |= Header::Data::DataHeaderFlag::FLAGS_CONSUMED;
                        if (reinterpret_cast<Header::Data *>(m_DecodedPacketBuffer.back().get())->m_Flags & Header::Data::DataHeaderFlag::FLAGS_END_OF_BLK)
                        {
                            c_Session->m_SequenceNumberForService++;
                        }
                        break;
                    }
                    catch (const std::bad_alloc &ex)
                    {
                        EXCEPTION_PRINT;
                    }
                } while (1);
            }
        }
        else
        {
            try
            {
                TEST_EXCEPTION(std::bad_alloc());
                m_EncodedPacketBuffer.emplace_back(std::unique_ptr<uint8_t[]>(new uint8_t[length]));
            }
            catch (const std::bad_alloc &ex)
            {
                EXCEPTION_PRINT;
                m_DecodingMatrix.clear();
                return;
            }
            memcpy(m_EncodedPacketBuffer.back().get(), buffer, length);
        }
    // Continue with decoding.
    case DECODING:
        if ((DataHeader->m_ExpectedRank == (m_DecodedPacketBuffer.size() + m_EncodedPacketBuffer.size())) &&
            (DataHeader->m_Flags & Header::Data::DataHeaderFlag::FLAGS_END_OF_BLK))
        {
            // Decoding.
            m_DecodingReady = true;
            if (c_Session->m_SequenceNumberForService == ntohs(DataHeader->m_CurrentBlockSequenceNumber))
            {
                ReceptionBlock **pp_block;
                ReceptionBlock *p_block;
                while (c_Session->m_SequenceNumberForService != c_Session->m_MaxSequenceNumberAwaitingAck &&
                       (pp_block = c_Session->m_Blocks.GetPtr(c_Session->m_SequenceNumberForService)))
                {
                    if ((*pp_block)->m_DecodingReady)
                    {
                        p_block = (*pp_block);
                        if (p_block->Decoding())
                        {
                            c_Session->m_SequenceNumberForService++;
                        }
                    }
                    else
                    {
                        for (uint8_t i = 0; i < (*pp_block)->m_DecodedPacketBuffer.size(); i++)
                        {
                            if (i != reinterpret_cast<Header::Data *>((*pp_block)->m_DecodedPacketBuffer[i].get())->m_ExpectedRank - 1)
                            {
                                break;
                            }
                            do
                            {
                                uint8_t *pkt;
                                try
                                {
                                    TEST_EXCEPTION(std::bad_alloc());
                                    bool result = false;
                                    pkt = new uint8_t[ntohs(reinterpret_cast<Header::Data *>((*pp_block)->m_DecodedPacketBuffer[i].get())->m_TotalSize)];
                                    memcpy(pkt, (*pp_block)->m_DecodedPacketBuffer[i].get(), ntohs(reinterpret_cast<Header::Data *>((*pp_block)->m_DecodedPacketBuffer[i].get())->m_TotalSize));
                                    do
                                    {
                                        result = c_Session->m_RxTaskQueue.Enqueue(
                                            [this, pkt]() -> void {
                                                if (c_Reception->m_RxCallback)
                                                {
                                                    c_Reception->m_RxCallback(pkt + sizeof(Header::Data) + reinterpret_cast<Header::Data *>(pkt)->m_MaximumRank - 1, ntohs(reinterpret_cast<Header::Data *>(pkt)->m_PayloadSize), (sockaddr *)&c_Session->m_SenderAddress.Addr, c_Session->m_SenderAddress.AddrLength);
                                                    delete[] pkt;
                                                }
                                                else
                                                {
                                                    std::unique_lock<std::mutex> Lock(c_Reception->m_PacketQueueLock);
                                                    do
                                                    {
                                                        try
                                                        {
                                                            TEST_EXCEPTION(std::bad_alloc());
                                                            c_Reception->m_PacketQueue.push_back(std::tuple<DataTypes::Address, uint8_t *>(c_Session->m_SenderAddress, pkt));
                                                            break;
                                                        }
                                                        catch (std::bad_alloc &ex)
                                                        {
                                                            EXCEPTION_PRINT;
                                                            std::this_thread::sleep_for(std::chrono::milliseconds(1));
                                                        }
                                                    } while (1);
                                                    c_Reception->m_Condition.notify_one();
                                                }
                                            });
                                    } while (result == false);
                                    reinterpret_cast<Header::Data *>((*pp_block)->m_DecodedPacketBuffer[i].get())->m_Flags |= Header::Data::DataHeaderFlag::FLAGS_CONSUMED;
                                    break;
                                }
                                catch (const std::bad_alloc &ex)
                                {
                                    EXCEPTION_PRINT;
                                }
                            } while (1);
                        }
                        break;
                    }
                }
            }
            Header::DataAck ack;
            ack.m_Type = Header::Common::HeaderType::DATA_ACK;
            ack.m_CheckSum = 0;
            ack.m_Sequence = DataHeader->m_CurrentBlockSequenceNumber;
            ack.m_Losses = DataHeader->m_TxCount - DataHeader->m_ExpectedRank;
            ack.m_CheckSum = Checksum::get(reinterpret_cast<uint8_t *>(&ack), sizeof(ack));
            sendto(c_Reception->c_Socket, (uint8_t *)&ack, sizeof(ack), 0, (sockaddr *)sender_addr, sender_addr_len);
        }
        break;
    }
}

ReceptionSession::ReceptionSession(Reception *const Session, const DataTypes::Address addr) : c_Reception(Session), m_SenderAddress(addr)
{
    m_SequenceNumberForService = 0;
    m_MinSequenceNumberAwaitingAck = 0;
    m_MaxSequenceNumberAwaitingAck = 0;
}

ReceptionSession::~ReceptionSession()
{
    m_Blocks.DoSomethingOnAllData([](ReceptionBlock *&block) { delete block; });
}

void ReceptionSession::Receive(uint8_t *const buffer, const uint16_t length, const sockaddr *const sender_addr, const uint32_t sender_addr_len)
{
    Header::Data *const DataHeader = reinterpret_cast<Header::Data *>(buffer);
    // update min and max sequence.
    if (STRICTLY_ASCENDING_ORDER((m_MinSequenceNumberAwaitingAck - 1), m_MinSequenceNumberAwaitingAck, ntohs(DataHeader->m_MinBlockSequenceNumber)))
    {
        for (; m_MinSequenceNumberAwaitingAck != ntohs(DataHeader->m_MinBlockSequenceNumber); m_MinSequenceNumberAwaitingAck++)
        {
            if (m_MinSequenceNumberAwaitingAck == m_SequenceNumberForService)
            {
                // This is the case of best effort service.
                ReceptionBlock **const pp_block = m_Blocks.GetPtr(m_SequenceNumberForService);
                if (pp_block)
                {
                    ReceptionBlock *const p_block = (*pp_block);
                    if (p_block->Decoding() == false)
                    {
                        for (uint8_t i = 0; i < p_block->m_DecodedPacketBuffer.size(); i++)
                        {
                            uint8_t *pkt = p_block->m_DecodedPacketBuffer[i].release();
                            bool result = false;
                            if (reinterpret_cast<Header::Data *>(pkt)->m_Flags & Header::Data::DataHeaderFlag::FLAGS_CONSUMED)
                            {
                                delete[] pkt;
                                continue;
                            }
                            do
                            {
                                result = m_RxTaskQueue.Enqueue(
                                    [this, pkt]() -> void {
                                        if (c_Reception->m_RxCallback)
                                        {
                                            c_Reception->m_RxCallback(pkt + sizeof(Header::Data) + reinterpret_cast<Header::Data *>(pkt)->m_MaximumRank - 1, ntohs(reinterpret_cast<Header::Data *>(pkt)->m_PayloadSize), (sockaddr *)&m_SenderAddress.Addr, m_SenderAddress.AddrLength);
                                            delete[] pkt;
                                        }
                                        else
                                        {
                                            std::unique_lock<std::mutex> Lock(c_Reception->m_PacketQueueLock);
                                            do
                                            {
                                                try
                                                {
                                                    TEST_EXCEPTION(std::bad_alloc());
                                                    c_Reception->m_PacketQueue.push_back(std::tuple<DataTypes::Address, uint8_t *>(m_SenderAddress, pkt));
                                                    break;
                                                }
                                                catch (std::bad_alloc &ex)
                                                {
                                                    EXCEPTION_PRINT;
                                                    std::this_thread::sleep_for(std::chrono::milliseconds(1));
                                                }
                                            } while (1);
                                            c_Reception->m_Condition.notify_one();
                                        }
                                    });
                            } while (result == false);
                        }
                    }
                }
                m_SequenceNumberForService++;
            }
            m_Blocks.Remove(m_MinSequenceNumberAwaitingAck, [this](ReceptionBlock *&data) {
                bool result = false;
                do
                {
                    result = m_RxTaskQueue.Enqueue(
                        [data]() -> void {
                            delete data;
                        });
                } while (result == false);
            });
        }
    }
    if (STRICTLY_ASCENDING_ORDER((m_MaxSequenceNumberAwaitingAck - 1), m_MaxSequenceNumberAwaitingAck, ntohs(DataHeader->m_MaxBlockSequenceNumber)))
    {
        m_MaxSequenceNumberAwaitingAck = ntohs(DataHeader->m_MaxBlockSequenceNumber);
    }
    if (STRICTLY_ASCENDING_ORDER(ntohs(DataHeader->m_CurrentBlockSequenceNumber), m_MinSequenceNumberAwaitingAck, m_MaxSequenceNumberAwaitingAck))
    {
        // If the sequence is less than min seq send ack and return.
        // But some packets can be received with significant delay.
        // Therefore, We must check if this packet is associated with the blocks in m_Blocks.
        ReceptionBlock **const pp_block = m_Blocks.GetPtr(ntohs(DataHeader->m_CurrentBlockSequenceNumber));
        if (pp_block && (*pp_block)->m_DecodingReady)
        {
            Header::DataAck ack;
            ack.m_Type = Header::Common::HeaderType::DATA_ACK;
            ack.m_CheckSum = 0;
            ack.m_Sequence = DataHeader->m_CurrentBlockSequenceNumber;
            ack.m_Losses = DataHeader->m_TxCount - DataHeader->m_ExpectedRank;
            ack.m_CheckSum = Checksum::get(reinterpret_cast<uint8_t *>(&ack), sizeof(ack));
            sendto(c_Reception->c_Socket, (uint8_t *)&ack, sizeof(ack), 0, (sockaddr *)sender_addr, sender_addr_len);
        }
        return;
    }

    ReceptionBlock **const pp_Block = m_Blocks.GetPtr(ntohs(DataHeader->m_CurrentBlockSequenceNumber));
    ReceptionBlock *p_Block = nullptr;
    if (pp_Block == nullptr)
    {
        try
        {
            TEST_EXCEPTION(std::bad_alloc());
            p_Block = new ReceptionBlock(c_Reception, this, ntohs(DataHeader->m_CurrentBlockSequenceNumber));
        }
        catch (const std::bad_alloc &ex)
        {
            EXCEPTION_PRINT;
            return;
        }
        if (m_Blocks.Insert(ntohs(DataHeader->m_CurrentBlockSequenceNumber), p_Block) == false)
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

Reception::Reception(const int32_t Socket, const std::function<void(uint8_t *buffer, uint16_t length, const sockaddr *const sender_addr, const uint32_t sender_addr_len)> rx) : c_Socket(Socket), m_RxCallback(rx) {}

Reception::~Reception()
{
    m_Sessions.DoSomethingOnAllData([](ReceptionSession *&session) { delete session; });
    m_Sessions.Clear();
}

void Reception::RxHandler(uint8_t *const buffer, const uint16_t size, const sockaddr *const sender_addr, const uint32_t sender_addr_len)
{
    Header::Common *CommonHeader = reinterpret_cast<Header::Common *>(buffer);
    if (Checksum::get(buffer, size) != 0x0)
    {
        return;
    }
    switch (CommonHeader->m_Type)
    {
    case Header::Common::HeaderType::DATA:
    {
        const DataTypes::SessionKey key = DataTypes::GetSessionKey(sender_addr, sender_addr_len);
        ReceptionSession **const pp_Session = m_Sessions.GetPtr(key);
        if (pp_Session == nullptr)
        {
            return;
        }
        ReceptionSession *const p_Session = (*pp_Session);
        p_Session->Receive(buffer, size, sender_addr, sender_addr_len);
    }
    break;

    case Header::Common::HeaderType::SYNC:
    {
        // create Rx Session.
        const DataTypes::SessionKey key = DataTypes::GetSessionKey(sender_addr, sender_addr_len);
        ReceptionSession **const pp_Session = m_Sessions.GetPtr(key);
        ReceptionSession *p_Session = nullptr;
        if (pp_Session == nullptr)
        {
            try
            {
                DataTypes::Address addr;
                if (sender_addr_len == sizeof(sockaddr_in))
                {
                    addr.Addr.IPv4 = *((sockaddr_in *)sender_addr);
                    addr.AddrLength = sender_addr_len;
                }
                else if (sender_addr_len == sizeof(sockaddr_in6))
                {
                    addr.Addr.IPv6 = *((sockaddr_in6 *)sender_addr);
                    addr.AddrLength = sender_addr_len;
                }
                TEST_EXCEPTION(std::bad_alloc());
                p_Session = new ReceptionSession(this, addr);
            }
            catch (const std::bad_alloc &ex)
            {
                EXCEPTION_PRINT;
                return;
            }
            if (m_Sessions.Insert(key, p_Session) == false)
            {
                delete p_Session;
                return;
            }
        }
        else
        {
            p_Session = (*pp_Session);
        }
        Header::Sync *const sync = reinterpret_cast<Header::Sync *>(buffer);
        p_Session->m_SequenceNumberForService = ntohs(sync->m_Sequence);
        p_Session->m_MinSequenceNumberAwaitingAck = ntohs(sync->m_Sequence);
        p_Session->m_MaxSequenceNumberAwaitingAck = ntohs(sync->m_Sequence);
        if (p_Session->m_Blocks.Size() > 0)
        {
            p_Session->m_Blocks.DoSomethingOnAllData([](ReceptionBlock *&block) { delete block; });
            p_Session->m_Blocks.Clear();
        }
        sync->m_Type = Header::Common::HeaderType::SYNC_ACK;
        sync->m_CheckSum = 0;
        sync->m_CheckSum = Checksum::get(buffer, size);
        sendto(c_Socket, buffer, size, 0, (sockaddr *)sender_addr, sender_addr_len);
    }
    break;

    case Header::Data::HeaderType::PING:
    {
        Header::Ping *const ping = reinterpret_cast<Header::Ping *>(buffer);
        ping->m_Type = Header::Data::HeaderType::PONG;
        ping->m_CheckSum = 0;
        ping->m_CheckSum = Checksum::get(buffer, size);
        sendto(c_Socket, buffer, size, 0, (sockaddr *)sender_addr, sender_addr_len);
    }
    break;

    default:
        break;
    }
}

bool Reception::Receive(uint8_t *const buffer, uint16_t *const length, sockaddr *const sender_addr, uint32_t *const sender_addr_len, uint32_t timeout)
{
    std::unique_lock<std::mutex> Lock(m_PacketQueueLock);
    if (m_PacketQueue.size() == 0)
    {
        if (timeout == 0)
        {
            m_Condition.wait(Lock);
        }
        else
        {
            m_Condition.wait_for(Lock, std::chrono::milliseconds(timeout));
            if (m_PacketQueue.size() == 0)
            {
                return false;
            }
        }
    }
    const DataTypes::Address addr = std::get<0>(m_PacketQueue.front());
    const uint8_t *const packet = std::get<1>(m_PacketQueue.front());
    if ((*length) < ntohs(reinterpret_cast<const Header::Data *const>(packet)->m_PayloadSize))
    {
        return false;
    }
    if ((*sender_addr_len) < addr.AddrLength)
    {
        return false;
    }
    m_PacketQueue.pop_front();

    memcpy(
        buffer,
        packet + sizeof(Header::Data) + reinterpret_cast<const Header::Data *const>(packet)->m_MaximumRank - 1,
        ntohs(reinterpret_cast<const Header::Data *const>(packet)->m_PayloadSize));
    (*length) = ntohs(reinterpret_cast<const Header::Data *const>(packet)->m_PayloadSize);

    memcpy(sender_addr, &addr.Addr, addr.AddrLength);
    (*sender_addr_len) = addr.AddrLength;
    delete[] packet;

    return true;
}
