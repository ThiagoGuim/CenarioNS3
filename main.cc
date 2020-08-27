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
 * Author:
 */

#include <ns3/netanim-module.h>
#include <ns3/core-module.h>
#include <ns3/network-module.h>
#include <ns3/csma-module.h>
#include <ns3/internet-module.h>
#include <ns3/ofswitch13-module.h>
#include <ns3/applications-module.h>
#include <ns3/netanim-module.h>
#include <ns3/mobility-module.h>

#include <boost/algorithm/string/replace.hpp>
#include <string>
#include "controller.h"
#include "udp-peer-helper.h"


using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("Main");

typedef std::vector<std::vector<NodeContainer> > TopologyNodes_t;
typedef std::vector<std::vector<NetDeviceContainer> > TopologyNetDevices_t;


//Print vectors with the switch ports information
void printPorts ();

//Set animation configurations
void createAnimation ();

void configureSwitches ();

void configureSlices (std::string config);

//Enable the log functions
void EnableVerbose (bool enable);
void EnableOfsLogs (bool enable);


//Global variables

//NetAnim or not
bool createAnim = false;


//Variables storing the topology config
size_t numberSlices = 0;
size_t numberHostsSWA = 0;
size_t numberHostsSWB = 0;
size_t numberServers = 0;
size_t numberHosts = 0;


//Vectors to allocate the ports of the switches
TopologyPorts_t switchPorts;
PortsVector_t interSwitchesPorts;

