/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2007 Georgia Tech Research Corporation
 * Copyright (c) 2010 Adrian Sai-wah Tam
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
 * Author: Adrian Sai-wah Tam <adrian.sw.tam@gmail.com>
 */

#define NS_LOG_APPEND_CONTEXT \
  if (m_node) { std::clog << Simulator::Now ().GetSeconds () << " [node " << m_node->GetId () << "] "; }

#include "ns3/abort.h"
#include "ns3/node.h"
#include "ns3/inet-socket-address.h"
#include "ns3/inet6-socket-address.h"
#include "ns3/log.h"
#include "ns3/ipv4.h"
#include "ns3/ipv6.h"
#include "ns3/ipv4-interface-address.h"
#include "ns3/ipv4-route.h"
#include "ns3/ipv6-route.h"
#include "ns3/ipv4-routing-protocol.h"
#include "ns3/ipv6-routing-protocol.h"
#include "ns3/simulation-singleton.h"
#include "ns3/simulator.h"
#include "ns3/packet.h"
#include "ns3/uinteger.h"
#include "ns3/double.h"
#include "ns3/pointer.h"
#include "ns3/trace-source-accessor.h"
#include "ns3/tcp-trace-helper.h"
#include "tcp-socket-base.h"
#include "tcp-l4-protocol.h"
#include "ipv4-end-point.h"
#include "ipv6-end-point.h"
#include "ipv6-l3-protocol.h"
#include "tcp-header.h"
#include "tcp-option-winscale.h"
#include "tcp-option-ts.h"
#include "rtt-estimator.h"
#include "mptcp-crypto.h"
#include "mptcp-subflow.h"
#include "mptcp-socket-base.h"
#include "tcp-option-mptcp.h"

#include <math.h>
#include <algorithm>

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("TcpSocketBase");

NS_OBJECT_ENSURE_REGISTERED (TcpSocketBase);



void 
DumpSocketState (std::ostream& os, Ptr<TcpSocketState> tcb )
{
  os << "CWND=" << tcb->m_cWnd << " SSTHRESH=" << tcb->m_ssThresh << std::endl
    << "ack fsm in state [" << tcb->m_ackState << "]"
    ;
}

TypeId
TcpSocketBase::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::TcpSocketBase")
    .SetParent<TcpSocket> ()
    .SetGroupName ("Internet")
    .AddConstructor<TcpSocketBase> ()
//    .AddAttribute ("TcpState", "State in TCP state machine",
//                   TypeId::ATTR_GET,
//                   EnumValue (CLOSED),
//                   MakeEnumAccessor (&TcpSocketBase::m_state),
//                   MakeEnumChecker (CLOSED, "Closed"))
    .AddAttribute ("MaxSegLifetime",
                   "Maximum segment lifetime in seconds, use for TIME_WAIT state transition to CLOSED state",
                   DoubleValue (120), /* RFC793 says MSL=2 minutes*/
                   MakeDoubleAccessor (&TcpSocketBase::m_msl),
                   MakeDoubleChecker<double> (0))
    .AddAttribute ("MaxWindowSize", "Max size of advertised window",
                   UintegerValue (65535),
                   MakeUintegerAccessor (&TcpSocketBase::m_maxWinSize),
                   MakeUintegerChecker<uint16_t> ())
    .AddAttribute ("NullISN",
                   "Set to false if you want to enable random isn",
                   BooleanValue (true),
            //      MakeBooleanAccessor (&TcpSocketBase::SetQosSupported,&RegularWifiMac::GetQosSupported)
                   MakeBooleanAccessor (&TcpSocketBase::m_nullIsn),
                   MakeBooleanChecker ())
    .AddAttribute ("EnableMpTcp",
                   "Enable or disable MPTCP support",
                   BooleanValue (false),
                   MakeBooleanAccessor (&TcpSocketBase::m_mptcpEnabled),
                   MakeBooleanChecker ())
    .AddAttribute ("IcmpCallback", "Callback invoked whenever an icmp error is received on this socket.",
                   CallbackValue (),
                   MakeCallbackAccessor (&TcpSocketBase::m_icmpCallback),
                   MakeCallbackChecker ())
    .AddAttribute ("IcmpCallback6", "Callback invoked whenever an icmpv6 error is received on this socket.",
                   CallbackValue (),
                   MakeCallbackAccessor (&TcpSocketBase::m_icmpCallback6),
                   MakeCallbackChecker ())
    .AddAttribute ("WindowScaling", "Enable or disable Window Scaling option",
                   BooleanValue (false),
                   MakeBooleanAccessor (&TcpSocketBase::m_winScalingEnabled),
                   MakeBooleanChecker ())
    .AddAttribute ("NoDelay", "Enable or disable Nagle algorithm",
                   BooleanValue (false),
                   MakeBooleanAccessor (&TcpSocketBase::GetTcpNoDelay,&TcpSocketBase::SetTcpNoDelay),
                   MakeBooleanChecker ())
    .AddAttribute ("Timestamp", "Enable or disable Timestamp option",
                   BooleanValue (false),
                   MakeBooleanAccessor (&TcpSocketBase::m_timestampEnabled),
                   MakeBooleanChecker ())
    .AddAttribute ("MinRto",
                   "Minimum retransmit timeout value",
                   TimeValue (Seconds (1.0)), // RFC 6298 says min RTO=1 sec, but Linux uses 200ms. See http://www.postel.org/pipermail/end2end-interest/2004-November/004402.html
                   MakeTimeAccessor (&TcpSocketBase::SetMinRto,
                                     &TcpSocketBase::GetMinRto),
                                     MakeTimeChecker ())
    .AddAttribute ("ClockGranularity",
                   "Clock Granularity used in RTO calculations",
                   TimeValue (MilliSeconds (1)), // RFC6298 suggest to use fine clock granularity
                   MakeTimeAccessor (&TcpSocketBase::SetClockGranularity,
                                     &TcpSocketBase::GetClockGranularity),
                                     MakeTimeChecker ())
    .AddAttribute ("TxBuffer",
                   "TCP Tx buffer",
                   PointerValue (),
                   MakePointerAccessor (&TcpSocketBase::GetTxBuffer),
                                       MakePointerChecker<TcpTxBuffer> ())
    .AddAttribute ("RxBuffer",
                   "TCP Rx buffer",
                   PointerValue (),
                   MakePointerAccessor (&TcpSocketBase::GetRxBuffer),
                   MakePointerChecker<TcpRxBuffer> ())
    .AddAttribute ("ReTxThreshold", "Threshold for fast retransmit",
                    UintegerValue (3),
                    MakeUintegerAccessor (&TcpSocketBase::m_retxThresh),
                    MakeUintegerChecker<uint32_t> ())
    .AddAttribute ("LimitedTransmit", "Enable limited transmit",
                   BooleanValue (false),
                   MakeBooleanAccessor (&TcpSocketBase::m_limitedTx),
                   MakeBooleanChecker ())
    .AddTraceSource ("RTO",
                     "Retransmission timeout",
                     MakeTraceSourceAccessor (&TcpSocketBase::m_rto),
                     "ns3::Time::TracedValueCallback")
    .AddTraceSource ("RTT",
                     "Last RTT sample",
                     MakeTraceSourceAccessor (&TcpSocketBase::m_lastRtt),
                     "ns3::Time::TracedValueCallback")
    .AddTraceSource ("NextTxSequence",
                     "Next sequence number to send (SND.NXT)",
                     MakeTraceSourceAccessor (&TcpSocketBase::m_nextTxSequence),
                     "ns3::SequenceNumber32TracedValueCallback")
    .AddTraceSource ("HighestSequence",
                     "Highest sequence number ever sent in socket's life time",
                     MakeTraceSourceAccessor (&TcpSocketBase::m_highTxMark),
                     "ns3::SequenceNumber32TracedValueCallback")
    .AddTraceSource ("State",
                     "TCP state",
                     MakeTraceSourceAccessor (&TcpSocketBase::m_state),
                     "ns3::TcpStatesTracedValueCallback")
    .AddTraceSource ("AckState",
                     "TCP ACK machine state",
                     MakeTraceSourceAccessor (&TcpSocketBase::m_ackStateTrace),
                     "ns3::TcpAckStatesTracedValueCallback")
    .AddTraceSource ("RWND",
                     "Remote side's flow control window",
                     MakeTraceSourceAccessor (&TcpSocketBase::m_rWnd),
                     "ns3::TracedValue::Uint32Callback")
    // This is strange since zis is decorellated from rxBuffer->NextRxSequence
    .AddTraceSource ("HighestRxSequence",
                     "Highest sequence number received from peer",
                     MakeTraceSourceAccessor (&TcpSocketBase::m_highRxMark),
                     "ns3::SequenceNumber32TracedValueCallback")
    .AddTraceSource ("HighestRxAck",
                     "Highest ack received from peer",
                     MakeTraceSourceAccessor (&TcpSocketBase::m_highRxAckMark),
                     "ns3::SequenceNumber32TracedValueCallback")
    .AddTraceSource ("CongestionWindow",
                     "The TCP connection's congestion window",
                     MakeTraceSourceAccessor (&TcpSocketBase::m_cWndTrace),
                     "ns3::TracedValue::Uint32Callback")
    .AddTraceSource ("SlowStartThreshold",
                     "TCP slow start threshold (bytes)",
                     MakeTraceSourceAccessor (&TcpSocketBase::m_ssThTrace),
                     "ns3::TracedValue::Uint32Callback")
    .AddTraceSource("UnackSequence",
                    "First unacknowledged sequence number (SND.UNA)",
                    MakeTraceSourceAccessor(&TcpSocketBase::m_firstTxUnack),
                    "ns3::SequenceNumber32TracedValueCallback")

  ;
  return tid;
}

TypeId
TcpSocketBase::GetInstanceTypeId() const
{
    return TcpSocketBase::GetTypeId();
}


TypeId
TcpSocketState::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::TcpSocketState")
    .SetParent<Object> ()
    .SetGroupName ("Internet")
    .AddConstructor <TcpSocketState> ()
    .AddTraceSource ("CongestionWindow",
                     "The TCP connection's congestion window",
                     MakeTraceSourceAccessor (&TcpSocketState::m_cWnd),
                     "ns3::TracedValue::Uint32Callback")
    .AddTraceSource ("SlowStartThreshold",
                     "TCP slow start threshold (bytes)",
                     MakeTraceSourceAccessor (&TcpSocketState::m_ssThresh),
                     "ns3::TracedValue::Uint32Callback")
    .AddTraceSource ("AckState",
                     "TCP ACK machine state",
                     MakeTraceSourceAccessor (&TcpSocketState::m_ackState),
                     "ns3::TracedValue::TcpAckStatesTracedValueCallback")
  ;
  return tid;
}

TcpSocketState::TcpSocketState (void)
  : Object (),
    m_cWnd (0),
    m_ssThresh (0),
    m_initialCWnd (0),
    m_initialSsThresh (0),
    m_segmentSize (0),
    m_ackState (OPEN)
{
}

TcpSocketState::TcpSocketState (const TcpSocketState &other)
  : Object (other),
    m_cWnd (other.m_cWnd),
    m_ssThresh (other.m_ssThresh),
    m_initialCWnd (other.m_initialCWnd),
    m_initialSsThresh (other.m_initialSsThresh),
    m_segmentSize (other.m_segmentSize),
    m_ackState (other.m_ackState)
{
}

const char* const
TcpSocketState::TcpAckStateName[TcpSocketState::LAST_ACKSTATE] =
{
  "OPEN", "DISORDER", "CWR", "RECOVERY", "LOSS"
};


TcpSocketBase& 
TcpSocketBase::operator=(const TcpSocketBase& sock)
{
  Socket::operator=(sock);
  m_delAckCount = 0;
  m_endPoint = 0;
  m_endPoint6 = 0;
//  m_rtt = 0;

  m_dupAckCount = sock.m_dupAckCount;
  m_delAckMaxCount = sock.m_delAckMaxCount;
  m_noDelay = sock.m_noDelay;
  m_cnRetries = sock.m_cnRetries;
  m_delAckTimeout = sock.m_delAckTimeout;
  m_persistTimeout = sock.m_persistTimeout;
  m_cnTimeout = sock.m_cnTimeout;

  m_node = sock.m_node;
  m_tcp = sock.m_tcp;

  m_firstTxUnack= sock.m_firstTxUnack;
  m_nextTxSequence = sock.m_nextTxSequence;
  m_highTxMark = sock.m_highTxMark;
  m_state = sock.m_state;
  m_errno = sock.m_errno;
  m_closeNotified = sock.m_closeNotified;
  m_closeOnEmpty = sock.m_closeOnEmpty;
  m_shutdownSend = sock.m_shutdownSend;
  m_shutdownRecv = sock.m_shutdownRecv;
  m_connected = sock.m_connected;
  m_msl = sock.m_msl;
  m_maxWinSize = sock.m_maxWinSize;
  m_rWnd = sock.m_rWnd;
  m_highRxMark = sock.m_highRxMark;
  m_highRxAckMark = sock.m_highRxAckMark;
  m_bytesAckedNotProcessed= 0;
  m_nullIsn= sock.m_nullIsn;
  m_localISN= sock.m_localISN;
  m_peerISN= sock.m_peerISN;
  m_mptcpEnabled = sock.m_mptcpEnabled;
  m_mptcpLocalKey = sock.m_mptcpLocalKey;
  m_mptcpLocalToken = sock.m_mptcpLocalToken;
  m_winScalingEnabled = sock.m_winScalingEnabled;
  m_sndScaleFactor = sock.m_sndScaleFactor;
  m_rcvScaleFactor = sock.m_rcvScaleFactor;
  m_timestampEnabled = sock.m_timestampEnabled;
  m_timestampToEcho = sock.m_timestampToEcho;
  m_retxThresh = sock.m_retxThresh;
  m_limitedTx = sock.m_limitedTx;
  m_lostOut = sock.m_lostOut;
  m_retransOut =sock.m_retransOut;

  m_txBuffer = CopyObject (sock.m_txBuffer);
  m_rxBuffer = CopyObject (sock.m_rxBuffer);

  if (sock.m_rtt)
    {
      m_rtt = sock.m_rtt->Copy ();
    }
    
  // TODO  there should be a way to prevent this from happening when looking to upgrade
  // TCP socket into an MPTCP one ?
  // Reset all callbacks to null
//  ResetUserCallbacks ();

  m_txBuffer = CopyObject (sock.m_txBuffer);
  m_rxBuffer = CopyObject (sock.m_rxBuffer);
  
  // MATT HACK to get around TcpSocketState limitations and let TcpCongestionOps 
  // get access to MPTCP subflows
  // 
  m_tcb = CopyObject (sock.m_tcb);
  
  // TODO remove 
  m_tcb->m_socket = this;
  
  if (sock.m_congestionControl)
    {
      m_congestionControl = sock.m_congestionControl->Fork ();
    }

  // MATT HACK to get around TcpSocketState limitations and let TcpCongestionOps 
  // get access to MPTCP subflows
  // 
//  m_tcb = CopyObject (sock.m_tcb);
//  m_tcb->m_socket = this;
//  
//  if (sock.m_congestionControl)
//    {
//      m_congestionControl = sock.m_congestionControl->Fork ();
//    }
//
//  bool ok;
//
//  ok = m_tcb->TraceConnectWithoutContext ("CongestionWindow",
//                                          MakeCallback (&TcpSocketBase::UpdateCwnd, this));
//  NS_ASSERT (ok == true);
//
//  ok = m_tcb->TraceConnectWithoutContext ("SlowStartThreshold",
//                                          MakeCallback (&TcpSocketBase::UpdateSsThresh, this));
//  NS_ASSERT (ok == true);
//
//  ok = m_tcb->TraceConnectWithoutContext ("AckState",
//                                          MakeCallback (&TcpSocketBase::UpdateAckState, this));
//  NS_ASSERT (ok == true);
  return *this;
}

TcpSocketBase::TcpSocketBase (void)
  :
    TcpSocket(),
    m_dupAckCount (0),
    m_delAckCount (0),
    m_endPoint (0),
    m_endPoint6 (0),
    m_node (0),
    m_tcp (0),
    m_rtt (0),
    m_firstTxUnack(0),
    m_nextTxSequence (0),
    // Change this for non-zero initial sequence number
    m_highTxMark (0),
    m_rxBuffer (0),
    m_txBuffer (0),
    m_state (CLOSED),
    m_errno (ERROR_NOTERROR),
    m_closeNotified (false),
    m_closeOnEmpty (false),
    m_shutdownSend (false),
    m_shutdownRecv (false),
    m_connected (false),
    // For attribute initialization consistency (quiet valgrind)
    m_rWnd (0),
    m_highRxMark (0),
    m_highRxAckMark (0),
    m_bytesAckedNotProcessed(0),
    m_nullIsn(true),
    m_localISN(0),
    m_peerISN(0),
    m_mptcpEnabled(false),
    m_mptcpLocalKey (0),
    m_mptcpLocalToken(0),
    m_winScalingEnabled(false),
    m_sndScaleFactor (0),
    m_rcvScaleFactor (0),
    m_timestampEnabled (true),
    m_timestampToEcho (0),
    m_retxThresh (3),
    m_limitedTx (false),
    m_lostOut (0),
    m_retransOut (0),
    m_congestionControl (0)

{
  NS_LOG_FUNCTION (this);
  m_rxBuffer = CreateObject<TcpRxBuffer> ();
  m_txBuffer = CreateObject<TcpTxBuffer> ();
  m_tcb      = CreateObject<TcpSocketState> ();

  // HACK MATT, no need anymore, remove
  m_tcb->m_socket = this;
  
  bool ok;

  ok = m_tcb->TraceConnectWithoutContext ("CongestionWindow",
                                          MakeCallback (&TcpSocketBase::UpdateCwnd, this));
  NS_ASSERT (ok == true);

  ok = m_tcb->TraceConnectWithoutContext ("SlowStartThreshold",
                                          MakeCallback (&TcpSocketBase::UpdateSsThresh, this));
  NS_ASSERT (ok == true);

  ok = m_tcb->TraceConnectWithoutContext ("AckState",
                                          MakeCallback (&TcpSocketBase::UpdateAckState, this));
  NS_ASSERT (ok == true);
}

