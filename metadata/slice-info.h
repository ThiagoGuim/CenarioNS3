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

#ifndef SLICE_INFO_H
#define SLICE_INFO_H

#include <ns3/core-module.h>
#include "../application/udp-peer-helper.h"

namespace ns3 {

class SliceInfo;

/** A list of slice information objects. */
typedef std::vector<Ptr<SliceInfo> > SliceInfoList_t;

/**
 * Class for slice metadata.
 */
class SliceInfo : public Object
{
public:
  SliceInfo ();           //!< Default constructor.
  virtual ~SliceInfo ();  //!< Dummy destructor, see DoDispose.

  /**
   * Register this type.
   * \return The object TypeId.
   */
  static TypeId GetTypeId (void);

  /**
   * \name Private member accessors for slice information.
   * \param value The value to set.
   */
  //\{
  void SetSharing     (bool value);
  void SetSliceId     (uint16_t value);
  void SetPriority    (uint16_t value);
  void SetQuota       (uint16_t value);
  void SetNumHostsA   (uint16_t value);
  void SetNumHostsB   (uint16_t value);
  //\}

  void createAppHelpers();
  void removeBraces(std::string *str);
  void removeParenthesis(std::string *str);
  Ipv4Header::DscpType stringToDscpType(std::string dscpString);

  /**
   * \name Private member accessors for slice information.
   * \return The requested information.
   */
  //\{
  bool     GetSharing     (void) const;
  uint16_t GetSliceId     (void) const;
  uint16_t GetPriority    (void) const;
  uint16_t GetQuota       (void) const;
  uint16_t GetNumHostsA   (void) const;
  uint16_t GetNumHostsB   (void) const;
  uint16_t GetNumHostsC   (void) const;
  //\}

  /**
   * Get the application helper to configure the traffic for this slice.
   * \return The application helper.
   */
  Ptr<UdpPeerHelper> GetAppHelper (void);

  /**
   * Get the applications configuration for this slice.
   * \return The applications configuration.
   */
  std::string GetAppsConfig (void);

  /**
   * Get the applications helpers for this slice.
   * \return The application helpers vector.
   */
  std::vector<Ptr<UdpPeerHelper>> GetAppHelpers (void);

  /**
   * Get the number of slices.
   * \return The number of slices
   */
  static size_t GetNSlices (void);

  /**
   * Get the list of slice information.
   * \return The const reference to the list of slice information.
   */
  static const SliceInfoList_t& GetList (void);

  /**
   * Get the slice information from the global map for a given ID.
   * \param sliceId The slice ID.
   * \return The slice information.
   */
  static Ptr<SliceInfo> GetPointer (uint16_t sliceId);

  /**
   * Custom comparator for slice priority.
   * \param slice1 The first slice.
   * \param slice2 The second slice.
   * \return True if the priority of slice1 is lower than slice2.
   */
  static bool PriorityComparator (Ptr<SliceInfo> slice1, Ptr<SliceInfo> slice2);

protected:
  /** Destructor implementation. */
  virtual void DoDispose ();

  // Inherited from ObjectBase.
  void NotifyConstructionCompleted (void);

private:
  /**
   * Register the slice information in global map for further usage.
   * \param lInfo The slice information to save.
   */
  static void RegisterSliceInfo (Ptr<SliceInfo> sliceInfo);

  bool                m_sharing;    //!< Slice sharing bandwitdh.
  uint16_t            m_sliceId;    //!< Slice identifier.
  uint16_t            m_prio;       //!< Slice priority.
  uint16_t            m_quota;      //!< Slice quota.
  uint16_t            m_numHostsA;  //!< Number of hosts attached to switch A.
  uint16_t            m_numHostsB;  //!< Number of hosts attached to switch B.
  Ptr<UdpPeerHelper>  m_appHelper;  //!< Application helper for this slice.


  std::vector<Ptr<UdpPeerHelper>> m_appHelpers; //!< Application helpers for this slice.
  std::string m_appsConfig;//!< Configuration for slice applications.


  /**
   * Map saving slice ID / slice information.
   */
  typedef std::map<uint16_t, Ptr<SliceInfo> > SliceInfoMap_t;
  static SliceInfoMap_t  m_sliceInfoById;   //!< Global slice info map.
  static SliceInfoList_t m_sliceInfoList;   //!< Global list of slice info.
};

} // namespace ns3
#endif /* SLICE_INFO_H */
