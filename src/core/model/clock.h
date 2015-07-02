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
TODO rename into VirtualClock ?
Inspired by clknetsim
**/
class Clock : public Object
{
public:
    typedef Callback<void, double, double> FrequencyCallback;


    typedef Callback<void, double, double> TimeStepCallback;

    static TypeId GetTypeId (void);
    virtual TypeId
    GetInstanceTypeId (void) const;
//enum Result {
//	 TIME_INS = 1 /* insert leap second */
//	 TIME_DEL = 2 /* delete leap second */
//	 TIME_OOP  3 /* leap second in progress */
//	 TIME_WAIT 4 /* leap second has occurred */
//	 TIME_BAD  5 /* clock not synchronized */
//}
    virtual ~Clock() {};

    /**
     *
     */
    virtual Time GetTime() = 0;

    /**
     *
     */
    virtual int SetTime(Time) = 0;

    /**
     *
    **/
    virtual int AdjTime(Time delta, Time *olddelta) = 0;

    virtual void SetFrequencyChangeCallback(FrequencyCallback);

    //! TODO
    virtual void SettimeStepCallback( TimeStepCallback );


    void DoDispose();

protected:
    void NotifyNewFrequency (double oldFreq, double newFreq);

private:

    // Set total frequency as a TraceSource ?
    // or as DeviceAdditionListenerList, ie if several classes
    // want to get notified ?
    TimeStepCallback m_onTimeStep;
    FrequencyCallback m_onNewFrequency;


    /**
    Permit to send time relatively to the absolute time
     */
//    virtual void SetOffsetFromAbsoluteTime(Time) = 0;

//    virtual void SetResolution(Time::Unit );
// Ptr<OffsetGenerator> m_offsetGenerator;	//!<
};

}

#endif
