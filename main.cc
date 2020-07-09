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

#include <boost/algorithm/string/replace.hpp>
#include <string>

#include "controller.h"


using namespace ns3;


typedef std::vector<std::vector<NodeContainer>> TopologyNodes_t;
typedef std::vector<std::vector<NetDeviceContainer>> TopologyNetDevices_t;



//Print vectors with the switch ports information
void printPorts();

//Set animation configurations
void create_animation();

void configure_switches();

void configure_slices(std::string config);

void EnableOfsLogs (bool);


//Global variables

//NetAnim or not
bool create_anim = false;


//Variables storing the topology config
size_t numberSlices = 0;
size_t numberHostsSWA = 0;
size_t numberHostsSWB = 0;
size_t numberServers = 0;
size_t numberHosts = 0;


//Vectors to allocate the ports of the switches
TopologyPorts_t switch_ports;
PortsVector_t interSwitchesPorts;

//Containers that will store Node, NetDevice and Interface objects
TopologyNodes_t slice_nodes;
TopologyNetDevices_t slice_netDevices;
TopologyIfaces_t slice_interfaces;
NodeContainer switches;


//Variable to set the ofswitch13 devices on the switches
OFSwitch13DeviceContainer switchDevices;

//Helper to configure the controller and the switches
Ptr<OFSwitch13InternalHelper> of13Helper;
Ptr<Node> controllerNode;
Ptr<Controller> controllerApp;

//Containers and helper to configure de CSMA links between hosts and switches
NetDeviceContainer pairDevs;
CsmaHelper csmaHelperEndPoints;
CsmaHelper csmaHelperinterSwitchesPorts;


//Prefixes used by output filenames.
static ns3::GlobalValue
  g_outputPrefix ("OutputPrefix", "Common prefix for input filenames.",
                  ns3::StringValue (std::string ()),
                  ns3::MakeStringChecker ());

int
main (int argc, char *argv[])
{ 

  Config::SetDefault ("ns3::CsmaChannel::FullDuplex", BooleanValue (true));

  std::string animFile = "animation.xml";

  int simTime = 10;
  bool verbose = false;
  bool trace = false;
  bool ofsLog   = false;

  //Must be in format = "na:nb|na:nb|na:nb". 'na' being number of hosts attached to switch 
  //end 'nb' being the number of hosts attached to switch B. The streches indicates which slice
  //hosts belong to.
  std::string config = "";


  // Configure command line parameters
  CommandLine cmd;
  cmd.AddValue ("SimTime", "Simulation time (seconds)", simTime);
  cmd.AddValue ("verbose", "Enable verbose output", verbose);
  cmd.AddValue ("Trace", "Enable datapath stats and pcap traces", trace);
  cmd.AddValue ("Create_anim", "Enable netAnim", create_anim);
  cmd.AddValue ("Config", "Configure slices and their hosts", config);
  cmd.AddValue ("OfsLog",   "Enable ofsoftswitch13 logs.", ofsLog);
  cmd.Parse (argc, argv);



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

  //Enable OfsLogs or not
  EnableOfsLogs (ofsLog);

  controllerNode = CreateObject<Node> ();

  if(config.size() != 0){
    configure_switches();
    configure_slices(config);
    controllerApp->NotifyTopology(slice_interfaces, switch_ports);
  }
 

  // Set the name for each host node.
  for (size_t i = 0; i < numberSlices; i++){ 
      
      std::string NameHosts;
      std::string NameServers;

      for(size_t j = 0; j < slice_nodes[i][0].GetN(); j++){
        NameHosts = "Slice" + std::to_string(i) + "hostSWA" + std::to_string(j+1);
        Names::Add (NameHosts, slice_nodes[i][0].Get (j));
      }
      
      for(size_t j = 0; j < slice_nodes[i][1].GetN(); j++){
        NameHosts = "Slice" + std::to_string(i) + "hostSWB" + std::to_string(j+1);
        Names::Add (NameHosts, slice_nodes[i][1].Get (j));
      }

      for(size_t j = 0; j < slice_nodes[i][2].GetN(); j++){
        NameServers = "Slice" + std::to_string(i) + "server" + std::to_string(j+1);
        Names::Add (NameServers, slice_nodes[i][2].Get (j));
      }
   
  }


  //Configure ping application between hosts
  ApplicationContainer pingApps;

  for(size_t i = 0; i < numberSlices; i++){


    for(size_t j = 0; j < slice_interfaces[i][2].GetN(); j++){ //Server Interfaces

      V4PingHelper pingHelper = V4PingHelper (slice_interfaces[i][2].GetAddress (j));
      pingHelper.SetAttribute ("Verbose", BooleanValue (true));
      pingApps = pingHelper.Install (slice_nodes[i][3].Get(j));
      pingApps.Start (Seconds (1));

    }

  }
  
  
  // Enable datapath stats and pcap traces at hosts, switch(es), and controller(s)
  if (trace)
    {
      of13Helper->EnableOpenFlowPcap ("openflow");
      of13Helper->EnableDatapathStats ("switch-stats");
      //csmaHelperinterSwitchesPorts.EnablePcapAll("logPcap", true);

      for(size_t i = 0; i < numberSlices; i++){

        for(size_t j = 0; j < slice_nodes[i][0].GetN(); j++)
          csmaHelperinterSwitchesPorts.EnablePcap ("hostSWA", slice_netDevices[i][0], true);

        for(size_t j = 0; j < slice_nodes[i][1].GetN(); j++)
          csmaHelperinterSwitchesPorts.EnablePcap ("hostSWB", slice_netDevices[i][1], true);

        for(size_t j = 0; j < slice_nodes[i][2].GetN(); j++)
          csmaHelperinterSwitchesPorts.EnablePcap ("server", slice_netDevices[i][2], true);
      }

      
    }
  

  //ArpCache::PopulateArpCaches ();


  // Run the simulation
  Simulator::Stop (Seconds (simTime));
  Simulator::Run ();
  Simulator::Destroy ();
}


