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
 * Author: Thiago Guimarães <thiago.guimaraes@ice.ufjf.br>
 *         Luciano Chaves <luciano.chaves@ice.ufjf.br>
 */

#ifdef NS3_OFSWITCH13

#include "controller.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("Controller");
NS_OBJECT_ENSURE_REGISTERED (Controller);

Controller::Controller ()
{
  NS_LOG_FUNCTION (this);
}

Controller::~Controller ()
{
  NS_LOG_FUNCTION (this);
}

TypeId
Controller::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::Controller")
    .SetParent<OFSwitch13Controller> ()
    .AddConstructor<Controller> ()

    .AddAttribute ("ExtraStep", "Extra bit rate adjustment step.",
                   DataRateValue (DataRate ("12Mbps")),
                   MakeDataRateAccessor (&Controller::m_extraStep),
                   MakeDataRateChecker ())
    .AddAttribute ("GuardStep", "Link guard bit rate.",
                   DataRateValue (DataRate ("10Mbps")),
                   MakeDataRateAccessor (&Controller::m_guardStep),
                   MakeDataRateChecker ())
    .AddAttribute ("MeterStep", "Meter bit rate adjustment step.",
                   DataRateValue (DataRate ("1Mbps")),
                   MakeDataRateAccessor (&Controller::m_meterStep),
                   MakeDataRateChecker ())
    .AddAttribute ("SliceMode", "Inter-slice operation mode.",
                   TypeId::ATTR_GET | TypeId::ATTR_CONSTRUCT,
                   EnumValue (SliceMode::DYNA),
                   MakeEnumAccessor (&Controller::m_sliceMode),
                   MakeEnumChecker (SliceMode::NONE, SliceModeStr (SliceMode::NONE),
                                    SliceMode::SHAR, SliceModeStr (SliceMode::SHAR),
                                    SliceMode::STAT, SliceModeStr (SliceMode::STAT),
                                    SliceMode::DYNA, SliceModeStr (SliceMode::DYNA)))
    .AddAttribute ("SliceTimeout", "Inter-slice adjustment timeout.",
                   TimeValue (Seconds (20)),
                   MakeTimeAccessor (&Controller::m_sliceTimeout),
                   MakeTimeChecker ())
    .AddAttribute ("SpareUse", "Use spare link bit rate for sharing purposes.",
                   TypeId::ATTR_GET | TypeId::ATTR_CONSTRUCT,
                   EnumValue (OpMode::ON),
                   MakeEnumAccessor (&Controller::m_spareUse),
                   MakeEnumChecker (OpMode::OFF, OpModeStr (OpMode::OFF),
                                    OpMode::ON,  OpModeStr (OpMode::ON)))
  ;
  return tid;
}

void
Controller::NotifyHost (
  Ptr<OFSwitch13Port> swPort, Ptr<NetDevice> hostDev)
{
  NS_LOG_FUNCTION (this << swPort << hostDev);

  // Installing the forwarding rules to host
  std::ostringstream cmd;
  cmd << "flow-mod cmd=add,table=0,prio=1000"
      << " eth_type=0x0800,ip_dst=" << Ipv4AddressHelper::GetAddress (hostDev)
      << " write:output=" << swPort->GetPortNo ()
      << " goto:2";
  DpctlExecute (swPort->GetSwitchDevice ()->GetDpId (), cmd.str ());

  NS_LOG_INFO ("Host IPv4 address " << Ipv4AddressHelper::GetAddress (hostDev) <<
               " connected to port " << swPort->GetPortNo () <<
               " at switch with id " << swPort->GetSwitchDevice ()->GetDpId ());
}

