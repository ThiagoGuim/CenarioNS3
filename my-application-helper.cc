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

#include "my-application-helper.h"

namespace ns3 {

MyApplicationHelper::MyApplicationHelper ()
{
  m_clientFactory.SetTypeId (MyUdpClient::GetTypeId ());
  m_serverFactory.SetTypeId (MyUdpServer::GetTypeId ());
}

MyApplicationHelper::MyApplicationHelper (TypeId clientType, TypeId serverType)
{
  m_clientFactory.SetTypeId (clientType);
  m_serverFactory.SetTypeId (serverType);
}

void
MyApplicationHelper::SetClientAttribute (std::string name,
                                       const AttributeValue &value)
{
  m_clientFactory.Set (name, value);
}

void
MyApplicationHelper::SetServerAttribute (std::string name,
                                       const AttributeValue &value)
{
  m_serverFactory.Set (name, value);
}

Ptr<MyUdpClient>
MyApplicationHelper::Install (Ptr<Node> clientNode, Ptr<Node> serverNode,
                            Ipv4Address clientAddr, Ipv4Address serverAddr,
                            uint16_t clientPort, uint16_t serverPort,
                            Ipv4Header::DscpType dscp)
{
  Ptr<MyUdpClient> clientApp;
  clientApp = m_clientFactory.Create ()->GetObject<MyUdpClient> ();
  NS_ASSERT_MSG (clientApp, "Invalid client type id.");

  Ptr<MyUdpServer> serverApp;
  serverApp = m_serverFactory.Create ()->GetObject<MyUdpServer> ();
  NS_ASSERT_MSG (serverApp, "Invalid server type id.");

  InetSocketAddress serverInetAddr (serverAddr, serverPort);
  serverInetAddr.SetTos (dscp << 2);
  clientApp->SetAttribute ("LocalPort", UintegerValue (clientPort));
  clientApp->SetServer (serverApp, serverInetAddr);
  clientNode->AddApplication (clientApp);

  InetSocketAddress clientInetAddr (clientAddr, clientPort);
  clientInetAddr.SetTos (dscp << 2);
  serverApp->SetAttribute ("LocalPort", UintegerValue (serverPort));
  serverApp->SetClient (clientApp, clientInetAddr);
  serverNode->AddApplication (serverApp);

  return clientApp;
}

} // namespace ns3
