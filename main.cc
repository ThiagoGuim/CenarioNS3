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
#include <ns3/config-store-module.h>
#include <ns3/core-module.h>
#include <ns3/network-module.h>
#include <ns3/csma-module.h>
#include <ns3/internet-module.h>
#include <ns3/ofswitch13-module.h>
#include <ns3/applications-module.h>
#include <ns3/mobility-module.h>

#include <boost/algorithm/string/replace.hpp>
#include <iomanip>
#include <iostream>
#include <string>
#include <fstream>

#include "infrastructure/controller.h"
#include "application/udp-peer-helper.h"
#include "metadata/slice-info.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("Main");

// Prefixes used by input and output filenames.
static ns3::GlobalValue
  g_inputPrefix ("InputPrefix", "Common prefix for output filenames.",
                 ns3::StringValue (std::string ()),
                 ns3::MakeStringChecker ());

static ns3::GlobalValue
  g_outputPrefix ("OutputPrefix", "Common prefix for input filenames.",
                  ns3::StringValue (std::string ()),
                  ns3::MakeStringChecker ());

// Dump timeout for logging statistics.
static ns3::GlobalValue
  g_dumpTimeout ("DumpStatsTimeout", "Periodic statistics dump interval.",
                 ns3::TimeValue (Seconds (1)),
                 ns3::MakeTimeChecker ());

// Simulation lenght.
static ns3::GlobalValue
  g_simTime ("SimTime", "Simulation stop time.",
             ns3::TimeValue (Seconds (0)),
             ns3::MakeTimeChecker ());

// Flag for error messages at the stderr stream.
static ns3::GlobalValue
  g_seeLogs ("SeeCerr", "Tell user to check the stderr stream.",
             ns3::BooleanValue (false),
             ns3::MakeBooleanChecker ());

void ForceDefaults      (void);
void EnableProgress     (int);
void EnableVerbose      (bool);
void EnableOfsLogs      (bool);
void CreateAnimation    (bool);
void SliceFactoryParse  (std::string, ObjectFactory&);

typedef std::vector<std::vector<NodeContainer>> TopologyNodes_t;
typedef std::vector<std::vector<NetDeviceContainer>> TopologyNetDevices_t;

//Set animation configurations

//Nodes and CSMA links configuration

void configureSlices (std::string config);


//Variables storing the topology config

size_t numberHostsSWA = 0;
size_t numberHostsSWB = 0;

//Vector to allocate the pointers to the slice objects
SliceInfoList_t slices;

//Vectors to allocate the ports of the switches
TopologyPorts_t switchPorts;


//Containers that will store Node, NetDevice and Interface objects
TopologyNodes_t sliceNodes;
TopologyNetDevices_t sliceNetDevices;
TopologyIfaces_t sliceInterfaces;



