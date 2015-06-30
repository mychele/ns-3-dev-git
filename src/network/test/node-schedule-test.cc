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


#include "ns3/test.h"
#include "ns3/simulator.h"
#include "ns3/clock.h"
#include "ns3/clock-perfect.h"
//#include "ns3/list-scheduler.h"
//#include "ns3/heap-scheduler.h"
//#include "ns3/map-scheduler.h"
//#include "ns3/calendar-scheduler.h"

using namespace ns3;




/*
Idea of this test is :
-to setup a drifting clock,
-schedule an event
-change the clock at some point in time
-check that the previous event is correctly re-scheduled when the clock frequency changed
*/
#if 0

struct ClockTestParameters {
double frequency;
Time adjTimeStart;
Time adjTimeOffset;
Time maxSlewRate;
};


class ClockTestCase : public TestCase
{
public:
  ClockTestCase(ClockTestParameters);
  virtual void DoRun (void);

  double m_frequency;
  Ptr<ClockPerfect> m_clock;
};



ClockTestCase::ClockTestCase()
{
    m_clock = CreateObject<ClockPerfect>();
    NS_ASSERT(m_clock->SetFrequency());
}


void
ClockTestCase::DoRun (void)
{
    // TODO create a node
    Simulator::
}

#endif

class NodeClockTestSuite : public TestSuite
{
public:
  NodeClockTestSuite ()
    : TestSuite ("node-clock-interactions")
  {
//    ObjectFactory factory;
//    factory.SetTypeId (ListScheduler::GetTypeId ());

//    AddTestCase (new SimulatorEventsTestCase (factory), TestCase::QUICK);

  }
} g_nodeClockTestSuite;

