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
 */


#include <iostream>
#include <cmath>
#include "ns3/mptcp-mapping.h"
#include "ns3/simulator.h"
#include "ns3/log.h"
#include "ns3/abort.h"
#include "ns3/mptcp-subflow-owd.h"
#include "ns3/mptcp-socket-base.h"
#include "ns3/tcp-l4-protocol.h"
#include "ns3/ipv4-address.h"
#include "ns3/ipv4-end-point.h"
#include "ipv6-end-point.h" // it is not exported in ns3.19
#include "ns3/node.h"
#include "ns3/ptr.h"
#include "ns3/tcp-option-mptcp.h"
#include "ns3/mptcp-id-manager.h"
//#include "ns3/ipv4-address.h"
#include "ns3/trace-helper.h"
#include <algorithm>


namespace ns3 {

NS_LOG_COMPONENT_DEFINE("MpTcpSubflowOwd");

NS_OBJECT_ENSURE_REGISTERED(MpTcpSubflowOwd);



TypeId
MpTcpSubflowOwd::GetTypeId(void)
{
  static TypeId tid = TypeId("ns3::MpTcpSubflowOwd")
      .SetParent<MpTcpSubflow>()
      .SetGroupName ("Internet")
      .AddConstructor<MpTcpSubflowOwd>()
    ;
  return tid;
}



TypeId
MpTcpSubflowOwd::GetInstanceTypeId (void) const
{
  return MpTcpSubflowOwd::GetTypeId();
}

MpTcpSubflowOwd::MpTcpSubflowOwd () :
    MpTcpSubflow ()
{
    NS_LOG_FUNCTION_NOARGS();
}

MpTcpSubflowOwd::~MpTcpSubflowOwd ()
{
    NS_LOG_FUNCTION_NOARGS();
}

int
MpTcpSubflowOwd::ProcessOptionMpTcp (const Ptr<const TcpOption> option)
{

  return MpTcpSubflow::ProcessOptionMpTcp (option);
}

//
//
//
//void
//MpTcpSubflowOwd::DumpInfo () const
//{
//      NS_LOG_LOGIC ("MpTcpSubflow " << this << " SendPendingData" <<
////          " w " << w <<
//          " rxwin " << m_rWnd <<
////          " segsize " << GetSegSize() <<
//          " nextTxSeq " << m_nextTxSequence <<
//          " highestRxAck " << FirstUnackedSeq() <<
////          " pd->Size " << m_txBuffer->Size () <<
//          " pd->SFS " << m_txBuffer->SizeFromSequence (m_nextTxSequence)
//          );
//}


} // end of ns3
