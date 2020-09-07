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

#ifndef SLICE_H
#define SLICE_H

#include <ns3/core-module.h>

namespace ns3 {

/**
 * 
 */
class Slice : public Object
{
public:
  Slice ();           //!< Default constructor.
  virtual ~Slice ();  //!< Dummy destructor, see DoDispose.

  /**
   * Register this type.
   * \return The object TypeId.
   */
  static TypeId GetTypeId (void);

  /**
   * Record the identifier of this Slice.
   * \param number that identifies this Slice.
   */
  void SetSliceId(uint16_t value);

  /**
   * Record the priority of this Slice.
   * \param priority number of this Slice .
   */
  void SetPrio(uint16_t value);

  /**
   * Record the quota given to this Slice.
   * \param number that represents the quota given to this Slice.
   */
  void SetQuota(uint16_t value);

  /**
   * Record the number of hosts attached to switch A that belong to this Slice.
   * \param number of hosts attached to switch A that belong to this Slice.
   */
  void SetNumberHostsSWA(uint16_t value);

  /**
   * Record the number of hosts attached to switch B that belong to this Slice.
   * \param number of hosts attached to switch B that belong to this Slice.
   */
  void SetNumberHostsSWB(uint16_t value);

  /**
   * Get this Slice identifier.
   * \return A integer that represents this Slice identifier.
   */
  uint16_t GetSliceId();

  /**
   * Get this Slice priority.
   * \return A integer that represents this Slice priority.
   */
  uint16_t GetPrio();

  /**
   * Get the quota given to this Slice.
   * \return A integer that represents this Slice Quota.
   */
  uint16_t GetQuota();

  /**
   * Get the number of hosts attached to switch A that belong to this Slice.
   * \return The number of hosts attached to switch A that belong to this Slice.
   */
  uint16_t GetNumberHostsSWA();

  /**
   * Get the number of hosts attached to switch B that belong to this Slice.
   * \return The number of hosts attached to switch B that belong to this Slice.
   */
  uint16_t GetNumberHostsSWB();


protected:
  /** Destructor implementation. */
  virtual void DoDispose ();

private:
  
  uint16_t m_sliceId;      //!< Slice identifier.
  uint16_t m_prio;         //!< Slice priority.
  uint16_t m_quota;        //!< The given quota to this Slice.
  uint16_t m_hostsSWA;    //!< Number of hosts attached to switch A.
  uint16_t m_hostsSWB;    //!< Number of hosts attached to switch B.

};

} // namespace ns3
#endif /* SLICE_H */