//Containers that will store Node, NetDevice and Interface objects
std::vector<int> sliceQuotas;
TopologyNodes_t sliceNodes;
TopologyNetDevices_t sliceNetDevices;
TopologyIfaces_t sliceInterfaces;
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

  // Customizing the OpenFlow queue type on switch ports.
  Config::SetDefault ("ns3::OFSwitch13Port::QueueFactory", StringValue ("ns3::QosQueue"));

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
  cmd.AddValue ("Verbose", "Enable verbose output", verbose);
  cmd.AddValue ("Trace", "Enable datapath stats and pcap traces", trace);
  cmd.AddValue ("createAnim", "Enable netAnim", createAnim);
  cmd.AddValue ("Config", "Configure slices and their hosts", config);
  cmd.AddValue ("OfsLog",   "Enable ofsoftswitch13 logs.", ofsLog);
  cmd.Parse (argc, argv);


  // Enable checksum computations (required by OFSwitch13 module)
  GlobalValue::Bind ("ChecksumEnabled", BooleanValue (true));

  //Enable OfsLogs or not

  EnableOfsLogs (ofsLog);
  EnableVerbose (verbose);

  controllerNode = CreateObject<Node> ();

  if (config.size () != 0)
    {

      configureSwitches ();
      controllerApp->NotifySwitches (interSwitchesPorts, switchDevices);

      configureSlices (config);
      controllerApp->NotifyClientsServers (sliceInterfaces, switchPorts);

      controllerApp->ConfigureMeters (sliceQuotas);
    }


  // Set the name for each host node.
  for (size_t i = 0; i < numberSlices; i++)
    {

      std::string NameHosts;
      std::string NameServers;

      for (size_t j = 0; j < sliceNodes[i][HOSTS_SWA].GetN (); j++)
        {
          NameHosts = "Slice" + std::to_string (i) + "hostSWA" + std::to_string (j + 1);
          Names::Add (NameHosts, sliceNodes[i][HOSTS_SWA].Get (j));
        }

      for (size_t j = 0; j < sliceNodes[i][HOSTS_SWB].GetN (); j++)
        {
          NameHosts = "Slice" + std::to_string (i) + "hostSWB" + std::to_string (j + 1);
          Names::Add (NameHosts, sliceNodes[i][HOSTS_SWB].Get (j));
        }

      for (size_t j = 0; j < sliceNodes[i][SERVERS].GetN (); j++)
        {
          NameServers = "Slice" + std::to_string (i) + "server" + std::to_string (j + 1);
          Names::Add (NameServers, sliceNodes[i][SERVERS].Get (j));
        }

    }

  for (size_t i = 0; i < numberSlices; i++)
    {
      Ptr<ExponentialRandomVariable> startRng = CreateObject<ExponentialRandomVariable> ();
      startRng->SetAttribute ("Mean", DoubleValue (5));

      // Configuring traffic patterns for this slice
      Ptr<UdpPeerHelper> appHelper = CreateObject<UdpPeerHelper> ();
      // appHelper->SetAttribute ("NumApps", UintegerValue (5));
      // appHelper->SetAttribute ("StartInterval", PointerValue (startRng));

      // // Define the packet size and interval for UDP traffic.
      // appHelper->SetPeerAttribute ("PktInterval", StringValue ("ns3::NormalRandomVariable[Mean=0.01|Variance=0.001]"));
      // appHelper->SetPeerAttribute ("PktSize", StringValue ("ns3::UniformRandomVariable[Min=1024|Max=1460]"));

      // // Disable the traffic at the UDP server node.
      // appHelper->Set2ndAttribute ("PktInterval", StringValue ("ns3::ConstantRandomVariable[Constant=1000000]"));

      appHelper->Install (sliceNodes[i][ALL_HOSTS], sliceNodes[i][SERVERS],
                          sliceInterfaces[i][ALL_HOSTS], sliceInterfaces[i][SERVERS]);

      // for (size_t j = 0; j < sliceNodes[i][ALL_HOSTS].GetN (); j++)
      //   {

      //     appHelper->Install (sliceNodes[i][ALL_HOSTS].Get (j), sliceNodes[i][SERVERS].Get (j),
      //                        sliceInterfaces[i][ALL_HOSTS].GetAddress (j), sliceInterfaces[i][SERVERS].GetAddress (j),
      //                        9, 9, Seconds(5), Seconds (10), Ipv4Header::DscpDefault);

      //   }
    }

  // for (size_t i = 0; i < apps.GetN(); i++)
  // {
  //   Ptr<Application> app = apps.Get(i);
  //   app->SetStartTime(Seconds(i+1.0));
  // }

  //
  // Create a UdpEchoServer application.
  //
  /*ApplicationContainer apps;
  uint16_t port = 9;
  uint32_t MaxPacketSize = 1024;
  uint32_t maxPacketCount = 320;
  Time interPacketInterval = Seconds (0.05);

  for (size_t i = 0; i < numberSlices; i++)
    {

      //Set the servers up
      for (size_t j = 0; j < sliceNodes[i][SERVERS].GetN (); j++)
        {
          UdpEchoServerHelper server (port);
          apps = server.Install (sliceNodes[i][SERVERS].Get (j));
          apps.Start (Seconds (1.0));
          apps.Stop (Seconds (10.0));
        }

      //Set the clients up
      for (size_t j = 0; j < sliceNodes[i][SERVERS].GetN (); j++)
        {
          UdpEchoClientHelper client (sliceInterfaces[i][SERVERS].GetAddress (j), port);
          client.SetAttribute ("MaxPackets", UintegerValue (maxPacketCount));
          client.SetAttribute ("Interval", TimeValue (interPacketInterval));
          client.SetAttribute ("PacketSize", UintegerValue (MaxPacketSize));
          apps = client.Install (sliceNodes[i][ALL_HOSTS].Get (j));
          apps.Start (Seconds (2.0));
          apps.Stop (Seconds (10.0));
        }

    }*/

  // Create the OnOff applications to send data to the UDP receiver
  /*ApplicationContainer onOffApps;
  uint16_t port = 9;

  Address sinkLocalAddress (InetSocketAddress (Ipv4Address::GetAny (), port));
  PacketSinkHelper sinkHelper ("ns3::UdpSocketFactory", sinkLocalAddress);
  ApplicationContainer sinkApp;

  for (size_t i = 0; i < numberSlices; i++)
    {
      for (size_t j = 0; j < sliceNodes[i][SERVERS].GetN (); j++)
        {

          sinkApp.Add (sinkHelper.Install (sliceNodes[i][SERVERS].Get (j)));

          //Set DSCP field
          InetSocketAddress serverInetAddr (sliceInterfaces[i][SERVERS].GetAddress (j), port);
          serverInetAddr.SetTos (15);

          OnOffHelper onOffHelper ("ns3::UdpSocketFactory", serverInetAddr);
          onOffHelper.SetConstantRate (DataRate ("448kb/s"));
          onOffApps.Add (onOffHelper.Install (sliceNodes[i][ALL_HOSTS].Get (j)));

        }
    }

  sinkApp.Start (Seconds (1.0));
  sinkApp.Stop (Seconds (10.0));

  onOffApps.Start (Seconds (2.0));
  onOffApps.Stop (Seconds (9.0));*/

  //Configure an UDP-CLIENT-SERVER application
  /*ApplicationContainer apps;
  uint16_t port = 4000;
  uint32_t MaxPacketSize = 1024;
  uint32_t maxPacketCount = 320;
  Time interPacketInterval = Seconds (0.05);

  for (size_t i = 0; i < numberSlices; i++)
    {

      //Set the servers up
      for(size_t j = 0; j < sliceNodes[i][SERVERS].GetN(); j++){
        UdpServerHelper server (port);
        apps = server.Install (sliceNodes[i][SERVERS].Get(j));
        apps.Start (Seconds (1.0));
        apps.Stop (Seconds (10.0));
      }

      //Set the clients up
      for (size_t j = 0; j < sliceNodes[i][SERVERS].GetN(); j++)
        {
          UdpClientHelper client (sliceInterfaces[i][SERVERS].GetAddress(j), port);
          client.SetAttribute ("MaxPackets", UintegerValue (maxPacketCount));
          client.SetAttribute ("Interval", TimeValue (interPacketInterval));
          client.SetAttribute ("PacketSize", UintegerValue (MaxPacketSize));
          apps = client.Install (sliceNodes[i][ALL_HOSTS].Get(j));
          apps.Start (Seconds (2.0));
          apps.Stop (Seconds (10.0));
        }

    }*/


  //Configure ping application between hosts
  /*ApplicationContainer pingApps;

  for (size_t i = 0; i < numberSlices; i++)
    {


      for (size_t j = 0; j < sliceInterfaces[i][SERVERS].GetN (); j++) //Server Interfaces

        {
          V4PingHelper pingHelper = V4PingHelper (sliceInterfaces[i][SERVERS].GetAddress (j));
          pingHelper.SetAttribute ("Verbose", BooleanValue (true));
          pingApps = pingHelper.Install (sliceNodes[i][ALL_HOSTS].Get (j));
          pingApps.Start (Seconds (1));

        }

    }*/


  // Enable datapath stats and pcap traces at hosts, switch(es), and controller(s)
  if (trace)
    {
      of13Helper->EnableOpenFlowPcap ("openflow");
      of13Helper->EnableDatapathStats ("switch-stats");
      //csmaHelperinterSwitchesPorts.EnablePcapAll("logPcap", true);

      for (size_t i = 0; i < numberSlices; i++)
        {

          for (size_t j = 0; j < sliceNodes[i][HOSTS_SWA].GetN (); j++)
            {
              csmaHelperinterSwitchesPorts.EnablePcap ("hostSWA", sliceNetDevices[i][0], true);
            }

          for (size_t j = 0; j < sliceNodes[i][HOSTS_SWB].GetN (); j++)
            {
              csmaHelperinterSwitchesPorts.EnablePcap ("hostSWB", sliceNetDevices[i][1], true);
            }

          for (size_t j = 0; j < sliceNodes[i][SERVERS].GetN (); j++)
            {
              csmaHelperinterSwitchesPorts.EnablePcap ("server", sliceNetDevices[i][2], true);
            }
        }


    }


  //ArpCache::PopulateArpCaches ();


  // Run the simulation
  Simulator::Stop (Seconds (simTime));
  Simulator::Run ();
  Simulator::Destroy ();

}