int
main (int argc, char *argv[])
{
  bool        ofsLog    = false;
  bool        verbose   = false;
  bool        trace     = false;
  int         progress  = 1;
  std::string prefix    = std::string ();

  // Configure command line parameters
  CommandLine cmd;
  cmd.AddValue ("Prefix",   "Common prefix for filenames.", prefix);
  cmd.AddValue ("Progress", "Simulation progress interval (sec).", progress);
  cmd.AddValue ("Verbose",  "Enable verbose output.", verbose);
  cmd.AddValue ("OfsLog",   "Enable ofsoftswitch13 logs.", ofsLog);
  cmd.AddValue ("Trace",    "Enable pcap traces", trace);
  cmd.Parse (argc, argv);

  // Update input and output prefixes from command line prefix parameter.
  // This way, all files from this simulation will have the same prefix.
  NS_ASSERT_MSG (!prefix.empty (), "Unknown prefix.");
  std::ostringstream inputPrefix, outputPrefix;
  inputPrefix << prefix;
  char lastChar = *prefix.rbegin ();
  if (lastChar != '-')
    {
      inputPrefix << "-";
    }
  outputPrefix << inputPrefix.str () << RngSeedManager::GetRun () << "-";
  Config::SetGlobal ("InputPrefix", StringValue (inputPrefix.str ()));
  Config::SetGlobal ("OutputPrefix", StringValue (outputPrefix.str ()));

  // Enable verbose output for debug purposes.
  EnableVerbose (verbose);
  EnableOfsLogs (ofsLog);

  // Read the topology configuration file. This file is optional.
  // With this file, it is possible to override default attribute values.
  std::string topoFilename = prefix + ".topo";
  std::ifstream testFile (topoFilename.c_str (), std::ifstream::in);
  if (testFile.good ())
    {
      testFile.close ();

      NS_LOG_INFO ("Reading attributes file: " << topoFilename);
      Config::SetDefault ("ns3::ConfigStore::Mode", StringValue ("Load"));
      Config::SetDefault ("ns3::ConfigStore::FileFormat", StringValue ("RawText"));
      Config::SetDefault ("ns3::ConfigStore::Filename", StringValue (topoFilename));
      ConfigStore inputConfig;
      inputConfig.ConfigureDefaults ();
    }

  // Parse command line again so users can override values from configuration
  // file, and force some default attribute values that cannot be overridden.
  cmd.Parse (argc, argv);
  ForceDefaults ();

  NS_LOG_INFO ("Creating the simulation scenario...");


  // --------------------------------------------------------------------------
  // Configuring the OpenFlow network with three switches in line.

  Ptr<OFSwitch13InternalHelper> of13Helper = CreateObject<OFSwitch13InternalHelper>();
  of13Helper->SetDeviceAttribute ("PipelineTables", UintegerValue (OFS_TAB_TOTAL));

  // Create the controller node and configure it.
  Ptr<Node> controllerNode = CreateObject<Node> ();
  Ptr<Controller> controllerApp = CreateObject<Controller>();;
  of13Helper->InstallController (controllerNode, controllerApp);

  // Create the switch nodes and configure them.
  NodeContainer switchNodes;
  switchNodes.Create (3);
  Names::Add ("A", switchNodes.Get (0));
  Names::Add ("B", switchNodes.Get (1));
  Names::Add ("C", switchNodes.Get (2));

  OFSwitch13DeviceContainer switchDevices;
  switchDevices = of13Helper->InstallSwitch (switchNodes);

  CsmaHelper csmaHelper;
  csmaHelper.SetChannelAttribute ("DataRate", DataRateValue (DataRate ("100Mbps")));
  csmaHelper.SetChannelAttribute ("Delay", TimeValue (MilliSeconds (0)));

  NetDeviceContainer pairDevs;
  PortsList_t switchPorts;

  // Connect switch A to switch B
  pairDevs = csmaHelper.Install (switchNodes.Get (0), switchNodes.Get (1));
  switchPorts.push_back (switchDevices.Get (0)->AddSwitchPort (pairDevs.Get (0)));
  switchPorts.push_back (switchDevices.Get (1)->AddSwitchPort (pairDevs.Get (1)));
  CreateObject<LinkInfo> (switchPorts[0], switchPorts[1],
                          DynamicCast<CsmaChannel> (pairDevs.Get (0)->GetChannel ()));

  // Connect switch B to switch C
  pairDevs = csmaHelper.Install (switchNodes.Get (1), switchNodes.Get (2));
  switchPorts.push_back (switchDevices.Get (1)->AddSwitchPort (pairDevs.Get (0)));
  switchPorts.push_back (switchDevices.Get (2)->AddSwitchPort (pairDevs.Get (1)));
  CreateObject<LinkInfo> (switchPorts[2], switchPorts[3],
                          DynamicCast<CsmaChannel> (pairDevs.Get (0)->GetChannel ()));

  // Create the OpenFlow channel.
  of13Helper->CreateOpenFlowChannels ();

  // PCAP tracing
  if (trace)
    {
      std::string pcapPrefix = outputPrefix.str ();
      csmaHelper.EnablePcap (pcapPrefix + "sw", switchNodes, true);
      of13Helper->EnableOpenFlowPcap (pcapPrefix + "crtl", true);
    }

  // Notify the controler about the OpenFlow switches.
  controllerApp->NotifySwitches (switchPorts, switchDevices);


  // --------------------------------------------------------------------------
  // Configure the network slices

  // Read the slice configuration file. This file is mandatory.
  std::string slcFilename = prefix + ".slices";
  std::ifstream slcFile (slcFilename.c_str (), std::ifstream::in);
  NS_ASSERT_MSG (slcFile.good (), "Invalid slice config file " << slcFilename);

  int numberSlices = 0;
  int sumQuota = 0;
  std::string lineBuffer;
  while (!slcFile.eof ())
    {
      // Read the next non-empty line.
      getline (slcFile, lineBuffer);
      if (lineBuffer.empty ())
        {
          continue;
        }

      ObjectFactory sliceFac;
      SliceFactoryParse (lineBuffer, sliceFac);
      sliceFac.Set ("SliceId", UintegerValue (numberSlices + 1));
      Ptr<SliceInfo> slice = sliceFac.Create<SliceInfo>();

      sumQuota += slice->GetQuota ();
      NS_ASSERT_MSG (sumQuota <= 100, "Quota exceeded");

      numberSlices++;
    }
  slcFile.close ();



  csmaHelper.SetChannelAttribute ("DataRate", DataRateValue (DataRate ("100Gbps")));
  csmaHelper.SetChannelAttribute ("Delay", TimeValue (MilliSeconds (0)));





  // Notify the controler about the network slices. FIXME
  // controllerApp->NotifyClientsServers (sliceInterfaces, switchPorts);
  // controllerApp->ConfigureMeters (slices);




  // //Configure the application
  // for (size_t i = 0; i < numberSlices; i++)
  //   {
  //     Ptr<ExponentialRandomVariable> startRng = CreateObject<ExponentialRandomVariable> ();
  //     startRng->SetAttribute ("Mean", DoubleValue (5));

  //     // Configuring traffic patterns for this slice
  //     Ptr<UdpPeerHelper> appHelper = CreateObject<UdpPeerHelper> ();
  //     appHelper->SetAttribute ("NumApps", UintegerValue (20));
  //     appHelper->SetAttribute ("StartInterval", PointerValue (startRng));


  //     // // Define the packet size and interval for UDP traffic.
  //     // appHelper->SetBothAttribute ("PktInterval", StringValue ("ns3::NormalRandomVariable[Mean=0.01|Variance=0.001]"));
  //     // appHelper->SetBothAttribute ("PktSize", StringValue ("ns3::UniformRandomVariable[Min=1024|Max=1460]"));

  //     // // Disable the traffic at the UDP server node.
  //     // appHelper->Set2ndAttribute ("PktInterval", StringValue ("ns3::ConstantRandomVariable[Constant=1000000]"));

  //     appHelper->SetBothAttribute ("SliceId", UintegerValue (i));
  //     appHelper->Install (sliceNodes[i][ALL_HOSTS], sliceNodes[i][SERVERS],
  //                         sliceInterfaces[i][ALL_HOSTS], sliceInterfaces[i][SERVERS]);

  //   }

  // Set stop time and run the simulation.
  std::cout << "Simulating..." << std::endl;
  EnableProgress (progress);

  TimeValue timeValue;
  GlobalValue::GetValueByName ("SimTime", timeValue);
  Time stopAt = timeValue.Get () + MilliSeconds (100);

  Simulator::Stop (stopAt);
  Simulator::Run ();

  // Finish the simulation.
  Simulator::Destroy ();

  // Print the final status message.
  BooleanValue cerrValue;
  GlobalValue::GetValueByName ("SeeCerr", cerrValue);
  std::cout << "END OK";
  if (cerrValue.Get ())
    {
      std::cout << " - WITH ERRORS";
    }
  std::cout << std::endl;
  return 0;
}

