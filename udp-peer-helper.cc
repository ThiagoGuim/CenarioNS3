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

#include "udp-peer-helper.h"

namespace ns3 {

UdpPeerHelper::UdpPeerHelper ()
{
  m_app1stFactory.SetTypeId (UdpPeerApp::GetTypeId ());
  m_app2ndFactory.SetTypeId (UdpPeerApp::GetTypeId ());
}

void
UdpPeerHelper::SetPeerAttribute (std::string name, const AttributeValue &value)
{
  m_app1stFactory.Set (name, value);
  m_app2ndFactory.Set (name, value);
}

void
UdpPeerHelper::Set1stAttribute (std::string name, const AttributeValue &value)
{
  m_app1stFactory.Set (name, value);
}

void
UdpPeerHelper::Set2ndAttribute (std::string name, const AttributeValue &value)
{
  m_app2ndFactory.Set (name, value);
}

ApplicationContainer
UdpPeerHelper::Install (
  Ptr<Node> node1st, Ptr<Node> node2nd, Ipv4Address addr1st,
  Ipv4Address addr2nd, uint16_t port1st, uint16_t port2nd,
  Ipv4Header::DscpType dscp)
{
  uint8_t ipTos = dscp << 2;

  ApplicationContainer apps;
  Ptr<UdpPeerApp> app1st = m_app1stFactory.Create ()->GetObject<UdpPeerApp> ();
  Ptr<UdpPeerApp> app2nd = m_app2ndFactory.Create ()->GetObject<UdpPeerApp> ();
  apps.Add (app1st);
  apps.Add (app2nd);

  InetSocketAddress inetAddr2nd (addr2nd, port2nd);
  inetAddr2nd.SetTos (ipTos);
  app1st->SetAttribute ("LocalPort", UintegerValue (port1st));
  app1st->SetAttribute ("PeerAddress", AddressValue (inetAddr2nd));
  node1st->AddApplication (app1st);

  InetSocketAddress inetAddr1st (addr1st, port1st);
  inetAddr1st.SetTos (ipTos);
  app2nd->SetAttribute ("LocalPort", UintegerValue (port2nd));
  app2nd->SetAttribute ("PeerAddress", AddressValue (inetAddr1st));
  node2nd->AddApplication (app2nd);

  return apps;
}

} // namespace ns3
