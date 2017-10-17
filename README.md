# NetworkCodingRev
## Implementatioin of intra-session network coding. (This project deprecates NetworkCoding repo.)
1. It supports both of reliable transmission and best-effort transmission.
2. Packet order is preserved.
3. Unlike tcp, data is not stream. Sender sends X bytes packet then receiver receives X bytes packet.
4. It is easy to use. Please refer to main.cpp.
5. It supports IPv4 and IPv6.
6. Java native interface is supported.
## Example code
```
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <iostream>
#include <thread>
#include "api.h"

using namespace std;

#define USE_RX_CALLBACK (0)

void SyncReceive(const char *port)
{
    std::cout << __FUNCTION__ << std::endl;
    FILE *p_File = nullptr;
    float mbyte_received = 0;
    float total_mbyte_received = 0;
    uint32_t samples = 0;

    uint8_t buffer[1500];
    uint16_t buffer_length = sizeof(buffer);
    union {
        sockaddr_in ip4;
        sockaddr_in6 ip6;
    } addr;
    uint32_t addr_length = sizeof(addr);

    void *handle = InitSocket(port, 500, 500, nullptr);
    const bool ret = Receive(handle, buffer, &buffer_length, &addr, &addr_length, 0);
    if (ret)
    {
        buffer[buffer_length] = 0;
        p_File = fopen((char *)buffer, "w");
        if (p_File)
        {
            std::cout << "Create File " << (char *)buffer << std::endl;
        }
        else
        {
            std::cout << "Cannot create file " << (char *)buffer << std::endl;
            exit(-1);
        }
    }
    std::thread bwchk = std::thread([&p_File, &total_mbyte_received, &mbyte_received, &samples]() {
        while (p_File)
        {
            std::this_thread::sleep_for(std::chrono::seconds(1));
            total_mbyte_received += mbyte_received;
            mbyte_received = 0;
            samples++;
            printf("[AVG %5.5f MB/s][%u seconds]\n", total_mbyte_received / samples, samples);
            fflush(stdout);
        }
    });
    bwchk.detach();
    do
    {
        buffer_length = sizeof(buffer);
        addr_length = sizeof(addr);
        if (Receive(handle, buffer, &buffer_length, &addr, &addr_length, 0))
        {
            if (!(buffer_length == 1 && buffer[0] == 0xff))
            {
                fwrite(buffer, 1, buffer_length, p_File);
                mbyte_received += ((float)buffer_length / (1000000.));
            }
        }

    } while (!(buffer_length == 1 && buffer[0] == 0xff));
    fclose(p_File);
    p_File = nullptr;
    FreeSocket(handle);
    handle = nullptr;
}

void AsyncReceive(const char *port)
{
    std::cout << __FUNCTION__ << std::endl;
    FILE *p_File = nullptr;
    float mbyte_received = 0;
    float total_mbyte_received = 0;
    uint32_t samples = 0;

    bool done = false;
    void *handle = InitSocket(
        port,
        500,
        500,
        [&p_File, &total_mbyte_received, &mbyte_received, &samples, &done](uint8_t *const buffer, const uint16_t length, const void *const address, const uint32_t sender_addr_len) {
            if (p_File == nullptr)
            {
                buffer[length] = 0;
                p_File = fopen((char *)buffer, "w");
                std::thread bwchk = std::thread([&total_mbyte_received, &mbyte_received, &samples, &done]() {
                    while (!done)
                    {
                        std::this_thread::sleep_for(std::chrono::seconds(1));
                        total_mbyte_received += mbyte_received;
                        mbyte_received = 0;
                        samples++;
                        printf("[AVG %5.5f MB/s][%u seconds]\n", total_mbyte_received / samples, samples);
                        fflush(stdout);
                    }
                });
                bwchk.detach();
            }
            else
            {
                if (!(length == 1 && buffer[0] == 0xff))
                {
                    fwrite(buffer, 1, length, p_File);
                    mbyte_received += ((float)length / (1000000.));
                }
                else
                {
                    done = true;
                }
            }
        });
    while (!done)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
    fclose(p_File);
    p_File = nullptr;
    FreeSocket(handle);
    handle = nullptr;
}

int main(int argc, char *argv[])
{
    if (argc == 3)
    {
        std::cout << "Receive Mode" << std::endl;
        if (std::string(argv[1]).compare("Sync") == 0)
        {
            SyncReceive(argv[2]);
        }
        else if (std::string(argv[1]).compare("Async") == 0)
        {
            AsyncReceive(argv[2]);
        }
        else
        {
            std::cout << "Invalid receive mode " << argv[1] << std::endl;
        }
    }
    else if (argc == 5)
    {
        std::cout << "Send Mode" << std::endl;
        void *handle = InitSocket(argv[1], 500, 500, nullptr);
        do
        {
            std::cout << "Connect to " << argv[2] << ":" << argv[3] << "." << std::endl;
        } while (false == Connect(handle, argv[2], argv[3], 1000, 0, 32, 0));

        FILE *p_File = nullptr;
        p_File = fopen(argv[4], "r");
        if (p_File == nullptr)
        {
            std::cout << "Cannot open file " << argv[4] << std::endl;
            exit(-1);
        }

        unsigned char buffer[1424];
        size_t readbytes;
        Send(handle, argv[2], argv[3], (unsigned char *)argv[4], strlen(argv[4]));
        while ((readbytes = fread(buffer, 1, sizeof(buffer), p_File)) > 0)
        {
            Send(handle, argv[2], argv[3], buffer, readbytes);
        }
        buffer[0] = 0xff;
        Send(handle, argv[2], argv[3], buffer, 1);
        WaitUntilTxIsCompleted(handle, argv[2], argv[3]);
        fclose(p_File);
        FreeSocket(handle);
    }
    else
    {
        std::cout << "Send Mode: run localport remoteIP remoteport filename" << std::endl;
        std::cout << "Receive Mode: run {Sync, Async} localport" << std::endl;
    }
    return 0;
}

```
