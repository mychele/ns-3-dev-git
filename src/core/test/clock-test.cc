/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2015 Université Pierre et Marie Curie (UPMC)
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
 * Author: Matthieu Coudron <matthieu.coudron@lip6.fr>
 */
#include "ns3/test.h"
#include "ns3/simulator.h"
#include "ns3/clock.h"
#include "ns3/clock-perfect.h"
#include "ns3/log.h"
#include <map>


using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("ClockTest");

//enum TestStep {
//CheckTime,
//ChangeFrequency,
//ChangeOffset
//};
//
//struct ClockTestParameters {
//
//ClockTestParameters( double freq ) :
//    action(ChangeFrequency),
//    frequency(freq)
//{
//}
//
//ClockTestParameters( Time t ) :
//    action(CheckTime),
//    value(t)
//{
//}
//
//TestStep action;
//double frequency;
//Time value;
//
//};


void FakeEvent()
{
    std::cout << "Simulator time advanced to " << Simulator::Now() << std::endl;
}

/** 
 *
 */
class ClockRawFrequencyTestCase : public TestCase
{
public:
//  typedef std::map<Time,Time> LocalToAbsTimeList;
  /* Absolute time */
//  typedef std::map<Time, ClockTestParameters > TestEvents;
//, ClockRawFrequencyTestCase::TestEvents
  ClockRawFrequencyTestCase(double rawFrequency1 );
  virtual ~ClockRawFrequencyTestCase () {};
  
  /**
   * From DoSetup, use a mix of CheckTime/ChangeFrequency/InjectOffset
   */
  virtual void DoSetup (void);
  virtual void DoRun (void);

//  virtual void OnNewFrequency(double oldFreq, double newFreq);
  virtual void OnNewFrequency (double oldFreq, double newFreq);
//  virtual void OnTimeStep();

protected:
  virtual void CheckTime (Time absTime, Time localTime);
  virtual void ChangeFrequency (Time absTime, double newFrequency);

  // Not implemented yet
  virtual void InjectOffset (Time absTime, Time offset);

  void SetupClock (void);

private:
  void ScheduledTimeCheck ( Time localTime);
  void ScheduledFrequencyChange ( double newFrequency);
  void ScheduledOffset ( Time offset);

  double m_frequency;           /**!<  */
  Ptr<ClockPerfect> m_clock;    /**!<  */

//  TestEvents  m_tests;
  int m_notifiedFrequencyChanges;    /**!< Check frequency was changed often enough */
  int m_expectedFrequencyChange;    /**!< Check frequency was changed often enough */
};


ClockRawFrequencyTestCase::ClockRawFrequencyTestCase (
    double rawFrequency
//    ,
//    ClockRawFrequencyTestCase::TestEvents  tests
    )
    : TestCase ("Clock frequency"),
    m_frequency (rawFrequency),
//    m_tests (tests),
    m_notifiedFrequencyChanges (0),
    m_expectedFrequencyChange (0)
{
    NS_LOG_FUNCTION (this);
}


//void ClockRawFrequencyTestCase::OnNewFrequency (double oldFreq, double newFreq)
void 
ClockRawFrequencyTestCase::OnNewFrequency (double oldFreq, double newFreq)
{
//    std::cout << "New frequency=" << newFreq << " (replacing " << oldFreq << ")" << std::endl;
    std::cout << "  Clock time=" << m_clock->GetLastTimeUpdateLocal() << std::endl;
    std::cout << "  Recorded Simu time=" << m_clock->GetLastTimeUpdateSim()
        << "(compare with " << Simulator::Now() << ")" <<std::endl;

    NS_TEST_ASSERT_MSG_EQ (m_clock->GetLastTimeUpdateSim (), Simulator::Now(), "Should be equal" );
    m_notifiedFrequencyChanges++;
}

  
void 
ClockRawFrequencyTestCase::ScheduledTimeCheck (Time localTime)
{
    std::cout << "Checking Local -> Absolute" << std::endl;
    NS_TEST_EXPECT_MSG_EQ (m_clock->GetTime (), localTime, "Clock returns wrong result");

//    NS_LOG_INFO ("Checking Absolute -> Local");
//    NS_TEST_EXPECT_MSG_EQ (m_clock->SimulatorTimeToLocalTime ( Simulator::Now(), &result), true, "Could not compute" );
//    NS_TEST_EXPECT_MSG_EQ (result, nodeTime, "test");

}


void 
ClockRawFrequencyTestCase::ScheduledFrequencyChange (double newFreq)
{
//    NS_LOG_INFO ("Checking Local -> Absolute");
//    NS_TEST_EXPECT_MSG_EQ (m_clock->GetTime (), localTime, "Clock returns wrong result");
    std::cout << "Setting frequency to " << newFreq << std::endl;
    NS_TEST_EXPECT_MSG_EQ (m_clock->SetRawFrequency (newFreq), true, "Could not set frequency");
//    NS_LOG_INFO ("Checking Absolute -> Local");
//    NS_TEST_EXPECT_MSG_EQ (m_clock->SimulatorTimeToLocalTime ( Simulator::Now(), &result), true, "Could not compute" );
//    NS_TEST_EXPECT_MSG_EQ (result, nodeTime, "test");

}

