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
//#include "ns3/list-scheduler.h"
//#include "ns3/heap-scheduler.h"
//#include "ns3/map-scheduler.h"
//#include "ns3/calendar-scheduler.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("ClockTestSuite");

//struct ClockTestParameters {
//double frequency;
//Time adjTimeStart;
//Time adjTimeOffset;
//Time maxSlewRate;
//};


class ClockRawFrequencyTestCase : public TestCase
{
public:
  typedef std::map<Time,Time> LocalToAbsTimeList;

  ClockRawFrequencyTestCase(double frequency, LocalToAbsTimeList);
  virtual ~ClockRawFrequencyTestCase() {}
  virtual void DoSetup (void);
  virtual void DoRun (void);
//  virtual void DoTeardown (void);
  double m_frequency;
  Ptr<ClockPerfect> m_clock;
//  Time m_absDuration;
//  Time m_localDuration;

  LocalToAbsTimeList m_checks;  /* localToAbs */

};


ClockRawFrequencyTestCase::ClockRawFrequencyTestCase(
    double frequency,
//    Time localDuration,
//    Time absDuration,
    LocalToAbsTimeList localToAbs
    )
    : TestCase("Clock frequency"),
    m_frequency(frequency),
    m_checks(localToAbs)
//        m_absDuration(absDuration),
//        m_localDuration(localDuration),
{
    NS_LOG_FUNCTION(this);
}


void
ClockRawFrequencyTestCase::DoSetup (void)
{
    m_clock = CreateObject<ClockPerfect>();
}


void
ClockRawFrequencyTestCase::DoRun (void)
{

    NS_ASSERT(m_clock->SetRawFrequency(m_frequency));
    NS_ASSERT(m_clock->GetRawFrequency() == m_frequency);

    // TODO create a node
//    Simulator::
//    NS_ASSERT_MSG()
    for(LocalToAbsTimeList::const_iterator i(m_checks.begin());
        i != m_checks.end();
        ++i
        )
        {
            NS_LOG_UNCOND("hello world");
            //!
//            i->first // local time
//            i->second // abs time
            Time result;
            NS_TEST_EXPECT_MSG_EQ(m_clock->LocalTimeToAbsTime(i->first, result), true, "Time found");
            NS_TEST_EXPECT_MSG_EQ(result, i->second, "Computed ");


            NS_TEST_EXPECT_MSG_EQ(m_clock->AbsTimeToLocalTime(i->second, result) );
            NS_ASSERT(result == i->first);
        }

}

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
//    ObjectFactory factory;
//    factory.SetTypeId (ListScheduler::GetTypeId ());
    ClockRawFrequencyTestCase::LocalToAbsTimeList checks;
    checks.insert(std::make_pair( Time(1), Time(2) ));
    checks.insert(std::make_pair( Time(2), Time(4) ));


    // Node's time goes twice faster than simulation time
    AddTestCase (new ClockRawFrequencyTestCase (2.0, checks), TestCase::QUICK);

    checks.clear();
    checks.insert(std::make_pair( Time(2), Time(1) ));
    checks.insert(std::make_pair( Time(4), Time(2) ));
    // Node's time grows at half simulation time speed
//    AddTestCase (new ClockRawFrequencyTestCase (2.0, checks), TestCase::QUICK);

//    AddTestCase (new ClockRawFrequencyTestCase (1.0, checks), TestCase::QUICK);
//    AddTestCase (new ClockWithAdjTimeTestCase (factory), TestCase::QUICK);

  }
} g_clockTestSuite;
