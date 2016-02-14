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



static ObjectFactory schedulerFactory;

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

enum TestStep {
CheckTime,
ChangeFrequency,
ChangeOffset,
ScheduleCheck,
CancelEvent
};



/**
In this test everything is scheduled and tests are executed during or after Simulator::Run
Deals with a single node
**/

class NodeEventsTestCase : public TestCase
{
public:

  // TODO pouvoir passer plusieurs horloges
  NodeEventsTestCase (std::string descr, ObjectFactory schedulerFactory);


  void ChangeRawFrequency (double newFreq);
//  void GenericEvent (int a);
  EventId ScheduleCheck (Time nodeTime, Time expectedSimTime);
  void GenericEvent (Time expectedSimTime, Time expectedNodeTime
//                     , int uid
                     );

  virtual void CancelEvent (EventId id, Time absTime);
  
  
  virtual EventId ScheduleNewClockFrequency  (Time localTime, double newFreq);
  
//virtual void ChangeClockFrequency (double newFreq);
//  virtual void ChangeClockFrequency (Time localTime);
  
//  void CheckTime (Time absTime, double newFreq);    
    
    // TODO later
//    virtual void RemoveEvent (Time absTime);
protected:

  ObjectFactory m_schedulerFactory;


  void SetupNode (Ptr<Node> node, double frequency);
  

  virtual void DoSetup (void);
  virtual void DoRun (void);
//private:
  // True
//  bool m_executed[NB_EVENTS];  //!<
//  std::map<int, EventId > m_eventIds; //!< Map test uid, EventId
  Ptr<Node> m_node;     //!< Node
  Ptr<ClockPerfect> m_clock;    //!< clock aggregated to the node (for convenience)
  double m_clockRawFrequency;   //!<
  int m_executed;   //!< number of scheduled and run events
  int m_expectedEventNumber;   //!< number of scheduled and run events
};



NodeEventsTestCase::NodeEventsTestCase (
    std::string descr,
    ObjectFactory schedulerFactory
//    TestParameters tests
    )
  : TestCase ( descr + " [scheduler=" +
              schedulerFactory.GetTypeId ().GetName () + "]"),
    m_schedulerFactory (schedulerFactory),
    m_clockRawFrequency (1.0),
    m_expectedEventNumber (0)
//    m_tests(tests)
{
//    NS_LOG_FUNCTION
}

void
NodeEventsTestCase::SetupNode (Ptr<Node> node, double frequency)
{
    // TODO should be able to set the clock
//    Ptr<ClockPerfect> clock = CreateObject<ClockPerfect>();
    m_node = CreateObject<Node>();
//    Ptr<ClockPerfect> clock = m_node->GetObject<ClockPerfect>("ns3::ClockPerfect");
    //!
    m_clock = m_node->GetObject<ClockPerfect>();
//    Ptr<ClockPerfect> clock = m_node->GetObject<ClockPerfect>();
    NS_ASSERT (m_clock->SetRawFrequency (frequency));
}




EventId
NodeEventsTestCase::ScheduleCheck (Time nodeTime, Time expectedSimTime)
{
    //!
    return m_node->Schedule ( nodeTime, 
            &NodeEventsTestCase::GenericEvent, this, 
            expectedSimTime, nodeTime );
}

//void
//NodeEventsTestCase::ChangeClockFrequency (Time localTime, double newFreq)
//{
//    m_node->
//}

EventId
NodeEventsTestCase::ScheduleNewClockFrequency (Time localTime, double newFreq)
{
    //!
    return m_node->Schedule (localTime, 
            &NodeEventsTestCase::ChangeRawFrequency, this, 
            newFreq);
}


void 
NodeEventsTestCase::CancelEvent (EventId id, Time absTime)
{
    //!
    m_node->Cancel (id);
}