void
SliceFactoryParse (std::string str, ObjectFactory &factory)
{
  std::string::size_type lbracket, rbracket;
  lbracket = str.find ("[");
  rbracket = str.find ("]");
  if (lbracket == std::string::npos && rbracket == std::string::npos)
    {
      factory.SetTypeId (str);
      return;
    }
  if (lbracket == std::string::npos || rbracket == std::string::npos)
    {
      return;
    }
  NS_ASSERT (lbracket != std::string::npos);
  NS_ASSERT (rbracket != std::string::npos);
  std::string tid = str.substr (0, lbracket);
  std::string parameters = str.substr (lbracket + 1,rbracket - (lbracket + 1));
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
          if (!factory.GetTypeId ().LookupAttributeByName (name, &info))
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
              factory.Set (name, StringValue (value));
            }
        }
    }
}

void
CreateAnimation (bool animation)
{
  if (animation)
    {
      // // Create the animation object and configure for specified output
      // std::string animFile = "animation.xml";

      // Ptr<ListPositionAllocator> listPosAllocator;
      // listPosAllocator = CreateObject<ListPositionAllocator> ();

      // listPosAllocator->Add (Vector (  75, 25, 0)); // Switch A
      // listPosAllocator->Add (Vector ( 100, 25, 0)); // Switch B
      // listPosAllocator->Add (Vector ( 125, 25, 0)); // Switch C
      // listPosAllocator->Add (Vector ( 100, 50, 0)); // Controller

      // NodeContainer allHosts;
      // NodeContainer allServers;

      // int start = 0;
      // for (size_t i = 0; i < numberSlices; i ++)
      // {

      //   size_t j = 0;

      //   for (j = 0; j < sliceNodes[i][ALL_HOSTS].GetN (); j ++)  //Clients-hosts
      //   {
      //     listPosAllocator->Add (Vector (0, (start + 25 * j), 0));
      //   }

      //   for (j = 0; j < sliceNodes[i][SERVERS].GetN (); j++)   //Servers
      //     {
      //       listPosAllocator->Add (Vector (200, (start + 25 * j), 0));
      //     }

      //   start = start + (25 * j);

      //   allHosts.Add (sliceNodes[i][ALL_HOSTS]);
      //   allServers.Add (sliceNodes[i][SERVERS]);
      // }



      // MobilityHelper mobilityHelper;
      // mobilityHelper.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
      // mobilityHelper.SetPositionAllocator (listPosAllocator);
      // mobilityHelper.Install (NodeContainer (switches, controllerNode, allHosts, allServers));

    }
}

