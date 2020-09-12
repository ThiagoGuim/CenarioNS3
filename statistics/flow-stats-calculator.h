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

#ifndef FLOW_STATS_CALCULATOR_H
#define FLOW_STATS_CALCULATOR_H

#include <ns3/core-module.h>
#include <ns3/network-module.h>

namespace ns3 {

/**
 * This class monitors basic QoS statistics at link level in the OpenFlow
 * network. This class monitors some basic QoS statistics of a traffic
 * flow. It counts the number of transmitted, received and dropped bytes and
 * packets. It computes the loss ratio, the average delay, and the jitter.
 */
class FlowStatsCalculator : public Object
{
public:
  /** Reason for packet drops at OpenFlow backhaul network. */
  enum DropReason
  {
    TABLE = 0,    //!< Unmatched packets at flow tables.
    PLOAD = 1,    //!< Switch pipeline capacity overloaded.
    METER = 2,    //!< EPC bearer MBR meter.
    SLICE = 3,    //!< OpenFlow EPC infrastructure slicing.
    QUEUE = 4,    //!< Network device queues.
    ALL   = 5     //!< ALL previous reasons.
  };

  // Total number of DropReason items + 1.
  #define N_DROP_REASONS (static_cast<int> (DropReason::ALL) + 1)

  FlowStatsCalculator ();          //!< Default constructor.
  virtual ~FlowStatsCalculator (); //!< Dummy destructor, see DoDispose.

  /**
   * Register this type.
   * \return The object TypeId.
   */
  static TypeId GetTypeId (void);

  /**
   * Get the continuous attribute value.
   * \return The continuous value.
   */
  bool IsContinuous (void) const;

  /**
   * Reset all internal counters.
   */
  void ResetCounters (void);

  /**
   * Update TX counters for a new transmitted packet.
   * \param txBytes The total number of bytes in this packet.
   * \return The counter of TX packets.
   */
  void NotifyTx (uint32_t txBytes);

  /**
   * Update RX counters for a new received packet.
   * \param rxBytes The total number of bytes in this packet.
   * \param timestamp The timestamp when this packet was sent.
   */
  void NotifyRx (uint32_t rxBytes, Time timestamp = Simulator::Now ());

  /**
   * Update drop counters for a new dropped packet.
   * \param dpBytes The total number of bytes in this packet.
   * \param reason The drop reason.
   */
  void NotifyDrop (uint32_t dpBytes, DropReason reason);

  /**
   * Get the traffic active time. When the ActiveSinceReset flag is true, the
   * GetActiveTime () method will consider the entire interval since the last
   * reset operation, otherwise it will consider the first and last TX/RX
   * events.
   * \return The traffic active time.
   */
  Time GetActiveTime (void) const;

  /**
   * Get QoS statistics.
   * \param reason The drop reason.
   * \return The statistic value.
   */
  //\{
  int64_t   GetDpBytes      (DropReason reason) const;
  int64_t   GetDpPackets    (DropReason reason) const;
  int64_t   GetLostPackets  (void) const;
  double    GetLossRatio    (void) const;
  int64_t   GetTxPackets    (void) const;
  int64_t   GetTxBytes      (void) const;
  int64_t   GetRxPackets    (void) const;
  int64_t   GetRxBytes      (void) const;
  Time      GetRxDelay      (void) const;
  Time      GetRxJitter     (void) const;
  DataRate  GetRxThroughput (void) const;
  //\}

  /**
   * Get the header for the print operator <<.
   * \param os The output stream.
   * \return The output stream.
   * \internal Keep this method consistent with the << operator below.
   */
  static std::ostream & PrintHeader (std::ostream &os);

  /**
   * TracedCallback signature for FlowStatsCalculator.
   * \param stats The statistics.
   */
  typedef void (*FlowStatsCallback)(Ptr<const FlowStatsCalculator> stats);

protected:
  /** Destructor implementation. */
  virtual void DoDispose ();

private:
  /**
   * Get the time of the first and last TX/RX events since last reset.
   * \return The requested value.
   */
  //\{
  Time GetFirstTxRxTime (void) const;
  Time GetLastTxRxTime (void) const;
  //\}

  bool      m_continuous;                 //!< Continuous traffic monitorement.
  int64_t   m_dpPackets [N_DROP_REASONS]; //!< Number of dropped packets.
  int64_t   m_dpBytes [N_DROP_REASONS];   //!< Number of dropped bytes.
  int64_t   m_txPackets;                  //!< Number of TX packets.
  int64_t   m_txBytes;                    //!< Number of TX bytes.
  int64_t   m_rxPackets;                  //!< Number of RX packets.
  int64_t   m_rxBytes;                    //!< Number of RX bytes.
  Time      m_firstTxTime;                //!< First TX time.
  Time      m_firstRxTime;                //!< First RX time.
  Time      m_lastTxTime;                 //!< Last TX time.
  Time      m_lastRxTime;                 //!< Last RX time.
  Time      m_lastTimestamp;              //!< Last timestamp.
  Time      m_lastResetTime;              //!< Last reset time.
  int64_t   m_jitter;                     //!< Jitter estimation.
  Time      m_delaySum;                   //!< Sum of packet delays.
};

/**
 * Print the traffic flow QoS metadata on an output stream.
 * \param os The output stream.
 * \param stats The FlowStatsCalculator object.
 * \returns The output stream.
 * \internal Keep this method consistent with the
 *           FlowStatsCalculator::PrintHeader ().
 */
std::ostream & operator << (std::ostream &os, const FlowStatsCalculator &stats);

} // namespace ns3
#endif /* FLOW_STATS_CALCULATOR_H */
