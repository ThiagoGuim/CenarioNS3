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

#include <iomanip>
#include <iostream>
#include "flow-stats-calculator.h"
#include "../common.h"

using namespace std;

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("FlowStatsCalculator");
NS_OBJECT_ENSURE_REGISTERED (FlowStatsCalculator);

FlowStatsCalculator::FlowStatsCalculator ()
{
  NS_LOG_FUNCTION (this);

  ResetCounters ();
}

FlowStatsCalculator::~FlowStatsCalculator ()
{
  NS_LOG_FUNCTION (this);
}

TypeId
FlowStatsCalculator::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::FlowStatsCalculator")
    .SetParent<Object> ()
    .AddConstructor<FlowStatsCalculator> ()
    .AddAttribute ("Continuous", "Monitoring the traffic continuously.",
                   TypeId::ATTR_GET | TypeId::ATTR_CONSTRUCT,
                   BooleanValue (false),
                   MakeBooleanAccessor (&FlowStatsCalculator::m_continuous),
                   MakeBooleanChecker ())
  ;
  return tid;
}

bool
FlowStatsCalculator::IsContinuous (void) const
{
  return m_continuous;
}

void
FlowStatsCalculator::ResetCounters (void)
{
  NS_LOG_FUNCTION (this);

  m_txPackets = 0;
  m_txBytes = 0;
  m_rxPackets = 0;
  m_rxBytes = 0;
  m_firstTxTime = Time::Max ();
  m_firstRxTime = Time::Max ();
  m_lastTxTime = Time::Min ();
  m_lastRxTime = Time::Min ();
  m_lastTimestamp = Time (0);
  m_lastResetTime = Simulator::Now ();
  m_jitter = 0;
  m_delaySum = Time ();

  for (int r = 0; r < N_DROP_REASONS; r++)
    {
      m_dpBytes [r] = 0;
      m_dpPackets [r] = 0;
    }
}

void
FlowStatsCalculator::NotifyTx (uint32_t txBytes)
{
  NS_LOG_FUNCTION (this << txBytes);

  Time now = Simulator::Now ();

  // Check for the first TX packet.
  if (GetTxPackets () == 0)
    {
      m_firstTxTime = now;
      m_lastTxTime = now;
    }

  m_txPackets++;
  m_txBytes += txBytes;

  m_lastTxTime = now;
}

void
FlowStatsCalculator::NotifyRx (uint32_t rxBytes, Time timestamp)
{
  NS_LOG_FUNCTION (this << rxBytes << timestamp);

  Time now = Simulator::Now ();

  // Check for the first RX packet.
  if (GetRxPackets () == 0)
    {
      m_firstRxTime = now;
      m_lastRxTime = now;
      m_lastTimestamp = timestamp;
    }

  m_rxPackets++;
  m_rxBytes += rxBytes;

  // Updating the jitter estimation and delay sum.
  // The jitter is calculated using the RFC 1889 (RTP) jitter definition.
  Time delta = (now - m_lastRxTime) - (timestamp - m_lastTimestamp);
  m_jitter += ((Abs (delta)).GetTimeStep () - m_jitter) >> 4;
  m_delaySum += (now - timestamp);

  m_lastRxTime = now;
  m_lastTimestamp = timestamp;
}

void
FlowStatsCalculator::NotifyDrop (uint32_t dpBytes, DropReason reason)
{
  NS_LOG_FUNCTION (this << dpBytes << reason);

  m_dpPackets [reason]++;
  m_dpBytes [reason] += dpBytes;

  m_dpPackets [DropReason::ALL]++;
  m_dpBytes [DropReason::ALL] += dpBytes;
}

int64_t
FlowStatsCalculator::GetDpBytes (DropReason reason) const
{
  NS_LOG_FUNCTION (this << reason);

  return m_dpBytes [reason];
}

int64_t
FlowStatsCalculator::GetDpPackets (DropReason reason) const
{
  NS_LOG_FUNCTION (this << reason);

  return m_dpPackets [reason];
}

Time
FlowStatsCalculator::GetActiveTime (void) const
{
  NS_LOG_FUNCTION (this);

  if (IsContinuous ())
    {
      return Simulator::Now () - m_lastResetTime;
    }
  else
    {
      if (GetTxPackets () || GetRxPackets ())
        {
          return GetLastTxRxTime () - GetFirstTxRxTime ();
        }
      else
        {
          return Time (0);
        }
    }
}

