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
 *
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
#include "ns3/string.h"
#include "ns3/tcp-socket-base.h"
#include "ns3/mptcp-socket-base.h"
#include "ns3/mptcp-subflow.h"

// Test to get netanim working
#include "ns3/netanim-module.h"
#include "ns3/constant-position-mobility-model.h"

#include "ns3/ipv4-end-point.h"
#include "ns3/arp-l3-protocol.h"
#include "ns3/ipv4-l3-protocol.h"
#include "ns3/ipv6-l3-protocol.h"
#include "ns3/icmpv4-l4-protocol.h"
#include "ns3/icmpv6-l4-protocol.h"
#include "ns3/udp-l4-protocol.h"
#include "ns3/tcp-l4-protocol.h"
#include "ns3/trace-helper.h"
#include "ns3/point-to-point-helper.h"
#include "ns3/ipv4-address-helper.h"
#include "ns3/sequence-number.h"
#include "ns3/trace-helper.h"
#include "ns3/global-route-manager.h"
#include "ns3/abort.h"
#include "ns3/tcp-trace-helper.h"
// Copy
#include <string>
#include <fstream>
#include "ns3/core-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/internet-module.h"
#include "ns3/applications-module.h"
#include "ns3/network-module.h"
#include "ns3/packet-sink.h"

#include <string>
#include <fstream>

NS_LOG_COMPONENT_DEFINE ("MpTcpExample");

using namespace ns3;

//std::ofstream source_f;
//std::ofstream server_f;

// rename to serverPort
const uint16_t serverPort = 50000;


// Check dce-iperf-mptcp
void setPos (Ptr<Node> n, int x, int y, int z)
{
  Ptr<ConstantPositionMobilityModel> loc = CreateObject<ConstantPositionMobilityModel> ();
  n->AggregateObject (loc);
  Vector locVec2 (x, y, z);
  loc->SetPosition (locVec2);
}


/**
TODO name generation could be moved to tcp-trace-helper ?!
TODO prefixer avec nom de la meta etc...
**/
void
OnNewSocket (Ptr<TcpSocket> socket)
{
  NS_LOG_DEBUG ("New socket incoming");
  
  TcpTraceHelper tcpHelper;
  std::stringstream os;
  //! we start at 1 because it's nicer
  // m_tracePrefix << 
  static int subflowCounter = 0;
  static int metaCounter = 0;

//      std::string filename;
//      if (explicitFilename)
//        {
//          filename = prefix;
//        }
//      else
//        {
//          filename = asciiTraceHelper.GetFilenameFromInterfacePair (prefix, ipv4, interface);
//        }

  // No reason to fail
  Ptr<TcpSocketBase> sock = DynamicCast<TcpSocketBase>(socket);

  //! choose a prefix depending on if it's subflow or meta
  // TODO improve the doc to mark that isChildOf is strict
  if(sock->GetInstanceTypeId().IsChildOf( MpTcpSubflow::GetTypeId()) 
    || sock->GetInstanceTypeId() == MpTcpSubflow::GetTypeId())
  {
    //! TODO prefixer avec le nom de la meta 
    os << Simulator::GetContext() << "-subflow" <<  subflowCounter++;
    tcpHelper.SetupSocketTracing (sock, os.str());
  }
  else if(sock->GetInstanceTypeId().IsChildOf( MpTcpSocketBase::GetTypeId())
      || sock->GetInstanceTypeId() == MpTcpSocketBase::GetTypeId()
      )
  {
    os << Simulator::GetContext() << "-meta" <<  metaCounter++;
    tcpHelper.SetupSocketTracing (sock, os.str());
  }
  else 
  {
    NS_LOG_INFO ("Not mptcp, do nothing: typeid=" << sock->GetInstanceTypeId().GetName ());
  }
}

static const Ipv4Mask g_netmask = Ipv4Mask(0xffffff00);

/**
 *
 */
 #if 0
class MpTcpMultihomedTestCase : public TestCase
{
public:
  MpTcpMultihomedTestCase (uint32_t totalStreamSize,
               uint32_t sourceWriteSize,
               uint32_t sourceReadSize,
               uint32_t serverWriteSize,
               uint32_t serverReadSize,
               uint8_t nb_of_devices,
               uint8_t nb_of_subflows_per_device,
               bool useIpv6);

