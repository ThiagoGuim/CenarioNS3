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
 *         Vitor M. Eichemberger <vitor.marge@gmail.com>
 *
 * Two hosts connected to different OpenFlow switches.
 * Both switches are managed by the default learning controller application.
 *
 *                       Learning Controller
 *                                |
 *                         +-------------+
 *                         |             |
 *                  +----------+     +----------+
 *       Host 0 === | Switch 0 | === | Switch 1 | === Host 1
 *                  +----------+     +----------+
 */


#include <ns3/netanim-module.h>

#include <ns3/core-module.h>
#include <ns3/network-module.h>
#include <ns3/csma-module.h>
#include <ns3/internet-module.h>
#include <ns3/ofswitch13-module.h>
#include <ns3/internet-apps-module.h>
#include <ns3/netanim-module.h>
#include <ns3/mobility-module.h>

#include "controller.h"

using namespace ns3;

void printVectors(std::vector<Ptr<OFSwitch13Port>> hostsSWAports, std::vector<Ptr<OFSwitch13Port>> hostsSWBports, std::vector<Ptr<OFSwitch13Port>> serverPorts, std::vector<Ptr<OFSwitch13Port>> interSwitchesPorts){


  for(uint16_t i = 0; i < hostsSWAports.size(); i++)
    std::cout << hostsSWAports[i]->GetPortNo() << std::endl;
  std::cout << "----------------" << std::endl;

  for(uint16_t i = 0; i < hostsSWBports.size(); i++)
    std::cout << hostsSWBports[i]->GetPortNo() << std::endl;
  std::cout << "----------------" << std::endl;

  for(uint16_t i = 0; i < serverPorts.size(); i++)
    std::cout << serverPorts[i]->GetPortNo() << std::endl;
  std::cout << "----------------" << std::endl;
 
  for(uint16_t i = 0; i < interSwitchesPorts.size(); i++)
    std::cout << interSwitchesPorts[i]->GetPortNo() << std::endl;
  std::cout << "----------------" << std::endl;


}


void configure_slice(){


  






  
}




