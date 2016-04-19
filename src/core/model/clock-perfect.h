/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
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
 * Authors: Matthieu.coudron <matthieu.coudron@lip6.fr>
 */
#ifndef CLOCK_PERFECT_H
#define CLOCK_PERFECT_H


#include "ns3/clock.h"
#include "ns3/nstime.h"
#include "ns3/event-id.h"
#include "ns3/traced-value.h"

namespace ns3 {

class Time;


/**
* \see ClockMonoticIncreasing
* No drift
 TODO maybe rename into ClockImpl
 * TODO keep clock perfect simple and move threshold completion into ClockAdjtimex ?
 */
class ClockPerfect : public Clock
{

public:

    static TypeId GetTypeId (void);

    ClockPerfect ();
    virtual ~ClockPerfect ();

    virtual TypeId GetInstanceTypeId () const;

    //! inherited
    virtual Time GetTime () ;
    virtual void SetTime (Time);
    bool LocalTimeToSimulatorTime (Time localTime, Time *absTime);
    bool SimulatorTimeToLocalTime (Time absTime, Time *localTime);

    
    // Maybe the simulator should define a rawFrequency too
    virtual double GetRawFrequency () const;

    /**
    Adds ss_slew
    if frequency can change, make it mutable
    \warn
     */
    virtual double GetTotalFrequency () const;

    /**
     * Ideally we would pass a frequency generator that adds some noise
     * but for now we stick to simple
     */
    bool SetRawFrequency (double freq);


    /**
     * Conversion is composed of 2 phases; one with SS frequency,
     * one whithout
     */
    // Rename into SimulatorToLocalDuration ?
    bool LocalToAbsDuration (Time localDuration, Time* absDuration);
    Time AbsToLocalDuration (Time absDuration);
//    bool LocalToAbsDuration(Time localDuration, Time& absDuration);

    /**
     * @return Last local recorded time
     * @see GetLastTimeUpdateSim
     */
    Time GetLastTimeUpdateLocal () const;

    /**
     * @return Last simulator recorded time
     * @see GetLastTimeUpdateLocal
     */
    Time GetLastTimeUpdateSim () const;

protected:
  /** Records a pair (localTime mapped to simulator time */
  TracedValue<std::pair<Time,Time> > m_timeOfLastUpdate;

  TracedValue<double> m_rawFrequency;
};

}

#endif
