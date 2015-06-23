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
		static TypeId GetTypeId (void);
		virtual TypeId
		GetInstanceTypeId (void) const;

    virtual ~Clock() {};

    /**
     *
     */
    virtual Time GetTime() = 0;

    /**
     *
     */
    virtual void SetTime(Time) = 0;

    /**
     *
     */
//    Adjtime()
    /**
    Permit to send time relatively to the absolute time
     */
//    virtual void SetOffsetFromAbsoluteTime(Time) = 0;

//    virtual void SetResolution(Time::Unit );
// Ptr<OffsetGenerator> m_offsetGenerator;	//!<
};

}

#endif
