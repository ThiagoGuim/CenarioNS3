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

#include "slice.h"
#include "../common.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("Slice");
NS_OBJECT_ENSURE_REGISTERED (Slice);

Slice::Slice ()
  : m_sliceId (0),
  m_prio (0),
  m_quota (0),
  m_hostsA (0),
  m_hostsB (0)
{
  NS_LOG_FUNCTION (this);
}

Slice::~Slice ()
{
  NS_LOG_FUNCTION (this);
}

TypeId
Slice::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::Slice")
    .SetParent<Object> ()
    .AddConstructor<Slice> ()
    .AddAttribute ("SliceId", "Slice identifier.",
                   UintegerValue (0),
                   MakeUintegerAccessor (&Slice::m_sliceId),
                   MakeUintegerChecker<uint16_t> (SLICE_UNKN, N_MAX_SLICES))
    .AddAttribute ("Priority", "Slice priority.",
                   UintegerValue (0),
                   MakeUintegerAccessor (&Slice::m_prio),
                   MakeUintegerChecker<uint16_t> ())
    .AddAttribute ("Quota", "Slice quota.",
                   UintegerValue (0),
                   MakeUintegerAccessor (&Slice::m_quota),
                   MakeUintegerChecker<uint16_t> (0, 100))
    .AddAttribute ("HostsA", "Number of hosts attached to switch A.",
                   UintegerValue (0),
                   MakeUintegerAccessor (&Slice::m_hostsA),
                   MakeUintegerChecker<uint16_t> (0, 255))
    .AddAttribute ("HostsB", "Number of hosts attached to switch B.",
                   UintegerValue (0),
                   MakeUintegerAccessor (&Slice::m_hostsB),
                   MakeUintegerChecker<uint16_t> (0, 255))
  ;
  return tid;
}

void
Slice::SetSliceId (uint16_t value)
{
  NS_LOG_FUNCTION (this << value);

  m_sliceId = value;
}

void
Slice::SetPriority (uint16_t value)
{
  NS_LOG_FUNCTION (this << value);

  m_prio = value;
}

void
Slice::SetQuota (uint16_t value)
{
  NS_LOG_FUNCTION (this << value);

  m_quota = value;
}

void
Slice::SetHostsA (uint16_t value)
{
  NS_LOG_FUNCTION (this << value);

  m_hostsA = value;
}

void
Slice::SetHostsB (uint16_t value)
{
  NS_LOG_FUNCTION (this << value);

  m_hostsB = value;
}

uint16_t
Slice::GetSliceId (void) const
{
  return m_sliceId;
}

uint16_t
Slice::GetPriority (void) const
{
  return m_prio;
}

uint16_t
Slice::GetQuota (void) const
{
  return m_quota;
}

uint16_t
Slice::GetHostsA (void) const
{
  return m_hostsA;
}

uint16_t
Slice::GetHostsB (void) const
{
  return m_hostsB;
}

void
Slice::DoDispose ()
{
  Object::DoDispose ();
}

} // namespace ns3
