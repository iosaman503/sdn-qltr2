#include "controller.h"
#include <ns3/applications-module.h>
#include <ns3/core-module.h>
#include <ns3/internet-module.h>
#include <ns3/network-module.h>
#include <ns3/aqua-sim-ng-module.h>
#include <ns3/ofswitch13-module.h>

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
        LogComponentEnable("AquaSimRouting", LOG_LEVEL_ALL);
        LogComponentEnable("OFSwitch13Controller", LOG_LEVEL_ALL);
        LogComponentEnable("QLTRController", LOG_LEVEL_ALL);
    }

    // Create nodes for the underwater network
    NodeContainer nodes;
    nodes.Create(5);

    // Setup Aqua-Sim NG for underwater communication
    AquaSimHelper asHelper;
    asHelper.SetChannel("ns3::AquaSimSingleChannel");
    NetDeviceContainer devices = asHelper.Install(nodes);

    // Install Internet stack
    InternetStackHelper internet;
    internet.Install(nodes);

    // Set up mobility
    AquaSimMobilityHelper mobility;
    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.Install(nodes);

    // Assign IP addresses
    Ipv4AddressHelper ipv4;
    ipv4.SetBase("10.0.0.0", "255.255.255.0");
    Ipv4InterfaceContainer interfaces = ipv4.Assign(devices);

    // Set up SDN controller (QLTRController)
    Ptr<QLTRController> qltrController = CreateObject<QLTRController>();

    // Set up OpenFlow switches and controller
    OFSwitch13Helper ofSwitchHelper;
    ofSwitchHelper.InstallController(nodes.Get(0), qltrController); // Controller installed on node 0
    ofSwitchHelper.InstallSwitch(nodes.Get(1), devices); // Switch installed on node 1
    ofSwitchHelper.CreateOpenFlowChannels(); // Create OpenFlow channels

    // Set up traffic generation from clients to servers
    // PacketSinkHelper for receiving data at destination node
    PacketSinkHelper sinkHelper("ns3::TcpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), 9));
    ApplicationContainer sinkApps = sinkHelper.Install(nodes.Get(1)); // Install sink at node 1 (server)
    sinkApps.Start(Seconds(1.0));

    // BulkSendHelper for sending data from client to server
    BulkSendHelper bulkSender("ns3::TcpSocketFactory", InetSocketAddress(interfaces.GetAddress(1), 9)); 
    ApplicationContainer senderApps = bulkSender.Install(nodes.Get(2)); // Install sender at node 2 (client)
    senderApps.Start(Seconds(2.0));

    // Enable tracing
    if (verbose)
    {
        asHelper.EnableAsciiAll("aqua-sim");
        ofSwitchHelper.EnableOpenFlowPcap("openflow");
        ofSwitchHelper.EnableDatapathStats("switch-stats");
    }

    // Run the simulation
    Simulator::Stop(Seconds(simTime));
    Simulator::Run();
    Simulator::Destroy();

    // Print results for throughput and efficiency
    qltrController->PrintResults();

    // Print the total bytes received at the sink node
    Ptr<PacketSink> sink = DynamicCast<PacketSink>(sinkApps.Get(0));
    std::cout << "Bytes received: " << sink->GetTotalRx() << std::endl;

    return 0;
}
