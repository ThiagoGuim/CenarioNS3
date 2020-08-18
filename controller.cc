/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
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

#ifdef NS3_OFSWITCH13

#include "controller.h"

NS_LOG_COMPONENT_DEFINE ("Controller");

namespace ns3 {

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
Controller::DoDispose ()
{
  NS_LOG_FUNCTION (this);

  m_learnedInfo.clear ();
  OFSwitch13Controller::DoDispose ();
}

SliceMode
Controller::GetInterSliceMode (void) const
{
  NS_LOG_FUNCTION (this);

  return m_sliceMode;
}

OpMode
Controller::GetSpareUseMode (void) const
{
  NS_LOG_FUNCTION (this);

  return m_spareUse;
}

ofl_err
Controller::HandleFlowRemoved (
  struct ofl_msg_flow_removed *msg, Ptr<const RemoteSwitch> swtch,
  uint32_t xid)
{
  NS_LOG_FUNCTION (this << swtch << xid);

  // Get the switch datapath ID
  uint64_t swDpId = swtch->GetDpId ();

  NS_LOG_DEBUG ( "Flow entry expired. Removing from L2 switch table.");
  auto it = m_learnedInfo.find (swDpId);
  if (it != m_learnedInfo.end ())
    {
      Mac48Address mac48;
      struct ofl_match_tlv *ethSrc =
        oxm_match_lookup (OXM_OF_ETH_DST, (struct ofl_match*)msg->stats->match);
      mac48.CopyFrom (ethSrc->value);

      L2Table_t *l2Table = &it->second;
      auto itSrc = l2Table->find (mac48);
      if (itSrc != l2Table->end ())
        {
          l2Table->erase (itSrc);
        }
    }

  // All handlers must free the message when everything is ok
  ofl_msg_free_flow_removed (msg, true, 0);
  return 0;
}

/********** Private methods **********/
void
Controller::HandshakeSuccessful (
  Ptr<const RemoteSwitch> swtch)
{
  NS_LOG_FUNCTION (this << swtch);

  // Get the switch datapath ID
  uint64_t swDpId = swtch->GetDpId ();

  // After a successfull handshake, let's install the table-miss entry, setting
  // to 128 bytes the maximum amount of data from a packet that should be sent
  // to the controller.
  DpctlExecute (swDpId, "flow-mod cmd=add,table=0,prio=0 apply:output=ctrl:128");

  //Flooding all ARP packets
  DpctlExecute (swDpId, "flow-mod cmd=add,table=0,prio=10 eth_type=0x0806 apply:output=flood");

  //Standard table 1 Rule
  DpctlExecute (swDpId, "flow-mod cmd=add,table=1,prio=0 goto:2");

  //Standard table 2 Rule
  DpctlExecute (swDpId, "flow-mod cmd=add,table=2,prio=0 ");


  //QoS output queues rules.
  for (auto const &it : Dscp2QueueMap ())
    {
      std::ostringstream cmd;
      cmd << "flow-mod cmd=add,prio=32"
          << ",table="        << QOS_TAB
          << ",flags="        << FLAGS_REMOVED_OVERLAP_RESET
          << " eth_type="     << IPV4_PROT_NUM
          << ",ip_dscp="      << static_cast<uint16_t> (it.first)
          << " write:queue="  << static_cast<uint32_t> (it.second);
      DpctlExecute (swDpId, cmd.str ());
    }


  // Configure te switch to buffer packets and send only the first 128 bytes of
  // each packet sent to the controller when not using an output action to the
  // OFPP_CONTROLLER logical port.
  DpctlExecute (swDpId, "set-config miss=128");

  // Create an empty L2SwitchingTable and insert it into m_learnedInfo
  L2Table_t l2Table;
  std::pair<uint64_t, L2Table_t> entry (swDpId, l2Table);
  auto ret = m_learnedInfo.insert (entry);
  if (ret.second == false)
    {
      NS_LOG_ERROR ("Table exists for this datapath.");
    }
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


  //Installing the fowarding rules between switches.

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
Controller::ConfigureMeters (std::vector<int> sliceQuotas)
{


  for (size_t sliceId = 0; sliceId < m_numberSlices; sliceId++)
    {
      // Install slicing meters in both link directions.
      for (int d = 0; d < N_LINK_DIRS; d++)
        {
          LinkInfo::LinkDir dir = static_cast<LinkInfo::LinkDir> (d);

          bool success = true;
          success &= m_lInfoA->UpdateQuota (dir, sliceId, sliceQuotas[sliceId]);
          success &= m_lInfoB->UpdateQuota (dir, sliceId, sliceQuotas[sliceId]);
          NS_ASSERT_MSG (success, "Error when setting slice quotas.");

        }
    }

  // ---------------------------------------------------------------------
  // Meter table
  //
  // Install inter-slicing meters, depending on the InterSliceMode attribute.
  switch (GetInterSliceMode ())
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
      << ",table="      << METER_TAB
      << ",flags="      << FLAGS_REMOVED_OVERLAP_RESET;


  uint32_t meterId = MeterIdSlcCreate (sliceId, static_cast<int> (LinkInfo::FWD));

  // Build the match string.
  std::ostringstream mtc;
  mtc << " eth_type="   << IPV4_PROT_NUM
      << ",ip_dst=10." << sliceId << ".2.0/255.255.255.0"
      << " meter:" << meterId
      << " goto:" << QOS_TAB;

  DpctlExecute (lInfo->GetSwDpId (static_cast<int> (LinkInfo::FWD)), cmd.str () + mtc.str ());


  meterId = MeterIdSlcCreate (sliceId, static_cast<int> (LinkInfo::BWD));

  // Build the match string.
  std::ostringstream mtc1;
  mtc1 << " eth_type="   << IPV4_PROT_NUM
       << ",ip_dst=10." << sliceId << ".1.0/255.255.255.0"
       << " meter:" << meterId
       << " goto:" << QOS_TAB;
  DpctlExecute (lInfo->GetSwDpId (static_cast<int> (LinkInfo::BWD)), cmd.str () + mtc1.str ());

}

void
Controller::SlicingMeterAdjust (
  Ptr<LinkInfo> lInfo, int sliceId)
{
  NS_LOG_FUNCTION (this << lInfo << sliceId);

  // Update inter-slicing meter, depending on the InterSliceMode attribute.
  NS_ASSERT_MSG (sliceId < SLICE_ALL, "Invalid slice for this operation.");
  switch (GetInterSliceMode ())
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
          if (GetSpareUseMode () == OpMode::ON)
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

  NS_ASSERT_MSG (GetInterSliceMode () != SliceMode::NONE,
                 "Invalid inter-slice operation mode.");

  // Install slicing meters in both link directions.
  for (int d = 0; d < N_LINK_DIRS; d++)
    {
      LinkInfo::LinkDir dir = static_cast<LinkInfo::LinkDir> (d);

      int64_t meterBitRate = 0;
      if (sliceId == SLICE_ALL)
        {
          NS_ASSERT_MSG (GetInterSliceMode () == SliceMode::SHAR,
                         "Invalid inter-slice operation mode.");

          // Iterate over slices with enabled bandwidth sharing
          // to sum the quota bit rate.
          for (size_t slc = 0; slc < m_numberSlices; slc++)
            {
              meterBitRate += lInfo->GetQuoBitRate (dir, slc);
            }
          // When enable, sum the spare bit rate too.
          if (GetSpareUseMode () == OpMode::ON)
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
