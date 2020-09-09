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
  void SetSliceId   (uint16_t value);
  void SetPriority  (uint16_t value);
  void SetQuota     (uint16_t value);
  void SetHostsA    (uint16_t value);
  void SetHostsB    (uint16_t value);
  //\}

  /**
   * \name Private member accessors for slice information.
   * \return The requested information.
   */
  //\{
  uint16_t GetSliceId   (void) const;
  uint16_t GetPriority  (void) const;
  uint16_t GetQuota     (void) const;
  uint16_t GetHostsA    (void) const;
  uint16_t GetHostsB    (void) const;
  //\}

protected:
  /** Destructor implementation. */
  virtual void DoDispose ();

private:
  /**
   * Register the slice information in global map for further usage.
   * \param lInfo The slice information to save.
   */
  static void RegisterSliceInfo (Ptr<SliceInfo> sliceInfo);

  uint16_t m_sliceId;      //!< Slice identifier.
  uint16_t m_prio;         //!< Slice priority.
  uint16_t m_quota;        //!< Slice quota.
  uint16_t m_hostsA;       //!< Number of hosts attached to switch A.
  uint16_t m_hostsB;       //!< Number of hosts attached to switch B.

  /**
   * Map saving slice ID / slice information.
   */
  typedef std::map<uint16_t, Ptr<SliceInfo> > SliceInfoMap_t;
  static SliceInfoMap_t  m_sliceInfoById;   //!< Global slice info map.
  static SliceInfoList_t m_sliceInfoList;   //!< Global list of slice info.
};

} // namespace ns3
#endif /* SLICE_INFO_H */
