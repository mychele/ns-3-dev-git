/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2006 Georgia Tech Research Corporation, INRIA
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
 * Authors: George F. Riley<riley@ece.gatech.edu>
 *          Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 */
#include "node.h"
#include "node-list.h"
#include "net-device.h"
#include "application.h"
#include "ns3/packet.h"
#include "ns3/simulator.h"
#include "ns3/object-vector.h"
#include "ns3/uinteger.h"
#include "ns3/log.h"
#include "ns3/assert.h"
#include "ns3/global-value.h"
#include "ns3/boolean.h"
#include "ns3/simulator.h"
#include "ns3/clock.h"
#include "ns3/clock-perfect.h" // Could be removed
#include "ns3/nstime.h"
#include "ns3/callback.h"
#include "ns3/scheduler.h"
#include "ns3/object-factory.h"
#include "ns3/map-scheduler.h"
#include "ns3/simulator-impl.h"
#include "ns3/scheduler.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("Node");

NS_OBJECT_ENSURE_REGISTERED (Node);

/**
 * \brief A global switch to enable all checksums for all protocols.
 */
static GlobalValue g_checksumEnabled  = GlobalValue ("ChecksumEnabled",
                                                     "A global switch to enable all checksums for all protocols",
                                                     BooleanValue (false),
                                                     MakeBooleanChecker ());


TypeId
Node::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::Node")
    .SetParent<SimulatorImpl> ()
    .SetGroupName("Network")
    .AddConstructor<Node> ()
    .AddAttribute ("DeviceList", "The list of devices associated to this Node.",
                   ObjectVectorValue (),
                   MakeObjectVectorAccessor (&Node::m_devices),
                   MakeObjectVectorChecker<NetDevice> ())
    .AddAttribute ("ApplicationList", "The list of applications associated to this Node.",
                   ObjectVectorValue (),
                   MakeObjectVectorAccessor (&Node::m_applications),
                   MakeObjectVectorChecker<Application> ())
    .AddAttribute ("Id", "The id (unique integer) of this Node.",
                   TypeId::ATTR_GET, // allow only getting it.
                   UintegerValue (0),
                   MakeUintegerAccessor (&Node::m_id),
                   MakeUintegerChecker<uint32_t> ())
    .AddAttribute ("SystemId", "The systemId of this node: a unique integer used for parallel simulations.",
                   TypeId::ATTR_GET || TypeId::ATTR_SET,
                   UintegerValue (0),
                   MakeUintegerAccessor (&Node::m_sid),
                   MakeUintegerChecker<uint32_t> ())
    // TODO Might remove the global one and make it instance dependant ?
    // SetScheduler
    .AddAttribute("SchedulerType",
                  "The object class to use as the scheduler implementation",
                  TypeIdValue (MapScheduler::GetTypeId ()),
                  MakeTypeIdAccessor ( (void (Node::*)(TypeId ))&Node::SetScheduler),
                  MakeTypeIdChecker ())    
//    .AddAttribute("ClockType",
//                  "The type of clock",
//                  TypeIdValue (ClockPerfect::GetTypeId ()),
//                  MakeTypeIdAccessor(&Node::m_clockTypeId),
//                  MakeTypeIdChecker ())
;
  return tid;
}

Node::Node()
  : m_id (0),
    m_sid (0),
    m_localUid(0)
{
  NS_LOG_FUNCTION (this);
  Construct ();
}

Node::Node(uint32_t sid)
  : m_id (0),
    m_sid (sid)
{ 
  NS_LOG_FUNCTION (this << sid);
  Construct ();
}

void
Node::Construct (void)
{
  NS_LOG_FUNCTION (this);
  m_id = NodeList::Add (this);

  ObjectFactory factory;
  factory.SetTypeId (ClockPerfect::GetTypeId());
  SetClock (factory.Create<Clock> ());
}

// Look at simulator.cc GetImpl
Ptr<Clock>
Node::GetClock () const 
{
  return m_clock;
}

