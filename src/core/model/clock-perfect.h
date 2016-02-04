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
//#include "ns3/trace-source-accessor.h"
#include "ns3/traced-value.h"

namespace ns3 {

class Time;
class RandomVariableStream;
class UniformRandomVariable;




#if 0
enum AdjMode {
ADJ_OFFSET            = 0x0001 /* time offset */
ADJ_FREQUENCY         = 0x0002 /* frequency offset */
ADJ_MAXERROR          = 0x0004 /* maximum time error */
ADJ_ESTERROR          = 0x0008 /* estimated time error */
ADJ_STATUS            = 0x0010 /* clock status */
ADJ_TIMECONST         = 0x0020 /* pll time constant */
ADJ_TICK              = 0x4000 /* tick value */
ADJ_OFFSET_SINGLESHOT = 0x8001 /* old-fashioned adjtime() */
};
#endif

/**
Kinda like timex under linux.
**/
struct ClockParameters {

double raw_frequency;
};


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


    virtual Time GetTime () ;
    virtual int SetTime (Time);

    virtual int AdjTime (Time delta, Time *olddelta);

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

//    GetMaxFrequency()
//    SetMaxFrequency()
//    SetFrequencyGenerator

    /**
     * Maximum offset we can add to absolute time
     *
     * Ideally we should be able to pass a distribution/random generator
     */
//    virtual void SetMaxRandomOffset(double);
    virtual void ResetSingleShotParameters ();

//    virtual void SetPrecision(Time);
//    virtual Time GetPrecision(Time);

    // Will call Node
//    int RefreshEvents();

    /**
    \param t time at which ss offset compensation should finish
    \return true if singleshot offset compensation is running
    */
//    bool
//    AbsTimeLimitOfSSOffsetCompensation(Time& t);

    
    virtual int InjectOffset (Time delta) ;
    
    /**
     LocalTime must be >

     May be merged with the next one
    */
    bool
    LocalTimeToSimulatorTime (Time localTime, Time *absTime);

    /**
     * \param duration convert duration from abs duration to local duration
     * \param oldParameters use old frequency etc...
     *  \return original offset
    */
//    Time AbsToLocal(Time , bool oldParameters = false);
    bool
    SimulatorTimeToLocalTime (Time absTime, Time *localTime);

    /**
     * Conversion is composed of 2 phases; one with SS frequency,
     * one whithout
     */
    // Rename into SimulatorToLocalDuration ?
    bool LocalToAbsDuration (Time localDuration, Time* absDuration);
    Time AbsToLocalDuration (Time absDuration);
//    bool LocalToAbsDuration(Time localDuration, Time& absDuration);
    Time GetLastTimeUpdateLocal () const;
    Time GetLastTimeUpdateSim () const;

protected:
    // TODO move that to clock.h
    /**
     * local/abs
     */
    TracedValue<std::pair<Time,Time> > m_timeOfLastUpdate;

private:


    double m_ss_slew;  //!< SingleShot slew
    Time m_ss_offset;  //!< SingleShot offset

//    ClockParameters parameters[2];  //!< old and new parameters (use std::swap)
//	void UpdateTime ();

	// TODO swhould be a traced Value so that node can Trace
	// node should be made friend then
	TracedValue<double> m_rawFrequency;  //!< m_node should connect to it
//	double m_maxFrequency;
//	double m_minFrequency;

	int m_maxSlewRate;	//!< cap frequency change

	EventId m_ssOffsetCompletion;   //!< Timer that says when to reset ss_slew and ss_offset


//    double m_maxRandomOffset;
//    Ptr<RandomVariableStream> m_gen;
//    Ptr<UniformRandomVariable> m_gen;
};

}

#endif
