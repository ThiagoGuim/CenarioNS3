/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2015 University of Campinas (Unicamp)
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

#include <iomanip>
#include <iostream>
#include "network-statistics.h"
#include "../metadata/slice-info.h"

using namespace std;

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("NetworkStatistics");
NS_OBJECT_ENSURE_REGISTERED (NetworkStatistics);

NetworkStatistics::NetworkStatistics ()
{
  NS_LOG_FUNCTION (this);

  // Clear slice metadata.
  memset (m_slices, 0, sizeof (SliceMetadata) * (SLICE_ALL));

  // for (int s = 0; s < (SLICE_ALL); s++)
  //   {
  //     for (int t = 0; t < N_TRAFFIC_TYPES; t++)
  //       {
  //         m_slices [s].flowStats [t] = 0;
  //       }
  //     m_slices [s].bwdWrapper = 0;
  //     m_slices [s].tffWrapper = 0;
  //   }

  // Connect this stats calculator to required trace sources.
  Config::Connect (
    "/NodeList/*/ApplicationList/*/$ns3::UdpPeerApp/TxPkt",
    MakeCallback (&NetworkStatistics::TxPacket, this));
  Config::Connect (
    "/NodeList/*/ApplicationList/*/$ns3::UdpPeerApp/RxPkt",
    MakeCallback (&NetworkStatistics::RxPacket, this));
  Config::Connect (
    "/NodeList/*/$ns3::OFSwitch13Device/OverloadDrop",
    MakeCallback (&NetworkStatistics::OverloadDropPacket, this));
  Config::Connect (
    "/NodeList/*/$ns3::OFSwitch13Device/MeterDrop",
    MakeCallback (&NetworkStatistics::MeterDropPacket, this));
  Config::Connect (
    "/NodeList/*/$ns3::OFSwitch13Device/TableDrop",
    MakeCallback (&NetworkStatistics::TableDropPacket, this));
  Config::Connect (
    "/NodeList/*/$ns3::OFSwitch13Device/PortList/*/PortQueue/Drop",
    MakeCallback (&NetworkStatistics::QueueDropPacket, this));
}

NetworkStatistics::~NetworkStatistics ()
{
  NS_LOG_FUNCTION (this);
}

TypeId
NetworkStatistics::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::NetworkStatistics")
    .SetParent<Object> ()
    .AddConstructor<NetworkStatistics> ()
    .AddAttribute ("BwdStatsFilename", "Filename for bandwidth statistics.",
                   StringValue ("bandwidth"),
                   MakeStringAccessor (&NetworkStatistics::m_bwdFilename),
                   MakeStringChecker ())
    .AddAttribute ("TffStatsFilename", "Filename for traffic statistics.",
                   StringValue ("traffic"),
                   MakeStringAccessor (&NetworkStatistics::m_tffFilename),
                   MakeStringChecker ())
  ;
  return tid;
}

void
NetworkStatistics::DoDispose ()
{
  NS_LOG_FUNCTION (this);

  for (int s = 0; s < (SLICE_ALL); s++)
    {
      for (int t = 0; t < N_TRAFFIC_TYPES; t++)
        {
          m_slices [s].flowStats [t] = 0;
        }
      m_slices [s].bwdWrapper = 0;
      m_slices [s].tffWrapper = 0;
    }

  Object::DoDispose ();
}