void
Node::SetClock (Ptr<Clock> clock)
{
  NS_LOG_FUNCTION (this);
  NS_ASSERT (clock);

  //! if there is an existing clock, set offset & Reschedule currently scheduled event
  if (m_clock)
  {
    clock->SetTime (m_clock->GetTime());
    // Cancel currently scheduled event if any
    if (GetNextEventSim ().IsRunning ())
    {
      GetNextEventSim ().Cancel ();
    }

    SyncNodeWithMainSimulator ();
  }
  else 
  {
    clock->SetTime (Simulator::Now());
  }

  m_clock = clock;
  m_clock->SetSimulatorSyncCallback (MakeCallback(&Node::SyncNodeWithMainSimulator, Ptr<Node> (this)));
}


void
Node::SetScheduler (TypeId schedulerType)
{
  NS_LOG_FUNCTION (this << schedulerType);
  ObjectFactory schedulerFactory;
  schedulerFactory.SetTypeId(schedulerType);
  SetScheduler (schedulerFactory);
}

void
Node::SetScheduler (ObjectFactory schedulerFactory)
{
  NS_LOG_FUNCTION (this << schedulerFactory);

  Ptr<Scheduler> scheduler = schedulerFactory.Create<Scheduler> ();

  if (m_events != 0)
    {
      while (!m_events->IsEmpty ())
        {
          Scheduler::Event next = m_events->RemoveNext ();
          scheduler->Insert (next);
        }
    }
  m_events = scheduler;
}


Node::~Node ()
{
  NS_LOG_FUNCTION (this);
}

uint32_t
Node::GetId (void) const
{
  NS_LOG_FUNCTION (this);
  return m_id;
}

Time
Node::GetLocalTime (void) const
{
  NS_LOG_FUNCTION (this);
  if (m_clock)
  {
    return m_clock->GetTime();
  }
  return Simulator::Now();
}

EventId
Node::GetNextEvent () const
{
    return m_nextEvent.first;
}

EventId
Node::GetNextEventSim () const
{
    return m_nextEvent.second;
}

/**
it must check if there is an event already scheduled in main:
- if there isn't, it calls Simulator::Schedule
- If there is:
   > if the new event happens before then:
   cancel the matching EventId in Simulator
   > Simulator::Schedule
- otherwise

Also the simulator should
 */
void
Node::SyncNodeWithMainSimulator ()
{
  NS_LOG_FUNCTION_NOARGS ();

  if (m_events->IsEmpty ()) 
  {
    NS_LOG_LOGIC ( "No event in queue. Stop here." );
    return;
  }

  // caller takes ownership of the returned pointer ?
  Scheduler::Event nextEvent = m_events->PeekNext ();
  
  EventId nodeEventId (nextEvent.impl, nextEvent.key.m_ts, nextEvent.key.m_context, nextEvent.key.m_uid);
  bool enqueueNextEvent = false;

  if (GetNextEventSim ().IsRunning ())
  {
    NS_LOG_DEBUG ( "An event is already scheduled. at local Ts=" << GetNextEvent ().GetTs () );
    
    // if the newly scheduled event should be scheduled before
     if (nodeEventId.GetTs () < GetNextEvent ().GetTs ())
     {
        NS_LOG_LOGIC ( "Replace current first event with a new one." );
        // we should cancel the running one
        GetNextEventSim ().Cancel ();
        enqueueNextEvent = true;
     }
     else
     {
        NS_LOG_DEBUG ( "Currently scheduled happens first. Do nothing." );
     }
  }
  else 
  {
    NS_LOG_DEBUG ( "No node event running in Simulator yet" );
    enqueueNextEvent = true;
  }

  // if we are not the next event=, abort here,
  if (!enqueueNextEvent) 
  {
    NS_LOG_DEBUG ( "Don't queue first" );
    return;
  }

  ForceLocalEventIntoSimulator (nodeEventId);
}


void 
Node::ForceLocalEventIntoSimulator (EventId nodeEventId)
{
  NS_LOG_FUNCTION_NOARGS ();

  Time eventSimTime;

  bool res = m_clock->LocalTimeToSimulatorTime ( Time (nodeEventId.GetTs ()), &eventSimTime);
  NS_ASSERT_MSG ( res, "Could not compute timelapse" );

  NS_LOG_DEBUG ( "Enqueuing event to Simulator in " << eventSimTime - Simulator::Now() );
  EventId simEventId = Simulator::ScheduleWithContext ( nodeEventId.GetContext(),
                          eventSimTime - Simulator::Now(), 
                          &Node::ExecOnNode, this
                    );

  m_nextEvent = std::make_pair (nodeEventId, simEventId);
}

