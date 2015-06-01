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
#ifndef SLL_HEADER_H
#define SLL_HEADER_H

#include "ns3/buffer.h"
#include "ns3/header.h"
#include <stdint.h>

namespace ns3 {

/**
 * \ingroup packet
 *
 * \brief Protocol header serialization and deserialization.
 *
http://www.tcpdump.org/linktypes/LINKTYPE_LINUX_SLL.html


+---------------------------+
|         Packet type       |
|         (2 Octets)        |
+---------------------------+
|        ARPHRD_ type       |
|         (2 Octets)        |
+---------------------------+
| Link-layer address length |
|         (2 Octets)        |
+---------------------------+
|    Link-layer address     |
|         (8 Octets)        |
+---------------------------+
|        Protocol type      |
|         (2 Octets)        |
+---------------------------+
|           Payload         |
.                           .
.                           .
.                           .

 */
class SllHeader : public Header
{
public:
  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);
  TypeId GetInstanceTypeId (void) const;


  SllHeader();
  virtual ~SllHeader ();

  /**
   *  The ARPHRD_ type field is in network byte order; it contains a Linux ARPHRD_ value for the link-layer device type.
   *
   */
  uint16_t GetArpType() const;

  //! Inherited
  virtual uint32_t GetSerializedSize (void) const;
  virtual void Serialize (Buffer::Iterator start) const;
  virtual uint32_t Deserialize (Buffer::Iterator start);

  virtual void Print (std::ostream &os) const;


//protected:


/*
The packet type field is in network byte order (big-endian);
it contains a value that is one of:

    0, if the packet was specifically sent to us by somebody else;
    1, if the packet was broadcast by somebody else;
    2, if the packet was multicast, but not broadcast, by somebody else;
    3, if the packet was sent to somebody else by somebody else;
    4, if the packet was sent by us.
*/

    // declared in packet order
    uint16_t m_packetType;
    uint16_t m_arphdType;
    uint16_t m_addressLength;
    uint64_t m_address;
    uint16_t m_protocolType;



private:
};



std::ostream & operator << (std::ostream &os, const SllHeader &header);

} // namespace ns3

#endif /* SLL_HEADER_H */