void
NetworkStatistics::NotifyConstructionCompleted (void)
{
  NS_LOG_FUNCTION (this);

  StringValue stringValue;
  GlobalValue::GetValueByName ("OutputPrefix", stringValue);
  std::string prefix = stringValue.Get ();
  SetAttribute ("BwdStatsFilename", StringValue (prefix + m_bwdFilename));
  SetAttribute ("TffStatsFilename", StringValue (prefix + m_tffFilename));

  for (int s = 0; s <= static_cast<int> (SliceInfo::GetNSlices ()); s++)
    {
      std::string sliceStr = std::to_string (s);
      SliceMetadata &slData = m_slices [s];
      for (int t = 0; t < N_TRAFFIC_TYPES; t++)
        {
          slData.flowStats [t] =
            CreateObjectWithAttributes<FlowStatsCalculator> (
              "Continuous", BooleanValue (true));
        }

      // Create the output files for this slice.
      slData.bwdWrapper = Create<OutputStreamWrapper> (
          m_bwdFilename + "-" + sliceStr + ".log", std::ios::out);
      slData.tffWrapper = Create<OutputStreamWrapper> (
          m_tffFilename + "-" + sliceStr + ".log", std::ios::out);

      // Print the headers in output files.
      *slData.bwdWrapper->GetStream ()
        << boolalpha << right << fixed << setprecision (3)
        << " " << setw (8) << "TimeSec"
        << " " << setw (7) << "LinkDir";
      LinkInfo::PrintHeader (*slData.bwdWrapper->GetStream ());
      *slData.bwdWrapper->GetStream () << std::endl;

      *slData.tffWrapper->GetStream ()
        << boolalpha << right << fixed << setprecision (3)
        << " " << setw (8) << "TimeSec"
        << " " << setw (8) << "QosType";
      FlowStatsCalculator::PrintHeader (*slData.tffWrapper->GetStream ());
      *slData.tffWrapper->GetStream () << std::endl;
    }

  // Schedule the first dump.
  Simulator::Schedule (Seconds (1), &NetworkStatistics::DumpStatistics, this);

  Object::NotifyConstructionCompleted ();
}
void
NetworkStatistics::DumpStatistics (void)
{
  NS_LOG_FUNCTION (this);

  // Dump statistics for each network slice.
  for (int s = 0; s <= static_cast<int> (SliceInfo::GetNSlices ()); s++)
    {
      SliceMetadata &slData = m_slices [s];

      // Trick to print slice ALL with index 0
      int sliceId = (s == 0 ? SLICE_ALL : s);

      // Dump slice bandwidth usage for each link.
      for (auto const &link : LinkInfo::GetList ())
        {
          for (int d = 0; d < N_LINK_DIRS; d++)
            {
              LinkInfo::LinkDir dir = static_cast<LinkInfo::LinkDir> (d);

              *slData.bwdWrapper->GetStream ()
                << " " << setw (8) << Simulator::Now ().GetSeconds ()
                << " " << setw (7) << LinkInfo::LinkDirStr (dir);
              link->PrintValues (*slData.bwdWrapper->GetStream (), dir, sliceId);
              *slData.bwdWrapper->GetStream () << std::endl;
            }
        }
      *slData.bwdWrapper->GetStream () << std::endl;

      // Dump slice traffic stats for each direction.
      for (int t = 0; t < N_TRAFFIC_TYPES; t++)
        {
          TQosType type = static_cast<TQosType> (t);
          Ptr<FlowStatsCalculator> flowStats = slData.flowStats [t];

          *slData.tffWrapper->GetStream ()
            << " " << setw (8) << Simulator::Now ().GetSeconds ()
            << " " << setw (8) << TQosTypeStr (type)
            << *flowStats
            << std::endl;
          flowStats->ResetCounters ();
        }
      *slData.tffWrapper->GetStream () << std::endl;
    }

  // Schedule the next dump.
  Simulator::Schedule (Seconds (1), &NetworkStatistics::DumpStatistics, this);
}

void
NetworkStatistics::OverloadDropPacket (std::string context, Ptr<const Packet> packet)
{
  NS_LOG_FUNCTION (this << context << packet);

  SliceTag sliceTag;
  if (packet->PeekPacketTag (sliceTag))
    {
      uint16_t sliceId = sliceTag.GetSliceId ();
      TQosType qosType = sliceTag.GetTQosType ();

      Ptr<FlowStatsCalculator> slcStats = m_slices [sliceId].flowStats [qosType];
      slcStats->NotifyDrop (packet->GetSize (), FlowStatsCalculator::PLOAD);

      Ptr<FlowStatsCalculator> aggStats = m_slices [0].flowStats [qosType];
      aggStats->NotifyDrop (packet->GetSize (), FlowStatsCalculator::PLOAD);
    }
}

