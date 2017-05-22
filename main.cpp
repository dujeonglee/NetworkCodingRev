#include <stdio.h>
#include <iostream>
#include "ncsocket.h"
using namespace std;

bool Done = false;

int main(int argc, char *argv[])
{

    if(argc == 2)
    {
        unsigned short local_port;
        FILE* p_File = nullptr;
        float rxsize = 0;

        std::cout<<"Recv Mode"<<std::endl;
        sscanf(argv[1], "%hu", &local_port);

        NetworkCoding::NCSocket socket(htons(local_port), 500, 500, [&p_File, &rxsize](unsigned char* buffer, unsigned int length, const sockaddr_in * const sender_addr, const u32 sender_addr_len){
            if(p_File == nullptr)
            {
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
        socket.Send(inet_addr(remote_ip), htons(remote_port), (unsigned char*)argv[4], strlen(argv[4]));
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
