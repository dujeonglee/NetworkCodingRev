#ifndef __TX_H__
#define __TX_H__

#include "common.h"

namespace NetworkCoding
{
// Class declarations for transmission
class TransmissionBlock;
class TransmissionSession;
class Transmission;

/**
 * @brief Network coding block
 */
class TransmissionBlock
{
public:
  TransmissionSession *const p_Session;         ///< Pointer of the TransmissionSession object that this block is associated.
  const uint8_t m_BlockSize;                    ///< Block size. @see NetworkCoding::Parameter::BLOCK_SIZE. @warning This member should be constant value.
  const uint8_t m_TransmissionMode;             ///< Mode. @see NetworkCoding::Parameter::TRANSMISSION_MODE. @warning This member should be constant value.
  const uint16_t m_BlockSequenceNumber;         ///< The sequence number of this block. Each block has unique sequence number. The sequence number is incremented by one in the next block. @warning This member should be constant value.
  const uint16_t m_RetransmissionRedundancy;    ///< Maximum number of remedy packets. It is valid only when mode is BEST_EFFORT_TRANSMISSION_MODE. @see NetworkCoding::Parameter::TRANSMISSION_MODE. @warning This member should be constant value.
  uint16_t m_LargestOriginalPacketSize;         ///< The largest packet size in this block, i.e., encoding packet size. @warning m_LargestOriginalPacketSize should be less than MAXIMUM_BUFFER_SIZE.
  uint8_t m_TransmissionCount;                  ///< The nunber of transmission including retranmission.
  uint8_t m_AckedRank;                          ///< Acknowleged rank from receiver. m_AckedRank == m_BlockSize means that receiver has recovered all of the packets in this block.

  uint8_t m_RemedyPacketBuffer[Parameter::MAXIMUM_BUFFER_SIZE];     ///< Remedy packet buffer.
  std::vector<std::unique_ptr<uint8_t[]>> m_OriginalPacketBuffer;   ///< An array of original packets. The size of array should be match with m_BlockSize.

  TransmissionBlock() = delete;
  TransmissionBlock(const TransmissionBlock &) = delete;
  TransmissionBlock(TransmissionSession &&) = delete;
  /**
   * @brief Construct a new TransmissionBlock object
   * @param p_session : Pointer of the TransmissionSession object that this block is associated.
   */
  TransmissionBlock(TransmissionSession *const p_session);
  /**
   * @brief Destroy the Transmission Block object
   */
  ~TransmissionBlock();

  TransmissionBlock &operator=(const TransmissionBlock &) = delete;
  TransmissionBlock &operator=(TransmissionBlock &&) = delete;

