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

#include "my-udp-client.h"
#include "my-udp-server.h"

/*#undef NS_LOG_APPEND_CONTEXT
#define NS_LOG_APPEND_CONTEXT                             \
  std::clog << "[" << GetAppName ()                       \
            << " client teid " << GetTeidHex () << "] ";*/

namespace ns3 {


NS_LOG_COMPONENT_DEFINE ("MyUdpClient");
NS_OBJECT_ENSURE_REGISTERED (MyUdpClient);

MyUdpClient::MyUdpClient ()
  : m_appStats (CreateObject<AppStatsCalculator> ()),
  m_socket (0),
  m_serverApp (0),
  m_active (false),
  m_forceStop (EventId ()),
  m_forceStopFlag (false),
  m_bearerId (1),   // This is the default BID.
  m_teid (0),
  
  m_sendEvent (EventId ()),
  m_stopEvent (EventId ())
{
  NS_LOG_FUNCTION (this);

  if(m_log == 0){
    
    m_log = Create<OutputStreamWrapper> ("iperfs.txt", std::ios::out);
    // Print the header in output file.
    *m_log->GetStream ()
      << "Inicio(seg)"
      << std::setw(16) << "Duracao(seg)"
      << std::setw(16) << "Banda(Kbps)"    
      << std::setw(8) << "PCli"    
      << std::setw(8) << "PServ"
      << std::endl; 
  }

}

MyUdpClient::~MyUdpClient ()
{
  NS_LOG_FUNCTION (this);
}

TypeId
MyUdpClient::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::MyUdpClient")
    .SetParent<Application> ()
    .AddConstructor<MyUdpClient> ()
    .AddAttribute ("AppName", "The application name.",
                   StringValue ("NoName"),
                   MakeStringAccessor (&MyUdpClient::m_name),
                   MakeStringChecker ())
    .AddAttribute ("MaxOnTime", "A hard duration time threshold.",
                   TimeValue (Time ()),
                   MakeTimeAccessor (&MyUdpClient::m_maxOnTime),
                   MakeTimeChecker ())
    .AddAttribute ("TrafficLength",
                   "A random variable used to pick the traffic length [s].",
                   StringValue ("ns3::ConstantRandomVariable[Constant=30.0]"),
                   MakePointerAccessor (&MyUdpClient::m_lengthRng),
                   MakePointerChecker <RandomVariableStream> ())

    .AddAttribute ("ServerAddress", "The server socket address.",
                   AddressValue (),
                   MakeAddressAccessor (&MyUdpClient::m_serverAddress),
                   MakeAddressChecker ())
    .AddAttribute ("LocalPort", "Local port.",
                   UintegerValue (10000),
                   MakeUintegerAccessor (&MyUdpClient::m_localPort),
                   MakeUintegerChecker<uint16_t> ())

    .AddTraceSource ("AppStart", "MyUdpClient start trace source.",
                     MakeTraceSourceAccessor (&MyUdpClient::m_appStartTrace),
                     "ns3::MyUdpClient::EpcAppTracedCallback")
    .AddTraceSource ("AppStop", "MyUdpClient stop trace source.",
                     MakeTraceSourceAccessor (&MyUdpClient::m_appStopTrace),
                     "ns3::MyUdpClient::EpcAppTracedCallback")
    .AddTraceSource ("AppError", "MyUdpClient error trace source.",
                     MakeTraceSourceAccessor (&MyUdpClient::m_appErrorTrace),
                     "ns3::MyUdpClient::EpcAppTracedCallback")


    // These attributes must be configured for the desired traffic pattern.
    .AddAttribute ("DataRate", "The data rate in on state.",
                    StringValue ("ns3::ExponentialRandomVariable[Mean=1024]"),
                    MakePointerAccessor (&MyUdpClient::m_cbrRng),
                    MakePointerChecker <RandomVariableStream>()
                    )
    .AddAttribute ("PktSize", "The size of packets sent in on state",
                    UintegerValue (512),
                    MakeUintegerAccessor (&MyUdpClient::m_pktSize),
                    MakeUintegerChecker<uint32_t> (1))
  ;
  return tid;
}

std::string
MyUdpClient::GetAppName (void) const
{
  // No log to avoid infinite recursion.
  return m_name;
}

std::string
MyUdpClient::GetNameTeid (void) const
{
  // No log to avoid infinite recursion.
  std::ostringstream value;
  value << GetAppName () << " over bearer teid " << GetTeidHex ();
  return value.str ();
}

bool
MyUdpClient::IsActive (void) const
{
  NS_LOG_FUNCTION (this);

  return m_active;
}

Time
MyUdpClient::GetMaxOnTime (void) const
{
  NS_LOG_FUNCTION (this);

  return m_maxOnTime;
}