void 
Node::ExecOnNode ()
{
  NS_LOG_FUNCTION ( Simulator::Now() );

  Scheduler::Event next = m_events->RemoveNext();
  NS_ASSERT (next.key.m_ts >= Simulator::Now());
  NS_LOG_DEBUG (next.key.m_uid);

  next.impl->Invoke ();
  next.impl->Unref ();
  // TODO mark event as finisehd ?
  // Now that it's finished, check if we need to add another one
  SyncNodeWithMainSimulator ();
}

EventId 
Node::Schedule (Time const &timeOffset, EventImpl *event) 
{
    return DoSchedule( timeOffset, event);
}

EventId
Node::ScheduleWithContext (uint32_t context, Time const &time, EventImpl *event)
{
    NS_FATAL_ERROR ("not implemented");
}

EventId
Node::DoSchedule (Time const &timeOffset, EventImpl *event)
{
  NS_LOG_FUNCTION (this << timeOffset.GetTimeStep () << event);

  Time localTime = GetLocalTime ();
  Time eventLocalTime = localTime + timeOffset;

  NS_ASSERT (eventLocalTime.IsPositive ());

  // In all cases I insert the event
  Scheduler::Event newEvent; // rename into newEvent
  newEvent.impl = event;
  NS_LOG_DEBUG ( "DoSchedule at localtime=" << eventLocalTime);
  newEvent.key.m_ts = (uint64_t) eventLocalTime.GetTimeStep ();
  newEvent.key.m_context = this->GetId ();
  newEvent.key.m_uid = m_localUid;
  m_localUid++;

  m_events->Insert (newEvent);

  EventId nodeEventId (event, newEvent.key.m_ts, newEvent.key.m_context, newEvent.key.m_uid);
  nodeEventId.m_node = this;

   SyncNodeWithMainSimulator ();

  return nodeEventId;
}


EventId
Node::ScheduleNow (EventImpl *event)
{
    return DoSchedule (Time (0), event);
}


void
Node::Remove (const EventId &id)
{
    //!
    NS_LOG_WARN ("implemented as a cancel");
    Cancel (id);
}

bool
Node::IsExpired (const EventId &id) const
{
  NS_LOG_FUNCTION_NOARGS ();

  Time localTs = GetLocalTime();
  if (id.PeekEventImpl () == 0 ||
      id.GetTs () < localTs ||
      (id.GetTs () == localTs && id.GetUid () <= m_localUid) ||
      id.PeekEventImpl ()->IsCancelled ()
      )
    {
      return true;
    }
  else
  {
    return false;
  }
}

void
Node::Cancel (const EventId &localId)
{
  NS_LOG_FUNCTION (localId.GetUid());

  if (!IsExpired (localId))
    {
      NS_LOG_LOGIC ("Cancelling the implementation");
      localId.PeekEventImpl ()->Cancel ();
    }

  SyncNodeWithMainSimulator ();
}

uint32_t 
Node::GetContext (void) const
{
  NS_LOG_WARN ("stub");
  return 0;
}

void 
Node::Run (void)
{
  NS_FATAL_ERROR("Not implemented");
}

Time 
Node::Now (void) const
{
  return GetLocalTime();
}

bool 
Node::IsFinished (void) const
{
  NS_FATAL_ERROR("Not implemented");
}
void 
Node::Stop (void)
{
  NS_FATAL_ERROR("Not implemented");
}
void 
Node::Stop (Time const &time)
{
  NS_FATAL_ERROR("Not implemented");
}

Time 
Node::GetMaximumSimulationTime (void) const
{
  NS_FATAL_ERROR("Not implemented");
}
  
Time 
Node::GetDelayLeft (const EventId &id) const
{
  NS_FATAL_ERROR("Not implemented");
}

EventId 
Node::ScheduleDestroy (EventImpl *event)
{
  NS_FATAL_ERROR("Not implemented");
}

