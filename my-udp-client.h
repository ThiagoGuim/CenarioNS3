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

#ifndef MY_UDP_CLIENT_H
#define MY_UDP_CLIENT_H

#include <ns3/core-module.h>
#include <ns3/lte-module.h>
#include <ns3/network-module.h>
#include <ns3/internet-module.h>
#include <ns3/lte-module.h>
#include "app-stats-calculator.h"
#include "common.h"

namespace ns3 {

class MyUdpServer;

/**
 * \ingroup svelte
 * \defgroup svelteApps Applications
 * Applications prepared to work with the SVELTE architecture.
 */


/**
 * \ingroup svelteApps
 * This is the client side of a generic UDP traffic generator, sending and
 * receiving UDP datagrams following the configure traffic pattern.
 */
static  Ptr<OutputStreamWrapper>        m_log;        //!< Arquivo de log.

class MyUdpClient : public Application
{
  friend class MyUdpServer;

public:
  MyUdpClient ();            //!< Default constructor.
  virtual ~MyUdpClient ();   //!< Dummy destructor, see DoDispose.

  /**
   * Get the type ID.
   * \return the object TypeId.
   */
  static TypeId GetTypeId (void);

  /**
   * \name Private member accessors.
   * \return The requested value.
   */
  //\{
  std::string                   GetAppName        (void) const;
  Ptr<const AppStatsCalculator> GetAppStats       (void) const;
  EpsBearer                     GetEpsBearer      (void) const;
  uint8_t                       GetEpsBearerId    (void) const;
  Time                          GetMaxOnTime      (void) const;
  std::string                   GetNameTeid       (void) const;
  Ptr<MyUdpServer>                GetServerApp      (void) const;
  Ptr<const AppStatsCalculator> GetServerAppStats (void) const;
  uint32_t                      GetTeid           (void) const;
  std::string                   GetTeidHex        (void) const;
  bool                          IsActive          (void) const;
  bool                          IsForceStop       (void) const;
  //\}

  /**
   * \name Private member modifiers.
   * \param value The value to set.
   */
  //\{
  void                          SetEpsBearer      (EpsBearer  value);
  void                          SetEpsBearerId    (uint8_t    value);
  void                          SetTeid           (uint32_t   value);
  //\}

  /**
   * \brief Set the server application.
   * \param serverApp The pointer to server application.
   * \param serverAddress The Inet socket address of the server.
   */
  void SetServer (Ptr<MyUdpServer> serverApp, Address serverAddress);

  /**
   * Start this application. Reset internal counters, notify the server
   * application, fire the start trace source, and start traffic generation.
   */
  virtual void Start ();

  /**
   * TracedCallback signature for Ptr<MyUdpClient>.
   * \param app The client application.
   */
  typedef void (*EpcAppTracedCallback)(Ptr<MyUdpClient> app);

protected:
  /** Destructor implementation */
  virtual void DoDispose (void);

  /**
   * Force this application to stop. This function will interrupt traffic
   * generation, allowing on-transit packets to reach the destination before
   * closing sockets and notifying the stop event.
   */
  virtual void ForceStop ();

  /**
   * Get the random traffic length for this application.
   * \return The random traffic length.
   */
  Time GetTrafficLength ();

  /**
   * Notify the stop event on this client application. This function is
   * expected to be called only after application traffic is completely stopped
   * (no pending bytes for transmission, no in-transit packets, and closed
   * sockets). It will fire stop trace source.
   * \param withError If true indicates an event with erros, false otherwise.
   */
  virtual void NotifyStop (bool withError = false);

  /**
   * Update TX counter for a new transmitted packet on server stats calculator.
   * \param txBytes The total number of bytes in this packet.
   * \return The next TX sequence number to use.
   */
  uint32_t NotifyTx (uint32_t txBytes);

  /**
   * Update RX counter for a new received packet on client stats calculator.
   * \param rxBytes The total number of bytes in this packet.
   * \param timestamp The timestamp when this packet was sent.
   */
  void NotifyRx (uint32_t rxBytes, Time timestamp = Simulator::Now ());

  Ptr<AppStatsCalculator> m_appStats;         //!< QoS statistics.
  Ptr<Socket>             m_socket;           //!< Local socket.
  uint16_t                m_localPort;        //!< Local port.
  Address                 m_serverAddress;    //!< Server address.
  Ptr<MyUdpServer>        m_serverApp;        //!< Server application.

  /** Trace source fired when application start. */
  TracedCallback<Ptr<MyUdpClient> > m_appStartTrace;

  /** Trace source fired when application stops. */
  TracedCallback<Ptr<MyUdpClient> > m_appStopTrace;

  /** Trace source fired when application reports an error. */
  TracedCallback<Ptr<MyUdpClient> > m_appErrorTrace;

private:
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

  std::string               m_name;           //!< Application name.
  bool                      m_active;         //!< Active state.
  Ptr<RandomVariableStream> m_lengthRng;      //!< Random traffic length.
  Time                      m_maxOnTime;      //!< Max duration time.
  EventId                   m_forceStop;      //!< Max duration stop event.
  bool                      m_forceStopFlag;  //!< Force stop flag.

  // LTE EPS metadata.
  EpsBearer                 m_bearer;         //!< EPS bearer info.
  uint8_t                   m_bearerId;       //!< EPS bearer ID.
  uint32_t                  m_teid;           //!< GTP TEID.

  // UDP data
  Ptr<RandomVariableStream>   m_cbrRng;
  DataRate                    m_cbrRate;      //!< Data rate.
  uint32_t                    m_pktSize;      //!< Pkt size.
  EventId                     m_sendEvent;    //!< SendPacket event.
  EventId                     m_stopEvent;    //!< Stop event.
};

} // namespace ns3
#endif /* MY_UDP_CLIENT_H */