  /**
   * @brief Enqueue a packet into a transmission buffer of the relavent session.
   * @see NetworkCoding::TransmissionSession::PushDataPacket
   * @param buffer : Buffer for transmission.
   * @param buffersize : Size of the buffer.
   * @return true : Success
   * @return false : Failure
   */
  bool Send(uint8_t *const buffer, const uint16_t buffersize);
  /**
   * @brief Transmit a remedy packet.
   * @return true : Remedy packet has been successfully transmitted.
   * @return false : Either receiver has received all of the packets for this block or connection is lost.
   */
  const bool Retransmission();
};

/**
 * @brief Network coding session
 * @todo Implement socket bonding on multiple interfaces. { socket, m_RoundTripTime,  m_CongestionWindow, m_BytesUnacked }
 */
class TransmissionSession
{
public:
  Transmission *const c_Transmission;             ///< Pointer of the Transmission object that this session is associated.
  const int32_t c_Socket;                         ///< Socket descriptor used to transmit and receive packets.
  const DataTypes::Address c_Addr;                ///< Remote host's address. @see NetworkCoding::DataTypes::Address
  std::atomic<bool> m_IsConnected;                ///< Boolean variable indicating connection status.
  std::atomic<uint16_t> m_MinBlockSequenceNumber; ///< Minimum block sequence number under transmission.
  std::atomic<uint16_t> m_MaxBlockSequenceNumber; ///< Maximum block sequence number under transmission.
  std::map<int32_t, int16_t> m_AckList;           ///< A pair of block sequence number(int32_t) and acked rank(int16_t).
  std::mutex m_AckListLock;                       ///< Lock for m_AckList.
  uint16_t m_RoundTripTime;                       ///< Measured RTT. It is measured using ping and pong packets. @see NetworkCoding::Header::Ping @see NetworkCoding::Header::Pong
  uint16_t m_RetransmissionRedundancy;            ///< Maximum number of remedy packets. It is valid only when mode is BEST_EFFORT_TRANSMISSION_MODE. @see NetworkCoding::Parameter::TRANSMISSION_MODE.
  uint8_t m_TransmissionMode;                     ///< Mode. @see NetworkCoding::Parameter::TRANSMISSION_MODE.
  uint8_t m_BlockSize;                            ///< Block size. @see NetworkCoding::Parameter::BLOCK_SIZE.
  enum TaskPriority                               ///< Priority of lambda task queued on m_Timer.
  {
    HIGH_PRIORITY = 0,
    MIDDLE_PRIORITY,
    LOW_PRIORITY,
    PRIORITY_LEVELS
  };
  SingleShotTimer<TaskPriority::PRIORITY_LEVELS, 1> m_Timer;    ///< Single-threaded work queue. @see NetworkCoding::TransmissionSession::TaskPriority
  std::atomic<CLOCK::time_point::duration::rep> m_LastPongTime; ///< Last pong time. It is used to measure RTT.
  std::queue<std::tuple<uint8_t *const, const uint16_t, const bool, TransmissionBlock *const>> m_TxQueue; ///< Tx buffer.

  TransmissionBlock *p_TransmissionBlock;                       ///< Pointer of the TransmissionBlock object currently in transmission.
  std::atomic<uint32_t> m_CongestionWindow;                     ///< Congestion window size. It is doubled whenever ack is received within m_RoundTripTime.
  std::atomic<uint32_t> m_BytesUnacked;                         ///< Bytes unacked m_BytesUnacked < m_CongestionWindow.

  /**
   * @brief Construct a new TransmissionSession object
   * @param transmission : Pointer of the Transmission object that this session is associated.
   * @param Socket : Socket descriptor for this session.
   * @param Addr : Remote host's address. @see NetworkCoding::DataTypes::Address.
   * @param TransmissionMode : mode. @see Parameter::TRANSMISSION_MODE (default RELIABLE_TRANSMISSION_MODE).
   * @param BlockSize : The size of block. @see NetworkCoding::Parameter::BLOCK_SIZE (default BLOCK_SIZE_04).
   * @param RetransmissionRedundancy : Retransmission redundancy. It is valid only when mode is BEST_EFFORT_TRANSMISSION_MODE (default 0).
   */
  TransmissionSession(Transmission *const transmission, const int32_t Socket, const DataTypes::Address Addr,
                      const Parameter::TRANSMISSION_MODE TransmissionMode = Parameter::TRANSMISSION_MODE::RELIABLE_TRANSMISSION_MODE,
                      const Parameter::BLOCK_SIZE BlockSize = Parameter::BLOCK_SIZE::BLOCK_SIZE_04,
                      const uint16_t RetransmissionRedundancy = 0);
  TransmissionSession() = delete;
  TransmissionSession(const TransmissionSession &) = delete;
  TransmissionSession(TransmissionSession &&) = delete;
  /**
   * @brief Destroy the TransmissionSession object
   */
  ~TransmissionSession();
  TransmissionSession &operator=(const TransmissionSession &) = delete;
  TransmissionSession &operator=(TransmissionSession &&) = delete;

