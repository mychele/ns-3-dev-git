/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2007 Georgia Tech Research Corporation
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
#include "sll-header.h"
#include "ns3/log.h"


namespace ns3
{

NS_LOG_COMPONENT_DEFINE ("SllHeader");

NS_OBJECT_ENSURE_REGISTERED (SllHeader);

SllHeader::SllHeader() :
    m_packetType(0),
    m_arphdType(0),
    m_addressLength(0),
    m_address(0),
    m_protocolType(0)
{
    NS_LOG_FUNCTION(this);
}

SllHeader::~SllHeader()
{
    NS_LOG_FUNCTION(this);
}


TypeId
SllHeader::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::SllHeader")
    .SetParent<Header> ()
    .SetGroupName ("Internet")
    .AddConstructor<SllHeader> ()
  ;
  return tid;
}

TypeId
SllHeader::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}




uint16_t
SllHeader::GetArpType() const
{
    return m_arphdType;
}


void
SllHeader::Print (std::ostream &os)  const
{
  os << "SLL header";

//  os<<" Seq="<<m_sequenceNumber<<" Ack="<<m_ackNumber<<" Win="<<m_windowSize;
//
//  TcpOptionList::const_iterator op;
//
//  for (op = m_options.begin (); op != m_options.end (); ++op)
//    {
//      os << " " << (*op)->GetInstanceTypeId ().GetName () << "(";
//      (*op)->Print (os);
//      os << ")";
//    }
}

uint32_t
SllHeader::GetSerializedSize (void)  const
{
  return 2 + 2 + 2 + 8 + 2;
}

void
SllHeader::Serialize (Buffer::Iterator start)  const
{
  Buffer::Iterator i = start;
  i.WriteHtonU16 (m_packetType);
  i.WriteHtonU16 (m_arphdType);
  i.WriteHtonU16 (m_addressLength);
  i.WriteHtonU64 (m_address);
  i.WriteHtonU16 (m_protocolType);

//  i.WriteHtonU32 (m_sequenceNumber.GetValue ());
//  i.WriteHtonU32 (m_ackNumber.GetValue ());
//  i.WriteHtonU16 (GetLength () << 12 | m_flags); //reserved bits are all zero
//  i.WriteHtonU16 (m_windowSize);
//  i.WriteHtonU16 (0);
//  i.WriteHtonU16 (m_urgentPointer);

}

uint32_t
SllHeader::Deserialize (Buffer::Iterator start)
{
  Buffer::Iterator i = start;
  m_packetType = i.ReadNtohU16 ();
  m_arphdType = i.ReadNtohU16 ();
  m_addressLength = i.ReadNtohU16 ();
  m_address = i.ReadNtohU64 ();
  m_protocolType = i.ReadNtohU16 ();
//  uint16_t field = i.ReadNtohU16 ();
//  m_flags = field & 0x3F;
//  m_length = field>>12;
//  m_windowSize = i.ReadNtohU16 ();
//  i.Next (2);
//  m_urgentPointer = i.ReadNtohU16 ();

  // Deserialize options if they exist
//  m_options.clear ();
//  uint32_t optionLen = (m_length - 5) * 4;
//  if (optionLen > 40)
//    {
//      NS_LOG_ERROR ("Illegal TCP option length " << optionLen << "; options discarded");
//      return 20;
//    }
//
//
//  if (m_length != CalculateHeaderLength ())
//    {
//      NS_LOG_ERROR ("Mismatch between calculated length and in-header value");
//    }

  return GetSerializedSize ();
}


}
