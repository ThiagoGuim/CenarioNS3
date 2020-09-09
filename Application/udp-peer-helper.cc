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

NS_LOG_COMPONENT_DEFINE ("UdpPeerHelper");
NS_OBJECT_ENSURE_REGISTERED (UdpPeerHelper);

// Initial port number
uint16_t UdpPeerHelper::m_port = 10000;

UdpPeerHelper::UdpPeerHelper ()
  : m_startRng (0),
  m_startTime (Seconds (0))
{
  NS_LOG_FUNCTION (this);

  m_app1stFactory.SetTypeId (UdpPeerApp::GetTypeId ());
  m_app2ndFactory.SetTypeId (UdpPeerApp::GetTypeId ());
}

UdpPeerHelper::~UdpPeerHelper ()
{
  NS_LOG_FUNCTION (this);
}

TypeId
UdpPeerHelper::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::UdpPeerHelper")
    .SetParent<Object> ()
    .AddConstructor<UdpPeerHelper> ()
    .AddAttribute ("NumApps", "The number of apps for each pair of nodes.",
                   UintegerValue (1),
                   MakeUintegerAccessor (&UdpPeerHelper::m_numApps),
                   MakeUintegerChecker<uint16_t> ())
    .AddAttribute ("StartOffset",
                   "The initial interval before starting apps.",
                   TimeValue (Seconds (0)),
                   MakeTimeAccessor (&UdpPeerHelper::m_startOff),
                   MakeTimeChecker ())
    .AddAttribute ("StartInterval",
                   "A random variable for the interval between app starts.",
                   StringValue ("ns3::ExponentialRandomVariable[Mean=60.0]"),
                   MakePointerAccessor (&UdpPeerHelper::m_startRng),
                   MakePointerChecker<RandomVariableStream> ())
  ;
  return tid;
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

void
UdpPeerHelper::SetBothAttribute (std::string name, const AttributeValue &value)
{
  m_app1stFactory.Set (name, value);
  m_app2ndFactory.Set (name, value);
}

void
UdpPeerHelper::Install (
  NodeContainer nodes1st, NodeContainer nodes2nd,
  Ipv4InterfaceContainer addr1st, Ipv4InterfaceContainer addr2nd)
{
  NS_ASSERT_MSG (nodes1st.GetN () == nodes2nd.GetN ()
                 && addr1st.GetN () == addr2nd.GetN ()
                 && nodes1st.GetN () == addr1st.GetN (),
                 "Inconsistent number of nodes or interfaces.");

  // For each pair of nodes, install m_numApps applications.
  for (uint32_t i = 0; i < nodes1st.GetN (); i++)
    {
      m_startTime = m_startOff;
      for (uint16_t j = 0; j < m_numApps; j++)
        {
          m_startTime += Seconds (std::abs (m_startRng->GetValue ()));
          InstallApp (nodes1st.Get (i), nodes2nd.Get (i),
                      addr1st.GetAddress (i), addr2nd.GetAddress (i),
                      GetNextPortNo (), Ipv4Header::DscpDefault, m_startTime);
          NS_LOG_INFO ("App " << j << " at node " << i <<
                       " will start at " << m_startTime.GetSeconds ());
        }
    }
}

ApplicationContainer
UdpPeerHelper::InstallApp (
  Ptr<Node> node1st, Ptr<Node> node2nd,
  Ipv4Address addr1st, Ipv4Address addr2nd,
  uint16_t portNo, Ipv4Header::DscpType dscp, Time startTime)
{
  uint8_t ipTos = dscp << 2;

  ApplicationContainer apps;
  Ptr<UdpPeerApp> app1st = m_app1stFactory.Create ()->GetObject<UdpPeerApp> ();
  Ptr<UdpPeerApp> app2nd = m_app2ndFactory.Create ()->GetObject<UdpPeerApp> ();
  apps.Add (app1st);
  apps.Add (app2nd);

  InetSocketAddress inetAddr2nd (addr2nd, portNo);
  inetAddr2nd.SetTos (ipTos);
  app1st->SetAttribute ("LocalPort", UintegerValue (portNo));
  app1st->SetAttribute ("PeerAddress", AddressValue (inetAddr2nd));
  node1st->AddApplication (app1st);

  InetSocketAddress inetAddr1st (addr1st, portNo);
  inetAddr1st.SetTos (ipTos);
  app2nd->SetAttribute ("LocalPort", UintegerValue (portNo));
  app2nd->SetAttribute ("PeerAddress", AddressValue (inetAddr1st));
  node2nd->AddApplication (app2nd);

  Simulator::Schedule (startTime, &UdpPeerApp::StartTraffic, app1st);
  Simulator::Schedule (startTime, &UdpPeerApp::StartTraffic, app2nd);

  return apps;
}

void
UdpPeerHelper::DoDispose ()
{
  Object::DoDispose ();
}

uint16_t
UdpPeerHelper::GetNextPortNo ()
{
  NS_ABORT_MSG_IF (m_port == 0xFFFF, "No more ports available for use.");
  return m_port++;
}

} // namespace ns3
