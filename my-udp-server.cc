/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2018 University of Campinas (Unicamp)
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
 * along with this program; if not, see <http://www.gnu.org/licenses/>.
 *
 * Author: Luciano Chaves <luciano@lrc.ic.unicamp.br>
 */


#include "my-udp-server.h"
#include "my-udp-client.h"

/*#undef NS_LOG_APPEND_CONTEXT
#define NS_LOG_APPEND_CONTEXT                             \
  std::clog << "[" << GetAppName ()                       \
            << " server teid " << GetTeidHex () << "] ";*/

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("MyUdpServer");
NS_OBJECT_ENSURE_REGISTERED (MyUdpServer);

TypeId
MyUdpServer::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::MyUdpServer")
    .SetParent<Application> ()
    .AddConstructor<MyUdpServer> ()

    .AddAttribute ("ClientAddress", "The client socket address.",
                   AddressValue (),
                   MakeAddressAccessor (&MyUdpServer::m_clientAddress),
                   MakeAddressChecker ())
    .AddAttribute ("LocalPort", "Local port.",
                   UintegerValue (10000),
                   MakeUintegerAccessor (&MyUdpServer::m_localPort),
                   MakeUintegerChecker<uint16_t> ())

    // These attributes must be configured for the desired traffic pattern.
    .AddAttribute ("PktInterval",
                   "A random variable used to pick the packet "
                   "inter-arrival time [s].",
                   StringValue ("ns3::ConstantRandomVariable[Constant=1]"),
                   MakePointerAccessor (&MyUdpServer::m_pktInterRng),
                   MakePointerChecker <RandomVariableStream> ())
    .AddAttribute ("PktSize", "The size of packets sent in on state",
                    UintegerValue (512),
                    MakeUintegerAccessor (&MyUdpServer::m_pktSize),
                    MakeUintegerChecker<uint32_t> (1))  
  ;
  return tid;
}

MyUdpServer::MyUdpServer ()
  : m_appStats (CreateObject<AppStatsCalculator> ()),
  m_socket (0),
  m_clientApp (0),
  m_sendEvent (EventId ())
{
  NS_LOG_FUNCTION (this);
}

MyUdpServer::~MyUdpServer ()
{
  NS_LOG_FUNCTION (this);
}

void
MyUdpServer::DoDispose (void)
{
  NS_LOG_FUNCTION (this);

  m_sendEvent.Cancel ();
}

void
MyUdpServer::StartApplication (void)
{
  NS_LOG_FUNCTION (this);

  NS_LOG_INFO ("Opening the UDP socket.");
  TypeId udpFactory = TypeId::LookupByName ("ns3::UdpSocketFactory");
  m_socket = Socket::CreateSocket (GetNode (), udpFactory);
  m_socket->Bind (InetSocketAddress (Ipv4Address::GetAny (), m_localPort));
  m_socket->Connect (InetSocketAddress::ConvertFrom (m_clientAddress));
  m_socket->SetRecvCallback (
    MakeCallback (&MyUdpServer::ReadPacket, this));

}

void
MyUdpServer::StopApplication ()
{
  NS_LOG_FUNCTION (this);

  if (m_socket != 0)
    {
      m_socket->Close ();
      m_socket->Dispose ();
      m_socket = 0;
    }
}

void
MyUdpServer::NotifyStart ()
{
  NS_LOG_FUNCTION (this);

  // Chain up to reset statistics.
  ResetAppStats ();

  // Start traffic.
  m_sendEvent.Cancel ();
  Time send = Seconds (std::abs (m_pktInterRng->GetValue ()));
  m_sendEvent = Simulator::Schedule (send, &MyUdpServer::SendPacket, this);
}

void
MyUdpServer::NotifyForceStop ()
{
  NS_LOG_FUNCTION (this);

  // Chain up just for log.
  NS_LOG_INFO ("Forcing the server application to stop.");

  // Stop traffic.
  m_sendEvent.Cancel ();
}

void
MyUdpServer::SendPacket ()
{
  NS_LOG_FUNCTION (this);

  Ptr<Packet> packet = Create<Packet> (m_pktSize);

  SeqTsHeader seqTs;
  seqTs.SetSeq (NotifyTx (packet->GetSize () + seqTs.GetSerializedSize ()));
  packet->AddHeader (seqTs);

  int bytes = m_socket->Send (packet);
  if (bytes == static_cast<int> (packet->GetSize ()))
    {
      NS_LOG_DEBUG ("Server TX " << bytes << " bytes with " <<
                    "sequence number " << seqTs.GetSeq ());
    }
  else
    {
      NS_LOG_ERROR ("Server TX error.");
    }

  // Schedule next packet transmission.
  Time send = Seconds (std::abs (m_pktInterRng->GetValue ()));
  m_sendEvent = Simulator::Schedule (send, &MyUdpServer::SendPacket, this);
}

void
MyUdpServer::SetClient (Ptr<MyUdpClient> clientApp, Address clientAddress)
{
  NS_LOG_FUNCTION (this << clientApp << clientAddress);

  m_clientApp = clientApp;
  m_clientAddress = clientAddress;
}


void
MyUdpServer::ReadPacket (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this << socket);

  // Receive the datagram from the socket.
  Ptr<Packet> packet = socket->Recv ();

  SeqTsHeader seqTs;
  packet->PeekHeader (seqTs);
  NotifyRx (packet->GetSize (), seqTs.GetTs ());
  NS_LOG_DEBUG ("Server RX " << packet->GetSize () << " bytes with " <<
                "sequence number " << seqTs.GetSeq ());
}

uint32_t
MyUdpServer::NotifyTx (uint32_t txBytes)
{
  NS_LOG_FUNCTION (this << txBytes);

  NS_ASSERT_MSG (m_clientApp, "Client application undefined.");
  return m_clientApp->m_appStats->NotifyTx (txBytes);
}

void
MyUdpServer::NotifyRx (uint32_t rxBytes, Time timestamp)
{
  NS_LOG_FUNCTION (this << rxBytes << timestamp);

  m_appStats->NotifyRx (rxBytes, timestamp);
}

void
MyUdpServer::ResetAppStats ()
{
  NS_LOG_FUNCTION (this);

  m_appStats->ResetCounters ();
}

Ptr<const AppStatsCalculator>
MyUdpServer::GetAppStats (void) const
{
  NS_LOG_FUNCTION (this);

  return m_appStats;
}

} // Namespace ns3
