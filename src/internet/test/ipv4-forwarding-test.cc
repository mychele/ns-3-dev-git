/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2013 Universita' di Firenze
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
 * Author: Tommaso Pecorella <tommaso.pecorella@unifi.it>
 */

#include "ns3/test.h"
#include "ns3/socket-factory.h"
#include "ns3/udp-socket-factory.h"
#include "ns3/tcp-socket-factory.h"
#include "ns3/simulator.h"
#include "ns3/simple-channel.h"
#include "ns3/simple-net-device.h"
#include "ns3/drop-tail-queue.h"
#include "ns3/socket.h"
#include "ns3/boolean.h"

#include "ns3/log.h"
#include "ns3/node.h"
#include "ns3/inet-socket-address.h"

#include "ns3/arp-l3-protocol.h"
#include "ns3/ipv4-l3-protocol.h"
#include "ns3/icmpv4-l4-protocol.h"
#include "ns3/udp-l4-protocol.h"
#include "ns3/tcp-l4-protocol.h"
#include "ns3/ipv4-static-routing.h"
#include "ns3/tcp-socket.h"
#include "ns3/ipv4-address.h"

#include <string>
#include <limits>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("Ipv4ForwardingTest");

static void
AddInternetStack (Ptr<Node> node)
{
  //ARP
  Ptr<ArpL3Protocol> arp = CreateObject<ArpL3Protocol> ();
  node->AggregateObject (arp);
  //IPV4
  Ptr<Ipv4L3Protocol> ipv4 = CreateObject<Ipv4L3Protocol> ();
  //Routing for Ipv4
  Ptr<Ipv4StaticRouting> ipv4Routing = CreateObject<Ipv4StaticRouting> ();
  ipv4->SetRoutingProtocol (ipv4Routing);
  node->AggregateObject (ipv4);
  node->AggregateObject (ipv4Routing);
  //ICMP
  Ptr<Icmpv4L4Protocol> icmp = CreateObject<Icmpv4L4Protocol> ();
  node->AggregateObject (icmp);
  //UDP
  Ptr<UdpL4Protocol> udp = CreateObject<UdpL4Protocol> ();
  node->AggregateObject (udp);
  //UDP
  Ptr<TcpL4Protocol> tcp = CreateObject<TcpL4Protocol> ();
  node->AggregateObject (tcp);
}


class Ipv4ForwardingTest : public TestCase
{
  Ptr<Packet> m_receivedPacket;
  void DoSendData (Ptr<Socket> socket, std::string to);
  void SendData (Ptr<Socket> socket, std::string to);

public:
  virtual void DoRun (void);
  Ipv4ForwardingTest ();

  void ReceivePkt (Ptr<Socket> socket);
};

Ipv4ForwardingTest::Ipv4ForwardingTest ()
  : TestCase ("UDP socket implementation") 
{
}

void Ipv4ForwardingTest::ReceivePkt (Ptr<Socket> socket)
{
  uint32_t availableData;
  availableData = socket->GetRxAvailable ();
  m_receivedPacket = socket->Recv (std::numeric_limits<uint32_t>::max (), 0);
  NS_ASSERT (availableData == m_receivedPacket->GetSize ());
}

void
Ipv4ForwardingTest::DoSendData (Ptr<Socket> socket, std::string to)
{
  Address realTo = InetSocketAddress (Ipv4Address (to.c_str ()), 1234);
  NS_TEST_EXPECT_MSG_EQ (socket->SendTo (Create<Packet> (123), 0, realTo),
                         123, "100");
}

void
Ipv4ForwardingTest::SendData (Ptr<Socket> socket, std::string to)
{
  Simulator::ScheduleWithContext (socket->GetNode ()->GetId (), Seconds (0),
                                  &Ipv4ForwardingTest::DoSendData, this, socket, to);
  Simulator::Run ();
}

