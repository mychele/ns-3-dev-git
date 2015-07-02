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



class NodeEventsTestCase : public TestCase
{
public:
  SimulatorEventsTestCase (ObjectFactory schedulerFactory);
  virtual void DoRun (void);
  void EventA (int a);
  void EventB (int b);
  void EventC (int c);
  void EventD (int d);
  void Eventfoo0 (void);
  uint64_t NowUs (void);
  void destroy (void);
  bool m_b;
  bool m_a;
  bool m_c;
  bool m_d;
  EventId m_idC;
  bool m_destroy;
  EventId m_destroyId;
  ObjectFactory m_schedulerFactory;
};



void
SimulatorEventsTestCase::DoRun (void)
{
  m_a = true;
  m_b = false;
  m_c = true;
  m_d = false;

  Simulator::SetScheduler (m_schedulerFactory);

  EventId a = Simulator::Schedule (MicroSeconds (10), &SimulatorEventsTestCase::EventA, this, 1);
  Simulator::Schedule (MicroSeconds (11), &SimulatorEventsTestCase::EventB, this, 2);
  m_idC = Simulator::Schedule (MicroSeconds (12), &SimulatorEventsTestCase::EventC, this, 3);

  NS_TEST_EXPECT_MSG_EQ (!m_idC.IsExpired (), true, "");
  NS_TEST_EXPECT_MSG_EQ (!a.IsExpired (), true, "");
  Simulator::Cancel (a);
  NS_TEST_EXPECT_MSG_EQ (a.IsExpired (), true, "");
  Simulator::Run ();
  NS_TEST_EXPECT_MSG_EQ (m_a, true, "Event A did not run ?");
  NS_TEST_EXPECT_MSG_EQ (m_b, true, "Event B did not run ?");
  NS_TEST_EXPECT_MSG_EQ (m_c, true, "Event C did not run ?");
  NS_TEST_EXPECT_MSG_EQ (m_d, true, "Event D did not run ?");

  EventId anId = Simulator::ScheduleNow (&SimulatorEventsTestCase::Eventfoo0, this);
  EventId anotherId = anId;
  NS_TEST_EXPECT_MSG_EQ (!(anId.IsExpired () || anotherId.IsExpired ()), true, "Event should not have expired yet.");

  Simulator::Remove (anId);
  NS_TEST_EXPECT_MSG_EQ (anId.IsExpired (), true, "Event was removed: it is now expired");
  NS_TEST_EXPECT_MSG_EQ (anotherId.IsExpired (), true, "Event was removed: it is now expired");

  m_destroy = false;
  m_destroyId = Simulator::ScheduleDestroy (&SimulatorEventsTestCase::destroy, this);
  NS_TEST_EXPECT_MSG_EQ (!m_destroyId.IsExpired (), true, "Event should not have expired yet");
  m_destroyId.Cancel ();
  NS_TEST_EXPECT_MSG_EQ (m_destroyId.IsExpired (), true, "Event was canceled: should have expired now");

  m_destroyId = Simulator::ScheduleDestroy (&SimulatorEventsTestCase::destroy, this);
  NS_TEST_EXPECT_MSG_EQ (!m_destroyId.IsExpired (), true, "Event should not have expired yet");
  Simulator::Remove (m_destroyId);
  NS_TEST_EXPECT_MSG_EQ (m_destroyId.IsExpired (), true, "Event was canceled: should have expired now");

  m_destroyId = Simulator::ScheduleDestroy (&SimulatorEventsTestCase::destroy, this);
  NS_TEST_EXPECT_MSG_EQ (!m_destroyId.IsExpired (), true, "Event should not have expired yet");

  Simulator::Run ();
  NS_TEST_EXPECT_MSG_EQ (!m_destroyId.IsExpired (), true, "Event should not have expired yet");
  NS_TEST_EXPECT_MSG_EQ (!m_destroy, true, "Event should not have run");

  Simulator::Destroy ();
  NS_TEST_EXPECT_MSG_EQ (m_destroyId.IsExpired (), true, "Event should have expired now");
  NS_TEST_EXPECT_MSG_EQ (m_destroy, true, "Event should have run");
}

void
SimulatorEventsTestCase::EventA (int a)
{
  m_a = false;
}

void
SimulatorEventsTestCase::EventB (int b)
{
  if (b != 2 || NowUs () != 11)
    {
      m_b = false;
    }
  else
    {
      m_b = true;
    }
  Simulator::Remove (m_idC);
  Simulator::Schedule (MicroSeconds (10), &SimulatorEventsTestCase::EventD, this, 4);
}

void
SimulatorEventsTestCase::EventC (int c)
{
  m_c = false;
}

void
SimulatorEventsTestCase::EventD (int d)
{
  if (d != 4 || NowUs () != (11+10))
    {
      m_d = false;
    }
  else
    {
      m_d = true;
    }
}

void
SimulatorEventsTestCase::Eventfoo0 (void)
{}



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