// Comparator for slice priorities.
bool
PriComp (Ptr<SliceInfo> slice1, Ptr<SliceInfo> slice2)
{
  return slice1->GetPriority () < slice2->GetPriority ();
}

void
configureSlices (std::string config)
{



  // int maxQuota = 0;
  // int currQuota;
  // int currNumberHostsSWA;
  // int currNumberHostsSWB;
  // int currSlice;

  // //Addressing info
  // InternetStackHelper internet;
  // Ipv4AddressHelper ipv4helpr;
  // std::string baseAddressStr = "0.x.y.1";
  // Ipv4Address baseAddress;

  // while (!file.eof ())
  //   {

  //     getline (file, buffer);


  //     ObjectFactory sliceFac;
  //     parse (buffer, sliceFac);
  //     sliceFac.Set ("SliceId", UintegerValue (numberSlices + 1));
  //     Ptr<SliceInfo> slice = sliceFac.Create<SliceInfo>();

  //     currSlice = (slice->GetSliceId () - 1);
  //     currQuota = slice->GetQuota ();
  //     currNumberHostsSWA = slice->GetHostsA ();
  //     currNumberHostsSWB = slice->GetHostsB ();
  //     numberHostsSWA += currNumberHostsSWA;
  //     numberHostsSWB += currNumberHostsSWB;
  //     maxQuota += currQuota;

  //     NS_ASSERT_MSG (maxQuota <= 100, "Quota exceeded");

  //     slices.push_back (slice);

  //     //Creates the NodeContainers and assign them to the respective slice
  //     NodeContainer hostsSWA;
  //     NodeContainer hostsSWB;
  //     NodeContainer servers;

  //     hostsSWA.Create (currNumberHostsSWA);
  //     hostsSWB.Create (currNumberHostsSWB);
  //     servers.Create (currNumberHostsSWA + currNumberHostsSWB);

  //     std::vector<NodeContainer> machines;

  //     machines.push_back (hostsSWA);
  //     machines.push_back (hostsSWB);
  //     machines.push_back (servers);


  //     NodeContainer allHostsPerSliceContainer (hostsSWA, hostsSWB);

  //     machines.push_back (allHostsPerSliceContainer);

  //     sliceNodes.push_back (machines);

  //     //Configure the CSMA links between hosts and switches for each devices
  //     std::vector<Ptr<OFSwitch13Port>> hostsSWAports;
  //     std::vector<Ptr<OFSwitch13Port>> hostsSWBports;
  //     std::vector<Ptr<OFSwitch13Port>> serverPorts;

  //     std::vector<std::vector<Ptr<OFSwitch13Port>> > ports;

  //     ports.push_back (hostsSWAports);
  //     ports.push_back (hostsSWBports);
  //     ports.push_back (serverPorts);

  //     switchPorts.push_back (ports);

  //     NetDeviceContainer hostDevicesSWA;
  //     NetDeviceContainer hostDevicesSWB;
  //     NetDeviceContainer serversDevices;

  //     //Configuring CSMA links of the hostsSWA
  //     for (size_t j = 0; j < sliceNodes[currSlice][HOSTS_SWA].GetN (); j++)
  //       {
  //         pairDevs = csmaHelperEndPoints.Install (sliceNodes[currSlice][HOSTS_SWA].Get (j), switches.Get (0));
  //         hostDevicesSWA.Add (pairDevs.Get (0));
  //         switchPorts[currSlice][HOSTS_SWA].push_back (switchDevices.Get (0)->AddSwitchPort (pairDevs.Get (1)));
  //       }

  //     //Configuring CSMA links of the hostsSWB
  //     for (size_t j = 0; j < sliceNodes[currSlice][HOSTS_SWB].GetN (); j++)
  //       {
  //         pairDevs = csmaHelperEndPoints.Install (sliceNodes[currSlice][HOSTS_SWB].Get (j), switches.Get (1));
  //         hostDevicesSWB.Add (pairDevs.Get (0));
  //         switchPorts[currSlice][HOSTS_SWB].push_back (switchDevices.Get (1)->AddSwitchPort (pairDevs.Get (1)));
  //       }

  //     //Configuring CSMA links of the servers
  //     for (size_t j = 0; j < sliceNodes[currSlice][SERVERS].GetN (); j++)
  //       {
  //         pairDevs = csmaHelperEndPoints.Install (sliceNodes[currSlice][SERVERS].Get (j), switches.Get (2));
  //         serversDevices.Add (pairDevs.Get (0));
  //         switchPorts[currSlice][SERVERS].push_back (switchDevices.Get (2)->AddSwitchPort (pairDevs.Get (1)));
  //       }

  //     std::vector<NetDeviceContainer> devices;

  //     devices.push_back (hostDevicesSWA);
  //     devices.push_back (hostDevicesSWB);
  //     devices.push_back (serversDevices);

  //     sliceNetDevices.push_back (devices);

  //     //Install the TCP/IP stack into hosts nodes (All hosts of all slices)
  //     internet.Install (sliceNodes[currSlice][HOSTS_SWA]); //HostsSWA
  //     internet.Install (sliceNodes[currSlice][HOSTS_SWB]); //HostsSWB
  //     internet.Install (sliceNodes[currSlice][SERVERS]); //Servers

  //     //Set IPv4 host addresses
  //     Ipv4InterfaceContainer hostIpIfacesSWA;
  //     Ipv4InterfaceContainer hostIpIfacesSWB;
  //     Ipv4InterfaceContainer serversIpIfaces;

  //     std::string baseAddressTmp = baseAddressStr;

  //     //Number to identify if the address is given to a host or a server
  //     int hostServer = 1;

  //     std::string slice_id = std::to_string (currSlice);
  //     std::string hostServer_str = std::to_string (hostServer);

  //     boost::replace_all (baseAddressTmp, "x", slice_id);
  //     boost::replace_all (baseAddressTmp, "y", hostServer_str);

  //     baseAddress.Set (baseAddressTmp.c_str ());
  //     ipv4helpr.SetBase ("10.0.0.0", "255.0.0.0", baseAddress);
  //     hostIpIfacesSWA = ipv4helpr.Assign (sliceNetDevices[currSlice][HOSTS_SWA]); //hostDevicesSWA
  //     hostIpIfacesSWB = ipv4helpr.Assign (sliceNetDevices[currSlice][HOSTS_SWB]); //hostDevicesSWB


  //     baseAddressTmp = baseAddressStr;
  //     hostServer = hostServer + 1;
  //     hostServer_str = std::to_string (hostServer);
  //     boost::replace_all (baseAddressTmp, "x", slice_id);
  //     boost::replace_all (baseAddressTmp, "y", hostServer_str);


  //     baseAddress.Set (baseAddressTmp.c_str ());
  //     ipv4helpr.SetBase ("10.0.0.0", "255.0.0.0", baseAddress);
  //     serversIpIfaces = ipv4helpr.Assign (sliceNetDevices[currSlice][SERVERS]); //serversDevices

  //     Ipv4InterfaceContainer allHostsIpIfaces;
  //     allHostsIpIfaces.Add (hostIpIfacesSWA);
  //     allHostsIpIfaces.Add (hostIpIfacesSWB);

  //     std::vector<Ipv4InterfaceContainer> interfaces;

  //     interfaces.push_back (hostIpIfacesSWA);
  //     interfaces.push_back (hostIpIfacesSWB);
  //     interfaces.push_back (serversIpIfaces);
  //     interfaces.push_back (allHostsIpIfaces);

  //     sliceInterfaces.push_back (interfaces);

  //     numberSlices++;
  //   }

  // std::stable_sort (slices.begin (), slices.end (), PriComp);

  // for (std::vector<Ptr<SliceInfo>>::iterator it = slices.begin (); it != slices.end (); ++it)
  //   {
  //     std::cout << "SliceId = " << (*it)->GetSliceId () << "| ";
  //     std::cout << "Prio = " << (*it)->GetPriority () << "| ";
  //     std::cout << "Quota = " << (*it)->GetQuota () << "| ";
  //     std::cout << "HostsSWA = " << (*it)->GetHostsA () << "| ";
  //     std::cout << "HostsSWB = " << (*it)->GetHostsB () << std::endl;
  //   }

  // file.close ();
}

