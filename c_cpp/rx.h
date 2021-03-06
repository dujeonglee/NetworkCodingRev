#ifndef RX_H
#define RX_H

#include "common.h"
#include "ThreadPool.h"

namespace NetworkCoding
{
class ReceptionBlock;
class ReceptionSession;
class Reception;

class ReceptionBlock
{
  friend class ReceptionSession;
  friend class Reception;

private:
  Reception *const c_Reception;
  ReceptionSession *const c_Session;
  const uint16_t m_BlockSequenceNumber;
  std::vector<std::unique_ptr<uint8_t[]>> m_DecodedPacketBuffer;
  std::vector<std::unique_ptr<uint8_t[]>> m_EncodedPacketBuffer;
  std::vector<std::unique_ptr<uint8_t[]>> m_DecodingMatrix;
  bool m_DecodingReady;
  const uint8_t FindMaximumRank(const Header::Data *const hdr = nullptr);
  const bool FindEndOfBlock(const Header::Data *const hdr = nullptr);
  enum ReceiveAction
  {
    DROP = 0,
    ENQUEUE_AND_DECODING,
    DECODING,
  };

  ReceiveAction FindAction(const uint8_t *const buffer, const uint16_t length);
  bool Decoding();

public:
  ReceptionBlock() = delete;
  ReceptionBlock(const ReceptionBlock &) = delete;
  ReceptionBlock(ReceptionBlock &&) = delete;
  ReceptionBlock &operator=(const ReceptionBlock &) = delete;
  ReceptionBlock &operator=(ReceptionBlock &&) = delete;

  ReceptionBlock(Reception *const reception, ReceptionSession *const session, const uint16_t BlockSequenceNumber);
  ~ReceptionBlock();
  void Receive(uint8_t *const buffer, const uint16_t length, const sockaddr *const sender_addr, const uint32_t sender_addr_len);
};

class ReceptionSession
{
  friend class ReceptionBlock;
  friend class Reception;

private:
  Reception *const c_Reception;
  AVLTree<uint16_t, ReceptionBlock *> m_Blocks;
  ThreadPool<1, 1> m_RxTaskQueue;
  uint16_t m_SequenceNumberForService;
  uint16_t m_MinSequenceNumberAwaitingAck;
  uint16_t m_MaxSequenceNumberAwaitingAck;
  const DataTypes::Address m_SenderAddress;

public:
  ReceptionSession(const ReceptionSession &) = delete;
  ReceptionSession(ReceptionSession &&) = delete;
  ReceptionSession &operator=(const ReceptionSession &) = delete;
  ReceptionSession &operator=(ReceptionSession &&) = delete;

  ReceptionSession(Reception *const Session, const DataTypes::Address addr);
  ~ReceptionSession();
  void SendDataAck(const Header::Data *const header, const sockaddr *const sender_addr, const uint32_t sender_addr_len, const uint8_t completed = 1);
  void Receive(uint8_t *const buffer, const uint16_t length, const sockaddr *const sender_addr, const uint32_t sender_addr_len);
};

class Reception
{
  friend class ReceptionBlock;
  friend class ReceptionSession;

private:
  const int32_t c_Socket;
  AVLTree<DataTypes::SessionKey, ReceptionSession *> m_Sessions;

public:
  const std::function<void(uint8_t *const buffer, const uint16_t length, const sockaddr *const sender_addr, const uint32_t sender_addr_len)> m_RxCallback;
  std::mutex m_PacketQueueLock;
  std::condition_variable m_Condition;
  std::deque<std::tuple<DataTypes::Address, uint8_t *>> m_PacketQueue;
  Reception(const int32_t Socket, const std::function<void(uint8_t *buffer, uint16_t length, const sockaddr *const sender_addr, const uint32_t sender_addr_len)> rx);
  ~Reception();

public:
  void RxHandler(uint8_t *const buffer, const uint16_t size, const sockaddr *const sender_addr, const uint32_t sender_addr_len);
  bool Receive(uint8_t *const buffer, uint16_t *const length, sockaddr *const sender_addr, uint32_t *const sender_addr_len, uint32_t timeout);
};
}

#endif
