/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2016 University of Campinas (Unicamp)
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

#ifndef MY_UDP_SERVER_H
#define MY_UDP_SERVER_H

#include <ns3/core-module.h>
#include <ns3/lte-module.h>
#include <ns3/network-module.h>
#include <ns3/internet-module.h>
#include <ns3/lte-module.h>
#include "app-stats-calculator.h"
#include <ns3/seq-ts-header.h>
#include "common.h"

namespace ns3 {

class MyUdpClient;

/**
 * \ingroup svelteApps
 * This is the server side of a generic UDP traffic generator, sending and
 * receiving UDP datagrams following the configure traffic pattern.
 */

class MyUdpServer : public Application
{
  friend class MyUdpClient;

public:
  MyUdpServer ();            //!< Default constructor.
  virtual ~MyUdpServer ();   //!< Dummy destructor, see DoDispose.


  /**
   * Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);

  /**
   * \name Private member accessors.
   * \return The requested value.
   */
  //\{
  std::string                   GetAppName    (void) const;
  Ptr<const AppStatsCalculator> GetAppStats   (void) const;
  Ptr<MyUdpClient>              GetClientApp  (void) const;
  std::string                   GetTeidHex    (void) const;
  bool                          IsActive      (void) const;
  bool                          IsForceStop   (void) const;
  //\}

  /**
   * \brief Set the client application.
   * \param clientApp The pointer to client application.
   * \param clientAddress The Inet socket address of the client.
   */
  void SetClient (Ptr<MyUdpClient> clientApp, Address clientAddress);

protected:
  /** Destructor implementation. */
  virtual void DoDispose (void);

  /**
   * Notify this server of a start event on the client application. The server
   * must reset internal counters and start traffic when applicable.
   */
  virtual void NotifyStart ();

  /**
   * Notify this server of a force stop event on the client application. When
   * applicable, the server must stop generating traffic.
   */
  virtual void NotifyForceStop ();

  /**
   * Update TX counter for a new transmitted packet on client stats calculator.
   * \param txBytes The total number of bytes in this packet.
   * \return The next TX sequence number to use.
   */
  uint32_t NotifyTx (uint32_t txBytes);

  /**
   * Update RX counter for a new received packet on server stats calculator.
   * \param rxBytes The total number of bytes in this packet.
   * \param timestamp The timestamp when this packet was sent.
   */
  void NotifyRx (uint32_t rxBytes, Time timestamp = Simulator::Now ());

  /**
   * Reset the QoS statistics.
   */
  void ResetAppStats ();


  // Inherited from Application.
  virtual void StartApplication (void);
  virtual void StopApplication (void);


  /**
   * \brief Socket receive callback.
   * \param socket Socket with data available to be read.
   */
  void ReadPacket (Ptr<Socket> socket);
  
  /**
   * \brief Handle a packet transmission.
   */
  void SendPacket ();

  Ptr<AppStatsCalculator> m_appStats;         //!< QoS statistics.
  Ptr<Socket>             m_socket;           //!< Local socket.
  uint16_t                m_localPort;        //!< Local port.
  Address                 m_clientAddress;    //!< Client address.
  Ptr<MyUdpClient>          m_clientApp;        //!< Client application.

  //UDP data
  uint32_t                    m_pktSize;      //!< Pkt size.
  Ptr<RandomVariableStream>   m_pktInterRng;  //!< Pkt inter-arrival time.
  //Ptr<RandomVariableStream>   m_pktSizeRng;   //!< Pkt size.
  EventId                     m_sendEvent;    //!< SendPacket event.
  
};

} // namespace ns3
#endif /* MY_UDP_SERVER_H */
