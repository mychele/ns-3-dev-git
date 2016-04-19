/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2015 Université Pierre et Marie Curie
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
//#include "ns3/integer.h"
//#include "ns3/uinteger.h"
//#include "ns3/double.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("ClockPerfect");

inline Time
LocalToAbs (Time duration, double frequency)
{
    return duration*frequency;
}

inline Time
AbsToLocal (Time duration, double frequency)
{
    return duration/frequency;
}

TypeId
ClockPerfect::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::ClockPerfect")
    .SetParent<Clock> ()
    .AddConstructor<ClockPerfect> ()
    .AddTraceSource ("LastUpdate", 
                     "Tracks local time/reference time mappings",
                     MakeTraceSourceAccessor (&ClockPerfect::m_timeOfLastUpdate)
                     )
    .AddTraceSource ("RawFrequency",
                    "frequency",
                     MakeTraceSourceAccessor (&ClockPerfect::m_rawFrequency)
                     )
  ;
  return tid;
}

TypeId
ClockPerfect::GetInstanceTypeId () const
{
  return GetTypeId ();
}

ClockPerfect::ClockPerfect () :
        m_rawFrequency(1.)
{
    NS_LOG_FUNCTION(this);
    m_timeOfLastUpdate.Set (std::make_pair (Simulator::Now (), Simulator::Now ()));
}

ClockPerfect::~ClockPerfect ()
{
    NS_LOG_FUNCTION(this);
}

bool
ClockPerfect::SetRawFrequency (double freq)
{

    NS_LOG_INFO ("New frequency=" << freq << " oldFreq=" << m_rawFrequency);
    NS_ASSERT_MSG (freq > 0, "Can't set a negative frequency");
    double oldFreq = m_rawFrequency;

    Time currentTime = GetLastTimeUpdateSim () + AbsToLocalDuration(Simulator::Now () - GetLastTimeUpdateSim ());
//    Time currentTime = GetLastTimeUpdateSim() + AbsToLocalDuration( Simulator::Now() - GetLastTimeUpdateSim() );
    m_timeOfLastUpdate = std::make_pair (currentTime, Simulator::Now() );
    m_rawFrequency = freq;

    NotifyClockUpdate ();
    return true;
}

Time
ClockPerfect::GetLastTimeUpdateLocal () const
{
    return m_timeOfLastUpdate.Get ().first;
}

Time
ClockPerfect::GetLastTimeUpdateSim () const
{
    return m_timeOfLastUpdate.Get ().second;
}

Time
ClockPerfect::GetTime()
{
    NS_LOG_FUNCTION (this);

    Time result;
    result = GetLastTimeUpdateLocal ();
    result += AbsToLocalDuration (Simulator::Now() - GetLastTimeUpdateSim());
    return result;
}

double
ClockPerfect::GetRawFrequency () const
{
    return m_rawFrequency;
}

bool
ClockPerfect::SimulatorTimeToLocalTime (Time absTime, Time *localTime)
{
    NS_LOG_FUNCTION(absTime);

    NS_ASSERT_MSG(absTime >= GetLastTimeUpdateSim(), "Can't convert to a time in the past");
    Time localDuration;
    localDuration = AbsToLocalDuration ( absTime - GetLastTimeUpdateSim());
    *localTime = GetLastTimeUpdateLocal() + localDuration;
    
    return true;
}

bool
ClockPerfect::LocalTimeToSimulatorTime (Time localTime, Time *absTime)
{
    NS_LOG_FUNCTION (localTime);
    NS_ASSERT_MSG (localTime >= GetLastTimeUpdateLocal(), "Can't convert to a time in the past");

    Time absDuration;
    LocalToAbsDuration (localTime - GetLastTimeUpdateLocal(), &absDuration);

    *absTime = GetLastTimeUpdateSim () + absDuration;
    NS_LOG_DEBUG ( *absTime << " (absTime) =" << GetLastTimeUpdateSim() << " (LastTimeUpdateSim) + " << absDuration << " (absDuration)");
    return true;
}

bool
ClockPerfect::LocalToAbsDuration (Time localDuration, Time *absDuration)
{
    NS_LOG_FUNCTION (localDuration);

    *absDuration = localDuration / GetTotalFrequency ();
    return true;
}

void
ClockPerfect::SetTime (Time now)
{
    m_timeOfLastUpdate.Set (std::make_pair(now, Simulator::Now()));
}

double
ClockPerfect::GetTotalFrequency () const
{
    return m_rawFrequency;
}

Time
ClockPerfect::AbsToLocalDuration (Time absDuration)
{
    NS_LOG_FUNCTION (absDuration);
    Time localDuration = absDuration * GetTotalFrequency();
    return localDuration;
}

}