void
Controller::NotifySwitches (
  OFSwitch13DeviceContainer switchDevices, PortsList_t switchesPorts)
{
  NS_LOG_FUNCTION (this);

  // Installing the forwarding rules between the switches.
  NS_ASSERT_MSG (switchDevices.GetN () == 3, "Invalid number of switches.");
  NS_ASSERT_MSG (switchesPorts.size () == 4, "Invalid number of ports.");
  NS_LOG_INFO ("Configuring connections between switches.");

  // Switch A - right (to switch B)
  std::ostringstream cmd1;
  cmd1 << "flow-mod cmd=add,table=0,prio=500"
       << " eth_type=0x0800,ip_dst=10.0.2.0/255.0.255.0"
       << " write:output=" << switchesPorts.at (0)->GetPortNo ()
       << " goto:1";
  DpctlExecute (switchDevices.Get (0)->GetDpId (), cmd1.str ());

  // Switch B - left (to switch A)
  std::ostringstream cmd2;
  cmd2 << "flow-mod cmd=add,table=0,prio=500"
       << " eth_type=0x0800,ip_dst=10.0.1.0/255.0.255.0"
       << " write:output=" << switchesPorts.at (1)->GetPortNo ()
       << " goto:1";
  DpctlExecute (switchDevices.Get (1)->GetDpId (), cmd2.str ());

  // Switch B - right (to switch C)
  std::ostringstream cmd3;
  cmd3 << "flow-mod cmd=add,table=0,prio=500"
       << " eth_type=0x0800,ip_dst=10.0.2.0/255.0.255.0"
       << " write:output=" << switchesPorts.at (2)->GetPortNo ()
       << " goto:1";
  DpctlExecute (switchDevices.Get (1)->GetDpId (), cmd3.str ());

  // Switch C - left (to switch B)
  std::ostringstream cmd4;
  cmd4 << "flow-mod cmd=add,table=0,prio=500"
       << " eth_type=0x0800,ip_dst=10.0.1.0/255.0.255.0"
       << " write:output=" << switchesPorts.at (3)->GetPortNo ()
       << " goto:1";
  DpctlExecute (switchDevices.Get (2)->GetDpId (), cmd4.str ());
}

void
Controller::NotifySlices (SliceInfoList_t slices)
{
  NS_LOG_FUNCTION (this << slices);

  // Iterate over slices
  for (auto const &slice : slices)
    {
      // Saving slices for further usage on this controller.
      m_slicesAll.push_back (slice);
      if (slice->GetSharing () == OpMode::ON)
        {
          m_slicesSha.push_back (slice);
        }

      // Iterate over links and configure the initial quotas.
      for (auto const &link : LinkInfo::GetList ())
        {
          bool success = true;
          success &= link->UpdateQuota (LinkInfo::FWD, slice->GetSliceId (), slice->GetQuota ());
          success &= link->UpdateQuota (LinkInfo::BWD, slice->GetSliceId (), slice->GetQuota ());
          NS_ASSERT_MSG (success, "Error when setting slice quotas.");
        }
    }

  // Sort slice in increasing priority order.
  std::stable_sort (m_slicesAll.begin (), m_slicesAll.end (), SliceInfo::PriorityComparator);
  std::stable_sort (m_slicesSha.begin (), m_slicesSha.end (), SliceInfo::PriorityComparator);

  // Iterate over slices in increasing priority order and dump information.
  for (auto const &slice : m_slicesAll)
    {
      std::cout << "SliceId = "   << slice->GetSliceId ()   << " | ";
      std::cout << "Prio = "      << slice->GetPriority ()  << " | ";
      std::cout << "Quota = "     << slice->GetQuota ()     << " | ";
      std::cout << "Sharing = "   << slice->GetSharing ()   << " | ";
      std::cout << "NumHostsA = " << slice->GetNumHostsA () << " | ";
      std::cout << "NumHostsB = " << slice->GetNumHostsB () << std::endl;
    }

  // Install inter-slicing meters, depending on the SliceMode attribute.
  switch (m_sliceMode)
    {
    case SliceMode::NONE:
      // No meter to install when inter-slicing is disabled.
      return;

    case SliceMode::SHAR:
      for (auto const &link : LinkInfo::GetList ())
        {
          // Install high-priority individual Non-GBR meter entries for slices
          // with disabled bandwidth sharing and the low-priority shared
          // Non-GBR meter entry for other slices.
          SlicingMeterInstall (link, SLICE_ALL);
          for (auto const &slice : GetSliceList ())
            {
              if (slice->GetSharing () == OpMode::OFF)
                {
                  SlicingMeterInstall (link, slice->GetSliceId ());
                }
            }
        }
      break;

    case SliceMode::STAT:
    case SliceMode::DYNA:
      for (auto const &link : LinkInfo::GetList ())
        {
          // Install individual Non-GBR meter entries for all slices.
          for (auto const &slice : GetSliceList ())
            {
              SlicingMeterInstall (link, slice->GetSliceId ());
            }
        }
      break;

    default:
      NS_LOG_WARN ("Undefined inter-slicing operation mode.");
      break;
    }
}

