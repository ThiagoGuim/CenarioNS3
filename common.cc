#include "common.h"

using namespace std;

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("Common");

double
Bps2Kbps (int64_t bitrate)
{
  return static_cast<double> (bitrate) / 1000;
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

uint32_t
MeterIdSlcCreate (uint32_t sliceId, uint32_t linkdir)
{
  NS_ASSERT_MSG (sliceId <= 0xF, "Slice ID cannot exceed 4 bits.");
  NS_ASSERT_MSG (linkdir <= 0xF, "Link direction cannot exceed 4 bits.");

  uint32_t meterId = 0xC;
  meterId <<= 4;
  meterId |= static_cast<uint32_t> (sliceId);
  meterId <<= 24;
  meterId |= linkdir;
  return meterId;
}

/*std::string
SliceIdStr (int slice)
{
  switch (slice)
    {
    case SliceId::HTC:
      return "htc";
    case SliceId::MTC:
      return "mtc";
    case SliceId::TMP:
      return "tmp";
    case SliceId::ALL:
      return "all";
    case SliceId::UNKN:
      return "unknown";
    default:
      NS_LOG_ERROR ("Invalid SVELTE logical slice ID.");
      return std::string ();
    }
}*/

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

std::string
GetUint32Hex (uint32_t value)
{
  char valueStr [11];
  sprintf (valueStr, "0x%08x", value);
  return std::string (valueStr);
}

uint8_t
Dscp2Tos (Ipv4Header::DscpType dscp)
{
  switch (dscp)
    {
    case Ipv4Header::DSCP_EF:
      return 0x10;
    case Ipv4Header::DSCP_AF41:
      return 0x18;
    case Ipv4Header::DSCP_AF32:
    case Ipv4Header::DSCP_AF31:
    case Ipv4Header::DSCP_AF21:
    case Ipv4Header::DSCP_AF11:
      return 0x00;
    case Ipv4Header::DscpDefault:
      return 0x08;
    default:
      NS_ABORT_MSG ("No ToS mapped value for DSCP " << dscp);
      return 0x00;
    }
}

}// namespace ns3
