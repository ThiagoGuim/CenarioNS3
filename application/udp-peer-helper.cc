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
  : m_startTime (Seconds (0))
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
    .AddAttribute ("DataRate", "A random variable for the traffic data rate [kbps].",
                   StringValue ("ns3::ExponentialRandomVariable[Mean=1024]"),
                   MakePointerAccessor (&UdpPeerHelper::m_rateRng),
                   MakePointerChecker <RandomVariableStream> ())
    .AddAttribute ("StartInterval", "A random variable for the start interval between app [s].",
                   StringValue ("ns3::ExponentialRandomVariable[Mean=30.0]"),
                   MakePointerAccessor (&UdpPeerHelper::m_startRng),
                   MakePointerChecker<RandomVariableStream> ())
    .AddAttribute ("StartOffset", "The waiting interval before starting apps [s].",
                   TimeValue (Seconds (0)),
                   MakeTimeAccessor (&UdpPeerHelper::m_startOff),
                   MakeTimeChecker ())
    .AddAttribute ("TrafficLength", "A random variable for the traffic length [s].",
                   StringValue ("ns3::NormalRandomVariable[Mean=30|Variance=100]"),
                   MakePointerAccessor (&UdpPeerHelper::m_lengthRng),
                   MakePointerChecker <RandomVariableStream> ())
    .AddAttribute ("FullDuplex", "A boolean value to determine the traffic direction.",
                   BooleanValue (false),
                   MakeBooleanAccessor (&UdpPeerHelper::m_fullDuplex),
                   MakeBooleanChecker ())
    .AddAttribute ("SplitTraffic", "Two different caracteristics in the same traffic.",
                  //  TypeId::ATTR_GET | TypeId::ATTR_CONSTRUCT,
                   BooleanValue (false),
                   MakeBooleanAccessor (&UdpPeerHelper::m_split),
                   MakeBooleanChecker ())
    .AddAttribute ("DSCP", "Type of traffic DSCP based.",
                  //  TypeId::ATTR_GET | TypeId::ATTR_CONSTRUCT,
                   EnumValue (Ipv4Header::DscpDefault),
                   MakeEnumAccessor (&UdpPeerHelper::m_DSCP),
                   MakeEnumChecker (Ipv4Header::DSCP_EF, "DSCP_EF",
                                    Ipv4Header::DSCP_AF41, "DSCP_AF41",
                                    Ipv4Header::DSCP_AF31, "DSCP_AF31",
                                    Ipv4Header::DSCP_AF32, "DSCP_AF32",
                                    Ipv4Header::DSCP_AF21, "DSCP_AF21",
                                    Ipv4Header::DSCP_AF11, "DSCP_AF11",
                                    Ipv4Header::DscpDefault, "DscpDefault"))
    .AddAttribute ("TQosType", "Different types of traffic.",
                   EnumValue (TQosType::ALL),
                   MakeEnumAccessor (&UdpPeerHelper::m_tQosType),
                   MakeEnumChecker (TQosType::AF, TQosTypeStr (TQosType::AF),
                                    TQosType::BE, TQosTypeStr (TQosType::BE),
                                    TQosType::PRIO, TQosTypeStr (TQosType::PRIO)))

 
  ;
  return tid;
}



uint16_t
UdpPeerHelper::GetNumApps (void) const
{
  return m_numApps;
}

Time
UdpPeerHelper::GetStartOffset (void) const
{
  return m_startOff;
}

