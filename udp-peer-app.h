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

#ifndef UDP_PEER_APP
#define UDP_PEER_APP

#include <ns3/core-module.h>
#include <ns3/network-module.h>
#include <ns3/internet-module.h>

namespace ns3 {

/**
 * This is a generic UDP peer application, sending and receiving UDP datagrams
 * following the configure traffic pattern.
 */
class UdpPeerApp : public Application
{
public:
  UdpPeerApp ();            //!< Default constructor.
  virtual ~UdpPeerApp ();   //!< Dummy destructor, see DoDispose.

  /**
   * Get the type ID.
   * \return the object TypeId.
   */
  static TypeId GetTypeId (void);

  /**
   * Start the application traffic.
   */
  void StartTraffic (void);

protected:
  /** Destructor implementation */
  virtual void DoDispose (void);

private:
  // Inherited from Application.
  virtual void StartApplication (void);
  virtual void StopApplication (void);

  /**
   * Socket receive callback.
   * \param socket Socket with data available to be read.
   */
  void ReadPacket (Ptr<Socket> socket);

  /**
   * Handle a packet transmission.
   */
  void SendPacket (void);

  Ptr<Socket>                 m_localSocket;    //!< Local socket.
  uint16_t                    m_localPort;      //!< Local port.
  Address                     m_peerAddress;    //!< Peer address.
  Ptr<RandomVariableStream>   m_pktInterRng;    //!< Packet interval time.
  Ptr<RandomVariableStream>   m_pktSizeRng;     //!< Packet size.
  Ptr<RandomVariableStream>   m_lengthRng;      //!< Traffic length.
  EventId                     m_sendEvent;      //!< SendPacket event.
};

} // namespace ns3
#endif /* UDP_PEER_APP */