TcpSocketBase::TcpSocketBase (const TcpSocketBase& sock)
  : TcpSocket (sock),
    //copy object::m_tid and socket::callbacks
    m_dupAckCount (sock.m_dupAckCount),
    m_delAckCount (0),
    m_delAckMaxCount (sock.m_delAckMaxCount),
    m_noDelay (sock.m_noDelay),
    m_cnRetries (sock.m_cnRetries),
    m_delAckTimeout (sock.m_delAckTimeout),
    m_persistTimeout (sock.m_persistTimeout),
    m_cnTimeout (sock.m_cnTimeout),
    m_endPoint (0),
    m_endPoint6 (0),
    m_node (sock.m_node),
    m_tcp (sock.m_tcp),
    m_rtt (0),
    m_firstTxUnack(sock.m_firstTxUnack),
    m_nextTxSequence (sock.m_nextTxSequence),
    m_highTxMark (sock.m_highTxMark),
    m_state (sock.m_state),
    m_errno (sock.m_errno),
    m_closeNotified (sock.m_closeNotified),
    m_closeOnEmpty (sock.m_closeOnEmpty),
    m_shutdownSend (sock.m_shutdownSend),
    m_shutdownRecv (sock.m_shutdownRecv),
    m_connected (sock.m_connected),
    m_msl (sock.m_msl),
    m_maxWinSize (sock.m_maxWinSize),
    m_rWnd (sock.m_rWnd),
    m_highRxMark (sock.m_highRxMark),
    m_highRxAckMark (sock.m_highRxAckMark),
    m_bytesAckedNotProcessed(0),
    m_nullIsn(sock.m_nullIsn),
    m_localISN(sock.m_localISN),
    m_peerISN(sock.m_peerISN),
    m_mptcpEnabled (sock.m_mptcpEnabled),
    m_mptcpLocalKey (sock.m_mptcpLocalKey),
    m_mptcpLocalToken (sock.m_mptcpLocalToken),
    m_winScalingEnabled (sock.m_winScalingEnabled),
    m_sndScaleFactor (sock.m_sndScaleFactor),
    m_rcvScaleFactor (sock.m_rcvScaleFactor),
    m_timestampEnabled (sock.m_timestampEnabled),
    m_timestampToEcho (sock.m_timestampToEcho),
    m_retxThresh (sock.m_retxThresh),
    m_limitedTx (sock.m_limitedTx),
    m_lostOut (sock.m_lostOut),
    m_retransOut (sock.m_retransOut)
{
  NS_LOG_FUNCTION (this);
  NS_LOG_LOGIC ("Invoked the copy constructor");
  // Copy the rtt estimator if it is set
  if (sock.m_rtt)
    {
      m_rtt = sock.m_rtt->Copy ();
    }
    
  // TODO  there should be a way to prevent this from happening when looking to upgrade
  // TCP socket into an MPTCP one ?
  // Reset all callbacks to null
//  ResetUserCallbacks ();

  m_txBuffer = CopyObject (sock.m_txBuffer);
  m_rxBuffer = CopyObject (sock.m_rxBuffer);
  
  // MATT HACK to get around TcpSocketState limitations and let TcpCongestionOps 
  // get access to MPTCP subflows
  // 
  m_tcb = CopyObject (sock.m_tcb);
  
  // TODO remove 
  m_tcb->m_socket = this;
  
  if (sock.m_congestionControl)
    {
      m_congestionControl = sock.m_congestionControl->Fork ();
    }

  bool ok;

  ok = m_tcb->TraceConnectWithoutContext ("CongestionWindow",
                                          MakeCallback (&TcpSocketBase::UpdateCwnd, this));
  NS_ASSERT (ok == true);

  ok = m_tcb->TraceConnectWithoutContext ("SlowStartThreshold",
                                          MakeCallback (&TcpSocketBase::UpdateSsThresh, this));
  NS_ASSERT (ok == true);

  ok = m_tcb->TraceConnectWithoutContext ("AckState",
                                          MakeCallback (&TcpSocketBase::UpdateAckState, this));
  NS_ASSERT (ok == true);
}

TcpSocketBase::~TcpSocketBase (void)
{
  NS_LOG_FUNCTION (this);
  m_node = 0;
  if (m_endPoint != 0)
    {
      NS_ASSERT (m_tcp != 0);
      /*
       * Upon Bind, an Ipv4Endpoint is allocated and set to m_endPoint, and
       * DestroyCallback is set to TcpSocketBase::Destroy. If we called
       * m_tcp->DeAllocate, it wil destroy its Ipv4EndpointDemux::DeAllocate,
       * which in turn destroys my m_endPoint, and in turn invokes
       * TcpSocketBase::Destroy to nullify m_node, m_endPoint, and m_tcp.
       */
      NS_ASSERT (m_endPoint != 0);
      m_tcp->DeAllocate (m_endPoint);
      NS_ASSERT (m_endPoint == 0);
    }
  if (m_endPoint6 != 0)
    {
      NS_ASSERT (m_tcp != 0);
      NS_ASSERT (m_endPoint6 != 0);
      m_tcp->DeAllocate (m_endPoint6);
      NS_ASSERT (m_endPoint6 == 0);
    }
  m_tcp = 0;
  CancelAllTimers ();
}


/* Associate a node with this TCP socket */
void
TcpSocketBase::ResetUserCallbacks (void)
{

  Callback<void, Ptr< Socket > > vPS = MakeNullCallback<void, Ptr<Socket> > ();
  Callback<void, Ptr<Socket>, const Address &> vPSA = MakeNullCallback<void, Ptr<Socket>, const Address &> ();
  Callback<void, Ptr<Socket>, uint32_t> vPSUI = MakeNullCallback<void, Ptr<Socket>, uint32_t> ();
  SetConnectCallback (vPS, vPS);
  SetDataSentCallback (vPSUI);
  SetSendCallback (vPSUI);
  SetRecvCallback (vPS);
}

/* Associate a node with this TCP socket */
void
TcpSocketBase::SetNode (Ptr<Node> node)
{
  m_node = node;
}

/* Associate the L4 protocol (e.g. mux/demux) with this socket */
void
TcpSocketBase::SetTcp (Ptr<TcpL4Protocol> tcp)
{
  m_tcp = tcp;
}

/* Set an RTT estimator with this socket */
void
TcpSocketBase::SetRtt (Ptr<RttEstimator> rtt)
{
  m_rtt = rtt;
}

/* Inherit from Socket class: Returns error code */
enum Socket::SocketErrno
TcpSocketBase::GetErrno (void) const
{
  return m_errno;
}

/* Inherit from Socket class: Returns socket type, NS3_SOCK_STREAM */
enum Socket::SocketType
TcpSocketBase::GetSocketType (void) const
{
  return NS3_SOCK_STREAM;
}

/* Inherit from Socket class: Returns associated node */
Ptr<Node>
TcpSocketBase::GetNode (void) const
{
  NS_LOG_FUNCTION_NOARGS ();
  return m_node;
}

SequenceNumber32
TcpSocketBase::FirstUnackedSeq() const
{
  return m_firstTxUnack.Get();
}

/* Inherit from Socket class: Bind socket to an end-point in TcpL4Protocol */
int
TcpSocketBase::Bind (void)
{
  NS_LOG_FUNCTION (this);
  m_endPoint = m_tcp->Allocate ();
  if (0 == m_endPoint)
    {
      m_errno = ERROR_ADDRNOTAVAIL;
      return -1;
    }

  m_tcp->AddSocket(this);

  return SetupCallback ();
}

int
TcpSocketBase::Bind6 (void)
{
  NS_LOG_FUNCTION (this);
  m_endPoint6 = m_tcp->Allocate6 ();
  if (0 == m_endPoint6)
    {
      m_errno = ERROR_ADDRNOTAVAIL;
      return -1;
    }

  m_tcp->AddSocket(this);

  return SetupCallback ();
}

Ptr<NetDevice>
TcpSocketBase::MapIpToInterface(Ipv4Address addr) const
{
    NS_LOG_DEBUG(addr);
      Ptr<Ipv4> ipv4client = m_node->GetObject<Ipv4>();
      for( uint32_t n =0; n < ipv4client->GetNInterfaces(); n++){
        for( uint32_t a=0; a < ipv4client->GetNAddresses(n); a++){
            NS_LOG_UNCOND( "Client addr " << n <<"/" << a << "=" << ipv4client->GetAddress(n,a));
            if(addr ==ipv4client->GetAddress(n,a).GetLocal()) {
                NS_LOG_UNCOND("EUREKA same ip=" << addr);
                // That function is buggy
//                BindToNetDevice();
                return m_node->GetDevice(n);
//                m_endPoint->BindToNetDevice();
////                m_boundnetdevice = m_endPoint->GetBoundNetDevice();
//                break;
            }
        }
      }
    return 0;
}

/* Inherit from Socket class: Bind socket (with specific address) to an end-point in TcpL4Protocol */
int
TcpSocketBase::Bind (const Address &address)
{
  NS_LOG_FUNCTION (this << address);

  if (InetSocketAddress::IsMatchingType (address))
    {

      InetSocketAddress transport = InetSocketAddress::ConvertFrom (address);
      Ipv4Address ipv4 = transport.GetIpv4 ();
      NS_LOG_DEBUG(ipv4 );
      uint16_t port = transport.GetPort ();
      if (ipv4 == Ipv4Address::GetAny () && port == 0)
        {
          m_endPoint = m_tcp->Allocate ();
        }
      else if (ipv4 == Ipv4Address::GetAny () && port != 0)
        {
          m_endPoint = m_tcp->Allocate (port);
        }
      else if (ipv4 != Ipv4Address::GetAny () && port == 0)
        {
          m_endPoint = m_tcp->Allocate (ipv4);
        }
      else if (ipv4 != Ipv4Address::GetAny () && port != 0)
        {
          m_endPoint = m_tcp->Allocate (ipv4, port);
        }

      NS_LOG_UNCOND("TATA" );
      if (0 == m_endPoint)
        {
          m_errno = port ? ERROR_ADDRINUSE : ERROR_ADDRNOTAVAIL;
          return -1;
        }

      Ptr<NetDevice> dev = MapIpToInterface(m_endPoint->GetLocalAddress());

      if(dev) {
                m_endPoint->BindToNetDevice(dev);
                m_boundnetdevice = m_endPoint->GetBoundNetDevice();
      }
      #if 0
      Ptr<Ipv4> ipv4client = m_node->GetObject<Ipv4>();
      for( uint32_t n =0; n < ipv4client->GetNInterfaces(); n++){
        for( uint32_t a=0; a < ipv4client->GetNAddresses(n); a++){
            NS_LOG_UNCOND( "Client addr " << n <<"/" << a << "=" << ipv4client->GetAddress(n,a));
            if(m_endPoint->GetLocalAddress() ==ipv4client->GetAddress(n,a).GetLocal()) {
                NS_LOG_UNCOND("EUREKA same ip=" << m_endPoint->GetLocalAddress());
                // That function is buggy
//                BindToNetDevice();
                m_endPoint->BindToNetDevice(m_node->GetDevice(n));
//                m_boundnetdevice = m_node->GetDevice(n);
                m_boundnetdevice = m_endPoint->GetBoundNetDevice();
                break;
            }
        }
      }
      #endif
      NS_LOG_UNCOND("BOUND NETDEV=" << m_boundnetdevice );
//      BindToNetDevice(
//      m_endPoint->GetBoundNetDevice();

    }
  else if (Inet6SocketAddress::IsMatchingType (address))
    {
      Inet6SocketAddress transport = Inet6SocketAddress::ConvertFrom (address);
      Ipv6Address ipv6 = transport.GetIpv6 ();
      uint16_t port = transport.GetPort ();
      if (ipv6 == Ipv6Address::GetAny () && port == 0)
        {
          m_endPoint6 = m_tcp->Allocate6 ();
        }
      else if (ipv6 == Ipv6Address::GetAny () && port != 0)
        {
          m_endPoint6 = m_tcp->Allocate6 (port);
        }
      else if (ipv6 != Ipv6Address::GetAny () && port == 0)
        {
          m_endPoint6 = m_tcp->Allocate6 (ipv6);
        }
      else if (ipv6 != Ipv6Address::GetAny () && port != 0)
        {
          m_endPoint6 = m_tcp->Allocate6 (ipv6, port);
        }
      if (0 == m_endPoint6)
        {
          m_errno = port ? ERROR_ADDRINUSE : ERROR_ADDRNOTAVAIL;
          return -1;
        }
    }
  else
    {
      m_errno = ERROR_INVAL;
      return -1;
    }

  m_tcp->AddSocket(this);

  NS_LOG_LOGIC ("TcpSocketBase " << this << " got an endpoint: " << m_endPoint);
  m_endPoint->Print(std::cout);

  return SetupCallback ();
}

void
TcpSocketBase::InitializeCwnd (void)
{
  m_tcb->m_cWnd = GetInitialCwnd() * GetSegSize();
  m_tcb->m_ssThresh = GetInitialSSThresh();
}

void
TcpSocketBase::SetInitialSSThresh (uint32_t threshold)
{
  NS_ABORT_MSG_UNLESS ( (m_state == CLOSED) || threshold == m_tcb->m_initialSsThresh,
    "TcpSocketBase::SetSSThresh() cannot change initial ssThresh after connection started.");

  m_tcb->m_initialSsThresh = threshold;
}

uint32_t
TcpSocketBase::GetInitialSSThresh (void) const
{
  return m_tcb->m_initialSsThresh;
}

void
TcpSocketBase::SetInitialCwnd (uint32_t cwnd)
{
  NS_ABORT_MSG_UNLESS ( (m_state == CLOSED) || cwnd == m_tcb->m_initialCWnd,
    "TcpSocketBase::SetInitialCwnd() cannot change initial cwnd after connection started.");

  m_tcb->m_initialCWnd = cwnd;
}

uint32_t
TcpSocketBase::GetInitialCwnd (void) const
{
  return m_tcb->m_initialCWnd;
}

void
TcpSocketBase::ScaleSsThresh (uint8_t scaleFactor)
{
  m_tcb->m_ssThresh <<= scaleFactor;
}

/* Inherit from Socket class: Initiate connection to a remote address:port */
int
TcpSocketBase::Connect (const Address & address)
{
  NS_LOG_FUNCTION (this << address);

  InitializeCwnd ();
  GenerateUniqueMpTcpKey();

  // If haven't do so, Bind() this socket first
  if (InetSocketAddress::IsMatchingType (address) && m_endPoint6 == 0)
    {
      if (m_endPoint == 0)
        {
          if (Bind () == -1)
            {
              NS_ASSERT (m_endPoint == 0);
              return -1; // Bind() failed
            }
          NS_ASSERT (m_endPoint != 0);
        }
      InetSocketAddress transport = InetSocketAddress::ConvertFrom (address);
      m_endPoint->SetPeer (transport.GetIpv4 (), transport.GetPort ());
      m_endPoint6 = 0;

      // Get the appropriate local address and port number from the routing protocol and set up endpoint
      if (SetupEndpoint () != 0)
        {
          NS_LOG_ERROR("Route to destination does not exist ?!");
          return -1;
        }
    }
  else if (Inet6SocketAddress::IsMatchingType (address)  && m_endPoint == 0)
    {
      // If we are operating on a v4-mapped address, translate the address to
      // a v4 address and re-call this function
      Inet6SocketAddress transport = Inet6SocketAddress::ConvertFrom (address);
      Ipv6Address v6Addr = transport.GetIpv6 ();
      if (v6Addr.IsIpv4MappedAddress () == true)
        {
          Ipv4Address v4Addr = v6Addr.GetIpv4MappedAddress ();
          return Connect (InetSocketAddress (v4Addr, transport.GetPort ()));
        }

      if (m_endPoint6 == 0)
        {
          if (Bind6 () == -1)
            {
              NS_ASSERT (m_endPoint6 == 0);
              return -1; // Bind() failed
            }
          NS_ASSERT (m_endPoint6 != 0);
        }
      m_endPoint6->SetPeer (v6Addr, transport.GetPort ());
      m_endPoint = 0;

      // Get the appropriate local address and port number from the routing protocol and set up endpoint
      if (SetupEndpoint6 () != 0)
        { // Route to destination does not exist
          return -1;
        }
    }
  else
    {
      m_errno = ERROR_INVAL;
      return -1;
    }

  // Re-initialize parameters in case this socket is being reused after CLOSE
  m_rtt->Reset ();
  m_cnCount = m_cnRetries;

  // TODO remove regenerate ISN
//   InitLocalISN();

  // DoConnect() will do state-checking and send a SYN packet
  return DoConnect ();
}

/* Inherit from Socket class: Listen on the endpoint for an incoming connection */
int
TcpSocketBase::Listen (void)
{
  NS_LOG_FUNCTION (this);

  InitializeCwnd ();

  // Linux quits EINVAL if we're not in CLOSED state, so match what they do
  if (m_state != CLOSED)
    {
      m_errno = ERROR_INVAL;
      return -1;
    }
  // In other cases, set the state to LISTEN and done
  NS_LOG_INFO ("CLOSED -> LISTEN");
  m_state = LISTEN;
  return 0;
}

/* Inherit from Socket class: Kill this socket and signal the peer (if any) */
int
TcpSocketBase::Close (void)
{
  NS_LOG_FUNCTION (this << TcpStateName[m_state]);
  /// \internal
  /// First we check to see if there is any unread rx data.
  /// \bugid{426} claims we should send reset in this case.
  if (m_rxBuffer->Size () != 0)
    {
      NS_LOG_INFO ("Socket " << this << " << unread rx data during close.  Sending reset");
      SendRST ();
      return 0;
    }

  uint32_t unackedTx = m_txBuffer->SizeFromSequence (m_nextTxSequence);
  
  NS_LOG_DEBUG (unackedTx << " more bytes to send");
  if (unackedTx > 0)
    { 
//      NS_LOG_DEBUG ("");
      // App close with pending data must wait until all data transmitted
      if (m_closeOnEmpty == false)
        {
          m_closeOnEmpty = true;
          NS_LOG_INFO ("Socket " << this << " deferring close to send " << unackedTx << " more bytes, state " << TcpStateName[m_state]);
        }
      return 0;
    }
  return DoClose ();
}

/* Inherit from Socket class: Signal a termination of send */
int
TcpSocketBase::ShutdownSend (void)
{
  NS_LOG_FUNCTION (this);

  //this prevents data from being added to the buffer
  m_shutdownSend = true;
  m_closeOnEmpty = true;
  //if buffer is already empty, send a fin now
  //otherwise fin will go when buffer empties.
  if (m_txBuffer->Size () == 0)
    {
      if (m_state == ESTABLISHED || m_state == CLOSE_WAIT)
        {
          NS_LOG_INFO("Emtpy tx buffer, send fin");
          SendEmptyPacket (TcpHeader::FIN);

          if (m_state == ESTABLISHED)
            { // On active close: I am the first one to send FIN
              NS_LOG_INFO ("ESTABLISHED -> FIN_WAIT_1");
              m_state = FIN_WAIT_1;
            }
          else
            { // On passive close: Peer sent me FIN already
              NS_LOG_INFO ("CLOSE_WAIT -> LAST_ACK");
              m_state = LAST_ACK;
            }
        }
    }

  return 0;
}

