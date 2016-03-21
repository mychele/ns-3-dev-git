
/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2007 Georgia Tech Research Corporation
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
 * Authors: Matthieu Coudron 
 */

#include "ns3/test.h"
#include "ns3/socket-factory.h"
#include "ns3/tcp-socket-factory.h"
#include "ns3/simulator.h"
#include "ns3/simple-channel.h"
#include "ns3/simple-net-device.h"
#include "ns3/drop-tail-queue.h"
#include "ns3/config.h"
//#include "ns3/ipv4-static-routing.h"
//#include "ns3/ipv4-list-routing.h"
//#include "ns3/ipv6-static-routing.h"
//#include "ns3/ipv6-list-routing.h"
//#include "ns3/node.h"
#include "ns3/inet-socket-address.h"
#include "ns3/inet6-socket-address.h"
//#include "ns3/uinteger.h"
//#include "ns3/log.h"

#include "ns3/ipv4-end-point.h"
#include "ns3/arp-l3-protocol.h"
#include "ns3/ipv4-l3-protocol.h"
#include "ns3/ipv6-l3-protocol.h"
//#include "ns3/icmpv4-l4-protocol.h"
//#include "ns3/icmpv6-l4-protocol.h"
//#include "ns3/udp-l4-protocol.h"
#include "ns3/tcp-l4-protocol.h"

#include "ns3/core-module.h"
//#include "ns3/point-to-point-helper.h"
#include "ns3/internet-stack-helper.h"
//#include "ns3/applications-module.h"
#include "ns3/network-module.h"
//#include "ns3/mp-tcp-socket-factory-impl.h"
#include "ns3/internet-module.h"
//#include "ns3/applications-module.h"
#include "ns3/network-module.h"
#include <string>

//NS_LOG_COMPONENT_DEFINE ("MpTcpIdTest");

using namespace ns3;


/**
To run with
NS_LOG="MpTcpTestSuite" ./waf --run "test-runner --suite=mptcp"
we should try not to add to internet dependancies ['bridge', 'mpi', 'network', 'core']
**/




/**
Number of interfaces per node
**/
class MpTcpIdManagerTestCase : public TestCase
{
public:

  MpTcpIdManagerTestCase (Ptr<MpTcpPathIdManager>);
  virtual ~MpTcpIdManagerTestCase () {};

protected:
//  virtual void DoSetup ();
  virtual void DoTeardown ();

//  virtual Ptr<Node> CreateMonohomedNode(uint8_t nbOfInterfaces);
//  virtual Ptr<Node> CreateNode(uint8_t nbOfInterfaces);

//  void AdvertiseAllAvailableAddresses();
//  void AdvertiseThenRemoveAllAdvertisedAddresses();
//  void AdvertiseThenCreateRemoveAllAdvertisedAddresses();
//  void CreateNsubflows();
//  void CreateFullmesh();

private:
  virtual void DoRun(void);
  Ptr<MpTcpPathIdManager> m_mgr;
};

MpTcpIdManagerTestCase::MpTcpIdManagerTestCase (Ptr<MpTcpPathIdManager> mgr) :
  TestCase("Id management for mptcp"),
  m_mgr (mgr)
{
  //!
  NS_ASSERT (m_mgr);
}


void
MpTcpIdManagerTestCase::DoTeardown (void)
{
  Simulator::Destroy ();
}



void
MpTcpIdManagerTestCase::DoRun (void)
{
//  NS_LOG_LOGIC ("Doing run in MpTcpIdManagerTestCase");


  InetSocketAddress addr0 ("192.168.1.0", 42 );
  InetSocketAddress addr1 ("192.168.1.1", 42 );
  InetSocketAddress addr2 ("192.168.1.2");
//  Inet6SocketAddress addr0 ( )

  std::set<Address> localAddresses;
  std::set<Address> remoteAddresses;

  NS_TEST_EXPECT_MSG_EQ (m_mgr->GetTypeId().IsChildOf ( MpTcpPathIdManager::GetTypeId () ), 
    true, "Should have a proper parent");
  
//  NS_LOG_LOGIC("Simulation ended");
//  m_mgr->AddLocalId ();
  uint8_t addrId0_0, addrId0_1, result;
  NS_TEST_EXPECT_MSG_EQ (m_mgr->AddLocalId (&addrId0_0, addr0), true, 
    "Could not add localId even though the list is empty");

  NS_TEST_EXPECT_MSG_EQ (m_mgr->AddLocalId (&addrId0_1, addr0), false, 
    "It should not be possible to register twice the same id");

  NS_TEST_EXPECT_MSG_EQ (m_mgr->AddId ( 4, addr0), true, 
    "First added remote id, should succeed !");
  NS_TEST_EXPECT_MSG_EQ (m_mgr->AddId ( 4, addr1), false, 
    "This test must fail (a similar id has been registered just before)");
  NS_TEST_EXPECT_MSG_EQ (m_mgr->AddLocalId (&addrId0_1, addr1), true, 
    "Adding a new address should be ok");
//  NS_TEST_ASSERT_MSG_EQ (true, true, "true doesn't equal true for some reason");

    
  NS_TEST_EXPECT_MSG_EQ (m_mgr->RemoveId (addrId0_1), true, 
    "Just registered so it should be possible to remove it");
  
//  m_mgr->GetIds (localAddresses);
//
//  //! Now I need to check that alls id are made available
//  NS_TEST_EXPECT_MSG_EQ (localAddresses.size(), 2, "We have added 2 addresses");
//  
  NS_TEST_EXPECT_MSG_EQ (m_mgr->GetMatch(&result, addr0), true, 
    "Must find it");
  NS_TEST_EXPECT_MSG_EQ (result, addrId0_0, "Ids must be the same");
//  m_mgr->GetRemoteIds (remoteAddresses);
}



static class MpTcpIdTestSuite : public TestSuite
{
public:
  MpTcpIdTestSuite ()
    : TestSuite ("mptcp-pm", UNIT)
  {
    Ptr<MpTcpPathIdManager> mgr = CreateObject<MpTcpPathIdManagerImpl> ();
    AddTestCase (new MpTcpIdManagerTestCase (mgr), TestCase::QUICK);
//    AddTestCase (new MpTcpIdManagerTestCase(), TestCase::QUICK);

  }

} g_MpTcpPmTestSuite;


