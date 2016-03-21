/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2015 University of Sussex
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
 * Author:  Matthieu Coudron <matthieu.coudron@lip6.fr>
 *          Morteza Kheirkhah <m.kheirkhah@sussex.ac.uk>
 */
#ifndef MPTCP_ID_MANAGER_H
#define MPTCP_ID_MANAGER_H

#include "ns3/callback.h"
//#include "ns3/mptcp-mapping.h"
//#include "ns3/tcp-socket-base.h"
//#include "ns3/mp-tcp-path-manager.h"
//#include "ns3/gnuplot.h"
//#include "mp-tcp-subflow.h"

#include "ns3/object.h"
#include "ns3/address.h"
#include "ns3/inet-socket-address.h"
#include <map>
#include <vector>


namespace ns3
{


/**
* TODO setup callbacks in order to know if we shall accept the addition
* \brief The MPTCP path manager tracks ADD_ADDR/REM_ADDR in case the user wants to later open new subflows.
* It is possible to use callbacks from the MPTCP metasocket to be notified in case of a new ADD_ADDR.
* Thus it is possible to immediately create the desired subflows
*
RFC6824
   The Address IDs of the subflow used in the initial SYN exchange of
   the first subflow in the connection are implicit, and have the value
   zero.  A host MUST store the mappings between Address IDs and
   addresses both for itself and the remote host.  An implementation
   will also need to know which local and remote Address IDs are
   associated with which established subflows, for when addresses are
   removed from a local or remote host.

* There should be a testsuite to
* \class MpTcpPathIdManager
* TODO tempalte


Id identifies only one endpoint (

**/
class MpTcpPathIdManager
  : public Object
{

public:
  static TypeId GetTypeId();
  //const bool generate_uid
  MpTcpPathIdManager ();

  virtual ~MpTcpPathIdManager() {};

  /**
  Will generate an appropriate ID
  (InetSocketAddress addr
  **/
//  virtual uint8_t GetIdForLocalAddr( Ipv4Address address );
//  virtual bool RemoveLocalId(Ipv4Address addr);

  /**
  \param addresses
  \warning Don't forget to clear the vector first !
  **/
//  GetAllAdvertisedDestinations (std::vector<Address>& addresses) = 0;
//  virtual void
//  GetRegisteredIds (std::vector<Address>& addresses) = 0;

  // TODO move callbacks here + local address Mgmt ?

  /**
   * Find closest match
   *
   * \return true if found a match
   * \todo should be const
   */
  virtual bool GetMatch (uint8_t *result, const Address& address ) = 0;

  /**
  Can force the ID with which to register
  //    const Ipv4Address& address, uint16_t port = 0
  
   */
  virtual bool
  AddId (uint8_t addrId, const Address& address) = 0;

  virtual bool
  AddLocalId (uint8_t *addrId, const Address& address) = 0;

  /**
   * del/rem
   *
   */
  virtual bool RemoveId (uint8_t addrId) = 0;
  
/**
   * Called when closing the subflow
   */
//  virtual bool
//  RemoveLocalId (uint8_t addrId) = 0;

  
//  virtual uint8_t
//  GetLocalAddrId (const InetSocketAddress& address) = 0;
//  RegisterLocalAddress()

  
  /**
   * Called when meta receives a REMOVE_ADDRESS. It just contians ids then.
   */
//  virtual bool
//  RemoveLocalId(uint8_t addrId) = 0;
//
//protected:
//  const bool m_localPM;
};



}



#endif // MPTCP_ID_MANAGER_H