/* Inherit from Socket class: Signal a termination of receive */
int
TcpSocketBase::ShutdownRecv (void)
{
  NS_LOG_FUNCTION (this);
  m_shutdownRecv = true;
  return 0;
}

/* Inherit from Socket class: Send a packet. Parameter flags is not used.
    Packet has no TCP header. Invoked by upper-layer application */
int
TcpSocketBase::Send (Ptr<Packet> p, uint32_t flags)
{
  NS_LOG_FUNCTION (this << p);
  NS_ABORT_MSG_IF (flags, "use of flags is not supported in TcpSocketBase::Send()");
  if (m_state == ESTABLISHED || m_state == SYN_SENT || m_state == CLOSE_WAIT)
    {
      // Store the packet into Tx buffer
      if (!m_txBuffer->Add (p))
        { // TxBuffer overflow, send failed
          m_errno = ERROR_MSGSIZE;
          return -1;
        }
      if (m_shutdownSend)
        {
          m_errno = ERROR_SHUTDOWN;
          return -1;
        }
      // Submit the data to lower layers
      NS_LOG_LOGIC ("txBufSize=" << m_txBuffer->Size () << " state " << TcpStateName[m_state]);
      if (m_state == ESTABLISHED || m_state == CLOSE_WAIT)
        { // Try to send the data out
          if (!m_sendPendingDataEvent.IsRunning ())
            {
              m_sendPendingDataEvent = Simulator::Schedule ( TimeStep (1), &TcpSocketBase::SendPendingData, this, m_connected);
            }
        }
      // This does not match the return type uint32_t != int
      return p->GetSize ();
    }
  else
    { // Connection not established yet
      m_errno = ERROR_NOTCONN;
      return -1; // Send failure
    }
}

/* Inherit from Socket class: In TcpSocketBase, it is same as Send() call */
int
TcpSocketBase::SendTo (Ptr<Packet> p, uint32_t flags, const Address &address)
{
  return Send (p, flags); // SendTo() and Send() are the same
}

/* Inherit from Socket class: Return data to upper-layer application. Parameter flags
   is not used. Data is returned as a packet of size no larger than maxSize */
Ptr<Packet>
TcpSocketBase::Recv (uint32_t maxSize, uint32_t flags)
{
  NS_LOG_FUNCTION (this);
  NS_ABORT_MSG_IF (flags, "use of flags is not supported in TcpSocketBase::Recv()");
  if (m_rxBuffer->Size () == 0 && m_state == CLOSE_WAIT)
    {
      return Create<Packet> (); // Send EOF on connection close
    }
  Ptr<Packet> outPacket = m_rxBuffer->Extract (maxSize);
  if (outPacket != 0 && outPacket->GetSize () != 0)
    {
      SocketAddressTag tag;
      if (m_endPoint != 0)
        {
          tag.SetAddress (InetSocketAddress (m_endPoint->GetPeerAddress (), m_endPoint->GetPeerPort ()));
        }
      else if (m_endPoint6 != 0)
        {
          tag.SetAddress (Inet6SocketAddress (m_endPoint6->GetPeerAddress (), m_endPoint6->GetPeerPort ()));
        }
      outPacket->AddPacketTag (tag);
    }
  return outPacket;
}

/* Inherit from Socket class: Recv and return the remote's address */
Ptr<Packet>
TcpSocketBase::RecvFrom (uint32_t maxSize, uint32_t flags, Address &fromAddress)
{
  NS_LOG_FUNCTION (this << maxSize << flags);
  Ptr<Packet> packet = Recv (maxSize, flags);
  // Null packet means no data to read, and an empty packet indicates EOF
  if (packet != 0 && packet->GetSize () != 0)
    {
      if (m_endPoint != 0)
        {
          fromAddress = InetSocketAddress (m_endPoint->GetPeerAddress (), m_endPoint->GetPeerPort ());
        }
      else if (m_endPoint6 != 0)
        {
          fromAddress = Inet6SocketAddress (m_endPoint6->GetPeerAddress (), m_endPoint6->GetPeerPort ());
        }
      else
        {
          fromAddress = InetSocketAddress (Ipv4Address::GetZero (), 0);
        }
    }
  return packet;
}

/* Inherit from Socket class: Get the max number of bytes an app can send */
uint32_t
TcpSocketBase::GetTxAvailable (void) const
{
  NS_LOG_FUNCTION (this);
  return m_txBuffer->Available ();
}

/* Inherit from Socket class: Get the max number of bytes an app can read */
uint32_t
TcpSocketBase::GetRxAvailable (void) const
{
  NS_LOG_FUNCTION (this);
  return m_rxBuffer->Available ();
}

/* Inherit from Socket class: Return local address:port */
int
TcpSocketBase::GetSockName (Address &address) const
{
  NS_LOG_FUNCTION (this);
  if (m_endPoint != 0)
    {
      address = InetSocketAddress (m_endPoint->GetLocalAddress (), m_endPoint->GetLocalPort ());
    }
  else if (m_endPoint6 != 0)
    {
      address = Inet6SocketAddress (m_endPoint6->GetLocalAddress (), m_endPoint6->GetLocalPort ());
    }
  else
    { // It is possible to call this method on a socket without a name
      // in which case, behavior is unspecified
      // Should this return an InetSocketAddress or an Inet6SocketAddress?
      address = InetSocketAddress (Ipv4Address::GetZero (), 0);
    }
  return 0;
}

/* Inherit from Socket class: Bind this socket to the specified NetDevice */
void
TcpSocketBase::BindToNetDevice (Ptr<NetDevice> netdevice)
{
  NS_LOG_FUNCTION (netdevice);
  Socket::BindToNetDevice (netdevice); // Includes sanity check
  if (m_endPoint == 0)
    {
      if (Bind () == -1)
        {
          NS_ASSERT (m_endPoint == 0);
          return;
        }
      NS_ASSERT (m_endPoint != 0);
      m_endPoint->BindToNetDevice (netdevice);
    }
  else if (m_endPoint6 == 0)
    {
      if (Bind6 () == -1)
        {
          NS_ASSERT (m_endPoint6 == 0);
          return;
        }
      NS_ASSERT (m_endPoint6 != 0);
      m_endPoint6->BindToNetDevice (netdevice);
    }
  else {
    NS_FATAL_ERROR("What the hell happened ?");
  }

  return;
}

/* Clean up after Bind. Set up callback functions in the end-point. */
int
TcpSocketBase::SetupCallback (void)
{
  NS_LOG_FUNCTION (this);

  if (m_endPoint == 0 && m_endPoint6 == 0)
    {
      return -1;
    }
  if (m_endPoint != 0)
    {
      m_endPoint->SetRxCallback (MakeCallback (&TcpSocketBase::ForwardUp, Ptr<TcpSocketBase> (this)));
      m_endPoint->SetIcmpCallback (MakeCallback (&TcpSocketBase::ForwardIcmp, Ptr<TcpSocketBase> (this)));
      m_endPoint->SetDestroyCallback (MakeCallback (&TcpSocketBase::Destroy, Ptr<TcpSocketBase> (this)));
    }
  else if (m_endPoint6 != 0)
    {
      m_endPoint6->SetRxCallback (MakeCallback (&TcpSocketBase::ForwardUp6, Ptr<TcpSocketBase> (this)));
      m_endPoint6->SetIcmpCallback (MakeCallback (&TcpSocketBase::ForwardIcmp6, Ptr<TcpSocketBase> (this)));
      m_endPoint6->SetDestroyCallback (MakeCallback (&TcpSocketBase::Destroy6, Ptr<TcpSocketBase> (this)));
    }

  return 0;
}

/* Perform the real connection tasks: Send SYN if allowed, RST if invalid */
int
TcpSocketBase::DoConnect (void)
{
  NS_LOG_FUNCTION (this);

  // A new connection is allowed only if this socket does not have a connection
  if (m_state == CLOSED || m_state == LISTEN || m_state == SYN_SENT || m_state == LAST_ACK || m_state == CLOSE_WAIT)
    { // send a SYN packet and change state into SYN_SENT
      SendEmptyPacket (TcpHeader::SYN);
      NS_LOG_INFO (TcpStateName[m_state] << " -> SYN_SENT");
      m_state = SYN_SENT;
    }
  else if (m_state != TIME_WAIT)
    { // In states SYN_RCVD, ESTABLISHED, FIN_WAIT_1, FIN_WAIT_2, and CLOSING, an connection
      // exists. We send RST, tear down everything, and close this socket.
      SendRST ();
      CloseAndNotify ();
    }
  return 0;
}

/* Do the action to close the socket. Usually send a packet with appropriate
    flags depended on the current m_state. */
int
TcpSocketBase::DoClose (void)
{
  NS_LOG_FUNCTION (this);
  switch (m_state)
    {
    case SYN_RCVD:
    case ESTABLISHED:
      // send FIN to close the peer
      SendEmptyPacket (TcpHeader::FIN);
      NS_LOG_INFO ("ESTABLISHED -> FIN_WAIT_1");
      m_state = FIN_WAIT_1;
      break;
    case CLOSE_WAIT:
      // send FIN+ACK to close the peer
      SendEmptyPacket (TcpHeader::FIN | TcpHeader::ACK);
      NS_LOG_INFO ("CLOSE_WAIT -> LAST_ACK");
      m_state = LAST_ACK;
      break;
    case SYN_SENT:
    case CLOSING:
      // Send RST if application closes in SYN_SENT and CLOSING
      SendRST ();
      CloseAndNotify ();
      break;
    case LISTEN:
    case LAST_ACK:
      // In these three states, move to CLOSED and tear down the end point
      CloseAndNotify ();
      break;
    case CLOSED:
    case FIN_WAIT_1:
    case FIN_WAIT_2:
    case TIME_WAIT:
    default: /* mute compiler */
      // Do nothing in these four states
      break;
    }
  return 0;
}

/* Peacefully close the socket by notifying the upper layer and deallocate end point */
void
TcpSocketBase::CloseAndNotify (void)
{
  NS_LOG_FUNCTION (this);

  if (!m_closeNotified)
    {
      NotifyNormalClose ();
      m_closeNotified = true;
    }

  NS_LOG_INFO (TcpStateName[m_state] << " -> CLOSED");
  m_state = CLOSED;
  DeallocateEndPoint ();
}


/* Tell if a sequence number range is out side the range that my rx buffer can
    accpet */
bool
TcpSocketBase::OutOfRange (SequenceNumber32 head, SequenceNumber32 tail) const
{
  if (m_state == LISTEN || m_state == SYN_SENT || m_state == SYN_RCVD)
    { // Rx buffer in these states are not initialized.
      return false;
    }
  if (m_state == LAST_ACK || m_state == CLOSING || m_state == CLOSE_WAIT)
    { // In LAST_ACK and CLOSING states, it only wait for an ACK and the
      // sequence number must equals to m_rxBuffer->NextRxSequence ()
      return (m_rxBuffer->NextRxSequence () != head);
    }

  // In all other cases, check if the sequence number is in range
  return (tail < m_rxBuffer->NextRxSequence () || m_rxBuffer->MaxRxSequence () <= head);
}

/* Function called by the L3 protocol when it received a packet to pass on to
    the TCP. This function is registered as the "RxCallback" function in
    SetupCallback(), which invoked by Bind(), and CompleteFork() */
void
TcpSocketBase::ForwardUp (Ptr<Packet> packet, Ipv4Header header, uint16_t port,
                          Ptr<Ipv4Interface> incomingInterface)
{
  NS_LOG_LOGIC ("Socket " << this << " forward up " <<
                m_endPoint->GetPeerAddress () <<
                ":" << m_endPoint->GetPeerPort () <<
                " to " << m_endPoint->GetLocalAddress () <<
                ":" << m_endPoint->GetLocalPort ());

  Address fromAddress = InetSocketAddress (header.GetSource (), port);
  Address toAddress = InetSocketAddress (header.GetDestination (),
                                         m_endPoint->GetLocalPort ());

  DoForwardUp (packet, fromAddress, toAddress);
}

void
TcpSocketBase::ForwardUp6 (Ptr<Packet> packet, Ipv6Header header, uint16_t port,
                           Ptr<Ipv6Interface> incomingInterface)
{
  NS_LOG_LOGIC ("Socket " << this << " forward up " <<
                m_endPoint6->GetPeerAddress () <<
                ":" << m_endPoint6->GetPeerPort () <<
                " to " << m_endPoint6->GetLocalAddress () <<
                ":" << m_endPoint6->GetLocalPort ());

  Address fromAddress = Inet6SocketAddress (header.GetSourceAddress (), port);
  Address toAddress = Inet6SocketAddress (header.GetDestinationAddress (),
                                          m_endPoint6->GetLocalPort ());

  DoForwardUp (packet, fromAddress, toAddress);
}

void
TcpSocketBase::ForwardIcmp (Ipv4Address icmpSource, uint8_t icmpTtl,
                            uint8_t icmpType, uint8_t icmpCode,
                            uint32_t icmpInfo)
{
  NS_LOG_FUNCTION (this << icmpSource << (uint32_t)icmpTtl << (uint32_t)icmpType <<
                   (uint32_t)icmpCode << icmpInfo);
  if (!m_icmpCallback.IsNull ())
    {
      m_icmpCallback (icmpSource, icmpTtl, icmpType, icmpCode, icmpInfo);
    }
}

void
TcpSocketBase::ForwardIcmp6 (Ipv6Address icmpSource, uint8_t icmpTtl,
                            uint8_t icmpType, uint8_t icmpCode,
                            uint32_t icmpInfo)
{
  NS_LOG_FUNCTION (this << icmpSource << (uint32_t)icmpTtl << (uint32_t)icmpType <<
                   (uint32_t)icmpCode << icmpInfo);
  if (!m_icmpCallback6.IsNull ())
    {
      m_icmpCallback6 (icmpSource, icmpTtl, icmpType, icmpCode, icmpInfo);
    }
}

void
TcpSocketBase::DoForwardUp (Ptr<Packet> packet, const Address &fromAddress,
                            const Address &toAddress)
{
  // Peel off TCP header and do validity checking
  TcpHeader tcpHeader;
  uint32_t bytesRemoved = packet->RemoveHeader (tcpHeader);
  if (bytesRemoved == 0 || bytesRemoved > 60)
    {
      NS_LOG_ERROR ("Bytes removed: " << bytesRemoved << " invalid");
      return; // Discard invalid packet
    }
    

   m_receivedHeader = tcpHeader;
   // We should call a function that does validity cheking here and that could be 
   // overriden by MPTCP meta
   // Might cause pb when meta is not set

//  NS_LOG_UNCOND("TOTO=" << packet->GetSize ());
    // TODO bad idea to put it here.
    // should first check that packet is in range
    // + processing should be state dependant
//  ReadOptions (tcpHeader);

  if (tcpHeader.GetFlags () & TcpHeader::ACK)
    {
      EstimateRtt (tcpHeader);
    }

  // Discard fully out of range data packets
  if (packet->GetSize ()
      && OutOfRange (tcpHeader.GetSequenceNumber (), tcpHeader.GetSequenceNumber () + packet->GetSize ()))
    {
      NS_LOG_LOGIC ("At state " << TcpStateName[m_state] <<
                    " received packet of seq [" << tcpHeader.GetSequenceNumber () <<
                    ":" << tcpHeader.GetSequenceNumber () + packet->GetSize () <<
                    ") out of range [" << m_rxBuffer->NextRxSequence () << ":" <<
                    m_rxBuffer->MaxRxSequence () << ")");
      // Acknowledgement should be sent for all unacceptable packets (RFC793, p.69)
      if (m_state == ESTABLISHED && !(tcpHeader.GetFlags () & TcpHeader::RST))
        {
          SendEmptyPacket (TcpHeader::ACK);
        }
      return;
    }

  // Update Rx window size, i.e. the flow control window
  if (GetRwnd () == 0 && tcpHeader.GetWindowSize () != 0 && m_persistEvent.IsRunning ())
    { // persist probes end
      NS_LOG_LOGIC (this << " Leaving zerowindow persist state");
      m_persistEvent.Cancel ();
    }

  if (tcpHeader.GetFlags () & TcpHeader::ACK)
    {
      UpdateWindowSize (tcpHeader);
    }

  // TCP state machine code in different process functions
  // C.f.: tcp_rcv_state_process() in tcp_input.c in Linux kernel
  switch (m_state)
    {
    case ESTABLISHED:
      ProcessEstablished (packet, tcpHeader);
      break;
    case LISTEN:
      ProcessListen (packet, tcpHeader, fromAddress, toAddress);
      break;
    case TIME_WAIT:
      // Do nothing
      break;
    case CLOSED:
      // Send RST if the incoming packet is not a RST
      if ((tcpHeader.GetFlags () & ~(TcpHeader::PSH | TcpHeader::URG)) != TcpHeader::RST)
        { // Since m_endPoint is not configured yet, we cannot use SendRST here
          TcpHeader h;
          GenerateEmptyPacketHeader(h, TcpHeader::RST);
//          h.SetSequenceNumber (m_nextTxSequence);
//          h.SetAckNumber (m_rxBuffer->NextRxSequence ());
          h.SetSourcePort (tcpHeader.GetDestinationPort ());
          h.SetDestinationPort (tcpHeader.GetSourcePort ());
//          h.SetWindowSize (AdvertisedWindowSize ());
          // Do we really need that in a RST ?
//          AddOptions (h);
          SendPacket(h, Create<Packet> ());
//          m_tcp->SendPacket (Create<Packet> (), h, toAddress, fromAddress, m_boundnetdevice);
        }
      break;
    case SYN_SENT:
      ProcessSynSent (packet, tcpHeader);
      break;
    case SYN_RCVD:
      ProcessSynRcvd (packet, tcpHeader, fromAddress, toAddress);
      break;
    case FIN_WAIT_1:
    case FIN_WAIT_2:
    case CLOSE_WAIT:
      ProcessWait (packet, tcpHeader);
      break;
    case CLOSING:
      ProcessClosing (packet, tcpHeader);
      break;
    case LAST_ACK:
      ProcessLastAck (packet, tcpHeader);
      break;
    default: // mute compiler
      break;
    }
}

/* Received a packet upon ESTABLISHED state. This function is mimicking the
    role of tcp_rcv_established() in tcp_input.c in Linux kernel. */