  void HandleSubflowConnected (Ptr<MpTcpSubflow> subflow);
  void HandleSubflowCreated (Ptr<MpTcpSubflow> subflow);

private:
  virtual void DoSetup (void);
  virtual void DoRun (void);
  virtual void DoTeardown (void);
  void SetupDefaultSim (void);
  void SetupDefaultSim6 (void);

  Ptr<Node> CreateInternetNode (void);
  Ptr<Node> CreateInternetNode6 (void);
  Ptr<SimpleNetDevice> AddSimpleNetDevice (Ptr<Node> node, const char* ipaddr, const char* netmask);
  Ptr<SimpleNetDevice> AddSimpleNetDevice6 (Ptr<Node> node, Ipv6Address ipaddr, Ipv6Prefix prefix);
  void ServerHandleConnectionCreated (Ptr<Socket> s, const Address & addr);
  void ServerHandleRecv (Ptr<Socket> sock);
  void ServerHandleSend (Ptr<Socket> sock, uint32_t available);
  void SourceHandleSend (Ptr<Socket> sock, uint32_t available);
  void SourceHandleRecv (Ptr<Socket> sock);
  void SourceConnectionSuccessful (Ptr<Socket> sock);
  void SourceConnectionFailed (Ptr<Socket> sock);





  uint32_t m_totalBytes;
  uint32_t m_sourceWriteSize;
  uint32_t m_sourceReadSize;
  uint32_t m_serverWriteSize;
  uint32_t m_serverReadSize;
  uint32_t m_currentSourceTxBytes;
  uint32_t m_currentSourceRxBytes;
  uint32_t m_currentServerRxBytes;
  uint32_t m_currentServerTxBytes;
  uint8_t *m_sourceTxPayload;
  uint8_t *m_sourceRxPayload;
  uint8_t* m_serverRxPayload;

  int m_nb_of_successful_connections;
  int m_nb_of_successful_creations;
//  int m_nb_of_subflow_connections;

  bool m_connect_cb_called;
  bool m_useIpv6;

  uint8_t m_number_of_devices;
  uint8_t m_number_of_subflow_per_device;

  Ptr<Node> m_serverNode;
  Ptr<Node> m_sourceNode;
};
#endif



/**
Normally this should be called twice, first when server replied with MPTCP option,
2nd when DataAck was received, i.e., FullyEstablished
**/
void
MpTcpMultihomedTestCase::SourceConnectionSuccessful (Ptr<Socket> sock)
{

  Ptr<MpTcpSocketBase> meta =  DynamicCast<MpTcpSocketBase>(sock);
  NS_ASSERT_MSG(meta, "WTF ?!!");

  NS_LOG_LOGIC("connection successful. Meta state=" << TcpSocket::TcpStateName[meta->GetState() ]
//              << " received a DSS: " << meta->m_receivedDSS
              );


  if (!meta->FullyEstablished ())
  {
    //!
    NS_LOG_INFO ("No DSS received yet => Not fully established. Can't create any subflow");
    return;
  }

  m_nb_of_successful_connections++;
  NS_LOG_INFO("At least one DSS received => fully established. Asking for a new subflow");

//  NS_LOG_WARN("TODO check this is called after the receival of 1st DSS !!!");
  m_connect_cb_called = true;

//   NS_LOG_DEBUG("meta in state " << meta->m_state);

//#if 0
  /*
  first address on 2nd interface
  TODO this should depend on the number of interfaces
  */
  
//    static const int MaxNbOfDevices = 3;
//    static const int SubflowPerDevice = 1;
//  TODO loop over devices
    // GetNDevice
//     GetDevice
  Ptr<Ipv4> ipv4Local = m_sourceNode->GetObject<Ipv4> ();
  NS_ABORT_MSG_UNLESS (ipv4Local, "GetObject for <Ipv4> interface failed");

//      bool isForwarding = false;
      for (uint32_t j = 0; j < ipv4Local->GetNInterfaces (); ++j )
        {
          if (
//          ipv4Local->GetNetDevice (j) == ndLocal && 
          ipv4Local->IsUp (j) &&
              ipv4Local->IsForwarding (j)   // to remove localhost
              ) 
            {
//              isForwarding = true;
              // TODO call to CreateNewSubflow
              int nb_of_subflows_to_create = m_number_of_subflow_per_device;
              // If it's the same interface as master subflow, then remove one
              if (ipv4Local->GetInterfaceForPrefix( "192.168.0.0", g_netmask))
              {
                nb_of_subflows_to_create--;
              }
              
              // create subflows for this device
              for (int i = 0; i < nb_of_subflows_to_create; ++i)
              {
                // Create new subflow
                Ipv4Address serverAddr = m_serverNode->GetObject<Ipv4>()->GetAddress(2,0).GetLocal();
                Ipv4Address sourceAddr = m_sourceNode->GetObject<Ipv4>()->GetAddress(2,0).GetLocal();

                //! TODO, we should be able to not specify a port but it seems buggy so for now, let's set a port
                InetSocketAddress local(sourceAddr, 4420);
                InetSocketAddress remote(serverAddr, serverPort);

                meta->ConnectNewSubflow (local, remote);
              }
            }
        }
        
    #if 0
    uint32_t numDevices = node->GetNDevices ();
    ->
    for (int nb_of_devices = 1; nb_of_devices < MaxNbOfDevices; ++nb_of_devices) {

        // TODO account for master subflow
        for (int subflow_per_device = 1; subflow_per_device < SubflowPerDevice; ++subflow_per_device) {

        }
    }
    // GetAddress (interface, index ) index = 0 as each device has only one IP.
    Ipv4Address serverAddr = m_serverNode->GetObject<Ipv4>()->GetAddress(2,0).GetLocal();
    Ipv4Address sourceAddr = m_sourceNode->GetObject<Ipv4>()->GetAddress(2,0).GetLocal();

    //! TODO, we should be able to not specify a port but it seems buggy so for now, let's set a port
    InetSocketAddress local(sourceAddr, 4420);
    InetSocketAddress remote(serverAddr, serverPort);

    meta->ConnectNewSubflow (local, remote);
    #endif

//#endif

}



