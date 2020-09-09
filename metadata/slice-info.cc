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
                   MakeUintegerChecker<uint16_t> (0, 100))
    .AddAttribute ("HostsA", "Number of hosts attached to switch A.",
                   TypeId::ATTR_GET | TypeId::ATTR_CONSTRUCT,
                   UintegerValue (0),
                   MakeUintegerAccessor (&SliceInfo::m_hostsA),
                   MakeUintegerChecker<uint16_t> (0, 255))
    .AddAttribute ("HostsB", "Number of hosts attached to switch B.",
                   TypeId::ATTR_GET | TypeId::ATTR_CONSTRUCT,
                   UintegerValue (0),
                   MakeUintegerAccessor (&SliceInfo::m_hostsB),
                   MakeUintegerChecker<uint16_t> (0, 255))
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
SliceInfo::SetHostsA (uint16_t value)
{
  NS_LOG_FUNCTION (this << value);

  m_hostsA = value;
}

void
SliceInfo::SetHostsB (uint16_t value)
{
  NS_LOG_FUNCTION (this << value);

  m_hostsB = value;
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

uint16_t
SliceInfo::GetHostsA (void) const
{
  return m_hostsA;
}

uint16_t
SliceInfo::GetHostsB (void) const
{
  return m_hostsB;
}

void
SliceInfo::DoDispose ()
{
  Object::DoDispose ();
}

void
SliceInfo::NotifyConstructionCompleted (void)
{
  NS_LOG_FUNCTION (this);

  RegisterSliceInfo (Ptr<SliceInfo> (this));
  Object::NotifyConstructionCompleted ();
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