void
TcpSocketBase::ProcessEstablished (Ptr<Packet> packet, const TcpHeader& tcpHeader)
{
  NS_LOG_FUNCTION (this << tcpHeader);

  // Extract the flags. PSH and URG are not honoured.
  uint8_t tcpflags = tcpHeader.GetFlags () & ~(TcpHeader::PSH | TcpHeader::URG);

  // Different flags are different events
  if (tcpflags == TcpHeader::ACK)
    {
      ProcessTcpOptions(tcpHeader);
      if (tcpHeader.GetAckNumber () < FirstUnackedSeq())
        {
          // Case 1:  If the ACK is a duplicate (SEG.ACK < SND.UNA), it can be ignored.
          // Pag. 72 RFC 793
          NS_LOG_LOGIC ("Ignored ack of " << tcpHeader.GetAckNumber ());

          // TODO: RFC 5961 5.2 [Blind Data Injection Attack].[Mitigation]
        }
      else if (tcpHeader.GetAckNumber() > m_nextTxSequence)
        {
          // If the ACK acks something not yet sent (SEG.ACK > SND.NXT) then
          // send an ACK, drop the segment, and return.
          // Pag. 72 RFC 793
          NS_LOG_LOGIC ("Ignored ack of " << tcpHeader.GetAckNumber ());

          SendEmptyPacket (TcpHeader::ACK);
        }
      else
        {
          // SND.UNA < SEG.ACK =< SND.NXT
          // Pag. 72 RFC 793
          ReceivedAck (packet, tcpHeader);
        }
    }
  else if (tcpflags == TcpHeader::SYN)
    { // Received SYN, old NS-3 behaviour is to set state to SYN_RCVD and
      // respond with a SYN+ACK. But it is not a legal state transition as of
      // RFC793. Thus this is ignored.
    }
  else if (tcpflags == (TcpHeader::SYN | TcpHeader::ACK))
    { // No action for received SYN+ACK, it is probably a duplicated packet
    }
  else if (tcpflags == TcpHeader::FIN || tcpflags == (TcpHeader::FIN | TcpHeader::ACK))
    { // Received FIN or FIN+ACK, bring down this socket nicely
      PeerClose (packet, tcpHeader);
    }
  else if (tcpflags == 0)
    { // No flags means there is only data
      ProcessTcpOptions(tcpHeader);
      ReceivedData (packet, tcpHeader);
      if (m_rxBuffer->Finished ())
        {
          PeerClose (packet, tcpHeader);
        }
    }
  else
    { // Received RST or the TCP flags is invalid, in either case, terminate this socket
      if (tcpflags != TcpHeader::RST)
        { // this must be an invalid flag, send reset
          NS_LOG_LOGIC ("Illegal flag " << TcpHeader::FlagsToString (tcpflags) << " received. Reset packet is sent.");
          SendRST ();
        }
      CloseAndNotify ();
    }
}


// TODO this function should be removed
void
TcpSocketBase::ReceivedAck (Ptr<Packet> packet,
                            const TcpHeader& tcpHeader
                            )
{
  // If there is any data piggybacked, store it into m_rxBuffer
  NS_LOG_FUNCTION(this << packet << tcpHeader);
  NS_ASSERT (0 != (tcpHeader.GetFlags () & TcpHeader::ACK));
  ReceivedAck(tcpHeader.GetAckNumber());
  if (packet->GetSize () > 0)
    {
      ReceivedData (packet, tcpHeader);
    }
}

/* Process the newly received ACK */
void
TcpSocketBase::ReceivedAck (SequenceNumber32 ack)
{
  NS_LOG_FUNCTION (this << ack);

  NS_ASSERT (m_tcb->m_segmentSize > 0);
//  SequenceNumber32 ack = tcpHeader.GetAckNumber ();

  uint32_t bytesAcked = ack - FirstUnackedSeq();
  uint32_t segsAcked  = bytesAcked / m_tcb->m_segmentSize;
  m_bytesAckedNotProcessed += bytesAcked % m_tcb->m_segmentSize;

  if (m_bytesAckedNotProcessed >= m_tcb->m_segmentSize)
    {
      segsAcked += 1;
      m_bytesAckedNotProcessed -= m_tcb->m_segmentSize;
    }

  NS_LOG_LOGIC ("Segments acked: " << segsAcked <<
                " bytes left: " << m_bytesAckedNotProcessed);

  if (ack == FirstUnackedSeq() &&
      ack < m_nextTxSequence)
    {
      // There is a DupAck
      ++m_dupAckCount;

      if (m_tcb->m_ackState == TcpSocketState::OPEN)
        {
          // From Open we go Disorder
          NS_ASSERT (m_dupAckCount == 1);
          m_tcb->m_ackState = TcpSocketState::DISORDER;
          NS_LOG_LOGIC ("Dupack. Open -> Disorder");
        }
      else if (m_tcb->m_ackState == TcpSocketState::DISORDER)
        {
          if (m_dupAckCount < m_retxThresh && m_limitedTx)
            {
              // RFC3042 Limited transmit: Send a new packet for each duplicated ACK before fast retransmit
              NS_LOG_INFO ("Limited transmit");
              NS_LOG_UNCOND("Limited transmit removed");
              uint32_t sz = 0;
//              uint32_t sz = SendDataPacket (m_nextTxSequence, GetSegSize(), true);
              m_nextTxSequence += sz;
            }
          // origina l version is ==
          else if (m_dupAckCount == m_retxThresh)
//          else if (m_dupAckCount >= m_retxThresh)
            {
              // triple duplicate ack triggers fast retransmit (RFC2582 sec.3 bullet #1)
              uint32_t oldSsThresh = m_tcb->m_ssThresh;
              m_tcb->m_ssThresh = m_congestionControl->GetSsThresh (m_tcb);
              if (oldSsThresh != m_tcb->m_ssThresh)
                {
                  // SsThresh is changed. Update cWnd. If not changed, we have
                  // an old congestion control (e.g. Tahoe)
                  m_tcb->m_cWnd = m_tcb->m_ssThresh;
                }
              m_recover = m_highTxMark;
              m_tcb->m_ackState = TcpSocketState::RECOVERY;
              m_lostOut++;
              NS_LOG_LOGIC ("Dupack. Disorder -> Recovery");
              NS_LOG_INFO ("Triple dupack. Enter fast recovery mode. Reset cwnd to " << m_tcb->m_cWnd <<
                           ", ssthresh to " << m_tcb->m_ssThresh << " at fast recovery seqnum " << m_recover);
              DoRetransmit ();
            }
          else
            {
                NS_LOG_UNCOND("m_retxThresh=" << m_retxThresh << " m_dupAckCount=" << m_dupAckCount);
//              NS_FATAL_ERROR ("m_dupAckCount > m_retxThresh and we still are in DISORDER state");
            }
        }
      else if (m_tcb->m_ackState == TcpSocketState::RECOVERY)
        {
          NS_LOG_INFO ("Dupack in fast recovery mode.");
          if (!m_sendPendingDataEvent.IsRunning ())
            {
              SendPendingData (m_connected);
            }
        }
      else if (m_tcb->m_ackState == TcpSocketState::LOSS)
        {
          // What happens if we are in loss state and a DUPack is received?
        }

      m_congestionControl->PktsAcked (m_tcb, 1, m_lastRtt);
    }
  else if (ack == FirstUnackedSeq() &&
           ack == m_nextTxSequence)
    {
      // Dupack, but the ACK is precisely equal to the nextTxSequence
    }
  else if (ack > FirstUnackedSeq())
    { // Case 3: New ACK, reset m_dupAckCount and update m_txBuffer
      NS_LOG_LOGIC ("New ack of " << ack);

      if (m_tcb->m_ackState == TcpSocketState::DISORDER)
        {
          // The network reorder packets. Linux changes the counting lost
          // packet algorithm from FACK to NewReno. We simply go back in Open.
          m_tcb->m_ackState = TcpSocketState::OPEN;
          NS_LOG_LOGIC ("New Ack. Disorder -> Open");
        }
      else if (m_tcb->m_ackState == TcpSocketState::RECOVERY)
        {
          if (ack < m_recover)
            {
              NS_LOG_INFO ("Partial ACK for seq " << ack << " in fast recovery");
              m_firstTxUnack = ack;
              UpdateTxBuffer();  //Bug 1850:  retransmit before newack
              m_retransOut--;
              DoRetransmit (); // Assume the next seq is lost. Retransmit lost packet
            }
          else if (ack >= m_recover)
            {
              m_tcb->m_ackState = TcpSocketState::OPEN;
              m_retransOut = 0;
              m_lostOut = 0;
              NS_LOG_INFO ("Received full ACK for seq " << ack <<". Leaving fast recovery with cwnd set to " << m_tcb->m_cWnd);
            }
        }
      else if (m_tcb->m_ackState == TcpSocketState::LOSS)
        {
          // Go back in OPEN state
          m_tcb->m_ackState = TcpSocketState::OPEN;
          m_retransOut = 0;
          m_lostOut = 0;
          NS_LOG_LOGIC ("New Ack. Loss -> Open");
        }

      m_congestionControl->PktsAcked (m_tcb, segsAcked-m_dupAckCount, m_lastRtt);

      for (uint32_t i=0; i<segsAcked; ++i)
        {
          m_congestionControl->IncreaseWindow (this, m_tcb);
        }

      NS_LOG_LOGIC ("Congestion control called: " <<
                    " cWnd: " << m_tcb->m_cWnd <<
                    " ssTh: " << m_tcb->m_ssThresh);

      NewAck (ack);
      m_dupAckCount = 0;
    }
}

/* Received a packet upon LISTEN state. */
void
TcpSocketBase::ProcessListen (Ptr<Packet> packet, const TcpHeader& tcpHeader,
                              const Address& fromAddress, const Address& toAddress)
{
  NS_LOG_FUNCTION (this << tcpHeader);

  // Extract the flags. PSH and URG are not honoured.
  uint8_t tcpflags = tcpHeader.GetFlags () & ~(TcpHeader::PSH | TcpHeader::URG);

  // Fork a socket if received a SYN. Do nothing otherwise.
  // C.f.: the LISTEN part in tcp_v4_do_rcv() in tcp_ipv4.c in Linux kernel
  if (tcpflags != TcpHeader::SYN)
    {
      return;
    }

  // Call socket's notify function to let the server app know we got a SYN
  // If the server app refuses the connection, do nothing
  if (!NotifyConnectionRequest (fromAddress))
    {
      return;
    }

   /**
    we first forked here, socket is not registered into TCPL4Protocol yet
   **/
   Ptr<TcpSocketBase> newSock = Fork ();
   newSock->ResetUserCallbacks ();

   Ptr<const TcpOptionMpTcpCapable> mpc;

   if (GetTcpOption (tcpHeader, mpc))
    {
      NS_ASSERT_MSG (!mpc->HasReceiverKey(), "Should not be the case");
      NS_LOG_LOGIC ("Fork & Upgrade to meta " << this);
      
      // TODO is it possible to move these to CompleteFork
      // would clutter less TcpSocketBase
      uint64_t localKey = this->GenerateUniqueMpTcpKey () ;
      Ptr<MpTcpSubflow> master = newSock->UpgradeToMeta (false, localKey, mpc->GetSenderKey());
     
      
      // TODO Move part of it to UpgradeToMeta 
      // HACK matt otherwise the new subflow sends the packet on the wroing interface
      // Now useless remove ?
      master->m_boundnetdevice = this->m_boundnetdevice;

      // Not even needed
      Ptr<MpTcpSocketBase> meta = DynamicCast<MpTcpSocketBase>(newSock);

      // We add the socket after initial parameters are correctly set so that
      // tracing doesn't contain strange values that mess up plotting
      bool result = m_tcp->AddSocket (meta);
      NS_ASSERT_MSG (result, "could not register meta");

      Simulator::ScheduleNow (&MpTcpSubflow::CompleteFork, master,
                          packet, tcpHeader, fromAddress, toAddress);

      return;
    }

  NS_LOG_LOGIC ("Cloned a TcpSocketBase " << newSock);
  Simulator::ScheduleNow (&TcpSocketBase::CompleteFork, newSock,
                          packet, tcpHeader, fromAddress, toAddress);
}


Ptr<MpTcpSubflow>
TcpSocketBase::UpgradeToMeta (bool connecting, uint64_t localKey, uint64_t peerKey)
{
  NS_LOG_FUNCTION("Upgrading to meta " << this);
  uint32_t refCount = this->GetReferenceCount();
//  NS_LOG_DEBUG( "refcount=" << this->GetReferenceCount() << " before upgrade" );
  /**
   TODO here we should find a way to call CompleteConstruct else some plots are wrong since
   TcpXxBuffers were not setup correctly (rWnd for instance)
  
   Also we don't necessarely want to create a MpTcpSubflow but maybe a subclass.
   The default subflow typeid is an attribute of meta socket which is not practical since subflow is created
   before meta. Hence move that type id to TcpL4Protocol
   */
//  MpTcpSubflow *subflow = new MpTcpSubflow (*this);
  Ptr<TcpSocketBase> temp =  this->Fork();
//  Ptr<TcpSocketBase> temp =  CopyObject<TcpSocketBase>(this);


  // TODO cancel only for forks, not when client gets upgraded Otherwise timers
  this->CancelAllTimers();
//  Ptr<MpTcpSocketBase> meta2 = DynamicCast<MpTcpSocketBase>(
//        m_tcp->CreateSocket(ns3::MpTcpSocketBase::GetTypeId())
//  );

//  *meta2 = *this;
 
  // TODO retrieve
//  Ptr<MpTcpSubflow> master = DynamicCast<MpTcpSubflow>(m_tcp->CreateSocket( ns3::MpTcpSubflow::GetTypeId() ) );
 
 
//  std::swap (*meta2, *this);
//  meta->CreateSubflow();
  
  // TODO could do sthg like CopyObject<MpTcpSubflow>(this) ?
  // Otherwise uncoimment CompleteConstruct
  // TODO first do completeConstruct, then call operator=(
//  Ptr<MpTcpSubflow> master (subflow, true);
  // CompletConstruct doit etre
//  Ptr<MpTcpSubflow> master = DynamicCast <MpTcpSubflow>(CompleteConstruct ( (TcpSocketBase*)subflow) );




//  Ipv4EndPoint *endPoint = m_endPoint;


//  this->~TcpSocketBase();
  // MpTcpSocketBase(*this) ?
//  MpTcpSocketBase* temp = new MpTcpSocketBase(*this);
//  std::memcpy (this, temp, sizeof(std::aligned_storage<sizeof(MpTcpSocketBase)>::type) ); // dest/src/size

  // I don't want the destructor to be called in that moment

  // TODO here we should be able to choose the typeid of the meta 
  MpTcpSocketBase* meta = new (this) MpTcpSocketBase();
  // update attributes with default
  CompleteConstruct (meta);
//  meta->SetCongestionControlAlgorithm( );

  while (this->GetReferenceCount() < refCount) 
  {
    this->Ref();
  }
  meta->CreateScheduler (); // meta->m_schedulerTypeId
  // Once typeids are ok, one can setup scheduler
//  temp->m_tcp->DumpSockets();

  meta->SetNode (temp->GetNode());
  
  
  // This setups tx/rx buffers, congestion control, rtt, tcp, (node ?)
  *meta = *temp;
//  meta->SetTcp (temp->m_tcp);

  meta->SetLocalKey (localKey);
  meta->SetPeerKey (peerKey);
  
  Ptr<MpTcpSubflow> master = meta->CreateSubflow (true);
//  master.Acquire();
  *master = *temp;
  NS_LOG_UNCOND (master->m_rWnd.Get () << " rwnd to compare with temp rwnd=" << temp->m_rWnd.Get ());
  if(connecting)
  {
      //! then we update
//      m_state = sf->GetState ();
//      m_mptcpLocalKey = sf->m_mptcpLocalKey;
//      m_mptcpLocalToken = sf->m_mptcpLocalToken;
//      NS_LOG_DEBUG("Set master key/token to "<< m_mptcpLocalKey << "/" << m_mptcpLocalToken);

      // Those may be overriden later
//      m_endPoint = endPoint;
//      m_endPoint6 = sf->m_endPoint6;

//      InetSocketAddress addr (endPoint->GetLocalAddress(), endPoint->GetLocalPort());
//      uint8_t id = 0;
//      bool ok = meta->AddLocalId(&id, addr);
//      NS_ASSERT_MSG (ok, "Master subflow has mptcp id " << (int) id);
//      NS_LOG_DEBUG ("Master subflow has mptcp id " << (int) id);

//      GetMeta ()->AddLocalId ();
  }

//InitLocalISN
  // we add it to tcp so that it can be freed and used for token lookup

//  MpTcpSocketBase* meta = new (this) MpTcpSocketBase();
//  meta->m_sendCb =sf->m_sendCb;
  meta->AddSubflow (master);

  // the master is always a new socket, hence we should register it
  // We can only do it when meta is set
//  bool result = m_tcp->AddSocket (master);
//  NS_ASSERT_MSG (result, "Could not register master");

  // TODO destroy and rebind
//  temp->Destroy();
//  m_tcp->AddSocket (meta);
  return master;
  
//  return 0;
}


int
TcpSocketBase::ProcessTcpOptions (const TcpHeader& header)
{
  NS_LOG_FUNCTION (this << header);

  TcpHeader::TcpOptionList options;
  header.GetOptions (options);

  for(TcpHeader::TcpOptionList::const_iterator it(options.begin()); it != options.end(); ++it)
  {
      //!
      Ptr<const TcpOption> option = *it;
      switch(option->GetKind())
      {
        case TcpOption::WINSCALE:
          if ((header.GetFlags () & TcpHeader::SYN) && m_winScalingEnabled && m_state < ESTABLISHED)
            {
              ProcessOptionWScale (option);
              ScaleSsThresh (m_sndScaleFactor);
            }
            break;
        case TcpOption::MPTCP:
            //! this will interrupt option processing but this function will be scheduled again
            //! thus some options may be processed twice, it should not trigger errors
            if(ProcessOptionMpTcp (option) != 0)
            {
                return 1;   //Means
            }
            break;
        case TcpOption::TS:
            if (m_timestampEnabled)
            {
                ProcessOptionTimestamp (option);
            }
            break;
        // Ignore those
        case TcpOption::NOP:
        case TcpOption::END:
            break;
        default:
            NS_LOG_WARN("Unsupported option [" << (int)option->GetKind() << "]");
            break;
      };
  }
  return 0;
}



void
TcpSocketBase::InitLocalISN ()
{
  NS_LOG_FUNCTION (this);
  SequenceNumber32 isn (0);
  if(!m_nullIsn)
  {
    NS_LOG_INFO("Generating Initial Sequence Number");
    isn = rand();
  }
  InitLocalISN (isn);
}

void
TcpSocketBase::InitLocalISN (const SequenceNumber32& localIsn)
{
  NS_LOG_FUNCTION (this << localIsn);

    NS_LOG_INFO ("Setting local ISN to " );
    // TODO check it was not initialized already ?
//    m_rxBuffer->SetNextRxSequence (peerIsn + SequenceNumber32 (1));
    m_nextTxSequence = localIsn;
    m_firstTxUnack = m_nextTxSequence;
    m_highTxMark = m_nextTxSequence;
    m_txBuffer->SetHeadSequence (m_nextTxSequence);
    m_localISN = m_nextTxSequence;
}

