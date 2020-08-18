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

#include "slice-tag.h"

// Metadata bitmap.
#define META_NODE 0
#define META_TYPE 1
#define META_AGGR 2

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED (SliceTag);

SliceTag::SliceTag ()
  : m_slice (0),
  m_type (0),
  m_time (Simulator::Now ().GetTimeStep ()),
  m_dpId (0)
{
}

/*SliceTag::SliceTag (uint32_t teid, EpcInputNode node,
                        QosType type, bool aggr)
  : m_meta (0),
  m_teid (teid),
  m_time (Simulator::Now ().GetTimeStep ())
{
  SetMetadata (node, type, aggr);
}*/

TypeId
SliceTag::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::SliceTag")
    .SetParent<Tag> ()
    .AddConstructor<SliceTag> ()
  ;
  return tid;
}

TypeId
SliceTag::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}

uint32_t
SliceTag::GetSerializedSize (void) const
{
  return 18;
}

void
SliceTag::Serialize (TagBuffer i) const
{
  i.WriteU8  (m_slice);
  i.WriteU8  (m_type);
  i.WriteU64  (m_time);
  i.WriteU64  (m_dpId);

  /*i.WriteU8  (m_meta);
  i.WriteU32 (m_teid);
  i.WriteU64 (m_time);*/
}

void
SliceTag::Deserialize (TagBuffer i)
{

  m_slice = i.ReadU8 ();
  m_type = i.ReadU8 ();
  m_time = i.ReadU64 ();
  m_dpId = i.ReadU64 ();

  /*m_meta = i.ReadU8 ();
  m_teid = i.ReadU32 ();
  m_time = i.ReadU64 ();*/
}

void
SliceTag::Print (std::ostream &os) const
{
  os << " slice=" << m_slice
    //<< " node=" << EpcInputNodeStr (GetInputNode ())
     << " type=" << META_TYPE
    //<< " aggr=" << (IsAggregated () ? "true" : "false")
     << " time=" << m_time
     << " dpId=" << m_dpId;
}

/*Direction
SliceTag::GetDirection () const
{
  return GetInputNode () == SliceTag::PGW ?
         Direction::DLINK : Direction::ULINK;
}*/

/*SliceTag::EpcInputNode
SliceTag::GetInputNode () const
{
  uint8_t node = (m_meta & (1U << META_NODE));
  return static_cast<SliceTag::EpcInputNode> (node >> META_NODE);
}

QosType
SliceTag::GetQosType () const
{
  if (IsAggregated ())
    {
      return QosType::NON;
    }

  uint8_t type = (m_meta & (1U << META_TYPE));
  return static_cast<QosType> (type >> META_TYPE);
}*/

/*SliceId
SliceTag::GetSliceId () const
{
  return TeidGetSliceId (m_teid);
}*/

uint8_t
SliceTag::GetType () const
{
  return m_type;
}

uint8_t
SliceTag::GetSliceId () const
{
  return m_slice;
}

Time
SliceTag::GetTimestamp () const
{
  return Time (m_time);
}

uint64_t
SliceTag::GetDpId () const
{
  return m_dpId;
}


/*bool
SliceTag::IsAggregated () const
{
  return (m_meta & (1U << META_AGGR));
}

std::string
SliceTag::EpcInputNodeStr (EpcInputNode node)
{
  switch (node)
    {
    case EpcInputNode::ENB:
      return "enb";
    case EpcInputNode::PGW:
      return "pgw";
    default:
      return std::string ();
    }
}

void
SliceTag::SetMetadata (EpcInputNode node, QosType type, bool aggr)
{
  NS_ASSERT_MSG (node <= 0x01, "Input node cannot exceed 1 bit.");
  NS_ASSERT_MSG (type <= 0x01, "QoS type cannot exceed 1 bit.");

  m_meta = 0x00;
  m_meta |= (static_cast<uint8_t> (node) << META_NODE);
  m_meta |= (static_cast<uint8_t> (type) << META_TYPE);
  m_meta |= (static_cast<uint8_t> (aggr) << META_AGGR);
}*/

} // namespace ns3
