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

/********** Public methods ***********/
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

    .AddAttribute ("ExtraStep",
                   "Extra bit rate adjustment step.",
                   DataRateValue (DataRate ("12Mbps")),
                   MakeDataRateAccessor (&Controller::m_extraStep),
                   MakeDataRateChecker ())
    .AddAttribute ("GuardStep",
                   "Link guard bit rate.",
                   DataRateValue (DataRate ("10Mbps")),
                   MakeDataRateAccessor (&Controller::m_guardStep),
                   MakeDataRateChecker ())
    .AddAttribute ("MeterStep",
                   "Meter bit rate adjustment step.",
                   DataRateValue (DataRate ("2Mbps")),
                   MakeDataRateAccessor (&Controller::m_meterStep),
                   MakeDataRateChecker ())

    .AddAttribute ("SliceMode",
                   "Inter-slice operation mode.",
                   TypeId::ATTR_GET | TypeId::ATTR_CONSTRUCT,
                   EnumValue (SliceMode::DYNA),
                   MakeEnumAccessor (&Controller::m_sliceMode),
                   MakeEnumChecker (SliceMode::NONE,
                                    SliceModeStr (SliceMode::NONE),
                                    SliceMode::SHAR,
                                    SliceModeStr (SliceMode::SHAR),
                                    SliceMode::STAT,
                                    SliceModeStr (SliceMode::STAT),
                                    SliceMode::DYNA,
                                    SliceModeStr (SliceMode::DYNA)))
    .AddAttribute ("SliceTimeout",
                   "Inter-slice adjustment timeout.",
                   TimeValue (Seconds (20)),
                   MakeTimeAccessor (&Controller::m_sliceTimeout),
                   MakeTimeChecker ())

    .AddAttribute ("SpareUse",
                   "Use spare link bit rate for sharing purposes.",
                   TypeId::ATTR_GET | TypeId::ATTR_CONSTRUCT,
                   EnumValue (OpMode::ON),
                   MakeEnumAccessor (&Controller::m_spareUse),
                   MakeEnumChecker (OpMode::OFF, OpModeStr (OpMode::OFF),
                                    OpMode::ON,  OpModeStr (OpMode::ON)))


  ;
  return tid;
}




void
Controller::NotifyClientsServers (TopologyIfaces_t sliceInterfaces,
                                  TopologyPorts_t switchPorts)
{

  m_numberSlices = sliceInterfaces.size ();

  //Go through slices
  for (size_t sl = 0; sl < m_numberSlices; sl++)
    {

      //Switch between HostsSWA/HostsSWB/Servers
      for (size_t sw = 0; sw < 3; sw++)
        {

          for (size_t iface = 0; iface < sliceInterfaces[sl][sw].GetN (); iface++)
            {

              //Address/Port pair
              Ipv4Address address = sliceInterfaces[sl][sw].GetAddress (iface);
              Ptr<OFSwitch13Port> port = switchPorts[sl][sw][iface];

              //Install the rule on switches
              std::ostringstream cmd;
              cmd << "flow-mod cmd=add,table=0,prio=1000"
                  << " eth_type=0x0800,ip_dst=" << address
                  << " write:output=" << port->GetPortNo ()
                  << " goto:2"; //Skip meter table
              DpctlExecute (port->GetSwitchDevice ()->GetDpId (), cmd.str ());


            }
        }

    }

}


void
Controller::NotifySwitches (PortsVector_t interSwitchesPorts,
                            OFSwitch13DeviceContainer switchDevices)
{


  // Installing the fowarding rules between switches.

  //SWA
  std::ostringstream cmd;
  cmd << "flow-mod cmd=add,table=0,prio=500"
      << " eth_type=0x0800,ip_dst=10.0.2.0/255.0.255.0"
      << " write:output=" << interSwitchesPorts[0]->GetPortNo ()
      << " goto:1";
  DpctlExecute (switchDevices.Get (0)->GetDpId (), cmd.str ());

  //SWB
  std::ostringstream cmd2;
  cmd2 << "flow-mod cmd=add,table=0,prio=500"
       << " eth_type=0x0800,ip_dst=10.0.1.0/255.0.255.0"
       << " write:output=" << interSwitchesPorts[1]->GetPortNo ()
       << " goto:1";
  DpctlExecute (switchDevices.Get (1)->GetDpId (), cmd2.str ());

  //SWB
  std::ostringstream cmd3;
  cmd3 << "flow-mod cmd=add,table=0,prio=500"
       << " eth_type=0x0800,ip_dst=10.0.2.0/255.0.255.0"
       << " write:output=" << interSwitchesPorts[2]->GetPortNo ()
       << " goto:1";
  DpctlExecute (switchDevices.Get (1)->GetDpId (), cmd3.str ());


  //Server's Switch
  std::ostringstream cmd4;
  cmd4 << "flow-mod cmd=add,table=0,prio=500"
       << " eth_type=0x0800,ip_dst=10.0.1.0/255.0.255.0"
       << " write:output=" << interSwitchesPorts[3]->GetPortNo ()
       << " goto:1";
  DpctlExecute (switchDevices.Get (2)->GetDpId (), cmd4.str ());



  //Get the pointers to link informations
  m_lInfoA = LinkInfo::GetPointer (1, 2);
  m_lInfoB = LinkInfo::GetPointer (2, 3);


}


