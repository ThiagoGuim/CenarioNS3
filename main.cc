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
#include <ns3/internet-apps-module.h>
#include <ns3/flow-monitor-helper.h>


#include "infrastructure/controller.h"
#include "application/udp-peer-helper.h"
#include "metadata/slice-info.h"
#include "statistics/network-statistics.h"

#include "infrastructure/scenario1.h"
#include "infrastructure/scenario2.h"
#include "infrastructure/scenario3.h"
#include "infrastructure/scenario4.h"

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

static ns3::GlobalValue
  g_scenarioQueues ("ScenarioQueues", "Which scenario to use",
             ns3::StringValue (std::string ()),
             ns3::MakeStringChecker ());

void ForceDefaults      (void);
void EnableProgress     (int);
void EnableVerbose      (bool);
void EnableOfsLogs      (bool);

static std::string scenarioQueues;

/*****************************************************************************/
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


  // --------------------------------------------------------------------------
  // Configuring the OpenFlow network with three switches in line.
  // --------------------------------------------------------------------------
  NS_LOG_INFO ("Creating the OpenFlow network...");
  Ptr<OFSwitch13InternalHelper> of13Helper = CreateObject<OFSwitch13InternalHelper>();
  of13Helper->SetDeviceAttribute ("PipelineTables", UintegerValue (OFS_TAB_TOTAL));

  // Create the controller node and configure it.
  Ptr<Node> controllerNode = CreateObject<Node> ();
  Ptr<Controller> controllerApp = CreateObject<Controller>();
  of13Helper->InstallController (controllerNode, controllerApp);

  //Set scenario queues
  // switch(controllerApp->GetScenarioConfig()){

  //   case 1:
  //     scenarioQueues = "ns3::Scenario1queues";
  //     break;
  //   case 2:
  //     scenarioQueues = "ns3::Scenario2queues";
  //     break;
  //   case 3:
  //     scenarioQueues = "ns3::Scenario3queues";
  //     break;
  //   case 4:
  //     scenarioQueues = "ns3::Scenario4queues";
  //     break;

  // }

  // Create the switch nodes and configure them.
  NodeContainer switchNodes;
  switchNodes.Create (2);
  Names::Add ("A", switchNodes.Get (0));
  Names::Add ("C", switchNodes.Get (1));

  OFSwitch13DeviceContainer switchDevices;
  switchDevices = of13Helper->InstallSwitch (switchNodes);
  PortsList_t switchPorts;

  CsmaHelper csmaHelper;
  csmaHelper.SetChannelAttribute ("DataRate", DataRateValue (DataRate ("160Mbps")));
  csmaHelper.SetChannelAttribute ("Delay", TimeValue (MilliSeconds (0)));

  // Connect switch A to switch C
  NetDeviceContainer pairDevs;
  pairDevs = csmaHelper.Install (switchNodes.Get (0), switchNodes.Get (1));
  switchPorts.push_back (switchDevices.Get (0)->AddSwitchPort (pairDevs.Get (0)));
  switchPorts.push_back (switchDevices.Get (1)->AddSwitchPort (pairDevs.Get (1)));
  CreateObject<LinkInfo> (switchPorts.at (0), switchPorts.at (1),
                          DynamicCast<CsmaChannel> (pairDevs.Get (0)->GetChannel ()));


  // Create the OpenFlow channel.
  of13Helper->CreateOpenFlowChannels ();

  // Enable OpenFlow switch statistics.
  of13Helper->EnableDatapathStats (outputPrefix.str () + "switch-stats", true);

  // PCAP tracing on the OpenFlow network only.
  if (trace)
    {
      std::string pcapPrefix = outputPrefix.str ();
      csmaHelper.EnablePcap (pcapPrefix + "sw", switchNodes, true);
      of13Helper->EnableOpenFlowPcap (pcapPrefix + "crtl", true);
    }

  // Notify the controler about the OpenFlow switches.
  controllerApp->NotifySwitches (switchDevices, switchPorts);

  // --------------------------------------------------------------------------
  // Configuring the network slices
  // --------------------------------------------------------------------------
  NS_LOG_INFO ("Creating the network slices...");
  int sliceCounter = 0;
  int sumQuota = 0;

  // Read the slice configuration file. This file is mandatory.
  std::string slcFilename = prefix + ".slices";
  std::ifstream slcFile (slcFilename.c_str (), std::ifstream::in);
  NS_ASSERT_MSG (slcFile.good (), "Invalid slice config file " << slcFilename);

  // Reconfigure the helper with higher data rate for host connections.
  csmaHelper.SetChannelAttribute ("DataRate", DataRateValue (DataRate ("100Gbps")));
  csmaHelper.SetChannelAttribute ("Delay", TimeValue (MilliSeconds (0)));

  NodeContainer hosts;
  NodeContainer servers;
  Ipv4InterfaceContainer serverIpIFaces;
  Ipv4InterfaceContainer hostsIpIFaces;

  while (!slcFile.eof ())
    {
      // Read the next non-empty line from file.
      std::string lineBuffer;
      getline (slcFile, lineBuffer);
      if (lineBuffer.empty ())
        {
          continue;
        }

      // Create the slice info object from pre-configured factory.
      std::istringstream iss (lineBuffer);
      ObjectFactory sliceFactory;
      iss >> sliceFactory;
      sliceFactory.Set ("SliceId", UintegerValue (++sliceCounter));
      Ptr<SliceInfo> slice = sliceFactory.Create<SliceInfo> ();

      std::cout << "SHA " << slice->GetSharing  () << std::endl;
      std::cout << "SLICEID " << slice->GetSliceId  () << std::endl;
      std::cout << "PRIO " << slice->GetPriority () << std::endl;
      std::cout << "QUOTA " << slice->GetQuota    () << std::endl;
      std::cout << "NUMHOSTA " << slice->GetNumHostsA() << std::endl;
      std::cout << "NUMHOSTB " << slice->GetNumHostsB() << std::endl;
      std::cout << "NUMHOSTC " << slice->GetNumHostsC() << std::endl;

      sumQuota += slice->GetQuota ();
      NS_ASSERT_MSG (sumQuota <= 100, "Quota exceeded");

      // Create the containers for all nodes, ports, devices, and IP addr.
      NodeContainer hostsA, hostsC;
      PortsList_t switchPortsA, switchPortsC;
      NetDeviceContainer hostDevicesA, hostDevicesC;
      Ipv4InterfaceContainer hostIpIfacesA, hostIpIfacesC;


      // Create the host nodes.
      hostsA.Create (slice->GetNumHostsA ());
      hostsC.Create (slice->GetNumHostsC ());

      hosts.Add(hostsA);
      servers.Add(hostsC);

      // Connect hosts to switches, saving ports and devices.
      for (size_t i = 0; i < hostsA.GetN (); i++)
        {
          pairDevs = csmaHelper.Install (hostsA.Get (i), switchNodes.Get (0));
          hostDevicesA.Add (pairDevs.Get (0));
          switchPortsA.push_back (switchDevices.Get (0)->AddSwitchPort (pairDevs.Get (1)));
        }
      
      for (size_t i = 0; i < hostsC.GetN (); i++)
        {
          pairDevs = csmaHelper.Install (hostsC.Get (i), switchNodes.Get (1));
          hostDevicesC.Add (pairDevs.Get (0));
          switchPortsC.push_back (switchDevices.Get (1)->AddSwitchPort (pairDevs.Get (1)));
        }

      // Install the TCP/IP stack into hosts nodes
      InternetStackHelper internet;
      internet.Install (hostsA);
      internet.Install (hostsC);

      // Configure the base addresses
      std::string sliceId = std::to_string (slice->GetSliceId ());
      std::string hostsAddrA = "0." + sliceId + ".1.1";
      std::string hostsAddrC  = "0." + sliceId + ".2.1";
      Ipv4Address baseAddressA (hostsAddrA.c_str ());
      Ipv4Address baseAddressC (hostsAddrC.c_str ());

      // Set IPv4 host addresses.
      // Hosts on switches A with IP address 10.SLICEID.1.HOSTNUM
      Ipv4AddressHelper ipv4Helper;
      ipv4Helper.SetBase ("10.0.0.0", "255.0.0.0", baseAddressA);
      hostIpIfacesA = ipv4Helper.Assign (hostDevicesA);
      hostsIpIFaces.Add(hostIpIfacesA);

      // Hosts on switch C with IP address 10.SLICEID.2.HOSTNUM
      ipv4Helper.SetBase ("10.0.0.0", "255.0.0.0", baseAddressC);
      hostIpIfacesC = ipv4Helper.Assign (hostDevicesC);
      serverIpIFaces.Add(hostIpIfacesC);

      // Notify controller about host connections
      for (size_t i = 0; i < hostsA.GetN (); i++)
        {
          controllerApp->NotifyHost (switchPortsA.at (i), hostDevicesA.Get (i));
        }
      
      for (size_t i = 0; i < hostsC.GetN (); i++)
        {
          controllerApp->NotifyHost (switchPortsC.at (i), hostDevicesC.Get (i));
        }

      //Create the applications helpers based on the params 'AppsConfig'
      slice->createAppHelpers();

      std::vector<Ptr<UdpPeerHelper>> appHelpers = slice->GetAppHelpers();
      for(size_t i = 0; i < appHelpers.size(); i++){
        appHelpers[i]->Install (hostsA, hostsC, hostIpIfacesA, hostIpIfacesC, slice->GetSliceId ());
        // appHelpers[i]->SetBothAttribute ("SliceId", UintegerValue (slice->GetSliceId ()));
      }
      
    }
  slcFile.close ();


  // Notify the controler about the network slices.
  controllerApp->NotifySlices (SliceInfo::GetList ());


  // --------------------------------------------------------------------------
  // Simulating the network
  // --------------------------------------------------------------------------
  NS_LOG_INFO ("Simulating...");
  EnableProgress (progress);

  // Create the network statistics object for bandwidth and traffic monitoring.
  Ptr<NetworkStatistics> statistics = CreateObject<NetworkStatistics> ();

  TimeValue timeValue;
  GlobalValue::GetValueByName ("SimTime", timeValue);
  Time stopAt = timeValue.Get () + MilliSeconds (100);

  Simulator::Stop (stopAt);
  Simulator::Run ();


  // Finish the simulation.
  Simulator::Destroy ();

  // Closing output statistic files.
  statistics->Dispose ();
  statistics = 0;

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
/*****************************************************************************/

void
ForceDefaults (void)
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

  // Config::SetDefault (
  //   "ns3::OFSwitch13Port::QueueFactory", StringValue ("ns3::Scenario1queues"));


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
      LogComponentEnable ("Main",                     LOG_LEVEL_ALL);
      LogComponentEnable ("Common",                   logLevelAll);

      // Applications
      LogComponentEnable ("UdpPeerApp",               logLevelWarn);
      LogComponentEnable ("UdpPeerHelper",            logLevelWarnInfo);

      // Infrastructure
      LogComponentEnable ("Controller",               logLevelAll);
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
