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

// #include "qos-queue.h"

// #undef NS_LOG_APPEND_CONTEXT
// #define NS_LOG_APPEND_CONTEXT 
//   std::clog << "[dp " << m_dpId << " port " << m_portNo << "] ";

// namespace ns3 {

// NS_LOG_COMPONENT_DEFINE ("QueueTest");
// NS_OBJECT_ENSURE_REGISTERED (QueueTest);

// TypeId
// QueueTest::GetTypeId (void)
// {
//   static TypeId tid = TypeId ("ns3::QueueTest")
//     .SetParent<OFSwitch13Queue> ()
//     .AddConstructor<QueueTest> ()
//   ;
//   return tid;
// }

// QueueTest::QueueTest ()
//   : OFSwitch13Queue (),
//   NS_LOG_TEMPLATE_DEFINE ("QueueTest")
// {
//   NS_LOG_FUNCTION (this);
// }

// QueueTest::~QueueTest ()
// {
//   NS_LOG_FUNCTION (this);
// }

// Ptr<Packet>
// QueueTest::Dequeue (void)
// {   
  
//   NS_LOG_FUNCTION (this);

//   Ptr<Packet> packet = GetInternalQueue (0)->Dequeue ();
// //   NotifyDequeue (packet); ??
 
//   if (!packet)
//     {
//       NS_LOG_DEBUG ("Queue empty");
//       return 0;
//     }

//   return packet;

// }

// Ptr<Packet>
// QueueTest::Remove (void)
// {
//   NS_LOG_FUNCTION (this);

//   int queueId = GetNextQueueToServe ();
//   if (queueId >= 0)
//     {
//       NS_LOG_DEBUG ("Packet to be removed from queue " << queueId);
//       Ptr<Packet> packet = GetQueue (queueId)->Remove ();
//       NotifyRemove (packet);
//       return packet;
//     }

//   NS_LOG_DEBUG ("Queue empty");
//   return 0;
// }

// Ptr<const Packet>
// QueueTest::Peek (void) const
// {
//   NS_LOG_FUNCTION (this);

//   NS_ABORT_MSG ("Unimplemented method.");
// }

// void
// QueueTest::DoInitialize ()
// {
//   NS_LOG_FUNCTION (this);

//   // Configuring the factory for internal queues.
//   ObjectFactory queueFactory;
//   queueFactory.SetTypeId ("ns3::DropTailQueue<Packet>");
//   queueFactory.Set ("MaxSize", StringValue ("100p"));

//   // Creating the internal queues.
//   for (int queueId = 0; queueId < m_queueNum; queueId++)
//     {
//       AddQueue (queueFactory.Create<Queue<Packet> > ());
//     }

//   // Initializing weights for the WRR algorithm.
//   m_queueTokens = m_queueWeight;

//   // Chain up.
//   OFSwitch13Queue::DoInitialize ();
// }

// int
// QueueTest::GetNextQueueToServe (void)
// {
//   NS_LOG_FUNCTION (this);

//   // Always check for packets in the priority queue.
//   if (GetQueue (0)->IsEmpty () == false)
//     {
//       return 0;
//     }

//   // Check for packets in other queues, respecting the WRR algorithm.
//   bool hasPackets = false;
//   for (int queueId = 1; queueId < GetNQueues (); queueId++)
//     {
//       if (GetQueue (queueId)->IsEmpty () == false)
//         {
//           hasPackets = true;
//           if (m_queueTokens [queueId] > 0)
//             {
//               m_queueTokens [queueId] -= 1;
//               return queueId;
//             }
//         }
//     }

//   // If we get here it is because we have at least one non-empty queue and no
//   // more tokens for non-empty queues. Let's reset the tokens and start again.
//   if (hasPackets)
//     {
//       NS_LOG_DEBUG ("Reseting queue tokens.");
//       m_queueTokens = m_queueWeight;
//       return GetNextQueueToServe ();
//     }

//   // All queues are empty.
//   return -1;
// }

// Ptr<Queue<Packet>>
// QueueTest::GetInternalQueue (std::size_t i)
// {
//   NS_LOG_FUNCTION (this);

//   NS_ASSERT (i < m_queueNum.size ());
//   return GetQueue(i);

// }

// } // namespace ns3
