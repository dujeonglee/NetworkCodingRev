#include <iostream>
#include "ncsocket.h"
using namespace std;

void RxCallback(unsigned char* buffer, unsigned int length, sockaddr_in addr)
{

}

int main(int argc, char *argv[])
{
    u08 buffer[1024] = {0};
    NetworkCoding::NCSocket<1004, 500, 500> socket(RxCallback);
    {
        socket.Connect(inet_addr("127.0.0.1"),
                           htons(1004),
                           NetworkCoding::Parameter::RELIABLE_TRANSMISSION_MODE,
                           NetworkCoding::Parameter::BLOCK_SIZE_04,
                           0,
                           1000);
    }

    {
        const bool result = socket.Send(inet_addr("127.0.0.1"),
                                        htons(1004),buffer, sizeof(buffer), false);
    }
    {
        const bool result = socket.Send(inet_addr("127.0.0.1"),
                                        htons(1004),buffer, sizeof(buffer), false);
    }
    {
        const bool result = socket.Send(inet_addr("127.0.0.1"),
                                        htons(1004),buffer, sizeof(buffer), false);
    }
    {
        const bool result = socket.Send(inet_addr("127.0.0.1"),
                                        htons(1004),buffer, sizeof(buffer), false);
    }

    while(1);
    return 0;
}