void
Ipv4ForwardingTest::DoRun (void)
{
  // Create topology

  // Receiver Node
  Ptr<Node> rxNode = CreateObject<Node> ();
  AddInternetStack (rxNode);
  Ptr<SimpleNetDevice> rxDev;
  { // first interface
    rxDev = CreateObject<SimpleNetDevice> ();
    rxDev->SetAddress (Mac48Address::ConvertFrom (Mac48Address::Allocate ()));
    rxNode->AddDevice (rxDev);
    Ptr<Ipv4> ipv4 = rxNode->GetObject<Ipv4> ();
    uint32_t netdev_idx = ipv4->AddInterface (rxDev);
    Ipv4InterfaceAddress ipv4Addr = Ipv4InterfaceAddress (Ipv4Address ("10.0.0.2"), Ipv4Mask (0xffff0000U));
    ipv4->AddAddress (netdev_idx, ipv4Addr);
    ipv4->SetUp (netdev_idx);
  }

  // Forwarding Node
  Ptr<Node> fwNode = CreateObject<Node> ();
  AddInternetStack (fwNode);
  Ptr<SimpleNetDevice> fwDev1, fwDev2;
  { // first interface
    fwDev1 = CreateObject<SimpleNetDevice> ();
    fwDev1->SetAddress (Mac48Address::ConvertFrom (Mac48Address::Allocate ()));
    fwNode->AddDevice (fwDev1);
    Ptr<Ipv4> ipv4 = fwNode->GetObject<Ipv4> ();
    uint32_t netdev_idx = ipv4->AddInterface (fwDev1);
    Ipv4InterfaceAddress ipv4Addr = Ipv4InterfaceAddress (Ipv4Address ("10.0.0.1"), Ipv4Mask (0xffff0000U));
    ipv4->AddAddress (netdev_idx, ipv4Addr);
    ipv4->SetUp (netdev_idx);
  }

  { // second interface
    fwDev2 = CreateObject<SimpleNetDevice> ();
    fwDev2->SetAddress (Mac48Address::ConvertFrom (Mac48Address::Allocate ()));
    fwNode->AddDevice (fwDev2);
    Ptr<Ipv4> ipv4 = fwNode->GetObject<Ipv4> ();
    uint32_t netdev_idx = ipv4->AddInterface (fwDev2);
    Ipv4InterfaceAddress ipv4Addr = Ipv4InterfaceAddress (Ipv4Address ("10.1.0.1"), Ipv4Mask (0xffff0000U));
    ipv4->AddAddress (netdev_idx, ipv4Addr);
    ipv4->SetUp (netdev_idx);
  }

  // Sender Node
  Ptr<Node> txNode = CreateObject<Node> ();
  AddInternetStack (txNode);
  Ptr<SimpleNetDevice> txDev;
  {
    txDev = CreateObject<SimpleNetDevice> ();
    txDev->SetAddress (Mac48Address::ConvertFrom (Mac48Address::Allocate ()));
    txNode->AddDevice (txDev);
    Ptr<Ipv4> ipv4 = txNode->GetObject<Ipv4> ();
    uint32_t netdev_idx = ipv4->AddInterface (txDev);
    Ipv4InterfaceAddress ipv4Addr = Ipv4InterfaceAddress (Ipv4Address ("10.1.0.2"), Ipv4Mask (0xffff0000U));
    ipv4->AddAddress (netdev_idx, ipv4Addr);
    ipv4->SetUp (netdev_idx);
    Ptr<Ipv4StaticRouting> ipv4StaticRouting = txNode->GetObject<Ipv4StaticRouting> ();
    ipv4StaticRouting->SetDefaultRoute(Ipv4Address("10.1.0.1"), netdev_idx);
  }

  // link the two nodes
  Ptr<SimpleChannel> channel1 = CreateObject<SimpleChannel> ();
  rxDev->SetChannel (channel1);
  fwDev1->SetChannel (channel1);

  Ptr<SimpleChannel> channel2 = CreateObject<SimpleChannel> ();
  fwDev2->SetChannel (channel2);
  txDev->SetChannel (channel2);

  // Create the UDP sockets
  Ptr<SocketFactory> udpRxSocketFactory = rxNode->GetObject<UdpSocketFactory> ();
  Ptr<Socket> udpRxSocket = udpRxSocketFactory->CreateSocket ();
  NS_TEST_EXPECT_MSG_EQ (udpRxSocket->Bind (InetSocketAddress (Ipv4Address ("10.0.0.2"), 1234)), 0, "trivial");
  udpRxSocket->SetRecvCallback (MakeCallback (&Ipv4ForwardingTest::ReceivePkt, this));

  Ptr<SocketFactory> udpTxSocketFactory = txNode->GetObject<UdpSocketFactory> ();
  Ptr<Socket> udpTxSocket = udpTxSocketFactory->CreateSocket ();
  udpTxSocket->SetAllowBroadcast (true);

  // ------ Now the tests ------------

  // Unicast test
  SendData (udpTxSocket, "10.0.0.2");
  NS_TEST_EXPECT_MSG_EQ (m_receivedPacket->GetSize (), 123, "IPv4 Forwarding on");

  m_receivedPacket->RemoveAllByteTags ();
  m_receivedPacket = 0;

  Ptr<Ipv4> ipv4 = fwNode->GetObject<Ipv4> ();
  ipv4->SetAttribute("IpForward", BooleanValue (false));
  SendData (udpTxSocket, "10.0.0.2");
  NS_TEST_EXPECT_MSG_EQ (m_receivedPacket, 0, "IPv4 Forwarding off; no received packet");

  Simulator::Destroy ();

}