void
MpTcpMultihomedTestCase::SourceConnectionFailed(Ptr<Socket> sock)
{

  NS_FATAL_ERROR("Connect failed");
}

void
MpTcpMultihomedTestCase::ServerHandleConnectionCreated (Ptr<Socket> s, const Address & addr)
{
  NS_LOG_DEBUG("ServerHandleConnectionCreated");
  s->SetRecvCallback (MakeCallback (&MpTcpMultihomedTestCase::ServerHandleRecv, this));
  s->SetSendCallback (MakeCallback (&MpTcpMultihomedTestCase::ServerHandleSend, this));

  // TODO setup tracing there !

  Ptr<MpTcpSocketBase> server_meta = DynamicCast<MpTcpSocketBase>(s);
  NS_ASSERT_MSG (server_meta, "Was expecting a meta socket !");
  m_nb_of_successful_creations++;
//  server_meta->SetupMetaTracing("server");
}

#if 0
void
MpTcpMultihomedTestCase::ServerHandleRecv (Ptr<Socket> sock)
{
  NS_LOG_DEBUG("ServerHandleRecv, Rx available [" << sock->GetRxAvailable () << "]");
  while (sock->GetRxAvailable () > 0)
    {

      uint32_t toRead = std::min (m_serverReadSize, sock->GetRxAvailable ());
      NS_LOG_DEBUG("Rx Available [" << toRead );
      Ptr<Packet> p = sock->Recv (toRead, 0);
      if (p == 0 && sock->GetErrno () != Socket::ERROR_NOTERROR)
        {
          NS_FATAL_ERROR ("Server could not read stream at byte " << m_currentServerRxBytes);
        }
      NS_TEST_EXPECT_MSG_EQ ((m_currentServerRxBytes + p->GetSize () <= m_totalBytes), true,
                             "Server received too many bytes");
      NS_LOG_DEBUG ("Server recv data=\"" << GetString (p) << "\"");
      p->CopyData (&m_serverRxPayload[m_currentServerRxBytes], p->GetSize ());
      m_currentServerRxBytes += p->GetSize ();
      ServerHandleSend (sock, sock->GetTxAvailable ());
    }
}