int64_t
FlowStatsCalculator::GetLostPackets (void) const
{
  NS_LOG_FUNCTION (this);

  if (GetTxPackets () > 0 && GetRxPackets () >= 0
      && GetTxPackets () > GetRxPackets ())
    {
      return GetTxPackets () - GetRxPackets ();
    }
  else
    {
      return 0;
    }
}

double
FlowStatsCalculator::GetLossRatio (void) const
{
  NS_LOG_FUNCTION (this);

  if (GetLostPackets ())
    {
      return static_cast<double> (GetLostPackets ()) / GetTxPackets ();
    }
  else
    {
      return 0.0;
    }
}

int64_t
FlowStatsCalculator::GetTxPackets (void) const
{
  NS_LOG_FUNCTION (this);

  return m_txPackets;
}

int64_t
FlowStatsCalculator::GetTxBytes (void) const
{
  NS_LOG_FUNCTION (this);

  return m_txBytes;
}

int64_t
FlowStatsCalculator::GetRxPackets (void) const
{
  NS_LOG_FUNCTION (this);

  return m_rxPackets;
}

int64_t
FlowStatsCalculator::GetRxBytes (void) const
{
  NS_LOG_FUNCTION (this);

  return m_rxBytes;
}

Time
FlowStatsCalculator::GetRxDelay (void) const
{
  NS_LOG_FUNCTION (this);

  if (GetRxPackets ())
    {
      return m_delaySum / static_cast<int64_t> (GetRxPackets ());
    }
  else
    {
      return m_delaySum;
    }
}

Time
FlowStatsCalculator::GetRxJitter (void) const
{
  NS_LOG_FUNCTION (this);

  return Time (m_jitter);
}

DataRate
FlowStatsCalculator::GetRxThroughput (void) const
{
  NS_LOG_FUNCTION (this);

  if (GetActiveTime ().IsZero ())
    {
      return DataRate (0);
    }
  else
    {
      return DataRate (GetRxBytes () * 8 / GetActiveTime ().GetSeconds ());
    }
}

std::ostream &
FlowStatsCalculator::PrintHeader (std::ostream &os)
{
  os << " " << setw (8)  << "ActvSec"
     << " " << setw (7)  << "TxPkts"
     << " " << setw (7)  << "RxPkts"
     << " " << setw (8)  << "TxBytes"
     << " " << setw (8)  << "RxBytes"
     << " " << setw (8)  << "DlyMsec"
     << " " << setw (8)  << "JitMsec"
     << " " << setw (11) << "ThpKbps"
     << " " << setw (8)  << "DpBytes"
     << " " << setw (6)  << "DpPkts"
     << " " << setw (6)  << "DpQue"
     << " " << setw (6)  << "DpSli"
     << " " << setw (6)  << "DpMbr"
     << " " << setw (6)  << "DpLoa"
     << " " << setw (6)  << "DpTab";
  return os;
}

void
FlowStatsCalculator::DoDispose ()
{
  NS_LOG_FUNCTION (this);

  Object::DoDispose ();
}

Time
FlowStatsCalculator::GetFirstTxRxTime (void) const
{
  return Min (m_firstTxTime, m_firstRxTime);
}

Time
FlowStatsCalculator::GetLastTxRxTime (void) const
{
  return Max (m_lastTxTime, m_lastRxTime);
}

std::ostream & operator << (std::ostream &os, const FlowStatsCalculator &stats)
{
  os << " " << setw (8)  << stats.GetActiveTime ().GetSeconds ()
     << " " << setw (7)  << stats.GetTxPackets ()
     << " " << setw (7)  << stats.GetRxPackets ()
     << " " << setw (8)  << stats.GetTxBytes ()
     << " " << setw (8)  << stats.GetRxBytes ()
     << " " << setw (8)  << stats.GetRxDelay ().GetSeconds () * 1000
     << " " << setw (8)  << stats.GetRxJitter ().GetSeconds () * 1000
     << " " << setw (11) << Bps2Kbps (stats.GetRxThroughput ())
     << " " << setw (8)  << stats.GetDpBytes (FlowStatsCalculator::ALL);

  for (int r = FlowStatsCalculator::ALL; r >= 0; r--)
    {
      os << " " << setw (6) << stats.GetDpPackets (
        static_cast<FlowStatsCalculator::DropReason> (r));
    }
  return os;
}

} // Namespace ns3
