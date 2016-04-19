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
 *
 * This is the base class that decides how Node time evolves.
 * The clock describes a model to convert local-to-simulator time and vice-versa.
 *
 * It is oblivious to how subclassed clock model elapsed time.
 *
 * In order to the respect the discrete nature, these subclasses 
 * need to respect a few constraints though:
 * - they must notify the node whenever one of its parameters get updated. 
 *  This allows the node to remove scheduled events and to reschedule these very events with a corrected time.
 * - clocks must also be able to compute the local duration between a future time and the last 
 *   clock parameter updates back to simulator time
 *
 *
 * This also extends the range of supported applications by DCE, which can potentially
 * now be used as a testing platform for ntpd, chrony or other ntp servers.
 * In that regards it is similar to clknetsim
 * \see clknetsim (https://github.com/mlichvar/clknetsim)
 *
 * For an example of a simple concrete clock, you can have a look at the ClockPerfect class
 */
class Clock : public Object
{
public:
    typedef Callback<void> ClockUpdateCallback;

    static TypeId GetTypeId (void);
    virtual TypeId GetInstanceTypeId (void) const;

    Clock ();
    virtual ~Clock ();

    /**
     * \return Local time
     */
    virtual Time GetTime () = 0;

    /**
     * Set local time to passed time regardless of any other consideration
     * 
     * @param [in] newTime New local time of the clock
     */
    virtual void SetTime (Time newTime) = 0;

    /**
     * Set function to be called when clock parameters change and require linked simulator
     * to fix past scheduled events
     *
     * @param [in] cb Function to call when past predictions may become invalid
     */
    virtual void SetSimulatorSyncCallback (ClockUpdateCallback cb);

    /**
     * Converts local time to simulator time
     * @param [in]  localTime The local time we want to convert into simulator time
     * @param [out] absTime Predicted simulator time equivalent to localTime
     *
     * @return False if could not convert because localTime predates last update
     */
    virtual bool LocalTimeToSimulatorTime(Time localTime, Time *absTime) = 0;

    /**
     * Converts local time to simulator time
     * @param [in] localTime The local time we want to convert into simulator time
     * @param [out] absTime Returned value
     *
     * @return False if could not convert because localTime predates last update
     */
    virtual bool SimulatorTimeToLocalTime(Time absTime, Time *localTime) = 0;

protected:
    /**
     * Notifies the node it should reschedule scheduled events to correct 
     * the previously predicted times
     */
    virtual void NotifyClockUpdate ();

    /**
     *
     */
    void DoDispose();

    /**
     * Should be set by owner of the clock so that it can reschedule event
     */
    ClockUpdateCallback m_rescheduleEventsCb;
};

}

#endif
