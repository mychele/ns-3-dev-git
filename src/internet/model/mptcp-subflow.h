/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2015 University of Sussex
 * Copyright (c) 2015 Université Pierre et Marie Curie (UPMC)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author:  Matthieu Coudron <matthieu.coudron@lip6.fr>
 *          Morteza Kheirkhah <m.kheirkhah@sussex.ac.uk>
 */
#ifndef MPTCP_SUBFLOW_H
#define MPTCP_SUBFLOW_H

#include <stdint.h>
#include <vector>
#include <queue>
#include <list>
#include <set>
#include <map>
#include "ns3/object.h"
#include "ns3/uinteger.h"
#include "ns3/traced-value.h"
#include "ns3/trace-source-accessor.h"
#include "ns3/sequence-number.h"
#include "ns3/rtt-estimator.h"
#include "ns3/event-id.h"
#include "ns3/packet.h"
#include "ns3/tcp-socket.h"
#include "ns3/ipv4-end-point.h"
#include "ns3/ipv4-address.h"
#include "ns3/tcp-socket-base.h"
#include "ns3/tcp-header.h"
#include "ns3/mptcp-mapping.h"
#include "ns3/tcp-option-mptcp.h"
//#include "ns3/tcp-option-mptcp.h"

using namespace std;

namespace ns3
{

class MpTcpSocketBase;
class MpTcpPathIdManager;
class TcpOptionMpTcpDSS;
class TcpOptionMpTcp;
class TcpOptionMpTcpDeltaOWD;
class TcpOptionMpTcpOwdTimeStamp;
class SubflowPair;

enum OwdEstimator {
LocalOwdEstimator = 0,
RemoteOwdEstimator = 1,
};

/**
 * \class MpTcpSubflow
*/
class MpTcpSubflow : public TcpSocketBase
{
public:

  static TypeId
  GetTypeId(void);

  virtual TypeId GetInstanceTypeId(void) const;


  /**
  * Todo remove, exists in Socket ?
  */
  uint32_t GetTxAvailable() const;


  /**
  the metasocket is the socket the application is talking to.
  Every subflow is linked to that socket.
  \param The metasocket it is linked to
  **/
  MpTcpSubflow();

  MpTcpSubflow(const TcpSocketBase& sock);
  MpTcpSubflow(const MpTcpSubflow&);
  virtual ~MpTcpSubflow();

//  TcpStates_t
//  GetState() const;

  virtual uint32_t
  UnAckDataCount();
  virtual uint32_t
  BytesInFlight();
  virtual uint32_t
  AvailableWindow();
  virtual uint32_t
  Window(void);               // Return the max possible number of unacked bytes
  // Undefined for now
//  virtual uint32_t
//  AvailableWindow(void);      // Return unfilled portion of window

  /**
   *
   */
  uint8_t GetLocalId () const;

  /**
   *
   */
  uint8_t GetRemoteId () const;

  /**
   * will update the meta rwnd. Called by subflows whose
   * \return true
  */
  virtual bool UpdateWindowSize (const TcpHeader& header);

  /**
  \return Value advertised by the meta socket
  */
  virtual uint16_t
  AdvertisedWindowSize(void);

  /**
   * \param metaSocket
   */
  virtual void
  SetMeta(Ptr<MpTcpSocketBase> metaSocket);
//  virtual int
//  Connect(const Address &address);      // Setup endpoint and call ProcessAction() to connect

//  static uint32_t
//  GenerateTokenForKey(uint64_t key);

//  uint8_t addrId,
  /**
  \warning for prototyping purposes, we let the user free to advertise an IP that doesn't belong to the node
  (in reference to MPTCP connection agility).
  \note Maybe we should change this behavior ?
  TODO convert to Address to work with IPv6
  */
  virtual void
  AdvertiseAddress(Ipv4Address , uint16_t port);


  /**
  \param dack left edge window at connection level
  \param ack
  \param mapping
  **/
  virtual bool
  DiscardAtMostOneTxMapping(SequenceNumber64 const& dack,
    MpTcpMapping& mapping);

  /**
  \brief Send a REM_ADDR for the specific address.
  \see AdvertiseAddress
  \return false if no id associated with the address which likely means it was never advertised in the first place
  */
  virtual bool
  StopAdvertisingAddress(Ipv4Address);

  /**
   * \brief This is important. This should first request data from the meta
   */
  virtual void
  NotifySend (uint32_t spaceAvailable);

  /**
   * for debug
   */
  void DumpInfo() const;
  virtual void Dump (std::ostream &os) const;