void
TcpSocketBase::InitPeerISN (const SequenceNumber32& peerIsn)
{
    NS_LOG_INFO("Setting peer ISN=" << peerIsn << " " << GetInstanceTypeId());
    // TODO check it was not initialized already ?
    m_rxBuffer->SetNextRxSequence (peerIsn + SequenceNumber32 (1));
    m_peerISN = peerIsn;
}


SequenceNumber32
TcpSocketBase::GetPeerIsn (void) const
{
    // TODO check it's connected
//    NS_ASSERT(m_connected);
    return m_peerISN;
}



SequenceNumber32
TcpSocketBase::GetLocalIsn(void) const
{
    // TODO check it's connected
//    NS_ASSERT(m_connected);
    return m_localISN;
}


/* Received a packet upon SYN_SENT */
void
TcpSocketBase::ProcessSynSent (Ptr<Packet> packet, const TcpHeader& tcpHeader)
{
  NS_LOG_FUNCTION (this << tcpHeader);
  
  // michele@polese.io : unused variables
  //uint64_t idsn;
  //uint32_t localToken;

  // Extract the flags. PSH and URG are not honoured.
  uint8_t tcpflags = tcpHeader.GetFlags () & ~(TcpHeader::PSH | TcpHeader::URG);

  if (tcpflags == 0)
    { // Bare data, accept it and move to ESTABLISHED state. This is not a normal behaviour. Remove this?
      NS_LOG_INFO ("SYN_SENT -> ESTABLISHED");
      m_state = ESTABLISHED;
      m_connected = true;
      m_retxEvent.Cancel ();
      m_delAckCount = m_delAckMaxCount;
      ReceivedData (packet, tcpHeader);
      Simulator::ScheduleNow (&TcpSocketBase::ConnectionSucceeded, this);
    }
  else if (tcpflags == TcpHeader::ACK)
    { // Ignore ACK in SYN_SENT
    }
  else if (tcpflags == TcpHeader::SYN)
    { // Received SYN, move to SYN_RCVD state and respond with SYN+ACK
      NS_LOG_INFO ("SYN_SENT -> SYN_RCVD");
      m_state = SYN_RCVD;
      m_cnCount = m_cnRetries;
//      m_rxBuffer->SetNextRxSequence (tcpHeader.GetSequenceNumber () + SequenceNumber32 (1));
      InitPeerISN (tcpHeader.GetSequenceNumber ());
      SendEmptyPacket (TcpHeader::SYN | TcpHeader::ACK);
    }
  else if (tcpflags == (TcpHeader::SYN | TcpHeader::ACK)
           && m_nextTxSequence + SequenceNumber32 (1) == tcpHeader.GetAckNumber ())
    { // Handshake completed


      Ptr<const TcpOptionMpTcpCapable> mpc;
      
      // Process the MP_CAPABLE only once, i.e., if we are not a subflow already
      // else, we end up processing ad vitam aeternam
//      NS_LOG_DEBUG ("Child of subflow ? instance typeid=" << GetInstanceTypeId()
//          << " to compare with " << MpTcpSubflow::GetTypeId() );
        
      if( !GetInstanceTypeId().IsChildOf(MpTcpSubflow::GetTypeId(), false) 
        && GetTcpOption(tcpHeader, mpc))
      {
        // save endpoint because its value is destroyed while upgrading to meta
        // these are backupo variables
        Ipv4EndPoint* endPoint = m_endPoint;
        Ptr<NetDevice> boundDev = m_boundnetdevice;
        
        
        NS_LOG_DEBUG("MATT " << this << " "<< GetInstanceTypeId());
        NS_LOG_DEBUG( "refcount=" << this->GetReferenceCount() << " before upgrade" );
       
        // master = first subflow
        Ptr<MpTcpSubflow> master = UpgradeToMeta (true, m_mptcpLocalKey, mpc->GetSenderKey() );
        master->ResetUserCallbacks ();

          // Subflow now created with m_tcp->CreateSocket=> already registered
//        bool result = m_tcp->AddSocket (master);
//        NS_ASSERT_MSG (result, "Could not register master");

        // Hack to retrigger the tcpL4protocol::OnNewSocket callback
        m_tcp->NotifyNewSocket (this);

        // Need to register an id
        InetSocketAddress addr (endPoint->GetLocalAddress(), endPoint->GetLocalPort());
        MpTcpSocketBase* meta = (MpTcpSocketBase*)this;
        uint8_t id = 0;
        bool ok = meta->AddLocalId (&id, addr);
        NS_ASSERT_MSG (ok, "Master subflow has mptcp id " << (int) id);
        NS_LOG_DEBUG ("Master subflow has mptcp id " << (int) id);

        // HACK matt otherwise the new subflow sends the packet to the wrong socket, i.e., its previous
        // owner, which has now become the meta
        master->m_boundnetdevice = boundDev;
        master->m_endPoint = endPoint;
        master->SetupCallback();

//        NS_LOG_DEBUG("MATT2 end of upgrade " << this << " "<< GetInstanceTypeId());
        NS_LOG_DEBUG("Local/peer ISNs: " << this->GetLocalIsn() << "/"<< this->GetPeerIsn()
            << "(peer isn set later by processSynSent)");
        NS_LOG_DEBUG( "refcount=" << this->GetReferenceCount() << " after upgrade" );
//        this->Ref();
//        master->Bind();

        // Carrément le renvoyer à forwardUp pour forcer la relecture de la windowsize
        
        Simulator::ScheduleNow ( &MpTcpSubflow::ProcessSynSent, master, packet, tcpHeader);
        return;
      }

      m_connected = true;
      m_retxEvent.Cancel ();
      InitPeerISN (tcpHeader.GetSequenceNumber ());
//      m_rxBuffer->SetNextRxSequence (tcpHeader.GetSequenceNumber () + SequenceNumber32 (1));
      m_highTxMark = ++m_nextTxSequence;
      m_txBuffer->SetHeadSequence (m_nextTxSequence);
      m_firstTxUnack = m_nextTxSequence;

      NS_LOG_INFO ("SYN_SENT -> ESTABLISHED");
      m_state = ESTABLISHED;

      SendEmptyPacket (TcpHeader::ACK);
      SendPendingData (m_connected);
      Simulator::ScheduleNow (&TcpSocketBase::ConnectionSucceeded, this);
      // Always respond to first data packet to speed up the connection.
      // Remove to get the behaviour of old NS-3 code.
      m_delAckCount = m_delAckMaxCount;
    }
  else
    { // Other in-sequence input
      if (tcpflags != TcpHeader::RST)
        { // When (1) rx of FIN+ACK; (2) rx of FIN; (3) rx of bad flags
          NS_LOG_LOGIC ("Illegal flag " << TcpHeader::FlagsToString (tcpflags) <<
                        " received. Reset packet is sent.");
          SendRST ();
        }
      CloseAndNotify ();
    }
}


/* Received a packet upon SYN_RCVD */
void
TcpSocketBase::ProcessSynRcvd (Ptr<Packet> packet, const TcpHeader& tcpHeader,
                               const Address& fromAddress, const Address& toAddress)
{
  NS_LOG_FUNCTION (this << tcpHeader);

  // Extract the flags. PSH and URG are not honoured.
  uint8_t tcpflags = tcpHeader.GetFlags () & ~(TcpHeader::PSH | TcpHeader::URG);

  if (tcpflags == 0
      || (tcpflags == TcpHeader::ACK
          && m_nextTxSequence + SequenceNumber32 (1) == tcpHeader.GetAckNumber ()))
    { // If it is bare data, accept it and move to ESTABLISHED state. This is
      // possibly due to ACK lost in 3WHS. If in-sequence ACK is received, the
      // handshake is completed nicely.
      NS_LOG_INFO ("SYN_RCVD -> ESTABLISHED");
      m_state = ESTABLISHED;
      m_connected = true;
      m_retxEvent.Cancel ();
      m_highTxMark = ++m_nextTxSequence;
//      SetTxHead()
      m_txBuffer->SetHeadSequence (m_nextTxSequence);
      m_firstTxUnack = m_nextTxSequence;


      if (m_endPoint)
        {
          m_endPoint->SetPeer (InetSocketAddress::ConvertFrom (fromAddress).GetIpv4 (),
                               InetSocketAddress::ConvertFrom (fromAddress).GetPort ());
        }
      else if (m_endPoint6)
        {
          m_endPoint6->SetPeer (Inet6SocketAddress::ConvertFrom (fromAddress).GetIpv6 (),
                                Inet6SocketAddress::ConvertFrom (fromAddress).GetPort ());
        }
      // Always respond to first data packet to speed up the connection.
      // Remove to get the behaviour of old NS-3 code.
      m_delAckCount = m_delAckMaxCount;

      ProcessTcpOptions (tcpHeader);
//      if(){
//        return;
//      }
      // TODO copy this socket into a subflow
      // or create
//      Ptr<MpTcpSubflow> sf = CopyObject<MpTcpSubflow>();
      // TODO check a faire dans Listen aussi

      // TODO do it afterwards, schedule it for instance ?
      ReceivedAck (packet, tcpHeader);


      NotifyNewConnectionCreated (this, fromAddress);

      // As this connection is established, the socket is available to send data now
      if (GetTxAvailable () > 0)
        {
          NotifySend (GetTxAvailable ());
        }
    }
  else if (tcpflags == TcpHeader::SYN)
    { // Probably the peer lost my SYN+ACK
      InitPeerISN (tcpHeader.GetSequenceNumber ());
//      m_rxBuffer->SetNextRxSequence (tcpHeader.GetSequenceNumber () + SequenceNumber32 (1));
      SendEmptyPacket (TcpHeader::SYN | TcpHeader::ACK);
    }
  else if (tcpflags == (TcpHeader::FIN | TcpHeader::ACK))
    {
      if (tcpHeader.GetSequenceNumber () == m_rxBuffer->NextRxSequence ())
        { // In-sequence FIN before connection complete. Set up connection and close.
          m_connected = true;
          m_retxEvent.Cancel ();
          m_highTxMark = ++m_nextTxSequence;
          m_txBuffer->SetHeadSequence (m_nextTxSequence);
          m_firstTxUnack =  m_nextTxSequence;
          if (m_endPoint)
            {
              m_endPoint->SetPeer (InetSocketAddress::ConvertFrom (fromAddress).GetIpv4 (),
                                   InetSocketAddress::ConvertFrom (fromAddress).GetPort ());
            }
          else if (m_endPoint6)
            {
              m_endPoint6->SetPeer (Inet6SocketAddress::ConvertFrom (fromAddress).GetIpv6 (),
                                    Inet6SocketAddress::ConvertFrom (fromAddress).GetPort ());
            }
          PeerClose (packet, tcpHeader);
        }
    }
  else
    { // Other in-sequence input
      if (tcpflags != TcpHeader::RST)
        { // When (1) rx of SYN+ACK; (2) rx of FIN; (3) rx of bad flags
          NS_LOG_LOGIC ("Illegal flag " << TcpHeader::FlagsToString (tcpflags) <<
                        " received. Reset packet is sent.");
          if (m_endPoint)
            {
              m_endPoint->SetPeer (InetSocketAddress::ConvertFrom (fromAddress).GetIpv4 (),
                                   InetSocketAddress::ConvertFrom (fromAddress).GetPort ());
            }
          else if (m_endPoint6)
            {
              m_endPoint6->SetPeer (Inet6SocketAddress::ConvertFrom (fromAddress).GetIpv6 (),
                                    Inet6SocketAddress::ConvertFrom (fromAddress).GetPort ());
            }
          SendRST ();
        }
      CloseAndNotify ();
    }
}

/* Received a packet upon CLOSE_WAIT, FIN_WAIT_1, or FIN_WAIT_2 states */
void
TcpSocketBase::ProcessWait (Ptr<Packet> packet, const TcpHeader& tcpHeader)
{
  NS_LOG_FUNCTION (this << tcpHeader);

  // Extract the flags. PSH and URG are not honoured.
  uint8_t tcpflags = tcpHeader.GetFlags () & ~(TcpHeader::PSH | TcpHeader::URG);

  if (packet->GetSize () > 0 && tcpflags != TcpHeader::ACK)
    { // Bare data, accept it
      ReceivedData (packet, tcpHeader);
    }
  else if (tcpflags == TcpHeader::ACK)
    { // Process the ACK, and if in FIN_WAIT_1, conditionally move to FIN_WAIT_2
      ReceivedAck (packet, tcpHeader);
      if (m_state == FIN_WAIT_1 && m_txBuffer->Size () == 0
          && tcpHeader.GetAckNumber () == m_highTxMark + SequenceNumber32 (1))
        { // This ACK corresponds to the FIN sent
          NS_LOG_INFO ("FIN_WAIT_1 -> FIN_WAIT_2");
          m_state = FIN_WAIT_2;
        }
    }
  else if (tcpflags == TcpHeader::FIN || tcpflags == (TcpHeader::FIN | TcpHeader::ACK))
    { // Got FIN, respond with ACK and move to next state
      if (tcpflags & TcpHeader::ACK)
        { // Process the ACK first
          ReceivedAck (packet, tcpHeader);
        }
      m_rxBuffer->SetFinSequence (tcpHeader.GetSequenceNumber ());
    }
  else if (tcpflags == TcpHeader::SYN || tcpflags == (TcpHeader::SYN | TcpHeader::ACK))
    { // Duplicated SYN or SYN+ACK, possibly due to spurious retransmission
      return;
    }
  else
    { // This is a RST or bad flags
      if (tcpflags != TcpHeader::RST)
        {
          NS_LOG_LOGIC ("Illegal flag " << TcpHeader::FlagsToString (tcpflags) <<
                        " received. Reset packet is sent.");
          SendRST ();
        }
      CloseAndNotify ();
      return;
    }

  // Check if the close responder sent an in-sequence FIN, if so, respond ACK
  if ((m_state == FIN_WAIT_1 || m_state == FIN_WAIT_2) && m_rxBuffer->Finished ())
    {
      if (m_state == FIN_WAIT_1)
        {
          NS_LOG_INFO ("FIN_WAIT_1 -> CLOSING");
          m_state = CLOSING;
          if (m_txBuffer->Size () == 0
              && tcpHeader.GetAckNumber () == m_highTxMark + SequenceNumber32 (1))
            { // This ACK corresponds to the FIN sent
              TimeWait ();
            }
        }
      else if (m_state == FIN_WAIT_2)
        {
          TimeWait ();
        }
      SendEmptyPacket (TcpHeader::ACK);
      if (!m_shutdownRecv)
        {
          NotifyDataRecv ();
        }
    }
}

/* Received a packet upon CLOSING */
void
TcpSocketBase::ProcessClosing (Ptr<Packet> packet, const TcpHeader& tcpHeader)
{
  NS_LOG_FUNCTION (this << tcpHeader);

  // Extract the flags. PSH and URG are not honoured.
  uint8_t tcpflags = tcpHeader.GetFlags () & ~(TcpHeader::PSH | TcpHeader::URG);

  if (tcpflags == TcpHeader::ACK)
    {
      if (tcpHeader.GetSequenceNumber () == m_rxBuffer->NextRxSequence ())
        { // This ACK corresponds to the FIN sent
          ProcessTcpOptions (tcpHeader);
          TimeWait ();
        }
    }
  else
    { // CLOSING state means simultaneous close, i.e. no one is sending data to
      // anyone. If anything other than ACK is received, respond with a reset.
      if (tcpflags == TcpHeader::FIN || tcpflags == (TcpHeader::FIN | TcpHeader::ACK))
        { // FIN from the peer as well. We can close immediately.
          ProcessTcpOptions(tcpHeader);
          SendEmptyPacket (TcpHeader::ACK);
        }
      else if (tcpflags != TcpHeader::RST)
        { // Receive of SYN or SYN+ACK or bad flags or pure data
          NS_LOG_LOGIC ("Illegal flag " << TcpHeader::FlagsToString (tcpflags) << " received. Reset packet is sent.");
          SendRST ();
        }
      CloseAndNotify ();
    }
}

/* Received a packet upon LAST_ACK */
void
TcpSocketBase::ProcessLastAck (Ptr<Packet> packet, const TcpHeader& tcpHeader)
{
  NS_LOG_FUNCTION (this << tcpHeader);

  // Extract the flags. PSH and URG are not honoured.
  uint8_t tcpflags = tcpHeader.GetFlags () & ~(TcpHeader::PSH | TcpHeader::URG);

  if (tcpflags == 0)
    {
      ProcessTcpOptions(tcpHeader);
      ReceivedData (packet, tcpHeader);
    }
  else if (tcpflags == TcpHeader::ACK)
    {
      if (tcpHeader.GetSequenceNumber () == m_rxBuffer->NextRxSequence ())
        { // This ACK corresponds to the FIN sent. This socket closed peacefully.
          ProcessTcpOptions(tcpHeader);
          CloseAndNotify ();
        }
    }
  else if (tcpflags == TcpHeader::FIN)
    { // Received FIN again, the peer probably lost the FIN+ACK
      ProcessTcpOptions(tcpHeader);
      SendEmptyPacket (TcpHeader::FIN | TcpHeader::ACK);
    }
  else if (tcpflags == (TcpHeader::FIN | TcpHeader::ACK) || tcpflags == TcpHeader::RST)
    {
      ProcessTcpOptions(tcpHeader);
      CloseAndNotify ();
    }
  else
    { // Received a SYN or SYN+ACK or bad flags
      NS_LOG_LOGIC ("Illegal flag " << TcpHeader::FlagsToString (tcpflags) << " received. Reset packet is sent.");
      SendRST ();
      CloseAndNotify ();
    }
}

/* Peer sent me a FIN. Remember its sequence in rx buffer. */
void
TcpSocketBase::PeerClose (Ptr<Packet> p, const TcpHeader& tcpHeader)
{
  NS_LOG_FUNCTION (this << tcpHeader);

  // Ignore all out of range packets
  if (tcpHeader.GetSequenceNumber () < m_rxBuffer->NextRxSequence ()
      || tcpHeader.GetSequenceNumber () > m_rxBuffer->MaxRxSequence ())
    {
      return;
    }
  // For any case, remember the FIN position in rx buffer first
  m_rxBuffer->SetFinSequence (tcpHeader.GetSequenceNumber () + SequenceNumber32 (p->GetSize ()));
  NS_LOG_LOGIC ("Accepted FIN at seq " << tcpHeader.GetSequenceNumber () + SequenceNumber32 (p->GetSize ()));
  // If there is any piggybacked data, process it
  if (p->GetSize ())
    {
      ReceivedData (p, tcpHeader);
    }
  // Return if FIN is out of sequence, otherwise move to CLOSE_WAIT state by DoPeerClose
  if (!m_rxBuffer->Finished ())
    {
      return;
    }

  // Simultaneous close: Application invoked Close() when we are processing this FIN packet
  if (m_state == FIN_WAIT_1)
    {
      NS_LOG_INFO ("FIN_WAIT_1 -> CLOSING");
      m_state = CLOSING;
      return;
    }

  DoPeerClose (); // Change state, respond with ACK
}