void
MpTcpMultihomedTestCase::ServerHandleSend (Ptr<Socket> sock, uint32_t available)
{
  NS_LOG_DEBUG("ServerHandleSend: TxAvailable=" << available
        << " m_currentServerTxBytes=" << m_currentServerTxBytes
        << " m_currentServerRxBytes=" << m_currentServerRxBytes

        );

  // en fait la seconde condition est zarb : kesako ?
  while (sock->GetTxAvailable () > 0 && m_currentServerTxBytes < m_currentServerRxBytes)
    {
      uint32_t left = m_currentServerRxBytes - m_currentServerTxBytes;
      uint32_t toSend = std::min (left, sock->GetTxAvailable ());
      NS_LOG_DEBUG ("toSend=min(nbBytesLeft=" << left << ",m_serverWriteSize=" << m_serverWriteSize << ")");
      toSend = std::min (toSend, m_serverWriteSize);
      Ptr<Packet> p = Create<Packet> (&m_serverRxPayload[m_currentServerTxBytes], toSend);
      NS_LOG_DEBUG ("Server send data=\"" << GetString (p) << "\"");
      int sent = sock->Send (p);
      NS_TEST_EXPECT_MSG_EQ ((sent != -1), true, "Server error during send ?");
      m_currentServerTxBytes += sent;
    }
  if (m_currentServerTxBytes == m_totalBytes)
    {
      NS_LOG_DEBUG ("Server received all the data. Closing socket.");
      sock->Close ();
    }
}

void
MpTcpMultihomedTestCase::SourceHandleSend (Ptr<Socket> sock, uint32_t available)
{
  NS_LOG_DEBUG("SourceHandleSend with available = " << available
                  << " m_currentSourceTxBytes=" << m_currentSourceTxBytes
                  << " m_totalBytes=" << m_totalBytes
                  );
  while (sock->GetTxAvailable () > 0 && m_currentSourceTxBytes < m_totalBytes)
    {
      uint32_t left = m_totalBytes - m_currentSourceTxBytes;
      uint32_t toSend = std::min (left, sock->GetTxAvailable ());
      toSend = std::min (toSend, m_sourceWriteSize);
      NS_LOG_DEBUG ("toSend=min(nbBytesLeft=" << left << ",sourceWriteSize=" << m_sourceWriteSize << ")");
      Ptr<Packet> p = Create<Packet> (&m_sourceTxPayload[m_currentSourceTxBytes], toSend);
      NS_LOG_DEBUG ("Source send data=\"" << GetString (p) << "\"");
      int sent = sock->Send (p);
      NS_TEST_EXPECT_MSG_EQ ((sent != -1), true, "Error during send ?");
      m_currentSourceTxBytes += sent;
    }
}

void
MpTcpMultihomedTestCase::SourceHandleRecv (Ptr<Socket> sock)
{
  NS_LOG_DEBUG("SourceHandleRecv : m_currentSourceRxBytes=" << m_currentSourceRxBytes);
  while (sock->GetRxAvailable () > 0 && m_currentSourceRxBytes < m_totalBytes)
    {
      uint32_t toRead = std::min (m_sourceReadSize, sock->GetRxAvailable ());
      Ptr<Packet> p = sock->Recv (toRead, 0);
      if (p == 0 && sock->GetErrno () != Socket::ERROR_NOTERROR)
        {
          NS_FATAL_ERROR ("Source could not read stream at byte " << m_currentSourceRxBytes);
        }
      NS_TEST_EXPECT_MSG_EQ ((m_currentSourceRxBytes + p->GetSize () <= m_totalBytes), true,
                             "Source received too many bytes");

      p->CopyData (&m_sourceRxPayload[m_currentSourceRxBytes], p->GetSize ());
      m_currentSourceRxBytes += p->GetSize ();

      NS_LOG_DEBUG ("Source recv data=\"" << GetString (p) << "\". m_currentSourceRxBytes=" << m_currentSourceRxBytes);
    }
  if (m_currentSourceRxBytes == m_totalBytes)
    {
      NS_LOG_DEBUG ("Client received all the data. Closing socket.");
      sock->Close ();
    }
}
#endif


//Callback<bool, const Address &> m_joinRequest
//Callback<void, Ptr<MpTcpSubflow> > m_joinConnectionSucceeded;


/** Here subflow should already have a correct id */
void
MpTcpMultihomedTestCase::HandleSubflowCreated (Ptr<MpTcpSubflow> subflow)
{
  NS_LOG_LOGIC("Created new subflow [" << subflow << "]. Is master: " << subflow->IsMaster());

  static int nb_of_successful_creations = 0;
  if(subflow->IsMaster())
  {
    NS_LOG_LOGIC("successful establishement of first subflow " << subflow);
  }
  else
  {
    //! ce n'est pas le master donc forcement il s'agit d'un join
    NS_LOG_LOGIC ("successful JOIN of subflow " << subflow );
  }

  NS_LOG_LOGIC ( "Subflow id=" << (int)subflow->GetLocalId() );
  NS_LOG_LOGIC ( "Subflow =" << subflow );

  // TODO check it's not called several times for the same sf ?
  nb_of_successful_creations++;
}