  /**
  \brief
  \note A Master socket is the first to initiate the connection, thus it will use the option MP_CAPABLE
      during the 3WHS while any additionnal subflow must resort to the MP_JOIN option
  \return True if this subflow is the first (should be unique) subflow attempting to connect
  **/
  virtual bool
  IsMaster() const;

  /**
  \return True if this subflow shall be used only when all the regular ones failed
  */
  virtual bool
  BackupSubflow() const;

  /**
  @brief According to rfc6824, the token is used to identify the MPTCP connection and is a
   cryptographic hash of the receiver's key, as exchanged in the initial
   MP_CAPABLE handshake (Section 3.1).  In this specification, the
   tokens presented in this option are generated by the SHA-1 algorithm, truncated to the most significant 32 bits.
  */
//  virtual uint32_t
//  GetLocalToken() const;

//  virtual void
//  DoForwardUp(Ptr<Packet> packet, Ipv4Header header, uint16_t port, Ptr<Ipv4Interface> incomingInterface);

  virtual bool
  SendPendingData(bool withAck = false);


  /**
  Disabled for now.
  SendMapping should be used instead.
  **/
  int
  Send(Ptr<Packet> p, uint32_t flags);

  //! disabled
  Ptr<Packet>
  RecvFrom(uint32_t maxSize, uint32_t flags, Address &fromAddress);

  //! disabled
  Ptr<Packet>
  Recv(uint32_t maxSize, uint32_t flags);

  //! Disabled
  Ptr<Packet>
  Recv(void);

  /**
  *
  * \param dsn will set the dsn of the beginning of the data
  * \param only_full_mappings Set to true if you want to extract only packets that match a whole mapping
  * \param dsn returns the head DSN of the returned packet
  *
  * \return this can return an EmptyPacket if on close
  * Use a maxsize param ? if buffers linked then useless ?
  *
  */
  virtual Ptr<Packet>
  ExtractAtMostOneMapping (uint32_t maxSize, bool only_full_mappings, SequenceNumber64 *dsn);

  //  RecvWithMapping(uint32_t maxSize, bool only_full_mappings, SequenceNumber32 &dsn);
//  Ptr<Packet>

  //! TODO should notify upper layer
//  virtual void
//  PeerClose(Ptr<Packet>, const TcpHeader&); // Received a FIN from peer, notify rx buffer
//  virtual void
//  DoPeerClose(void); // FIN is in sequence, notify app and respond with a FIN

  /**
  TODO used in previous implementation to notify meta of subflow closing.
  See it that's worth keeping
   */
  virtual void
  ClosingOnEmpty(TcpHeader& header);

  virtual void
  DeallocateEndPoint(void);

  /*
  TODO move to meta.
  This should generate an *absolute*
  mapping with 64bits DSN etc...

  */
//  virtual void ParseDSS(Ptr<Packet> p, const TcpHeader& header, Ptr<TcpOptionMpTcpDSS> dss);

    // State transition functions
//  virtual void
//  ProcessEstablished(Ptr<Packet>, const TcpHeader&); // Received a packet upon ESTABLISHED state

  /**
  * \
  * Why do I need this already :/ ?
  */
//  virtual Ptr<MpTcpSubflow>
//  ForkAsSubflow(void) = 0;

  /**
  * This should
  */
  virtual void NewAck(SequenceNumber32 const& ack);

  virtual void TimeWait();

  virtual void DoRetransmit();

/**
TODO move this up to TcpSocketBase
**/
//  virtual void
//  SetRemoteWindow(uint32_t );
//  virtual uint32_t
//  RemoteWindow();

  virtual int Listen(void);
  /**
  TODO some options should be forwarded to the meta socket
  */
//  bool ReadOptions(Ptr<Packet> pkt, const TcpHeader& mptcpHeader);

  // TODO ? Moved from meta
  //  void ProcessListen  (uint8_t sFlowIdx, Ptr<Packet>, const TcpHeader&, const Address&, const Address&);
  void
  ProcessListen(Ptr<Packet> packet, const TcpHeader& tcpHeader, const Address& fromAddress, const Address& toAddress);

  /**
   * Will send MP_JOIN or MP_CAPABLE depending on if it is master or not
   * Updates the meta endpoint
   *
   * \see TcpSocketBase::CompleteFork
   */
  virtual void
  CompleteFork(Ptr<const Packet> p, const TcpHeader& h, const Address& fromAddress, const Address& toAddress);

  virtual void
  ProcessSynRcvd(Ptr<Packet> packet, const TcpHeader& tcpHeader, const Address& fromAddress,
    const Address& toAddress);