class Ipv4BindToNetDeviceTest : public TestCase
{
  Ptr<Packet> m_receivedPacket;
  void DoSendData (Ptr<Socket> socket, std::string to, int expectedReturnValue);
  void SendData (Ptr<Socket> socket, std::string to, int expectedReturnValue);
  //, std::string to, int expectedReturnValue
  void OnConnection (Ptr<Socket> socket
//                     , const ns3::Address&
                     );

public:
  virtual void DoRun (void);
  Ipv4BindToNetDeviceTest ();

  void ReceivePkt (Ptr<Socket> socket);
};

Ipv4BindToNetDeviceTest::Ipv4BindToNetDeviceTest ()
  : TestCase ("Test that Socket::BindToNetDevice () works")
{
}

void Ipv4BindToNetDeviceTest::ReceivePkt (Ptr<Socket> socket)
{
  NS_LOG_DEBUG ("Recv cb");
  uint32_t availableData;
  availableData = socket->GetRxAvailable ();
  m_receivedPacket = socket->Recv (std::numeric_limits<uint32_t>::max (), 0);
  NS_ASSERT (availableData == m_receivedPacket->GetSize ());
}

void
Ipv4BindToNetDeviceTest::DoSendData (Ptr<Socket> socket, std::string to, int expectedReturnValue)
{
  Address realTo = InetSocketAddress (Ipv4Address (to.c_str ()), 1234);
  NS_TEST_EXPECT_MSG_EQ (socket->SendTo (Create<Packet> (123), 0, realTo),
                         expectedReturnValue, "SendTo failure");
}

void
Ipv4BindToNetDeviceTest::SendData (Ptr<Socket> socket, std::string to, int expectedReturnValue)
{
  Simulator::ScheduleWithContext (socket->GetNode ()->GetId (), Seconds (0),
                                  &Ipv4BindToNetDeviceTest::DoSendData, this, socket, to, expectedReturnValue);
}


void
Ipv4BindToNetDeviceTest::OnConnection (Ptr<Socket> socket
//                                       , const ns3::Address&
                                       )
{
    NS_LOG_DEBUG ("Send connection");
    int ret = socket->Send(Create<Packet> (123));
    NS_TEST_EXPECT_MSG_EQ ( ret, 0, "Could not send packet");
}

