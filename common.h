#ifndef COMMON_H
#define COMMON_H

// C++ Standard Library Header
//// IO
#include <iostream>

//// Containers
#include <map>
#include <vector>
#include <queue>

//// Multi-threading
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <functional>

//// DataStructures
#include <exception>
#include <new>
#include <stdexcept>

// C Standard Library
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <time.h>
#include <stddef.h>
#include <arpa/inet.h>

// C++ Library Header
#include "ThreadPool.h"
#include "AVLTree.h"
#include "SingleShotTimer.h"
#include "finite_field.h"

#include "debug.h"

typedef unsigned char u08;
typedef unsigned short u16;
typedef unsigned int u32;
typedef unsigned long long u64;
typedef char s08;
typedef short s16;
typedef int s32;
typedef long long s64;
typedef long laddr;

namespace NetworkCoding{
namespace Header{
struct Common
{
    u08 m_Type;
    enum HeaderType : u08
    {
        DATA = 1,
        SYNC,
        PING,
        DATA_ACK,
        SYNC_ACK,
        PONG
    };
}__attribute__((packed, may_alias));

struct Data : Common
{
    u16 m_TotalSize;
    u16 m_MinBlockSequenceNumber;
    u16 m_CurrentBlockSequenceNumber;
    u16 m_MaxBlockSequenceNumber;
    u08 m_ExpectedRank;
    u08 m_MaximumRank;
    u08 m_Flags;
    enum DataHeaderFlag : u08
    {
        FLAGS_ORIGINAL = 0x1,
        FLAGS_END_OF_BLK = 0x2,
        FLAGS_CONSUMED = 0x4
    };
    u08 m_TxCount;
    enum OffSets : u08
    {
        CodingOffset = (1+2+2+2+2+1+1+1+1)
    };
    // ^^^ This part is not encoded ^^^ //
    u16 m_PayloadSize;
    u08 m_LastIndicator;
    u08 m_Codes[1];
    // ^^^ This part is encoded ^^^ //
}__attribute__((packed, may_alias));

struct DataAck : Common
{
    u16 m_Sequence;
    u08 m_Losses;           /*1*/
}__attribute__((packed, may_alias));

struct Sync : Common
{
    u16 m_Sequence;
}__attribute__((packed, may_alias));

struct Ping : Common
{
    std::chrono::steady_clock::time_point::duration::rep m_PingTime;
}__attribute__((packed, may_alias));

struct Pong : Common
{
    std::chrono::steady_clock::time_point::duration::rep m_PingTime;
}__attribute__((packed, may_alias));

}// namespace Header

namespace Parameter
{
const u08 MAXIMUM_NUMBER_OF_CONCURRENT_RETRANSMISSION = 128;
const u16 MAXIMUM_BUFFER_SIZE = 1500;
const u16 PING_INTERVAL = 500; /* 500 ms */
const double CONNECTION_TIMEOUT = 10.0; /* 10 s */
const u16 MINIMUM_RETRANSMISSION_INTERVAL = 10;/* 10 ms */
enum TRANSMISSION_MODE: u08
{
    RELIABLE_TRANSMISSION_MODE = 0,
    BEST_EFFORT_TRANSMISSION_MODE
};
enum BLOCK_SIZE: u08
{
    INVALID_BLOCK_SIZE = 0,
    BLOCK_SIZE_02 = 2,
    BLOCK_SIZE_04 = 4,
    BLOCK_SIZE_08 = 8,
    BLOCK_SIZE_16 = 16,
    BLOCK_SIZE_32 = 32,
    BLOCK_SIZE_64 = 64
};
const u08 MAX_BLOCK_SIZE = BLOCK_SIZE_64;
}

namespace DataStructures{
class IPv4PortKey
{
public:
    u32 m_IPv4;
    u16 m_Port;
}__attribute__((packed));
}// namespace Parameter

}// namespace NetworkCoding

#endif // COMMON_H