  virtual void ProcessSynSent(Ptr<Packet> packet, const TcpHeader& tcpHeader);
  virtual void ProcessWait(Ptr<Packet> packet, const TcpHeader& tcpHeader);

  virtual void UpdateTxBuffer();
  /**
  */
  virtual void Retransmit(void);

  /**
   * Parse DSS essentially

   */
//  virtual int ProcessOptionMpTcpEstablished(const Ptr<const TcpOption> option);
  virtual int ProcessOptionMpTcpDSSEstablished(const Ptr<const TcpOptionMpTcpDSS> option);
  virtual int ProcessOptionMpTcpJoin(const Ptr<const TcpOptionMpTcp> option);
  virtual int ProcessOptionMpTcpCapable(const Ptr<const TcpOptionMpTcp> option);
//  virtual int ProcessTcpOptionMpTcpDSS(Ptr<const TcpOptionMpTcpDSS> dss);

  /** WIP TODO move this one to MptcpSubflowOwd */
  int ProcessOptionMpTcpDeltaOWD (const Ptr<const TcpOptionMpTcpDeltaOWD> option);
  int ProcessOptionMpTcpOwdTimeStamp (const Ptr<const TcpOptionMpTcpOwdTimeStamp> option);


    // TODO rename to get remote or local
  Ptr<MpTcpPathIdManager> GetIdManager ();

  /**
  Temporary, for debug
  **/
//  void
//  SetupTracing(const std::string prefix);
//  MpTcpMapping getSegmentOfACK( uint32_t ack);


protected:
  friend class MpTcpSocketBase;

  //

  /**
  * This is a public function in TcpSocketBase but it shouldn't be public here !
  **/
  virtual int Close(void);


  /**
  TODO move to TcpSocketBase. Split  SendDataPacket into several functions ?
  TODO remove
  */
  void GenerateDataPacketHeader(TcpHeader& header, SequenceNumber32 seq, uint32_t maxSize, bool withAck);


  virtual void
  CloseAndNotify(void);

  ///// Mappings related
  /**
  * \param mapping
  * \todo should check if mappings intersect, already exist etc...
  */
//  virtual bool AddPeerMapping(const MpTcpMapping& mapping);


  virtual void GetMappedButMissingData(
                std::set< MpTcpMapping >& missing
                );

  /**
   * Depending on if this subflow is master or not, we want to
   * trigger
   * Callbacks being private members
   * Overrides parent in order to warn meta
   **/
  virtual void ConnectionSucceeded(void);

//
//  /**
//   * \brief NOOP. Initial cwnd should be set by meta.
//   */
//  virtual void
//  SetSSThresh(uint32_t threshold);
//
//
//  /**
//   * \brief NOOP. Initial cwnd should be set by meta.
//   */
//  virtual void
//  SetInitialCwnd(uint32_t cwnd);


  /**
   * \return Value defined by meta socket GetInitialCwnd
   */
//  virtual uint32_t
//  GetInitialCwnd(void) const;

  int
  DoConnect();

  /**
  * TODO in fact, instead of relying on IsMaster etc...
  * this should depend on meta's state , if it is wait or not
  * and thus it should be pushed in meta (would also remove the need for crypto accessors)
  */
  virtual void
  AddOptionMpTcp3WHS(TcpHeader& hdr) const;

  // TO remove hopefully
  virtual void
  ProcessClosing(Ptr<Packet> packet, const TcpHeader& tcpHeader);


  /**
   * \brief Should be able to process any kind of MPTCP options.
   *
   * \warn When overriding this member function, call it last as
   *       it throws a fatal exception upon unknown options
   */
  virtual int ProcessOptionMpTcp (const Ptr<const TcpOption> option);

  /**
   * To deal with MP_JOIN/MP_CAPABLE
   */
//  virtual int ProcessOptionMpTcpSynSent(const Ptr<const TcpOption> optionMapTo);

    void StartOwdProbe ();
public:
  /**
   *
   */
  Ptr<MpTcpSocketBase> GetMeta() const;

  /**
   * Not implemented
   * \return false
   */
  bool IsInfiniteMappingEnabled() const;
    
  Ptr<RttEstimator> GetOwd (OwdEstimator);
protected:

  /** probe mechanism , check before sending a packet if we should */
  TcpOptionMpTcpOwdTimeStamp::State m_probeState;