void 
printPorts(){

  for(size_t i = 0; i < numberSlices; i++){

    for(size_t j = 0; j < switch_ports[i][0].size(); j++){

      std::cout << "SLICE " << i << " PORTAS SWA : ";
      std::cout << switch_ports[i][0][j]->GetPortNo() << std::endl;
    }
    std::cout << "----------------" << std::endl;

    for(size_t j = 0; j < switch_ports[i][1].size(); j++){
      std::cout << "SLICE " << i << " PORTAS SWB : ";
      std::cout << switch_ports[i][1][j]->GetPortNo() << std::endl;
    }
    std::cout << "----------------" << std::endl;

    for(size_t j = 0; j < switch_ports[i][2].size(); j++){
      std::cout << "SLICE " << i << " PORTAS SERVERS : ";
      std::cout << switch_ports[i][2][j]->GetPortNo() << std::endl;
    }
    std::cout << "----------------" << std::endl;

  }

  
  for(size_t i = 0; i < interSwitchesPorts.size(); i++){
    std::cout << " PORTAS INTER SWITCH : ";
    std::cout << interSwitchesPorts[i]->GetPortNo() << std::endl;
  }
  std::cout << "----------------" << std::endl;

}

void 
create_animation(){
  // Create the animation object and configure for specified output
  
  if(create_anim){

    Ptr<ListPositionAllocator> listPosAllocator;
    listPosAllocator = CreateObject<ListPositionAllocator> ();
    

    listPosAllocator->Add (Vector (  75,  25, 0));  // switch A
    listPosAllocator->Add (Vector (  100, 25, 0));  // switch B
    listPosAllocator->Add (Vector ( 125, 25, 0));  // switch Servers
    listPosAllocator->Add (Vector ( 100, 50, 0));  // controller


    NodeContainer all_hosts;
    NodeContainer all_servers;

    int start = 0;
    for(size_t i = 0; i < numberSlices; i++){

      size_t j = 0;

      for (j = 0; j < slice_nodes[i][3].GetN(); j++) //Clients-hosts
        listPosAllocator->Add (Vector (0, (start + 25 * j), 0)); 

      for (j = 0; j < slice_nodes[i][2].GetN(); j++) //Servers
        listPosAllocator->Add (Vector (200, (start + 25 * j), 0));

      start = start + (25 * j);

      all_hosts.Add(slice_nodes[i][3]);
      all_servers.Add(slice_nodes[i][2]);
    }

  
    
    MobilityHelper mobilityHelper;
    mobilityHelper.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
    mobilityHelper.SetPositionAllocator (listPosAllocator);
    mobilityHelper.Install (NodeContainer (switches, controllerNode, all_hosts, all_servers));

  }


}


