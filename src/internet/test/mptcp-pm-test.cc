
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
 * Authors: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 *          Raj Bhattacharjea <raj.b@gatech.edu>
 */

#include "ns3/test.h"
#include "ns3/socket-factory.h"
#include "ns3/tcp-socket-factory.h"
#include "ns3/simulator.h"
#include "ns3/simple-channel.h"
#include "ns3/simple-net-device.h"
#include "ns3/drop-tail-queue.h"
#include "ns3/config.h"
#include "ns3/ipv4-static-routing.h"
#include "ns3/ipv4-list-routing.h"
#include "ns3/ipv6-static-routing.h"
#include "ns3/ipv6-list-routing.h"
#include "ns3/node.h"
#include "ns3/inet-socket-address.h"
#include "ns3/inet6-socket-address.h"
#include "ns3/uinteger.h"
#include "ns3/log.h"

#include "ns3/ipv4-end-point.h"
#include "ns3/arp-l3-protocol.h"
#include "ns3/ipv4-l3-protocol.h"
#include "ns3/ipv6-l3-protocol.h"
#include "ns3/icmpv4-l4-protocol.h"
#include "ns3/icmpv6-l4-protocol.h"
#include "ns3/udp-l4-protocol.h"
#include "ns3/tcp-l4-protocol.h"

#include "ns3/core-module.h"
#include "ns3/point-to-point-helper.h"
#include "ns3/internet-stack-helper.h"
#include "ns3/applications-module.h"
#include "ns3/network-module.h"
//#include "ns3/mp-tcp-socket-factory-impl.h"
#include "ns3/mptcp-uncoupled.h"
#include "ns3/internet-module.h"
#include "ns3/applications-module.h"
#include "ns3/network-module.h"
#include "ns3/tcp-newreno.h"
#include "ns3/point-to-point-module.h"
#include "ns3/mptcp-uncoupled.h"
//#include "ns3/point-to-point-channel.h"
#include <string>

NS_LOG_COMPONENT_DEFINE ("MpTcpTestSuite");

using namespace ns3;


/**
To run with
NS_LOG="MpTcpTestSuite" ./waf --run "test-runner --suite=mptcp"
we should try not to add to internet dependancies ['bridge', 'mpi', 'network', 'core']
**/


//typedef MpTcpSocketBase SocketToBeTested;
//typedef TcpNewReno SocketToBeTested;


/**
Provide basic functions to generate a topology (or outsource them ?)

Control the Number of interfaces per node
**/
class MpTcpTestCase : public TestCase
{
public:

  MpTcpTestCase(std::string name,
                  uint8_t nbClientIfs,
                  uint8_t nbServerIfs,
                  uint8_t nbIpsPerClientIf,
                  uint8_t nbIpsPerServerIf);

  virtual ~MpTcpTestCase ();

//  virtual void AdvertiseAllAddressesOfNode();

protected:
  /**
  \brief Creates a star topology with all ends belonging to either the server or the client
  **/
  virtual void CreateNodes();

//  virtual void SetupDefaultSim (
//      Ptr<Node> n0, Ptr<Node> n1, int nbOfParallelLinks
//      );
//private:
  virtual void DoSetup(void) = 0;
  virtual void DoRun(void) = 0;
  virtual void DoTeardown (void);


  void ServerHandleConnectionCreated (Ptr<Socket> s, const Address & addr);
  void ServerHandleRecv (Ptr<Socket> sock);
  void ServerHandleSend (Ptr<Socket> sock, uint32_t available);
//  void SourceHandleSend (Ptr<Socket> sock, uint32_t available);
//  void SourceHandleRecv (Ptr<Socket> sock);


  uint8_t m_nbInterfacesClient;
  uint8_t m_nbInterfacesServer;
  uint8_t m_nbClientIPsPerNetDevice;
  uint8_t m_nbServerIPsPerNetDevice;

  Ptr<Node> m_client;
  Ptr<MpTcpSocketBase> m_clientSock;
  Ptr<Node> m_server;
  Ptr<MpTcpSocketBase> m_serverSock;
};



void
getAllIpsOfANode(Ptr<Node> node, std::vector<Ipv4Address>& addr )
{
  NS_ASSERT(node);

  // TODO That should go into a helper
  // Object from L3 to access to routing protocol, Interfaces and NetDevices and so on.
  Ptr<Ipv4L3Protocol> ipv4 = node->GetObject<Ipv4L3Protocol>();

  NS_ASSERT(ipv4);

  addr.clear();
  ipv4->GetAllRegisteredIPs(addr);

  /**
  Keep that code in case the patch sent to ns3 is not accepted
  **/
//  for (uint32_t i = 0; i < ipv4->GetNInterfaces(); i++)
//  {
//      //Ptr<NetDevice> device = m_node->GetDevice(i);
//      // Pourrait y avoir plusieurs IPs sur l'interface
//      Ptr<Ipv4Interface> interface = ipv4->GetInterface(i);
//      for( uint32_t j = 0; j < interface->GetNAddresses(); j++ )
//      {
//        Ipv4InterfaceAddress interfaceAddr = interface->GetAddress(j);
//
//
//        // Skip the loop-back
//        if (interfaceAddr.GetLocal() == Ipv4Address::GetLoopback())
//          continue;
//
//
//        addr.push_back( interfaceAddr.GetLocal() );
//      }
//
//
//
//
//  }
}