void 
Node::Destroy ()
{
  NS_FATAL_ERROR("Not implemented");
}
  
uint32_t
Node::GetSystemId (void) const
{
  NS_LOG_FUNCTION (this);
  return m_sid;
}

uint32_t
Node::AddDevice (Ptr<NetDevice> device)
{
  NS_LOG_FUNCTION (this << device);
  uint32_t index = m_devices.size ();
  m_devices.push_back (device);
  device->SetNode (this);
  device->SetIfIndex (index);
  device->SetReceiveCallback (MakeCallback (&Node::NonPromiscReceiveFromDevice, this));
  Simulator::ScheduleWithContext (GetId (), Seconds (0.0), 
                                  &NetDevice::Initialize, device);
  NotifyDeviceAdded (device);
  return index;
}
Ptr<NetDevice>
Node::GetDevice (uint32_t index) const
{
  NS_LOG_FUNCTION (this << index);
  NS_ASSERT_MSG (index < m_devices.size (), "Device index " << index <<
                 " is out of range (only have " << m_devices.size () << " devices).");
  return m_devices[index];
}
uint32_t 
Node::GetNDevices (void) const
{
  NS_LOG_FUNCTION (this);
  return m_devices.size ();
}

uint32_t 
Node::AddApplication (Ptr<Application> application)
{
  NS_LOG_FUNCTION (this << application);
  uint32_t index = m_applications.size ();
  m_applications.push_back (application);
  application->SetNode (this);
  Simulator::ScheduleWithContext (GetId (), Seconds (0.0), 
                                  &Application::Initialize, application);
  return index;
}
Ptr<Application> 
Node::GetApplication (uint32_t index) const
{
  NS_LOG_FUNCTION (this << index);
  NS_ASSERT_MSG (index < m_applications.size (), "Application index " << index <<
                 " is out of range (only have " << m_applications.size () << " applications).");
  return m_applications[index];
}
uint32_t 
Node::GetNApplications (void) const
{
  NS_LOG_FUNCTION (this);
  return m_applications.size ();
}

void 
Node::DoDispose ()
{
  NS_LOG_FUNCTION (this);
  m_deviceAdditionListeners.clear ();
  m_handlers.clear ();
  for (std::vector<Ptr<NetDevice> >::iterator i = m_devices.begin ();
       i != m_devices.end (); i++)
    {
      Ptr<NetDevice> device = *i;
      device->Dispose ();
      *i = 0;
    }
  m_devices.clear ();
  for (std::vector<Ptr<Application> >::iterator i = m_applications.begin ();
       i != m_applications.end (); i++)
    {
      Ptr<Application> application = *i;
      application->Dispose ();
      *i = 0;
    }
  m_applications.clear ();
  Object::DoDispose ();
}
void 
Node::DoInitialize (void)
{
  NS_LOG_FUNCTION (this);
  for (std::vector<Ptr<NetDevice> >::iterator i = m_devices.begin ();
       i != m_devices.end (); i++)
    {
      Ptr<NetDevice> device = *i;
      device->Initialize ();
    }
  for (std::vector<Ptr<Application> >::iterator i = m_applications.begin ();
       i != m_applications.end (); i++)
    {
      Ptr<Application> application = *i;
      application->Initialize ();
    }

  Object::DoInitialize ();
}

void
Node::RegisterProtocolHandler (ProtocolHandler handler, 
                               uint16_t protocolType,
                               Ptr<NetDevice> device,
                               bool promiscuous)
{
  NS_LOG_FUNCTION (this << &handler << protocolType << device << promiscuous);
  struct Node::ProtocolHandlerEntry entry;
  entry.handler = handler;
  entry.protocol = protocolType;
  entry.device = device;
  entry.promiscuous = promiscuous;

  // On demand enable promiscuous mode in netdevices
  if (promiscuous)
    {
      if (device == 0)
        {
          for (std::vector<Ptr<NetDevice> >::iterator i = m_devices.begin ();
               i != m_devices.end (); i++)
            {
              Ptr<NetDevice> dev = *i;
              dev->SetPromiscReceiveCallback (MakeCallback (&Node::PromiscReceiveFromDevice, this));
            }
        }
      else
        {
          device->SetPromiscReceiveCallback (MakeCallback (&Node::PromiscReceiveFromDevice, this));
        }
    }

  m_handlers.push_back (entry);
}

