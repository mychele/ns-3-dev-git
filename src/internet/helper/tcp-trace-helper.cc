/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2010 University of Washington
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
 */

#include <stdint.h>
#include <string>
#include <fstream>

#include "ns3/abort.h"
#include "ns3/assert.h"
#include "ns3/log.h"
#include "ns3/ptr.h"
#include "ns3/node.h"
#include "ns3/names.h"
#include "ns3/net-device.h"
#include "ns3/pcap-file-wrapper.h"
#include "ns3/sequence-number.h"
#include "ns3/tcp-socket-base.h"
#include "ns3/mptcp-socket-base.h"
#include "ns3/mptcp-subflow.h"
//#include "ns3/mptcp-subflow.h"

#include "tcp-trace-helper.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("TcpTraceHelper");


void
dumpStr (Ptr<OutputStreamWrapper> stream, int pos, std::string item)
{
  NS_LOG_DEBUG ("Recording item " 
  << item << " at time=" 
  << (Simulator::Now())
  );
  
  *stream->GetStream() << Simulator::Now() << ",";
  
  for (int i = 1;i < pos; ++i){
    *stream->GetStream() << ",";
  }
  
  *stream->GetStream() << item;

  for (int i = pos + 1;i < 10; ++i){
    *stream->GetStream() << ",";
  }
  
  *stream->GetStream()  << std::endl;
}

void
dumpSequence32(Ptr<OutputStreamWrapper> stream, int pos, std::string context, SequenceNumber32 oldSeq, SequenceNumber32 newSeq)
{
  //<< context <<
//  if (context == "NextTxSequence")
  std::ostringstream os;
  os << newSeq;
  dumpStr ( stream, pos, os.str());
//  *stream->GetStream() << Simulator::Now()
//                       << "," << oldSeq
//                       << "," << newSeq
//                       << std::endl;
}


void
dumpUint32(Ptr<OutputStreamWrapper> stream, int pos, std::string context, uint32_t oldVal, uint32_t newVal) {

//  NS_LOG_UNCOND("Context " << context << "oldVal=" << oldVal << "newVal=" << newVal);

//  *stream->GetStream() << Simulator::Now()
//                       << "," << oldVal
//                       << "," << newVal
//                       << std::endl;
  std::ostringstream os;
  os << newVal;
  dumpStr ( stream, pos, os.str());
}


void
dumpTcpState(Ptr<OutputStreamWrapper> stream, int pos, std::string context, TcpSocket::TcpStates_t oldVal, TcpSocket::TcpStates_t newVal) {
  // TODO rely
//  *stream->GetStream() << Simulator::Now()
//                      << "," << TcpSocket::TcpStateName[oldVal]
//                      << "," << TcpSocket::TcpStateName[newVal]
//                      << std::endl;
//  std::ostringstream os;
//  os << newSeq;
  dumpStr ( stream, pos, TcpSocket::TcpStateName[newVal]);
}


void
TcpTraceHelper::OnNewSocket (Ptr<TcpSocket> socket)
{
  NS_LOG_DEBUG ("New socket incoming");
  
  TcpTraceHelper tcpHelper;
  std::stringstream os;
  //! we start at 1 because it's nicer
  // m_tracePrefix << 
  static int subflowCounter = 0;
  static int metaCounter = 0;

//      std::string filename;
//      if (explicitFilename)
//        {
//          filename = prefix;
//        }
//      else
//        {
//          filename = asciiTraceHelper.GetFilenameFromInterfacePair (prefix, ipv4, interface);
//        }

  // No reason to fail
  Ptr<TcpSocketBase> sock = DynamicCast<TcpSocketBase>(socket);

  //! choose a prefix depending on if it's subflow or meta
  // TODO improve the doc to mark that isChildOf ( ,false) is strict
  if(sock->GetInstanceTypeId().IsChildOf( MpTcpSubflow::GetTypeId()) 
    || sock->GetInstanceTypeId() == MpTcpSubflow::GetTypeId())
  {
    //! TODO prefixer avec le nom de la meta et utiliser le subflow addrId
    os << Simulator::GetContext() << "-subflow" <<  subflowCounter++;
    tcpHelper.SetupSocketTracing (sock, os.str());
  }
  else if(sock->GetInstanceTypeId().IsChildOf( MpTcpSocketBase::GetTypeId())
      || sock->GetInstanceTypeId() == MpTcpSocketBase::GetTypeId()
      )
  {
    os << Simulator::GetContext() << "-meta" <<  metaCounter++;
    tcpHelper.SetupSocketTracing (sock, os.str());
  }
  else 
  {
    
    NS_LOG_INFO ("Not mptcp, do nothing: typeid=" << sock->GetInstanceTypeId().GetName ());
  }
}

