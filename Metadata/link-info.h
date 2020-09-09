/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2014 University of Campinas (Unicamp)
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

#ifndef LINK_INFO_H
#define LINK_INFO_H

#include <ns3/core-module.h>
#include <ns3/csma-module.h>
#include <ns3/network-module.h>
#include <ns3/ofswitch13-module.h>
#include "../common.h"

namespace ns3 {

class LinkInfo;

/** A list of link information objects. */
typedef std::vector<Ptr<LinkInfo> > LinkInfoList_t;

/** A set of link information objects. */
typedef std::set<Ptr<LinkInfo> > LinkInfoSet_t;

/**
 * \ingroup svelteInfra
 * Metadata associated to a link between two OpenFlow switches.
 *
 * The link is prepared to handle inter-slicing, and each slice has the
 * following metadata information associated to it:
 * - The slice quota, updated by the controller;
 * - The extra (over quota) bit rate, updated by the controller;
 * - The meter bit rate, updated by the controller;
 * - The reserved bit rate, updated by the controller;
 * - The transmitted bytes, updated by NotifyTxPacket method;
 * - The average throughput, periodically updated by EwmaUpdate method;
 *
 * The figure below shows the relationship among link and slice bit rates:
 * \verbatim
 *
 * |------------------------- Link bit rate -------------------------|
 *
 * |------------ Slice quota ------------|----- Extra -----|
 *                                       :                 :
 * |----------------------- Maximum -----------------------|
 *                                       :                 :
 * |-- Reserved --|-------------- Unreserved --------------|
 *                                       :                 :
 * |-- GBR use --|---------- Non-GBR use ---------|        :
 *                                                :        :
 * |-------------------- Use ---------------------|        :
 *                                       :        :        :
 *                                       |- Over -|- Idle -|
 * \endverbatim
 */
class LinkInfo : public Object
{
  friend class Controller;

public:
  /** Link direction. */
  enum LinkDir
  {
    // Don't change the order. Enum values are used as array indexes.
    FWD = 0,  //!< Forward direction (from first to second switch).
    BWD = 1   //!< Backward direction (from second to first switch).
  };

  // Total number of valid LinkDir items + 1.
  #define N_LINK_DIRS (static_cast<int> (LinkInfo::BWD) + 1)

  /**
   * Complete constructor.
   * \param port1 First switch port.
   * \param port2 Second switch port.
   * \param channel The CsmaChannel physical link connecting these ports.
   * \attention The port order must be the same as created by the CsmaHelper.
   * Internal channel handling is based on this order to get correct
   * full-duplex links.
   */
  LinkInfo (Ptr<OFSwitch13Port> port1, Ptr<OFSwitch13Port> port2,
            Ptr<CsmaChannel> channel);
  virtual ~LinkInfo ();   //!< Dummy destructor, see DoDispose.

  /**
   * Register this type.
   * \return The object TypeId.
   */
  static TypeId GetTypeId (void);

  /**
   * \name Private OpenFlow switch accessors.
   * \param idx The internal switch index.
   * \return The requested field.
   */
  //\{
  Mac48Address          GetPortAddr   (uint8_t idx) const;
  Ptr<CsmaNetDevice>    GetPortDev    (uint8_t idx) const;
  uint32_t              GetPortNo     (uint8_t idx) const;
  Ptr<OFSwitch13Queue>  GetPortQueue  (uint8_t idx) const;
  Ptr<OFSwitch13Device> GetSwDev      (uint8_t idx) const;
  uint64_t              GetSwDpId     (uint8_t idx) const;
  Ptr<OFSwitch13Port>   GetSwPort     (uint8_t idx) const;
  //\}

  /**
   * For two switches, this methods asserts that both datapath IDs are valid
   * for this link, and identifies the link direction based on source and
   * destination datapath IDs.
   * \param src The source switch datapath ID.
   * \param dst The destination switch datapath ID.
   * \return The link direction.
   */
  LinkInfo::LinkDir GetLinkDir (
    uint64_t src, uint64_t dst) const;

  /**
   * Inspect physical channel for half-duplex or full-duplex operation mode.
   * \return True when link in full-duplex mode, false otherwise.
   */
  bool IsFullDuplexLink (void) const;

  /**
   * Inspect physical channel for the assigned bit rate, which is the same for
   * both directions in full-duplex links.
   * \return The channel maximum nominal bit rate (bps).
   */
  int64_t GetLinkBitRate (void) const;

