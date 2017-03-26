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
#include "basiclibrary/threadpool/ThreadPool.h"
#include "basiclibrary/avltree/AVLTree.h"
#include "basiclibrary/singleshottimer/SingleShotTimer.h"
#include "finite_field.h"



#ifdef DEBUG
#define PRINT_ERR(ex)   std::cerr << __PRETTY_FUNCTION__ << ": Failed - " << ex.what() << '\n'
#else
#define PRINT_ERR(ex)
#endif

// Check windows
#if ((ULONG_MAX) == (UINT_MAX))
#define ENVIRONMENT32
#else
#define ENVIRONMENT64
#endif

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
        DATA_ACK,
        SYNC,
        SYNC_ACK
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
    laddr m_AckAddress; // Compatibility of 32 and 64bit machines.
#ifdef ENVIRONMENT32
    laddr m_Reserved;
#endif
    u08 m_Flags;
    enum DataHeaderFlag : u08
    {
        FLAGS_ORIGINAL = 0x1,
        FLAGS_END_OF_BLK = 0x2
    };
    u08 m_TxCount;
    enum OffSets : u08
    {
        CodingOffset = (1+2+2+2+2+1+1+4+4+1+1)
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
    laddr m_AckAddress;    // 32bit machine uses the first 4 bytes and the remaining memory shall be set to 0. This field should be used after casting to laddr.
#ifdef ENVIRONMENT32
    laddr m_Reserved; // 32bit machine uses the first 4 bytes and the remaining memory shall be set to 0. This field should be used after casting to laddr.
#endif
    u08 m_Losses;           /*1*/
}__attribute__((packed, may_alias));

struct Sync : Common
{
    u16 m_Sequence;
    laddr m_SessionAddress;    // 32bit machine uses the first 4 bytes and the remaining memory shall be set to 0. This field should be used after casting to laddr.
#ifdef ENVIRONMENT32
    laddr m_Reserved; // 32bit machine uses the first 4 bytes and the remaining memory shall be set to 0. This field should be used after casting to laddr.
#endif
}__attribute__((packed, may_alias));

}// namespace Header

namespace Parameter
{
const u08 MAXIMUM_NUMBER_OF_CONCURRENT_RETRANSMISSION = 64;
const u16 MAXIMUM_BUFFER_SIZE = 1500;
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
