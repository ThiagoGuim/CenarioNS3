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
#include <fstream>
#include "infrastructure/controller.h"
#include "application/udp-peer-helper.h"
#include "metadata/slice-info.h"


using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("Main");

typedef std::vector<std::vector<NodeContainer> > TopologyNodes_t;
typedef std::vector<std::vector<NetDeviceContainer> > TopologyNetDevices_t;


//Print vectors with the switch ports information
void printPorts ();

//Set animation configurations
void createAnimation ();

//Nodes and CSMA links configuration
void configureSwitches ();
void configureSlices (std::string config);

//Auxiliar function to parse the string passed by the command line file and get the slice attributes
void parse (std::string v, ObjectFactory &factory);

//Enable the log functions
void EnableVerbose (bool enable);
void EnableOfsLogs (bool enable);


///Global variables

//NetAnim or not
bool createAnim = false;

//Variables storing the topology config
size_t numberSlices = 0;
size_t numberHostsSWA = 0;
size_t numberHostsSWB = 0;

//Vector to allocate the pointers to the slice objects
std::vector<Ptr<SliceInfo>> slices;

//Vectors to allocate the ports of the switches
TopologyPorts_t switchPorts;
PortsVector_t interSwitchesPorts;

//Containers that will store Node, NetDevice and Interface objects
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

bool trace;

//Prefixes used by output filenames.
static ns3::GlobalValue
  g_outputPrefix ("OutputPrefix", "Common prefix for input filenames.",
                  ns3::StringValue (std::string ()),
                  ns3::MakeStringChecker ());

// Flag for error messages at the stderr stream.
static ns3::GlobalValue
  g_seeLogs ("SeeCerr", "Tell user to check the stderr stream.",
             ns3::BooleanValue (false),
             ns3::MakeBooleanChecker ());