void
Ipv4BindToNetDeviceTest::DoRun (void)
{
  /** Create topology
  ________________________________________
  |      txNode                           |
  | txDev1 (10.0.0.1) | txDev2 (11.0.0.1) |
  |___________________|___________________|
          |                    |
          |                    |
          |                    |
          |                    |
  ________________________________________
  |      rxNode                           |
  | rxDev1 (10.0.0.2) | rxDev2 (11.0.0.2) |
  |___________________|___________________|
  **/
  Ipv4Address receiverIP[2], senderIP[2];
  receiverIP[0] = Ipv4Address ("10.0.0.2");
  receiverIP[1] = Ipv4Address ("11.0.0.2");

  senderIP[0] = Ipv4Address ("10.0.0.1");
  senderIP[1] = Ipv4Address ("11.0.0.1");

  // Receiver Node
  Ptr<Node> rxNode = CreateObject<Node> ();
  AddInternetStack (rxNode);
  Ptr<SimpleNetDevice> rxDev1;
  Ptr<SimpleNetDevice> rxDev2;
  { // first interface
    rxDev1 = CreateObject<SimpleNetDevice> ();
    rxDev1->SetAddress (Mac48Address::ConvertFrom (Mac48Address::Allocate ()));
    rxNode->AddDevice (rxDev1);
    Ptr<Ipv4> ipv4 = rxNode->GetObject<Ipv4> ();
    uint32_t netdev_idx = ipv4->AddInterface (rxDev1);
    Ipv4InterfaceAddress ipv4Addr = Ipv4InterfaceAddress ( receiverIP[0], Ipv4Mask (0xffff0000U));
    ipv4->AddAddress (netdev_idx, ipv4Addr);
    ipv4->SetUp (netdev_idx);
  }
  { // second interface
    rxDev2 = CreateObject<SimpleNetDevice> ();
    rxDev2->SetAddress (Mac48Address::ConvertFrom (Mac48Address::Allocate ()));
    rxNode->AddDevice (rxDev2);
    Ptr<Ipv4> ipv4 = rxNode->GetObject<Ipv4> ();
    uint32_t netdev_idx = ipv4->AddInterface (rxDev2);
    Ipv4InterfaceAddress ipv4Addr = Ipv4InterfaceAddress (receiverIP[1], Ipv4Mask (0xffff0000U));
    ipv4->AddAddress (netdev_idx, ipv4Addr);
    ipv4->SetUp (netdev_idx);
  }

  // Sender Node
  Ptr<Node> txNode = CreateObject<Node> ();
  AddInternetStack (txNode);
  Ptr<SimpleNetDevice> txDev1;
  Ptr<SimpleNetDevice> txDev2;
  {
    // first interface
    txDev1 = CreateObject<SimpleNetDevice> ();
    txDev1->SetAddress (Mac48Address::ConvertFrom (Mac48Address::Allocate ()));
    txNode->AddDevice (txDev1);
    Ptr<Ipv4> ipv4 = txNode->GetObject<Ipv4> ();
    uint32_t netdev_idx = ipv4->AddInterface (txDev1);
    Ipv4InterfaceAddress ipv4Addr = Ipv4InterfaceAddress (senderIP[0], Ipv4Mask (0xffff0000U));
    ipv4->AddAddress (netdev_idx, ipv4Addr);
    ipv4->SetUp (netdev_idx);
  }
  {
    // second interface
    txDev2 = CreateObject<SimpleNetDevice> ();
    txDev2->SetAddress (Mac48Address::ConvertFrom (Mac48Address::Allocate ()));
    txNode->AddDevice (txDev2);
    Ptr<Ipv4> ipv4 = txNode->GetObject<Ipv4> ();
    uint32_t netdev_idx = ipv4->AddInterface (txDev2);
    Ipv4InterfaceAddress ipv4Addr = Ipv4InterfaceAddress (senderIP[1], Ipv4Mask (0xffff0000U));
    ipv4->AddAddress (netdev_idx, ipv4Addr);
    ipv4->SetUp (netdev_idx);
  }

  // link the two nodes
  Ptr<SimpleChannel> channel1 = CreateObject<SimpleChannel> ();
  rxDev1->SetChannel (channel1);
  txDev1->SetChannel (channel1);

  Ptr<SimpleChannel> channel2 = CreateObject<SimpleChannel> ();
  rxDev2->SetChannel (channel2);
  txDev2->SetChannel (channel2);

  // Create the UDP sockets
  Ptr<SocketFactory> udpRxSocketFactory = rxNode->GetObject<UdpSocketFactory> ();
  Ptr<Socket> udpRxSocket = udpRxSocketFactory->CreateSocket ();
  NS_TEST_EXPECT_MSG_EQ (udpRxSocket->Bind (InetSocketAddress ( receiverIP[0], 1234)), 0, "Successful call to Bind.");
  // Must call BindToNetDevice() after Bind()
  udpRxSocket->SetRecvCallback (MakeCallback (&Ipv4BindToNetDeviceTest::ReceivePkt, this));

  Ptr<SocketFactory> udpTxSocketFactory = txNode->GetObject<UdpSocketFactory> ();
  Ptr<Socket> udpTxSocket = udpTxSocketFactory->CreateSocket ();
  udpTxSocket->SetAllowBroadcast (true);

  // Create the TCP sockets
  Ptr<SocketFactory> tcpRxSocketFactory = rxNode->GetObject<TcpSocketFactory> ();
  Ptr<Socket> tcpRxSocket = tcpRxSocketFactory->CreateSocket ();
  NS_TEST_EXPECT_MSG_EQ (tcpRxSocket->Bind (InetSocketAddress (receiverIP[0], 1234)), 0, "Successful call to Bind.");
  // Must call BindToNetDevice() after Bind()
  tcpRxSocket->SetRecvCallback (MakeCallback (&Ipv4BindToNetDeviceTest::ReceivePkt, this));

  Ptr<SocketFactory> tcpTxSocketFactory = txNode->GetObject<UdpSocketFactory> ();
//  Ptr<TcpSocket> tcpTxSocket = DynamicCast<TcpSocket>(tcpTxSocketFactory->CreateSocket () );
  Ptr<Socket> tcpTxSocket = tcpTxSocketFactory->CreateSocket ();
//  tcpTxSocket->SetAllowBroadcast (true);

  // ------ Now the UDP tests ------------

  // Test that data is successful when RxNode binds to rxDev1 and TxNode binds
  // to txDev1
  NS_LOG_DEBUG ("Bind test case 1");
  udpRxSocket->BindToNetDevice (rxDev1);
  udpTxSocket->BindToNetDevice (txDev1);
  SendData (udpTxSocket, "10.0.0.2", 123);  // 123 is the expected Send() return value
  Simulator::Run ();
  NS_TEST_EXPECT_MSG_NE (m_receivedPacket, 0, "Incorrectly bound NetDevices");
//  NS_TEST_EXPECT_MSG_EQ (m_receivedPacket->GetSize (), 123, "Correctly bound NetDevices");
  m_receivedPacket = 0;

  // All three other bind combinations should fail 
  
  NS_LOG_DEBUG ("Bind test case 2");
  udpTxSocket = udpTxSocketFactory->CreateSocket ();
  udpRxSocket->BindToNetDevice (rxDev1);
  udpTxSocket->BindToNetDevice (txDev2);
  SendData (udpTxSocket, "10.0.0.2", -1);
  Simulator::Run ();
  NS_TEST_EXPECT_MSG_EQ (m_receivedPacket, 0, "No received packet");
  m_receivedPacket = 0;

  NS_LOG_DEBUG ("Bind test case 3");
  udpTxSocket = udpTxSocketFactory->CreateSocket ();
  udpRxSocket->BindToNetDevice (rxDev2);
  udpTxSocket->BindToNetDevice (txDev2);
  SendData (udpTxSocket, "10.0.0.2", -1);
  Simulator::Run ();
  NS_TEST_EXPECT_MSG_EQ (m_receivedPacket, 0, "No received packet");
  m_receivedPacket = 0;

  NS_LOG_DEBUG ("Bind test case 4");
  udpTxSocket = udpTxSocketFactory->CreateSocket ();
  udpRxSocket->BindToNetDevice (rxDev2);
  udpTxSocket->BindToNetDevice (txDev1);
  SendData (udpTxSocket, "10.0.0.2", 123);
  Simulator::Run ();
  NS_TEST_EXPECT_MSG_EQ (m_receivedPacket, 0, "No received packet");
  m_receivedPacket = 0;

  // ------ Now the TCP tests ------------
  
  // Test that data is successful when RxNode binds to rxDev1 and TxNode binds
  // to txDev1
  NS_LOG_DEBUG ("Bind test case 1");
  InetSocketAddress serverAddr (receiverIP[1], 1234 );
  InetSocketAddress senderAddr (senderIP[1],   4321 );

//  tcpRxSocket->BindToNetDevice (rxDev1);
//  tcpTxSocket->BindToNetDevice (txDev1);
  // MATT bind must be automatic, without any custom call to BindToNetDevice
  NS_TEST_EXPECT_MSG_EQ (tcpTxSocket->Bind (senderAddr), "" );
//  tcpRxSocket->SetAcceptCallback( MakeNullCallback<bool, Ptr<Socket>, const ns3::Address& > (),
//                                 MakeCallback(&Ipv4BindToNetDeviceTest::OnConnection, this) );

  tcpTxSocket->SetConnectCallback ( MakeCallback(&Ipv4BindToNetDeviceTest::OnConnection, this),
                                   MakeNullCallback<void, Ptr<Socket> > ()
                                  );

  tcpTxSocket->Connect( serverAddr );

  //! Make sure packet can only travel through channel2, which should be the logical path
  channel1->BlackList(txDev1, rxDev1);

//  SendData (tcpTxSocket, "10.0.0.2", 123);  // 123 is the expected Send() return value

  Simulator::Run ();
  NS_TEST_EXPECT_MSG_NE (m_receivedPacket, 0, "A packet should have been received");
//  NS_TEST_EXPECT_MSG_EQ (m_receivedPacket->GetSize (), 123, "Correctly bound NetDevices");
  m_receivedPacket = 0;

  // All three other bind combinations should fail 
  
//  NS_LOG_DEBUG ("Bind test case 2");
//  tcpTxSocket = tcpTxSocketFactory->CreateSocket ();
//  tcpRxSocket->BindToNetDevice (rxDev1);
//  tcpTxSocket->BindToNetDevice (txDev2);
//  SendData (tcpTxSocket, "10.0.0.2", -1);
//  Simulator::Run ();
//  NS_TEST_EXPECT_MSG_EQ (m_receivedPacket, 0, "No received packet");
//
//  NS_LOG_DEBUG ("Bind test case 3");
//  tcpTxSocket = tcpTxSocketFactory->CreateSocket ();
//  tcpRxSocket->BindToNetDevice (rxDev2);
//  tcpTxSocket->BindToNetDevice (txDev2);
//  SendData (tcpTxSocket, "10.0.0.2", -1);
//  Simulator::Run ();
//  NS_TEST_EXPECT_MSG_EQ (m_receivedPacket, 0, "No received packet");
//
//  NS_LOG_DEBUG ("Bind test case 4");
//  tcpTxSocket = tcpTxSocketFactory->CreateSocket ();
//  tcpRxSocket->BindToNetDevice (rxDev2);
//  tcpTxSocket->BindToNetDevice (txDev1);
//  SendData (tcpTxSocket, "10.0.0.2", 123);
//  Simulator::Run ();
//  NS_TEST_EXPECT_MSG_EQ (m_receivedPacket, 0, "No received packet");

  
  Simulator::Destroy ();

}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
class Ipv4ForwardingTestSuite : public TestSuite
{
public:
  Ipv4ForwardingTestSuite () : TestSuite ("ipv4-forwarding", UNIT)
  {
    AddTestCase (new Ipv4ForwardingTest, TestCase::QUICK);
    AddTestCase (new Ipv4BindToNetDeviceTest, TestCase::QUICK);
  }
} g_ipv4forwardingTestSuite;
