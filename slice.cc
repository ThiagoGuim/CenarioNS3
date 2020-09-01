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
 * Author: Luciano Chaves <luciano.chaves@ice.ufjf.br>
 */

#include "slice.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("Slice");
NS_OBJECT_ENSURE_REGISTERED (Slice);


Slice::Slice ()
  : m_sliceId (0),
  m_prio (0),
  m_quota (0),
  m_hostsSWA (0),
  m_hostsSWB (0)
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
                   MakeUintegerChecker<uint16_t> ())
    .AddAttribute ("Priority",
                   "Slice priority.",
                   UintegerValue (0),
                   MakeUintegerAccessor (&Slice::m_prio),
                   MakeUintegerChecker<uint16_t> ())
    .AddAttribute ("Quota",
                   "The given quota to this Slice.",
                   UintegerValue (0),
                   MakeUintegerAccessor (&Slice::m_quota),
                   MakeUintegerChecker<uint16_t> ())
    .AddAttribute ("hostsSWA",
                   "Number of hosts attached to switch A.",
                   UintegerValue (0),
                   MakeUintegerAccessor (&Slice::m_hostsSWA),
                   MakeUintegerChecker<uint16_t> ())
    .AddAttribute ("hostsSWB",
                   "Number of hosts attached to switch B.",
                   UintegerValue (0),
                   MakeUintegerAccessor (&Slice::m_hostsSWB),
                   MakeUintegerChecker<uint16_t> ())
  ;
  return tid;
}


void 
Slice::SetSliceId(uint16_t value)
{
  m_sliceId = value;
}

void 
Slice::SetPrio(uint16_t value)
{
  m_prio = value;
}

void 
Slice::SetQuota(uint16_t value)
{
  m_quota = value;
}

void 
Slice::SetNumberHostsSWA(uint16_t value)
{
  m_hostsSWA = value;
}

void 
Slice::SetNumberHostsSWB(uint16_t value)
{
  m_hostsSWB = value;
}

uint16_t 
Slice::GetSliceId()
{
  return m_sliceId;
}

uint16_t
Slice::GetPrio()
{
  return m_prio;
}

uint16_t 
Slice::GetQuota()
{
  return m_quota;
}

uint16_t 
Slice::GetNumberHostsSWA()
{
  return m_hostsSWA;
}

uint16_t 
Slice::GetNumberHostsSWB()
{
  return m_hostsSWB;
}


void
Slice::DoDispose ()
{
  Object::DoDispose ();
}

} // namespace ns3