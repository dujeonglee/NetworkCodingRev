#include <stdio.h>
#include <iostream>
#include "ncsocket.h"
using namespace std;

bool Done = false;
FILE* p_Output;
void ReliableRxCallback(unsigned char* buffer, unsigned int length, const sockaddr_in * const sender_addr, const u32 sender_addr_len)
{

    if(length == 1 && buffer[0] == 0xff)
    {
        Done = true;
    }
    else
    {
        fwrite(buffer, 1, length, p_Output);
    }
}

int main(int argc, char *argv[])
{

    if(argc == 2)
    {
        unsigned short local_port;
        std::cout<<"Recv Mode"<<std::endl;
        sscanf(argv[1], "%hu", &local_port);

        p_Output = fopen("output.txt", "w");
        NetworkCoding::NCSocket socket(htons(local_port), 500, 500, ReliableRxCallback);
        while(Done == false)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        fclose(p_Output);
    }
    else if(argc == 5)
    {
        unsigned short local_port;
        char* remote_ip = nullptr;
        unsigned short remote_port;
        unsigned char buffer[1024];
        size_t readbytes;
        FILE* p_File = nullptr;
        std::cout<<"Send Mode"<<std::endl;
        sscanf(argv[1], "%hu", &local_port);
        remote_ip = argv[2];
        sscanf(argv[3], "%hu", &remote_port);
        p_File = fopen(argv[4], "r");
        if(p_File == nullptr)
        {
            std::cout<<"Cannot open file "<<argv[4]<<std::endl;
            return -1;
        }

        NetworkCoding::NCSocket socket(htons(local_port), 500, 500, nullptr);
        while(false == socket.Connect(inet_addr(remote_ip), htons(remote_port), 1000, NetworkCoding::Parameter::RELIABLE_TRANSMISSION_MODE, NetworkCoding::Parameter::BLOCK_SIZE_16, (u16)0));
        while((readbytes = fread(buffer, 1, sizeof(buffer), p_File)) > 0)
        {
            socket.Send(inet_addr(remote_ip), htons(remote_port), buffer, readbytes);
        }

        buffer[0] = 0xff;
        socket.Send(inet_addr(remote_ip), htons(remote_port), buffer, 1);
        socket.WaitUntilTxIsCompleted(inet_addr(remote_ip), htons(remote_port));
        fclose(p_File);
    }
    else
    {
        std::cout<<"Send Mode: run localport remoteIP remoteport filename"<<std::endl;
        std::cout<<"Receive Mode: run localport"<<std::endl;
    }
    return 0;
}