const SliceInfoList_t&
Controller::GetSliceList (bool sharing) const
{
  NS_LOG_FUNCTION (this << sharing);

  return (sharing ? m_slicesSha : m_slicesAll);
}

void
Controller::DoDispose ()
{
  NS_LOG_FUNCTION (this);

  m_slicesAll.clear ();
  m_slicesSha.clear ();
  OFSwitch13Controller::DoDispose ();
}

void
Controller::NotifyConstructionCompleted (void)
{
  NS_LOG_FUNCTION (this);

  // Schedule the first slicing extra timeout operation only when in
  // dynamic inter-slicing operation mode.
  if (m_sliceMode == SliceMode::DYNA)
    {
      Simulator::Schedule (m_sliceTimeout, &Controller::SlicingDynamicTimeout, this);
    }

  OFSwitch13Controller::NotifyConstructionCompleted ();
}

ofl_err
Controller::HandleError (
  struct ofl_msg_error *msg, Ptr<const RemoteSwitch> swtch, uint32_t xid)
{
  NS_LOG_FUNCTION (this << swtch << xid);

  // Print the message.
  char *cStr = ofl_msg_to_string ((struct ofl_msg_header*)msg, 0);
  std::string msgStr (cStr);
  free (cStr);

  // All handlers must free the message when everything is ok.
  ofl_msg_free ((struct ofl_msg_header*)msg, 0);

  // Logging this error message on the standard error stream and continue.
  Config::SetGlobal ("SeeCerr", BooleanValue (true));
  std::cerr << Simulator::Now ().GetSeconds ()
            << " Controller received message xid " << xid
            << " from switch id " << swtch->GetDpId ()
            << " with error message: " << msgStr
            << std::endl;
  return 0;
}

ofl_err
Controller::HandleFlowRemoved (
  struct ofl_msg_flow_removed *msg, Ptr<const RemoteSwitch> swtch, uint32_t xid)
{
  NS_LOG_FUNCTION (this << swtch << xid);

  // Print the message.
  char *cStr = ofl_msg_to_string ((struct ofl_msg_header*)msg, 0);
  std::string msgStr (cStr);
  free (cStr);

  // All handlers must free the message when everything is ok
  ofl_msg_free_flow_removed (msg, true, 0);
  return 0;
}

ofl_err
Controller::HandlePacketIn (
  struct ofl_msg_packet_in *msg, Ptr<const RemoteSwitch> swtch, uint32_t xid)
{
  NS_LOG_FUNCTION (this << swtch << xid);

  // Print the message.
  char *cStr = ofl_structs_match_to_string (msg->match, 0);
  std::string msgStr (cStr);
  free (cStr);

  // All handlers must free the message when everything is ok.
  ofl_msg_free ((struct ofl_msg_header*)msg, 0);

  // Logging this packet-in message on the standard error stream and continue.
  Config::SetGlobal ("SeeCerr", BooleanValue (true));
  std::cerr << Simulator::Now ().GetSeconds ()
            << " Controller received message xid " << xid
            << " from switch id " << swtch->GetDpId ()
            << " with packet-in message: " << msgStr
            << std::endl;
  return 0;
}