  /**!< True if should add an MP_OWDTS option*/
  std::pair<bool, Ptr<TcpOptionMpTcpOwdTimeStamp> > m_owdProbeAnswer;

//  Time m_probeStartTime;    /**!< Record */
//  Ptr<SubflowPair> m_probingStats;
  Ptr<RttEstimator> m_owd[2];   /** 0 => local, 1 = remote */

//  void DumpInfo () const;
  /////////////////////////////////////////////
  //// DSS Mapping handling
  /////////////////////////////////////////////

  /**
   * Mapping is said "loose" because it is not tied to an SSN yet, this is the job
   * of this function: it will look for the FirstUnmappedSSN() and map the DSN to it.
   *
   * Thus you should call it with increased dsn.
   *
   * \param dsnHead
   */
  bool AddLooseMapping (SequenceNumber64 dsnHead, uint16_t length);

  /**
   * If no mappings set yet, then it returns the tail ssn of the Tx buffer.
   * Otherwise it returns the last registered mapping TailSequence
   */
  SequenceNumber32 FirstUnmappedSSN();

  /**
   * \brief Creates a DSS option if does not exist and configures it to have a dataack
   * TODO what happens if existing datack already set ?
   */
  virtual void AppendDSSAck();

  /**
   * Corresponds to mptcp_write_dss_mapping and mptcp_write_dss_ack
   */
  virtual void AddMpTcpOptionDSS(TcpHeader& header);

  /**
   * rename to addDSSFin
   */
  virtual void AppendDSSFin();
  virtual void AppendDSSMapping(const MpTcpMapping& mapping);

  virtual void ReceivedAck(Ptr<Packet>, const TcpHeader&); // Received an ACK packet
//  virtual void ReceivedAck(SequenceNumber32 seq); // Received an ACK packet

  virtual void ReceivedData(Ptr<Packet>, const TcpHeader&);


  /**

  */
  uint32_t
//  SendDataPacket(SequenceNumber32 seq, uint32_t maxSize, bool withAck); // Send a data packet
  SendDataPacket(TcpHeader& header, SequenceNumber32 ssn, uint32_t maxSize);

  /**
  * Like send, but pass on the global seq number associated with
  * \see Send
  **/
//  virtual int
//  SendMapping(Ptr<Packet> p,
//              //SequenceNumber32 seq
//              MpTcpMapping& mapping
//              );


  virtual void
  ReTxTimeout();
  /**
  This one overridesprevious one, adding MPTCP options when needed
  */
//  virtual void
//  SendEmptyPacket(uint8_t flags);
  virtual void
  SendEmptyPacket(uint8_t flags); // Send a empty packet that carries a flag, e.g. ACK

  virtual void
  SendEmptyPacket(TcpHeader& header);

  /**
   * Overrides the TcpSocketBase that just handles the MP_CAPABLE option.
   *
   */
  virtual void AddMpTcpOptions (TcpHeader& header);
//  virtual Ptr<TcpSocketBase>
//  Fork(void); // Call CopyObject<> to clone me


// N'existe plus dans TcpSocketBase
//  virtual void
//  DupAck(const TcpHeader& t, uint32_t count); // Received dupack

  /* TODO should be able to use parent's one little by little
  */
  virtual void
  CancelAllTimers(void); // Cancel all timer when endpoint is deleted


  uint8_t m_subflowId;   //!< Subflow's ID (TODO rename into subflowId ). Position of this subflow in MetaSock's subflows std::vector


  // Use Ptr here so that we don't have to unallocate memory manually ?
//  typedef std::list<MpTcpMapping> MappingList
//  MappingList
  MpTcpMappingContainer m_TxMappings;  //!< List of mappings to send
  MpTcpMappingContainer m_RxMappings;  //!< List of mappings to receive



protected:
  Ptr<MpTcpSocketBase> m_metaSocket;    //!< Meta
  virtual void SendPacket(TcpHeader header, Ptr<Packet> p);

//private:

private:
  // Delayed values to
  uint8_t m_dssFlags;           //!< used to know if AddMpTcpOptions should send a flag
  MpTcpMapping m_dssMapping;    //!< Pending ds configuration to be sent in next packet


  bool m_backupSubflow; //!< Priority
  bool m_masterSocket;  //!< True if this is the first subflow established (with MP_CAPABLE)

//  uint8_t m_localId;  //!< Store local host token, generated during the 3-way handshake
//  uint8_t m_remoteId;  //!< Store local host token, generated during the 3-way handshake

  uint32_t m_localNonce;  //!< Store local host token, generated during the 3-way handshake

  int m_prefixCounter;  //!< Temporary variable to help with prefix generation . To remove later
//  uint32_t m_remoteToken; //!< Store remote host token

};

}
#endif /* MP_TCP_SUBFLOW */