bool     
UdpPeerHelper::GetFullDuplex  (void) const
{
  return m_fullDuplex;
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
  Ipv4InterfaceContainer addr1st, Ipv4InterfaceContainer addr2nd, uint8_t sliceId,
  Ipv4Header::DscpType dscp)
{
  NS_ASSERT_MSG (nodes1st.GetN () == nodes2nd.GetN ()
                 && addr1st.GetN () == addr2nd.GetN ()
                 && nodes1st.GetN () == addr1st.GetN (),
                 "Inconsistent number of nodes or interfaces.");

  // For each pair of nodes, install m_numApps applications.
  
  Ptr<UniformRandomVariable> randomVar = CreateObject<UniformRandomVariable>();
  randomVar->SetAttribute("Min", DoubleValue(0));
  randomVar->SetAttribute("Max", DoubleValue(1));
	
  Time startCurrent;
  Time startTimeSplit;
  Time startTimeSplit2;	

  for (uint32_t i = 0; i < nodes1st.GetN (); i++)
    {
      m_startTime = m_startOff;
      startTimeSplit = Seconds (250);      
      startTimeSplit2 = Seconds (500);	

      for (uint16_t j = 0; j < m_numApps; j++)
        {
	
          double x = randomVar->GetValue();

          if(x > 0.3 && m_split)
            {
              startTimeSplit2 += Seconds (std::abs (m_startRng->GetValue ()));
              startCurrent = startTimeSplit2; 
            }
          else
            {
                    
              if(x > 0.15 && m_split)
                {
                  startTimeSplit += Seconds (std::abs (m_startRng->GetValue ()));
                  startCurrent = startTimeSplit;
                }
              else
                {
                  m_startTime += Seconds (std::abs (m_startRng->GetValue ()));
                  startCurrent = m_startTime;
                }

            }
          
          Time lengthTime = Seconds (std::abs (m_lengthRng->GetValue ()));

          ApplicationContainer apps;
          apps = InstallApp (nodes1st.Get (i), nodes2nd.Get (i),
                            addr1st.GetAddress (i), addr2nd.GetAddress (i),
                            GetNextPortNo (), m_DSCP, startCurrent, lengthTime, sliceId);


          // Fix the packet size at 1024 bytes and compute the packet interval
          // to match the desired data rate.
          DataRate cbr (std::abs (m_rateRng->GetValue () * 1000));
          double pktSize = 1024;
          double pktInterval = (pktSize * 8 / static_cast<double> (cbr.GetBitRate ()));

          // Update app attributes to the desired bit rate.
          apps.Get (0)->SetAttribute (
            "PktSize", PointerValue (
              CreateObjectWithAttributes<ConstantRandomVariable> (
                "Constant", DoubleValue (pktSize))));
          apps.Get (1)->SetAttribute (
            "PktSize", PointerValue (
              CreateObjectWithAttributes<ConstantRandomVariable> (
                "Constant", DoubleValue (pktSize))));
          apps.Get (0)->SetAttribute (
            "PktInterval", PointerValue (
              CreateObjectWithAttributes<ConstantRandomVariable> (
                "Constant", DoubleValue (pktInterval))));
          apps.Get (1)->SetAttribute (
            "PktInterval", PointerValue (
              CreateObjectWithAttributes<ConstantRandomVariable> (
                "Constant", DoubleValue (pktInterval))));

          NS_LOG_INFO ("App " << j << " at host " << i <<
                      " will start at " << startCurrent.GetSeconds () <<
                      " with data rate of " << Bps2Kbps (cbr) <<
                      " and traffic length of " << lengthTime.GetSeconds ());

          // std::cout << "App " << j << " at host " << i <<
          //           " will start at " << startCurrent.GetSeconds () <<
          //           " with data rate of " << Bps2Kbps (cbr) <<
          //           " and traffic length of " << lengthTime.GetSeconds () << std::endl;
        }
    }
}

ApplicationContainer
UdpPeerHelper::InstallApp (
  Ptr<Node> node1st, Ptr<Node> node2nd,
  Ipv4Address addr1st, Ipv4Address addr2nd,
  uint16_t portNo, Ipv4Header::DscpType dscp,
  Time startTime, Time lengthTime, uint8_t sliceId)
{
  // uint8_t ipTos = m_DSCP << 2;
  uint8_t ipTos = trafficTypeToDscpHeader(m_tQosType) << 2;

  ApplicationContainer apps;
  Ptr<UdpPeerApp> app1st = m_app1stFactory.Create ()->GetObject<UdpPeerApp> ();
  Ptr<UdpPeerApp> app2nd = m_app2ndFactory.Create ()->GetObject<UdpPeerApp> ();
  apps.Add (app1st);
  apps.Add (app2nd);

  InetSocketAddress inetAddr2nd (addr2nd, portNo);
  inetAddr2nd.SetTos (ipTos);
  app1st->SetAttribute ("LocalPort", UintegerValue (portNo));
  app1st->SetAttribute ("PeerAddress", AddressValue (inetAddr2nd));
  app1st->SetAttribute ("QosType", EnumValue (Dscp2QosType (m_DSCP)));
  app1st->SetAttribute ("TQosType", EnumValue(m_tQosType));
  app1st->SetAttribute ("SliceId", UintegerValue (sliceId));
  node1st->AddApplication (app1st);


  InetSocketAddress inetAddr1st (addr1st, portNo);
  inetAddr1st.SetTos (ipTos);
  app2nd->SetAttribute ("LocalPort", UintegerValue (portNo));
  app2nd->SetAttribute ("PeerAddress", AddressValue (inetAddr1st));
  app2nd->SetAttribute ("QosType", EnumValue (Dscp2QosType (m_DSCP)));
  app2nd->SetAttribute ("TQosType", EnumValue(m_tQosType));
  app2nd->SetAttribute ("SliceId", UintegerValue (sliceId));
  node2nd->AddApplication (app2nd);

  Simulator::Schedule (startTime, &UdpPeerApp::StartTraffic, app1st);

  if (m_fullDuplex)
    {
      Simulator::Schedule (startTime, &UdpPeerApp::StartTraffic, app2nd);
    }
  
  Simulator::Schedule (startTime + lengthTime, &UdpPeerApp::StopTraffic, app1st);
  Simulator::Schedule (startTime + lengthTime, &UdpPeerApp::StopTraffic, app2nd);

  return apps;
}

void
UdpPeerHelper::DoDispose ()
{
  m_startRng = 0;
  m_rateRng = 0;
  m_lengthRng = 0;
  Object::DoDispose ();
}

uint16_t
UdpPeerHelper::GetNextPortNo ()
{
  NS_ABORT_MSG_IF (m_port == 0xFFFF, "No more ports available for use.");
  return m_port++;
}

} // namespace ns3
