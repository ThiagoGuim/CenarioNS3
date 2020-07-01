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
#include <string>

#include "controller.h"


using namespace ns3;

//Print vectors with the switch ports information
void printVectors();

void configure_switches();

void configure_slices(std::string config);


//Global variables

//Variables storing the topology config
uint16_t numberSlices = 0;
uint16_t numberHostsSWA = 0;
uint16_t numberHostsSWB = 0;
uint16_t numberServers = 0;
uint16_t numberHosts = 0;


//Vectors to allocate the ports of the switches
std::vector<Ptr<OFSwitch13Port>> hostsSWAports;
std::vector<Ptr<OFSwitch13Port>> hostsSWBports;
std::vector<Ptr<OFSwitch13Port>> serverPorts;
std::vector<Ptr<OFSwitch13Port>> interSwitchesPorts;

//Containers that will store Node, NetDevice and Interface objects
std::vector<std::vector<NodeContainer>> slice_nodes;
std::vector<std::vector<NetDeviceContainer>> slice_netDevices;
std::vector<std::vector<Ipv4InterfaceContainer>> slice_interfaces;
NodeContainer switches;


//Variable to set the ofswitch13 devices on the switches
OFSwitch13DeviceContainer switchDevices;

//Helper to configure the controller and the switches
Ptr<OFSwitch13InternalHelper> of13Helper = CreateObject<OFSwitch13InternalHelper> ();

//Containers and helper to configure de CSMA links between hosts and switches
NetDeviceContainer pairDevs;
CsmaHelper csmaHelperEndPoints;




int
main (int argc, char *argv[])
{ 

  Config::SetDefault ("ns3::CsmaChannel::FullDuplex", BooleanValue (true));

  std::string animFile = "animation.xml";

  uint16_t simTime = 10;
  bool verbose = false;
  bool trace = false;

  //Must be in format = "na:nb|na:nb|na:nb". 'na' being number of hosts attached to switch 
  //end 'nb' being the number of hosts attached to switch B. The streches indicates which slice
  //hosts belong to.
  std::string config = "";


  //NetAnim or not
  bool create_anim = false;

  // Configure command line parameters
  CommandLine cmd;
  cmd.AddValue ("simTime", "Simulation time (seconds)", simTime);
  cmd.AddValue ("verbose", "Enable verbose output", verbose);
  cmd.AddValue ("trace", "Enable datapath stats and pcap traces", trace);
  cmd.AddValue ("create_anim", "Enable netAnim", create_anim);
  cmd.AddValue ("config", "Configure slices and their hosts", config);
  cmd.Parse (argc, argv);


  //uint16_t numberServers = numberHostsSWA + numberHostsSWB; 
  //uint16_t numberHosts = numberServers; 


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


  if(config.size() != 0){

    configure_switches();
    configure_slices(config); 
  }


  //Inicialize controller
  Ptr<Node> controllerNode = CreateObject<Node> ();
  Ptr<Controller> controllerApp = CreateObject<Controller>();
  of13Helper->InstallController (controllerNode, controllerApp);
  of13Helper->CreateOpenFlowChannels ();
  //---------------//



  // Create the animation object and configure for specified output
  /*
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

  }*/
  

  // Set the name for each host node.
  /*for (uint16_t i = 0; i < numberServers; i++)
    {
      std::string NameHosts;
      std::string NameServers;
      NameHosts = "host" + std::to_string(i+1);
      NameServers = "server" + std::to_string(i+1);
      Names::Add (NameHosts, hosts.Get (i));
      Names::Add (NameServers, servers.Get (i));

    }*/



  //Configure ping application between hosts

  /*ApplicationContainer pingApps;

  for(size_t i = 0; i < numberServers; i++){

    V4PingHelper pingHelper = V4PingHelper (serversIpIfaces.GetAddress (i));
    pingHelper.SetAttribute ("Verbose", BooleanValue (true));
    pingApps = pingHelper.Install (hosts.Get(i));
    pingApps.Start (Seconds (1));

  }*/
  
  
  // Enable datapath stats and pcap traces at hosts, switch(es), and controller(s)
  if (trace)
    {
      of13Helper->EnableOpenFlowPcap ("openflow");
      of13Helper->EnableDatapathStats ("switch-stats");
      //csmaHelperinterSwitchesPorts.EnablePcapAll("logPcap", true);
      //csmaHelperinterSwitchesPorts.EnablePcap ("switch", switchDevices, true);
      //csmaHelperinterSwitchesPorts.EnablePcap ("hostSWA", hostDevicesSWA, true);
      //csmaHelperinterSwitchesPorts.EnablePcap ("hostSWB", hostDevicesSWB, true);
      //csmaHelperinterSwitchesPorts.EnablePcap ("server", serversDevices, true);
    }
  

  //ArpCache::PopulateArpCaches ();


  //printVectors(hostsSWAports, hostsSWBports, serverPorts, interSwitchesPorts);


  /*if(create_anim){
    AnimationInterface anim (animFile);
    anim.SetStartTime (Seconds (0));
    anim.SetStopTime (Seconds (10));
  }*/
  

  // Run the simulation
  Simulator::Stop (Seconds (simTime));
  Simulator::Run ();
  Simulator::Destroy ();
}