void
Node::UnregisterProtocolHandler (ProtocolHandler handler)
{
  NS_LOG_FUNCTION (this << &handler);
  for (ProtocolHandlerList::iterator i = m_handlers.begin ();
       i != m_handlers.end (); i++)
    {
      if (i->handler.IsEqual (handler))
        {
          m_handlers.erase (i);
          break;
        }
    }
}

bool
Node::ChecksumEnabled (void)
{
  NS_LOG_FUNCTION_NOARGS ();
  BooleanValue val;
  g_checksumEnabled.GetValue (val);
  return val.Get ();
}

bool
Node::PromiscReceiveFromDevice (Ptr<NetDevice> device, Ptr<const Packet> packet, uint16_t protocol,
                                const Address &from, const Address &to, NetDevice::PacketType packetType)
{
  NS_LOG_FUNCTION (this << device << packet << protocol << &from << &to << packetType);
  return ReceiveFromDevice (device, packet, protocol, from, to, packetType, true);
}

bool
Node::NonPromiscReceiveFromDevice (Ptr<NetDevice> device, Ptr<const Packet> packet, uint16_t protocol,
                                   const Address &from)
{
  NS_LOG_FUNCTION (this << device << packet << protocol << &from);
  return ReceiveFromDevice (device, packet, protocol, from, device->GetAddress (), NetDevice::PacketType (0), false);
}

bool
Node::ReceiveFromDevice (Ptr<NetDevice> device, Ptr<const Packet> packet, uint16_t protocol,
                         const Address &from, const Address &to, NetDevice::PacketType packetType, bool promiscuous)
{
  NS_LOG_FUNCTION (this << device << packet << protocol << &from << &to << packetType << promiscuous);
  NS_ASSERT_MSG (Simulator::GetContext () == GetId (), "Received packet with erroneous context ; " <<
                 "make sure the channels in use are correctly updating events context " <<
                 "when transfering events from one node to another.");
  NS_LOG_DEBUG ("Node " << GetId () << " ReceiveFromDevice:  dev "
                        << device->GetIfIndex () << " (type=" << device->GetInstanceTypeId ().GetName ()
                        << ") Packet UID " << packet->GetUid ());
  bool found = false;

  for (ProtocolHandlerList::iterator i = m_handlers.begin ();
       i != m_handlers.end (); i++)
    {
      if (i->device == 0 ||
          (i->device != 0 && i->device == device))
        {
          if (i->protocol == 0 || 
              i->protocol == protocol)
            {
              if (promiscuous == i->promiscuous)
                {
                  i->handler (device, packet, protocol, from, to, packetType);
                  found = true;
                }
            }
        }
    }
  return found;
}
void 
Node::RegisterDeviceAdditionListener (DeviceAdditionListener listener)
{
  NS_LOG_FUNCTION (this << &listener);
  m_deviceAdditionListeners.push_back (listener);
  // and, then, notify the new listener about all existing devices.
  for (std::vector<Ptr<NetDevice> >::const_iterator i = m_devices.begin ();
       i != m_devices.end (); ++i)
    {
      listener (*i);
    }
}
void 
Node::UnregisterDeviceAdditionListener (DeviceAdditionListener listener)
{
  NS_LOG_FUNCTION (this << &listener);
  for (DeviceAdditionListenerList::iterator i = m_deviceAdditionListeners.begin ();
       i != m_deviceAdditionListeners.end (); i++)
    {
      if ((*i).IsEqual (listener))
        {
          m_deviceAdditionListeners.erase (i);
          break;
         }
    }
}
 
void 
Node::NotifyDeviceAdded (Ptr<NetDevice> device)
{
  NS_LOG_FUNCTION (this << device);
  for (DeviceAdditionListenerList::iterator i = m_deviceAdditionListeners.begin ();
       i != m_deviceAdditionListeners.end (); i++)
    {
      (*i) (device);
    }  
}
 

} // namespace ns3
