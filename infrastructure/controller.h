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

#ifndef CONTROLLER_H
#define CONTROLLER_H

#include <ns3/ofswitch13-controller.h>
#include <ns3/internet-module.h>
#include <ns3/ofswitch13-module.h>

// #include "../infrastructure/qos-queue.h"
#include "../infrastructure/scenario1.h"
#include "../infrastructure/scenario2.h"
#include "../infrastructure/scenario3.h"
#include "../infrastructure/scenario4.h"
#include "../metadata/link-info.h"
#include "../metadata/slice-info.h"

namespace ns3 {

class LinkInfo;

typedef std::vector<Ptr<OFSwitch13Port> > PortsList_t;

/**
 * Our custom controller.
 */
class Controller : public OFSwitch13Controller
{
public:
  Controller ();          //!< Default constructor
  virtual ~Controller (); //!< Dummy destructor.

  uint8_t GetScenarioConfig(void);


  /**
   * Register this type.
   * \return The object TypeId.
   */
  static TypeId GetTypeId (void);

  /**
   * Notify the controller of a new host node connected to the OpenFlow network.
   * \param swPort The OpenFlow port at the switch device.
   * \param hostDev The device created at the host node.
   */
  void NotifyHost (Ptr<OFSwitch13Port> swPort, Ptr<NetDevice> hostDev);

  /**
   * Notify the controller of all slices created and configured.
   * \param slices The list of slice information.
   */
  void NotifySlices (SliceInfoList_t slices);

  /**
   * Notify the controller that all switches have been configured
   * so it can install the respectives forwarding rules.
   * \param switchDevices The OpenFlow switch devices.
   * \param switchPorts The ports between the switches.
   */
  void NotifySwitches (OFSwitch13DeviceContainer switchDevices,
                       PortsList_t switchPorts);

  /**
   * Get the list of slices.
   * \param sharing Filter slices with enabled bandwdith sharing flag.
   * \return The list of slices.
   */
  const SliceInfoList_t& GetSliceList (bool sharing = false) const;

protected:
  /** Destructor implementation */
  virtual void DoDispose ();

  // Inherited from ObjectBase.
  virtual void NotifyConstructionCompleted (void);

  // Inherited from OFSwitch13Controller.
  virtual ofl_err HandleError (
    struct ofl_msg_error *msg, Ptr<const RemoteSwitch> swtch, uint32_t xid);
  virtual ofl_err HandleFlowRemoved (
    struct ofl_msg_flow_removed *msg, Ptr<const RemoteSwitch> swtch, uint32_t xid);
  virtual ofl_err HandlePacketIn (
    struct ofl_msg_packet_in *msg, Ptr<const RemoteSwitch> swtch, uint32_t xid);
  virtual void HandshakeSuccessful (
    Ptr<const RemoteSwitch> swtch);
  // Inherited from OFSwitch13Controller.

private:
/**
   * Periodically check for infrastructure bandwidth utilization over backhaul
   * links to adjust extra bit rate when in dynamic inter-slice operation mode.
   */
  void SlicingDynamicTimeout (void);

  /**
   * Adjust the infrastructure inter-slicing extra bit rate, depending on the
   * ExtraStep attribute value and current link configuration.
   * \param link The link information.
   * \param dir The link direction.
   */
  void SlicingExtraAdjust (Ptr<LinkInfo> link, LinkInfo::LinkDir dir);

  /**
   * Apply the infrastructure inter-slicing OpenFlow meters.
   * \param swtch The switch information.
   * \param slice The network slice.
   */
  void SlicingMeterApply (Ptr<LinkInfo> link, int sliceId);

  /**
   * Adjust the infrastructure inter-slicing OpenFlow meter, depending on the
   * MeterStep attribute value and current link configuration.
   * \param link The link information.
   * \param slice The network slice.
   */
  void SlicingMeterAdjust (Ptr<LinkInfo> link, int sliceId);

  /**
   * Install the infrastructure inter-slicing OpenFlow meters.
   * \param link The link information.
   * \param slice The network slice.
   */
  void SlicingMeterInstall (Ptr<LinkInfo> link, int sliceId);

  DataRate              m_extraStep;      //!< Extra adjustment step.
  DataRate              m_guardStep;      //!< Dynamic slice link guard.
  DataRate              m_meterStep;      //!< Meter adjustment step.
  SliceMode             m_sliceMode;      //!< Inter-slicing operation mode.
  Time                  m_sliceTimeout;   //!< Dynamic slice timeout interval.
  OpMode                m_spareUse;       //!< Spare bit rate sharing mode.
  SliceInfoList_t       m_slicesAll;      //!< All slices.
  SliceInfoList_t       m_slicesSha;      //!< Slices sharing bandwidth
  uint8_t m_scenarioConfig;
};

} // namespace ns3
#endif /* OFSWITCH13_LEARNING_CONTROLLER_H */