  /**
   * Get the slice quota for this link on the given direction,
   * optionally filtered by the network slice.
   * \param dir The link direction.
   * \param slice The network slice.
   * \return The slice quota.
   */
  int GetQuota (
    LinkDir dir, int slice = SLICE_ALL) const;

  /**
   * Get the quota bit rate for this link on the given direction,
   * optionally filtered by the network slice.
   * \param dir The link direction.
   * \param slice The network slice.
   * \return The maximum bit rate.
   */
  int64_t GetQuoBitRate (
    LinkDir dir, int slice = SLICE_ALL) const;

  /**
   * Get the maximum bit rate for this link on the given direction,
   * optionally filtered by the network slice.
   * \param dir The link direction.
   * \param slice The network slice.
   * \param type Traffic QoS type.
   * \return The reserved bit rate.
   */
  int64_t GetMaxBitRate (
    LinkDir dir, int slice = SLICE_ALL) const;

  /**
   * Get the reserved bit rate for this link on the given direction,
   * optionally filtered by the network slice.
   * \param dir The link direction.
   * \param slice The network slice.
   * \return The reserved bit rate.
   */
  int64_t GetResBitRate (
    LinkDir dir, int slice = SLICE_ALL) const;

  /**
   * Get the unreservedbit rate for this link on the given direction,
   * optionally filtered by the network slice.
   * \param dir The link direction.
   * \param slice The network slice.
   * \param type Traffic QoS type.
   * \return The available bit rate.
   */
  int64_t GetUnrBitRate (
    LinkDir dir, int slice = SLICE_ALL) const;

  /**
   * Get the EWMA throughput bit rate for this link on the given direction,
   * optionally filtered by the network slice and QoS traffic type.
   * \param dir The link direction.
   * \param slice The network slice.
   * \param type Traffic QoS type.
   * \return The EWMA throughput.
   */
  int64_t GetUseBitRate (
    LinkDir dir, int slice = SLICE_ALL, QosType type = QosType::BOTH) const;

  /**
   * Get the EWMA idle (not used) bit rate for this link on the given direction,
   * optionally filtered by the network slice.
   * \param dir The link direction.
   * \param slice The network slice.
   * \return The EWMA throughput.
   */
  int64_t GetIdlBitRate (
    LinkDir dir, int slice = SLICE_ALL) const;

  /**
   * Get the EWMA over (after quota) bit rate for this link on the given
   * direction, optionally filtered by the network slice.
   * \param dir The link direction.
   * \param slice The network slice.
   * \return The EWMA throughput.
   */
  int64_t GetOveBitRate (
    LinkDir dir, int slice = SLICE_ALL) const;

  /**
   * Get the extra bit rate for this link on the given direction,
   * optionally filtered by the network slice.
   * \param dir The link direction.
   * \param slice The network slice.
   * \return The extra bit rate.
   */
  int64_t GetExtBitRate (
    LinkDir dir, int slice = SLICE_ALL) const;

  /**
   * Get the meter bit rate for this link on the given direction,
   * optionally filtered by the network slice.
   * \param dir The link direction.
   * \param slice The network slice.
   * \return The available bit rate.
   */
  int64_t GetMetBitRate (
    LinkDir dir, int slice = SLICE_ALL) const;

  /**
   * Check for free (not reserved) bit rate that can be reserved for GBR
   * bearers, considering the GBR block threshold.
   * \param dir The link direction.
   * \param slice The network slice.
   * \param bitRate The bit rate to check.
   * \param blockThs The block threshold.
   * \return True if there is available bit rate, false otherwise.
   */
  bool HasBitRate (
    LinkDir dir, int slice, int64_t bitRate, double blockThs) const;

  /**
   * Print the link metadata for a specific link direction and network slice.
   * \param os The output stream.
   * \param dir The link direction.
   * \param slice The network slice.
   * \return The output stream.
   * \internal Keep this method consistent with the PrintHeader () method.
   */
  std::ostream & PrintValues (
    std::ostream &os, LinkDir dir, int slice) const;

  /**
   * Get the string representing the given direction.
   * \param dir The link direction.
   * \return The link direction string.
   */
  static std::string LinkDirStr (LinkDir dir);

  /**
   * Invert the given link direction.
   * \param link The original link direction.
   * \return The inverted link direction.
   */
  static LinkDir InvertDir (LinkDir dir);