  /**
   * @brief Change transmission mode. The change will take effect from the next TransmissionBlock.
   * @see NetworkCoding::Parameter::TRANSMISSION_MODE
   * @param TransmissionMode : mode
   */
  void ChangeTransmissionMode(const Parameter::TRANSMISSION_MODE TransmissionMode);
  /**
   * @brief Change block size. The change will take effect from the next TransmissionBlock.
   * @see NetworkCoding::Parameter::BLOCK_SIZE
   * @param BlockSize : block size
   */
  void ChangeBlockSize(const Parameter::BLOCK_SIZE BlockSize);
  /**
   * @brief Change retransmission redundancy.
   *        Retransmission redundancy is only valid for BEST_EFFORT_TRANSMISSION_MODE.
   *        The change will take effect from the next TransmissionBlock.
   * @param RetransmissionRedundancy
   */
  void ChangeRetransmissionRedundancy(const uint16_t RetransmissionRedundancy);
  /**
   * @brief Change the above session parameters at the same time. The changes will take effect from the next TransmissionBlock.
   * @param TransmissionMode : mode
   * @param BlockSize : block size
   * @param RetransmissionRedundancy : Retransmission redundancy
   */
  void ChangeSessionParameter(const Parameter::TRANSMISSION_MODE TransmissionMode, const Parameter::BLOCK_SIZE BlockSize, const uint16_t RetransmissionRedundancy);
  /**
   * @brief Send ping packet to the remote host.
   * @return true : Reschedule ping transmission.
   * @return false : Stop sending ping.
   */
  const bool SendPing();
  /**
   * @brief Process pong packet
   * @param Rtt : Round-trip-time of ping-pong
   */
  void ProcessPong(const uint16_t Rtt);
  /**
   * @brief Process ack
   * @param Rank : Acknowleged rank
   * @param MaxRank : Expected max rank
   * @param Loss : Packet loss measured at receiver
   * @param Sequence : Sequence number of TransmissionBlock of this ack
   */
  void ProcessDataAck(const uint8_t Rank, const uint8_t MaxRank, const uint8_t Loss, const uint16_t Sequence);
  /**
   * @brief Process ack sync.
   * @param sequence : The initial TransmissionBlock's sequence number.
   */
  void ProcessSyncAck(const uint16_t sequence);
  /**
   * @brief Enqueue a packet into m_TxQueue for transmission.
   * @param buffer : The packet to transmit.
   * @param size : Size of the packet
   * @param orig : True (Original packet) False (Encoded packet)
   * @param block : TransmissionBlock that this packet is associated.
   */
  void PushDataPacket(uint8_t *const buffer, const uint16_t size, bool orig, TransmissionBlock *const block);
  /**
   * @brief Dequeue a packet from m_TxQueue and transmit the packet.
   * @return uint16_t : Non-negative size of the transmitted packet. 0 on failure.
   */
  uint16_t PopDataPacket();
};

class Transmission
{
private:
  const int32_t c_Socket;
  AVLTree<DataTypes::SessionKey, TransmissionSession *> m_Sessions;
  std::mutex m_Lock;

public:
  Transmission(const int32_t Socket);
  ~Transmission();

public:
  bool Connect(const DataTypes::Address Addr,
               uint32_t ConnectionTimeout,
               const Parameter::TRANSMISSION_MODE TransmissionMode,
               const Parameter::BLOCK_SIZE BlockSize,
               const uint16_t RetransmissionRedundancy = 0);
  bool Send(const DataTypes::Address Addr, uint8_t *const buffer, const uint16_t buffersize);
  bool Flush(const DataTypes::Address Addr);
  void WaitUntilTxIsCompleted(const DataTypes::Address Addr);
  void Disconnect(const DataTypes::Address Addr);

public:
  void RxHandler(uint8_t *const buffer, const uint16_t size, const sockaddr *const sender_addr, const uint32_t sender_addr_len);
};
} // namespace NetworkCoding

#endif // TX_H