void 
ClockRawFrequencyTestCase::ScheduledOffset (Time offset)
{
    m_clock->InjectOffset (offset);
//    NS_LOG_INFO ("Checking Local -> Absolute");
//    NS_TEST_EXPECT_MSG_EQ (m_clock->GetTime (), localTime, "Clock returns wrong result");
//    NS_TEST_ASSERT (m_clock->SetRawFrequency (newFreq));
//    NS_LOG_INFO ("Checking Absolute -> Local");
//    NS_TEST_EXPECT_MSG_EQ (m_clock->SimulatorTimeToLocalTime ( Simulator::Now(), &result), true, "Could not compute" );
//    NS_TEST_EXPECT_MSG_EQ (result, nodeTime, "test");

}

void 
ClockRawFrequencyTestCase::CheckTime (Time absTime, Time localTime)
{
    Simulator::Schedule (absTime, &ClockRawFrequencyTestCase::ScheduledTimeCheck, this, localTime );
}

void 
ClockRawFrequencyTestCase::ChangeFrequency (Time absTime, double newFrequency)
{
    m_expectedFrequencyChange++;
    Simulator::Schedule (absTime, &ClockRawFrequencyTestCase::ScheduledFrequencyChange, this, newFrequency);
}



void 
ClockRawFrequencyTestCase::InjectOffset (Time absTime, Time offset) 
{
    Simulator::Schedule (absTime, &ClockRawFrequencyTestCase::ScheduledOffset, this, offset);
}
  

void
ClockRawFrequencyTestCase::SetupClock (void)
{
    std::cout << "Setup clock " << std::endl;
    m_clock = CreateObject<ClockPerfect> ();
    bool res = m_clock->SetRawFrequency (m_frequency);
//    m_clock->SetFrequencyChangeCallback( MakeCallback(&ClockRawFrequencyTestCase::OnNewFrequency, this));

    NS_ASSERT (res);

    //
    res = m_clock->TraceConnectWithoutContext ("RawFrequency", 
        MakeCallback ( &ClockRawFrequencyTestCase::OnNewFrequency, this) 
        );
    NS_ASSERT (res);

}

// TODO passer en virtual ?
void
ClockRawFrequencyTestCase::DoSetup (void)
{
    std::cout << "Setup" << std::endl;
    SetupClock ();

    // Check that at simulator time AbsoluteTime, local time is RelativeTime
    // hence when the simulator time is 2 
    CheckTime (Time (2), Time (1));
    CheckTime (Time (4), Time (2));
    // at simulator time 6, node time should be 3
    CheckTime (Time (6), Time (3));
    // Hence we change frequency so that clocks elapse at the same frequency
    ChangeFrequency (Time (6), 1.0);
    // 1 second elapsed since syntonization between simulator and node clock
    CheckTime (Time (7), Time (4));
}



void
ClockRawFrequencyTestCase::DoRun (void)
{


    NS_TEST_ASSERT_MSG_EQ (m_clock->GetRawFrequency (), m_frequency, "Wrong raw frequency. Did you call SetupClock ?");

//    m_clock->SetFrequencyChangeCallback( MakeCallback(&ClockRawFrequencyTestCase::OnNewFrequency, this));

    Simulator::Run ();

    NS_TEST_EXPECT_MSG_EQ (m_notifiedFrequencyChanges, m_expectedFrequencyChange, "The frequency change callback has not always been called");

};


/**
TODO
- write tests with offset
- with Adjtime correction

**/
// TODO with Offset
//ClockRawFrequencyTestCase::
#if 0
class ClockWithAdjTimeTestCase : public ClockRawFrequencyTestCase
{
    ClockWithAdjTimeTestCase(maxSlewRate);
};

ClockWithAdjTimeTestCase::ClockWithAdjTimeTestCase(
    double frequency,
    Time localDuration,
    Time absDuration,
    Time adjTimeOffset
//    Time whenToAddOffset
           )
{
    //!
}
#endif




class ClockTestSuite : public TestSuite
{
public:
  ClockTestSuite ()
    : TestSuite ("clock")
  {

//    ClockRawFrequencyTestCase::TestEvents tests;

    // TODO lui passer la clock ?
    {

        // Node clock is twice as slow as simulator's,
        ClockRawFrequencyTestCase *test0 = new ClockRawFrequencyTestCase (0.5);

//        test0->ChangeFrequency (Time (6), 1.0);

    //    test0->CheckTime (Time (7), Time (4));

        // Node's time goes twice faster than simulation time
        AddTestCase ( test0, TestCase::QUICK);
    }
    
    {
//        ClockRawFrequencyTestCase *test1 = new ClockRawFrequencyTestCase (1.0);
//        test1->CheckTime (Time (1), Time (1));
//        test1->CheckTime (Time (2), Time (2));
//        test1->ChangeFrequency (Time (3), 2.0);
    }
    // First clear the map, then add new tests
//    test.clear();
//    AddTestCase (new ClockRawFrequencyTestCase (0.5, tests), TestCase::QUICK);

  }
} g_clockTestSuite;