int
main (int argc, char *argv[])
{

  // Customizing the OpenFlow queue type on switch ports.
  Config::SetDefault ("ns3::OFSwitch13Port::QueueFactory", StringValue ("ns3::QosQueue"));

  Config::SetDefault ("ns3::CsmaChannel::FullDuplex", BooleanValue (true));

  std::string animFile = "animation.xml";
  int simTime = 20;
  bool verbose = false;
  trace = false;
  bool ofsLog   = false;

  //Name of the slice config file
  std::string config = "";


  // Configure command line parameters
  CommandLine cmd;
  cmd.AddValue ("SimTime", "Simulation time (seconds)", simTime);
  cmd.AddValue ("Verbose", "Enable verbose output", verbose);
  cmd.AddValue ("Trace", "Enable datapath stats and pcap traces", trace);
  cmd.AddValue ("createAnim", "Enable netAnim", createAnim);
  cmd.AddValue ("Config", "File containing the config info about the slices", config);
  cmd.AddValue ("OfsLog",   "Enable ofsoftswitch13 logs.", ofsLog);
  cmd.Parse (argc, argv);


  // Enable checksum computations (required by OFSwitch13 module)
  GlobalValue::Bind ("ChecksumEnabled", BooleanValue (true));

  //Enable OfsLogs or not
  EnableOfsLogs (ofsLog);
  EnableVerbose (verbose);

  //Do the proper configuration onto hosts, servers and switches.
  controllerNode = CreateObject<Node> ();
  if (config.size () != 0)
    {

      configureSwitches ();
      controllerApp->NotifySwitches (interSwitchesPorts, switchDevices);

      configureSlices(config);
      controllerApp->NotifyClientsServers (sliceInterfaces, switchPorts);

      controllerApp->ConfigureMeters (slices);
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

  //Configure the application
  for (size_t i = 0; i < numberSlices; i++)
    {
      Ptr<ExponentialRandomVariable> startRng = CreateObject<ExponentialRandomVariable> ();
      startRng->SetAttribute ("Mean", DoubleValue (5));

      // Configuring traffic patterns for this slice
      Ptr<UdpPeerHelper> appHelper = CreateObject<UdpPeerHelper> ();
      appHelper->SetAttribute ("NumApps", UintegerValue (20));
      appHelper->SetAttribute ("StartInterval", PointerValue (startRng));


      // // Define the packet size and interval for UDP traffic.
      // appHelper->SetBothAttribute ("PktInterval", StringValue ("ns3::NormalRandomVariable[Mean=0.01|Variance=0.001]"));
      // appHelper->SetBothAttribute ("PktSize", StringValue ("ns3::UniformRandomVariable[Min=1024|Max=1460]"));

      // // Disable the traffic at the UDP server node.
      // appHelper->Set2ndAttribute ("PktInterval", StringValue ("ns3::ConstantRandomVariable[Constant=1000000]"));

      appHelper->SetBothAttribute ("SliceId", UintegerValue (i));
      appHelper->Install (sliceNodes[i][ALL_HOSTS], sliceNodes[i][SERVERS],
                          sliceInterfaces[i][ALL_HOSTS], sliceInterfaces[i][SERVERS]);

    }


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

  // Run the simulation
  Simulator::Stop (Seconds (simTime));
  Simulator::Run ();
  Simulator::Destroy ();

}

void
parse (std::string v, ObjectFactory &factory)
 {

   std::string::size_type lbracket, rbracket;
   lbracket = v.find ("[");
   rbracket = v.find ("]");
   if (lbracket == std::string::npos && rbracket == std::string::npos)
     {
       factory.SetTypeId (v);
       return;
     }
   if (lbracket == std::string::npos || rbracket == std::string::npos)
     {
       return;
     }
   NS_ASSERT (lbracket != std::string::npos);
   NS_ASSERT (rbracket != std::string::npos);
   std::string tid = v.substr (0, lbracket);
   std::string parameters = v.substr (lbracket + 1,rbracket - (lbracket + 1));
   factory.SetTypeId (tid);
   std::string::size_type cur;
   cur = 0;
   while (cur != parameters.size ())
     {
       std::string::size_type equal = parameters.find ("=", cur);
       if (equal == std::string::npos)
         {
           break;
         }
       else
         {
           std::string name = parameters.substr (cur, equal - cur);
           struct TypeId::AttributeInformation info;
           if (!factory.GetTypeId().LookupAttributeByName (name, &info))
             {
               break;
             }
           else
             {
               std::string::size_type next = parameters.find ("|", cur);
               std::string value;
               if (next == std::string::npos)
                 {
                   value = parameters.substr (equal + 1, parameters.size () - (equal + 1));
                   cur = parameters.size ();
                 }
               else
                 {
                   value = parameters.substr (equal + 1, next - (equal + 1));
                   cur = next + 1;
                 }

              factory.Set(name, StringValue(value));

             }
         }
     }

   return;
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

// Comparator for slice priorities.
bool
PriComp (Ptr<SliceInfo> slice1, Ptr<SliceInfo> slice2)
{
  return slice1->GetPriority() < slice2->GetPriority();
}

void
configureSlices(std::string config){

  std::ifstream file;
  file.open(config);
  std::string buffer;

  NS_ASSERT_MSG (file.is_open(), "Error while opening the file");

  int maxQuota = 0;
  int currQuota;
  int currNumberHostsSWA;
  int currNumberHostsSWB;
  int currSlice;

  //Addressing info
  InternetStackHelper internet;
  Ipv4AddressHelper ipv4helpr;
  std::string baseAddressStr = "0.x.y.1";
  Ipv4Address baseAddress;

  while(!file.eof()){

    getline(file, buffer);

    ObjectFactory sliceFac;
    parse(buffer, sliceFac);
    sliceFac.Set("SliceId", UintegerValue(numberSlices));
    Ptr<SliceInfo> slice = sliceFac.Create<SliceInfo>();

    currSlice = slice->GetSliceId();
    currQuota = slice->GetQuota();
    currNumberHostsSWA = slice->GetHostsA();
    currNumberHostsSWB = slice->GetHostsB();
    numberHostsSWA += currNumberHostsSWA;
    numberHostsSWB += currNumberHostsSWB;
    maxQuota += currQuota;

    NS_ASSERT_MSG (currQuota >= 1, "Invalid Quota");
    NS_ASSERT_MSG (maxQuota <= 100, "Quota exceeded");
    NS_ASSERT_MSG (numberHostsSWA < 255, "Number of hostsSWA exceeded");
    NS_ASSERT_MSG (numberHostsSWB < 255, "Number of hostsSWB exceeded");
    NS_ASSERT_MSG (numberSlices < 255, "Number of slices exceeded");

    slices.push_back(slice);

    //Creates the NodeContainers and assign them to the respective slice
    NodeContainer hostsSWA;
    NodeContainer hostsSWB;
    NodeContainer servers;

    hostsSWA.Create (currNumberHostsSWA);
    hostsSWB.Create (currNumberHostsSWB);
    servers.Create (currNumberHostsSWA + currNumberHostsSWB);

    std::vector<NodeContainer> machines;

    machines.push_back (hostsSWA);
    machines.push_back (hostsSWB);
    machines.push_back (servers);


    NodeContainer allHostsPerSliceContainer (hostsSWA, hostsSWB);

    machines.push_back (allHostsPerSliceContainer);

    sliceNodes.push_back (machines);

    //Configure the CSMA links between hosts and switches for each devices
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
    for (size_t j = 0; j < sliceNodes[currSlice][HOSTS_SWA].GetN (); j++)
      {
        pairDevs = csmaHelperEndPoints.Install (sliceNodes[currSlice][HOSTS_SWA].Get (j), switches.Get (0));
        hostDevicesSWA.Add (pairDevs.Get (0));
        switchPorts[currSlice][HOSTS_SWA].push_back (switchDevices.Get (0)->AddSwitchPort (pairDevs.Get (1)));
      }

    //Configuring CSMA links of the hostsSWB
    for (size_t j = 0; j < sliceNodes[currSlice][HOSTS_SWB].GetN (); j++)
      {
        pairDevs = csmaHelperEndPoints.Install (sliceNodes[currSlice][HOSTS_SWB].Get (j), switches.Get (1));
        hostDevicesSWB.Add (pairDevs.Get (0));
        switchPorts[currSlice][HOSTS_SWB].push_back (switchDevices.Get (1)->AddSwitchPort (pairDevs.Get (1)));
      }

    //Configuring CSMA links of the servers
    for (size_t j = 0; j < sliceNodes[currSlice][SERVERS].GetN (); j++)
      {
        pairDevs = csmaHelperEndPoints.Install (sliceNodes[currSlice][SERVERS].Get (j), switches.Get (2));
        serversDevices.Add (pairDevs.Get (0));
        switchPorts[currSlice][SERVERS].push_back (switchDevices.Get (2)->AddSwitchPort (pairDevs.Get (1)));
      }

    std::vector<NetDeviceContainer> devices;

    devices.push_back (hostDevicesSWA);
    devices.push_back (hostDevicesSWB);
    devices.push_back (serversDevices);

    sliceNetDevices.push_back (devices);

    //Install the TCP/IP stack into hosts nodes (All hosts of all slices)
    internet.Install (sliceNodes[currSlice][HOSTS_SWA]); //HostsSWA
    internet.Install (sliceNodes[currSlice][HOSTS_SWB]); //HostsSWB
    internet.Install (sliceNodes[currSlice][SERVERS]); //Servers

    //Set IPv4 host addresses
    Ipv4InterfaceContainer hostIpIfacesSWA;
    Ipv4InterfaceContainer hostIpIfacesSWB;
    Ipv4InterfaceContainer serversIpIfaces;

    std::string baseAddressTmp = baseAddressStr;

    //Number to identify if the address is given to a host or a server
    int hostServer = 1;

    std::string slice_id = std::to_string (currSlice);
    std::string hostServer_str = std::to_string (hostServer);

    boost::replace_all (baseAddressTmp, "x", slice_id);
    boost::replace_all (baseAddressTmp, "y", hostServer_str);

    baseAddress.Set (baseAddressTmp.c_str ());
    ipv4helpr.SetBase ("10.0.0.0", "255.0.0.0", baseAddress);
    hostIpIfacesSWA = ipv4helpr.Assign (sliceNetDevices[currSlice][HOSTS_SWA]); //hostDevicesSWA
    hostIpIfacesSWB = ipv4helpr.Assign (sliceNetDevices[currSlice][HOSTS_SWB]); //hostDevicesSWB


    baseAddressTmp = baseAddressStr;
    hostServer = hostServer + 1;
    hostServer_str = std::to_string (hostServer);
    boost::replace_all (baseAddressTmp, "x", slice_id);
    boost::replace_all (baseAddressTmp, "y", hostServer_str);


    baseAddress.Set (baseAddressTmp.c_str ());
    ipv4helpr.SetBase ("10.0.0.0", "255.0.0.0", baseAddress);
    serversIpIfaces = ipv4helpr.Assign (sliceNetDevices[currSlice][SERVERS]); //serversDevices

    Ipv4InterfaceContainer allHostsIpIfaces;
    allHostsIpIfaces.Add (hostIpIfacesSWA);
    allHostsIpIfaces.Add (hostIpIfacesSWB);

    std::vector<Ipv4InterfaceContainer> interfaces;

    interfaces.push_back (hostIpIfacesSWA);
    interfaces.push_back (hostIpIfacesSWB);
    interfaces.push_back (serversIpIfaces);
    interfaces.push_back (allHostsIpIfaces);

    sliceInterfaces.push_back (interfaces);

    numberSlices++;
  }

  std::stable_sort (slices.begin (), slices.end (), PriComp);

  for(std::vector<Ptr<SliceInfo>>::iterator it = slices.begin(); it != slices.end(); ++it){
    std::cout << "SliceId = " << (*it)->GetSliceId() << "| ";
    std::cout << "Prio = " << (*it)->GetPriority() << "| ";
    std::cout << "Quota = " << (*it)->GetQuota() << "| ";
    std::cout << "HostsSWA = " << (*it)->GetHostsA() << "| ";
    std::cout << "HostsSWB = " << (*it)->GetHostsB() << std::endl;
  }

  file.close();

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


  csmaHelperEndPoints.SetChannelAttribute ("DataRate", DataRateValue (DataRate ("100Gbps")));
  csmaHelperEndPoints.SetChannelAttribute ("Delay", TimeValue (MilliSeconds (0)));

  pairDevs = csmaHelperinterSwitchesPorts.Install (switches.Get (0), switches.Get (1));
  interSwitchesPorts.push_back (switchDevices.Get (0)->AddSwitchPort (pairDevs.Get (0)));
  interSwitchesPorts.push_back (switchDevices.Get (1)->AddSwitchPort (pairDevs.Get (1)));

  //Tracing
  if(trace){

    csmaHelperinterSwitchesPorts.EnablePcap ("SWA", pairDevs.Get (0), true);
    csmaHelperinterSwitchesPorts.EnablePcap ("SWB-LEFT", pairDevs.Get (1), true);

  }

  //Creating the LinkInfo object between SWA - SWB
  Ptr<CsmaChannel> channelSwaSwb = DynamicCast<CsmaChannel> (pairDevs.Get (0)->GetChannel ());
  CreateObject<LinkInfo> (interSwitchesPorts[0], interSwitchesPorts[1], channelSwaSwb);


  pairDevs = csmaHelperinterSwitchesPorts.Install (switches.Get (1), switches.Get (2));
  interSwitchesPorts.push_back (switchDevices.Get (1)->AddSwitchPort (pairDevs.Get (0)));
  interSwitchesPorts.push_back (switchDevices.Get (2)->AddSwitchPort (pairDevs.Get (1)));

  //Creating the LinkInfo object between SWB - ServerSwitch
  Ptr<CsmaChannel> channelSwbServerSw = DynamicCast<CsmaChannel> (pairDevs.Get (0)->GetChannel ());
  CreateObject<LinkInfo> (interSwitchesPorts[2], interSwitchesPorts[3], channelSwbServerSw);

  //Inicialize controller
  controllerApp = CreateObject<Controller>();
  of13Helper->InstallController (controllerNode, controllerApp);
  of13Helper->CreateOpenFlowChannels ();

  //Tracing
  if(trace){

    csmaHelperinterSwitchesPorts.EnablePcap ("SWB-RIGHT", pairDevs.Get (0), true);
    csmaHelperinterSwitchesPorts.EnablePcap ("SERVERSW", pairDevs.Get (1), true);

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
