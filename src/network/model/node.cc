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
#include "ns3/clock-perfect.h"
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
    .SetParent<Object> ()
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
//    .AddAttribute("SchedulerType",
//                  "The object class to use as the scheduler implementation",
//                  TypeIdValue (MapScheduler::GetTypeId ()),
//                  MakeTypeIdAccessor()
//                  MakeTypeIdChecker ());
//                  )
;
  return tid;
}

Node::Node()
  : m_id (0),
    m_sid (0),
    m_localUid(0)
//  ,  m_clock(0)
//    m_currentActiveEventsArray(0)
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

  {
    ObjectFactory factory;
//    StringValue s;
//    GetAttribute( s);
//    factory.SetTypeId ();
    factory.SetTypeId (MapScheduler::GetTypeId ());
    this->SetScheduler (factory);
  }

  // should it be aggregated by default or not ?
  // In all cases
  // When one get aggregated, the node should register a few callbacks
  Ptr<Clock> clock = CreateObject<ClockPerfect>();
  AggregateObject(clock);
//  clock->m_last

//  TimeStepCallback
  // TODO do it evertyime a clock is aggregated

//  clock->SetTimeStepCallback( MakeBoundCallback(Node::RefreshEvents, this) );


}

void
Node::SetClock (Ptr<Clock> clock)
{
  NS_LOG_FUNCTION (this);

  //!
  if (clock != 0)
    {
        //!
      m_clock = clock;
      m_clock->SetFrequencyChangeCallback(MakeCallback(&Node::RefreshEvents, Ptr<Node> (this)));
    }
}