void
Controller::HandshakeSuccessful (Ptr<const RemoteSwitch> swtch)
{
  NS_LOG_FUNCTION (this << swtch);

  // Get the switch datapath ID
  uint64_t swDpId = swtch->GetDpId ();

  // Configure te switch to buffer packets and send only the first 128 bytes of
  // each packet sent to the controller when not using an output action to the
  // OFPP_CONTROLLER logical port.
  DpctlExecute (swDpId, "set-config miss=128");

  // Install the table-miss entries.
  DpctlExecute (swDpId, "flow-mod cmd=add,table=0,prio=0 apply:output=ctrl:128");
  DpctlExecute (swDpId, "flow-mod cmd=add,table=1,prio=0 goto:2");
  DpctlExecute (swDpId, "flow-mod cmd=add,table=2,prio=0 ");

  // Flood all ARP packets
  DpctlExecute (swDpId, "flow-mod cmd=add,table=0,prio=10 eth_type=0x0806 apply:output=flood");

  // Install rules for QoS output queues.
  for (auto const &it : Dscp2QueueMap ())
    {
      std::ostringstream cmd;
      cmd << "flow-mod cmd=add,prio=32"
          << ",table="        << OFS_TAB_QUEUE
          << ",flags="        << FLAGS_REMOVED_OVERLAP_RESET
          << " eth_type="     << IPV4_PROT_NUM
          << ",ip_dscp="      << static_cast<uint16_t> (it.first)
          << " write:queue="  << static_cast<uint32_t> (it.second);
      DpctlExecute (swDpId, cmd.str ());
    }
}

void
Controller::SlicingDynamicTimeout (void)
{
  NS_LOG_FUNCTION (this);

  // Adjust the extra bit rates in both directions for each backhaul link.
  for (auto &link : LinkInfo::GetList ())
    {
      for (int d = 0; d < N_LINK_DIRS; d++)
        {
          SlicingExtraAdjust (link, static_cast<LinkInfo::LinkDir> (d));
        }
    }

  // Schedule the next slicing extra timeout operation.
  Simulator::Schedule (m_sliceTimeout, &Controller::SlicingDynamicTimeout, this);
}

