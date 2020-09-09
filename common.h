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

#ifndef COMMON_H
#define COMMON_H

#include <ns3/internet-module.h>
#include <ns3/core-module.h>
#include <ns3/ofswitch13-module.h>

namespace ns3 {

// ----------------------------------------------------------------------------
/** Constants that represent the indexes in the vectors. */
typedef enum
{
  HOSTS_SWA = 0,    //!< The hosts of switch SWA inside de vectors.
  HOSTS_SWB = 1,    //!< The hosts of switch SWB inside de vectors.
  SERVERS   = 2,    //!< The servers inside de vectors.
  ALL_HOSTS = 3     //!< All the hosts (SWA+SWB) inside de vectors.
} Indexes;
// ----------------------------------------------------------------------------


typedef std::vector<Ptr<OFSwitch13Port>> PortsList_t;



// ----------------------------------------------------------------------------
// Valid number of slices and IDs
#define N_MAX_SLICES  14    //!< Slice IDs ranging from 1 to 14. Don't change!
#define SLICE_UNKN    0
#define SLICE_ALL     (N_MAX_SLICES + 1)
// ----------------------------------------------------------------------------


// ----------------------------------------------------------------------------
// Pipeline tables at OpenFlow switches.
#define OFS_TAB_ROUTE 0
#define OFS_TAB_METER 1
#define OFS_TAB_QUEUE 2
#define OFS_TAB_TOTAL 3

// Protocol numbers.
#define IPV4_PROT_NUM (static_cast<uint16_t> (Ipv4L3Protocol::PROT_NUMBER))
#define UDP_PROT_NUM  (static_cast<uint16_t> (UdpL4Protocol::PROT_NUMBER))
#define TCP_PROT_NUM  (static_cast<uint16_t> (TcpL4Protocol::PROT_NUMBER))

// OpenFlow flow-mod flags.
#define FLAGS_REMOVED_OVERLAP_RESET \
  ((OFPFF_SEND_FLOW_REM | OFPFF_CHECK_OVERLAP | OFPFF_RESET_COUNTS))
#define FLAGS_OVERLAP_RESET \
  ((OFPFF_CHECK_OVERLAP | OFPFF_RESET_COUNTS))
// ----------------------------------------------------------------------------


// ----------------------------------------------------------------------------
/** Enumeration of available QoS traffic types. */
typedef enum
{
  // Don't change the order. Enum values are used as array indexes.
  NON  = 0,    //!< Non-GBR traffic.
  GBR  = 1,    //!< GBR traffic.
  BOTH = 2     //!< Both GBR and Non-GBR traffic.
} QosType;

// Total number of valid QosType items.
#define N_QOS_TYPES (static_cast<int> (QosType::BOTH))
#define N_QOS_TYPES_BOTH (static_cast<int> (QosType::BOTH) + 1)

/**
 * Get the LTE QoS traffic type name.
 * \param type The LTE QoS traffic type.
 * \return The string with the LTE QoS traffic type name.
 */
std::string QosTypeStr (QosType type);
// ----------------------------------------------------------------------------


// ----------------------------------------------------------------------------
/** Enumeration of available inter-slicing operation modes. */
typedef enum
{
  NONE = 0,     //!< No inter-slicing.
  SHAR = 1,     //!< Partial Non-GBR shared inter-slicing.
  STAT = 2,     //!< Full static inter-slicing.
  DYNA = 3      //!< Full dinaymic inter-slicing.
} SliceMode;

// Total number of valid SliceMode items.
#define N_SLICE_MODES (static_cast<int> (SliceMode::DYNA) + 1)

/**
 * Get the inter-slicing operation mode name.
 * \param mode The inter-slicing operation mode.
 * \return The string with the inter-slicing operation mode name.
 */
std::string SliceModeStr (SliceMode mode);
// ----------------------------------------------------------------------------


// ----------------------------------------------------------------------------
/** Enumeration of available operation modes. */
typedef enum
{
  OFF  = 0,     //!< Always off.
  ON   = 1,     //!< Always on.
  AUTO = 2      //!< Automatic.
} OpMode;

// Total number of valid OpMode items.
#define N_OP_MODES (static_cast<int> (OpMode::AUTO) + 1)

/**
 * Get the operation mode name.
 * \param mode The operation mode.
 * \return The string with the operation mode name.
 */
std::string OpModeStr (OpMode mode);
// ----------------------------------------------------------------------------


// ----------------------------------------------------------------------------
/**
 * Convert the BPS to KBPS without precision loss.
 * \param bitrate The bit rate in BPS.
 * \return The bitrate in KBPS.
 */
double Bps2Kbps (int64_t bitrate);

/**
 * Convert DataRate BPS to KBPS without precision loss.
 * \param bitrate The DataRate.
 * \return The bitrate in KBPS.
 */
double Bps2Kbps (DataRate datarate);
// ----------------------------------------------------------------------------


// ----------------------------------------------------------------------------
/** Map saving IP DSCP value / OpenFlow queue id. */
typedef std::map<Ipv4Header::DscpType, uint32_t> DscpQueueMap_t;

/**
 * Get the mapped OpenFlow output queue ID for all DSCP used values.
 * \return The OpenFlow queue ID mapped values.
 *
 * \internal
 * Mapping the IP DSCP to the OpenFlow output queue ID.
 * \verbatim
 * DSCP_EF   --> OpenFlow queue 0 (priority)
 * DSCP_AF41 --> OpenFlow queue 1 (WRR)
 * DSCP_AF31 --> OpenFlow queue 1 (WRR)
 * DSCP_AF32 --> OpenFlow queue 1 (WRR)
 * DSCP_AF21 --> OpenFlow queue 1 (WRR)
 * DSCP_AF11 --> OpenFlow queue 1 (WRR)
 * DSCP_BE   --> OpenFlow queue 2 (WRR)
 * \endverbatim
 */
const DscpQueueMap_t& Dscp2QueueMap (void);

/**
 * Get the QoS type for a given DSCP value.
 * \param dscp The DSCP type value.
 * \return The QoS type.
 *
 * \internal
 * Mapping the IP DSCP to the QosType.
 * \verbatim
 * DSCP_EF   --> GBR
 * DSCP_AF41 --> GBR
 * DSCP_AF31 --> Non-GBR
 * DSCP_AF32 --> Non-GBR
 * DSCP_AF21 --> Non-GBR
 * DSCP_AF11 --> Non-GBR
 * DSCP_BE   --> Non-GBR
 * \endverbatim
 */
QosType Dscp2QosType (Ipv4Header::DscpType dscp);

/**
 * Get the DSCP type name.
 * \param dscp The DSCP type value.
 * \return The string with the DSCP type name.
 */
std::string DscpTypeStr (Ipv4Header::DscpType dscp);
// ----------------------------------------------------------------------------


// ----------------------------------------------------------------------------
/**
 * Compute the meter ID value globally used for slicing meters.
 * \param sliceId The slice ID.
 * \param linkdir The link direction.
 * \return The meter ID value.
 *
 * \internal
 * We are using the following meter ID allocation strategy:
 * \verbatim
 * Meter ID has 32 bits length: 0x 000000 0 0
 *                                |------|-|-|
 *                                   A    B C
 *
 *  24 (A) bits are used to identify a slicing meter, here fixed at 0x800000.
 *   4 (B) bits are used to identify the slice ID.
 *   4 (C) bits are used to identify the link direction.
 * \endverbatim
 */
uint32_t MeterIdSlcCreate (uint32_t sliceId, uint32_t linkdir);
// ----------------------------------------------------------------------------


// ----------------------------------------------------------------------------
/**
 * Convert the uint32_t parameter value to a hexadecimal string representation.
 * \param value The uint32_t value.
 * \return The hexadecimal string representation.
 */
std::string GetUint32Hex (uint32_t value);

/**
 * Convert the uint64_t parameter value to a hexadecimal string representation.
 * \param value The uint64_t value.
 * \return The hexadecimal string representation.
 */
std::string GetUint64Hex (uint64_t value);
// ----------------------------------------------------------------------------

} // namespace ns3
#endif // COMMON_H
