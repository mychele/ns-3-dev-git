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

#include "ns3/log.h"
#include "ns3/mptcp-cc-lia.h"
#include "ns3/mptcp-subflow.h"
#include "ns3/mptcp-socket-base.h"

NS_LOG_COMPONENT_DEFINE ("MpTcpCongestionLia");

namespace ns3
{

NS_OBJECT_ENSURE_REGISTERED (MpTcpCongestionLia);

TypeId
MpTcpCongestionLia::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::MpTcpCongestionLia")
//    .SetParent<TcpCongestionOps> ()
    .SetParent<TcpNewReno> ()
    .SetGroupName ("Internet")
    .AddConstructor<MpTcpCongestionLia> ()
//    .AddTraceSource ("Alpha",
//                     "Value of the alp",
//                     MakeTraceSourceAccessor (&MpTcpCongestionLia::m_alpha),
//                     "ns3::WTH_IS_ZIS_TracedValueCallback"
//                     )
      ;
  ;
  return tid;
}

TypeId 
MpTcpCongestionLia::GetInstanceTypeId (void)
{
  return GetTypeId ();
}


// TODO we should aggregate CC for the mptcp case ?
MpTcpCongestionLia::MpTcpCongestionLia() : 
//  TcpCongestionOps (),
  TcpNewReno(),
  m_alpha (1)
//  , m_totalCwnd (0)
{
  NS_LOG_FUNCTION_NOARGS();
}

MpTcpCongestionLia::~MpTcpCongestionLia ()
{
  NS_LOG_FUNCTION_NOARGS();
}


std::string
MpTcpCongestionLia::GetName () const
{
  return "MpTcpLia";
}


double
MpTcpCongestionLia::ComputeAlpha (Ptr<MpTcpSocketBase> metaSock, Ptr<TcpSocketState> tcb) const
{
  // this method is called whenever a congestion happen in order to regulate the agressivety of m_subflows
  // m_alpha = cwnd_total * MAX(cwnd_i / rtt_i^2) / {SUM(cwnd_i / rtt_i))^2}   //RFC 6356 formula (2)
  
  NS_LOG_FUNCTION(this);
  
  double alpha = 0;
  double maxi = 0; // Matches the MAX(cwnd_i / rtt_i^2) part
  double sumi = 0; // SUM(cwnd_i / rtt_i)

  
  NS_ASSERT (metaSock);
  // TODO here
  for (uint32_t i = 0; i < metaSock->GetNActiveSubflows(); i++)
    {
      Ptr<MpTcpSubflow> sFlow = metaSock->GetSubflow(i);

      Time time = sFlow->GetRttEstimator()->GetEstimate();
      double rtt = time.GetSeconds();
      double tmpi = tcb->m_cWnd.Get() / (rtt * rtt);
      
      if (maxi < tmpi)
        maxi = tmpi;

      sumi += tcb->m_cWnd.Get() / rtt;
    }
  alpha = (metaSock->m_tcb->m_cWnd.Get () * maxi) / (sumi * sumi);
  return alpha;
}


// TODO in this version, this function is called for each segment size
// in future versions (>3.25), one can 
void
MpTcpCongestionLia::IncreaseWindow (Ptr<TcpSocketBase> sf, Ptr<TcpSocketState> tcb)
{
  NS_LOG_FUNCTION (this);

  // Increase of cwnd based on current phase (slow start or congestion avoidance)
  if (tcb->m_cWnd < tcb->m_ssThresh)
    {
      tcb->m_cWnd += tcb->m_segmentSize;
      NS_LOG_INFO ("In SlowStart, updated tcb " << tcb << " cwnd to " << tcb->m_cWnd << " ssthresh " << tcb->m_ssThresh);
    }
  else
    { 
      
      Ptr<MpTcpSubflow> subflow = DynamicCast<MpTcpSubflow> (sf);
      Ptr<MpTcpSocketBase> metaSock = subflow->GetMeta();
      
      // rename into get ?
      uint32_t totalCwnd = metaSock->ComputeTotalCWND ();
//      metaSock->
  
      m_alpha = ComputeAlpha (metaSock, tcb);
      double alpha_scale = 1;
//         The alpha_scale parameter denotes the precision we want for computing
//   alpha
//                alpha * bytes_acked * MSS_i   bytes_acked * MSS_i
//          min ( --------------------------- , ------------------- )  (3)
//                 alpha_scale * cwnd_total              cwnd_i
    
    double adder = std::min ( 
      m_alpha* tcb->m_segmentSize * tcb->m_segmentSize / (totalCwnd* alpha_scale),
        static_cast<double>((tcb->m_segmentSize * tcb->m_segmentSize) / tcb->m_cWnd.Get ())
      );
    // Congestion avoidance mode, increase by (segSize*segSize)/cwnd. (RFC2581, sec.3.1)
      // To increase cwnd for one segSize per RTT, it should be (ackBytes*segSize)/cwnd
//      double adder = static_cast<double> (tcb->m_segmentSize * tcb->m_segmentSize) / tcb->m_cWnd.Get ();
//      adder = std::max (1.0, adder);
      tcb->m_cWnd += static_cast<uint32_t> (adder);
      NS_LOG_INFO ("In CongAvoid, updated tcb " << tcb << " cwnd to " << tcb->m_cWnd << " ssthresh " << tcb->m_ssThresh);
    }
}

Ptr<TcpCongestionOps> 
MpTcpCongestionLia::Fork ()
{
  return CreateObject<MpTcpCongestionLia>();
}


}
