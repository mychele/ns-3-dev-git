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
#include "ns3/random-variable-stream.h"
//#include "ns3/random-variable-uniform.h"
#include "ns3/integer.h"
#include "ns3/uinteger.h"
#include "ns3/double.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("ClockPerfect");

// TODO move to static
inline Time
LocalToAbs(Time duration, double frequency)
{
    return duration*frequency;
}

inline Time
AbsToLocal(Time duration, double frequency)
{
    return duration/frequency;
}


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
//    .AddAttribute ("MaxSlewRate",
//                    "Max slew rate",
//                    UintegerValue(500),
//                    MakeUintegerAccessor (&ClockPerfect::m_maxSlewRate),
//                    MakeUintegerChecker<uint16_t> ())
//    .AddAttribute ("MaxFrequency",
//                    "Max frequency",
//                    DoubleValue(500),
//                    MakeDoubleAccessor (&ClockPerfect::m_maxSlewRate),
//                    MakeDoubleChecker<double>())
//    .AddAttribute ("MinFrequency",
//                    "Min frequency",
//                    DoubleValue(500),
//                    MakeDoubleAccessor (&ClockPerfect::m_maxSlewRate),
//                    MakeDoubleChecker<double>())

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


ClockPerfect::ClockPerfect ()
//:
//        m_maxRandomOffset(0)
    :
//        GetLastTimeUpdateLocal()(0),
//        m_lastUpdateAbsTime(0),

        m_ss_offset(Time(0)),
        m_ss_slew(0),
        m_rawFrequency(1.)
{
    NS_LOG_FUNCTION(this);
    m_timeOfLastUpdate.Set (std::make_pair(Simulator::Now(), Simulator::Now()) );
//	GetLastTimeUpdateLocal()= Simulator::Now();
//	m_lastUpdateAbsTime= Simulator::Now();
    NS_LOG_UNCOND("Creating node at abs time"
               << GetLastTimeUpdateSim()
               );

//    m_gen = CreateObject<UniformRandomVariable> ();
//    m_gen->SetAttribute ("Min", DoubleValue(0));
//    m_gen->SetAttributeFailSafe ("Max", DoubleValue(0));
}

ClockPerfect::~ClockPerfect ()
{
    NS_LOG_FUNCTION(this);
}


int RefreshEvents();



//Time
//ClockPerfect::AbsTimeFromOffset()
//{
//    GetTime + AbsOffsetFromOffset;
//}


bool
ClockPerfect::SetRawFrequency(double freq)
{

    // TODO update after checking
//	m_rawFrequency
//	if (!(m_rawFrequency > m_minFrequency && this->freq < m_maxFrequency)) {
//		NS_LOG_ERROR("frequency outside allowed range ")
////               , this->freq - 1.0, MIN_FREQ - 1.0, MAX_FREQ - 1.0);
////		exit(1);
//        return false;
//	}
    NS_ASSERT(freq > 0);
    NS_LOG_INFO("New frequency=" << freq);
    double oldFreq = m_rawFrequency;



    // Move to a private function UpdateTime ?
    // Can't use
    Time currentTime = GetLastTimeUpdateSim() + AbsToLocalDuration( Simulator::Now() - GetLastTimeUpdateSim() );
    m_timeOfLastUpdate = std::make_pair( currentTime, Simulator::Now() );
    m_rawFrequency = freq;

    NotifyNewFrequency(oldFreq, m_rawFrequency);
    return true;
}

Time
ClockPerfect::GetLastTimeUpdateLocal() const
{
    //!
    return m_timeOfLastUpdate.Get().first;
}

Time
ClockPerfect::GetLastTimeUpdateSim() const
{
    //!
    return m_timeOfLastUpdate.Get().second;
}

void
ClockPerfect::UpdateTime()
{
    // For now support only adjtime
}