void
NodeEventsTestCase::DoSetup (void)
{
    SetupNode (m_node, m_clockRawFrequency);

//    1000000000.0ns/1
    int64_t data = 1000000000;
    double freq= 1;
    NS_LOG_UNCOND (data/freq);
    // Schedule in local time
//    EventId eventA = m_node->Schedule (Time(1), 
//            &NodeEventsTestCase::GenericEvent, this, 
//            Time (1), Time (1));
            
    EventId a = ScheduleCheck ( Seconds (1), Seconds (1) );
    EventId b = ScheduleCheck ( Seconds (2), Seconds (2) );
//    EventId eventB = m_node->Schedule ( Seconds(2), 
//            &NodeEventsTestCase::GenericEvent, this, Seconds (2), Seconds (2) );
    
    EventId c = ScheduleCheck ( Seconds (4), Seconds (4) );
    m_node->Cancel (c);
//    CancelEvent (c);

    m_expectedEventNumber = 2;
}

void
NodeEventsTestCase::ChangeRawFrequency (double newFreq)
{
    //!
    std::cout << "Changed frequency to " << newFreq << std::endl;
    m_clock->SetRawFrequency(newFreq);
}



//void
//NodeEventsTestCase::GenericChecks(int i)
//{
//    NS_ASSERT( i >= 0 && i < m_milestones.size());
//    Time expectedNodeTime = m_milestones[i].first ;
//    Time expectedSimTime = m_milestones[i].second ;
//
//    NS_TEST_EXPECT_MSG_EQ( expectedNodeTime, m_node->GetLocalTime(), "Wrong local time" );
//    NS_TEST_EXPECT_MSG_EQ( expectedSimTime, Simulator::Now(), "Wrong absolute time" );
//}