void
Controller::SlicingExtraAdjust (
  Ptr<LinkInfo> link, LinkInfo::LinkDir dir)
{
  NS_LOG_FUNCTION (this << link << dir);

  NS_ASSERT_MSG (m_sliceMode == SliceMode::DYNA, "Invalid inter-slice operation mode.");

  int64_t stepRate = static_cast<int64_t> (m_extraStep.GetBitRate ());
  NS_ASSERT_MSG (stepRate > 0, "Invalid ExtraStep attribute value.");

  // Iterate over slices with enabled bandwidth sharing
  // to sum the quota bit rate and the used bit rate.
  int64_t maxShareBitRate = 0;
  int64_t useShareBitRate = 0;
  for (auto const &slice : GetSliceList (true))
    {
      maxShareBitRate += link->GetQuoBitRate (dir, slice->GetSliceId ());
      useShareBitRate += link->GetUseBitRate (dir, slice->GetSliceId ());
    }
  // When enable, sum the spare bit rate too.
  if (m_spareUse == OpMode::ON)
    {
      maxShareBitRate += link->GetQuoBitRate (dir, SLICE_UNKN);
    }

  // Get the idle bit rate (apart from the guard bit rate) that can be used as
  // extra bit rate by overloaded slices.
  int64_t guardBitRate = static_cast<int64_t> (m_guardStep.GetBitRate ());
  int64_t idlShareBitRate = maxShareBitRate - guardBitRate - useShareBitRate;

  if (idlShareBitRate > 0)
    {
      // We have some unused bit rate step that can be distributed as extra to
      // any overloaded slice. Iterate over slices with enabled bandwidth
      // sharing in decreasing priority order, assigning one extra bit rate to
      // those slices that may benefit from it. Also, gets back one extra bit
      // rate from underloaded slices to reduce unnecessary overbooking.
      for (auto it = m_slicesSha.rbegin (); it != m_slicesSha.rend (); ++it)
        {
          // Get the idle and extra bit rates for this slice.
          uint16_t sliceId = (*it)->GetSliceId ();
          int64_t sliceIdl = link->GetIdlBitRate (dir, sliceId);
          int64_t sliceExt = link->GetExtBitRate (dir, sliceId);
          NS_LOG_DEBUG ("Current slice " << sliceId <<
                        " direction "    << LinkInfo::LinkDirStr (dir) <<
                        " extra "        << sliceExt <<
                        " idle "         << sliceIdl);

          if (sliceIdl < (stepRate / 2) && idlShareBitRate >= stepRate)
            {
              // This is an overloaded slice and we have idle bit rate.
              // Increase the slice extra bit rate by one step.
              NS_LOG_DEBUG ("Increase extra bit rate.");
              bool success = link->UpdateExtBitRate (dir, sliceId, stepRate);
              NS_ASSERT_MSG (success, "Error when updating extra bit rate.");
              idlShareBitRate -= stepRate;
            }
          else if (sliceIdl >= (stepRate * 2) && sliceExt >= stepRate)
            {
              // This is an underloaded slice with some extra bit rate.
              // Decrease the slice extra bit rate by one step.
              NS_LOG_DEBUG ("Decrease extra bit rate overbooking.");
              bool success = link->UpdateExtBitRate (dir, sliceId, -stepRate);
              NS_ASSERT_MSG (success, "Error when updating extra bit rate.");
            }
        }
    }
  else
    {
      // Link usage is over the safeguard threshold. First, iterate over slices
      // with enable bandwidth sharing and get back any unused extra bit rate
      // to reduce unnecessary overbooking.
      for (auto const &slice : GetSliceList (true))
        {
          // Get the idle and extra bit rates for this slice.
          uint16_t sliceId = slice->GetSliceId ();
          int64_t sliceIdl = link->GetIdlBitRate (dir, sliceId);
          int64_t sliceExt = link->GetExtBitRate (dir, sliceId);
          NS_LOG_DEBUG ("Current slice " << sliceId <<
                        " direction "    << LinkInfo::LinkDirStr (dir) <<
                        " extra "        << sliceExt <<
                        " idle "         << sliceIdl);

          // Remove all unused extra bit rate (step by step) from this slice.
          while (sliceIdl >= stepRate && sliceExt >= stepRate)
            {
              NS_LOG_DEBUG ("Decrease extra bit rate overbooking.");
              bool success = link->UpdateExtBitRate (dir, sliceId, -stepRate);
              NS_ASSERT_MSG (success, "Error when updating extra bit rate.");
              sliceIdl -= stepRate;
              sliceExt -= stepRate;
            }
        }

      // At this point there is no slices with more than one step of unused
      // extra bit rate. Now, iterate again over slices with enabled bandwidth
      // sharing in increasing priority order, removing some extra bit rate
      // from those slices that are using more than its quota to get the link
      // usage below the safeguard threshold again.
      bool removedFlag = false;
      auto it = m_slicesSha.begin ();
      auto sp = m_slicesSha.begin ();
      while (it != m_slicesSha.end () && idlShareBitRate < 0)
        {
          // Check if the slice priority has increased to update the sp.
          if ((*it)->GetPriority () > (*sp)->GetPriority ())
            {
              NS_ASSERT_MSG (!removedFlag, "Inconsistent removed flag.");
              sp = it;
            }

          // Get the idle and extra bit rates for this slice.
          uint16_t sliceId = (*it)->GetSliceId ();
          int64_t sliceIdl = link->GetIdlBitRate (dir, sliceId);
          int64_t sliceExt = link->GetExtBitRate (dir, sliceId);
          NS_LOG_DEBUG ("Current slice " << sliceId <<
                        " direction "    << LinkInfo::LinkDirStr (dir) <<
                        " extra "        << sliceExt <<
                        " idle "         << sliceIdl);

          // If possible, decrease the slice extra bit rate by one step.
          if (sliceExt >= stepRate)
            {
              removedFlag = true;
              NS_ASSERT_MSG (sliceIdl < stepRate, "Inconsistent bit rate.");
              NS_LOG_DEBUG ("Decrease extra bit rate for congested link.");
              bool success = link->UpdateExtBitRate (dir, sliceId, -stepRate);
              NS_ASSERT_MSG (success, "Error when updating extra bit rate.");
              idlShareBitRate += stepRate - sliceIdl;
            }

          // Select the slice for the next loop iteration.
          auto nextIt = std::next (it);
          bool isLast = (nextIt == m_slicesSha.end ());
          if ((!isLast && (*nextIt)->GetPriority () == (*it)->GetPriority ())
              || (removedFlag == false))
            {
              // Go to the next slice if it has the same priority of the
              // current one or if no more extra bit rate can be recovered from
              // slices with the current priority.
              it = nextIt;
            }
          else
            {
              // Go back to the first slice with the current priority (can be
              // the current slice) and reset the removed flag.
              NS_ASSERT_MSG (removedFlag, "Inconsistent removed flag.");
              it = sp;
              removedFlag = false;
            }
        }
    }

  // Update the slicing meters for all slices over this link.
  for (auto const &slice : GetSliceList (true))
    {
      SlicingMeterAdjust (link, slice->GetSliceId ());
    }
}

