/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2015 University of Campinas (Unicamp)
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
 * Author: Luciano Chaves <luciano@lrc.ic.unicamp.br>
 *         Thiago Guimarães <thiago.guimaraes@ice.ufjf.br>
 */

#include "slice-tag.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("SliceTag");
NS_OBJECT_ENSURE_REGISTERED (SliceTag);

SliceTag::SliceTag ()
  : m_time (Simulator::Now ().GetTimeStep ()),
  m_slice (0),
  m_type (QosType::NON),
  m_ttype (TQosType::BE)
{
}

SliceTag::SliceTag (uint8_t slice)
  : m_time (Simulator::Now ().GetTimeStep ()),
  m_slice (slice),
  m_type (QosType::NON),
  m_ttype (TQosType::BE)
{
}

SliceTag::SliceTag (uint8_t slice, QosType type, TQosType ttype)
  : m_time (Simulator::Now ().GetTimeStep ()),
  m_slice (slice),
  m_type (type),
  m_ttype (ttype)
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
  return 11;
}

void
SliceTag::Serialize (TagBuffer i) const
{
  i.WriteU64 (m_time);
  i.WriteU8  (m_slice);
  i.WriteU8  (m_type);
  i.WriteU8  (m_ttype);
}

void
SliceTag::Deserialize (TagBuffer i)
{
  m_time  = i.ReadU64 ();
  m_slice = i.ReadU8 ();
  m_type  = i.ReadU8 ();
  m_ttype  = i.ReadU8 ();
}

void
SliceTag::Print (std::ostream &os) const
{
  os << " time="  << m_time
     << " slice=" << m_slice
     << " type="  << QosTypeStr (GetQosType ());
}

Time
SliceTag::GetTimestamp () const
{
  return Time (m_time);
}

uint8_t
SliceTag::GetSliceId () const
{
  return m_slice;
}

QosType
SliceTag::GetQosType () const
{
  return static_cast<QosType> (m_type);
}

TQosType
SliceTag::GetTQosType () const
{
  return static_cast<TQosType> (m_ttype);
}

} // namespace ns3