void
printPorts ()
{

  for (size_t i = 0; i < numberSlices; i++)
    {

      for (size_t j = 0; j < switchPorts[i][HOSTS_SWA].size (); j++)
        {

          std::cout << "SLICE " << i << " PORTAS SWA : ";
          std::cout << switchPorts[i][HOSTS_SWA][j] -> GetPortNo () << std::endl;
        }
      std::cout << "----------------" << std::endl;

      for (size_t j = 0; j < switchPorts[i][HOSTS_SWB].size (); j ++)
        {
          std::cout << "SLICE " << i << " PORTAS SWB : ";
          std::cout << switchPorts[i][HOSTS_SWB][j] -> GetPortNo () << std::endl;
        }
      std::cout << "----------------" << std::endl;

      for (size_t j = 0; j < switchPorts[i][SERVERS].size (); j ++)
        {
          std::cout << "SLICE " << i << " PORTAS SERVERS : ";
          std::cout << switchPorts[i][SERVERS][j] -> GetPortNo () << std::endl;
        }
      std::cout << "----------------" << std::endl;

    }


  for (size_t i = 0; i < interSwitchesPorts.size (); i ++)
    {
      std::cout << " PORTAS INTER SWITCH : ";
      std::cout << interSwitchesPorts[i] -> GetPortNo () << std::endl;
    }
  std::cout << "----------------" << std::endl;

}