bool
MyUdpClient::IsForceStop (void) const
{
  NS_LOG_FUNCTION (this);

  return m_forceStopFlag;
}

EpsBearer
MyUdpClient::GetEpsBearer (void) const
{
  NS_LOG_FUNCTION (this);

  return m_bearer;
}

uint8_t
MyUdpClient::GetEpsBearerId (void) const
{
  NS_LOG_FUNCTION (this);

  return m_bearerId;
}

uint32_t
MyUdpClient::GetTeid (void) const
{
  NS_LOG_FUNCTION (this);

  return m_teid;
}

std::string
MyUdpClient::GetTeidHex (void) const
{
  // No log to avoid infinite recursion.
  return GetUint32Hex (m_teid);
}

Ptr<MyUdpServer>
MyUdpClient::GetServerApp (void) const
{
  NS_LOG_FUNCTION (this);

  return m_serverApp;
}

Ptr<const AppStatsCalculator>
MyUdpClient::GetAppStats (void) const
{
  NS_LOG_FUNCTION (this);

  return m_appStats;
}

Ptr<const AppStatsCalculator>
MyUdpClient::GetServerAppStats (void) const
{
  NS_LOG_FUNCTION (this);

  NS_ASSERT_MSG (m_serverApp, "Server application undefined.");
  return m_serverApp->GetAppStats ();
}

void
MyUdpClient::SetEpsBearer (EpsBearer value)
{
  NS_LOG_FUNCTION (this);

  m_bearer = value;
}

void
MyUdpClient::SetEpsBearerId (uint8_t value)
{
  NS_LOG_FUNCTION (this << value);

  m_bearerId = value;
}

void
MyUdpClient::SetTeid (uint32_t value)
{
  NS_LOG_FUNCTION (this << value);

  m_teid = value;
}

void
MyUdpClient::SetServer (Ptr<MyUdpServer> serverApp, Address serverAddress)
{
  NS_LOG_FUNCTION (this << serverApp << serverAddress);

  m_serverApp = serverApp;
  m_serverAddress = serverAddress;
}

void
MyUdpClient::Start ()
{
  NS_LOG_FUNCTION (this);
  NS_LOG_INFO ("Starting client application.");

  // Schedule the ForceStop method to stop traffic based on traffic length.
  Time stop = GetTrafficLength ();
  //Simulator::Schedule (stop, &MyUdpClient::RequestOnePacket, this);

  m_stopEvent = Simulator::Schedule (stop, &MyUdpClient::ForceStop, this);
  NS_LOG_INFO ("Set traffic length to " << stop.GetSeconds () << "s.");

  m_cbrRate = DataRate (m_cbrRng->GetValue()*1000);
  while(m_cbrRate.GetBitRate() <= 500) //Garante trafegos de pelo menos 0.5kbps
  {
    m_cbrRate = DataRate (m_cbrRng->GetValue()*1000);
  }
  
  // Chain up to reset statistics, notify server, and fire start trace source.
  //MyUdpClient::Start ();

  // Set the active flag.
  NS_ASSERT_MSG (!IsActive (), "Can't start an already active application.");
  m_active = true;

  // Reset internal statistics.
  ResetAppStats ();

  // Schedule the force stop event.
  m_forceStopFlag = false;
  if (!m_maxOnTime.IsZero ())
    {
      m_forceStop =
        Simulator::Schedule (m_maxOnTime, &MyUdpClient::ForceStop, this);
    }

  // Notify the server and fire start trace source.
  NS_ASSERT_MSG (m_serverApp, "Server application undefined.");
  m_serverApp->NotifyStart ();
  m_appStartTrace (this);

  // Start traffic.
  m_sendEvent.Cancel ();
  //Time send = Seconds (std::abs (m_pktInterRng->GetValue ()));
  Time send (Seconds (m_pktSize*8 / static_cast<double>(m_cbrRate.GetBitRate ()))); // Time till next packet
  m_sendEvent = Simulator::Schedule (send, &MyUdpClient::SendPacket, this);

  //Imprime informacoes sobre os trafegos
  *m_log->GetStream ()
      << std::fixed << std::setprecision(3) << Simulator::Now().GetSeconds()
      << std::setw(16) << stop.GetSeconds()
      << std::setw(16) << (double) m_cbrRate.GetBitRate()/1000    
      << std::setw(8) << m_localPort    
      << std::setw(8) << InetSocketAddress::ConvertFrom (m_serverAddress).GetPort ()
      << std::endl; 
}

