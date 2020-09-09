#ifndef SVELTE_COMMON_H
#define SVELTE_COMMON_H

#include <ns3/internet-module.h>
#include <ns3/core-module.h>

namespace ns3 {

/** Constants that represent the indexes in the vectors. */
typedef enum
{
  HOSTS_SWA = 0,    //!< The hosts of switch SWA inside de vectors.
  HOSTS_SWB = 1,    //!< The hosts of switch SWB inside de vectors.
  SERVERS   = 2,    //!< The servers inside de vectors.
  ALL_HOSTS = 3     //!< All the hosts (SWA+SWB) inside de vectors.
} Indexes;

// Valid number of slices and IDs
#define N_MAX_SLICES  14    //!< Slice IDs ranging from 1 to 14
#define SLICE_UNKN    0
#define SLICE_ALL     (N_MAX_SLICES + 1)

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

// Pipeline tables at OpenFlow Switches.
#define ROUTING_TAB 0
#define METER_TAB 1
#define QOS_TAB 2

// Protocol numbers.
#define IPV4_PROT_NUM (static_cast<uint16_t> (Ipv4L3Protocol::PROT_NUMBER))

// OpenFlow flow-mod flags.
#define FLAGS_REMOVED_OVERLAP_RESET ((OFPFF_SEND_FLOW_REM | OFPFF_CHECK_OVERLAP | OFPFF_RESET_COUNTS))
#define FLAGS_OVERLAP_RESET ((OFPFF_CHECK_OVERLAP | OFPFF_RESET_COUNTS))

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
 * Convert the BPS to KBPS without precision loss.
 * \param bitrate The bit rate in BPS.
 * \return The bitrate in KBPS.
 */
double Bps2Kbps (int64_t bitrate);

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
 * Compute the meter ID value globally used in the SVELTE architecture for
 * infrastructure slicing meters.
 * \param sliceId The SVELTE logical slice ID.
 * \param linkdir The link direction.
 * \return The meter ID value.
 *
 * \internal
 * We are using the following meter ID allocation strategy:
 * \verbatim
 * Meter ID has 32 bits length: 0x 0 0 00000 0
 *                                |-|-|-----|-|
 *                                 A B C     D
 *
 *  4 (A) bits are used to identify a slicing meter, here fixed at 0xC.
 *  4 (B) bits are used to identify the logical slice (slice ID).
 * 20 (C) bits are unused, here fixed at 0x00000.
 *  4 (D) bits are used to identify the link direction.
 * \endverbatim
 */
uint32_t MeterIdSlcCreate (uint32_t sliceId, uint32_t linkdir);

/**
 * Get the inter-slicing operation mode name.
 * \param mode The inter-slicing operation mode.
 * \return The string with the inter-slicing operation mode name.
 */
std::string SliceModeStr (SliceMode mode);

/**
 * Get the operation mode name.
 * \param mode The operation mode.
 * \return The string with the operation mode name.
 */
std::string OpModeStr (OpMode mode);

/**
 * Get the LTE QoS traffic type name.
 * \param type The LTE QoS traffic type.
 * \return The string with the LTE QoS traffic type name.
 */
std::string QosTypeStr (QosType type);

/**
 * Convert the uint32_t parameter value to a hexadecimal string representation.
 * \param value The uint32_t value.
 * \return The hexadecimal string representation.
 */
std::string GetUint32Hex (uint32_t value);

/**
 * Get the mapped IP ToS value for a specific DSCP.
 * \param dscp The IP DSCP value.
 * \return The IP ToS mapped for this DSCP.
 *
 * \internal
 * We are mapping the DSCP value (RFC 2474) to the IP Type of Service (ToS)
 * (RFC 1349) field because the pfifo_fast queue discipline from the traffic
 * control module still uses the old IP ToS definition. Thus, we are
 * 'translating' the DSCP values so we can keep the queuing consistency
 * both on traffic control module and OpenFlow port queues.
 * \verbatim
 * DSCP_EF   --> ToS 0x10 --> prio 6 --> pfifo band 0
 * DSCP_AF41 --> ToS 0x18 --> prio 4 --> pfifo band 1
 * DSCP_AF31 --> ToS 0x00 --> prio 0 --> pfifo band 1
 * DSCP_AF32 --> ToS 0x00 --> prio 0 --> pfifo band 1
 * DSCP_AF21 --> ToS 0x00 --> prio 0 --> pfifo band 1
 * DSCP_AF11 --> ToS 0x00 --> prio 0 --> pfifo band 1
 * DSCP_BE   --> ToS 0x08 --> prio 2 --> pfifo band 2
 * \endverbatim
 * \see See the ns3::Socket::IpTos2Priority for details.
 */
uint8_t Dscp2Tos (Ipv4Header::DscpType dscp);

} // namespace ns3
#endif // COMMON_H
