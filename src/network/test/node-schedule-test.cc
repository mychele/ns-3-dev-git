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
#include "ns3/map-scheduler.h"
#include "ns3/log.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("TestNodeScheduling");





/**
 * Available checks during test
 */
enum TestStep {
CheckTime,
ChangeFrequency,
ChangeOffset,
ScheduleCheck,
CancelEvent
};


/** 
 * TODO: we should check that it's possible to change clock during simulation
 * without invalidating results
 */


/**
 * In this test everything is scheduled and tests are executed during 
 * or after Simulator::Run 
 * deals with a single node
 */
class NodeEventsTestCase : public TestCase
{
public:

  NodeEventsTestCase (std::string descr, ObjectFactory schedulerFactory);


  void ChangeRawFrequency (double newFreq);
  EventId ScheduleCheck (Time nodeTime, Time expectedSimTime);
  void GenericEvent (Time expectedSimTime, Time expectedNodeTime);

  virtual void CancelEvent (EventId id, Time absTime);

  virtual EventId ScheduleNewClockFrequency  (Time localTime, double newFreq);

protected:

  ObjectFactory m_schedulerFactory;

  void SetupNode (Ptr<Node> node, double frequency);

  virtual void DoSetup (void);
  virtual void DoRun (void);

  Ptr<Node> m_node;     //!< Node
  Ptr<ClockPerfect> m_clock;    //!< clock aggregated to the node (for convenience)
  double m_clockRawFrequency;   //!<
  int m_executed;   //!< number of scheduled and run events
  int m_expectedEventNumber;   //!< number of scheduled and run events
};



NodeEventsTestCase::NodeEventsTestCase (
    std::string descr,
    ObjectFactory schedulerFactory
    )
  : TestCase ( descr + " [scheduler=" +
              schedulerFactory.GetTypeId ().GetName () + "]"),
    m_schedulerFactory (schedulerFactory),
    m_clockRawFrequency (1.0),
    m_expectedEventNumber (0)
{

}

void
NodeEventsTestCase::SetupNode (Ptr<Node> node, double frequency)
{
    m_clock = CreateObject<ClockPerfect>();
    m_node = CreateObject<Node>();
    m_node->SetClock (m_clock);

    m_clock->SetRawFrequency (frequency);
}




EventId
NodeEventsTestCase::ScheduleCheck (Time nodeTime, Time expectedSimTime)
{
    return m_node->Schedule ( nodeTime, 
            &NodeEventsTestCase::GenericEvent, this, 
            expectedSimTime, nodeTime );
}

EventId
NodeEventsTestCase::ScheduleNewClockFrequency (Time localTime, double newFreq)
{
    return m_node->Schedule (localTime, 
            &NodeEventsTestCase::ChangeRawFrequency, this, 
            newFreq);
}


void 
NodeEventsTestCase::CancelEvent (EventId id, Time absTime)
{
    m_node->Cancel (id);
}

void
NodeEventsTestCase::DoSetup (void)
{
    SetupNode (m_node, m_clockRawFrequency);

    int64_t data = 1000000000;
    double freq= 1;

    EventId a = ScheduleCheck ( Seconds (1), Seconds (1) );
    EventId b = ScheduleCheck ( Seconds (2), Seconds (2) );
    
    EventId c = ScheduleCheck ( Seconds (4), Seconds (4) );
    m_node->Cancel (c);

    m_expectedEventNumber = 2;
}

void
NodeEventsTestCase::ChangeRawFrequency (double newFreq)
{
    std::cout << "Changed frequency to " << newFreq << std::endl;
    m_clock->SetRawFrequency(newFreq);
}



void
NodeEventsTestCase::DoRun (void)
{

    Simulator::Run();

    NS_TEST_EXPECT_MSG_EQ (m_executed, m_expectedEventNumber, "Number of scheduled-cancelled events != number of executed.");

    Simulator::Destroy ();
}

void
NodeEventsTestCase::GenericEvent (Time expectedSimTime, Time expectedNodeTime)
{
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
    }

void
NodeChangeClockEventsTestCase::DoSetup (void)
{
    SetupNode (m_node, m_clockRawFrequency);

            
    EventId a = ScheduleCheck ( Seconds (1), Seconds (1) );
    EventId b = ScheduleCheck ( Seconds (2), Seconds (2) );

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

    // Node clock is the same as Simulator's
    AddTestCase (new NodeEventsTestCase ("Check basic event handling is working", factory), TestCase::QUICK);
    AddTestCase (new NodeChangeClockEventsTestCase (factory), TestCase::QUICK);

  }
} g_nodeSchedulingTestSuite;