void
Controller::ConfigureMeters (std::vector<Ptr<SliceInfo>> slices)
{


  for(std::vector<Ptr<SliceInfo>>::iterator it = slices.begin(); it != slices.end(); ++it)
    {
      // Install slicing meters in both link directions.
      for (int d = 0; d < N_LINK_DIRS; d++)
        {
          LinkInfo::LinkDir dir = static_cast<LinkInfo::LinkDir> (d);

          bool success = true;
          success &= m_lInfoA->UpdateQuota (dir, (*it)->GetSliceId(), (*it)->GetQuota());
          success &= m_lInfoB->UpdateQuota (dir, (*it)->GetSliceId(), (*it)->GetQuota());
          NS_ASSERT_MSG (success, "Error when setting slice quotas.");

        }
    }

  // ---------------------------------------------------------------------
  // Meter table
  //
  // Install inter-slicing meters, depending on the InterSliceMode attribute.
  switch (m_sliceMode)
    {
    case SliceMode::NONE:
      // Nothing to do when inter-slicing is disabled.
      return;

    case SliceMode::SHAR:
      for (auto &lInfo : LinkInfo::GetList ())
        {
          // Install high-priority individual Non-GBR meter entries for slices
          // with disabled bandwidth sharing and the low-priority shared
          // Non-GBR meter entry for other slices.
          SlicingMeterInstall (lInfo, SLICE_ALL);

        }
      break;

    case SliceMode::STAT:
    case SliceMode::DYNA:
      for (auto &lInfo : LinkInfo::GetList ())
        {
          // Install individual Non-GBR meter entries.
          for (size_t slc = 0; slc < m_numberSlices; slc++)
            {
              SlicingMeterInstall (lInfo, slc);
            }
        }
      break;

    default:
      NS_LOG_WARN ("Undefined inter-slicing operation mode.");
      break;
    }

  //
  for (size_t sliceId = 0; sliceId < m_numberSlices; sliceId++)
    {
      // Install slicing meters in both link directions.
      SlicingMeterApply (m_lInfoA, sliceId);
      SlicingMeterApply (m_lInfoB, sliceId);
    }


}


/********** Protected methods **********/

