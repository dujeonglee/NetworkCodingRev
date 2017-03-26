#include "rx.h"

using namespace NetworkCoding;

Reception::Reception(s32 Socket, std::function<void(u08* buffer, u16 length, sockaddr_in addr)> rx) : c_Socket(Socket), m_RxCallback(rx)
{

}

Reception::~Reception()
{

}

void Reception::RxHandler(u08* buffer, u16 size, const sockaddr_in * const sender_addr, const u32 sender_addr_len)
{
    Header::Common* CommonHeader = reinterpret_cast< Header::Common* >(buffer);
    switch(CommonHeader->m_Type)
    {
        case Header::Common::HeaderType::DATA:
        {
        }
        break;

        case Header::Common::HeaderType::SYNC:
        {
            // create Rx Session.
            Header::Sync* const sync = reinterpret_cast< Header::Sync* >(buffer);
            sync->m_Type = Header::Common::HeaderType::SYNC_ACK;
            sendto(c_Socket, buffer, size, 0, (sockaddr*)sender_addr, sender_addr_len);
        }
        break;
    default:
        break;
    }

}