void
Controller::SlicingMeterApply (Ptr<LinkInfo> link, int sliceId)
{
  NS_LOG_FUNCTION (this << link << sliceId);

  // Create meter id for both link directions.
  // FWD direction: switches A -> B -> C
  // BWD direction: switches A <- B <- C
  uint32_t meterIdFwd = MeterIdSlcCreate (sliceId, static_cast<int> (LinkInfo::FWD));
  uint32_t meterIdBwd = MeterIdSlcCreate (sliceId, static_cast<int> (LinkInfo::BWD));

  // Get OpenFlow switch datapath ID for both link directions.
  uint64_t dpIdFwd = link->GetSwDpId (static_cast<int> (LinkInfo::FWD));
  uint64_t dpIdBwd = link->GetSwDpId (static_cast<int> (LinkInfo::BWD));

  // -------------------------------------------------------------------------
  // Bandwidth table -- [from higher to lower priority]
  //
  // Build the command string.
  // Using a low-priority rule for ALL slice.
  std::ostringstream cmd;
  cmd << "flow-mod cmd=add"
      << ",prio="   << (sliceId == SLICE_ALL ? 32 : 64)
      << ",table="  << OFS_TAB_METER
      << ",flags="  << FLAGS_REMOVED_OVERLAP_RESET;

  // Build the match string for forward direction.
  std::ostringstream mtcFwd;
  mtcFwd << " eth_type="   << IPV4_PROT_NUM
         << ",ip_dst=10."  << sliceId
         << ".2.0/255.255.255.0"
         << " meter:"      << meterIdFwd
         << " goto:"       << OFS_TAB_QUEUE;
  DpctlExecute (dpIdFwd, cmd.str () + mtcFwd.str ());

  // Build the match string for backward direction.
  std::ostringstream mtcBwd;
  mtcBwd << " eth_type="   << IPV4_PROT_NUM
         << ",ip_dst=10."  << sliceId
         << ".1.0/255.255.255.0"
         << " meter:"      << meterIdBwd
         << " goto:"       << OFS_TAB_QUEUE;
  DpctlExecute (dpIdBwd, cmd.str () + mtcBwd.str ());
}

