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

//// DataStructures
#include <exception>
#include <new>
#include <stdexcept>

// C Standard Library
#include <netinet/in.h>
#include <sys/socket.h>
#include <time.h>

// C++ Library Header
#include "threadpool.h"
#include "avltree.h"
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
    // ^^^ This part is not encoded ^^^ //
    u16 m_PayloadSize;
    u08 m_LastIndicator;
    u08 m_Codes[1];
    // ^^^ This part is encoded ^^^ //
    enum : u16
    {
        CODING_OFFSET = 17
    };
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
const u08 MAX_CONCURRENCY = 64;
}

namespace DataStructures{
class IPv4PortKey
{
public:
    u32 m_IPv4;
    u16 m_Port;
};

struct PacketBuffer
{
    unsigned char buffer[1500];
}__attribute__((packed, may_alias));
}// namespace Parameter

}// namespace NetworkCoding

#endif // COMMON_H