void
createAnimation ()
{
  // Create the animation object and configure for specified output

  if (createAnim)
    {

      Ptr<ListPositionAllocator> listPosAllocator;
      listPosAllocator = CreateObject<ListPositionAllocator> ();


      listPosAllocator->Add (Vector (  75,  25, 0));// switch A
      listPosAllocator->Add (Vector (  100, 25, 0));// switch B
      listPosAllocator->Add (Vector ( 125, 25, 0)); // switch Servers
      listPosAllocator->Add (Vector ( 100, 50, 0)); // controller


      NodeContainer allHosts;
      NodeContainer allServers;

      int start = 0;
      for (size_t i = 0; i < numberSlices; i ++)
      {

        size_t j = 0;

        for (j = 0; j < sliceNodes[i][ALL_HOSTS].GetN (); j ++)  //Clients-hosts
        {
          listPosAllocator->Add (Vector (0, (start + 25 * j), 0));
        }

        for (j = 0; j < sliceNodes[i][SERVERS].GetN (); j++)   //Servers
          {
            listPosAllocator->Add (Vector (200, (start + 25 * j), 0));
          }

        start = start + (25 * j);

        allHosts.Add (sliceNodes[i][ALL_HOSTS]);
        allServers.Add (sliceNodes[i][SERVERS]);
      }



      MobilityHelper mobilityHelper;
      mobilityHelper.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
      mobilityHelper.SetPositionAllocator (listPosAllocator);
      mobilityHelper.Install (NodeContainer (switches, controllerNode, allHosts, allServers));

    }


}


void
configureSwitches ()
{

  of13Helper = CreateObject<OFSwitch13InternalHelper>();
  of13Helper->SetDeviceAttribute ("PipelineTables", UintegerValue (3));

  switches.Create (3);

  //To use in LinkInfo Ccreation
  Ptr<CsmaNetDevice> currPortDev, nextPortDev;

  //Use the CsmaHelper to inter-connect switches
  switchDevices = of13Helper->InstallSwitch (switches);


  csmaHelperinterSwitchesPorts.SetChannelAttribute ("DataRate", DataRateValue (DataRate ("100Mbps")));
  csmaHelperinterSwitchesPorts.SetChannelAttribute ("Delay", TimeValue (MilliSeconds (0)));


  csmaHelperEndPoints.SetChannelAttribute ("DataRate", DataRateValue (DataRate ("100Mbps")));
  csmaHelperEndPoints.SetChannelAttribute ("Delay", TimeValue (MilliSeconds (0)));

  pairDevs = csmaHelperEndPoints.Install (switches.Get (0), switches.Get (1));
  interSwitchesPorts.push_back (switchDevices.Get (0)->AddSwitchPort (pairDevs.Get (0)));
  interSwitchesPorts.push_back (switchDevices.Get (1)->AddSwitchPort (pairDevs.Get (1)));

  //Creating the LinkInfo object between SWA - SWB
  Ptr<CsmaChannel> channelSwaSwb = DynamicCast<CsmaChannel> (pairDevs.Get (0)->GetChannel ());
  CreateObject<LinkInfo> (interSwitchesPorts[0], interSwitchesPorts[1], channelSwaSwb);


  pairDevs = csmaHelperEndPoints.Install (switches.Get (1), switches.Get (2));
  interSwitchesPorts.push_back (switchDevices.Get (1)->AddSwitchPort (pairDevs.Get (0)));
  interSwitchesPorts.push_back (switchDevices.Get (2)->AddSwitchPort (pairDevs.Get (1)));

  //Creating the LinkInfo object between SWB - ServerSwitch
  Ptr<CsmaChannel> channelSwbServerSw = DynamicCast<CsmaChannel> (pairDevs.Get (0)->GetChannel ());
  CreateObject<LinkInfo> (interSwitchesPorts[2], interSwitchesPorts[3], channelSwbServerSw);

  //Inicialize controller
  controllerApp = CreateObject<Controller>();
  of13Helper->InstallController (controllerNode, controllerApp);
  of13Helper->CreateOpenFlowChannels ();

}


