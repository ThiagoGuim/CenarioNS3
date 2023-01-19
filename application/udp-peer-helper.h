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

#ifndef UDP_PEER_HELPER_H
#define UDP_PEER_HELPER_H

#include <ns3/core-module.h>
#include <ns3/network-module.h>
#include <ns3/internet-module.h>
#include "udp-peer-app.h"

namespace ns3 {

/**
 * This helper will make life easier for setting up UDP peer applications.
 */
class UdpPeerHelper : public Object
{
public:
  UdpPeerHelper ();           //!< Default constructor.
  virtual ~UdpPeerHelper ();  //!< Dummy destructor, see DoDispose.

  /**
   * Register this type.
   * \return The object TypeId.
   */
  static TypeId GetTypeId (void);

  /**
   * \name Private member accessors for slice information.
   * \return The requested information.
   */
  //\{
  uint16_t GetNumApps     (void) const;
  Time     GetStartOffset (void) const;
  bool     GetFullDuplex  (void) const;
  //\}

  /**
   * Record an attribute to be set in the first application.
   * \param name the name of the attribute to set.
   * \param value the value of the attribute to set.
   */
  void Set1stAttribute (std::string name, const AttributeValue &value);

  /**
   * Record an attribute to be set in the second application.
   * \param name the name of the attribute to set.
   * \param value the value of the attribute to set.
   */
  void Set2ndAttribute (std::string name, const AttributeValue &value);

  /**
   * Record an attribute to be set in both peer applications.
   * \param name the name of the attribute to set.
   * \param value the value of the attribute to set.
   */
  void SetBothAttribute (std::string name, const AttributeValue &value);

  /**
   * Install peer applications on input nodes.
   * \param node1st The container of nodes to install the first app.
   * \param node2nd The container of nodes to install the second app.
   * \param addr1st The container of IPv4 addresses of the first node.
   * \param addr2nd The container of IPv4 addresses of the second node.
   * \param dscp The DSCP value used to set the socket Type of Service field.
   */
  void Install (
    NodeContainer nodes1st, NodeContainer nodes2nd,
    Ipv4InterfaceContainer addr1st, Ipv4InterfaceContainer addr2nd, uint8_t sliceId,
    Ipv4Header::DscpType dscp = Ipv4Header::DscpDefault);

  /**
   * Create a pair of peer applications on input nodes.
   * \param node1st The node to install the first app.
   * \param node2nd The node to install the second app.
   * \param addr1st The IPv4 address of the first node.
   * \param addr2nd The IPv4 address of the second mpde.
   * \param portNo The port number for both nodes.
   * \param dscp The DSCP value used to set the socket Type of Service field.
   * \param startTime The time to start both applications.
   * \param lengthTime The length time for both applications.
   * \return The container with the pair of applications created.
   */
  ApplicationContainer InstallApp (
    Ptr<Node> node1st, Ptr<Node> node2nd,
    Ipv4Address addr1st, Ipv4Address addr2nd,
    uint16_t portNo, Ipv4Header::DscpType dscp,
    Time startTime, Time lengthTime, uint8_t sliceId);

protected:
  /** Destructor implementation. */
  virtual void DoDispose ();

private:
  /**
   * Get the next port number available for use.
   * \return The port number to use.
   */
  static uint16_t GetNextPortNo ();

  ObjectFactory m_app1stFactory;   //!< Application factory.
  ObjectFactory m_app2ndFactory;   //!< Application factory.

  bool                        m_fullDuplex;   //!< Traffic in both directions.
  uint16_t                    m_numApps;      //!< Number of apps per host.
  Time                        m_startOff;     //!< Start offset time.
  Time                        m_startTime;    //!< The cummulative start time.
  Ptr<RandomVariableStream>   m_startRng;     //!< Start interval time.
  Ptr<RandomVariableStream>   m_rateRng;      //!< The app traffic data rate.
  Ptr<RandomVariableStream>   m_lengthRng;    //!< The app traffic length.
  bool 			      m_split;
  Ipv4Header::DscpType m_DSCP; //!< DSCP type.
  TQosType m_tQosType; 

  static uint16_t             m_port;         //!< Port numbers for apps.
};

} // namespace ns3
#endif /* UDP_PEER_HELPER_H */