void 
configure_switches(){

  of13Helper = CreateObject<OFSwitch13InternalHelper>();

  switches.Create(3);

  
  //Use the CsmaHelper to inter-connect switches
  switchDevices = of13Helper->InstallSwitch(switches);

  
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


void 
configure_slices(std::string config){


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
  int server_count;

  //Loop that extracts the info about the slice (e.g 3:1)
  while ((pos_slice = str.find(delimiter_slice)) != std::string::npos) {

    server_count = 0;


    token = str.substr(0, pos_slice);
    //std::cout << token << std::endl;

    //Loop that extracts the number of hosts attached to each switch.
    while ((pos_switch = token.find(delimiter_switch)) != std::string::npos){

      token_hosts = token.substr(0, pos_switch);

      int int_token_hosts = stoi(token_hosts);

      numberHostsSWA = numberHostsSWA + int_token_hosts;

      numberHostsSWA_PerSlice.push_back(int_token_hosts);
      server_count = server_count + int_token_hosts;

      NS_ASSERT_MSG(int_token_hosts < 255, "Number of hostsSWA exceeded");

      //std::cout << token_hosts << std::endl;
      token.erase(0, pos_switch + delimiter_switch.length());
    }


    //Extracts info of the last part of the string about the hosts
    int int_token = stoi(token);
    numberHostsSWB = numberHostsSWB + int_token;

    
    numberHostsSWB_PerSlice.push_back(int_token);
    server_count = server_count  + int_token;

    NS_ASSERT_MSG(int_token < 255, "Number of hostsSWB exceeded");

    //std::cout << token << std::endl;
    str.erase(0, pos_slice + delimiter_slice.length());

    numberSlices++;

    NS_ASSERT_MSG(numberSlices < 254, "Number of slices exceeded");

    numberServers_PerSlice.push_back(server_count);

  }

  //std::cout << str << std::endl;

  //Extracts info of the last part of the string
  server_count = 0;
  if ((pos_switch = str.find(delimiter_switch)) != std::string::npos){

    token_hosts = str.substr(0, pos_switch);
    int int_token_hosts = stoi(token_hosts);

    numberHostsSWA = numberHostsSWA + int_token_hosts;  

    numberHostsSWA_PerSlice.push_back(int_token_hosts);
    server_count = server_count + int_token_hosts;

    NS_ASSERT_MSG(int_token_hosts < 255, "Number of hostsSWA exceeded");

    //std::cout << token_hosts << std::endl;
    str.erase(0, pos_switch + delimiter_switch.length());

  }

  numberSlices++;


  int int_str = stoi(str);
  numberHostsSWB = numberHostsSWB + int_str;


  numberHostsSWB_PerSlice.push_back(int_str);
  server_count = server_count + int_str;

  NS_ASSERT_MSG(int_str < 255, "Number of hostsSWB exceeded");

  numberServers_PerSlice.push_back(server_count);

  //std::cout << str << std::endl;
  //std::cout << numberHostsSWA << "/" << numberHostsSWB << std::endl;

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


    NodeContainer all_hostsPerSlice_container(hostsSWA, hostsSWB);
    
    machines.push_back(all_hostsPerSlice_container);

    slice_nodes.push_back(machines);
  
  }

  //COUT --------
  //for(size_t i = 0; i < slice_nodes.size(); i++)
    //for(size_t j = 0; j < slice_nodes[i].size(); j++)
      //std::cout << "slice " << i << ": " << slice_nodes[i][j].GetN() << std::endl;
  //--------
  


  //Configure the CSMA links between hosts and switches for each devices
  for(size_t i = 0; i < numberSlices; i++){


    std::vector<Ptr<OFSwitch13Port>> hostsSWAports;
    std::vector<Ptr<OFSwitch13Port>> hostsSWBports;
    std::vector<Ptr<OFSwitch13Port>> serverPorts;

    std::vector<std::vector<Ptr<OFSwitch13Port>>> ports;

    ports.push_back(hostsSWAports);
    ports.push_back(hostsSWBports);
    ports.push_back(serverPorts);

    switch_ports.push_back(ports);



    NetDeviceContainer hostDevicesSWA;
    NetDeviceContainer hostDevicesSWB;
    NetDeviceContainer serversDevices;

    //Configuring CSMA links of the hostsSWA
    for(size_t j = 0; j < slice_nodes[i][0].GetN(); j++){
      pairDevs = csmaHelperEndPoints.Install(slice_nodes[i][0].Get(j), switches.Get(0));
      hostDevicesSWA.Add (pairDevs.Get(0));
      switch_ports[i][0].push_back(switchDevices.Get(0)->AddSwitchPort(pairDevs.Get(1)));
    }

    //Configuring CSMA links of the hostsSWB
    for(size_t j = 0; j < slice_nodes[i][1].GetN(); j++){
      pairDevs = csmaHelperEndPoints.Install(slice_nodes[i][1].Get(j), switches.Get(1));
      hostDevicesSWB.Add (pairDevs.Get(0));
      switch_ports[i][1].push_back(switchDevices.Get(1)->AddSwitchPort(pairDevs.Get(1)));
    }

    //Configuring CSMA links of the servers
    for(size_t j = 0; j < slice_nodes[i][2].GetN(); j++){
      pairDevs = csmaHelperEndPoints.Install(slice_nodes[i][2].Get(j), switches.Get(2));
      serversDevices.Add (pairDevs.Get(0));
      switch_ports[i][2].push_back(switchDevices.Get(2)->AddSwitchPort(pairDevs.Get(1)));
    }
    
    std::vector<NetDeviceContainer> devices;

    devices.push_back(hostDevicesSWA);
    devices.push_back(hostDevicesSWB);
    devices.push_back(serversDevices);

    slice_netDevices.push_back(devices);
    
  }
  

  //printPorts();

  //Inicialize controller
  controllerApp = CreateObject<Controller>();
  of13Helper->InstallController (controllerNode, controllerApp);
  of13Helper->CreateOpenFlowChannels ();


  //Install the TCP/IP stack into hosts nodes (All hosts of all slices)
  InternetStackHelper internet;

  for(size_t i = 0; i < numberSlices; i++){

    internet.Install (slice_nodes[i][0]); //HostsSWA
    internet.Install (slice_nodes[i][1]); //HostsSWB
    internet.Install (slice_nodes[i][2]); //Servers

  }

  //Set IPv4 host addresses
  Ipv4AddressHelper ipv4helpr;


  std::string base_address_str = "0.x.y.1";
  Ipv4Address base_address;

  for(size_t i = 0; i < numberSlices; i++){

    Ipv4InterfaceContainer hostIpIfacesSWA;
    Ipv4InterfaceContainer hostIpIfacesSWB;
    Ipv4InterfaceContainer serversIpIfaces;

    std::string base_address_tmp = base_address_str;

    //Number to identify if the address is given to a host or a server 
    int host_server = 1;

    std::string slice_id = std::to_string(i);
    std::string host_server_str = std::to_string(host_server);

    boost::replace_all(base_address_tmp, "x", slice_id);
    boost::replace_all(base_address_tmp, "y", host_server_str);

    base_address.Set(base_address_tmp.c_str());
    ipv4helpr.SetBase ("10.0.0.0", "255.0.0.0", base_address);
    hostIpIfacesSWA = ipv4helpr.Assign (slice_netDevices[i][0]); //hostDevicesSWA
    hostIpIfacesSWB = ipv4helpr.Assign (slice_netDevices[i][1]); //hostDevicesSWB


    base_address_tmp = base_address_str;
    host_server = host_server + 1;
    host_server_str = std::to_string(host_server);
    boost::replace_all(base_address_tmp, "x", slice_id);
    boost::replace_all(base_address_tmp, "y", host_server_str);
    

    base_address.Set(base_address_tmp.c_str());
    ipv4helpr.SetBase ("10.0.0.0", "255.0.0.0", base_address);
    serversIpIfaces = ipv4helpr.Assign (slice_netDevices[i][2]); //serversDevices

    std::vector<Ipv4InterfaceContainer> interfaces;

    interfaces.push_back(hostIpIfacesSWA);
    interfaces.push_back(hostIpIfacesSWB);
    interfaces.push_back(serversIpIfaces);

    slice_interfaces.push_back(interfaces);

  }

}


void
EnableOfsLogs (bool enable)
{
  if (enable)
    {
      StringValue stringValue;
      GlobalValue::GetValueByName ("OutputPrefix", stringValue);
      std::string prefix = stringValue.Get ();
      ofs::EnableLibraryLog (true, prefix);
    }
}