void
NodeEventsTestCase::DoRun (void)
{

    Simulator::Run();


    NS_TEST_EXPECT_MSG_EQ (m_executed, m_expectedEventNumber, "Number of scheduled-cancelled events != number of executed.");

    Simulator::Destroy ();

//  Simulator::SetScheduler (m_schedulerFactory);

//  EventId a = m_node->Schedule ( GetSchedule(0), &NodeEventsTestCase::GenericEvent , this, 1);
//
//  m_node->Schedule ( GetSchedule(1), &NodeEventsTestCase::EventB, this, 2);
//  m_idC = m_node->Schedule (GetSchedule(2), &NodeEventsTestCase::EventC, this, 3);
//
////  m_idC = Simulator::Schedule (MicroSeconds (12), &NodeEventsTestCase::EventC, this, 3);
////  NS_TEST_EXPECT_MSG_EQ (!m_idC.IsExpired (), true, "");
//
//  NS_TEST_EXPECT_MSG_EQ (!a.IsExpired (), true, "");
//  m_node->Cancel (a);
//  NS_TEST_EXPECT_MSG_EQ (a.IsExpired (), true, "");
//  Simulator::Run ();
//  NS_TEST_EXPECT_MSG_EQ (m_a, false, "Event should not run");


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
NodeEventsTestCase::GenericEvent (Time expectedSimTime, Time expectedNodeTime
//                            , int uid
                            )
{
//  GenericChecks(0);
//    std::cout << "Event with uid="  << uid << std::endl;
    std::cout << " Expected at "  << expectedSimTime << "/" << expectedNodeTime << "(sim/node)" << std::endl;

    NS_TEST_EXPECT_MSG_EQ (expectedNodeTime, m_node->GetLocalTime(), "Wrong local time" );
    NS_TEST_EXPECT_MSG_EQ (expectedSimTime, Simulator::Now(), "Wrong absolute time" );
    m_executed++;
}



/**
At some point the clock wil lchange frequency
**/
class NodeChangeClockEventsTestCase : public NodeEventsTestCase
{
public:
    NodeChangeClockEventsTestCase (ObjectFactory schedulerFactory);
protected:
    virtual void DoSetup (void);
};


NodeChangeClockEventsTestCase::NodeChangeClockEventsTestCase (ObjectFactory schedulerFactory)
    : NodeEventsTestCase ("Test with a changing clock", schedulerFactory)
    {
        //!
    }

void
NodeChangeClockEventsTestCase::DoSetup (void)
{
    SetupNode (m_node, m_clockRawFrequency);

    // Schedule in local time
//    EventId eventA = m_node->Schedule (Time(1), 
//            &NodeEventsTestCase::GenericEvent, this, 
//            Time (1), Time (1));
            
    EventId a = ScheduleCheck ( Seconds (1), Seconds (1) );
    EventId b = ScheduleCheck ( Seconds (2), Seconds (2) );
    
//    EventId eventB = m_node->Schedule ( Seconds(2), 
//            &NodeEventsTestCase::GenericEvent, this, Seconds (2), Seconds (2) );
    // At local time 3 (=> sim time = 3), time in the node should elapse twice faster
    ScheduleNewClockFrequency(Seconds(3), 2);
    
    // BE CAREFUL HERE, it fails GetResolution
    // hence when 2 seconds elapsed in simulator time (From 3 to 5, then 1 second only should have 
    // elapsed in the simulator time hence 3 +1 => 4
    ScheduleCheck(Seconds(5), Seconds (4));

//    EventId c = ScheduleCheck ( Seconds (4), Seconds (4) );
//    m_node->Cancel (c);
//    CancelEvent (c);

    m_expectedEventNumber = 3;
}

/*
Idea of this test is :
-to setup a drifting clock,
-schedule an event
-change the clock at some point in time
-check that the previous event is correctly re-scheduled when the clock frequency changed

At some point the clock wil lchange frequency
**/
class CheckForAutomaticRescheduling : public NodeEventsTestCase
{
public:
    CheckForAutomaticRescheduling (ObjectFactory schedulerFactory);
protected:
    virtual void DoSetup (void);
};



void
CheckForAutomaticRescheduling::DoSetup (void)
{
    SetupNode (m_node, m_clockRawFrequency);

    // Schedule in local time
//    EventId eventA = m_node->Schedule (Time(1), 
//            &NodeEventsTestCase::GenericEvent, this, 
//            Time (1), Time (1));
            
    EventId b = ScheduleCheck ( Seconds (2), Seconds (2) );
    EventId a = ScheduleCheck ( Seconds (1), Seconds (1) );
    
    
//    EventId eventB = m_node->Schedule ( Seconds(2), 
//            &NodeEventsTestCase::GenericEvent, this, Seconds (2), Seconds (2) );
    // At local time 3 (=> sim time = 3), time in the node should elapse twice faster
    ScheduleNewClockFrequency(Seconds(3), 2);
    ScheduleNewClockFrequency(Seconds(3), 2);
    
    // BE CAREFUL HERE, it fails GetResolution
    // hence when 2 seconds elapsed in simulator time (From 3 to 5, then 1 second only should have 
    // elapsed in the simulator time hence 3 +1 => 4
    ScheduleCheck(Seconds(5), Seconds (4));

//    EventId c = ScheduleCheck ( Seconds (4), Seconds (4) );
//    m_node->Cancel (c);
//    CancelEvent (c);

    m_expectedEventNumber = 3;
}


/**
 *
 */
class NodeClockTestSuite : public TestSuite
{
public:
  NodeClockTestSuite ()
    : TestSuite ("node-scheduling")
  {

    ObjectFactory factory;
    factory.SetTypeId (ListScheduler::GetTypeId ());

    // Schedule a, cancel a
    // Schedule b,

    // Node clock is the same as Simulator's
    AddTestCase (new NodeEventsTestCase ("Check basic event handling is working", factory), TestCase::QUICK);
    AddTestCase (new NodeChangeClockEventsTestCase (factory), TestCase::QUICK);


    /// Node clock twice faster than sim time
    /////////////////////////////////////////////////////

//    AddTestCase (new NodeChangeClockEventsTestCase (factory), TestCase::QUICK);

  }
} g_nodeSchedulingTestSuite;

