# NetworkCodingRev
## Implementatioin of intra-session network coding. (This project deprecates NetworkCoding repo.)
1. It supports both of reliable transmission and best-effort transmission.
2. Packet order is preserved.
3. Unlike tcp, data is not stream. Sender sends X bytes packet then receiver receives X bytes packet.
4. It is easy to use. Please refer to main.cpp.
5. It supports IPv4 and IPv6. 
## Example code
```
#include <stdio.h>
#include <iostream>
#include "ncsocket.h"
using namespace std;

bool Done = false;

int main(int argc, char *argv[])
{

    if(argc == 2)
    {
        char* local_port;
        FILE* p_File = nullptr;
        float rxsize = 0;

        std::cout<<"Recv Mode"<<std::endl;
        local_port = argv[1];

        NetworkCoding::NCSocket socket(std::string(local_port), 500, 500, [&p_File, &rxsize](unsigned char* buffer, unsigned int length, const sockaddr* const sender_addr, const uint32_t sender_addr_len){
            if(p_File == nullptr)
            {
                buffer[length] = 0;
                p_File = fopen((char*)buffer, "w");
            }
            else if(length == 1 && buffer[0] == 0xff)
            {
                Done = true;
            }
            else
            {
                fwrite(buffer, 1, length, p_File);
                rxsize += (float)length;
            }
        });
        std::thread bwchk = std::thread([&rxsize](){
            while(Done == false)
            {
                std::this_thread::sleep_for(std::chrono::seconds(1));
                printf("%5.5f Mb/s\n", rxsize/(1000.*1000.));
                fflush(stdout);
                rxsize = 0;
            }
            printf("%5.5f Mb/s\n", rxsize/(1000.*1000.));
            fflush(stdout);
        });
        bwchk.detach();
        while(Done == false)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        fclose(p_File);
    }
    else if(argc == 5)
    {
        char* local_port = nullptr;
        char* remote_ip = nullptr;
        char* remote_port = nullptr;
        unsigned char buffer[1424];
        size_t readbytes;
        FILE* p_File = nullptr;
        std::cout<<"Send Mode"<<std::endl;
        local_port = argv[1];
        remote_ip = argv[2];
        remote_port = argv[3];
        p_File = fopen(argv[4], "r");
        if(p_File == nullptr)
        {
            std::cout<<"Cannot open file "<<argv[4]<<std::endl;
            return -1;
        }

        NetworkCoding::NCSocket socket(std::string(local_port), 500, 500, nullptr);
        while(false == socket.Connect(std::string(remote_ip), std::string(remote_port), 1000,
                                      NetworkCoding::Parameter::RELIABLE_TRANSMISSION_MODE,
                                      NetworkCoding::Parameter::BLOCK_SIZE_32, (uint16_t)0));
        socket.Send(std::string(remote_ip), std::string(remote_port), (unsigned char*)argv[4], strlen(argv[4]));
        while((readbytes = fread(buffer, 1, sizeof(buffer), p_File)) > 0)
        {
            socket.Send(std::string(remote_ip), std::string(remote_port), buffer, readbytes);
        }

        buffer[0] = 0xff;
        socket.Send(std::string(remote_ip), std::string(remote_port), buffer, 1);
        socket.WaitUntilTxIsCompleted(std::string(remote_ip), std::string(remote_port));
        fclose(p_File);
    }
    else
    {
        std::cout<<"Send Mode: run localport remoteIP remoteport filename"<<std::endl;
        std::cout<<"Receive Mode: run localport"<<std::endl;
    }
    return 0;
}
```
