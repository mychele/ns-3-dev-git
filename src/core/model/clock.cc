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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See theŔ
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Authors: Matthieu.coudron <matthieu.coudron@lip6.fr>
 */
#include "clock.h"
#include "ns3/log.h"
#include "ns3/nstime.h"


namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("Clock");


TypeId
Clock::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::Clock")
    .SetParent<Object> ()
  ;
  return tid;
}


Clock::Clock ()
{
}


Clock::~Clock ()
{
}


TypeId
Clock::GetInstanceTypeId (void) const
{
	return GetTypeId();
}


void
Clock::SetSimulatorSyncCallback (ClockUpdateCallback cb)
{
    NS_LOG_FUNCTION (this << &cb);
    m_rescheduleEventsCb = cb;
}

void
Clock::NotifyClockUpdate ()
{
  NS_LOG_FUNCTION (this);
  if (!m_rescheduleEventsCb.IsNull ())
  {
    m_rescheduleEventsCb ();
  }
}

void
Clock::DoDispose ()
{
    m_rescheduleEventsCb = MakeNullCallback<void> ();
}

}
