#include <stdio.h>
#include <iostream>
#include "ncsocket.h"
using namespace std;

void ReliableRxCallback(unsigned char* buffer, unsigned int length, const sockaddr_in * const sender_addr, const u32 sender_addr_len)
{
	std::cout<<"Receive Pakcet: "<<length<<std::endl;
}

int main(int argc, char *argv[])
{

	if(argc == 2)
	{
		unsigned short local_port;
		std::cout<<"Recv Mode"<<std::endl;
		sscanf(argv[1], "%hu", &local_port);

		NetworkCoding::NCSocket socket(htons(local_port), 500, 500, ReliableRxCallback);
		while(1);
	}
	else if(argc == 4)
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
		socket.Connect(inet_addr(remote_ip), htons(remote_port), 1000, NetworkCoding::Parameter::RELIABLE_TRANSMISSION_MODE, NetworkCoding::Parameter::BLOCK_SIZE_04, (u16)0);
		while((readbytes = fread(buffer, 1, sizeof(buffer), p_File)) > 0)
		{
			socket.Send(inet_addr(remote_ip), htons(remote_port), buffer, readbytes);
		}
		socket.Flush(inet_addr(remote_ip), htons(remote_port));
		socket.WaitUntilTxIsCompleted(inet_addr(remote_ip), htons(remote_port));
    }
	else
	{
		std::cout<<"Send Mode: run localport remoteIP remoteport filename"<<std::endl;
		std::cout<<"Receive Mode: run localport"<<std::endl;
	}
    return 0;
}
