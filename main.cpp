#include <iostream>
#include "ncsocket.h"
using namespace std;


void RxCallback(unsigned char* buffer, unsigned int length, const sockaddr_in * const sender_addr, const u32 sender_addr_len)
{
    printf("Get %hhu\n", buffer[0]);
}

int main(int argc, char *argv[])
{
    NetworkCoding::NCSocket socket1(htons(1004), 500, 500, RxCallback);
    NetworkCoding::NCSocket socket2(htons(1005), 500, 500, RxCallback);
    u08 buffer[1024] = {0};
    {
        if(socket1.Connect(inet_addr("127.0.0.1"),
                          htons(1005),
                          3000,
                          NetworkCoding::Parameter::RELIABLE_TRANSMISSION_MODE,
                          NetworkCoding::Parameter::BLOCK_SIZE_04,
                          (u16)0,
                          (u16)10) == false)
        {
            std::cout<<"Failed\n";
            return -1;
        }
        else
        {
            std::cout<<"Connected\n";
        }
    }
    while(1)
    {
        buffer[0]++;
        const auto result = socket1.Send(inet_addr("127.0.0.1"), htons(1005), buffer, sizeof(buffer), false);
        if(result == false)
        {
            std::cout<<"Send failed\n";
            socket1.Disconnect(inet_addr("127.0.0.1"), htons(1005));
        }
        std::this_thread::sleep_for (std::chrono::milliseconds(100));
    }
    return 0;
}
