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

enum TestStep {
CheckTime,
ChangeFrequency,
ChangeOffset
};

union TestValue {
double frequency;
Time matchingAbs;
};

struct ClockTestParameters {

ClockTestParameters( double freq ) :
    action(ChangeFrequency),
    frequency(freq)
{
}

ClockTestParameters( Time t ) :
    action(CheckTime),
    value(t)
{
}

TestStep action;
double frequency;
Time value;

};


void FakeEvent()
{
    std::cout << "Simulator time advanced to " << Simulator::Now() << std::endl;
}

class ClockRawFrequencyTestCase : public TestCase
{
public:
//  typedef std::map<Time,Time> LocalToAbsTimeList;
  /* Absolute time */
  typedef std::map<Time, ClockTestParameters > TestEvents;

  ClockRawFrequencyTestCase(double rawFrequency1, ClockRawFrequencyTestCase::TestEvents );
  virtual ~ClockRawFrequencyTestCase() {}
  virtual void DoSetup (void);
  virtual void DoRun (void);

  virtual void OnNewFrequency(double oldFreq, double newFreq);
//  virtual void OnTimeStep();

protected:

  double m_frequency;
  Ptr<ClockPerfect> m_clock;

  const TestEvents  m_tests;
  int m_nb_frequency_change;
};


ClockRawFrequencyTestCase::ClockRawFrequencyTestCase(
    double rawFrequency,
    ClockRawFrequencyTestCase::TestEvents  tests
    )
    : TestCase("Clock frequency"),
    m_frequency(rawFrequency),
    m_tests(tests),
    m_nb_frequency_change(0)
{
    NS_LOG_FUNCTION(this);
}


void ClockRawFrequencyTestCase::OnNewFrequency(double oldFreq, double newFreq)
{
    std::cout << "New frequency=" << newFreq << " (replacing " << oldFreq << ")" << std::endl;
    std::cout << "  Clock time=" << m_clock->GetLastTimeUpdateLocal() << std::endl;
    std::cout << "  Recorded Simu time=" << m_clock->GetLastTimeUpdateSim()
        << "(compare with " << Simulator::Now() << ")" <<std::endl;

    NS_TEST_ASSERT_MSG_EQ(m_clock->GetLastTimeUpdateSim(), Simulator::Now(), "Should be equal" );
    m_nb_frequency_change++;
}

void
ClockRawFrequencyTestCase::DoSetup (void)
{
    m_clock = CreateObject<ClockPerfect>();
    NS_ASSERT(m_clock->SetRawFrequency(m_frequency));

}


void
ClockRawFrequencyTestCase::DoRun (void)
{


    NS_TEST_ASSERT_MSG_EQ(m_clock->GetRawFrequency(), m_frequency, "Wrong raw frequency");

    m_clock->SetFrequencyChangeCallback( MakeCallback(&ClockRawFrequencyTestCase::OnNewFrequency, this));


    int nbFreqChanges = 0;

    for(TestEvents::const_iterator i(m_tests.begin());
        i != m_tests.end();
        ++i
        )
        {
            Time simTime = i->first;
            const ClockTestParameters& params = i->second;

            std::cout << "New event " << params.action << " at time " << simTime << std::endl;

            Simulator::Schedule( simTime - Simulator::Now(), &FakeEvent);
            Simulator::Run();

            switch(params.action)
            {
            case CheckTime:
                {
                    Time result;
                    const Time nodeTime = params.value;

                    NS_LOG_INFO("Checking Local -> Absolute");
                    NS_TEST_EXPECT_MSG_EQ(m_clock->GetTime(), nodeTime, "Clock returns wrong result");

                    NS_LOG_INFO("Checking Absolute -> Local");
                    NS_TEST_EXPECT_MSG_EQ(m_clock->AbsTimeToLocalTime(simTime, result), true, "Could not compute" );
                    NS_TEST_EXPECT_MSG_EQ(result, nodeTime, "test");
                }
                break;
            case ChangeFrequency:
                nbFreqChanges++;
                NS_ASSERT(m_clock->SetRawFrequency(params.frequency));
                break;
            case ChangeOffset:
            default:
                NS_FATAL_ERROR("Unhandled action");

            }
        }



    NS_TEST_EXPECT_MSG_EQ(m_nb_frequency_change, nbFreqChanges, "The frequency change callback has not always been called");

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

    ClockRawFrequencyTestCase::TestEvents tests;

    tests.insert( std::make_pair(Time(2), ClockTestParameters (Time(1)) ) );
    tests.insert( std::make_pair(Time(4), ClockTestParameters (Time(2)) ) );
    tests.insert( std::make_pair(Time(6), ClockTestParameters (1.0) ) );
    tests.insert( std::make_pair(Time(7), ClockTestParameters (Time(4)) ) );


    // Node's time goes twice faster than simulation time
    AddTestCase (new ClockRawFrequencyTestCase (2.0, tests), TestCase::QUICK);

    // First clear the map, then add new tests
//    test.clear();
//    AddTestCase (new ClockRawFrequencyTestCase (0.5, tests), TestCase::QUICK);

  }
} g_clockTestSuite;
