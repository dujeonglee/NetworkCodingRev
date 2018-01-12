#ifndef COMMON_H
#define COMMON_H

// C Standard Library
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <time.h>
#include <stddef.h>
#include <arpa/inet.h>
#include <netdb.h>

// C++ Standard Library Header
#include <iostream>
#include <set>
#include <vector>
#include <queue>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <exception>
#include <new>
#include <stdexcept>
#include <cstdint>

// C++ Library Header
#include "ThreadPool.h"
#include "AVLTree.h"
#include "SingleShotTimer.h"
#include "finite_field.h"

#include "debug.h"

namespace NetworkCoding
{
namespace Header
{
struct Common
{
    uint8_t m_Type;
    uint8_t m_CheckSum;
    enum HeaderType : uint8_t
    {
        DATA = 1,
        SYNC,
        PING,
        DATA_ACK,
        SYNC_ACK,
        PONG
    };
} __attribute__((packed, may_alias));

struct Data : Common
{
    uint16_t m_TotalSize;
    uint16_t m_MinBlockSequenceNumber;
    uint16_t m_CurrentBlockSequenceNumber;
    uint16_t m_MaxBlockSequenceNumber;
    uint8_t m_ExpectedRank;
    uint8_t m_MaximumRank;
    uint8_t m_Flags;
    enum DataHeaderFlag : uint8_t
    {
        FLAGS_ORIGINAL = 0x1,
        FLAGS_END_OF_BLK = 0x2,
        FLAGS_CONSUMED = 0x4
    };
    uint8_t m_TxCount;
    enum OffSets : uint8_t
    {
        CodingOffset = (1 + 1 + 2 + 2 + 2 + 2 + 1 + 1 + 1 + 1)
    };
    // ^^^ This part is not encoded ^^^ //
    uint16_t m_PayloadSize;
    uint8_t m_LastIndicator;
    uint8_t m_Codes[1];
    // ^^^ This part is encoded ^^^ //
} __attribute__((packed, may_alias));

struct DataAck : Common
{
    uint8_t m_Losses; /*1*/
    uint8_t m_Sequences;
    uint16_t m_SequenceList[255];
} __attribute__((packed, may_alias));

struct Sync : Common
{
    uint16_t m_Sequence;
} __attribute__((packed, may_alias));

struct Ping : Common
{
    CLOCK::time_point::duration::rep m_PingTime;
} __attribute__((packed, may_alias));

struct Pong : Common
{
    CLOCK::time_point::duration::rep m_PingTime;
} __attribute__((packed, may_alias));

} // namespace Header

namespace Parameter
{
const uint8_t MAXIMUM_NUMBER_OF_CONCURRENT_RETRANSMISSION = 5; /* 5 Blocks   */
const uint16_t MAXIMUM_BUFFER_SIZE = 1500;                     /* 1500 Bytes */
const uint16_t PING_INTERVAL = 100;                            /* 100 ms     */
const double CONNECTION_TIMEOUT = 10.0;                        /* 10 s       */
const uint16_t MINIMUM_RETRANSMISSION_INTERVAL = 10;           /* 10 ms      */
enum TRANSMISSION_MODE : uint8_t
{
    RELIABLE_TRANSMISSION_MODE = 0,
    BEST_EFFORT_TRANSMISSION_MODE
};
enum BLOCK_SIZE : uint8_t
{
    INVALID_BLOCK_SIZE = 0,
    BLOCK_SIZE_04 = 4,
    BLOCK_SIZE_08 = 8,
    BLOCK_SIZE_12 = 12,
    BLOCK_SIZE_16 = 16,
    BLOCK_SIZE_20 = 20,
    BLOCK_SIZE_24 = 24,
    BLOCK_SIZE_28 = 28,
    BLOCK_SIZE_32 = 32,
    BLOCK_SIZE_64 = 64,
    BLOCK_SIZE_128 = 128
};
const uint8_t MAX_BLOCK_SIZE = BLOCK_SIZE_128;
} // namespace Parameter

namespace DataTypes
{
class SessionKey
{
  public:
    uint64_t m_EUI;
    uint16_t m_Port;
} __attribute__((packed));

inline const SessionKey GetSessionKey(const sockaddr *addr, int size)
{
    if (size == sizeof(sockaddr_in))
    {
        SessionKey ret = {((sockaddr_in *)addr)->sin_addr.s_addr, ((sockaddr_in *)addr)->sin_port};
        return ret;
    }
    else
    {
        SessionKey ret = {((uint64_t *)(((sockaddr_in6 *)addr)->sin6_addr.s6_addr))[1], ((sockaddr_in6 *)addr)->sin6_port};
        return ret;
    }
}

class Address
{
  public:
    union {
        sockaddr_in IPv4;
        sockaddr_in6 IPv6;
    } Addr;
    uint32_t AddrLength;
};

inline const Address GetAddress(std::string IP, std::string Port)
{
    Address ret;
    addrinfo hints;
    addrinfo *result = nullptr;
    addrinfo *iter = nullptr;

    memset(&ret, 0, sizeof(ret));
    memset(&hints, 0, sizeof(hints));
    hints.ai_flags = AI_PASSIVE;
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_DGRAM;
    if (0 == getaddrinfo(IP.c_str(), Port.c_str(), &hints, &result))
    {
        for (iter = result; iter != nullptr; iter = iter->ai_next)
        {
            if (iter->ai_family == AF_INET)
            {
                memcpy(&ret.Addr, iter->ai_addr, sizeof(sockaddr_in));
                ret.AddrLength = sizeof(sockaddr_in);
            }
            else if (iter->ai_family == AF_INET6)
            {
                memcpy(&ret.Addr, iter->ai_addr, sizeof(sockaddr_in6));
                ret.AddrLength = sizeof(sockaddr_in6);
            }
        }
    }
    if (result)
    {
        freeaddrinfo(result);
    }
    return ret;
}
} // namespace DataStructures

} // namespace NetworkCoding

#endif // COMMON_H
