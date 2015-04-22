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
#ifndef CLOCK_PERFECT_H
#define CLOCK_PERFECT_H


#include "ns3/clock.h"
//#include "ns3/time.h"


namespace ns3 {

class Time;
class RandomVariableStream;
class UniformRandomVariable;

/**
* \see ClockMonoticIncreasing
* No drift
*/
class ClockPerfect : public Clock
{

public:
    ClockPerfect ();
    virtual ~ClockPerfect ();

    virtual Time GetTime() ;
    virtual void SetTime(Time);

    /**
     * Maximum offset we can add to absolute time
     *
     * Ideally we should be able to pass a distribution/random generator
     */
    virtual void SetMaxRandomOffset(double);

//    virtual void SetPrecision(Time);
//    virtual Time GetPrecision(Time);
protected:
    double m_maxRandomOffset;
//    Ptr<RandomVariableStream> m_gen;
    Ptr<UniformRandomVariable> m_gen;
};

}

#endif
