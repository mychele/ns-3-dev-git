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

#include "ns3/clock.h"


namespace ns3 {

class ClockMonoticIncreasing : public Clock
{
public:
    ClockMonoticIncreasing();


    virtual Time GetTime() = 0;
    virtual void SetTime(Time) = 0;

    /**
     *
     */
//    virtual void SetOffset();
    virt
    // or setDrift ?
    /**
     * Drift compared to perfect clock
     * expressed in PPS ?
     */
    virtual void SetDrift(float drift);


protected:
    std::pair<Time,Time> m_lastAccess;   //!< last get/set <absTime,wallclock>

    double m_drift;

};

}
