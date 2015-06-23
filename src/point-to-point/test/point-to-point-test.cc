/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2009 INRIA
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
 * Author: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 */

#include "ns3/test.h"
#include "ns3/drop-tail-queue.h"
#include "ns3/simulator.h"
#include "ns3/point-to-point-net-device.h"
#include "ns3/point-to-point-channel.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("PointToPointChannelTest");

/**
 * \brief Test class for PointToPoint model
 *
 * It tries to send one packet from one NetDevice to another, over a
 * PointToPointChannel.
 */
class PointToPointTest : public TestCase
{
public:
  /**
   * \brief Create the test
   * \param Forward propagation time
   * \param Backward propagation time
   */
  PointToPointTest (Time forwardOwd, Time backwardOwd);

  /**
   * \brief Run the test
   */
  virtual void DoRun (void);

private:
  /**
   * \brief Send one packet to the device specified
   *
   * \param device NetDevice to send to
   */
  void SendOnePacket (Ptr<PointToPointNetDevice> device);

	bool UponPacketReception(Ptr<NetDevice>, Ptr<const Packet>, uint16_t, const Address & );

  Time m_forwardOwd;		//!<
  Time m_backwardOwd;		//!<
};

PointToPointTest::PointToPointTest (Time forwardOwd, Time backwardOwd)
  : TestCase ("PointToPoint"),
  m_forwardOwd(forwardOwd),
  m_backwardOwd(backwardOwd)
{
}

void
PointToPointTest::SendOnePacket (Ptr<PointToPointNetDevice> device)
{
  Ptr<Packet> p = Create<Packet> ();
  device->Send (p, device->GetBroadcast (), 0x800);
}


//typedef Callback< bool, Ptr<NetDevice>, Ptr<const Packet>, uint16_t, const Address & > ns3::NetDevice::ReceiveCallback
bool
PointToPointTest::UponPacketReception(Ptr<NetDevice>, Ptr<const Packet>, uint16_t, const Address & )
{
	//! check that it took at least the propagation time
//  Ptr<Packet> p = Create<Packet> ();
//  device->Send (p, device->GetBroadcast (), 0x800);
}


void
PointToPointTest::DoRun (void)
{
  Ptr<Node> a = CreateObject<Node> ();
  Ptr<Node> b = CreateObject<Node> ();
  Ptr<PointToPointNetDevice> devA = CreateObject<PointToPointNetDevice> ();
  Ptr<PointToPointNetDevice> devB = CreateObject<PointToPointNetDevice> ();
  Ptr<PointToPointChannel> channel = CreateObject<PointToPointChannel> ();
	channel->SetAttribute("ForwardDelay", TimeValue (m_forwardOwd) );
	channel->SetAttribute("BackwardDelay", TimeValue (m_backwardOwd));

  devA->Attach (channel);
  devA->SetAddress (Mac48Address::Allocate ());
  devA->SetQueue (CreateObject<DropTailQueue> ());

  devB->Attach (channel);
  devB->SetAddress (Mac48Address::Allocate ());
  devB->SetQueue (CreateObject<DropTailQueue> ());
  devB->SetReceiveCallback( MakeBoundCallback(&));

  a->AddDevice (devA);
  b->AddDevice (devB);

  // Send 1 packet from A to B
  Simulator::Schedule (Seconds (1.0), &PointToPointTest::SendOnePacket, this, devA);

  Simulator::Run ();

  Simulator::Destroy ();
}

/**
 * \brief TestSuite for PointToPoint module
 */
class PointToPointTestSuite : public TestSuite
{
public:
  /**
   * \brief Constructor
   */
  PointToPointTestSuite ();
};

PointToPointTestSuite::PointToPointTestSuite ()
  : TestSuite ("devices-point-to-point", UNIT)
{
  // symmetric test
  AddTestCase (new PointToPointTest(MilliSeconds(10), MilliSeconds(10)), TestCase::QUICK);
  // asymmetric test
  AddTestCase (new PointToPointTest(MilliSeconds(100), MilliSeconds(50)), TestCase::QUICK);
}

static PointToPointTestSuite g_pointToPointTestSuite; //!< The testsuite
