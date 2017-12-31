#include "common.h"

using namespace NetworkCoding;

template <uint32_t LOOPS, uint32_t ITER>
class EncodingUnrolling
{
  public:
    static void Run(uint8_t *remedyPacketBuffer, uint8_t *originalPacketBuffer, const std::vector<uint8_t> &randomCoefficients, uint16_t &codingOffset, const uint8_t packetIndex)
    {
        remedyPacketBuffer[codingOffset + ITER] ^= FiniteField::instance()->mul(originalPacketBuffer[codingOffset + ITER], randomCoefficients[packetIndex]);
        EncodingUnrolling<LOOPS, ITER + 1>::Run(remedyPacketBuffer, originalPacketBuffer, randomCoefficients, codingOffset, packetIndex);
    }
};

template <uint32_t LOOPS>
class EncodingUnrolling<LOOPS, LOOPS>
{
  public:
    static void Run(uint8_t *remedyPacketBuffer, uint8_t *originalPacketBuffer, const std::vector<uint8_t> &randomCoefficients, uint16_t &codingOffset, const uint8_t packetIndex)
    {
        codingOffset += LOOPS;
    }
};

template <uint32_t LOOPS>
class EncodingPacket
{
  public:
    static void Run(uint8_t *remedyPacketBuffer, uint8_t *originalPacketBuffer, const std::vector<uint8_t> &randomCoefficients, uint16_t &codingOffset, const uint8_t packetIndex)
    {
        EncodingUnrolling<LOOPS, 0>::Run(remedyPacketBuffer, originalPacketBuffer, randomCoefficients, codingOffset, packetIndex);
    }
};

template <uint32_t LOOPS, uint32_t ITER>
class DecodingUnrolling
{
  public:
    static void Run(
        std::vector<std::unique_ptr<uint8_t[]>>& out, 
        std::vector<std::unique_ptr<uint8_t[]>>& decodedPacketBuffer, 
        const std::vector<std::unique_ptr<uint8_t[]>>& decodingMatrix,
        uint32_t &decodingOffset,
        const uint8_t decodingIndex,
        const uint8_t packetIndex)
    {
        out.back().get()[decodingOffset + ITER] ^= FiniteField::instance()->mul(decodingMatrix[packetIndex].get()[decodingIndex], decodedPacketBuffer[decodingIndex].get()[decodingOffset + ITER]);
        DecodingUnrolling<LOOPS, ITER + 1>::Run(out, decodedPacketBuffer, decodingMatrix, decodingOffset, decodingIndex, packetIndex);
    }
};

template <uint32_t LOOPS>
class DecodingUnrolling<LOOPS, LOOPS>
{
  public:
    static void Run(
        std::vector<std::unique_ptr<uint8_t[]>>& out, 
        std::vector<std::unique_ptr<uint8_t[]>>& decodedPacketBuffer, 
        const std::vector<std::unique_ptr<uint8_t[]>>& decodingMatrix,
        uint32_t &decodingOffset,
        const uint8_t decodingIndex,
        const uint8_t packetIndex)
    {
        decodingOffset += LOOPS;
    }
};

template <uint32_t LOOPS>
class DecodingPacket
{
  public:
    static void Run(
        std::vector<std::unique_ptr<uint8_t[]>>& out, 
        std::vector<std::unique_ptr<uint8_t[]>>& decodedPacketBuffer, 
        const std::vector<std::unique_ptr<uint8_t[]>>& decodingMatrix,
        uint32_t &decodingOffset,
        const uint8_t decodingIndex,
        const uint8_t packetIndex)
    {
        DecodingUnrolling<LOOPS, 0>::Run(out, decodedPacketBuffer, decodingMatrix, decodingOffset, decodingIndex, packetIndex);
    }
};