void
NetworkStatistics::MeterDropPacket (
  std::string context, Ptr<const Packet> packet, uint32_t meterId)
{
  NS_LOG_FUNCTION (this << context << packet << meterId);

  SliceTag sliceTag;
  if (packet->PeekPacketTag (sliceTag))
    {
      uint16_t sliceId = sliceTag.GetSliceId ();
      TQosType qosType = sliceTag.GetTQosType ();

      Ptr<FlowStatsCalculator> slcStats = m_slices [sliceId].flowStats [qosType];
      slcStats->NotifyDrop (packet->GetSize (), FlowStatsCalculator::SLICE);

      Ptr<FlowStatsCalculator> aggStats = m_slices [0].flowStats [qosType];
      aggStats->NotifyDrop (packet->GetSize (), FlowStatsCalculator::SLICE);
    }
}

void
NetworkStatistics::QueueDropPacket (std::string context, Ptr<const Packet> packet)
{
  NS_LOG_FUNCTION (this << context << packet);

  SliceTag sliceTag;
  if (packet->PeekPacketTag (sliceTag))
    {
      uint16_t sliceId = sliceTag.GetSliceId ();
      TQosType qosType = sliceTag.GetTQosType ();

      Ptr<FlowStatsCalculator> slcStats = m_slices [sliceId].flowStats [qosType];
      slcStats->NotifyDrop (packet->GetSize (), FlowStatsCalculator::QUEUE);

      Ptr<FlowStatsCalculator> aggStats = m_slices [0].flowStats [qosType];
      aggStats->NotifyDrop (packet->GetSize (), FlowStatsCalculator::QUEUE);
    }
}

void
NetworkStatistics::TableDropPacket (
  std::string context, Ptr<const Packet> packet, uint8_t tableId)
{
  NS_LOG_FUNCTION (this << context << packet << static_cast<uint16_t> (tableId));

  SliceTag sliceTag;
  if (packet->PeekPacketTag (sliceTag))
    {
      uint16_t sliceId = sliceTag.GetSliceId ();
      TQosType qosType = sliceTag.GetTQosType ();

      Ptr<FlowStatsCalculator> slcStats = m_slices [sliceId].flowStats [qosType];
      slcStats->NotifyDrop (packet->GetSize (), FlowStatsCalculator::TABLE);

      Ptr<FlowStatsCalculator> aggStats = m_slices [0].flowStats [qosType];
      aggStats->NotifyDrop (packet->GetSize (), FlowStatsCalculator::TABLE);
    }
}

void
NetworkStatistics::TxPacket (std::string context, Ptr<const Packet> packet)
{
  NS_LOG_FUNCTION (this << context << packet);

  SliceTag sliceTag;
  if (packet->PeekPacketTag (sliceTag))
    {
      uint16_t sliceId = sliceTag.GetSliceId ();
      TQosType qosType = sliceTag.GetTQosType ();


      Ptr<FlowStatsCalculator> slcStats = m_slices [sliceId].flowStats [qosType];
      slcStats->NotifyTx (packet->GetSize ());

      Ptr<FlowStatsCalculator> aggStats = m_slices [0].flowStats [qosType];
      aggStats->NotifyTx (packet->GetSize ());
    }
}

void
NetworkStatistics::RxPacket (std::string context, Ptr<const Packet> packet)
{
  NS_LOG_FUNCTION (this << context << packet);

  SliceTag sliceTag;
  if (packet->PeekPacketTag (sliceTag))
    {
      uint16_t sliceId = sliceTag.GetSliceId ();
      TQosType qosType = sliceTag.GetTQosType ();

      Ptr<FlowStatsCalculator> slcStats = m_slices [sliceId].flowStats [qosType];
      slcStats->NotifyRx (packet->GetSize (), sliceTag.GetTimestamp ());

      Ptr<FlowStatsCalculator> aggStats = m_slices [0].flowStats [qosType];
      aggStats->NotifyRx (packet->GetSize (), sliceTag.GetTimestamp ());
    }
}

} // Namespace ns3
