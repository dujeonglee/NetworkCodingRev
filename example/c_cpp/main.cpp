#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <iostream>
#include <thread>
#include "api.h"

using namespace std;

void SendFunction(const char * local_port, const char * remote_ip, const char * remote_port, const char * filename)
{
    void *handle = InitSocket(local_port, 500, 500, nullptr);
    do
    {
        std::cout << "Connect to " << remote_ip << ":" << remote_port << "." << std::endl;
    } while (false == Connect(handle, remote_ip, remote_port, 1000, 0, 32, 0));

    FILE *p_File = nullptr;
    p_File = fopen(filename, "r");
    if (p_File == nullptr)
    {
        std::cout << "Cannot open file " << filename << std::endl;
        exit(-1);
    }

    unsigned char buffer[1024];
    size_t readbytes;
    Send(handle, remote_ip, remote_port, (unsigned char *)filename, strlen(filename));
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

void SyncReceiveFunction(const char *port)
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

void AsyncReceiveFunction(const char *port)
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
            SyncReceiveFunction(argv[2]);
        }
        else if (std::string(argv[1]).compare("Async") == 0)
        {
            AsyncReceiveFunction(argv[2]);
        }
        else
        {
            std::cout << "Invalid receive mode " << argv[1] << std::endl;
        }
    }
    else if (argc == 5)
    {
        std::cout << "Send Mode" << std::endl;
        SendFunction(argv[1], argv[2], argv[3], argv[4]);
    }
    else
    {
        std::cout << "Send Mode: run localport remoteIP remoteport filename" << std::endl;
        std::cout << "Receive Mode: run {Sync, Async} localport" << std::endl;
    }
    return 0;
}
