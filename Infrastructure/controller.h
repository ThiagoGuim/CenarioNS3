/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
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

#ifndef CONTROLLER_H
#define CONTROLLER_H

#include <ns3/ofswitch13-controller.h>
#include <ns3/internet-module.h>
#include <ns3/ofswitch13-module.h>

#include "../Metadata/link-info.h"
#include "../Infrastructure/qos-queue.h"
#include "../Metadata/slice.h"

namespace ns3 {

class LinkInfo;

typedef std::vector<Ptr<OFSwitch13Port> > PortsVector_t;
typedef std::vector<std::vector<PortsVector_t> > TopologyPorts_t;
typedef std::vector<std::vector<Ipv4InterfaceContainer> > TopologyIfaces_t;

/**
 * \ingroup ofswitch13
 * \brief An Learning OpenFlow 1.3 controller (works as L2 switch)
 */
class Controller : public OFSwitch13Controller
{
public:
  Controller ();          //!< Default constructor
  virtual ~Controller (); //!< Dummy destructor.

  /**
   * Register this type.
   * \return The object TypeId.
   */
  static TypeId GetTypeId (void);

  /** Destructor implementation */
  virtual void DoDispose ();



  /**
   * Handle flow removed messages sent from switch to this controller. Look for
   * L2 switching information and removes associated entry.
   *
   * \param msg The flow removed message.
   * \param swtch The switch information.
   * \param xid Transaction id.
   * \return 0 if everything's ok, otherwise an error number.
   */
  ofl_err HandleFlowRemoved (
    struct ofl_msg_flow_removed *msg, Ptr<const RemoteSwitch> swtch,
    uint32_t xid);



  /**
   * Notify the controller that all nodes have been configured
   * such as the ports on the switches.
   *
   * \param slice_interfaces The vector that contains the info about nodes addressing.
   * \param switch_ports The vector that contains the pointer to each port on switches.
   * \return void.
   */
  void NotifyClientsServers (
    TopologyIfaces_t sliceInterfaces,
    TopologyPorts_t switchPorts);



  /**
   * Notify the controller that all switches have been configured
   * so it can install the respectives rules.
   *
   * \param interSwitchesPorts Vector that contains the Pointers to the ports between the switches.
   * \param switchDevices Container responsible to store de Devices of each switch.
   * \return void.
   */
  void
  NotifySwitches (PortsVector_t interSwitchesPorts,
                  OFSwitch13DeviceContainer switchDevices);

  /**
   * Install the meters for each slice into Meter Table.
   * 
   * \param sliceQuotas Vector that contains the slice quotas.
   * \return void.
   */
  void
  ConfigureMeters (std::vector<Ptr<Slice>> slices);

  /**
   * Periodically check for infrastructure bandwidth utilization over backhaul
   * links to adjust extra bit rate when in dynamic inter-slice operation mode.
   */
  void SlicingDynamicTimeout (void);

  /**
   * Adjust the infrastructure inter-slicing extra bit rate, depending on the
   * ExtraStep attribute value and current link configuration.
   * \param lInfo The link information.
   * \param dir The link direction.
   */
  void SlicingExtraAdjust (Ptr<LinkInfo> lInfo, LinkInfo::LinkDir dir);


  /**
   * Apply the infrastructure inter-slicing OpenFlow meters.
   * \param swtch The switch information.
   * \param slice The network slice.
   */
  void SlicingMeterApply (Ptr<LinkInfo> lInfo, int sliceId);

  /**
   * Adjust the infrastructure inter-slicing OpenFlow meter, depending on the
   * MeterStep attribute value and current link configuration.
   * \param lInfo The link information.
   * \param slice The network slice.
   */
  void SlicingMeterAdjust (Ptr<LinkInfo> lInfo, int sliceId);

  /**
   * Install the infrastructure inter-slicing OpenFlow meters.
   * \param lInfo The link information.
   * \param slice The network slice.
   */
  void SlicingMeterInstall (Ptr<LinkInfo> lInfo, int sliceId);

  /**
   * \name Attribute accessors.
   * \return The requested information.
   */
  //\{
  SliceMode GetInterSliceMode     (void) const;
  OpMode    GetSpareUseMode       (void) const;
  //\}

  //Poiters to link infos
  Ptr<LinkInfo> m_lInfoA;
  Ptr<LinkInfo> m_lInfoB;

protected:
  // Inherited from OFSwitch13Controller
  void HandshakeSuccessful (Ptr<const RemoteSwitch> swtch);

private:
  

  DataRate              m_extraStep;      //!< Extra adjustment step.
  DataRate              m_guardStep;      //!< Dynamic slice link guard.
  DataRate              m_meterStep;      //!< Meter adjustment step.
  SliceMode             m_sliceMode;      //!< Inter-slicing operation mode.
  Time                  m_sliceTimeout;   //!< Dynamic slice timeout interval.
  OpMode                m_spareUse;       //!< Spare bit rate sharing mode.

  size_t m_numberSlices;

  /** Map saving <IPv4 address / MAC address> */
  typedef std::map<Ipv4Address, Mac48Address> IpMacMap_t;
  IpMacMap_t m_arpTable; //!< ARP resolution table.

  /**
   * \name L2 switching structures
   */
  //\{
  /** L2SwitchingTable: map MacAddress to port */
  typedef std::map<Mac48Address, uint32_t> L2Table_t;

  /** Map datapathID to L2SwitchingTable */
  typedef std::map<uint64_t, L2Table_t> DatapathMap_t;

  /** Switching information for all dapataths */
  DatapathMap_t m_learnedInfo;
  //\}
};

} // namespace ns3
#endif /* OFSWITCH13_LEARNING_CONTROLLER_H */