void
configureSlices (std::string config)
{


///Parse the string first ------------

  std::string str = config;
  std::string delimiterSwitch = ":";
  std::string delimiterSlice = "|";
  std::string delimiterQuota = ";";

  size_t posSlice = 0;
  size_t posSwitch = 0;
  size_t posQuota = 0;

  std::string token;
  std::string tokenHosts;
  std::string tokenQuota;

  //Vectors to identify the number of each type of machines that compound the i slice
  std::vector<int> numberHostsSwaPerSlice;
  std::vector<int> numberHostsSwbPerSlice;
  std::vector<int> numberServersPerSlice;
  int serverCount;

  int maxQuota = 0;

  //Loop that extracts the info about the slice (e.g 3:1)
  while ((posSlice = str.find (delimiterSlice)) != std::string::npos)
    {

      //Get the quota of the given slice
      if ((posQuota = str.find (delimiterQuota)) != std::string::npos)
        {

          tokenQuota = str.substr (0, posQuota);
          int intTokenQuota = stoi (tokenQuota);
          sliceQuotas.push_back (intTokenQuota);
          maxQuota += intTokenQuota;

          NS_ASSERT_MSG (intTokenQuota >= 1, "Quota exceeded");
          NS_ASSERT_MSG (maxQuota <= 100, "Quota exceeded");

          str.erase (0, posQuota + delimiterQuota.length ());

        }

      serverCount = 0;

      posSlice = str.find (delimiterSlice);
      token = str.substr (0, posSlice);

      //Loop that extracts the number of hosts attached to each switch.
      while ((posSwitch = token.find (delimiterSwitch)) != std::string::npos)
        {

          tokenHosts = token.substr (0, posSwitch);

          int intTokenHosts = stoi (tokenHosts);

          numberHostsSWA = numberHostsSWA + intTokenHosts;

          numberHostsSwaPerSlice.push_back (intTokenHosts);
          serverCount = serverCount + intTokenHosts;

          NS_ASSERT_MSG (intTokenHosts < 255, "Number of hostsSWA exceeded");

          //std::cout << tokenHosts << std::endl;
          token.erase (0, posSwitch + delimiterSwitch.length ());
          //std::cout << token << std::endl;
        }


      //Extracts info of the last part of the string about the hosts
      int intToken = stoi (token);
      numberHostsSWB = numberHostsSWB + intToken;


      numberHostsSwbPerSlice.push_back (intToken);
      serverCount += intToken;

      NS_ASSERT_MSG (intToken < 255, "Number of hostsSWB exceeded");

      //std::cout << token << std::endl;
      str.erase (0, posSlice + delimiterSlice.length ());

      numberSlices++;

      NS_ASSERT_MSG (numberSlices < 254, "Number of slices exceeded");

      numberServersPerSlice.push_back (serverCount);

    }

  //std::cout << str << std::endl;


  //Extracts info of the last part of the string

  if ((posQuota = str.find (delimiterQuota)) != std::string::npos)
    {

      tokenQuota = str.substr (0, posQuota);
      int intTokenQuota = stoi (tokenQuota);
      sliceQuotas.push_back (intTokenQuota);
      maxQuota += intTokenQuota;

      NS_ASSERT_MSG (intTokenQuota >= 1, "Quota exceeded");
      NS_ASSERT_MSG (maxQuota <= 100, "Quota exceeded");

      str.erase (0, posQuota + delimiterQuota.length ());

    }


  serverCount = 0;
  if ((posSwitch = str.find (delimiterSwitch)) != std::string::npos)
    {

      tokenHosts = str.substr (0, posSwitch);
      int intTokenHosts = stoi (tokenHosts);

      numberHostsSWA = numberHostsSWA + intTokenHosts;

      numberHostsSwaPerSlice.push_back (intTokenHosts);
      serverCount = serverCount + intTokenHosts;

      NS_ASSERT_MSG (intTokenHosts < 255, "Number of hostsSWA exceeded");

      //std::cout << tokenHosts << std::endl;
      str.erase (0, posSwitch + delimiterSwitch.length ());

    }

  numberSlices++;


  int intStr = stoi (str);
  numberHostsSWB = numberHostsSWB + intStr;


  numberHostsSwbPerSlice.push_back (intStr);
  serverCount = serverCount + intStr;

  NS_ASSERT_MSG (intStr < 255, "Number of hostsSWB exceeded");

  numberServersPerSlice.push_back (serverCount);

  //std::cout << str << std::endl;
  //std::cout << numberHostsSWA << "/" << numberHostsSWB << std::endl;

//End of parsing ------------

  numberServers = numberHostsSWA + numberHostsSWB;
  numberHosts = numberServers;


  //Creates the NodeContainers and assign them to the respective slice
  for (size_t i = 0; i < numberSlices; i++)
    {

      NodeContainer hostsSWA;
      NodeContainer hostsSWB;
      NodeContainer servers;


      hostsSWA.Create (numberHostsSwaPerSlice[i]);
      hostsSWB.Create (numberHostsSwbPerSlice[i]);
      servers.Create (numberServersPerSlice[i]);

      std::vector<NodeContainer> machines;

      machines.push_back (hostsSWA);
      machines.push_back (hostsSWB);
      machines.push_back (servers);


      NodeContainer allHostsPerSliceContainer (hostsSWA, hostsSWB);

      machines.push_back (allHostsPerSliceContainer);

      sliceNodes.push_back (machines);

    }

  //COUT --------
  //for(size_t i = 0; i < sliceNodes.size(); i++)
  //for(size_t j = 0; j < sliceNodes[i].size(); j++)
  //std::cout << "slice " << i << ": " << sliceNodes[i][j].GetN() << std::endl;
  //--------



  //Configure the CSMA links between hosts and switches for each devices
  for (size_t i = 0; i < numberSlices; i++)
    {


      std::vector<Ptr<OFSwitch13Port> > hostsSWAports;
      std::vector<Ptr<OFSwitch13Port> > hostsSWBports;
      std::vector<Ptr<OFSwitch13Port> > serverPorts;

      std::vector<std::vector<Ptr<OFSwitch13Port> > > ports;

      ports.push_back (hostsSWAports);
      ports.push_back (hostsSWBports);
      ports.push_back (serverPorts);

      switchPorts.push_back (ports);

      NetDeviceContainer hostDevicesSWA;
      NetDeviceContainer hostDevicesSWB;
      NetDeviceContainer serversDevices;

      //Configuring CSMA links of the hostsSWA
      for (size_t j = 0; j < sliceNodes[i][HOSTS_SWA].GetN (); j++)
        {
          pairDevs = csmaHelperEndPoints.Install (sliceNodes[i][HOSTS_SWA].Get (j), switches.Get (0));
          hostDevicesSWA.Add (pairDevs.Get (0));
          switchPorts[i][HOSTS_SWA].push_back (switchDevices.Get (0)->AddSwitchPort (pairDevs.Get (1)));
        }

      //Configuring CSMA links of the hostsSWB
      for (size_t j = 0; j < sliceNodes[i][HOSTS_SWB].GetN (); j++)
        {
          pairDevs = csmaHelperEndPoints.Install (sliceNodes[i][HOSTS_SWB].Get (j), switches.Get (1));
          hostDevicesSWB.Add (pairDevs.Get (0));
          switchPorts[i][HOSTS_SWB].push_back (switchDevices.Get (1)->AddSwitchPort (pairDevs.Get (1)));
        }

      //Configuring CSMA links of the servers
      for (size_t j = 0; j < sliceNodes[i][SERVERS].GetN (); j++)
        {
          pairDevs = csmaHelperEndPoints.Install (sliceNodes[i][SERVERS].Get (j), switches.Get (2));
          serversDevices.Add (pairDevs.Get (0));
          switchPorts[i][SERVERS].push_back (switchDevices.Get (2)->AddSwitchPort (pairDevs.Get (1)));
        }

      std::vector<NetDeviceContainer> devices;

      devices.push_back (hostDevicesSWA);
      devices.push_back (hostDevicesSWB);
      devices.push_back (serversDevices);

      sliceNetDevices.push_back (devices);

    }


  //printPorts();



  //Install the TCP/IP stack into hosts nodes (All hosts of all slices)
  InternetStackHelper internet;

  for (size_t i = 0; i < numberSlices; i++)
    {

      internet.Install (sliceNodes[i][HOSTS_SWA]); //HostsSWA
      internet.Install (sliceNodes[i][HOSTS_SWB]); //HostsSWB
      internet.Install (sliceNodes[i][SERVERS]); //Servers

    }

  //Set IPv4 host addresses
  Ipv4AddressHelper ipv4helpr;


  std::string baseAddressStr = "0.x.y.1";
  Ipv4Address baseAddress;

  for (size_t i = 0; i < numberSlices; i++)
    {

      Ipv4InterfaceContainer hostIpIfacesSWA;
      Ipv4InterfaceContainer hostIpIfacesSWB;
      Ipv4InterfaceContainer serversIpIfaces;

      std::string baseAddressTmp = baseAddressStr;

      //Number to identify if the address is given to a host or a server
      int hostServer = 1;

      std::string slice_id = std::to_string (i);
      std::string hostServer_str = std::to_string (hostServer);

      boost::replace_all (baseAddressTmp, "x", slice_id);
      boost::replace_all (baseAddressTmp, "y", hostServer_str);

      baseAddress.Set (baseAddressTmp.c_str ());
      ipv4helpr.SetBase ("10.0.0.0", "255.0.0.0", baseAddress);
      hostIpIfacesSWA = ipv4helpr.Assign (sliceNetDevices[i][HOSTS_SWA]); //hostDevicesSWA
      hostIpIfacesSWB = ipv4helpr.Assign (sliceNetDevices[i][HOSTS_SWB]); //hostDevicesSWB


      baseAddressTmp = baseAddressStr;
      hostServer = hostServer + 1;
      hostServer_str = std::to_string (hostServer);
      boost::replace_all (baseAddressTmp, "x", slice_id);
      boost::replace_all (baseAddressTmp, "y", hostServer_str);


      baseAddress.Set (baseAddressTmp.c_str ());
      ipv4helpr.SetBase ("10.0.0.0", "255.0.0.0", baseAddress);
      serversIpIfaces = ipv4helpr.Assign (sliceNetDevices[i][SERVERS]); //serversDevices

      Ipv4InterfaceContainer allHostsIpIfaces;
      allHostsIpIfaces.Add (hostIpIfacesSWA);
      allHostsIpIfaces.Add (hostIpIfacesSWB);

      std::vector<Ipv4InterfaceContainer> interfaces;

      interfaces.push_back (hostIpIfacesSWA);
      interfaces.push_back (hostIpIfacesSWB);
      interfaces.push_back (serversIpIfaces);
      interfaces.push_back (allHostsIpIfaces);

      sliceInterfaces.push_back (interfaces);

    }

}

