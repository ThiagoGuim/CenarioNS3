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
  m_time (Simulator::Now ().GetTimeStep ()),
  m_type (0)
{
}

SliceTag::SliceTag (uint8_t sliceId)
  : m_slice (sliceId),
  m_time (Simulator::Now ().GetTimeStep ()),
  m_type (0)
{
}

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
  return 10;
}

void
SliceTag::Serialize (TagBuffer i) const
{
  i.WriteU8  (m_slice);
  i.WriteU64  (m_time); 
  i.WriteU8  (m_type);
}

void
SliceTag::Deserialize (TagBuffer i)
{

  m_slice = i.ReadU8 ();
  m_time = i.ReadU64 ();
  m_type = i.ReadU8 ();

}

void
SliceTag::Print (std::ostream &os) const
{
  os << " slice=" << m_slice
     << " time=" << m_time;
}

uint8_t
SliceTag::GetSliceId () const
{
  return m_slice;
}

uint8_t
SliceTag::GetType () const
{
  return m_type;
}

Time
SliceTag::GetTimestamp () const
{
  return Time (m_time);
}

} // namespace ns3
