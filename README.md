# NetworkCodingRev
## Implementatioin of intra-session network coding. (This project deprecates NetworkCoding repo.)
1. It supports both of reliable transmission and best-effort transmission.
2. Packet order is preserved.
3. Unlike tcp, data is not stream. Sender sends X bytes packet then receiver receives X bytes packet.
4. It is easy to use. Please refer to main.cpp.
5. It supports IPv4 and IPv6. 
## Example code
```
/*
Example code using static and shared library.
*/
#include <stdio.h>
#include <string.h>
#include <iostream>
#include <thread>
#include "api.h"
#ifdef DYNAMIC
#include <dlfcn.h>
#endif

using namespace std;

bool Done = false;

FILE *p_File = nullptr;
float rxsize = 0;
float cummulative_bandwith = 0;
uint32_t samples = 0;

void RxCallback(uint8_t *const buffer, const uint16_t length, const void *const sockaddr_type, const uint32_t sender_addr_len)
{
    if (p_File == nullptr)
    {
        buffer[length] = 0;
        std::cout << "File " << (char *)buffer << std::endl;
        p_File = fopen((char *)buffer, "w");
    }
    else if (length == 1 && buffer[0] == 0xff)
    {
        std::cout << "Done" << std::endl;
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
        if (p_File)
        {
            const float sample_bandwidth = rxsize / (1000. * 1000.);
            rxsize = 0;
            cummulative_bandwith += sample_bandwidth;
            samples++;
            printf("[AVG %5.5f MB/s][%u seconds]\n", cummulative_bandwith / samples, samples);
            fflush(stdout);
        }
    }
    printf("[AVG %5.5f MB/s][%u seconds]\n", cummulative_bandwith / samples, samples);
    fflush(stdout);
}

int main(int argc, char *argv[])
{
    if (argc == 2)
    {
        const char *const local_port = argv[1];
        std::cout << "Recv Mode" << std::endl;
#ifndef DYNAMIC
        std::cout << "Static" << std::endl;
        void *handle = InitSocket(local_port, 500, 500, RxCallback);
#else
        std::cout << "Dynamic" << std::endl;
        void *pLibHandle = dlopen("./libncsocket.so", RTLD_LAZY);
        void *handle = ((InitSocketFuncType)dlsym(pLibHandle, "InitSocket"))(local_port, 500, 500, RxCallback);
#endif
        std::thread bwchk = std::thread(BandwidthCheck);
        bwchk.detach();
        while (Done == false)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        fclose(p_File);
#ifndef DYNAMIC
        FreeSocket(handle);
#else
        ((FreeSocketFuncType)dlsym(pLibHandle, "FreeSocket"))(handle);
        dlclose(pLibHandle);
#endif
    }
    else if (argc == 5)
    {
        const char *const local_port = argv[1];
        const char *const remote_ip = argv[2];
        const char *const remote_port = argv[3];
        unsigned char buffer[1424];
        size_t readbytes;
        FILE *p_File = nullptr;
        std::cout << "Send Mode" << std::endl;
        p_File = fopen(argv[4], "r");
        if (p_File == nullptr)
        {
            std::cout << "Cannot open file " << argv[4] << std::endl;
            return -1;
        }
#ifndef DYNAMIC
        std::cout << "Static" << std::endl;
        void *handle = InitSocket(local_port, 500, 500, nullptr);
        do
        {

        } while (false == Connect(handle, remote_ip, remote_port, 1000, 0, 32, 0));
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
#else
        std::cout << "Dynamic" << std::endl;
        void *pLibHandle = dlopen("./libncsocket.so", RTLD_LAZY);
        void *handle = ((InitSocketFuncType)dlsym(pLibHandle, "InitSocket"))(local_port, 500, 500, RxCallback);
        do
        {

        } while (false == ((ConnectFuncType)dlsym(pLibHandle, "Connect"))(handle, remote_ip, remote_port, 1000, 0, 32, 0));
        ((SendFuncType)dlsym(pLibHandle, "Send"))(handle, remote_ip, remote_port, (unsigned char *)argv[4], strlen(argv[4]));
        while ((readbytes = fread(buffer, 1, sizeof(buffer), p_File)) > 0)
        {
            ((SendFuncType)dlsym(pLibHandle, "Send"))(handle, remote_ip, remote_port, buffer, readbytes);
        }
        buffer[0] = 0xff;
        ((SendFuncType)dlsym(pLibHandle, "Send"))(handle, remote_ip, remote_port, buffer, 1);
        ((WaitUntilTxIsCompletedFuncType)dlsym(pLibHandle, "WaitUntilTxIsCompleted"))(handle, remote_ip, remote_port);
        fclose(p_File);
        ((FreeSocketFuncType)dlsym(pLibHandle, "FreeSocket"))(handle);
        dlclose(pLibHandle);
#endif
    }
    else
    {
        std::cout << "Send Mode: run localport remoteIP remoteport filename" << std::endl;
        std::cout << "Receive Mode: run localport" << std::endl;
    }
    return 0;
}
```
