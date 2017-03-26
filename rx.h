#ifndef RX_H
#define RX_H
#include "common.h"

namespace NetworkCoding
{
class Reception
{
private:
    const s32 c_Socket;
    const std::function <void (u08* buffer, u16 length, sockaddr_in addr)> m_RxCallback;
    //AVLTree< DataStructures::IPv4PortKey , TransmissionSession* > m_Sessions;
    std::mutex m_Lock;
public:
    Reception(s32 Socket, std::function <void (u08* buffer, u16 length, sockaddr_in addr)> rx);
    ~Reception();
public:
    void RxHandler(u08* buffer, u16 size, const sockaddr_in * const sender_addr, const u32 sender_addr_len);
};
}

#endif
