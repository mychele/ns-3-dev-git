/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2015 Université Pierre et Marie Curie, UPMC
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
 * Authors: Matthieu.coudron <matthieu.coudron@lip6.fr>
 */
#include "clock.h"


namespace ns3 {

TypeId
Clock::GetTypeId (void)
{
 // TODO add accuracy/freq/offset ?
  static TypeId tid = TypeId ("ns3::Clock")
    .SetParent<Object> ()
//    .AddConstructor<Clock> ()
//    .AddTraceSource ("Resolution",
//                     "Drop UDP packet due to receive buffer overflow",
//                     MakeTraceSourceAccessor (&UdpSocketImpl::m_dropTrace),
//                     "ns3::Packet::TracedCallback")
//    .AddAttribute ("IcmpCallback", "Callback invoked whenever an icmp error is received on this socket.",
//                   CallbackValue (),
//                   MakeCallbackAccessor (&UdpSocketImpl::m_icmpCallback),
//                   MakeCallbackChecker ())
//    .AddAttribute ("IcmpCallback6", "Callback invoked whenever an icmpv6 error is received on this socket.",
//                   CallbackValue (),
//                   MakeCallbackAccessor (&UdpSocketImpl::m_icmpCallback6),
//                   MakeCallbackChecker ())
  ;
  return tid;
}

TypeId
Clock::GetInstanceTypeId (void) const
{
//	NS_LOG_FUNCTION (this);
	return GetTypeId();
}

}