void
ForceDefaults ()
{
  //
  // Since we are using an external OpenFlow library that expects complete
  // network packets, we must enable checksum computations.
  //
  Config::SetGlobal (
    "ChecksumEnabled", BooleanValue (true));

  //
  // Whenever possible, use the full-duplex CSMA channel to improve throughput.
  // The code will automatically fall back to half-duplex mode for more than
  // two devices in the same channel.
  // This implementation is not available in default ns-3 code, and I got it
  // from https://codereview.appspot.com/187880044/
  //
  Config::SetDefault (
    "ns3::CsmaChannel::FullDuplex", BooleanValue (true));

  //
  // Customizing the OpenFlow queue type on switch ports.
  //
  Config::SetDefault (
    "ns3::OFSwitch13Port::QueueFactory", StringValue ("ns3::QosQueue"));

  //
  // Reducing the OpenFlow datapath timeout interval from 100ms to 50ms to
  // get a more precise token refill operation at meter entries. This change
  // requires reducing the EWMA alpha attribute at OpenFlow stats calculator
  // to continue getting consistent average values for 1s interval.
  //
  Config::SetDefault (
    "ns3::OFSwitch13Device::TimeoutInterval", TimeValue (MilliSeconds (50)));
  Config::SetDefault (
    "ns3::OFSwitch13StatsCalculator::EwmaAlpha", DoubleValue (0.1));

  //
  // Enable detailed OpenFlow datapath statistics.
  //
  Config::SetDefault (
    "ns3::OFSwitch13StatsCalculator::FlowTableDetails", BooleanValue (true));
}

void
EnableProgress (int interval)
{
  if (interval)
    {
      int64_t now = Simulator::Now ().ToInteger (Time::S);
      std::cout << "Current simulation time: +" << now << ".0s" << std::endl;
      Simulator::Schedule (Seconds (interval), &EnableProgress, interval);
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

      // Applications
      LogComponentEnable ("UdpPeerApp",               logLevelWarn);
      LogComponentEnable ("UdpPeerHelper",            logLevelWarn);

      // Infrastructure
      LogComponentEnable ("Controller",               logLevelWarn);
      LogComponentEnable ("QosQueue",                 logLevelWarn);

      // Metadata
      LogComponentEnable ("LinkInfo",                 logLevelWarn);
      LogComponentEnable ("SliceInfo",                logLevelWarn);
      LogComponentEnable ("SliceTag",                 logLevelWarn);

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
