/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2016 University of Campinas (Unicamp)
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
 * Author: Luciano Chaves <luciano@lrc.ic.unicamp.br>
 */

#ifndef MY_APPLICATION_HELPER_H
#define MY_APPLICATION_HELPER_H

#include <ns3/core-module.h>
#include <ns3/network-module.h>
#include <ns3/internet-module.h>
#include "my-udp-client.h"
#include "my-udp-server.h"

namespace ns3 {

/**
 * \ingroup svelteApps
 * This helper will make life easier for people trying to set up client/server
 * applications on the SVELTE architecture.
 */
class MyApplicationHelper
{
public:
  MyApplicationHelper ();     //!< Default constructor.

  /**
   * Complete constructor.
   * \param clientType The TypeId of client application class.
   * \param serverType The TypeId of server application class.
   */
  MyApplicationHelper (TypeId clientType, TypeId serverType);

  /**
   * Record an attribute to be set in each client application.
   * \param name the name of the attribute to set.
   * \param value the value of the attribute to set.
   */
  void SetClientAttribute (std::string name, const AttributeValue &value);

  /**
   * Record an attribute to be set in each server application.
   * \param name the name of the attribute to set.
   * \param value the value of the attribute to set.
   */
  void SetServerAttribute (std::string name, const AttributeValue &value);

  /**
   * Create a pair of client + server applications on input nodes.
   * \param clientNode The node to install the client app.
   * \param serverNode The node to install the server app.
   * \param clientAddr The IPv4 address of the client.
   * \param serverAddr The IPv4 address of the server.
   * \param clientPort The port number of the client.
   * \param serverPort The port number of the server.
   * \param dscp The DSCP value used to set the socket type of service field.
   * \return The client application created.
   */
  Ptr<MyUdpClient> Install (
    Ptr<Node> clientNode, Ptr<Node> serverNode, Ipv4Address clientAddr,
    Ipv4Address serverAddr, uint16_t clientPort, uint16_t serverPort,
    Ipv4Header::DscpType dscp = Ipv4Header::DscpDefault);

private:
  ObjectFactory m_clientFactory; //!< Object client factory.
  ObjectFactory m_serverFactory; //!< Object server factory.
};

} // namespace ns3
#endif /* MY_APPLICATION_HELPER_H */