/**


**/
class PathEventTracker
{
public:
  PathEventTracker();
  ~PathEventTracker() {}


};

PathEventTracker::PathEventTracker()
{
}

/**
can be kept global ? MpTcpTestCase
, std::vector<uint16_t> ports
**/
void AdvertiseAllAddresses(Ptr<MpTcpSocketBase> mptcp)
{
  NS_ASSERT(mptcp);

  Ptr<Node> m_node = mptcp->GetNode();

  // Il peut en avoir plusieurs ?
  if (!mptcp->IsConnected())
  {
    NS_LOG_ERROR("This node has no established MPTCP connection");
    return;
  }

  std::vector<Ipv4Address> addresses;
  getAllIpsOfANode( m_node, addresses);

  for(std::vector<Ipv4Address>::const_iterator i = addresses.begin(); i != addresses.end(); i++ )
  {
    mptcp->GetSubflow(0)->AdvertiseAddress( *i, 0 );
  }


}


//class MpTcpTestCase : public MpTcpTestCase
//{
//public:
//
//}

/**
Number of interfaces per node
**/
class MpTcpAddressTestCase : public MpTcpTestCase
{
public:
  /* Depending on the test */
  typedef enum {
  REM_UNREGISTERED_ADDRESS,
  ADD_THEN_REM_ADDRESS,
  ADD_CREATE_ADDRESS,
  ADD_CREATE_CLOSE
  } TestType;

  MpTcpAddressTestCase(TestType testType,
                    uint8_t nbClientIfs,
                  uint8_t nbServerIfs,
                  uint8_t nbIpsPerClientIf,
                  uint8_t nbIpsPerServerIf);
  virtual ~MpTcpAddressTestCase () {};

  virtual void DoSetup();

//  virtual Ptr<Node> CreateMonohomedNode(uint8_t nbOfInterfaces);
//  virtual Ptr<Node> CreateNode(uint8_t nbOfInterfaces);

  void AdvertiseAllAvailableAddresses();
//  void AdvertiseThenRemoveAllAdvertisedAddresses();
//  void AdvertiseThenCreateRemoveAllAdvertisedAddresses();
//  void CreateNsubflows();
//  void CreateFullmesh();

private:
  virtual void DoRun(void);
  TestType m_testType;
};

MpTcpTestCase::MpTcpTestCase(
                  std::string name,
                  ) :
  TestCase(name)
{
  NS_LOG_LOGIC (this);

}

MpTcpTestCase::~MpTcpTestCase()
{
}


void
MpTcpTestCase::CreateNodes()
{
  Config::SetDefault("ns3::TcpL4Protocol::SocketType",
			             TypeIdValue (MpTcpCCUncoupled::GetTypeId()));
//  Config::SetDefault ("ns3::TcpSocket::SegmentSize", UintegerValue (1000));
//  Config::SetDefault ("ns3::TcpSocket::DelAckCount", UintegerValue (1));
//  Config::SetDefault ("ns3::DropTailQueue::MaxPackets", UintegerValue (20));

}



void
MpTcpTestCase::DoTeardown (void)
{
  Simulator::Destroy ();
}



void
MpTcpAddressTestCase::DoRun (void)
{
  NS_LOG_LOGIC ("Doing run in MpTcpAddressTestCase");


  Simulator::Run();




  NS_LOG_LOGIC("Simulation ended");

//  NS_TEST_EXPECT_MSG_EQ (0, 1, "This test must fail (not ready)");
//  NS_TEST_ASSERT_MSG_EQ (true, true, "true doesn't equal true for some reason");
}



static class MpTcpTestSuite : public TestSuite
{
public:
  MpTcpTestSuite ()
    : TestSuite ("mptcp-pm", UNIT)
  {
//    CommandLine cmd;

    // KESAKO ?
    Packet::EnablePrinting ();  // Enable packet metadata for all test cases
    // Arguments to these test cases are 1) totalStreamSize,
    // 2) source write size, 3) source read size
    // 4) server write size, and 5) server read size
    // with units of bytes
    AddTestCase (new MpTcpAddressTestCase(MpTcpAddressTestCase::REM_UNREGISTERED_ADDRESS,1,1,1,1), TestCase::QUICK);
    AddTestCase (new MpTcpAddressTestCase(MpTcpAddressTestCase::ADD_THEN_REM_ADDRESS  ,1,1,1,1), TestCase::QUICK);

  }

} g_MpTcpPmTestSuite;


