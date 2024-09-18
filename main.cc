#include "controller.h"
#include "ns3/ofswitch13-module.h"

using namespace ns3;

int
main (int argc, char *argv[])
{
  uint16_t simTime = 10;
  bool verbose = true;
  bool trace = true;

  // Configure command line parameters
  CommandLine cmd (__FILE__);
  cmd.AddValue ("simTime", "Simulation time (seconds)", simTime);
  cmd.AddValue ("verbose", "Enable verbose output", verbose);
  cmd.AddValue ("trace", "Enable datapath stats and pcap traces", trace);
  cmd.Parse (argc, argv);

  if (verbose)
    {
      OFSwitch13Helper::EnableDatapathLogs ();
    }

  // Create nodes
  NodeContainer switches;
  switches.Create (1);
  NodeContainer hosts;
  hosts.Create (4);

  // Create links
  CsmaHelper csmaHelper;
  csmaHelper.SetChannelAttribute ("DataRate", DataRateValue (DataRate ("100Mbps")));
  csmaHelper.SetChannelAttribute ("Delay", TimeValue (MilliSeconds (2)));

  NetDeviceContainer hostDevices;
  NetDeviceContainer switchPorts;
  for (size_t i = 0; i < hosts.GetN (); i++)
    {
      NetDeviceContainer link = csmaHelper.Install (NodeContainer (hosts.Get (i), switches.Get (0)));
      hostDevices.Add (link.Get (0));
      switchPorts.Add (link.Get (1));
    }

  // Create switch device
  Ptr<OFSwitch13InternalHelper> of13Helper = CreateObject<OFSwitch13InternalHelper> ();
  Ptr<OFSwitch13Device> ofSwitch = of13Helper->InstallSwitch (switches.Get (0), switchPorts);

  // // Create controller
  // Ptr<SDNController> controller = CreateObject<SDNController> ();
  // controller->SetupSwitch (ofSwitch);
  // Create controller
// Ptr<SDNController> sdnController = CreateObject<SDNController> ();
// sdnController->SetupSwitch (ofSwitch);
// Ptr<OFSwitch13Controller> controller = DynamicCast<OFSwitch13Controller> (sdnController);
// of13Helper->InstallController (controller);


  Ptr<Node> ofSwitch = CreateObject<Node> (); // Ensure ofSwitch is a Node
Ptr<SDNController> sdnController = CreateObject<SDNController> ();

// Assuming SetupSwitch is a method that sets up a node with a controller
ofSwitch->SetupController(sdnController);

// Install the OFSwitch13 controller using the correct type
Ptr<OFSwitch13Controller> ofSwitchController = DynamicCast<OFSwitch13Controller> (sdnController);

if (ofSwitchController == nullptr) {
    NS_LOG_ERROR ("Failed to cast SDNController to OFSwitch13Controller");
    return 1;
}

of13Helper->InstallController(ofSwitchController);


 // of13Helper->InstallController (controller);

  // Install Internet stack
  InternetStackHelper internet;
  internet.Install (hosts);

  // Configure IP addresses
  Ipv4AddressHelper ipv4Helper;
  Ipv4InterfaceContainer hostIpIfaces;
  ipv4Helper.SetBase ("10.1.1.0", "255.255.255.0");
  hostIpIfaces = ipv4Helper.Assign (hostDevices);

  // Configure traffic application
  uint16_t port = 9;
  Address serverAddress (InetSocketAddress (hostIpIfaces.GetAddress (0), port));
  PacketSinkHelper packetSinkHelper ("ns3::TcpSocketFactory", serverAddress);
  ApplicationContainer serverApps = packetSinkHelper.Install (hosts.Get (0));
  serverApps.Start (Seconds (0.0));

  OnOffHelper onoff ("ns3::TcpSocketFactory", serverAddress);
  onoff.SetConstantRate (DataRate ("10Mb/s"));
  onoff.SetAttribute ("PacketSize", UintegerValue (1000));
  ApplicationContainer clientApps = onoff.Install (hosts.Get (1));
  clientApps.Start (Seconds (1.0));

  // Enable datapath stats and pcap traces at hosts and switch ports
  if (trace)
    {
      of13Helper->EnableDatapathStats ("switch-stats");
      csmaHelper.EnablePcap ("switch", switchPorts, true);
      csmaHelper.EnablePcap ("host", hostDevices);
    }

  // Run the simulation
  Simulator::Stop (Seconds (simTime));
  Simulator::Run ();
  Simulator::Destroy ();

  // Calculate and output throughput and efficiency
  Ptr<PacketSink> sink = DynamicCast<PacketSink> (serverApps.Get (0));
  double throughput = sink->GetTotalRx () * 8.0 / (simTime * 1000000.0); // Mbps
  double efficiency = throughput / 10.0; // Assuming 10Mbps was the sending rate

  std::cout << "Throughput: " << throughput << " Mbps" << std::endl;
  std::cout << "Efficiency: " << efficiency * 100 << "%" << std::endl;

  return 0;
}