void
Controller::DoDispose ()
{
  NS_LOG_FUNCTION (this);

  OFSwitch13Controller::DoDispose ();
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

  // After a successfull handshake, let's install the table-miss entries.
  DpctlExecute (swDpId, "flow-mod cmd=add,table=0,prio=0 apply:output=ctrl:128");
  DpctlExecute (swDpId, "flow-mod cmd=add,table=1,prio=0 goto:2");
  DpctlExecute (swDpId, "flow-mod cmd=add,table=2,prio=0 ");

  // Flooding all ARP packets
  DpctlExecute (swDpId, "flow-mod cmd=add,table=0,prio=10 eth_type=0x0806 apply:output=flood");

  // QoS output queues rules.
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

/********** Private methods **********/

void
Controller::SlicingDynamicTimeout (void)
{
  NS_LOG_FUNCTION (this);

  // Adjust the extra bit rates in both directions for each backhaul link.
  for (auto &lInfo : LinkInfo::GetList ())
    {
      for (int d = 0; d < N_LINK_DIRS; d++)
        {
          SlicingExtraAdjust (lInfo, static_cast<LinkInfo::LinkDir> (d));
        }
    }

  // Schedule the next slicing extra timeout operation.
  Simulator::Schedule (m_sliceTimeout, &Controller::SlicingDynamicTimeout, this);
}

void
Controller::SlicingExtraAdjust (
  Ptr<LinkInfo> lInfo, LinkInfo::LinkDir dir)
{
  NS_LOG_FUNCTION (this << lInfo << dir);

//   NS_ASSERT_MSG (m_sliceMode == SliceMode::DYNA,
//                  "Invalid inter-slice operation mode.");

//   const LinkInfo::EwmaTerm lTerm = LinkInfo::LTERM;
//   int64_t stepRate = static_cast<int64_t> (m_extraStep.GetBitRate ());
//   NS_ASSERT_MSG (stepRate > 0, "Invalid ExtraStep attribute value.");

//   // Iterate over slices with enabled bandwidth sharing
//   // to sum the quota bit rate and the used bit rate.
//   int64_t maxShareBitRate = 0;
//   int64_t useShareBitRate = 0;
//   for (auto const &ctrl : GetSliceControllerList (true))
//     {
//       SliceId slice = ctrl->GetSliceId ();
//       maxShareBitRate += lInfo->GetQuoBitRate (dir, slice);
//       useShareBitRate += lInfo->GetUseBitRate (lTerm, dir, slice);
//     }
//   // When enable, sum the spare bit rate too.
//   if (m_spareUse == OpMode::ON)
//     {
//       maxShareBitRate += lInfo->GetQuoBitRate (dir, SliceId::UNKN);
//     }

//   // Get the idle bit rate (apart from the guard bit rate) that can be used as
//   // extra bit rate by overloaded slices.
//   int64_t guardBitRate = static_cast<int64_t> (m_guardStep.GetBitRate ());
//   int64_t idlShareBitRate = maxShareBitRate - guardBitRate - useShareBitRate;

//   if (idlShareBitRate > 0)
//     {
//       // We have some unused bit rate step that can be distributed as extra to
//       // any overloaded slice. Iterate over slices with enabled bandwidth
//       // sharing in decreasing priority order, assigning one extra bit rate to
//       // those slices that may benefit from it. Also, gets back one extra bit
//       // rate from underloaded slices to reduce unnecessary overbooking.
//       for (auto it = m_sliceCtrlsSha.rbegin ();
//            it != m_sliceCtrlsSha.rend (); ++it)
//         {
//           // Get the idle and extra bit rates for this slice.
//           SliceId slice = (*it)->GetSliceId ();
//           int64_t sliceIdl = lInfo->GetIdlBitRate (lTerm, dir, slice);
//           int64_t sliceExt = lInfo->GetExtBitRate (dir, slice);
//           NS_LOG_DEBUG ("Current slice " << SliceIdStr (slice) <<
//                         " direction "    << LinkInfo::LinkDirStr (dir) <<
//                         " extra "        << sliceExt <<
//                         " idle "         << sliceIdl);

//           if (sliceIdl < (stepRate / 2) && idlShareBitRate >= stepRate)
//             {
//               // This is an overloaded slice and we have idle bit rate.
//               // Increase the slice extra bit rate by one step.
//               NS_LOG_DEBUG ("Increase extra bit rate.");
//               bool success = lInfo->UpdateExtBitRate (dir, slice, stepRate);
//               NS_ASSERT_MSG (success, "Error when updating extra bit rate.");
//               idlShareBitRate -= stepRate;
//             }
//           else if (sliceIdl >= (stepRate * 2) && sliceExt >= stepRate)
//             {
//               // This is an underloaded slice with some extra bit rate.
//               // Decrease the slice extra bit rate by one step.
//               NS_LOG_DEBUG ("Decrease extra bit rate overbooking.");
//               bool success = lInfo->UpdateExtBitRate (dir, slice, -stepRate);
//               NS_ASSERT_MSG (success, "Error when updating extra bit rate.");
//             }
//         }
//     }
//   else
//     {
//       // Link usage is over the safeguard threshold. First, iterate over slices
//       // with enable bandwidth sharing and get back any unused extra bit rate
//       // to reduce unnecessary overbooking.
//       for (auto const &ctrl : GetSliceControllerList (true))
//         {
//           // Get the idle and extra bit rates for this slice.
//           SliceId slice = ctrl->GetSliceId ();
//           int64_t sliceIdl = lInfo->GetIdlBitRate (lTerm, dir, slice);
//           int64_t sliceExt = lInfo->GetExtBitRate (dir, slice);
//           NS_LOG_DEBUG ("Current slice " << SliceIdStr (slice) <<
//                         " direction "    << LinkInfo::LinkDirStr (dir) <<
//                         " extra "        << sliceExt <<
//                         " idle "         << sliceIdl);

//           // Remove all unused extra bit rate (step by step) from this slice.
//           while (sliceIdl >= stepRate && sliceExt >= stepRate)
//             {
//               NS_LOG_DEBUG ("Decrease extra bit rate overbooking.");
//               bool success = lInfo->UpdateExtBitRate (dir, slice, -stepRate);
//               NS_ASSERT_MSG (success, "Error when updating extra bit rate.");
//               sliceIdl -= stepRate;
//               sliceExt -= stepRate;
//             }
//         }

//       // At this point there is no slices with more than one step of unused
//       // extra bit rate. Now, iterate again over slices with enabled bandwidth
//       // sharing in increasing priority order, removing some extra bit rate
//       // from those slices that are using more than its quota to get the link
//       // usage below the safeguard threshold again.
//       bool removedFlag = false;
//       auto it = m_sliceCtrlsSha.begin ();
//       auto sp = m_sliceCtrlsSha.begin ();
//       while (it != m_sliceCtrlsSha.end () && idlShareBitRate < 0)
//         {
//           // Check if the slice priority has increased to update the sp.
//           if ((*it)->GetPriority () > (*sp)->GetPriority ())
//             {
//               NS_ASSERT_MSG (!removedFlag, "Inconsistent removed flag.");
//               sp = it;
//             }

//           // Get the idle and extra bit rates for this slice.
//           SliceId slice = (*it)->GetSliceId ();
//           int64_t sliceIdl = lInfo->GetIdlBitRate (lTerm, dir, slice);
//           int64_t sliceExt = lInfo->GetExtBitRate (dir, slice);
//           NS_LOG_DEBUG ("Current slice " << SliceIdStr (slice) <<
//                         " direction "    << LinkInfo::LinkDirStr (dir) <<
//                         " extra "        << sliceExt <<
//                         " idle "         << sliceIdl);

//           // If possible, decrease the slice extra bit rate by one step.
//           if (sliceExt >= stepRate)
//             {
//               removedFlag = true;
//               NS_ASSERT_MSG (sliceIdl < stepRate, "Inconsistent bit rate.");
//               NS_LOG_DEBUG ("Decrease extra bit rate for congested link.");
//               bool success = lInfo->UpdateExtBitRate (dir, slice, -stepRate);
//               NS_ASSERT_MSG (success, "Error when updating extra bit rate.");
//               idlShareBitRate += stepRate - sliceIdl;
//             }

//           // Select the slice for the next loop iteration.
//           auto nextIt = std::next (it);
//           bool isLast = (nextIt == m_sliceCtrlsSha.end ());
//           if ((!isLast && (*nextIt)->GetPriority () == (*it)->GetPriority ())
//               || (removedFlag == false))
//             {
//               // Go to the next slice if it has the same priority of the
//               // current one or if no more extra bit rate can be recovered from
//               // slices with the current priority.
//               it = nextIt;
//             }
//           else
//             {
//               // Go back to the first slice with the current priority (can be
//               // the current slice) and reset the removed flag.
//               NS_ASSERT_MSG (removedFlag, "Inconsistent removed flag.");
//               it = sp;
//               removedFlag = false;
//             }
//         }
//     }

//   // Update the slicing meters for all slices over this link.
//   for (auto const &ctrl : GetSliceControllerList (true))
//     {
//       SlicingMeterAdjust (lInfo, ctrl->GetSliceId ());
//     }
}

void
Controller::SlicingMeterApply (Ptr<LinkInfo> lInfo, int sliceId)
{
  NS_LOG_FUNCTION (this << lInfo << sliceId);

  // -------------------------------------------------------------------------
  // Bandwidth table -- [from higher to lower priority]
  //
  // Build the command string.
  // Using a low-priority rule for ALL slice.
  std::ostringstream cmd;
  cmd << "flow-mod cmd=add"
      << ",prio="       << (sliceId == SLICE_ALL ? 32 : 64)
      << ",table="      << OFS_TAB_METER
      << ",flags="      << FLAGS_REMOVED_OVERLAP_RESET;


  uint32_t meterId = MeterIdSlcCreate (sliceId, static_cast<int> (LinkInfo::FWD));

  // Build the match string.
  std::ostringstream mtc;
  mtc << " eth_type="   << IPV4_PROT_NUM
      << ",ip_dst=10." << sliceId << ".2.0/255.255.255.0"
      << " meter:" << meterId
      << " goto:" << OFS_TAB_QUEUE;

  DpctlExecute (lInfo->GetSwDpId (static_cast<int> (LinkInfo::FWD)), cmd.str () + mtc.str ());


  meterId = MeterIdSlcCreate (sliceId, static_cast<int> (LinkInfo::BWD));

  // Build the match string.
  std::ostringstream mtc1;
  mtc1 << " eth_type="   << IPV4_PROT_NUM
       << ",ip_dst=10." << sliceId << ".1.0/255.255.255.0"
       << " meter:" << meterId
       << " goto:" << OFS_TAB_QUEUE;
  DpctlExecute (lInfo->GetSwDpId (static_cast<int> (LinkInfo::BWD)), cmd.str () + mtc1.str ());

}

void
Controller::SlicingMeterAdjust (
  Ptr<LinkInfo> lInfo, int sliceId)
{
  NS_LOG_FUNCTION (this << lInfo << sliceId);

  // Update inter-slicing meter, depending on the InterSliceMode attribute.
  NS_ASSERT_MSG (sliceId < SLICE_ALL, "Invalid slice for this operation.");
  switch (m_sliceMode)
    {
    case SliceMode::NONE:
      // Nothing to do when inter-slicing is disabled.
      return;

    case SliceMode::SHAR:
      // Identify the Non-GBR meter entry to adjust: individual or shared.
      sliceId = SLICE_ALL;

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

      int64_t meterBitRate = 0;
      if (sliceId == SLICE_ALL)
        {
          // Iterate over slices with enabled bandwidth sharing
          // to sum the quota bit rate.
          for (size_t slc = 0; slc < m_numberSlices; slc++)
            {
              meterBitRate += lInfo->GetUnrBitRate (dir, slc);
            }
          // When enable, sum the spare bit rate too.
          if (m_spareUse == OpMode::ON)
            {
              meterBitRate += lInfo->GetUnrBitRate (dir, SLICE_UNKN);
            }
        }
      else
        {
          meterBitRate = lInfo->GetUnrBitRate (dir, sliceId);
        }

      int64_t currBitRate = lInfo->GetMetBitRate (dir, sliceId);
      uint64_t diffBitRate = std::abs (currBitRate - meterBitRate);
      NS_LOG_DEBUG ("Current slice " << sliceId <<
                    " direction "    << LinkInfo::LinkDirStr (dir) <<
                    " diff rate "    << diffBitRate);

      if (diffBitRate >= m_meterStep.GetBitRate ())
        {
          uint32_t meterId = MeterIdSlcCreate (sliceId, d);
          int64_t meterKbps = Bps2Kbps (meterBitRate);
          bool success = lInfo->SetMetBitRate (dir, sliceId, meterKbps * 1000);
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
          DpctlExecute (lInfo->GetSwDpId (d), cmd.str ());
        }
    }
}

void
Controller::SlicingMeterInstall (Ptr<LinkInfo> lInfo, int sliceId)
{
  NS_LOG_FUNCTION (this << lInfo << sliceId);

  NS_ASSERT_MSG (m_sliceMode != SliceMode::NONE,
                 "Invalid inter-slice operation mode.");

  // Install slicing meters in both link directions.
  for (int d = 0; d < N_LINK_DIRS; d++)
    {
      LinkInfo::LinkDir dir = static_cast<LinkInfo::LinkDir> (d);

      int64_t meterBitRate = 0;
      if (sliceId == SLICE_ALL)
        {
          NS_ASSERT_MSG (m_sliceMode == SliceMode::SHAR,
                         "Invalid inter-slice operation mode.");

          // Iterate over slices with enabled bandwidth sharing
          // to sum the quota bit rate.
          for (size_t slc = 0; slc < m_numberSlices; slc++)
            {
              meterBitRate += lInfo->GetQuoBitRate (dir, slc);
            }
          // When enable, sum the spare bit rate too.
          if (m_spareUse == OpMode::ON)
            {
              meterBitRate += lInfo->GetQuoBitRate (dir, SLICE_UNKN);
            }
        }
      else
        {
          meterBitRate = lInfo->GetQuoBitRate (dir, sliceId);
        }

      uint32_t meterId = MeterIdSlcCreate (sliceId, d);
      int64_t meterKbps = Bps2Kbps (meterBitRate);
      bool success = lInfo->SetMetBitRate (dir, sliceId, meterKbps * 1000);
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
      DpctlExecute (lInfo->GetSwDpId (d), cmd.str ());
    }
}

} // namespace ns3
#endif // NS3_OFSWITCH13
