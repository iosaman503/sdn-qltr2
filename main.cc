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
    ipv4.Assign(devices);

    // Setup SDN Controller
    Ptr<QLTRController> controller = CreateObject<QLTRController>();
    NodeContainer controllerNode;
    controllerNode.Create(1);
    controllerNode.Get(0)->AddApplication(controller);

    // Install OFSwitch13 and connect to SDN controller
    OFSwitch13Helper ofSwitchHelper;
    ofSwitchHelper.SetStackIoModel(OFStackIoModel::IO_TYPE_NETLINK);
    NetDeviceContainer switchDevices = ofSwitchHelper.Install(nodes);

    // Connect the OFSwitch13 to the SDN controller
    ofSwitchHelper.InstallController(controller, switchDevices);

    // Set up applications (e.g., traffic generators)
    uint16_t port = 9;
    OnOffHelper onOffHelper("ns3::UdpSocketFactory", InetSocketAddress(Ipv4Address("255.255.255.255"), port));
    onOffHelper.SetAttribute("DataRate", StringValue("1Mbps"));
    onOffHelper.SetAttribute("PacketSize", UintegerValue(1024));

    ApplicationContainer apps = onOffHelper.Install(nodes.Get(0));
    apps.Start(Seconds(1.0));
    apps.Stop(Seconds(simTime - 1.0));

    // Install packet sink on the receiver node
    PacketSinkHelper packetSinkHelper("ns3::UdpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), port));
    apps = packetSinkHelper.Install(nodes.Get(1));
    apps.Start(Seconds(0.0));
    apps.Stop(Seconds(simTime));

    // Configure and run the simulation
    Simulator::Stop(Seconds(simTime));
    Simulator::Run();
    Simulator::Destroy();

    return 0;
}