int
main (int argc, char *argv[])
{ 

  Config::SetDefault ("ns3::CsmaChannel::FullDuplex", BooleanValue (true));

  std::string animFile = "animation.xml";

  uint16_t simTime = 10;
  bool verbose = false;
  bool trace = false;

  //Informing the number of clients is enough to set the client/server pairs
  //since there will be one server to each client.

  //Number of hosts linked to the first switch
  uint16_t numberHostsSWA = 1;

  //Number of hosts linked to the second switch
  uint16_t numberHostsSWB = 1;

  //NetAnim or not
  bool create_anim = false;

  // Configure command line parameters
  CommandLine cmd;
  cmd.AddValue ("simTime", "Simulation time (seconds)", simTime);
  cmd.AddValue ("verbose", "Enable verbose output", verbose);
  cmd.AddValue ("trace", "Enable datapath stats and pcap traces", trace);
  cmd.AddValue ("numberHostsSWA", "Number of clients attached to the first switch", numberHostsSWA);
  cmd.AddValue ("numberHostsSWB", "Number of clients attached to the second switch", numberHostsSWB);
  cmd.AddValue ("create_anim", "Enable netAnim", create_anim);
  cmd.Parse (argc, argv);


  uint16_t numberServers = numberHostsSWA + numberHostsSWB; 
  uint16_t numberHosts = numberServers; 


  if (verbose)
    {
      OFSwitch13Helper::EnableDatapathLogs ();
      LogComponentEnable ("OFSwitch13Interface", LOG_LEVEL_ALL);
      LogComponentEnable ("OFSwitch13Device", LOG_LEVEL_ALL);
      LogComponentEnable ("OFSwitch13Port", LOG_LEVEL_ALL);
      LogComponentEnable ("OFSwitch13Queue", LOG_LEVEL_ALL);
      LogComponentEnable ("OFSwitch13SocketHandler", LOG_LEVEL_ALL);
      LogComponentEnable ("OFSwitch13Controller", LOG_LEVEL_ALL);
      LogComponentEnable ("OFSwitch13LearningController", LOG_LEVEL_ALL);
      LogComponentEnable ("OFSwitch13Helper", LOG_LEVEL_ALL);
      LogComponentEnable ("OFSwitch13InternalHelper", LOG_LEVEL_ALL);
    }

  // Enable checksum computations (required by OFSwitch13 module)
  GlobalValue::Bind ("ChecksumEnabled", BooleanValue (true));


  //Create the hosts nodes (->switch 1)
  NodeContainer hostsSWA;
  hostsSWA.Create (numberHostsSWA);
  
  //Create the hosts nodes (->switch 2)
  NodeContainer hostsSWB;
  hostsSWB.Create (numberHostsSWB);

  //Create a container with all nodes
  NodeContainer hosts;
  hosts.Add(hostsSWA);
  hosts.Add(hostsSWB);
  

  //Create the servers nodes
  NodeContainer servers;
  servers.Create (numberServers);

  // Create two switch nodes
  NodeContainer switches;
  switches.Create (3);


  // Create the controller node
  Ptr<Node> controllerNode = CreateObject<Node> ();



  // Create the animation object and configure for specified output


  if(create_anim){

    Ptr<ListPositionAllocator> listPosAllocator;
    listPosAllocator = CreateObject<ListPositionAllocator> ();
    

    listPosAllocator->Add (Vector (  75,  25, 0));  // switch A
    listPosAllocator->Add (Vector (  100, 25, 0));  // switch B
    listPosAllocator->Add (Vector ( 125, 25, 0));  // switch Servers
    listPosAllocator->Add (Vector ( 100, 50, 0));  // controller


    for (size_t i = 0; i < numberHosts; i++)
        listPosAllocator->Add (Vector (0, 25 * i, 0)); // Clients

    for (size_t i = 0; i < numberServers; i++)
        listPosAllocator->Add (Vector (200, 25 * i, 0)); // Servers


  

    MobilityHelper mobilityHelper;
    mobilityHelper.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
    mobilityHelper.SetPositionAllocator (listPosAllocator);
    mobilityHelper.Install (NodeContainer (switches, controllerNode, hostsSWA, hostsSWB, servers));

  }
  


  //Create the vectors to allocate the ports
  std::vector<Ptr<OFSwitch13Port>> hostsSWAports;
  std::vector<Ptr<OFSwitch13Port>> hostsSWBports;
  std::vector<Ptr<OFSwitch13Port>> serverPorts;
  std::vector<Ptr<OFSwitch13Port>> interSwitchesPorts;



  // Set the name for each host node.
  for (uint16_t i = 0; i < numberServers; i++)
    {
      std::string NameHosts;
      std::string NameServers;
      NameHosts = "host" + std::to_string(i+1);
      NameServers = "server" + std::to_string(i+1);
      Names::Add (NameHosts, hosts.Get (i));
      Names::Add (NameServers, servers.Get (i));

    }



  Ptr<OFSwitch13InternalHelper> of13Helper = CreateObject<OFSwitch13InternalHelper> ();

  OFSwitch13DeviceContainer switchDevices;
  switchDevices = of13Helper->InstallSwitch(switches);


  // Use the CsmaHelper to connect hosts and switches
  CsmaHelper csmaHelperinterSwitchesPorts;
  csmaHelperinterSwitchesPorts.SetChannelAttribute ("DataRate", DataRateValue (DataRate ("1Gbps")));
  csmaHelperinterSwitchesPorts.SetChannelAttribute ("Delay", TimeValue (MilliSeconds (0)));


  NetDeviceContainer pairDevs;
  CsmaHelper csmaHelperEndPoints;
  csmaHelperEndPoints.SetChannelAttribute ("DataRate", DataRateValue (DataRate ("1Gbps")));
  csmaHelperEndPoints.SetChannelAttribute ("Delay", TimeValue (MilliSeconds (0)));

  pairDevs = csmaHelperEndPoints.Install(switches.Get(0), switches.Get(1));
  interSwitchesPorts.push_back(switchDevices.Get(0)->AddSwitchPort(pairDevs.Get(0)));
  interSwitchesPorts.push_back(switchDevices.Get(1)->AddSwitchPort(pairDevs.Get(1)));


  pairDevs = csmaHelperEndPoints.Install(switches.Get(1), switches.Get(2));
  interSwitchesPorts.push_back(switchDevices.Get(1)->AddSwitchPort(pairDevs.Get(0)));
  interSwitchesPorts.push_back(switchDevices.Get(2)->AddSwitchPort(pairDevs.Get(1)));
  

  NetDeviceContainer hostDevicesSWA;
  for (size_t i = 0; i < numberHostsSWA; i++)
  {   

    pairDevs = csmaHelperEndPoints.Install(hostsSWA.Get(i), switches.Get(0));
    hostDevicesSWA.Add (pairDevs.Get(0));
    hostsSWAports.push_back(switchDevices.Get(0)->AddSwitchPort(pairDevs.Get(1)));

  }

  NetDeviceContainer hostDevicesSWB;
  for (size_t i = 0; i < numberHostsSWB; i++)
  {   

    pairDevs = csmaHelperEndPoints.Install(hostsSWB.Get(i), switches.Get(1));
    hostDevicesSWB.Add (pairDevs.Get(0));
    hostsSWBports.push_back(switchDevices.Get(1)->AddSwitchPort(pairDevs.Get(1)));

  }

  NetDeviceContainer serversDevices;
  for (size_t i = 0; i < numberServers; i++)
  {   

    pairDevs = csmaHelperEndPoints.Install(servers.Get(i), switches.Get(2));
    serversDevices.Add (pairDevs.Get(0));
    serverPorts.push_back(switchDevices.Get(2)->AddSwitchPort(pairDevs.Get(1)));

  }


  //Inicialize controller
  Ptr<Controller> controllerApp = CreateObject<Controller>();
  of13Helper->InstallController (controllerNode, controllerApp);
  of13Helper->CreateOpenFlowChannels ();
  //---------------//



  // Install the TCP/IP stack into hosts nodes
  InternetStackHelper internet;
  internet.Install (hostsSWA);
  internet.Install (hostsSWB);
  internet.Install (servers);

  // Set IPv4 host addresses
  Ipv4AddressHelper ipv4helpr;
  ipv4helpr.SetBase ("10.0.0.0", "255.255.0.0", "0.0.1.1");
  Ipv4InterfaceContainer hostIpIfacesSWA = ipv4helpr.Assign (hostDevicesSWA);
  Ipv4InterfaceContainer hostIpIfacesSWB = ipv4helpr.Assign (hostDevicesSWB);


  ipv4helpr.SetBase ("10.0.0.0", "255.255.0.0", "0.0.2.1");
  Ipv4InterfaceContainer serversIpIfaces = ipv4helpr.Assign (serversDevices);

  
  std::cout << "HOSTS SWA: " << std::endl;
  for(size_t i = 0; i < numberHostsSWA; i++)
    std::cout << hostIpIfacesSWA.GetAddress(i) << std::endl;

  std::cout << "HOSTS SWB: " << std::endl;
  for(size_t i = 0; i < numberHostsSWB; i++)
    std::cout << hostIpIfacesSWB.GetAddress(i) << std::endl;

  std::cout << "HOSTS SERVERS: " << std::endl;
  for(size_t i = 0; i < numberServers; i++)
    std::cout << serversIpIfaces.GetAddress(i) << std::endl;


  //Configure ping application between hosts

  ApplicationContainer pingApps;

  for(size_t i = 0; i < numberServers; i++){

    V4PingHelper pingHelper = V4PingHelper (serversIpIfaces.GetAddress (i));
    pingHelper.SetAttribute ("Verbose", BooleanValue (true));
    pingApps = pingHelper.Install (hosts.Get(i));
    pingApps.Start (Seconds (1));

  }
  
  
  // Enable datapath stats and pcap traces at hosts, switch(es), and controller(s)
  if (trace)
    {
      of13Helper->EnableOpenFlowPcap ("openflow");
      of13Helper->EnableDatapathStats ("switch-stats");
      //csmaHelperinterSwitchesPorts.EnablePcapAll("logPcap", true);
      //csmaHelperinterSwitchesPorts.EnablePcap ("switch", switchDevices, true);
      csmaHelperinterSwitchesPorts.EnablePcap ("hostSWA", hostDevicesSWA, true);
      csmaHelperinterSwitchesPorts.EnablePcap ("hostSWB", hostDevicesSWB, true);
      csmaHelperinterSwitchesPorts.EnablePcap ("server", serversDevices, true);
    }
  

  //ArpCache::PopulateArpCaches ();


  printVectors(hostsSWAports, hostsSWBports, serverPorts, interSwitchesPorts);


  if(create_anim){
    AnimationInterface anim (animFile);
    anim.SetStartTime (Seconds (0));
    anim.SetStopTime (Seconds (10));
  }
  

  // Run the simulation
  Simulator::Stop (Seconds (simTime));
  Simulator::Run ();
  Simulator::Destroy ();
}
