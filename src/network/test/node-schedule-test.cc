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
#include "ns3/node.h"
#include "ns3/clock.h"
#include "ns3/clock-perfect.h"
#include "ns3/list-scheduler.h"
//#include "ns3/heap-scheduler.h"
#include "ns3/map-scheduler.h"
//#include "ns3/calendar-scheduler.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("TestNodeScheduling");


/**
TODO
-should be able to pass a clock
-create another class

rename ConstantFrequency
TODO schedule 2 events at the same time
**/

//struct LocalToAbs
//{
//Time local;
//Time abs;
//};

static const int MIN_NB = 2;
class NodeEventsTestCase : public TestCase
{
public:
  typedef std::vector< std::pair<Time,Time> > LocalToAbsTimeList;

  // TODO pouvoir passer plusieurs horloges
  NodeEventsTestCase (ObjectFactory schedulerFactory, double clockRawFrequency, LocalToAbsTimeList);
  virtual void DoRun (void);

  // These function change
  void EventA (int a);
  void EventB (int b);
  void EventC (int c);
  void EventD (int d);
  void Eventfoo0 (void);
//  uint64_t NowUs (void);
//  void destroy (void);

  void DoSetup (void);



  EventId m_idC;
//  bool m_destroy;
//  EventId m_destroyId;
  ObjectFactory m_schedulerFactory;

protected:
    // Rename to CommonToAllEvents
  void GenericChecks(int i);
  Time GetSchedule(int i);

private:
  // True
  bool m_a;
  bool m_b;
  bool m_c;
  bool m_d;
  Ptr<Node> m_node;
  double m_clockRawFrequency;   //!<
  LocalToAbsTimeList m_milestones;  //!<
};

NodeEventsTestCase::NodeEventsTestCase (
    ObjectFactory schedulerFactory,
    double clockRawFrequency,
    LocalToAbsTimeList milestones
    )
  : TestCase ("Check that basic event handling is working with " +
              schedulerFactory.GetTypeId ().GetName ()),
    m_schedulerFactory (schedulerFactory),
    m_clockRawFrequency(clockRawFrequency),
    m_milestones(milestones)
{
//    NS_LOG_FUNCTION
}

void
NodeEventsTestCase::DoSetup (void)
{
    NS_ASSERT_MSG(m_milestones.size() >= MIN_NB, "There should be as many milestones as event scheduled during the test");
    // TODO should be able to set the clock
//    Ptr<ClockPerfect> clock = CreateObject<ClockPerfect>();
    m_node = CreateObject<Node>();
//    Ptr<ClockPerfect> clock = m_node->GetObject<ClockPerfect>("ns3::ClockPerfect");
    Ptr<ClockPerfect> clock = m_node->GetObject<ClockPerfect>();
//    Ptr<ClockPerfect> clock = m_node->GetObject<ClockPerfect>();
    NS_ASSERT(clock->SetRawFrequency(m_clockRawFrequency));
    // TODO set scheduler

    // By default no event is marked as executed
    m_a = false;
    m_b = false;
    m_c = false;
    m_d = false;
}

Time
NodeEventsTestCase::GetSchedule(int i)
{
    NS_ASSERT( i >= 0 && i < m_milestones.size());
    return m_milestones[i].first;
}

void
NodeEventsTestCase::GenericChecks(int i)
{
    NS_ASSERT( i >= 0 && i < m_milestones.size());
    Time expectedNodeTime = m_milestones[i].first ;
    Time expectedSimTime = m_milestones[i].second ;

    NS_TEST_EXPECT_MSG_EQ( expectedNodeTime, m_node->GetWallTime(), "Wrong local time" );
    NS_TEST_EXPECT_MSG_EQ( expectedSimTime, Simulator::Now(), "Wrong absolute time" );
}

