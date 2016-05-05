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
 */
#ifndef MPTCP_SUBFLOW_OWD_H
#define MPTCP_SUBFLOW_OWD_H

#include <stdint.h>
#include <vector>
#include <queue>
#include <list>
#include <set>
#include <map>
#include "ns3/mptcp-subflow.h"
//#include "ns3/tcp-option-mptcp.h"

using namespace std;

namespace ns3
{

class MpTcpSocketBase;
class MpTcpPathIdManager;
class TcpOptionMpTcpDSS;
class TcpOptionMpTcp;

/**
 * \class MpTcpSubflow
*/
class MpTcpSubflowOwd : public MpTcpSubflow
{
public:

  static TypeId
  GetTypeId(void);

  virtual TypeId GetInstanceTypeId(void) const;


  /**
  the metasocket is the socket the application is talking to.
  Every subflow is linked to that socket.
  \param The metasocket it is linked to
  **/
  MpTcpSubflowOwd ();

  virtual ~MpTcpSubflowOwd ();



  virtual int ProcessOptionMpTcp (const Ptr<const TcpOption> option);

};

}
#endif /* MP_TCP_SUBFLOW */
