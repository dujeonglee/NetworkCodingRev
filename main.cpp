#include <stdio.h>
#include <iostream>
#include "c_stub.h"
using namespace std;

bool Done = false;

FILE *p_File = nullptr;
float rxsize = 0;

void RxCallback(uint8_t *const buffer, const uint16_t length, const void *const sockaddr_type, const uint32_t sender_addr_len)
{
    if (p_File == nullptr)
    {
        buffer[length] = 0;
        p_File = fopen((char *)buffer, "w");
    }
    else if (length == 1 && buffer[0] == 0xff)
    {
        Done = true;
    }
    else
    {
        fwrite(buffer, 1, length, p_File);
        rxsize += (float)length;
    }
}

void BandwidthCheck()
{
    while (Done == false)
    {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        printf("%5.5f Mb/s\n", rxsize / (1000. * 1000.));
        fflush(stdout);
        rxsize = 0;
    }
    printf("%5.5f Mb/s\n", rxsize / (1000. * 1000.));
    fflush(stdout);
}

int main(int argc, char *argv[])
{
    if (argc == 2)
    {
        char *local_port;

        std::cout << "Recv Mode" << std::endl;
        local_port = argv[1];

        void *handle = InitSocket(local_port, 500, 500, RxCallback);
        std::thread bwchk = std::thread(BandwidthCheck);
        bwchk.detach();
        while (Done == false)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        fclose(p_File);
        FreeSocket(handle);
    }
    else if (argc == 5)
    {
        char *local_port = nullptr;
        char *remote_ip = nullptr;
        char *remote_port = nullptr;
        unsigned char buffer[1424];
        size_t readbytes;
        FILE *p_File = nullptr;
        std::cout << "Send Mode" << std::endl;
        local_port = argv[1];
        remote_ip = argv[2];
        remote_port = argv[3];
        p_File = fopen(argv[4], "r");
        if (p_File == nullptr)
        {
            std::cout << "Cannot open file " << argv[4] << std::endl;
            return -1;
        }

        void *handle = InitSocket(local_port, 500, 500, nullptr);
        while (false == Connect(handle, remote_ip, remote_port, 1000,
                                NetworkCoding::Parameter::RELIABLE_TRANSMISSION_MODE,
                                NetworkCoding::Parameter::BLOCK_SIZE_32, (uint16_t)0))
            ;
        Send(handle, remote_ip, remote_port, (unsigned char *)argv[4], strlen(argv[4]));
        while ((readbytes = fread(buffer, 1, sizeof(buffer), p_File)) > 0)
        {
            Send(handle, remote_ip, remote_port, buffer, readbytes);
        }

        buffer[0] = 0xff;
        Send(handle, remote_ip, remote_port, buffer, 1);
        WaitUntilTxIsCompleted(handle, remote_ip, remote_port);
        fclose(p_File);
        FreeSocket(handle);
    }
    else
    {
        std::cout << "Send Mode: run localport remoteIP remoteport filename" << std::endl;
        std::cout << "Receive Mode: run localport" << std::endl;
    }
    return 0;
}