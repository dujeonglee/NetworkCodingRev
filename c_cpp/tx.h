#ifndef TX_H
#define TX_H

#include "common.h"

namespace NetworkCoding
{
// Class declarations for transmission
class TransmissionBlock;
class TransmissionSession;
class Transmission;

class TransmissionBlock
{
public:
  TransmissionSession *const p_Session;
  const uint8_t m_BlockSize;
  const uint8_t m_TransmissionMode;
  const uint16_t m_BlockSequenceNumber;
  const uint16_t m_RetransmissionRedundancy;
  uint16_t m_LargestOriginalPacketSize;
  uint8_t m_TransmissionCount;

  uint8_t m_RemedyPacketBuffer[Parameter::MAXIMUM_BUFFER_SIZE];
  std::vector<std::unique_ptr<uint8_t[]>> m_OriginalPacketBuffer;

  TransmissionBlock() = delete;
  TransmissionBlock(const TransmissionBlock &) = delete;
  TransmissionBlock(TransmissionSession &&) = delete;
  TransmissionBlock(TransmissionSession *const p_session);
  ~TransmissionBlock();

  TransmissionBlock &operator=(const TransmissionBlock &) = delete;
  TransmissionBlock &operator=(TransmissionBlock &&) = delete;

  const uint16_t AckIndex();
  bool Send(uint8_t *const buffer, const uint16_t buffersize);
  const bool Retransmission();
};

class TransmissionSession
{
public:
  Transmission *const c_Transmission;
  const int32_t c_Socket;
  const DataStructures::AddressType c_Addr;
  std::atomic<bool> m_IsConnected;
  std::atomic<uint16_t> m_MinBlockSequenceNumber;
  std::atomic<uint16_t> m_MaxBlockSequenceNumber;
  std::atomic<bool> m_AckList[Parameter::MAXIMUM_NUMBER_OF_CONCURRENT_RETRANSMISSION * 2];
  uint16_t m_RetransmissionRedundancy;
  uint16_t m_RetransmissionInterval;
  uint8_t m_TransmissionMode;
  uint8_t m_BlockSize;
  enum TaskPriority
  {
    HIGH_PRIORITY = 0,
    MIDDLE_PRIORITY,
    LOW_PRIORITY,
    PRIORITY_LEVELS
  };
  SingleShotTimer<TaskPriority::PRIORITY_LEVELS, 1> m_Timer;
  std::atomic<CLOCK::time_point::duration::rep> m_LastPongTime;

  TransmissionBlock *p_TransmissionBlock;
  std::atomic<uint32_t> m_ConcurrentRetransmissions;

  TransmissionSession(Transmission *const transmission, const int32_t Socket, const DataStructures::AddressType Addr,
                      const Parameter::TRANSMISSION_MODE TransmissionMode = Parameter::TRANSMISSION_MODE::RELIABLE_TRANSMISSION_MODE,
                      const Parameter::BLOCK_SIZE BlockSize = Parameter::BLOCK_SIZE::BLOCK_SIZE_04,
                      const uint16_t RetransmissionRedundancy = 0);
  TransmissionSession() = delete;
  TransmissionSession(const TransmissionSession &) = delete;
  TransmissionSession(TransmissionSession &&) = delete;
  ~TransmissionSession();
  TransmissionSession &operator=(const TransmissionSession &) = delete;
  TransmissionSession &operator=(TransmissionSession &&) = delete;

  void ChangeTransmissionMode(const Parameter::TRANSMISSION_MODE TransmissionMode);
  void ChangeBlockSize(const Parameter::BLOCK_SIZE BlockSize);
  void ChangeRetransmissionRedundancy(const uint16_t RetransmissionRedundancy);
  void ChangeSessionParameter(const Parameter::TRANSMISSION_MODE TransmissionMode, const Parameter::BLOCK_SIZE BlockSize, const uint16_t RetransmissionRedundancy);
  const bool SendPing();
  void ProcessPong(const uint16_t rtt);
  void ProcessDataAck(const uint16_t sequence, const uint8_t loss);
  void ProcessSyncAck(const uint16_t sequence);
};

class Transmission
{
private:
  const int32_t c_Socket;
  AVLTree<DataStructures::SessionKey, TransmissionSession *> m_Sessions;
  std::mutex m_Lock;

public:
  Transmission(const int32_t Socket);
  ~Transmission();

public:
  bool Connect(const DataStructures::AddressType Addr,
               uint32_t ConnectionTimeout,
               const Parameter::TRANSMISSION_MODE TransmissionMode,
               const Parameter::BLOCK_SIZE BlockSize,
               const uint16_t RetransmissionRedundancy = 0);
  bool Send(const DataStructures::AddressType Addr, uint8_t *const buffer, const uint16_t buffersize);
  bool Flush(const DataStructures::AddressType Addr);
  void WaitUntilTxIsCompleted(const DataStructures::AddressType Addr);
  void Disconnect(const DataStructures::AddressType Addr);

public:
  void RxHandler(uint8_t *const buffer, const uint16_t size, const sockaddr *const sender_addr, const uint32_t sender_addr_len);
};
}

#endif // TX_H