void printVectors(){

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

void configure_switches(){

  switches.Create(3);


  //Use the CsmaHelper to inter-connect switches
  switchDevices = of13Helper->InstallSwitch(switches);

  CsmaHelper csmaHelperinterSwitchesPorts;
  csmaHelperinterSwitchesPorts.SetChannelAttribute ("DataRate", DataRateValue (DataRate ("1Gbps")));
  csmaHelperinterSwitchesPorts.SetChannelAttribute ("Delay", TimeValue (MilliSeconds (0)));


  csmaHelperEndPoints.SetChannelAttribute ("DataRate", DataRateValue (DataRate ("1Gbps")));
  csmaHelperEndPoints.SetChannelAttribute ("Delay", TimeValue (MilliSeconds (0)));

  pairDevs = csmaHelperEndPoints.Install(switches.Get(0), switches.Get(1));
  interSwitchesPorts.push_back(switchDevices.Get(0)->AddSwitchPort(pairDevs.Get(0)));
  interSwitchesPorts.push_back(switchDevices.Get(1)->AddSwitchPort(pairDevs.Get(1)));


  pairDevs = csmaHelperEndPoints.Install(switches.Get(1), switches.Get(2));
  interSwitchesPorts.push_back(switchDevices.Get(1)->AddSwitchPort(pairDevs.Get(0)));
  interSwitchesPorts.push_back(switchDevices.Get(2)->AddSwitchPort(pairDevs.Get(1)));



}


void configure_slices(std::string config){


///Parse the string first ------------

  std::string str = config;
  std::string delimiter_switch = ":";
  std::string delimiter_slice = "|";

  size_t pos_slice = 0;
  size_t pos_switch = 0;


  std::string token;
  std::string token_hosts;

  pos_slice = str.find(delimiter_slice);
  pos_switch = str.find(delimiter_switch);

  //Vectors to identify the number of each type of machines that compound the i slice
  std::vector<int> numberHostsSWA_PerSlice;
  std::vector<int> numberHostsSWB_PerSlice;
  std::vector<int> numberServers_PerSlice;
  uint16_t server_count;

  while ((pos_slice = str.find(delimiter_slice)) != std::string::npos) {

    server_count = 0;


    token = str.substr(0, pos_slice);
    std::cout << token << std::endl;

    while ((pos_switch = token.find(delimiter_switch)) != std::string::npos){

      token_hosts = token.substr(0, pos_switch);

      numberHostsSWA = numberHostsSWA + stoi(token_hosts);

      numberHostsSWA_PerSlice.push_back(stoi(token_hosts));
      server_count = server_count + stoi(token_hosts);

      std::cout << token_hosts << std::endl;
      token.erase(0, pos_switch + delimiter_switch.length());
    }


    numberHostsSWB = numberHostsSWB + stoi(token);

    
    numberHostsSWB_PerSlice.push_back(stoi(token));
    server_count = server_count  + stoi(token);

    std::cout << token << std::endl;
    str.erase(0, pos_slice + delimiter_slice.length());

    numberSlices++;

    numberServers_PerSlice.push_back(server_count);

  }

  std::cout << str << std::endl;

  
  server_count = 0;
  while ((pos_switch = str.find(delimiter_switch)) != std::string::npos){

    token_hosts = str.substr(0, pos_switch);

    numberHostsSWA = numberHostsSWA + stoi(token_hosts);  

    numberHostsSWA_PerSlice.push_back(stoi(token_hosts));
    server_count = server_count + stoi(token_hosts);

    std::cout << token_hosts << std::endl;
    str.erase(0, pos_switch + delimiter_switch.length());

  }

  numberSlices++;



  numberHostsSWB = numberHostsSWB + stoi(str);


  numberHostsSWB_PerSlice.push_back(stoi(str));
  server_count = server_count + stoi(str);

  numberServers_PerSlice.push_back(server_count);


  std::cout << str << std::endl;
  std::cout << numberHostsSWA << "/" << numberHostsSWB << std::endl;

//End of parsing ------------

  numberServers = numberHostsSWA + numberHostsSWB;
  numberHosts = numberServers;


  //Creates the NodeContainers and assign them to the respective slice
  for(size_t i = 0; i < numberSlices; i++){

    NodeContainer hostsSWA;
    NodeContainer hostsSWB;
    NodeContainer servers;
    

    hostsSWA.Create(numberHostsSWA_PerSlice[i]);
    hostsSWB.Create(numberHostsSWB_PerSlice[i]);
    servers.Create(numberServers_PerSlice[i]);
    
    std::vector<NodeContainer> machines;

    machines.push_back(hostsSWA);
    machines.push_back(hostsSWB);
    machines.push_back(servers);

    slice_nodes.push_back(machines);
  
  }

  //COUT --------
  for(size_t i = 0; i < slice_nodes.size(); i++)
    for(size_t j = 0; j < slice_nodes[i].size(); j++)
      std::cout << "slice " << i << ": " << slice_nodes[i][j].GetN() << std::endl;
  //--------
  

  //Configure the CSMA links between hosts and switches for each devices
  for(size_t i = 0; i < numberSlices; i++){

    NetDeviceContainer hostDevicesSWA;
    NetDeviceContainer hostDevicesSWB;
    NetDeviceContainer serversDevices;


    for(size_t j = 0; j < slice_nodes[i][0].GetN(); j++){
      //std::cout << slice_nodes[i][0].GetN() << std::endl;
      pairDevs = csmaHelperEndPoints.Install(slice_nodes[i][0].Get(j), switches.Get(0));
      hostDevicesSWA.Add (pairDevs.Get(0));
      hostsSWAports.push_back(switchDevices.Get(0)->AddSwitchPort(pairDevs.Get(1)));
    }

    for(size_t j = 0; j < slice_nodes[i][1].GetN(); j++){
      //std::cout << slice_nodes[i][1].GetN() << std::endl;
      pairDevs = csmaHelperEndPoints.Install(slice_nodes[i][1].Get(j), switches.Get(1));
      hostDevicesSWB.Add (pairDevs.Get(0));
      hostsSWBports.push_back(switchDevices.Get(1)->AddSwitchPort(pairDevs.Get(1)));
    }

    for(size_t j = 0; j < slice_nodes[i][2].GetN(); j++){
      //std::cout << slice_nodes[i][2].GetN() << std::endl;
      pairDevs = csmaHelperEndPoints.Install(slice_nodes[i][2].Get(j), switches.Get(2));
      serversDevices.Add (pairDevs.Get(0));
      serverPorts.push_back(switchDevices.Get(2)->AddSwitchPort(pairDevs.Get(1)));
    }
    
    std::vector<NetDeviceContainer> devices;

    devices.push_back(hostDevicesSWA);
    devices.push_back(hostDevicesSWB);
    devices.push_back(serversDevices);

    slice_netDevices.push_back(devices);
    
  }
  

  //Install the TCP/IP stack into hosts nodes (All hosts of all slices)
  InternetStackHelper internet;

  for(size_t i = 0; i < numberSlices; i++){

    internet.Install (slice_nodes[i][0]); //HostsSWA
    internet.Install (slice_nodes[i][1]); //HostsSWB
    internet.Install (slice_nodes[i][2]); //Servers

  }

  //Set IPv4 host addresses
  Ipv4AddressHelper ipv4helpr;

  

  std::string base_address_str;
  Ipv4Address base_address;
  for(size_t i = 0; i < numberSlices; i++){


    base_address_str = "10.x.y.1";


    uint16_t host_server = 1;

    std::string slice_id = std::to_string(i);
    std::string host_server_str = std::to_string(host_server);

    uint16_t slice_id_len = slice_id.length();
    uint16_t host_server_len = host_server_str.length();

    base_address_str.replace(3, 1, slice_id);
    base_address_str.replace( (4+slice_id_len) , 1, host_server_str);
    
    Ipv4InterfaceContainer hostIpIfacesSWA;
    Ipv4InterfaceContainer hostIpIfacesSWB;
    Ipv4InterfaceContainer serversIpIfaces;

    
    base_address.Set(base_address_str.c_str());
    base_address.Print(std::cout);

    ipv4helpr.SetBase ("10.0.0.0", "255.0.0.0", base_address);
    hostIpIfacesSWA = ipv4helpr.Assign (slice_netDevices[i][0]); //hostDevicesSWA
    hostIpIfacesSWB = ipv4helpr.Assign (slice_netDevices[i][1]); //hostDevicesSWB


    host_server = host_server + 1;
    host_server_str = std::to_string(host_server);
    base_address_str.replace( (4+slice_id_len) , host_server_len, host_server_str);

    std::cout << base_address_str << std::endl;
    base_address.Set(base_address_str.c_str());

    ipv4helpr.SetBase ("10.0.0.0", "255.0.0.0", base_address);
    serversIpIfaces = ipv4helpr.Assign (slice_netDevices[i][2]); //serversDevices

    std::vector<Ipv4InterfaceContainer> interfaces;

    interfaces.push_back(hostIpIfacesSWA);
    interfaces.push_back(hostIpIfacesSWB);
    interfaces.push_back(serversIpIfaces);

    slice_interfaces.push_back(interfaces);

  }


  /*for(size_t i = 0; i < numberSlices; i++){

    std::cout << "HOSTS SWA SLICE " << i << ": " << std::endl;

    for(size_t j = 0; j < slice_nodes[i][0].GetN(); j++)
      std::cout << slice_interfaces[i][0].GetAddress(j) << std::endl;

  }

  for(size_t i = 0; i < numberSlices; i++){

    std::cout << "HOSTS SWB SLICE " << i << ": " << std::endl;

    for(size_t j = 0; j < slice_nodes[i][1].GetN(); j++)
      std::cout << slice_interfaces[i][1].GetAddress(j) << std::endl;

  }

  for(size_t i = 0; i < numberSlices; i++){

    std::cout << "HOSTS SERVERS SLICE " << i << ": " << std::endl;

    for(size_t j = 0; j < slice_nodes[i][2].GetN(); j++)
      std::cout << slice_interfaces[i][2].GetAddress(j) << std::endl;

  }*/

}