/** Here subflow should already have a correct id */
void
MpTcpMultihomedTestCase::HandleSubflowConnected (Ptr<MpTcpSubflow> subflow)
{
  NS_LOG_LOGIC ("successful connection of a subflow");

  static nb_of_successful_connections = 0;
  if (subflow->IsMaster ())
  {
    NS_LOG_LOGIC ("successful establishement of first subflow " << subflow);
  }
  else
  {
    //! ce n'est pas le master donc forcement il s'agit d'un join
    NS_LOG_LOGIC ("successful JOIN of subflow " << subflow );
  }

  nb_of_successful_connections++;
  // TODO check it's not called several times for the same sf ?
  
}





using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("TcpBulkSendExample");

int
main (int argc, char *argv[])
{

  bool tracing = false;
  uint32_t maxBytes = 0;
  int nbOfDevices = 1;

//
// Allow the user to override any of the defaults at
// run-time, via command-line arguments
//
  CommandLine cmd;
  cmd.AddValue ("tracing", "Flag to enable/disable tracing", tracing);
  cmd.AddValue ("sfPerDevices",
                "Total number of devices", nbOfDevices);

  cmd.AddValue ("nbOfDevices",
                "Total number of devices", nbOfDevices);
  cmd.AddValue ("maxBytes",
                "Total number of bytes for application to send", maxBytes);
  cmd.Parse (argc, argv);

//
// Explicitly create the nodes required by the topology (shown above).
//
  NS_LOG_INFO ("Create nodes.");
  NodeContainer nodes;
  nodes.Create (2);

  NS_LOG_INFO ("Create channels.");

//
// Explicitly create the point-to-point link required by the topology (shown above).
//
  PointToPointHelper pointToPoint;
  pointToPoint.SetDeviceAttribute ("DataRate", StringValue ("500Kbps"));
  pointToPoint.SetChannelAttribute ("Delay", StringValue ("5ms"));

  NetDeviceContainer devices;
  devices = pointToPoint.Install (nodes);

//
// Install the internet stack on the nodes
//
  InternetStackHelper internet;
  internet.Install (nodes);

//
// We've got the "hardware" in place.  Now we need to add IP addresses.
//
  NS_LOG_INFO ("Assign IP Addresses.");
  Ipv4AddressHelper ipv4;
  ipv4.SetBase ("10.1.1.0", "255.255.255.0");
  Ipv4InterfaceContainer i = ipv4.Assign (devices);

  
  
  m_serverNode = CreateInternetNode ();
  m_sourceNode = CreateInternetNode ();

  // For netanim
  setPos (m_serverNode, 0,0,0);
  setPos (m_sourceNode, 100,0,0);

  int nbOfDevices = m_number_of_devices;

// TODO ptet essayer avec AddSimpleNetDevice
  for(int i = 0; i < nbOfDevices; ++i)
  {
    // Use 10.0. instead !
    // TODO use SimpleNetDevice instead if you want to upstream !
    std::stringstream netAddr;
    netAddr << "192.168." << i << ".0";

    PointToPointHelper p2p;
    p2p.SetDeviceAttribute ("DataRate", StringValue ("100Mbps"));
    p2p.SetChannelAttribute ("Delay", StringValue ("10ms"));
    p2p.SetChannelAttribute ("AlternateDelay", StringValue ("10ms"));
    NetDeviceContainer cont = p2p.Install(m_serverNode,m_sourceNode);
    p2p.EnablePcapAll("mptcp-multi", true);

    Ipv4AddressHelper ipv4;
    NS_LOG_DEBUG ("setting ipv4 base " << netAddr.str());
    ipv4.SetBase ( netAddr.str().c_str(), g_netmask);
    ipv4.Assign(cont);
    /// Added by matt for debugging purposes

    //PointToPointHelper helper;
    //helper.EnablePcapAll("test",true);
    //helper.EnablePcapAll("testmptcp",false);

  }
  //pcap.EnablePcapInternal("mptcp",dev,true,true);

  // TODO addition
  GlobalRouteManager::BuildGlobalRoutingDatabase ();
  GlobalRouteManager::InitializeRoutes ();

  Ptr<SocketFactory> sockFactory0 = m_serverNode->GetObject<TcpSocketFactory> ();
  Ptr<SocketFactory> sockFactory1 = m_sourceNode->GetObject<TcpSocketFactory> ();

  // a wrapper that calls m_tcp->CreateSocket ();
  Ptr<Socket> server = sockFactory0->CreateSocket ();
  Ptr<Socket> source = sockFactory1->CreateSocket ();

  // TODO this should fail :/
//  Ptr<MpTcpSocketBase> server_meta = DynamicCast<MpTcpSocketBase>(server);
//  Ptr<MpTcpSocketBase> source_meta = DynamicCast<MpTcpSocketBase>(source);




  /* We want to control over which socket the meta binds first
   GetAddress(interface, noIp);
   sachant que l'interface 0 est le loopback en fait
  */
  Ipv4Address serverMainAddr = m_serverNode->GetObject<Ipv4>()->GetAddress(1,0).GetLocal();
  InetSocketAddress serverlocaladdr (serverMainAddr, serverPort);
  InetSocketAddress serverremoteaddr (serverMainAddr, serverPort);
  NS_LOG_DEBUG("serverMainAddr=" << serverlocaladdr);
  server->Bind (serverlocaladdr);
  server->Listen ();
  server->SetAcceptCallback (MakeNullCallback<bool, Ptr< Socket >, const Address &> (),
                             MakeCallback (&MpTcpMultihomedTestCase::ServerHandleConnectionCreated,this));

//  NS_LOG_UNCOND("Server Meta:" << server_meta << " NodeId:" << server_meta->GetNode()->GetId());
//  NS_LOG_UNCOND("Client Meta:" << source_meta << " NodeId:" << source_meta->GetNode()->GetId());
//  NS_LOG_INFO( "test" << server);
  source->SetRecvCallback (MakeCallback (&MpTcpMultihomedTestCase::SourceHandleRecv, this));
  source->SetSendCallback (MakeCallback (&MpTcpMultihomedTestCase::SourceHandleSend, this));

  // SetConnectCallback
  source->SetConnectCallback (
//    Callback< void, Ptr< Socket > > connectionSucceeded, Callback< void, Ptr< Socket > > connectionFailed
    MakeCallback (&MpTcpMultihomedTestCase::SourceConnectionSuccessful, this),
    MakeCallback (&MpTcpMultihomedTestCase::SourceConnectionFailed, this)

    );
  source->Connect (serverremoteaddr);


  NS_LOG_INFO ("Create Applications.");

//
// Create a BulkSendApplication and install it on node 0
//
  uint16_t port = 9;  // well-known echo port number


  BulkSendHelper source ("ns3::TcpSocketFactory",
                         InetSocketAddress (i.GetAddress (1), port));
  // Set the amount of data to send in bytes.  Zero is unlimited.
  source.SetAttribute ("MaxBytes", UintegerValue (maxBytes));
  ApplicationContainer sourceApps = source.Install (nodes.Get (0));
  sourceApps.Start (Seconds (0.0));
  sourceApps.Stop (Seconds (10.0));

//
// Create a PacketSinkApplication and install it on node 1
//
  PacketSinkHelper sink ("ns3::TcpSocketFactory",
                         InetSocketAddress (Ipv4Address::GetAny (), port));
  ApplicationContainer sinkApps = sink.Install (nodes.Get (1));
  sinkApps.Start (Seconds (0.0));
  sinkApps.Stop (Seconds (10.0));

//
// Set up tracing if enabled
//
  if (tracing)
    {
      AsciiTraceHelper ascii;
      pointToPoint.EnableAsciiAll (ascii.CreateFileStream ("tcp-bulk-send.tr"));
      pointToPoint.EnablePcapAll ("tcp-bulk-send", false);
    }

//
// Now, do the actual simulation.
//
  NS_LOG_INFO ("Run Simulation.");

  AnimationInterface anim ("animation.xml");
  Simulator::Stop (Seconds (10.0));
  Simulator::Run ();
  Simulator::Destroy ();
  NS_LOG_INFO ("Done.");

  Ptr<PacketSink> sink1 = DynamicCast<PacketSink> (sinkApps.Get (0));
  std::cout << "Total Bytes Received: " << sink1->GetTotalRx () << std::endl;
}
