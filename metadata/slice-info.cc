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

#include "slice-info.h"
#include "../common.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("SliceInfo");
NS_OBJECT_ENSURE_REGISTERED (SliceInfo);

// Initializing Slice static members.
SliceInfo::SliceInfoMap_t SliceInfo::m_sliceInfoById;
SliceInfoList_t SliceInfo::m_sliceInfoList;

SliceInfo::SliceInfo ()
{
  NS_LOG_FUNCTION (this);
}

SliceInfo::~SliceInfo ()
{
  NS_LOG_FUNCTION (this);
}

TypeId
SliceInfo::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::SliceInfo")
    .SetParent<Object> ()
    .AddConstructor<SliceInfo> ()
    .AddAttribute ("SliceId", "Slice identifier.",
                   TypeId::ATTR_GET | TypeId::ATTR_CONSTRUCT,
                   UintegerValue (0),
                   MakeUintegerAccessor (&SliceInfo::m_sliceId),
                   MakeUintegerChecker<uint16_t> (1, N_MAX_SLICES))
    .AddAttribute ("Priority", "Slice priority.",
                   UintegerValue (0),
                   MakeUintegerAccessor (&SliceInfo::m_prio),
                   MakeUintegerChecker<uint16_t> ())
    .AddAttribute ("Quota", "Slice quota.",
                   UintegerValue (0),
                   MakeUintegerAccessor (&SliceInfo::m_quota),
                   MakeUintegerChecker<uint16_t> (1, 100))
    .AddAttribute ("Sharing", "Enable bandwidth sharing.",
                   TypeId::ATTR_GET | TypeId::ATTR_CONSTRUCT,
                   EnumValue (OpMode::ON),
                   MakeEnumAccessor (&SliceInfo::m_sharing),
                   MakeEnumChecker (OpMode::OFF, OpModeStr (OpMode::OFF),
                                    OpMode::ON,  OpModeStr (OpMode::ON)))
    .AddAttribute ("NumHostsA", "Number of hosts attached to switch A.",
                   TypeId::ATTR_GET | TypeId::ATTR_CONSTRUCT,
                   UintegerValue (0),
                   MakeUintegerAccessor (&SliceInfo::m_numHostsA),
                   MakeUintegerChecker<uint16_t> (0, 255))
    .AddAttribute ("NumHostsB", "Number of hosts attached to switch B.",
                   TypeId::ATTR_GET | TypeId::ATTR_CONSTRUCT,
                   UintegerValue (0),
                   MakeUintegerAccessor (&SliceInfo::m_numHostsB),
                   MakeUintegerChecker<uint16_t> (0, 255))
    .AddAttribute ("AppHelper", "Application helper for traffic configuration.",
                   TypeId::ATTR_GET | TypeId::ATTR_CONSTRUCT,
                   StringValue ("ns3::UdpPeerHelper[]"),
                   MakePointerAccessor (&SliceInfo::m_appHelper),
                   MakePointerChecker<UdpPeerHelper> ())
  ;
  return tid;
}

void
SliceInfo::SetSliceId (uint16_t value)
{
  NS_LOG_FUNCTION (this << value);

  m_sliceId = value;
}

void
SliceInfo::SetPriority (uint16_t value)
{
  NS_LOG_FUNCTION (this << value);

  m_prio = value;
}

void
SliceInfo::SetQuota (uint16_t value)
{
  NS_LOG_FUNCTION (this << value);

  m_quota = value;
}

void
SliceInfo::SetSharing (bool value)
{
  NS_LOG_FUNCTION (this << value);

  m_sharing = value;
}

void
SliceInfo::SetNumHostsA (uint16_t value)
{
  NS_LOG_FUNCTION (this << value);

  m_numHostsA = value;
}

void
SliceInfo::SetNumHostsB (uint16_t value)
{
  NS_LOG_FUNCTION (this << value);

  m_numHostsB = value;
}

uint16_t
SliceInfo::GetSliceId (void) const
{
  return m_sliceId;
}

uint16_t
SliceInfo::GetPriority (void) const
{
  return m_prio;
}

uint16_t
SliceInfo::GetQuota (void) const
{
  return m_quota;
}

bool
SliceInfo::GetSharing (void) const
{
  return m_sharing;
}

uint16_t
SliceInfo::GetNumHostsA (void) const
{
  return m_numHostsA;
}

uint16_t
SliceInfo::GetNumHostsB (void) const
{
  return m_numHostsB;
}

uint16_t
SliceInfo::GetNumHostsC (void) const
{
  return m_numHostsA + m_numHostsB;
}

Ptr<UdpPeerHelper>
SliceInfo::GetAppHelper (void)
{
  return m_appHelper;
}

bool
SliceInfo::PriorityComparator (Ptr<SliceInfo> slice1, Ptr<SliceInfo> slice2)
{
  return slice1->GetPriority () < slice2->GetPriority ();
}

void
SliceInfo::DoDispose ()
{
  m_appHelper = 0;
  Object::DoDispose ();
}

void
SliceInfo::NotifyConstructionCompleted (void)
{
  NS_LOG_FUNCTION (this);

  RegisterSliceInfo (Ptr<SliceInfo> (this));
  Object::NotifyConstructionCompleted ();
}

size_t
SliceInfo::GetNSlices (void)
{
  return m_sliceInfoList.size ();
}

const SliceInfoList_t&
SliceInfo::GetList (void)
{
  return SliceInfo::m_sliceInfoList;
}

Ptr<SliceInfo>
SliceInfo::GetPointer (uint16_t sliceId)
{
  Ptr<SliceInfo> sInfo = 0;
  auto ret = SliceInfo::m_sliceInfoById.find (sliceId);
  if (ret != SliceInfo::m_sliceInfoById.end ())
    {
      sInfo = ret->second;
    }
  return sInfo;
}

void
SliceInfo::RegisterSliceInfo (Ptr<SliceInfo> sInfo)
{
  uint16_t key = sInfo->GetSliceId ();
  std::pair<uint16_t, Ptr<SliceInfo> > entry (key, sInfo);
  auto ret = SliceInfo::m_sliceInfoById.insert (entry);
  NS_ABORT_MSG_IF (ret.second == false, "Existing slice info for this key");

  SliceInfo::m_sliceInfoList.push_back (sInfo);
}

} // namespace ns3