  /**
   * Get the list of link information.
   * \return The const reference to the list of link information.
   */
  static const LinkInfoList_t& GetList (void);

  /**
   * Get the link information from the global map for a pair of OpenFlow
   * datapath IDs.
   * \param dpId1 The first datapath ID.
   * \param dpId2 The second datapath ID.
   * \return The link information for this pair of datapath IDs.
   */
  static Ptr<LinkInfo> GetPointer (uint64_t dpId1, uint64_t dpId2);

  /**
   * Get the header for the PrintSliceValues () method.
   * \param os The output stream.
   * \return The output stream.
   * \internal Keep this method consistent with the PrintSliceValues () method.
   */
  static std::ostream & PrintHeader (std::ostream &os);

protected:
  /** Destructor implementation. */
  virtual void DoDispose ();

  // Inherited from ObjectBase.
  void NotifyConstructionCompleted (void);

private:
  /**
   * Notify this link of a successfully transmitted packet in link
   * channel. This method will update internal byte counters.
   * \param packet The transmitted packet.
   */
  void NotifyTxPacket (
    std::string context, Ptr<const Packet> packet);

  /**
   * Update the slice quota for this link on the given direction.
   * \param dir The link direction.
   * \param slice The network slice.
   * \param quota The value to update.
   * \return True if succeeded, false otherwise.
   */
  bool UpdateQuota (
    LinkDir dir, int slice, int quota);

  /**
   * Update the reserved bit rate for this link on the given direction.
   * \param dir The link direction.
   * \param slice The network slice.
   * \param bitRate The value to update.
   * \return True if succeeded, false otherwise.
   */
  bool UpdateResBitRate (
    LinkDir dir, int slice, int64_t bitRate);

  /**
   * Update the extra bit rate for this link on the given direction.
   * \param dir The link direction.
   * \param slice The network slice.
   * \param bitRate The value to update.
   * \return True if succeeded, false otherwise.
   */
  bool UpdateExtBitRate (
    LinkDir dir, int slice, int64_t bitRate);

  /**
   * Set the meter bit rate for this link on the given direction.
   * \param dir The link direction.
   * \param slice The network slice.
   * \param bitRate The value to set.
   * \return True if succeeded, false otherwise.
   */
  bool SetMetBitRate (
    LinkDir dir, int slice, int64_t bitRate);

  /**
   * Update EWMA average statistics.
   */
  void EwmaUpdate (void);

  /**
   * Register the link information in global map for further usage.
   * \param lInfo The link information to save.
   */
  static void RegisterLinkInfo (Ptr<LinkInfo> lInfo);

  /** Metadata associated to a network slice. */
  struct SliceMetadata
  {
    int     quota;                      //!< Slice quota (0-100%).
    int64_t extra;                      //!< Extra (over quota) bit rate.
    int64_t meter;                      //!< OpenFlow meter bit rate.
    int64_t reserved;                   //!< Reserved bit rate.
    int64_t ewmaThp [N_QOS_TYPES_BOTH]; //!< EWMA throughput.
    int64_t txBytes [N_QOS_TYPES_BOTH]; //!< TX byte counter.
  };

  Ptr<CsmaChannel>      m_channel;              //!< The CSMA link channel.
  Ptr<OFSwitch13Port>   m_ports [2];            //!< OpenFlow ports.

  /** Metadata for each network slice in each link direction. */
  SliceMetadata         m_slices [N_LINK_DIRS][SLICE_ALL + 1];

  // EWMA throughput calculation.
  double                m_ewmaAlpha;            //!< EWMA alpha.
  Time                  m_ewmaTimeout;          //!< EWMA update timeout.
  Time                  m_ewmaLastTime;         //!< Last EWMA update time.

  /** A pair of switch datapath IDs. */
  typedef std::pair<uint64_t, uint64_t> DpIdPair_t;

  /**
   * Map saving pair of switch datapath IDs / link information.
   * The pair of switch datapath IDs are saved in increasing order.
   */
  typedef std::map<DpIdPair_t, Ptr<LinkInfo> > LinkInfoMap_t;
  static LinkInfoMap_t  m_linkInfoByDpIds;      //!< Global link info map.

  static LinkInfoList_t m_linkInfoList;         //!< Global link info list.
};

} // namespace ns3
#endif  // LINK_INFO_H