Time
ClockPerfect::GetTime()
{
    NS_LOG_FUNCTION(this);

    // TODO display both separately
//    Time res = Simulator::Now();
    Time res = GetLastTimeUpdateLocal() + AbsToLocalDuration( Simulator::Now() - GetLastTimeUpdateSim() );
    NS_LOG_DEBUG ("PerfectTime=" << res);
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

double
ClockPerfect::GetRawFrequency() const
{
    return m_rawFrequency;
}


void
ClockPerfect::ResetSingleShotParameters()
{
    //!
    m_ss_slew = 0;
    m_ss_offset = Time(0);
    m_ssOffsetCompletion.Cancel();

//    double oldFrequency =
    NotifyNewFrequency(GetTotalFrequency(), GetRawFrequency());
}


//
//bool
//ClockPerfect::AbsTimeLimitOfSSOffsetCompensation (Time& t)
//{
//    //!
//    if(m_ssOffsetCompletion.IsExpired()){
//        return false;
//    }
//
//    t = Time(m_ssOffsetCompletion.GetTs());
//    return true;
//}


//Time
//ClockPerfect::LocalDurationToAbsTime(Time duration)
//{
//    LocalDurationToAbsDuration
//}

// TODO return Time instead
bool
ClockPerfect::AbsTimeToLocalTime(Time absTime, Time& localTime)
{
    //!
    NS_LOG_FUNCTION(absTime);

    NS_ASSERT_MSG(absTime >= GetLastTimeUpdateSim(), "Can't convert to a time in the past");

    localTime = AbsToLocalDuration( absTime - GetLastTimeUpdateSim());
    localTime += GetLastTimeUpdateLocal() ;
    return true;
}



// OK
bool
ClockPerfect::LocalTimeToAbsTime(Time localTime, Time &absTime)
{
    NS_LOG_FUNCTION(localTime);
    NS_ASSERT_MSG(localTime >= GetLastTimeUpdateLocal(), "Can't convert to a time in the past");
//    if(localTime < GetLastTimeUpdateLocal()) {
//        NS_LOG_WARN("Requested time in the past " << localTime << " < " << GetLastTimeUpdateLocal());
//        return false;
//    }

//    NS_ASSERT()

    // Returns
    LocalToAbsDuration(localTime - GetLastTimeUpdateLocal(), absTime);
    absTime += GetLastTimeUpdateSim();
    return true;
}

// Ok
bool
//ClockPerfect::LocalDurationToAbsDuration(Time duration, Time& absDuration)
ClockPerfect::LocalToAbsDuration (Time localDuration, Time& absDuration)
{
    NS_LOG_FUNCTION(localDuration);
//
//    NS_ASSERT_MSG(localStart > GetLastTimeUpdateLocal(), "We can't remember the frequency back then");

    absDuration = localDuration * GetTotalFrequency();
    return true;
}

int 
ClockPerfect::InjectOffset (Time delta) 
{
    //
    m_timeOfLastUpdate.Set (std::make_pair( GetTime () + delta, Simulator::Now()) );
}

double
ClockPerfect::GetTotalFrequency() const
{
    //!
    return m_ss_slew + m_rawFrequency;
}

//, Time& localDuration
Time
ClockPerfect::AbsToLocalDuration(Time absDuration)
{
    NS_LOG_FUNCTION(absDuration);
    Time localDuration = absDuration / GetTotalFrequency();
    NS_LOG_DEBUG( absDuration << "/" << GetTotalFrequency() << " =" << localDuration << " (localDuration)");
    return localDuration;
}

#if 0
// intervertir bool et Time dans le proto
// Ca depend de quand ca commence
bool
//ClockPerfect::LocalDurationToAbsDuration(Time duration, Time& absDuration)
ClockPerfect::LocalToAbsDuration(Time localStart, Time localDuration, Time& absDuration)
{
    /**
     We need to distinguish bounds for phases between several phases depending :
     1/ in the first phase we may have a singleshot additionnal frequency
     2/ in the 2nd phase we haven't
    */
    Time ssTimeLimit(0);
    Time absDuration(0);
    Time localDurationWithSSCorrection (0);
    if(AbsTimeLimitOfSSOffsetCompensation(ssTimeLimit))
    {
        // local frequency * AbsDurationOf the
        localDurationWithSSCorrection = (ssTimeLimit - Simulator::Now()) / (GetRawFrequency() + m_ss_slew);

        // if the first phase englobes "duration"
        if(localDurationWithSSCorrection > duration) {
            // regle de 3
            absDuration = (ssTimeLimit - Simulator::Now()) * (duration/localDurationWithSSCorrection);
            return absDuration;
        }
        absDuration = ssTimeLimit - Simulator::Now();
    }

    // We are now in the 2nd phase, ie there is no SS frequency component anymore
    absDuration += (GetRawFrequency()) * (duration-localDurationWithSSCorrection);

    return absDuration;
}
#endif

// eglibc-2.19/sysdeps/unix/sysv/linux/adjtime.c
/*
TODO here we should schedule an element that resets ss_offset and ss_slew.
*/
int
ClockPerfect::AdjTime (Time delta, Time *olddelta)
{
	NS_LOG_INFO("Adjtime called");
//    frequency
//	if (olddelta) {
//		olddelta->tv_sec = ss_offset / 1000000;
//		olddelta->tv_usec = ss_offset % 1000000;
//	}
//2145 and -2145 according to man adjtime (glibc parameters)
    Time offsetCancellation = delta/m_maxSlewRate;
    NS_LOG_DEBUG("Delta=" << delta << " should be solved in " << MilliSeconds(offsetCancellation)
                 << "ms with a maxSlew of " << m_maxSlewRate
                 );


    m_ssOffsetCompletion = Simulator::Schedule(offsetCancellation, &ClockPerfect::ResetSingleShotParameters, this);

//    ss_offset/GetTotalFrequency()
//	if (delta) {
//		ss_offset = delta->tv_sec * 1000000 + delta->tv_usec;
//    }

	return -5; //TIME_BAD
}

//void
//ClockPerfect::SetMaxRandomOffset(double max)
//{
//    NS_LOG_FUNCTION(this << max);
//    m_gen->SetAttributeFailSafe ("Max", DoubleValue (max));
//}


}