/* Received a in-sequence FIN. Close down this socket. */
void
TcpSocketBase::DoPeerClose (void)
{
  NS_ASSERT (m_state == ESTABLISHED || m_state == SYN_RCVD);

  // Move the state to CLOSE_WAIT
  NS_LOG_INFO (TcpStateName[m_state] << " -> CLOSE_WAIT");
  m_state = CLOSE_WAIT;

  if (!m_closeNotified)
    {
      // The normal behaviour for an application is that, when the peer sent a in-sequence
      // FIN, the app should prepare to close. The app has two choices at this point: either
      // respond with ShutdownSend() call to declare that it has nothing more to send and
      // the socket can be closed immediately; or remember the peer's close request, wait
      // until all its existing data are pushed into the TCP socket, then call Close()
      // explicitly.
      NS_LOG_LOGIC ("TCP " << this << " calling NotifyNormalClose");
      NotifyNormalClose ();
      m_closeNotified = true;
    }
  if (m_shutdownSend)
    { // The application declares that it would not sent any more, close this socket
      Close ();
    }
  else
    { // Need to ack, the application will close later
      SendEmptyPacket (TcpHeader::ACK);
    }
  if (m_state == LAST_ACK)
    {
      NS_LOG_LOGIC ("TcpSocketBase " << this << " scheduling LATO1");
      Time lastRto = m_rtt->GetEstimate () + Max (m_clockGranularity, m_rtt->GetVariation ()*4);
      m_lastAckEvent = Simulator::Schedule (lastRto, &TcpSocketBase::LastAckTimeout, this);
    }
}

/* Kill this socket. This is a callback function configured to m_endpoint in
   SetupCallback(), invoked when the endpoint is destroyed. */
void
TcpSocketBase::Destroy (void)
{
  NS_LOG_FUNCTION (this);
  m_endPoint = 0;
  if (m_tcp != 0)
    {
      m_tcp->RemoveSocket(this);
    }
  NS_LOG_LOGIC (this << " Cancelled ReTxTimeout event which was set to expire at " <<
                (Simulator::Now () + Simulator::GetDelayLeft (m_retxEvent)).GetSeconds ());
  CancelAllTimers ();
}

/* Kill this socket. This is a callback function configured to m_endpoint in
   SetupCallback(), invoked when the endpoint is destroyed. */
void
TcpSocketBase::Destroy6 (void)
{
  NS_LOG_FUNCTION (this);
  m_endPoint6 = 0;
  if (m_tcp != 0)
    {
      m_tcp->RemoveSocket(this);
    }
  NS_LOG_LOGIC (this << " Cancelled ReTxTimeout event which was set to expire at " <<
                (Simulator::Now () + Simulator::GetDelayLeft (m_retxEvent)).GetSeconds ());
  CancelAllTimers ();
}

void
TcpSocketBase::GenerateEmptyPacketHeader(TcpHeader& header, uint8_t flags)
{
  NS_LOG_FUNCTION (this << TcpHeader::FlagsToString(flags));

  SequenceNumber32 s = m_nextTxSequence;

  if (flags & TcpHeader::FIN)
    {
      flags |= TcpHeader::ACK;
    }
  //!
  else if (m_state == FIN_WAIT_1 || m_state == LAST_ACK || m_state == CLOSING)
    {
      ++s;
    }


  header.SetFlags (flags);
  header.SetSequenceNumber (s);

  if(flags & TcpHeader::ACK)
  {
        header.SetAckNumber (m_rxBuffer->NextRxSequence ());
  }

//  header.SetAckNumber (m_rxBuffer->NextRxSequence ());
  NS_LOG_DEBUG(this << " endpoint=" << m_endPoint);
  if (m_endPoint != 0)
    {
      header.SetSourcePort (m_endPoint->GetLocalPort ());
      header.SetDestinationPort (m_endPoint->GetPeerPort ());
    }
  else
    {
      NS_ASSERT(m_endPoint6 != 0);
      header.SetSourcePort (m_endPoint6->GetLocalPort ());
      header.SetDestinationPort (m_endPoint6->GetPeerPort ());
    }
//  AddOptions (header);
  header.SetWindowSize (AdvertisedWindowSize ());
}


void
TcpSocketBase::SendPacket(TcpHeader header, Ptr<Packet> p)
{
  NS_LOG_LOGIC ("Send packet via TcpL4Protocol with flags"
//                << TcpHeader::FlagsToString (flags)
                );

  // Matt this might be a problem later on
  NS_ASSERT (header.GetWindowSize() == AdvertisedWindowSize ());


  /*
   * Add tags for each socket option.
   * Note that currently the socket adds both IPv4 tag and IPv6 tag
   * if both options are set. Once the packet got to layer three, only
   * the corresponding tags will be read.
   */
  if (IsManualIpTos ())
    {
      SocketIpTosTag ipTosTag;
      ipTosTag.SetTos (GetIpTos ());
      p->AddPacketTag (ipTosTag);
    }

  if (IsManualIpv6Tclass ())
    {
      SocketIpv6TclassTag ipTclassTag;
      ipTclassTag.SetTclass (GetIpv6Tclass ());
      p->AddPacketTag (ipTclassTag);
    }

  if (IsManualIpTtl ())
    {
      SocketIpTtlTag ipTtlTag;
      ipTtlTag.SetTtl (GetIpTtl ());
      p->AddPacketTag (ipTtlTag);
    }

  if (IsManualIpv6HopLimit ())
    {
      SocketIpv6HopLimitTag ipHopLimitTag;
      ipHopLimitTag.SetHopLimit (GetIpv6HopLimit ());
      p->AddPacketTag (ipHopLimitTag);
    }

  AddOptions (header);

  if (m_endPoint != 0)
    {
      m_tcp->SendPacket (p, header, m_endPoint->GetLocalAddress (),
                         m_endPoint->GetPeerAddress (), m_boundnetdevice);
    }
  else
    {
      NS_ASSERT(m_endPoint6 != 0);
      m_tcp->SendPacket (p, header, m_endPoint6->GetLocalAddress (),
                         m_endPoint6->GetPeerAddress (), m_boundnetdevice);
    }


  if (header.GetFlags() & TcpHeader::ACK)
    { // If sending an ACK, cancel the delay ACK as well
      m_delAckEvent.Cancel ();
      m_delAckCount = 0;
    }
}

/* Send an empty packet with specified TCP flags
TODO pass on a sequence number ?
*/
void
TcpSocketBase::SendEmptyPacket (uint8_t flags)
{
    TcpHeader header;
    GenerateEmptyPacketHeader(header, flags);
    SendEmptyPacket(header);
}

void
TcpSocketBase::SendEmptyPacket (TcpHeader& header)
{
//  NS_LOG_FUNCTION (this << (uint32_t)flags);
  NS_LOG_FUNCTION (this << header);
  Ptr<Packet> p = Create<Packet> ();


  if (m_endPoint == 0 && m_endPoint6 == 0)
    {
      NS_LOG_WARN ("Failed to send empty packet due to null endpoint");
      return;
    }

  // RFC 6298, clause 2.4
  // TODO GetRTO
  m_rto = Max (m_rtt->GetEstimate () + Max (m_clockGranularity, m_rtt->GetVariation ()*4), m_minRto);
  uint8_t flags = header.GetFlags();
  bool hasSyn = flags & TcpHeader::SYN;
  bool hasFin = flags & TcpHeader::FIN;
  bool isAck = flags == TcpHeader::ACK;
  if (hasSyn)
    {
      if (m_cnCount == 0)
        { // No more connection retries, give up
          NS_LOG_LOGIC ("Connection failed.");
          m_rtt->Reset (); //According to recommendation -> RFC 6298
          CloseAndNotify ();
          return;
        }
      else
        { // Exponential backoff of connection time out
          int backoffCount = 0x1 << (m_cnRetries - m_cnCount);
          m_rto = m_cnTimeout * backoffCount;
          m_cnCount--;
        }
    }
    SendPacket(header, p);

  if (m_retxEvent.IsExpired () && (hasSyn || hasFin) && !isAck )
    { // Retransmit SYN / SYN+ACK / FIN / FIN+ACK to guard against lost
      NS_LOG_LOGIC ("Schedule retransmission timeout at time "
                    << Simulator::Now ().GetSeconds () << " to expire at time "
                    << (Simulator::Now () + m_rto.Get ()).GetSeconds ());
      m_retxEvent = Simulator::Schedule (m_rto, (void (TcpSocketBase::*)(uint8_t ))&TcpSocketBase::SendEmptyPacket, this, header.GetFlags());
    }
}

/* This function closes the endpoint completely. Called upon RST_TX action. */
void
TcpSocketBase::SendRST (void)
{
  NS_LOG_FUNCTION (this);
  SendEmptyPacket (TcpHeader::RST);
  NotifyErrorClose ();
  DeallocateEndPoint ();
}

/* Deallocate the end point and cancel all the timers */
void
TcpSocketBase::DeallocateEndPoint (void)
{
  if (m_endPoint != 0)
    {
      CancelAllTimers ();
      m_endPoint->SetDestroyCallback (MakeNullCallback<void> ());
      m_tcp->DeAllocate (m_endPoint);
      m_endPoint = 0;
      m_tcp->RemoveSocket(this);
    }
  else if (m_endPoint6 != 0)
    {
      CancelAllTimers ();
      m_endPoint6->SetDestroyCallback (MakeNullCallback<void> ());
      m_tcp->DeAllocate (m_endPoint6);
      m_endPoint6 = 0;
      m_tcp->RemoveSocket(this);
    }
}

/* Configure the endpoint to a local address. Called by Connect() if Bind() didn't specify one. */
int
TcpSocketBase::SetupEndpoint ()
{
  NS_LOG_FUNCTION (this);
  Ptr<Ipv4> ipv4 = m_node->GetObject<Ipv4> ();
  NS_ASSERT (ipv4 != 0);
  if (ipv4->GetRoutingProtocol () == 0)
    {
      NS_FATAL_ERROR ("No Ipv4RoutingProtocol in the node");
    }
  // Create a dummy packet, then ask the routing function for the best output
  // interface's address
  Ipv4Header header;
  header.SetDestination (m_endPoint->GetPeerAddress ());
  Socket::SocketErrno errno_;
  Ptr<Ipv4Route> route;
  Ptr<NetDevice> oif = m_boundnetdevice;
  route = ipv4->GetRoutingProtocol ()->RouteOutput (Ptr<Packet> (), header, oif, errno_);
  if (route == 0)
    {
      NS_LOG_LOGIC ("Route to " << m_endPoint->GetPeerAddress () << " does not exist");
      NS_LOG_ERROR (errno_);
      m_errno = errno_;
      return -1;
    }
  NS_LOG_LOGIC ("Route exists");
  m_endPoint->SetLocalAddress (route->GetSource ());
  return 0;
}

int
TcpSocketBase::SetupEndpoint6 ()
{
  NS_LOG_FUNCTION (this);
  Ptr<Ipv6L3Protocol> ipv6 = m_node->GetObject<Ipv6L3Protocol> ();
  NS_ASSERT (ipv6 != 0);
  if (ipv6->GetRoutingProtocol () == 0)
    {
      NS_FATAL_ERROR ("No Ipv6RoutingProtocol in the node");
    }
  // Create a dummy packet, then ask the routing function for the best output
  // interface's address
  Ipv6Header header;
  header.SetDestinationAddress (m_endPoint6->GetPeerAddress ());
  Socket::SocketErrno errno_;
  Ptr<Ipv6Route> route;
  Ptr<NetDevice> oif = m_boundnetdevice;
  route = ipv6->GetRoutingProtocol ()->RouteOutput (Ptr<Packet> (), header, oif, errno_);
  if (route == 0)
    {
      NS_LOG_LOGIC ("Route to " << m_endPoint6->GetPeerAddress () << " does not exist");
      NS_LOG_ERROR (errno_);
      m_errno = errno_;
      return -1;
    }
  NS_LOG_LOGIC ("Route exists");
  m_endPoint6->SetLocalAddress (route->GetSource ());
  return 0;
}

/* This function is called only if a SYN received in LISTEN state. After
   TcpSocketBase cloned, allocate a new end point to handle the incoming
   connection and send a SYN+ACK to complete the handshake. */
void
TcpSocketBase::CompleteFork (Ptr<const Packet> p, const TcpHeader& h,
                             const Address& fromAddress, const Address& toAddress)
{
  NS_LOG_FUNCTION (this);

  // Get port and address from peer (connecting host)
  if (InetSocketAddress::IsMatchingType (toAddress))
    {
      m_endPoint = m_tcp->Allocate (InetSocketAddress::ConvertFrom (toAddress).GetIpv4 (),
                                    InetSocketAddress::ConvertFrom (toAddress).GetPort (),
                                    InetSocketAddress::ConvertFrom (fromAddress).GetIpv4 (),
                                    InetSocketAddress::ConvertFrom (fromAddress).GetPort ());

      Ptr<NetDevice> dev = MapIpToInterface(InetSocketAddress::ConvertFrom (toAddress).GetIpv4 ());
      if(dev) {
        NS_LOG_UNCOND("device found; binding to it");
        m_endPoint->BindToNetDevice(dev);
        m_boundnetdevice = m_endPoint->GetBoundNetDevice();
      }

      /* la il faut chercher */
      m_endPoint6 = 0;
      NS_ASSERT(m_endPoint);
    }
  else if (Inet6SocketAddress::IsMatchingType (toAddress))
    {
      m_endPoint6 = m_tcp->Allocate6 (Inet6SocketAddress::ConvertFrom (toAddress).GetIpv6 (),
                                      Inet6SocketAddress::ConvertFrom (toAddress).GetPort (),
                                      Inet6SocketAddress::ConvertFrom (fromAddress).GetIpv6 (),
                                      Inet6SocketAddress::ConvertFrom (fromAddress).GetPort ());
      m_endPoint = 0;
      NS_ASSERT(m_endPoint6);
    }
    
  // Change the cloned socket from LISTEN state to SYN_RCVD
  NS_LOG_INFO ("LISTEN -> SYN_RCVD");
  m_state = SYN_RCVD;
  m_cnCount = m_cnRetries;
  SetupCallback ();
  
  // TODO move out
  // Set the sequence number and send SYN+ACK
  InitLocalISN ();
  InitPeerISN (h.GetSequenceNumber ());
  // No need for UpdateWindowSize here ?!
  m_rWnd = h.GetWindowSize();
  
  // Call addsocket after setting up initial parameters to get nice plots
  bool result = m_tcp->AddSocket(this);
  if(!result)
  {
    NS_LOG_WARN ("Can't add socket: already registered ? Can be because of mptcp");
  }

  SendEmptyPacket (TcpHeader::SYN | TcpHeader::ACK);
}

void
TcpSocketBase::ConnectionSucceeded ()
{ // Wrapper to protected function NotifyConnectionSucceeded() so that it can
  // be called as a scheduled event
  NotifyConnectionSucceeded ();
  // The if-block below was moved from ProcessSynSent() to here because we need
  // to invoke the NotifySend() only after NotifyConnectionSucceeded() to
  // reflect the behaviour in the real world.
  if (GetTxAvailable () > 0)
    {
      NotifySend (GetTxAvailable ());
    }
}

/* Extract at most maxSize bytes from the TxBuffer at sequence seq, add the
    TCP header, and send to TcpL4Protocol */
uint32_t
TcpSocketBase::SendDataPacket (SequenceNumber32 seq, uint32_t maxSize, bool withAck)
{
    //!<
    TcpHeader header;
    GenerateEmptyPacketHeader(header, withAck ? TcpHeader::ACK : 0);
    return SendDataPacket(header, seq, maxSize);
}


uint32_t
TcpSocketBase::SendDataPacket (TcpHeader& header, SequenceNumber32 seq, uint32_t maxSize)
{
  NS_LOG_FUNCTION (this << seq << maxSize);

  bool isRetransmission = false;
  if ( seq == FirstUnackedSeq() )
    {
      isRetransmission = true;
    }

  Ptr<Packet> p = m_txBuffer->CopyFromSequence (maxSize, seq);
  uint32_t sz = p->GetSize (); // Size of packet
  uint32_t remainingData = m_txBuffer->SizeFromSequence (seq + SequenceNumber32 (sz));

  if (m_closeOnEmpty && (remainingData == 0))
    {
      header.SetFlags(TcpHeader::FIN | header.GetFlags());
      if (m_state == ESTABLISHED)
        { // On active close: I am the first one to send FIN
          NS_LOG_INFO ("ESTABLISHED -> FIN_WAIT_1");
          m_state = FIN_WAIT_1;
        }
      else if (m_state == CLOSE_WAIT)
        { // On passive close: Peer sent me FIN already
          NS_LOG_INFO ("CLOSE_WAIT -> LAST_ACK");
          m_state = LAST_ACK;
        }
    }

  header.SetSequenceNumber (seq);
//  AddOptions (header);

  if (m_retxEvent.IsExpired () )
    {
      // RFC 6298, clause 2.5
      Time doubledRto = m_rto + m_rto;
      m_rto = Min (doubledRto, Time::FromDouble (60,  Time::S));

      // Schedules retransmit

      NS_LOG_LOGIC (this << " SendDataPacket Schedule ReTxTimeout at time " <<
                    Simulator::Now ().GetSeconds () << " to expire at time " <<
                    (Simulator::Now () + m_rto.Get ()).GetSeconds () );
      m_retxEvent = Simulator::Schedule (m_rto, &TcpSocketBase::ReTxTimeout, this);
    }


  SendPacket(header, p);

  // update the history of sequence numbers used to calculate the RTT
  if (isRetransmission == false)
    { // This is the next expected one, just log at end
      m_history.push_back (RttHistory (seq, sz, Simulator::Now () ));
    }
  else
    { // This is a retransmit, find in list and mark as re-tx
      for (RttHistory_t::iterator i = m_history.begin (); i != m_history.end (); ++i)
        {
          if ((seq >= i->seq) && (seq < (i->seq + SequenceNumber32 (i->count))))
            { // Found it
              i->retx = true;
              i->count = ((seq + SequenceNumber32 (sz)) - i->seq); // And update count in hist
              break;
            }
        }
    }

  // Notify the application of the data being sent unless this is a retransmit
  if (seq == m_highTxMark)
    {
      Simulator::ScheduleNow (&TcpSocketBase::NotifyDataSent, this, sz);
    }
  // Update highTxMark
  m_highTxMark = std::max (seq + sz, m_highTxMark.Get ());
  return sz;
}

