/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2006 Georgia Tech Research Corporation, INRIA
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
#include "ns3/clock-perfect.h"
#include "ns3/simulator.h"
#include "ns3/log.h"
#include "ns3/random-variable-stream.h"
//#include "ns3/random-variable-uniform.h"
#include "ns3/double.h"
#include "ns3/integer.h"
#include "ns3/uinteger.h"
#include "ns3/double.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("ClockPerfect");

// TODO rename into ClockImpl
TypeId
ClockPerfect::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::ClockPerfect")
    .SetParent<Clock> ()
    .AddConstructor<ClockPerfect> ()
//    .AddTraceSource ("Slew",
//                     "Virtual component of the frequency.",
//                     MakeTraceSourceAccessor (&ClockPerfect::m_slew),
//                     "ns3::Packet::TracedCallback")

	// TODO define MinFreq/MaxFreq ?


//0.5ms per second
    .AddAttribute ("MaxSlewRate",
								"Max slew rate",
										UintegerValue(500),
                   MakeUintegerAccessor (&ClockPerfect::m_maxSlewRate),
                   MakeUintegerChecker<uint16_t> ())
    .AddAttribute ("MaxFrequency",
									 "Max frequency",
										DoubleValue(500),
                   MakeDoubleAccessor (&ClockPerfect::m_maxSlewRate),
                   MakeDoubleChecker())
		.AddAttribute ("MinFrequency",
									 "Min frequency",
										DoubleValue(500),
                   MakeDoubleAccessor (&ClockPerfect::m_maxSlewRate),
                   MakeDoubleChecker())

//    .AddAttribute("FrequencyGenerator")
//                   CallbackValue (),
//                   MakeCallbackAccessor (&ClockPerfect::m_icmpCallback),
//                   MakeCallbackChecker ())
//    .AddAttribute ("IcmpCallback6", "Callback invoked whenever an icmpv6 error is received on this socket.",
//                   CallbackValue (),
//                   MakeCallbackAccessor (&UdpSocketImpl::m_icmpCallback6),
//                   MakeCallbackChecker ())
  ;
  return tid;
}


ClockPerfect::ClockPerfect () :
    m_maxRandomOffset(0)
{
	NS_LOG_FUNCTION(this);
    m_gen = CreateObject<UniformRandomVariable> ();
    m_gen->SetAttribute ("Min", DoubleValue(0));
//    m_gen->SetAttributeFailSafe ("Max", DoubleValue(0));
}

ClockPerfect::~ClockPerfect ()
{
	NS_LOG_FUNCTION(this);
}

bool
Clock::SetFrequency(double freq) {

    // TODO update after checking
	this->freq = freq + 1.0;
	if (!(this->freq > m_minFrequency && this->freq < m_maxFrequency)) {
		NS_LOG_ERROR("frequency outside allowed range ")
//               , this->freq - 1.0, MIN_FREQ - 1.0, MAX_FREQ - 1.0);
//		exit(1);
        return false;
	}
	return true;
}

Time
ClockPerfect::GetTime()
{
    NS_LOG_FUNCTION(this);

    // TODO display both separately
    Time res = Simulator::Now();
    NS_LOG_DEBUG("PerfectTime=" << res);
//    res += Time(m_gen->GetValue());
//    NS_LOG_UNCOND("Time after random (" << m_gen->GetMin() << ", " << m_gen->GetMax() << " offset =" << res);
    return res;
}

int
ClockPerfect::SetTime(Time)
{
	//! Noop, do nothing
	NS_LOG_WARN("SetTime has no meaning for the perfect clock");
	return -1;
}


// eglibc-2.19/sysdeps/unix/sysv/linux/adjtime.c
int
ClockPerfect::AdjTime(Time delta, Time *olddelta)
{
	NS_LOG_INFO("Adjtime called");

	if (olddelta) {
		olddelta->tv_sec = ss_offset / 1000000;
		olddelta->tv_usec = ss_offset % 1000000;
	}
//2145 and -2145 according to man adjtime (glibc parameters)

	if (delta) {
		ss_offset = delta->tv_sec * 1000000 + delta->tv_usec;
    }

	return -5; //TIME_BAD
}

//void
//ClockPerfect::SetMaxRandomOffset(double max)
//{
//    NS_LOG_FUNCTION(this << max);
//    m_gen->SetAttributeFailSafe ("Max", DoubleValue (max));
//}


}