void
NodeEventsTestCase::DoRun (void)
{

  NS_TEST_EXPECT_MSG_EQ (m_a, false, "Wrong setup");
  NS_TEST_EXPECT_MSG_EQ (m_b, false, "Wrong setup");
  NS_TEST_EXPECT_MSG_EQ (m_c, false, "Wrong setup");
  NS_TEST_EXPECT_MSG_EQ (m_d, false, "Wrong setup");


//  Simulator::SetScheduler (m_schedulerFactory);

  EventId a = m_node->Schedule ( GetSchedule(0), &NodeEventsTestCase::EventA, this, 1);

  m_node->Schedule ( GetSchedule(1), &NodeEventsTestCase::EventB, this, 2);
  m_idC = m_node->Schedule (GetSchedule(2), &NodeEventsTestCase::EventC, this, 3);

//  m_idC = Simulator::Schedule (MicroSeconds (12), &NodeEventsTestCase::EventC, this, 3);
//  NS_TEST_EXPECT_MSG_EQ (!m_idC.IsExpired (), true, "");

  NS_TEST_EXPECT_MSG_EQ (!a.IsExpired (), true, "");
  m_node->Cancel (a);
  NS_TEST_EXPECT_MSG_EQ (a.IsExpired (), true, "");
  Simulator::Run ();
  NS_TEST_EXPECT_MSG_EQ (m_a, false, "Event should not run");


//  NS_TEST_EXPECT_MSG_EQ (m_b, true, "Event B did not run ?");
//  NS_TEST_EXPECT_MSG_EQ (m_c, true, "Event C did not run ?");
//  NS_TEST_EXPECT_MSG_EQ (m_d, true, "Event D did not run ?");
//
//  EventId anId = Simulator::ScheduleNow (&NodeEventsTestCase::Eventfoo0, this);
//  EventId anotherId = anId;
//  NS_TEST_EXPECT_MSG_EQ (!(anId.IsExpired () || anotherId.IsExpired ()), true, "Event should not have expired yet.");

//  Simulator::Remove (anId);
//  NS_TEST_EXPECT_MSG_EQ (anId.IsExpired (), true, "Event was removed: it is now expired");
//  NS_TEST_EXPECT_MSG_EQ (anotherId.IsExpired (), true, "Event was removed: it is now expired");

//  m_destroy = false;
//  m_destroyId = Simulator::ScheduleDestroy (&NodeEventsTestCase::destroy, this);
//  NS_TEST_EXPECT_MSG_EQ (!m_destroyId.IsExpired (), true, "Event should not have expired yet");
//  m_destroyId.Cancel ();
//  NS_TEST_EXPECT_MSG_EQ (m_destroyId.IsExpired (), true, "Event was canceled: should have expired now");
//
//  m_destroyId = Simulator::ScheduleDestroy (&NodeEventsTestCase::destroy, this);
//  NS_TEST_EXPECT_MSG_EQ (!m_destroyId.IsExpired (), true, "Event should not have expired yet");
//  Simulator::Remove (m_destroyId);
//  NS_TEST_EXPECT_MSG_EQ (m_destroyId.IsExpired (), true, "Event was canceled: should have expired now");
//
//  m_destroyId = Simulator::ScheduleDestroy (&NodeEventsTestCase::destroy, this);
//  NS_TEST_EXPECT_MSG_EQ (!m_destroyId.IsExpired (), true, "Event should not have expired yet");
//
//  Simulator::Run ();
//  NS_TEST_EXPECT_MSG_EQ (!m_destroyId.IsExpired (), true, "Event should not have expired yet");
//  NS_TEST_EXPECT_MSG_EQ (!m_destroy, true, "Event should not have run");
//
//  Simulator::Destroy ();
//  NS_TEST_EXPECT_MSG_EQ (m_destroyId.IsExpired (), true, "Event should have expired now");
//  NS_TEST_EXPECT_MSG_EQ (m_destroy, true, "Event should have run");
}

void
NodeEventsTestCase::EventA (int a)
{
  GenericChecks(0);
  m_a = false;
}

void
NodeEventsTestCase::EventB (int b)
{
    GenericChecks(1);
//  if (b != 2 || NowUs () != 11)
//    {
//      m_b = false;
//    }
//  else
//    {
//      m_b = true;
//    }
//  Simulator::Remove (m_idC);
//  Simulator::Schedule (MicroSeconds (10), &NodeEventsTestCase::EventD, this, 4);
}

void
NodeEventsTestCase::EventC (int c)
{
  m_c = false;
}

void
NodeEventsTestCase::EventD (int d)
{
//  if (d != 4 || NowUs () != (11+10))
//    {
//      m_d = false;
//    }
//  else
//    {
//      m_d = true;
//    }
}

void
NodeEventsTestCase::Eventfoo0 (void)
{}


/**
At some point the clock wil lchange frequency
**/
//class NodeChangeClockEventsTestCase : public NodeEventsTestCase
//{
//    //!
//    NodeChangeClockEventsTestCase
//};

/*
Idea of this test is :
-to setup a drifting clock,
-schedule an event
-change the clock at some point in time
-check that the previous event is correctly re-scheduled when the clock frequency changed
*/
#if 0

#endif

class NodeClockTestSuite : public TestSuite
{
public:
  NodeClockTestSuite ()
    : TestSuite ("node-clock-interactions")
  {
//          m_clock = CreateObject<ClockPerfect>();

    ObjectFactory factory;
    factory.SetTypeId (ListScheduler::GetTypeId ());
    NodeEventsTestCase::LocalToAbsTimeList checks;
    checks.push_back(std::make_pair( Time(1), Time(1) ));
    checks.push_back(std::make_pair( Time(2), Time(2) ));
    checks.push_back(std::make_pair( Time(3), Time(3) ));

    AddTestCase (new NodeEventsTestCase (factory, 1.0, checks), TestCase::QUICK);


    /// Node clock twice faster than sim time
    /////////////////////////////////////////////////////
//    checks.clear();
//    checks.push_back(std::make_pair( Time(1), Time(2) ));
//    checks.push_back(std::make_pair( Time(2), Time(4) ));
//    checks.push_back(std::make_pair( Time(3), Time(6) ));
//
//    AddTestCase (new NodeEventsTestCase (factory, 2.0, checks), TestCase::QUICK);

  }
} g_nodeClockTestSuite;