/* Send as much pending data as possible according to the Tx window. Note that
 *  this function did not implement the PSH flag
 */
bool
TcpSocketBase::SendPendingData (bool withAck)
{
  NS_LOG_FUNCTION (this << withAck << " in state " << TcpStateName[m_state]);
  if (m_txBuffer->Size () == 0)
    {
      NS_LOG_DEBUG("Nothing to send");
      return false;                           // Nothing to send
    }
  if (m_endPoint == 0 && m_endPoint6 == 0)
    {
      NS_LOG_INFO ("TcpSocketBase::SendPendingData: No endpoint; m_shutdownSend=" << m_shutdownSend);
      return false; // Is this the right way to handle this condition?
    }
  uint32_t nPacketsSent = 0;
  while (m_txBuffer->SizeFromSequence (m_nextTxSequence))
    {
      uint32_t w = AvailableWindow (); // Get available window size
      // Stop sending if we need to wait for a larger Tx window (prevent silly window syndrome)
      if (w < m_tcb->m_segmentSize && m_txBuffer->SizeFromSequence (m_nextTxSequence) > w)
        {
          NS_LOG_LOGIC ("Preventing Silly Window Syndrome. Wait to send.");
          break; // No more
        }
      // Nagle's algorithm (RFC896): Hold off sending if there is unacked data
      // in the buffer and the amount of data to send is less than one segment
      if (!m_noDelay && UnAckDataCount () > 0
          && m_txBuffer->SizeFromSequence (m_nextTxSequence) < m_tcb->m_segmentSize)
        {
          NS_LOG_LOGIC ("Invoking Nagle's algorithm. Wait to send.");
          break;
        }
      NS_LOG_LOGIC ("TcpSocketBase " << this << " SendPendingData" <<
                    " w " << w <<
                    " rxwin " << GetRwnd () <<
                    " segsize " << m_tcb->m_segmentSize <<
//                    " nextTxSeq " <<  <<
                    " highestRxAck " << FirstUnackedSeq() <<
                    " TxBufferSize=" << m_txBuffer->Size () <<
                    " pd->SizeFromSequence( " << m_nextTxSequence << ")="
                    << m_txBuffer->SizeFromSequence (m_nextTxSequence));
      uint32_t s = std::min (w, m_tcb->m_segmentSize);  // Send no more than window
      uint32_t sz = SendDataPacket (m_nextTxSequence, s, withAck);
      nPacketsSent++;                             // Count sent this loop
      m_nextTxSequence += sz;                     // Advance next tx sequence
    }
  NS_LOG_LOGIC ("SendPendingData sent " << nPacketsSent << " packets");
  return (nPacketsSent > 0);
}

uint32_t
TcpSocketBase::UnAckDataCount ()
{
  NS_LOG_FUNCTION (this);
  return m_nextTxSequence.Get () - FirstUnackedSeq();
}

uint32_t
TcpSocketBase::BytesInFlight ()
{
  NS_LOG_FUNCTION (this);
  return m_highTxMark.Get () - FirstUnackedSeq();
}


uint32_t
TcpSocketBase::GetRwnd() const
{
  return m_rWnd.Get();
}

uint32_t
TcpSocketBase::Window (void) 
{
  NS_LOG_FUNCTION (this);
//  return std::min (m_rWnd.Get (), m_tcb->m_cWnd.Get ());

  return std::min (
    GetRwnd(), 
    m_tcb->m_cWnd.Get ()
    
  );
}

/*
 * In Fast Recovery, RFC says that for each duplicate ACK the implementation
 * should increment cWnd by one segment size. The reasoning behind this
 * temporarily inflation of cwnd is to be able to send more segments out for
 * each incoming duplicate-ACK (which indicates that another segment made it
 * to the other side). This is necessary because TCP's sliding window is stuck
 * and will not slide until the first non-duplicate ACK comes back. As soon as
 * the first non-duplicate ACK comes back cwnd is set back to ssthresh and the
 * window continues sliding in normal congestion-avoidance mode. The implementation
 * of TCP in Linux kernel avoids this "shamanism" (they're so funny in commenting
 * their code) by improving the estimate of the in-flight packet. In RFC and old ns-3,
 * the calculus is:
 *
 *    AvailableWindow = cWnd - (SND.NXT - SND.UNA)
 *
 * example: cWnd = 10, SND.NXT = 20, SND.UNA = 10.
 * You receive 3 ACKs for 10. When receiving the third, you set cWnd to 13, and so:
 *
 *    AvailableWindow = 13 - (20 - 10) = 3
 *
 * and 3 packets could be sent. For each additional DUPACK, cWnd is
 * incremented by 1 MSS, and one packet could be sent. When a full ACK is
 * received, cWnd goes back to the right value (which is the recalculated ssth).
 * In Linux [1], the calculus is:
 *
 *    AvailableWindow = cWnd - (SND.NXT-SND.UNA) - left_out + retrans_out
 *
 * What are these new values? retrans_out is the number of packet retransmitted, and
 *
 *    left_out = sacked_out + lost_out
 *
 * where sacked_out is the number of packets arrived at the received but not
 * acked. With SACK this is easy to obtain, but with DUPACK is easy too
 * (sacket_out=m_dupAckCount). lost_out is the only guessed value: with FACK,
 * which is the most conservative heuristic, you assume that all not SACKed
 * packets until the most forward SACK are lost. Since we have not SACK, NewReno
 * estimate could be used, which basically assumes that only one segment is
 * lost (classical Reno). If we are in recovery and a partial ACK arrives,
 * it means that one more packet has been lost.
 */
uint32_t
TcpSocketBase::AvailableWindow ()
{
  NS_LOG_FUNCTION_NOARGS ();
  uint32_t unack = UnAckDataCount (); // Number of outstanding bytes
  uint32_t win = Window ();           // Number of bytes allowed to be outstanding
  uint32_t leftOut = m_dupAckCount + m_lostOut;

  NS_LOG_LOGIC ("UnAckCount=" << unack << ", Win=" << win <<
                ", leftOut=" << leftOut << ", retransOut=" << m_retransOut);

  return (win < unack) ? 0 : (win - unack - leftOut + m_retransOut);
}

uint16_t
TcpSocketBase::AdvertisedWindowSize ()
{
  uint32_t w = m_rxBuffer->MaxBufferSize () - m_rxBuffer->Size ();

  w >>= m_sndScaleFactor;

  if (w > m_maxWinSize)
    {
      NS_LOG_WARN ("There is a loss in the adv win size, wrt buffer size");
      NS_LOG_WARN ("Loss=" << w - m_maxWinSize);
      w = m_maxWinSize;
    }

  return (uint16_t) w;
}

// Receipt of new packet, put into Rx buffer
void
TcpSocketBase::ReceivedData (Ptr<Packet> p, const TcpHeader& tcpHeader)
{
  NS_LOG_FUNCTION (this << tcpHeader);
  NS_LOG_LOGIC ("seq " << tcpHeader.GetSequenceNumber () <<
                " ack " << tcpHeader.GetAckNumber () <<
                " pkt size " << p->GetSize () );

  // Put into Rx buffer
  SequenceNumber32 expectedSeq = m_rxBuffer->NextRxSequence ();
  if (!m_rxBuffer->Add (p, tcpHeader.GetSequenceNumber ()))
    { // Insert failed: No data or RX buffer full
      SendEmptyPacket (TcpHeader::ACK);
      return;
    }
  // Now send a new ACK packet acknowledging all received and delivered data
  if (m_rxBuffer->Size () > m_rxBuffer->Available () || m_rxBuffer->NextRxSequence () > expectedSeq + p->GetSize ())
    { // A gap exists in the buffer, or we filled a gap: Always ACK
      SendEmptyPacket (TcpHeader::ACK);
    }
  else
    { // In-sequence packet: ACK if delayed ack count allows
      if (++m_delAckCount >= m_delAckMaxCount)
        {
          m_delAckEvent.Cancel ();
          m_delAckCount = 0;
          SendEmptyPacket (TcpHeader::ACK);
        }
      else if (m_delAckEvent.IsExpired ())
        {
          m_delAckEvent = Simulator::Schedule (m_delAckTimeout,
                                               &TcpSocketBase::DelAckTimeout, this);
          NS_LOG_LOGIC (this << " scheduled delayed ACK at " << (Simulator::Now () + Simulator::GetDelayLeft (m_delAckEvent)).GetSeconds ());
        }
    }
  // Notify app to receive if necessary
  if (expectedSeq < m_rxBuffer->NextRxSequence ())
    { // NextRxSeq advanced, we have something to send to the app
      if (!m_shutdownRecv)
        {
          NotifyDataRecv ();
        }
      // Handle exceptions
      if (m_closeNotified)
        {
          NS_LOG_WARN ("Why TCP " << this << " got data after close notification?");
        }
      // If we received FIN before and now completed all "holes" in rx buffer,
      // invoke peer close procedure
      if (m_rxBuffer->Finished () && (tcpHeader.GetFlags () & TcpHeader::FIN) == 0)
        {
          DoPeerClose ();
        }
    }
}

/**
 * \brief Estimate the RTT
 *
 * Called by ForwardUp() to estimate RTT.
 *
 * \param tcpHeader TCP header for the incoming packet
 */
void
TcpSocketBase::EstimateRtt (const TcpHeader& tcpHeader)
{
  SequenceNumber32 ackSeq = tcpHeader.GetAckNumber();
  Time m = Time (0.0);

  // An ack has been received, calculate rtt and log this measurement
  // Note we use a linear search (O(n)) for this since for the common
  // case the ack'ed packet will be at the head of the list
  if (!m_history.empty ())
    {
      RttHistory& h = m_history.front ();
      if (!h.retx && ackSeq >= (h.seq + SequenceNumber32 (h.count)))
        { // Ok to use this sample
          if (m_timestampEnabled && tcpHeader.HasOption (TcpOption::TS))
            {
                NS_LOG_WARN("Reenable that");
//              Ptr<TcpOptionTS> ts;
//              ts = DynamicCast<TcpOptionTS> (tcpHeader.GetOption (TcpOption::TS));
//              m = TcpOptionTS::ElapsedTimeFromTsValue (ts->GetEcho ());
            }
          else
            {
              m = Simulator::Now () - h.time; // Elapsed time
            }
        }
    }

  // Now delete all ack history with seq <= ack
  while(!m_history.empty ())
    {
      RttHistory& h = m_history.front ();
      if ((h.seq + SequenceNumber32 (h.count)) > ackSeq) break;               // Done removing
      m_history.pop_front (); // Remove
    }

  if (!m.IsZero ())
    {
      m_rtt->Measurement (m);                // Log the measurement
      // RFC 6298, clause 2.4
      m_rto = Max (m_rtt->GetEstimate () + Max (m_clockGranularity, m_rtt->GetVariation ()*4), m_minRto);
      m_lastRtt = m_rtt->GetEstimate ();
      NS_LOG_FUNCTION(this << m_lastRtt);
    }
}

Ptr<const RttEstimator>
TcpSocketBase::GetRttEstimator()
{
    //!
    return m_rtt;
}

Time
TcpSocketBase::ComputeRTO() const
{
    return Max (m_rtt->GetEstimate () + Max (m_clockGranularity, m_rtt->GetVariation ()*4), m_minRto);
}

// Called by the ReceivedAck() when new ACK received and by ProcessSynRcvd()
// when the three-way handshake completed. This cancels retransmission timer
// and advances Tx window
void
TcpSocketBase::NewAck (SequenceNumber32 const& ack)
{
  NS_LOG_FUNCTION (this << ack);

  if (m_state != SYN_RCVD)
    { // Set RTO unless the ACK is received in SYN_RCVD state
      NS_LOG_LOGIC (this << " Cancelled ReTxTimeout event which was set to expire at " <<
                    (Simulator::Now () + Simulator::GetDelayLeft (m_retxEvent)).GetSeconds ());
      m_retxEvent.Cancel ();
      // On receiving a "New" ack we restart retransmission timer .. RFC 6298
      // RFC 6298, clause 2.4
      m_rto = ComputeRTO();

      NS_LOG_LOGIC (this << " Schedule ReTxTimeout at time " <<
                    Simulator::Now ().GetSeconds () << " to expire at time " <<
                    (Simulator::Now () + m_rto.Get ()).GetSeconds ());
      m_retxEvent = Simulator::Schedule (m_rto, &TcpSocketBase::ReTxTimeout, this);
    }
  if (GetRwnd () == 0 && m_persistEvent.IsExpired ())
    { // Zero window: Enter persist state to send 1 byte to probe
      NS_LOG_LOGIC (this << "Enter zerowindow persist state");
      NS_LOG_LOGIC (this << "Cancelled ReTxTimeout event which was set to expire at " <<
                    (Simulator::Now () + Simulator::GetDelayLeft (m_retxEvent)).GetSeconds ());
      m_retxEvent.Cancel ();
      NS_LOG_LOGIC ("Schedule persist timeout at time " <<
                    Simulator::Now ().GetSeconds () << " to expire at time " <<
                    (Simulator::Now () + m_persistTimeout).GetSeconds ());
      m_persistEvent = Simulator::Schedule (m_persistTimeout, &TcpSocketBase::PersistTimeout, this);
      NS_ASSERT (m_persistTimeout == Simulator::GetDelayLeft (m_persistEvent));
    }
  // Note the highest ACK and tell app to send more
  NS_LOG_LOGIC ("TCP " << this << " NewAck " << ack <<
                " numberAck " << (ack - FirstUnackedSeq())); // Number bytes ack'ed

//  m_firstTxUnack = std::min(ack, m_txBuffer->TailSequence());
  m_firstTxUnack = ack;
  UpdateTxBuffer ();

  if (GetTxAvailable () > 0)
    {
      NotifySend (GetTxAvailable ());
    }
  if (ack > m_nextTxSequence)
    {
      m_nextTxSequence = m_firstTxUnack; // If advanced
    }
  if (m_txBuffer->Size () == 0 && m_state != FIN_WAIT_1 && m_state != CLOSING)
    { // No retransmit timer if no data to retransmit
      NS_LOG_LOGIC (this << " Cancelled ReTxTimeout event which was set to expire at " <<
                    (Simulator::Now () + Simulator::GetDelayLeft (m_retxEvent)).GetSeconds ());
      m_retxEvent.Cancel ();
    }
  // Try to send more data
  if (!m_sendPendingDataEvent.IsRunning ())
    {
      m_sendPendingDataEvent = Simulator::Schedule ( TimeStep (1), &TcpSocketBase::SendPendingData, this, m_connected);
    }
}

void
TcpSocketBase::UpdateTxBuffer()
{
  NS_LOG_FUNCTION(this);
  m_txBuffer->DiscardUpTo (m_firstTxUnack);
}

// Retransmit timeout
void
TcpSocketBase::ReTxTimeout ()
{
  NS_LOG_FUNCTION (this);
  NS_LOG_LOGIC (this << " ReTxTimeout Expired at time " << Simulator::Now ().GetSeconds ());
  
  // duplicated code here with Retransmit()
  // If erroneous timeout in closed/timed-wait state, just return
  if (m_state == CLOSED || m_state == TIME_WAIT)
    {
      NS_LOG_WARN ("Retransmit while state=" <<  TcpSocket::TcpStateName[GetState()] );
      return;
    }
  // If all data are received (non-closing socket and nothing to send), just return
  // used to be FirstUnackedSeq() >= m_highTxMark
  if (m_state <= ESTABLISHED && FirstUnackedSeq() > m_highTxMark)
    {
      NS_LOG_WARN ("Retransmit while state=" <<  TcpSocket::TcpStateName[GetState()] 
        << "Check " << FirstUnackedSeq() << ">= " << m_highTxMark );
      return;
    }

  Retransmit ();
}

void
TcpSocketBase::DelAckTimeout (void)
{
  m_delAckCount = 0;
  SendEmptyPacket (TcpHeader::ACK);
}

void
TcpSocketBase::LastAckTimeout (void)
{
  NS_LOG_FUNCTION (this);

  m_lastAckEvent.Cancel ();
  if (m_state == LAST_ACK)
    {
      CloseAndNotify ();
    }
  if (!m_closeNotified)
    {
      m_closeNotified = true;
    }
}

// Send 1-byte data to probe for the window size at the receiver when
// the local knowledge tells that the receiver has zero window size
// C.f.: RFC793 p.42, RFC1112 sec.4.2.2.17
void
TcpSocketBase::PersistTimeout ()
{
  NS_LOG_LOGIC ("PersistTimeout expired at " << Simulator::Now ().GetSeconds ());
  m_persistTimeout = std::min (Seconds (60), Time (2 * m_persistTimeout)); // max persist timeout = 60s
  Ptr<Packet> p = m_txBuffer->CopyFromSequence (1, m_nextTxSequence);
  TcpHeader tcpHeader;
  GenerateEmptyPacketHeader(tcpHeader, 0);
//  AddOptions (tcpHeader);
  SendPacket(tcpHeader,p);

  NS_LOG_LOGIC ("Schedule persist timeout at time "
                << Simulator::Now ().GetSeconds () << " to expire at time "
                << (Simulator::Now () + m_persistTimeout).GetSeconds ());
  m_persistEvent = Simulator::Schedule (m_persistTimeout, &TcpSocketBase::PersistTimeout, this);
}

void
TcpSocketBase::Retransmit ()
{
  // BOTH CHECKS already done in ReTxTimeout
  // If erroneous timeout in closed/timed-wait state, just return
  if (m_state == CLOSED || m_state == TIME_WAIT) return;
  
  // If all data are received (non-closing socket and nothing to send), just return
  // TODO uncommenting this makes test fail
//  if (m_state <= ESTABLISHED && FirstUnackedSeq() >= m_highTxMark) {
//    NS_LOG_WARN("WARN");
//    return;
//  }

  /*
   * When a TCP sender detects segment loss using the retransmission timer
   * and the given segment has not yet been resent by way of the
   * retransmission timer, the value of ssthresh MUST be set to no more
   * than the value given in equation (4):
   *
   *   ssthresh = max (FlightSize / 2, 2*SMSS)            (4)
   *
   * where, as discussed above, FlightSize is the amount of outstanding
   * data in the network.
   *
   * On the other hand, when a TCP sender detects segment loss using the
   * retransmission timer and the given segment has already been
   * retransmitted by way of the retransmission timer at least once, the
   * value of ssthresh is held constant.
   */

  if (m_tcb->m_ackState != TcpSocketState::LOSS)
    {
      m_tcb->m_ssThresh = m_congestionControl->GetSsThresh (m_tcb);
      m_tcb->m_cWnd = m_tcb->m_segmentSize;
    }

  m_nextTxSequence = FirstUnackedSeq (); // Restart from highest Ack
  m_dupAckCount = 0;
  m_tcb->m_ackState = TcpSocketState::LOSS;
  NS_LOG_INFO ("RTO. Reset cwnd to " << m_tcb->m_cWnd <<
               ", ssthresh to " << m_tcb->m_ssThresh << ", restart from seqnum " << m_nextTxSequence);
  DoRetransmit ();                          // Retransmit the packet
}

