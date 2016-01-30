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
#ifndef CLOCK_H
#define CLOCK_H

#include <vector>

#include "ns3/object.h"
#include "ns3/callback.h"
#include "ns3/ptr.h"


namespace ns3 {

class Time;

/**
 * TODO rename into VirtualClock ?
 * Monotonic ?
 * trigger error when it wraps
 *
 * This is the base class that decides how Node time evolves.
 * The clock describes a model to convert local-to-simulator time and vice-versa.
 *
 * It is oblivious to how subclassed clock model elapsed time.
 *
 * In order to the respect the discrete nature, these subclasses 
 * need to respect a few constraints though:
 * - they must notify the node whenever one of its parameters get updated. This allows
 *   node to remove scheduled events and to reschedule these very events with a corrected time.
 * - clocks must also be able to compute the local duration between a future time and the last 
 *   clock parameter updates back to simulator time
 *
 * 
 * Many network protocols rely on accurate (with varying precisions)
 * timing for their good working (TDMA protocols etc...).
 *
 *
 * This also extends the range of supported applications by DCE, which can potentially
 * now be used as a testing platform for ntpd, chrony or other ntp servers.
 * In that regards it is similar to clknetsim
 * \see clknetsim (https://github.com/mlichvar/clknetsim)
 *
 * For an introduction, you should have a look at ClockPerfect.
 */
class Clock : public Object
{
public:
    typedef Callback<void> FrequencyCallback;
//    typedef Callback<void, double, double> FrequencyCallback;
//    typedef Callback<void, double, double> TimeStepCallback;

    static TypeId GetTypeId (void);
    virtual TypeId GetInstanceTypeId (void) const;
//enum Result {
//	 TIME_INS = 1 /* insert leap second */
//	 TIME_DEL = 2 /* delete leap second */
//	 TIME_OOP  3 /* leap second in progress */
//	 TIME_WAIT 4 /* leap second has occurred */
//	 TIME_BAD  5 /* clock not synchronized */
//}
    Clock ();
    virtual ~Clock () {};

    /**
     * \return Local time
     */
    virtual Time GetTime () = 0;

    /**
     * reestablish later ?
     */
//    virtual int SetTime (Time) = 0;

    /**
     *
     */
    virtual int InjectOffset (Time delta) = 0 ;
    
//    virtual int AdjTime (Time delta, Time *olddelta) = 0;

    virtual void SetFrequencyChangeCallback(FrequencyCallback);

    //! TODO
//    virtual void SettimeStepCallback( TimeStepCallback );


    void DoDispose();

protected:

    /**
     * Converts local time to simulator time
     * \param localTime The local time we want to convert into simulator time
     * \param absTime Returned value
     *
     * \return False if could not convert because localTime predates last update
     */
    virtual bool LocalTimeToSimulatorTime(Time localTime, Time *absTime) = 0;

    /**
     * Converts local time to simulator time
     * \param localTime The local time we want to convert into simulator time
     * \param absTime Returned value
     *
     * \return False if could not convert because localTime predates last update
     */
    virtual bool SimulatorTimeToLocalTime(Time absTime, Time *localTime) = 0;
    
    /**
     * Notifies the node it should reschedule scheduled events to correct their scheduled time
     *
     * Parameters are 
     */
    virtual void NotifyNewParameters ();

private:

    // or as DeviceAdditionListenerList, ie if several classes
    // want to get notified ?
//    TimeStepCallback m_onTimeStep;
    FrequencyCallback m_onNewFrequency;
};

}

#endif
