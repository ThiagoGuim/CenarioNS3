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

#include "udp-peer-app.h"
#include "../metadata/slice-tag.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("UdpPeerApp");
NS_OBJECT_ENSURE_REGISTERED (UdpPeerApp);

UdpPeerApp::UdpPeerApp ()
  : m_localSocket (0),
  m_localPort (0),
  m_peerAddress (Address ()),
  m_pktInterRng (0),
  m_pktSizeRng (0),
  m_sendEvent (EventId ())
{
  NS_LOG_FUNCTION (this);
}

UdpPeerApp::~UdpPeerApp ()
{
  NS_LOG_FUNCTION (this);
}

TypeId
UdpPeerApp::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::UdpPeerApp")
    .SetParent<Application> ()
    .AddConstructor<UdpPeerApp> ()
    .AddAttribute ("LocalPort", "Local port.",
                   UintegerValue (10000),
                   MakeUintegerAccessor (&UdpPeerApp::m_localPort),
                   MakeUintegerChecker<uint16_t> ())
    .AddAttribute ("PeerAddress", "The peer socket address.",
                   AddressValue (),
                   MakeAddressAccessor (&UdpPeerApp::m_peerAddress),
                   MakeAddressChecker ())
    .AddAttribute ("PktInterval",
                   "A random variable for the interval between packets [s].",
                   StringValue ("ns3::ConstantRandomVariable[Constant=0.08]"),
                   MakePointerAccessor (&UdpPeerApp::m_pktInterRng),
                   MakePointerChecker <RandomVariableStream> ())
    .AddAttribute ("PktSize",
                   "A random variable for the packet size [bytes].",
                   StringValue ("ns3::ConstantRandomVariable[Constant=1000]"),
                   MakePointerAccessor (&UdpPeerApp::m_pktSizeRng),
                   MakePointerChecker <RandomVariableStream> ())
    .AddAttribute ("TrafficLength",
                   "A random variable for the traffic length [s].",
                   StringValue ("ns3::ConstantRandomVariable[Constant=60]"),
                   MakePointerAccessor (&UdpPeerApp::m_lengthRng),
                   MakePointerChecker <RandomVariableStream> ())
    .AddAttribute ("SliceId", "Slice identifier.",
                   UintegerValue (0),
                   MakeUintegerAccessor (&UdpPeerApp::m_sliceId),
                   MakeUintegerChecker<uint8_t> ())
    .AddAttribute ("QosType", "Traffic QoS indicator.",
                   EnumValue (QosType::NON),
                   MakeEnumAccessor (&UdpPeerApp::m_qosType),
                   MakeEnumChecker (QosType::NON, QosTypeStr (QosType::NON),
                                    QosType::GBR, QosTypeStr (QosType::GBR)))
  ;
  return tid;
}

void
UdpPeerApp::StartTraffic (void)
{
  NS_LOG_FUNCTION (this);

  NS_ASSERT_MSG (m_localSocket, "Closed UDP socket.");

  // Schedule the first packet transmission.
  m_sendEvent.Cancel ();
  Time sendTime = Seconds (std::abs (m_pktInterRng->GetValue ()));
  m_sendEvent = Simulator::Schedule (sendTime, &UdpPeerApp::SendPacket, this);

  // Schedule the application to stop.
  m_stopEvent.Cancel ();
  Time stopTime = Seconds (std::abs (m_lengthRng->GetValue ()));
  m_stopEvent = Simulator::Schedule (stopTime, &UdpPeerApp::StopApplication, this);
}

void
UdpPeerApp::DoDispose (void)
{
  NS_LOG_FUNCTION (this);

  m_localSocket = 0;
  m_pktInterRng = 0;
  m_pktSizeRng = 0;
  m_sendEvent.Cancel ();
  Application::DoDispose ();
}

void
UdpPeerApp::StartApplication (void)
{
  NS_LOG_FUNCTION (this);

  // This method just open the socket, but does not start the traffic.
  if (m_localSocket == 0)
    {
      NS_LOG_INFO ("Opening the UDP socket.");
      TypeId udpFactory = TypeId::LookupByName ("ns3::UdpSocketFactory");
      m_localSocket = Socket::CreateSocket (GetNode (), udpFactory);
      m_localSocket->Bind (InetSocketAddress (Ipv4Address::GetAny (), m_localPort));
      m_localSocket->Connect (InetSocketAddress::ConvertFrom (m_peerAddress));
      m_localSocket->SetRecvCallback (MakeCallback (&UdpPeerApp::ReadPacket, this));
    }
  m_sendEvent.Cancel ();
}

void
UdpPeerApp::StopApplication ()
{
  NS_LOG_FUNCTION (this);

  // Cancel any pending send event and close the socket.
  m_sendEvent.Cancel ();
  if (m_localSocket != 0)
    {
      m_localSocket->Close ();
      m_localSocket->Dispose ();
      m_localSocket = 0;
    }
}

void
UdpPeerApp::ReadPacket (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this << socket);

  Ptr<Packet> packet = socket->Recv ();
  NS_LOG_DEBUG ("RX packet with " << packet->GetSize () << " bytes.");
}

void
UdpPeerApp::SendPacket ()
{
  NS_LOG_FUNCTION (this);

  Ptr<Packet> packet = Create<Packet> (m_pktSizeRng->GetInteger ());
  SliceTag tag (m_sliceId, m_qosType);
  packet->AddPacketTag (tag);

  int bytes = m_localSocket->Send (packet);
  if (bytes == static_cast<int> (packet->GetSize ()))
    {
      NS_LOG_DEBUG ("TX packet with " << bytes << " bytes.");
    }
  else
    {
      NS_LOG_ERROR ("TX error.");
    }

  // Schedule the next packet transmission.
  Time sendTime = Seconds (std::abs (m_pktInterRng->GetValue ()));
  m_sendEvent = Simulator::Schedule (sendTime, &UdpPeerApp::SendPacket, this);
}

} // namespace ns3