void
Controller::SlicingMeterAdjust (
  Ptr<LinkInfo> link, int sliceId)
{
  NS_LOG_FUNCTION (this << link << sliceId);

  // Update inter-slicing meter, depending on the SliceMode attribute.
  NS_ASSERT_MSG (sliceId < SLICE_ALL, "Invalid slice for this operation.");
  switch (m_sliceMode)
    {
    case SliceMode::NONE:
      // Nothing to do when inter-slicing is disabled.
      return;

    case SliceMode::SHAR:
      // Identify the Non-GBR meter entry to adjust: individual or shared.
      if (SliceInfo::GetPointer (sliceId)->GetSharing () == OpMode::ON)
        {
          sliceId = SLICE_ALL;
        }
      break;

    case SliceMode::STAT:
    case SliceMode::DYNA:
      // Update the individual Non-GBR meter entry.
      break;

    default:
      NS_LOG_WARN ("Undefined inter-slicing operation mode.");
      break;
    }

  // Check for updated slicing meters in both link directions.
  for (int d = 0; d < N_LINK_DIRS; d++)
    {
      LinkInfo::LinkDir dir = static_cast<LinkInfo::LinkDir> (d);

      // Calculate the meter bit rate.
      int64_t meterBitRate = 0;
      if (sliceId == SLICE_ALL)
        {
          // Iterate over slices with enabled bandwidth sharing
          // to sum the quota bit rate.
          for (auto const &slice : GetSliceList (true))
            {
              meterBitRate += link->GetUnrBitRate (dir, slice->GetSliceId ());
            }
          // When enable, sum the spare bit rate too.
          if (m_spareUse == OpMode::ON)
            {
              meterBitRate += link->GetUnrBitRate (dir, SLICE_UNKN);
            }
        }
      else
        {
          meterBitRate = link->GetUnrBitRate (dir, sliceId);
        }

      int64_t currBitRate = link->GetMetBitRate (dir, sliceId);
      uint64_t diffBitRate = std::abs (currBitRate - meterBitRate);
      NS_LOG_DEBUG ("Current slice " << sliceId <<
                    " direction "    << LinkInfo::LinkDirStr (dir) <<
                    " diff rate "    << diffBitRate);

      if (diffBitRate >= m_meterStep.GetBitRate ())
        {
          uint32_t meterId = MeterIdSlcCreate (sliceId, d);
          int64_t meterKbps = Bps2Kbps (meterBitRate);
          bool success = link->SetMetBitRate (dir, sliceId, meterKbps * 1000);
          NS_ASSERT_MSG (success, "Error when setting meter bit rate.");

          NS_LOG_INFO ("Update slice " << sliceId <<
                       " direction "   << LinkInfo::LinkDirStr (dir) <<
                       " meter ID "    << GetUint32Hex (meterId) <<
                       " bitrate "     << meterKbps << " Kbps");

          std::ostringstream cmd;
          cmd << "meter-mod cmd=mod"
              << ",flags="      << OFPMF_KBPS
              << ",meter="      << meterId
              << " drop:rate="  << meterKbps;
          DpctlExecute (link->GetSwDpId (d), cmd.str ());
        }
    }
}

void
Controller::SlicingMeterInstall (Ptr<LinkInfo> link, int sliceId)
{
  NS_LOG_FUNCTION (this << link << sliceId);

  NS_ASSERT_MSG (m_sliceMode != SliceMode::NONE,
                 "Invalid inter-slice operation mode.");

  // Install slicing meters in both link directions.
  for (int d = 0; d < N_LINK_DIRS; d++)
    {
      LinkInfo::LinkDir dir = static_cast<LinkInfo::LinkDir> (d);

      // Calculate the meter bit rate.
      int64_t meterBitRate = 0;
      if (sliceId == SLICE_ALL)
        {
          NS_ASSERT_MSG (m_sliceMode == SliceMode::SHAR,
                         "Invalid inter-slice operation mode.");

          // Iterate over slices with enabled bandwidth sharing
          // to sum the quota bit rate.
          for (auto const &slice : GetSliceList (true))
            {
              meterBitRate += link->GetQuoBitRate (dir, slice->GetSliceId ());
            }
          // When enable, sum the spare bit rate too.
          if (m_spareUse == OpMode::ON)
            {
              meterBitRate += link->GetQuoBitRate (dir, SLICE_UNKN);
            }
        }
      else
        {
          meterBitRate = link->GetQuoBitRate (dir, sliceId);
        }

      uint32_t meterId = MeterIdSlcCreate (sliceId, d);
      int64_t meterKbps = Bps2Kbps (meterBitRate);
      bool success = link->SetMetBitRate (dir, sliceId, meterKbps * 1000);
      NS_ASSERT_MSG (success, "Error when setting meter bit rate.");

      NS_LOG_INFO ("Create slice " << sliceId <<
                   " direction "   << LinkInfo::LinkDirStr (dir) <<
                   " meter ID "    << GetUint32Hex (meterId) <<
                   " bitrate "     << meterKbps << " Kbps");

      std::ostringstream cmd;
      cmd << "meter-mod cmd=add"
          << ",flags="      << OFPMF_KBPS
          << ",meter="      << meterId
          << " drop:rate="  << meterKbps;
      DpctlExecute (link->GetSwDpId (d), cmd.str ());
    }

  // Install the rules to apply the meters we just created.
  SlicingMeterApply (link, sliceId);
}

} // namespace ns3
#endif // NS3_OFSWITCH13
