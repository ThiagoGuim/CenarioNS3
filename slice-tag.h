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

#ifndef SLICE_TAG_H
#define SLICE_TAG_H

#include "ns3/tag.h"
#include "ns3/nstime.h"
#include "ns3/simulator.h"
#include "common.h"

namespace ns3 {

class Tag;

/**
 * Tag used for GTP packets withing LTE EPC.
 */
class SliceTag : public Tag
{
public:
  /** LTE EPC element where this tagged was inserted into the packet */
  enum EpcInputNode
  {
    ENB = 0,  //!< At the eNB node
    PGW = 1   //!< At the P-GW node
  };

  static TypeId GetTypeId (void);
  virtual TypeId GetInstanceTypeId (void) const;

  /** Constructors */
  SliceTag ();
  //EpcGtpuTag (uint32_t teid, EpcInputNode node, QosType type, bool aggr);

  // Inherited from Tag
  virtual void Serialize (TagBuffer i) const;
  virtual void Deserialize (TagBuffer i);
  virtual uint32_t GetSerializedSize (void) const;
  virtual void Print (std::ostream &os) const;

  /**
   * Private member accessors for tag information.
   * \return The requested information.
   */
  //\{
  //Direction     GetDirection  (void) const;
  //EpcInputNode  GetInputNode  (void) const;
  //QosType       GetQosType    (void) const;
  //SliceId       GetSliceId    (void) const;
  //uint32_t      GetTeid       (void) const;
  Time          GetTimestamp  (void) const;
  //bool          IsAggregated  (void) const;
  uint8_t GetType (void) const;
  uint8_t GetSliceId (void) const;
  uint64_t GetDpId (void) const;
  //\}

  /**
   * Get the EPC input node name.
   * \param node The EPC input node.
   * \return The string with the EPC input node name.
   */
  //static std::string EpcInputNodeStr (EpcInputNode node);

private:
  /**
   * Set internal metadata field.
   * \param node The input node.
   * \param type The QoS traffic type.
   * \param aggr True for aggregated traffic.
   */
  //void SetMetadata (EpcInputNode node, QosType type, bool aggr);

  uint8_t   m_slice;       //!< SliceId.
  uint8_t  m_type;        //!< GTP teid.
  uint64_t  m_time;        //!< Input timestamp.
  uint64_t  m_dpId;        //!< Switch datapath.

};

} // namespace ns3
#endif // SLICE_TAG_H