void
TcpSocketBase::DoRetransmit ()
{
  NS_LOG_FUNCTION (this);
  // Retransmit SYN packet
  if (m_state == SYN_SENT)
    {
      if (m_cnCount > 0)
        {
          SendEmptyPacket (TcpHeader::SYN);
        }
      else
        {
          NotifyConnectionFailed ();
        }
      return;
    }
  // Retransmit non-data packet: Only if in FIN_WAIT_1 or CLOSING state
  if (m_txBuffer->Size () == 0)
    {
      if (m_state == FIN_WAIT_1 || m_state == CLOSING)
        { // Must have lost FIN, re-send
          SendEmptyPacket (TcpHeader::FIN);
          // TODO write a member called SendFin
//          SendFin();
        }
      return;
    }
  // Retransmit a data packet: Call SendDataPacket
  NS_LOG_LOGIC ("TcpSocketBase " << this << " retxing seq " << FirstUnackedSeq());
  uint32_t sz = SendDataPacket (FirstUnackedSeq(), GetSegSize(), true);
  // In case of RTO, advance m_nextTxSequence
  m_nextTxSequence = std::max (m_nextTxSequence.Get (), FirstUnackedSeq() + sz);

  // Increment the retransmit count.
  m_retransOut++;
}

void
TcpSocketBase::CancelAllTimers ()
{
  NS_LOG_FUNCTION_NOARGS();
  m_retxEvent.Cancel ();
  m_persistEvent.Cancel ();
  m_delAckEvent.Cancel ();
  m_lastAckEvent.Cancel ();
  m_timewaitEvent.Cancel ();
  m_sendPendingDataEvent.Cancel ();
}

/* Move TCP to Time_Wait state and schedule a transition to Closed state */
void
TcpSocketBase::TimeWait ()
{
  NS_LOG_INFO (TcpStateName[m_state] << " -> TIME_WAIT");
  m_state = TIME_WAIT;
  CancelAllTimers ();
  // Move from TIME_WAIT to CLOSED after 2*MSL. Max segment lifetime is 2 min
  // according to RFC793, p.28
  m_timewaitEvent = Simulator::Schedule (Seconds (2 * m_msl),
                                         &TcpSocketBase::CloseAndNotify, this);
}

/* Below are the attribute get/set functions */

void
TcpSocketBase::SetSndBufSize (uint32_t size)
{
  m_txBuffer->SetMaxBufferSize (size);
}

uint32_t
TcpSocketBase::GetSndBufSize (void) const
{
  return m_txBuffer->MaxBufferSize ();
}

void
TcpSocketBase::SetRcvBufSize (uint32_t size)
{
  m_rxBuffer->SetMaxBufferSize (size);
}

uint32_t
TcpSocketBase::GetRcvBufSize (void) const
{
  return m_rxBuffer->MaxBufferSize ();
}

void
TcpSocketBase::SetSegSize (uint32_t size)
{
  NS_ABORT_MSG_UNLESS ( (m_state == CLOSED) || (m_tcb->m_segmentSize == size), "Cannot change segment size dynamically.");
  m_tcb->m_segmentSize = size;

}

uint32_t
TcpSocketBase::GetSegSize (void) const
{
  return m_tcb->m_segmentSize;
}

void
TcpSocketBase::SetConnTimeout (Time timeout)
{
  m_cnTimeout = timeout;
}

Time
TcpSocketBase::GetConnTimeout (void) const
{
  return m_cnTimeout;
}

void
TcpSocketBase::SetConnCount (uint32_t count)
{
  m_cnRetries = count;
}

uint32_t
TcpSocketBase::GetConnCount (void) const
{
  return m_cnRetries;
}

void
TcpSocketBase::SetDelAckTimeout (Time timeout)
{
  m_delAckTimeout = timeout;
}

Time
TcpSocketBase::GetDelAckTimeout (void) const
{
  return m_delAckTimeout;
}

void
TcpSocketBase::SetDelAckMaxCount (uint32_t count)
{
  m_delAckMaxCount = count;
}

uint32_t
TcpSocketBase::GetDelAckMaxCount (void) const
{
  return m_delAckMaxCount;
}

void
TcpSocketBase::SetTcpNoDelay (bool noDelay)
{
  m_noDelay = noDelay;
}

bool
TcpSocketBase::GetTcpNoDelay (void) const
{
  return m_noDelay;
}

void
TcpSocketBase::SetPersistTimeout (Time timeout)
{
  m_persistTimeout = timeout;
}

Time
TcpSocketBase::GetPersistTimeout (void) const
{
  return m_persistTimeout;
}

bool
TcpSocketBase::SetAllowBroadcast (bool allowBroadcast)
{
  // Broadcast is not implemented. Return true only if allowBroadcast==false
  return (!allowBroadcast);
}

bool
TcpSocketBase::GetAllowBroadcast (void) const
{
  return false;
}

#if 0
void
TcpSocketBase::ReadOptions (const TcpHeader& header)
{
  NS_LOG_FUNCTION (this << header);

  TcpHeader::TcpOptionList options;
  header.GetOptions (options);

  for(TcpHeader::TcpOptionList::const_iterator it(options.begin()); it != options.end(); ++it)
  {
      //!
      Ptr<const TcpOption> option = *it;
      switch(option->GetKind())
      {
        case TcpOption::WINSCALE:
          if ((header.GetFlags () & TcpHeader::SYN) && m_winScalingEnabled && m_state < ESTABLISHED)
            {
              ProcessOptionWScale (option);
              ScaleSsThresh (m_sndScaleFactor);
            }
            break;
        case TcpOption::MPTCP:
            {
                ProcessOptionMpTcp(option);
            }
            break;
        case TcpOption::TS:
            if (m_timestampEnabled)
            {
                ProcessOptionTimestamp (option);
            }
            break;
        case TcpOption::NOP:
            {

            }
            break;
        default:
            NS_LOG_WARN("Unsupported option [" << (int)option->GetKind() << "]");
            break;
      };
  }

}
#endif

// TODO add
//IsTcpOptionEnabled()
//{
//
//}
bool
TcpSocketBase::IsTcpOptionAllowed (uint8_t kind) const
{
    NS_LOG_FUNCTION(this << (int)kind);

    switch(kind)
    {
    case TcpOption::TS:
        return m_timestampEnabled;
    case TcpOption::WINSCALE:
        return m_winScalingEnabled;
    case TcpOption::MPTCP:
        NS_LOG_INFO("MpTcp activé=" << m_mptcpEnabled);
        return m_mptcpEnabled;
    default:
        break;
    };
    return false;
}

//bool
//TcpSocketBase::IsTcpOptionEnabled(uint8_t kind) const
//{
//    switch(kind)
//    {
//    case TcpOption::TS:
//        return m_timestampEnabled;
//    case TcpOption::WINSCALE:
//        return m_winScalingEnabled;
//    case TcpOption::MPTCP:
//        return m_mptcpEnabled;
//    default:
//        return false;
//    };
//}

uint64_t
TcpSocketBase::GenerateUniqueMpTcpKey () const
{
  NS_LOG_FUNCTION ("Generating key");
  NS_ASSERT(m_tcp);
  // TODO rather use NS3 random generator
//  NS_ASSERT_MSG( m_mptcpLocalKey != 0, "Key already generated");

  uint64_t localKey, idsn;
  uint32_t localToken;

  do
  {
    //! arbitrary function, TODO replace with ns3 random gneerator
    localKey = (rand() % 1000 + 1);
    GenerateTokenForKey ( HMAC_SHA1, localKey, &localToken, &idsn );
  }
  while (m_tcp->LookupMpTcpToken (localToken));

  return localKey;
}


void
TcpSocketBase::AddMpTcpOptions (TcpHeader& header)
{
    NS_LOG_FUNCTION(this);
    // If key not genereated yet
//    if (m_mptcpLocalKey == 0)
//    {
//      // for the sake of simplicity, we generate a key even if unused
//      GenerateUniqueMpTcpKey();
//      NS_LOG_DEBUG("Key/token set to " << m_mptcpLocalKey << "/" << m_mptcpLocalToken);
//    }

    if((header.GetFlags () == TcpHeader::SYN))
    {
        // Append the MPTCP capable option
        Ptr<TcpOptionMpTcpCapable> mpc = CreateObject<TcpOptionMpTcpCapable>();
        mpc->SetSenderKey (m_mptcpLocalKey);
        bool res = header.AppendOption (mpc);
        NS_ASSERT (res);
    }
}

void
TcpSocketBase::AddOptions (TcpHeader& header)
{
  NS_LOG_FUNCTION (this << header);

  //GetState()
  if(IsTcpOptionAllowed (TcpOption::MPTCP))
  {
    NS_LOG_DEBUG("MPTCP enabled");
    AddMpTcpOptions(header);
  }

  // The window scaling option is set only on SYN packets
  if (IsTcpOptionAllowed (TcpOption::WINSCALE) && (header.GetFlags () & TcpHeader::SYN))
    {
      AddOptionWScale (header);
    }

  if (m_timestampEnabled)
    {
      AddOptionTimestamp (header);
    }
}

int
TcpSocketBase::ProcessOptionMpTcp ( const Ptr<const TcpOption> option)
{
    //!
//    NS_LOG_DEBUG("Does nothing");
  Ptr<const TcpOptionMpTcpCapable> mpc = DynamicCast<const TcpOptionMpTcpCapable>(option);

//  if(!GetTcpOption(header, mpc))
  if (!mpc)
  {
      NS_LOG_WARN("Invalid option " << option);
      return 0;
  }
  // else save peerKey ?
  return 1;
}

void
TcpSocketBase::ProcessOptionWScale (const Ptr<const TcpOption> option)
{
  NS_LOG_FUNCTION (this << option);

  Ptr<const TcpOptionWinScale> ws = DynamicCast<const TcpOptionWinScale> (option);

//   // TODO inverser pr que cela retourne avant de finir
//  if ((header.GetFlags () & TcpHeader::SYN))
//    {
//      if (m_winScalingEnabled)
//        {
//          m_winScalingEnabled = false;
//
//          if (header.HasOption (TcpOption::WINSCALE))
//            {
//              m_winScalingEnabled = true;

//            }
//        }
//    }

  // In naming, we do the contrary of RFC 1323. The received scaling factor
  // is Rcv.Wind.Scale (and not Snd.Wind.Scale)
  m_rcvScaleFactor = ws->GetScale ();

  if (m_rcvScaleFactor > 14)
    {
      NS_LOG_WARN ("Possible error; m_rcvScaleFactor exceeds 14: " << m_rcvScaleFactor);
      m_rcvScaleFactor = 14;
    }

  NS_LOG_INFO (m_node->GetId () << " Received a scale factor of " <<
                 static_cast<int> (m_rcvScaleFactor));
}

// TODO this should be a static function with params maxBufferSize
uint8_t
TcpSocketBase::CalculateWScale () const
{
  NS_LOG_FUNCTION (this);
  uint32_t maxSpace = m_rxBuffer->MaxBufferSize ();
  uint8_t scale = 0;

  while (maxSpace > m_maxWinSize)
    {
      maxSpace = maxSpace >> 1;
      ++scale;
    }

  if (scale > 14)
    {
      NS_LOG_WARN ("Possible error; scale exceeds 14: " << scale);
      scale = 14;
    }

  NS_LOG_INFO ("Node " << m_node->GetId () << " calculated wscale factor of " <<
               static_cast<int> (scale) << " for buffer size " << m_rxBuffer->MaxBufferSize ());
  return scale;
}

void
TcpSocketBase::AddOptionWScale (TcpHeader &header)
{
  NS_LOG_FUNCTION (this << header);
  NS_ASSERT(header.GetFlags () & TcpHeader::SYN);

  Ptr<TcpOptionWinScale> option = CreateObject<TcpOptionWinScale> ();

  // In naming, we do the contrary of RFC 1323. The sended scaling factor
  // is Snd.Wind.Scale (and not Rcv.Wind.Scale)

  m_sndScaleFactor = CalculateWScale ();
  option->SetScale (m_sndScaleFactor);

  bool res = header.AppendOption (option);
  NS_ASSERT (res);

  NS_LOG_INFO (m_node->GetId () << " Send a scaling factor of " <<
                 static_cast<int> (m_sndScaleFactor));
}

void
TcpSocketBase::ProcessOptionTimestamp (const Ptr<const TcpOption> option)
{
  NS_LOG_FUNCTION (this << option);

  Ptr<const TcpOptionTS> ts = DynamicCast<const TcpOptionTS> (option);
  m_timestampToEcho = ts->GetTimestamp ();

  // TODO move (part of) EstimateRtt here
  NS_LOG_INFO (m_node->GetId () << " Got timestamp=" <<
               m_timestampToEcho << " and Echo="     << ts->GetEcho ());
}


TcpSocket::TcpStates_t
TcpSocketBase::GetState() const
{
    return m_state;
}

void
TcpSocketBase::AddOptionTimestamp (TcpHeader& header)
{
  NS_LOG_FUNCTION (this << header);

  Ptr<TcpOptionTS> option = CreateObject<TcpOptionTS> ();

  option->SetTimestamp (TcpOptionTS::NowToTsValue ());
  option->SetEcho (m_timestampToEcho);

  bool res = header.AppendOption (option);
  NS_ASSERT (res);
  NS_LOG_INFO (m_node->GetId () << " Add option TS, ts=" <<
               option->GetTimestamp () << " echo=" << m_timestampToEcho);
}


bool
TcpSocketBase::UpdateWindowSize (const TcpHeader &tcpHeader)
{
  NS_LOG_FUNCTION (this << tcpHeader);
  return UpdateWindowSize (
    tcpHeader.GetWindowSize (),
    tcpHeader.GetSequenceNumber (),
    tcpHeader.GetAckNumber ()
  );
}

/**
 * TODO should split those in various functions, really not practical, what if no 
 */
bool
TcpSocketBase::UpdateWindowSize (
  uint32_t receivedUnscaledWindow,
  SequenceNumber32 sequenceNumber,
  SequenceNumber32 ackNumber
)
{
  NS_LOG_FUNCTION (this << receivedUnscaledWindow << sequenceNumber << ackNumber);
  //  If the connection is not established, the window size is always
  //  updated
  uint32_t receivedWindow = receivedUnscaledWindow;
  receivedWindow <<= m_rcvScaleFactor;
  NS_LOG_DEBUG ("Received (scaled) window is " << receivedWindow << " bytes");
  if (m_state < ESTABLISHED)
    {
      m_rWnd = receivedWindow;
      NS_LOG_DEBUG ("State less than ESTABLISHED; updating rWnd to " << m_rWnd);
      return true;
    }

  // Test for conditions that allow updating of the window
  // 1) segment contains new data (advancing the right edge of the receive
  // buffer),
  // 2) segment does not contain new data but the segment acks new data
  // (highest sequence number acked advances), or
  // 3) the advertised window is larger than the current send window
  bool update = false;
  if (ackNumber == m_highRxAckMark && receivedWindow > m_rWnd)
    {
      // right edge of the send window is increased (window update)
      update = true;
    }
  if (ackNumber > m_highRxAckMark)
    {
      m_highRxAckMark = ackNumber;
      update = true;
    }
  
  // TODO check it's within bounds or is it done elsewhere ? where ?
  if (sequenceNumber > m_highRxMark)
    {
      m_highRxMark = sequenceNumber;
      update = true;
    }
  if (update == true)
    {
      m_rWnd = receivedWindow;
      NS_LOG_DEBUG ("updating rWnd to " << m_rWnd);
    }
  return update;
}

void
TcpSocketBase::SetMinRto (Time minRto)
{
  NS_LOG_FUNCTION (this << minRto);
  m_minRto = minRto;
}

Time
TcpSocketBase::GetMinRto (void) const
{
  return m_minRto;
}

void
TcpSocketBase::SetClockGranularity (Time clockGranularity)
{
  NS_LOG_FUNCTION (this << clockGranularity);
  m_clockGranularity = clockGranularity;
}

Time
TcpSocketBase::GetClockGranularity (void) const
{
  return m_clockGranularity;
}

Ptr<TcpTxBuffer>
TcpSocketBase::GetTxBuffer (void) const
{
  return m_txBuffer;
}

Ptr<TcpRxBuffer>
TcpSocketBase::GetRxBuffer (void) const
{
  return m_rxBuffer;
}

void
TcpSocketBase::UpdateCwnd (uint32_t oldValue, uint32_t newValue)
{
  m_cWndTrace (oldValue, newValue);
}

void
TcpSocketBase::Dump (std::ostream &os) const
{

  //! TODO assuming it's ipv4
  if (m_endPoint) {
    os << m_endPoint->GetLocalAddress () << ":" << m_endPoint->GetLocalPort () << std::endl;
    os << m_endPoint->GetPeerAddress () << ":" << m_endPoint->GetPeerPort () << std::endl;
  }
  os << " TCB:" << std::endl;
  DumpSocketState (os, m_tcb);
  os << std::endl;
  os << "SND.NXT=" << m_nextTxSequence
     << " SND.UNA=" << FirstUnackedSeq()
     << " SND.HIGH=" << m_highTxMark
     << std::endl;
}

void
TcpSocketBase::UpdateSsThresh (uint32_t oldValue, uint32_t newValue)
{
  m_ssThTrace (oldValue, newValue);
}

void
TcpSocketBase::UpdateAckState (TcpSocketState::TcpAckState_t oldValue,
                               TcpSocketState::TcpAckState_t newValue)
{
  m_ackStateTrace (oldValue, newValue);
}

void
TcpSocketBase::SetCongestionControlAlgorithm (Ptr<TcpCongestionOps> algo)
{
  NS_LOG_FUNCTION (this << algo << algo->GetInstanceTypeId());
  m_congestionControl = algo;
}

Ptr<TcpSocketBase>
TcpSocketBase::Fork (void)
{
//  return CopyObject<TcpSocketBase> (this);
  char *addr = new char[sizeof(std::aligned_storage<sizeof(MpTcpSocketBase)>::type)];
  Ptr<TcpSocketBase> p = Ptr<TcpSocketBase> (new (addr) TcpSocketBase(*this), false);
  return p;
}

//RttHistory methods
RttHistory::RttHistory (SequenceNumber32 s, uint32_t c, Time t)
  : seq (s), count (c), time (t), retx (false)
{
}

RttHistory::RttHistory (const RttHistory& h)
  : seq (h.seq), count (h.count), time (h.time), retx (h.retx)
{
}

} // namespace ns3
