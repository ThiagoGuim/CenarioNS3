/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2020 Federal University of Juiz de Fora (UFJF)
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
 * Author: Luciano Chaves <luciano.chaves@ice.ufjf.br>
 */

#ifndef UDP_PEER_HELPER_H
#define UDP_PEER_HELPER_H

#include <ns3/core-module.h>
#include <ns3/network-module.h>
#include <ns3/internet-module.h>
#include "udp-peer-app.h"

namespace ns3 {

/**
 * This helper will make life easier for setting up UDP peer applications.
 */
class UdpPeerHelper
{
public:
  UdpPeerHelper ();     //!< Default constructor.

  /**
   * Record an attribute to be set in each peer application.
   * \param name the name of the attribute to set.
   * \param value the value of the attribute to set.
   */
  void SetPeerAttribute (std::string name, const AttributeValue &value);

  /**
   * Record an attribute to be set in the first application.
   * \param name the name of the attribute to set.
   * \param value the value of the attribute to set.
   */
  void Set1stAttribute (std::string name, const AttributeValue &value);

  /**
   * Record an attribute to be set in the second application.
   * \param name the name of the attribute to set.
   * \param value the value of the attribute to set.
   */
  void Set2ndAttribute (std::string name, const AttributeValue &value);

  /**
   * Create a pair of peer applications on input nodes.
   * \param node1st The node to install the first app.
   * \param node2nd The node to install the second app.
   * \param addr1st The IPv4 address of the first node.
   * \param addr2nd The IPv4 address of the second mpde.
   * \param port1st The port number of the first node.
   * \param port2nd The port number of the second node.
   * \param dscp The DSCP value used to set the socket Type of Service field.
   * \return The container with the pair of applications created.
   */
  ApplicationContainer Install (
    Ptr<Node> node1st, Ptr<Node> node2nd, Ipv4Address addr1st,
    Ipv4Address addr2nd, uint16_t port1st, uint16_t port2nd,
    Ipv4Header::DscpType dscp = Ipv4Header::DscpDefault);

private:
  ObjectFactory m_app1stFactory;   //!< Application factory.
  ObjectFactory m_app2ndFactory;   //!< Application factory.
};

} // namespace ns3
#endif /* UDP_PEER_HELPER_H */
