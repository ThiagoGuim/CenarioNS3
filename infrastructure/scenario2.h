/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2019 University of Campinas (Unicamp)
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

#ifndef SCENARIO_2_H
#define SCENARIO_2_H

#include <ns3/ofswitch13-module.h>

namespace ns3 {

/**
 * \ingroup svelteInfra
 * This class implements the specialized QoS queue for the SVELTE architecture.
 * It holds a priority queue (ID 0) that is always served first, while other
 * queues (IDs 1 to N-1) are served in weighted rounding robin (WRR) order.
 * The drop tail queues are operating in packet mode with size of 100 packets.
 * The total number of queues and the WRR weights can be customized.
 */
class Scenario2queues : public OFSwitch13Queue
{
public:
  /**
   * Register this type.
   * \return The object TypeId.
   */
  static TypeId GetTypeId (void);

  Scenario2queues ();           //!< Default constructor.
  virtual ~Scenario2queues ();  //!< Dummy destructor, see DoDispose.

  // Inherited from Queue.
  Ptr<Packet> Dequeue (void);
  Ptr<Packet> Remove (void);
  Ptr<const Packet> Peek (void) const;

protected:
  // Inherited from Object.
  virtual void DoInitialize (void);

private:
  /**
   * Identify the next non-empty queue to serve, respecting the custom
   * output scheduling algorithm for the SVELTE architecture.
   * \return The queue ID.
   */
  int GetNextQueueToServe (void);

  /**
   * This is the WRR weight assigned to each internal queue, representing the
   * maximum number of packets to serve before reseting the weights. Note that
   * the first weight is zero as this is not used by the priority queue.
   */
  const std::vector<int> m_queueWeight = {4, 1, 0};
  const int              m_queueNum = 3; //!< Total number of queues.
  std::vector<int>       m_queueTokens;  //!< Tokens for WRR scheduling.

  NS_LOG_TEMPLATE_DECLARE;          //!< Redefinition of the log component.
};

} // namespace ns3
#endif /* SCENARIO_2_H */
