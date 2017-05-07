#include <iostream>
#include "ncsocket.h"
using namespace std;

#define RELIABLE_TRANSMISSION_TEST    (1)
#define BEST_EFFORT_TRANSMISSION_TEST (1)
#if BEST_EFFORT_TRANSMISSION_TEST
#define REDUNDANCY                    (3)
#endif

unsigned char reliable_exptected_data = 1;
float reliable_received_packets = 0;
float reliable_sent_packets = 0.;
void ReliableRxCallback(unsigned char* buffer, unsigned int length, const sockaddr_in * const sender_addr, const u32 sender_addr_len)
{
    if(reliable_exptected_data != buffer[1])
    {
        std::cout<<"Relaible Test Failed"<<0+reliable_exptected_data<<":"<<0+buffer[0]<<std::endl;
        exit(-1);
    }
    reliable_exptected_data++;
    reliable_received_packets = reliable_received_packets + 1.;
}

float best_effort_received_packets = 0.;
float best_effort_sent_packets = 0.;
unsigned char session = 0;
void BestEffortRxCallback(unsigned char* buffer, unsigned int length, const sockaddr_in * const sender_addr, const u32 sender_addr_len)
{
    if(session == buffer[0])
    {
        best_effort_received_packets++;
    }
}

int main(int argc, char *argv[])
{
    try
    {
        NetworkCoding::NCSocket socket1(htons(1004), 500, 500, BestEffortRxCallback);
        NetworkCoding::NCSocket socket2(htons(1005), 500, 500, ReliableRxCallback);
        u08 buffer[1024] = {0};

#if RELIABLE_TRANSMISSION_TEST
        while(false == socket1.Connect(inet_addr("127.0.0.1"),
                          htons(1005),
                          1000,
                          NetworkCoding::Parameter::RELIABLE_TRANSMISSION_MODE,
                          NetworkCoding::Parameter::BLOCK_SIZE_04,
                          (u16)0))
        {
            std::cout<<"Retry connect..."<<std::endl;
        }
#endif
#if BEST_EFFORT_TRANSMISSION_TEST
        while(false == socket2.Connect(inet_addr("127.0.0.1"),
                          htons(1004),
                          1000,
                          NetworkCoding::Parameter::BEST_EFFORT_TRANSMISSION_MODE,
                          NetworkCoding::Parameter::BLOCK_SIZE_04,
                          (u16)(REDUNDANCY)))
        {
            std::cout<<"Retry connect..."<<std::endl;
        }
#endif
        bool reliable_disconnected = false;
        bool best_effort_disconnected = false;
        do
        {
            reliable_exptected_data = 1;
            reliable_received_packets = 0;
            reliable_sent_packets = 0;
            best_effort_received_packets = 0.;
            best_effort_sent_packets = 0.;
            buffer[0]++;
            session = buffer[0];
            buffer[1] = 0;
            for(unsigned long i = 0 ; i < 4*1024 ; i++)
            {
                buffer[1]++;
#if RELIABLE_TRANSMISSION_TEST
                if(!reliable_disconnected)
                {
                    if(socket1.Send(inet_addr("127.0.0.1"), htons(1005), buffer, sizeof(buffer)) == false)
                    {
                        std::cout<<"Socket 1 failed\n";
                        reliable_disconnected = true;
                    }
                    else
                    {
                        reliable_sent_packets++;
                    }
                }
#endif
#if BEST_EFFORT_TRANSMISSION_TEST
                if(!best_effort_disconnected)
                {
                    if(socket2.Send(inet_addr("127.0.0.1"), htons(1004), buffer, sizeof(buffer)) == false)
                    {
                        std::cout<<"Socket 2 failed\n";
                        best_effort_disconnected = true;
                    }
                    else
                    {
                        best_effort_sent_packets++;
                    }
                }
#endif
            }
            std::this_thread::sleep_for (std::chrono::milliseconds(1000));
#if RELIABLE_TRANSMISSION_TEST
            if(!reliable_disconnected)
            {
                while(reliable_received_packets != reliable_sent_packets)
                {
                    usleep(1);
                }
                std::cout<<"Relaible Transmission Success"<<std::endl;
            }
#endif
#if BEST_EFFORT_TRANSMISSION_TEST
            if(!best_effort_disconnected)
            {
                std::cout<<"Best effort transmission Loss"<<1.-(best_effort_received_packets/best_effort_sent_packets)<<std::endl;
            }
#endif
        }
        while (1);
    }
    catch(const std::bad_alloc& ex)
    {
        std::cout<<"Creation failed"<<std::endl;
    }

    return 0;
}