void
EnableVerbose (bool enable)
{
  if (enable)
    {
      LogLevel logLevelWarn = static_cast<ns3::LogLevel> (
          LOG_PREFIX_FUNC | LOG_PREFIX_TIME | LOG_LEVEL_WARN);
      NS_UNUSED (logLevelWarn);

      LogLevel logLevelWarnInfo = static_cast<ns3::LogLevel> (
          LOG_PREFIX_FUNC | LOG_PREFIX_TIME | LOG_LEVEL_WARN | LOG_INFO);
      NS_UNUSED (logLevelWarnInfo);

      LogLevel logLevelInfo = static_cast<ns3::LogLevel> (
          LOG_PREFIX_FUNC | LOG_PREFIX_TIME | LOG_LEVEL_INFO);
      NS_UNUSED (logLevelInfo);

      LogLevel logLevelAll = static_cast<ns3::LogLevel> (
          LOG_PREFIX_FUNC | LOG_PREFIX_TIME | LOG_LEVEL_ALL);
      NS_UNUSED (logLevelAll);

      // Common components.
      LogComponentEnable ("Main",                     logLevelAll);
      LogComponentEnable ("Common",                   logLevelAll);

      // Traffic and applications.
      LogComponentEnable ("UdpPeerApp",               logLevelWarn);
      LogComponentEnable ("UdpPeerHelper",            logLevelWarnInfo);

      // OFSwitch13 module components.
      LogComponentEnable ("OFSwitch13Controller",     logLevelWarn);
      LogComponentEnable ("OFSwitch13Device",         logLevelWarn);
      LogComponentEnable ("OFSwitch13Helper",         logLevelWarn);
      LogComponentEnable ("OFSwitch13Interface",      logLevelWarn);
      LogComponentEnable ("OFSwitch13Port",           logLevelWarn);
      LogComponentEnable ("OFSwitch13Queue",          logLevelWarn);
      LogComponentEnable ("OFSwitch13SocketHandler",  logLevelWarn);
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
