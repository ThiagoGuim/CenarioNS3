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
 */

#include "common.h"

using namespace std;

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("Common");

std::string
QosTypeStr (QosType type)
{
  switch (type)
    {
    case QosType::NON:
      return "NonGBR";
    case QosType::GBR:
      return "GBR";
    case QosType::BOTH:
      return "Both";
    default:
      NS_LOG_ERROR ("Invalid QoS traffic type.");
      return std::string ();
    }
}

std::string
SliceModeStr (SliceMode mode)
{
  switch (mode)
    {
    case SliceMode::NONE:
      return "none";
    case SliceMode::SHAR:
      return "shared";
    case SliceMode::STAT:
      return "static";
    case SliceMode::DYNA:
      return "dynamic";
    default:
      NS_LOG_ERROR ("Invalid inter-slice operation mode.");
      return std::string ();
    }
}

std::string
OpModeStr (OpMode mode)
{
  switch (mode)
    {
    case OpMode::OFF:
      return "off";
    case OpMode::ON:
      return "on";
    case OpMode::AUTO:
      return "auto";
    default:
      NS_LOG_ERROR ("Invalid operation mode.");
      return std::string ();
    }
}

double
Bps2Kbps (int64_t bitrate)
{
  return static_cast<double> (bitrate) / 1000;
}

double
Bps2Kbps (DataRate datarate)
{
  return Bps2Kbps (datarate.GetBitRate ());
}

const DscpQueueMap_t&
Dscp2QueueMap ()
{
  static DscpQueueMap_t queueByDscp;

  // Populating the map at the first time.
  if (queueByDscp.empty ())
    {
      queueByDscp.insert (std::make_pair (Ipv4Header::DSCP_EF,     0));
      queueByDscp.insert (std::make_pair (Ipv4Header::DSCP_AF41,   1));
      queueByDscp.insert (std::make_pair (Ipv4Header::DSCP_AF31,   1));
      queueByDscp.insert (std::make_pair (Ipv4Header::DSCP_AF32,   1));
      queueByDscp.insert (std::make_pair (Ipv4Header::DSCP_AF21,   1));
      queueByDscp.insert (std::make_pair (Ipv4Header::DSCP_AF11,   1));
      queueByDscp.insert (std::make_pair (Ipv4Header::DscpDefault, 2));
    }

  return queueByDscp;
}

std::string
DscpTypeStr (Ipv4Header::DscpType dscp)
{
  switch (dscp)
    {
    case Ipv4Header::DscpDefault:
      return "BE";
    case Ipv4Header::DSCP_CS1:
      return "CS1";
    case Ipv4Header::DSCP_AF11:
      return "AF11";
    case Ipv4Header::DSCP_AF12:
      return "AF12";
    case Ipv4Header::DSCP_AF13:
      return "AF13";
    case Ipv4Header::DSCP_CS2:
      return "CS2";
    case Ipv4Header::DSCP_AF21:
      return "AF21";
    case Ipv4Header::DSCP_AF22:
      return "AF22";
    case Ipv4Header::DSCP_AF23:
      return "AF23";
    case Ipv4Header::DSCP_CS3:
      return "CS3";
    case Ipv4Header::DSCP_AF31:
      return "AF31";
    case Ipv4Header::DSCP_AF32:
      return "AF32";
    case Ipv4Header::DSCP_AF33:
      return "AF33";
    case Ipv4Header::DSCP_CS4:
      return "CS4";
    case Ipv4Header::DSCP_AF41:
      return "AF41";
    case Ipv4Header::DSCP_AF42:
      return "AF42";
    case Ipv4Header::DSCP_AF43:
      return "AF43";
    case Ipv4Header::DSCP_CS5:
      return "CS5";
    case Ipv4Header::DSCP_EF:
      return "EF";
    case Ipv4Header::DSCP_CS6:
      return "CS6";
    case Ipv4Header::DSCP_CS7:
      return "CS7";
    default:
      NS_LOG_ERROR ("Invalid DSCP type.");
      return std::string ();
    }
}

uint32_t
MeterIdSlcCreate (uint32_t sliceId, uint32_t linkdir)
{
  NS_ASSERT_MSG (sliceId <= 0xF, "Slice ID cannot exceed 4 bits.");
  NS_ASSERT_MSG (linkdir <= 0xF, "Link direction cannot exceed 4 bits.");

  uint32_t meterId = 0x800000;
  meterId <<= 4;
  meterId |= static_cast<uint32_t> (sliceId);
  meterId <<= 4;
  meterId |= linkdir;
  return meterId;
}

std::string
GetUint32Hex (uint32_t value)
{
  char valueStr [11];
  sprintf (valueStr, "0x%08x", value);
  return std::string (valueStr);
}

std::string
GetUint64Hex (uint64_t value)
{
  char valueStr [19];
  sprintf (valueStr, "0x%016lx", value);
  return std::string (valueStr);
}

}// namespace ns3
