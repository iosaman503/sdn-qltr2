#include "controller.h"
#include "ns3/applications-module.h"
#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "ns3/network-module.h"
#include "ns3/ofswitch13-module.h"

using namespace ns3;

int main(int argc, char* argv[])
{
    uint16_t clients = 2;
    uint16_t simTime = 50;
    bool verbose = true;

    CommandLine cmd;
    cmd.AddValue("clients", "Number of client nodes", clients);
    cmd.AddValue("simTime", "Simulation time (seconds)", simTime);
    cmd.Parse(argc, argv);

    if (verbose)
    {
        OFSwitch13Helper::EnableDatapathLogs();
        LogComponentEnable("OFSwitch13Controller", LOG_LEVEL_ALL);
        LogComponentEnable("QLTRController", LOG_LEVEL_ALL);
    }

    // Create nodes for the network
    NodeContainer nodes;
    nodes.Create(5);

    // Install Internet stack
    InternetStackHelper internet;
    internet.Install(nodes);

    // Set up CSMA devices (wired network links)
    CsmaHelper csma;
    csma.SetChannelAttribute("DataRate", DataRateValue(DataRate("100Mbps")));
    csma.SetChannelAttribute("Delay", TimeValue(MilliSeconds(2)));
    NetDeviceContainer devices = csma.Install(nodes);

    // Assign IP addresses
    Ipv4AddressHelper ipv4;
    ipv4.SetBase("10.0.0.0", "255.255.255.0");
    Ipv4InterfaceContainer interfaces = ipv4.Assign(devices);

    // Set up SDN controller (QLTRController)
    Ptr<QLTRController> qltrController = CreateObject<QLTRController>();

    // Set up OpenFlow controller and switch
    OFSwitch13InternalHelper ofSwitchHelper;
    ofSwitchHelper.InstallController(nodes.Get(0), qltrController); // Controller installed on node 0
    ofSwitchHelper.InstallSwitch(nodes.Get(1), devices.Get(1));     // Switch on node 1
    ofSwitchHelper.CreateOpenFlowChannels(); // Create OpenFlow channels

    // Set up traffic generation from clients to servers
    PacketSinkHelper sinkHelper("ns3::TcpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), 9));
    ApplicationContainer sinkApps = sinkHelper.Install(nodes.Get(1)); // Sink on node 1 (server)
    sinkApps.Start(Seconds(1.0));

    BulkSendHelper bulkSender("ns3::TcpSocketFactory", InetSocketAddress(interfaces.GetAddress(1), 9)); 
    ApplicationContainer senderApps = bulkSender.Install(nodes.Get(2)); // Sender on node 2 (client)
    senderApps.Start(Seconds(2.0));

    // Enable pcap tracing (optional)
    csma.EnablePcapAll("qltr-simulation");

    // Run the simulation
    Simulator::Stop(Seconds(simTime));
    Simulator::Run();
    Simulator::Destroy();

    // Print throughput and efficiency results
    qltrController->PrintResults();

    return 0;
}