void
Node::NotifyNewAggregate ()
{
  NS_LOG_FUNCTION (this);
  if (m_clock == 0)
    {
      Ptr<Clock> clock = this->GetObject<Clock> ();
      //verify that it's a valid node and that
      //the node was not set before
//      if (clock != 0)
//        {
          this->SetClock (clock);

//        }
    }

  Object::NotifyNewAggregate ();
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
Node::GetLocalTime(void) const
{
  NS_LOG_FUNCTION (this);
  Ptr<Clock> clock = GetObject<Clock>();
  if(clock) {
    return clock->GetTime();
  }
  return Simulator::Now();
//  return m_clock;
}


EventId
Node::GetNextEvent() const
{
    return m_nextEvent.first;
}

EventId
Node::GetNextEventSim() const
{
    return m_nextEvent.second;
}

/*
Used to update absolute time of
, EventImpl *event
TODO add overload for
*/
void
//Node::SwapNextEvent(Event newNextEvent)
Node::SwapNextEvent(

// Mm un eventId
                    EventId localEvent
//                    EventImpl* newNextEvent
                    )
{
  NS_LOG_DEBUG(&localEvent);

  Ptr<ClockPerfect> clock = GetObject<ClockPerfect>();
  // Do the conversion eventSimTime <<
  Time eventSimTime;
//    Time eventSimTime;
  NS_ASSERT(clock->LocalTimeToAbsTime( Time(localEvent.GetTs()), eventSimTime) );

  // if nextEvent is replaced by a sooner one, then we need to remove it from scheduled
  if(GetNextEvent().IsRunning())
  {
    // TODO check that it does not destory the EventImpl* ?
    // otherwise one would need to play with refCount or CopyObject
    Simulator::Cancel( GetNextEventSim() );

//    eventSimTime- Schedule::Now(),
  }

  EventId simEventId = Simulator::Schedule( eventSimTime - Simulator::Now(), localEvent.PeekEventImpl());
  m_nextEvent = std::make_pair(localEvent, simEventId);
}

// On frequency change
//UpdateNextEvent()

//PushEventToSimulator()
//{
//
//}

//
EventId
Node::DoSchedule (Time const &timeOffset, EventImpl *event)
{
  NS_LOG_FUNCTION (this << timeOffset.GetTimeStep () << event);
//  NS_ASSERT_MSG (SystemThread::Equals (m_main), "Simulator::Schedule Thread-unsafe invocation!");


  Time localTime = GetLocalTime();
  Time eventLocalTime = localTime + timeOffset;


//  NS_ASSERT (eventLocalTime.IsPositive ());
//  NS_ASSERT (eventLocalTime >= localTime);

  // In all cases I insert the event
  Scheduler::Event newEvent; // rename into newEvent
  newEvent.impl = event;
  newEvent.key.m_ts = (uint64_t) eventLocalTime.GetTimeStep ();
  newEvent.key.m_context = this->GetId();
//  ev.key.m_uid = Simulator::GetImplementation()->GetFreeUid();
  newEvent.key.m_uid = m_localUid;
  m_localUid++;
//  m_unscheduledEvents++;
  m_events->Insert (newEvent);
  EventId nodeEventId( event, newEvent.key.m_ts, newEvent.key.m_context, newEvent.key.m_uid);


/* TODO quand on l'insère on doit se demander si :
  - il y a un nextEvent (isRunning ?
  - si oui comparer les clés Scheduler::EventKey pour maj ou pas le m_next
  */
  // Hack
  if(GetNextEvent().IsRunning())
  {


     // if the newly scheduled event should be scheduled before
     if(nodeEventId.GetTs() < GetNextEvent().GetTs()){
//     if(nodeEventId < GetNextEvent()){
        // TODO we should swap next events
        SwapNextEvent( nodeEventId);
        return nodeEventId;
     }
  }
  // if no valid nextEvent, we setup the new one
  else
  {
    // Fix that
    SwapNextEvent( nodeEventId);
//    EventId simEventId = Simulator::Schedule(eventSimTime - Simulator::Now, event);
//    std::make_pair(nodeEventId,simEventId);
  }

  // otherwise
//  Event = m_events->PeekNext()


  return nodeEventId;
}


EventId
Node::ScheduleNow (EventImpl *event)
{
    return DoSchedule(Time(0), event);
}

void
Node::Cancel (const EventId &id)
{
    //
//    NS_FATAL_ERROR("Not implemented yet");
    // If already in simulator list
    if(id == GetNextEvent()) {

        // not sure that's good
        id.PeekEventImpl()->Cancel();
        Simulator::Cancel(m_nextEvent.second);
        Scheduler::Event newEvent = m_events->PeekNext();
//        EventId nodeNext,
        EventId nodeEventId( newEvent.impl, newEvent.key.m_ts, newEvent.key.m_context, newEvent.key.m_uid);

        SwapNextEvent(nodeEventId);
        // et la on doit insérer le prochain pour le remplacer
    }
    else {

      // Version de DefaultSimulator
      // ====
//      if (!IsExpired (id))
// TODO may need some more checks
      if (!id.PeekEventImpl ()->IsCancelled ())
        {
          id.PeekEventImpl ()->Cancel ();
        }
        // ====
//        m_events
    }
}

void
Node::RefreshEvents(double oldFreq, double newFreq)
{
    NS_LOG_DEBUG("Should now refresh event expiration times " << oldFreq << "/" << newFreq);
//    Loop through all events whose context is this node ID
#if 0
    int nextArray = (m_currentActiveEventsArray  + 1) %2;

    // Look for all events belonging to this node
    // And add a matching offset
    for(std::list<EventId>::iterator i = m_events[ m_currentActiveEventsArray ].begin();
        i != m_events[ m_currentActiveEventsArray ].end();
        ++i
    )
    {
        //! TODO en fait on ne le détruit pas mais on le cancel !
//        Simulator::Cancel(*i);
        i->Cancel();
//        Time absTimeElapsed = i->GetTs() - Simulator::Now();


        Simulator::Schedule()
        m_events[ nextArray ].push_back(event);
    }
#endif
}


//Time
//Node::GetTrueTime(void) const
//{
//  NS_LOG_FUNCTION (this);
//
//}

//void
//Node::SetClock(Ptr<Clock> clock)
//{
//  //!
//  NS_LOG_FUNCTION (this << clock);
//  NS_ASSERT(clock);
//
//  m_clock = clock;
//}


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