void
MyUdpClient::DoDispose (void)
{
  NS_LOG_FUNCTION (this);

  m_stopEvent.Cancel ();
  m_sendEvent.Cancel ();
  m_log = 0;

  m_appStats = 0;
  m_lengthRng = 0;
  m_socket = 0;
  m_serverApp = 0;
  m_forceStop.Cancel ();

  Application::DoDispose ();
}


void
MyUdpClient::ForceStop ()
{
  NS_LOG_FUNCTION (this);
  NS_LOG_INFO ("Forcing the client application to stop.");

  // Cancel (possible) pending stop event and stop the traffic.
  m_stopEvent.Cancel ();
  m_sendEvent.Cancel ();

  // Chain up to notify server.
  //MyUdpClient::ForceStop ();

  // Set the force stop flag.
  NS_ASSERT_MSG (IsActive (), "Can't stop an inactive application.");
  m_forceStopFlag = true;
  m_forceStop.Cancel ();

  // Notify the server.
  NS_ASSERT_MSG (m_serverApp, "Server application undefined.");
  m_serverApp->NotifyForceStop ();

  // Notify the stopped application one second later.
  Simulator::Schedule (Seconds (1), &MyUdpClient::NotifyStop, this, false);
}

void
MyUdpClient::StartApplication (void)
{
  NS_LOG_FUNCTION (this);

  NS_LOG_INFO ("Opening the UDP socket.");
  TypeId udpFactory = TypeId::LookupByName ("ns3::UdpSocketFactory");
  m_socket = Socket::CreateSocket (GetNode (), udpFactory);
  m_socket->Bind (InetSocketAddress (Ipv4Address::GetAny (), m_localPort));
  m_socket->Connect (InetSocketAddress::ConvertFrom (m_serverAddress));
  m_socket->SetRecvCallback (
    MakeCallback (&MyUdpClient::ReadPacket, this));


  m_serverApp->StartApplication();
  Start();

}

void
MyUdpClient::StopApplication ()
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
MyUdpClient::SendPacket ()
{
  NS_LOG_FUNCTION (this);

  Ptr<Packet> packet = Create<Packet> (m_pktSize);

  SeqTsHeader seqTs;
  seqTs.SetSeq (NotifyTx (packet->GetSize () + seqTs.GetSerializedSize ()));
  packet->AddHeader (seqTs);

  int bytes = m_socket->Send (packet);
  if (bytes == static_cast<int> (packet->GetSize ()))
    {
      NS_LOG_DEBUG ("Client TX " << bytes << " bytes with " <<
                    "sequence number " << seqTs.GetSeq ());
    }
  else
    {
      NS_LOG_ERROR ("Client TX error.");
    }

  // Schedule next packet transmission.
  Time send = Seconds (m_pktSize*8 / static_cast<double>(m_cbrRate.GetBitRate ())); // Time till next packet
  m_sendEvent = Simulator::Schedule (send, &MyUdpClient::SendPacket, this);
}

void
MyUdpClient::ReadPacket (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this << socket);

  // Receive the datagram from the socket.
  Ptr<Packet> packet = socket->Recv ();

  SeqTsHeader seqTs;
  packet->PeekHeader (seqTs);
  NotifyRx (packet->GetSize (), seqTs.GetTs ());
  NS_LOG_DEBUG ("Client RX " << packet->GetSize () << " bytes with " <<
                "sequence number " << seqTs.GetSeq ());
}

Time
MyUdpClient::GetTrafficLength ()
{
  NS_LOG_FUNCTION (this);

  return Seconds (std::abs (m_lengthRng->GetValue ()));
}

void
MyUdpClient::NotifyStop (bool withError)
{
  NS_LOG_FUNCTION (this << withError);
  NS_LOG_INFO ("Client application stopped.");

  // Set the active flag.
  NS_ASSERT_MSG (IsActive (), "Can't stop an inactive application.");
  m_active = false;
  m_forceStop.Cancel ();

  // Fire the stop trace source.
  if (withError)
    {
      NS_LOG_ERROR ("Client application stopped with error.");
      m_appErrorTrace (this);
    }
  else
    {
      m_appStopTrace (this);
    }
}

uint32_t
MyUdpClient::NotifyTx (uint32_t txBytes)
{
  NS_LOG_FUNCTION (this << txBytes);

  NS_ASSERT_MSG (m_serverApp, "Server application undefined.");
  return m_serverApp->m_appStats->NotifyTx (txBytes);
}

void
MyUdpClient::NotifyRx (uint32_t rxBytes, Time timestamp)
{
  NS_LOG_FUNCTION (this << rxBytes << timestamp);

  m_appStats->NotifyRx (rxBytes, timestamp);
}

void
MyUdpClient::ResetAppStats ()
{
  NS_LOG_FUNCTION (this);

  m_appStats->ResetCounters ();
}

} // namespace ns3