/**
TODO move that elsewhere, and plot the first line to get the initial value else it makes
for bad plots.
**/
void
TcpTraceHelper::SetupSocketTracing (Ptr<TcpSocketBase> sock, const std::string prefix)
{
  NS_LOG_INFO("sock " << sock << " and prefix [" << prefix << "]");
  std::ios::openmode mode = std::ofstream::out | std::ofstream::trunc;


  
  // Or everything could be just written in the same file (uses fewer file descriptors)
  AsciiTraceHelper asciiTraceHelper;
  Ptr<OutputStreamWrapper> fd = asciiTraceHelper.CreateFileStream (prefix+ "_data.csv", mode);

  Time now = Simulator::Now();
  // TODO use GetInitialCwnd, GetValue  etc...
  *fd->GetStream() 
    << "Time,txNext,highestSeq,unackSeq,rxNext,rxAvailable,rxTotal,cwnd,rWnd,ssThresh,state" << std::endl
    << now << "," << sock->m_nextTxSequence
          << "," << sock->m_highTxMark 
          << "," << sock->FirstUnackedSeq() 
          << "," << sock->m_rxBuffer->NextRxSequence()
          << "," << sock->GetRxAvailable ()
          << "," << sock->GetRcvBufSize()
          << "," << sock->GetInitialCwnd()
          << "," << sock->Window()
          << "," << sock->GetInitialSSThresh()
          << "," << TcpSocket::TcpStateName[sock->GetState()]
          << std::endl
   ;

//  *streamTxHighest->GetStream() << "Time,oldHighestSequence,newHighestSequence" << std::endl
//                              << now << ",0," << sock->m_highTxMark << std::endl
//                                ;
  // In fact it might be acked but as it neds to be moved on a per-mapping basis
  //
//  *streamTxUnack->GetStream() << "Time,oldUnackSequence,newUnackSequence" << std::endl
//                                  << now << ",0," << sock->FirstUnackedSeq() << std::endl
//                                  ;
//  *streamRxNext->GetStream() << "Time,oldRxNext,newRxNext" << std::endl
//                             << now << ",0," << sock->m_rxBuffer->NextRxSequence() << std::endl
//                             ;
//  *streamRxAvailable->GetStream() << "Time,oldRxAvailable,newRxAvailable" << std::endl
//                                  << now << ",," << sock->GetRxAvailable () << std::endl
//                                  ;
//  *streamRxTotal->GetStream() << "Time,oldRxTotal,newRxTotal" << std::endl
//                                  << now << ",," << sock->GetRcvBufSize() << std::endl
//                                  ;
  // TODO
//  *streamCwnd->GetStream() << "Time,oldCwnd,newCwnd" << std::endl
//                          << now << ",0," << sock->GetInitialCwnd() << std::endl;
//  *streamRwnd->GetStream() << "Time,oldRwnd,newRwnd" << std::endl
//                           << now << ",0," << sock->Window() << std::endl
//                           ;

  // We don't plot it, just looking at it so we don't care of the initial state
//  *streamStates->GetStream() << "Time,oldState,newState" << std::endl
//                           << now << ",," << TcpSocket::TcpStateName[sock->GetState()] << std::endl
//            ;

//  NS_ASSERT(f.is_open());
  // TODO je devrais etre capable de voir les CongestionWindow + tailles de buffer/ Out of order
//  CongestionWindow
//  Ptr<MpTcpSocketBase> sock(this);
//  NS_ASSERT(sock->TraceConnect ("RcvBufSize", "RcvBufSize", MakeBoundCallback(&dumpSequence32, streamTxNext)));
//  NS_ASSERT(sock->TraceConnect ("SndBufSize", "SndBufSize", MakeBoundCallback(&dumpSequence32, streamTxNext)));

  // TODO pass on the index (10 elements indexed from 0)
  bool res = sock->TraceConnect ("NextTxSequence", "NextTxSequence", MakeBoundCallback(&dumpSequence32, fd, 1));
  NS_ASSERT (res);
  
  res = sock->TraceConnect ("HighestSequence", "HighestSequence", MakeBoundCallback(&dumpSequence32, fd, 2));
  NS_ASSERT(res);

  res &= sock->TraceConnect ("UnackSequence", "UnackSequence", MakeBoundCallback(&dumpSequence32, fd,3));
  // TODO bug here
  res &= sock->m_rxBuffer->TraceConnect ("NextRxSequence", "NextRxSequence", MakeBoundCallback(&dumpSequence32, fd,4) );

  res &= sock->m_rxBuffer->TraceConnect ("RxAvailable", "RxAvailable", MakeBoundCallback(&dumpUint32, fd, 5) );
  res &= sock->m_rxBuffer->TraceConnect ("RxTotal", "RxTotal", MakeBoundCallback(&dumpUint32, fd, 6) );
  
  res &= sock->TraceConnect ("CongestionWindow", "CongestionWindow", MakeBoundCallback(&dumpUint32, fd,7));
  res &= sock->TraceConnect ("RWND", "Remote WND", MakeBoundCallback(&dumpUint32, fd, 8));

  // TODO show SlowStart Threshold
  res &= sock->TraceConnect ("SlowStartThreshold", "SlowStartThreshold", MakeBoundCallback(&dumpUint32, fd, 9));
  res &= sock->TraceConnect ("State", "State", MakeBoundCallback(&dumpTcpState, fd, 10) );
  NS_ASSERT(res);
//  Ptr<MpTcpSocketBase> sock2 = DynamicCast<MpTcpSocketBase>(sock);
//  Ptr<TcpTxBuffer> txBuffer( &sock->m_txBuffer);
  
  
  // HighestRxAck is not in sync with RxBuffer nextRxSequence, hence useless + it is not used in MPTCP stacks
//  NS_ASSERT(sock->TraceConnect ("HighestRxAck", "HighestRxAck", MakeBoundCallback(&dumpSequence32, streamRxNext) ));
//  HighestRxSequence
}




} // namespace ns3

