#include "controller.h"
#include <ns3/applications-module.h>
#include <ns3/core-module.h>
#include <ns3/csma-module.h>
#include <ns3/internet-module.h>
#include <ns3/network-module.h>
#include <ns3/ofswitch13-module.h>
#include <ns3/aqua-sim-ng-module.h> // AquaSim module for underwater communication

using namespace ns3;

int main(int argc, char* argv[])
{
    uint16_t clients = 2;
    uint16_t simTime = 10;
    bool verbose = false;
    bool trace = false;

    CommandLine cmd;
    cmd.AddValue("clients", "Number of client nodes", clients);
    cmd.AddValue("simTime", "Simulation time (seconds)", simTime);
    cmd.AddValue("verbose", "Enable verbose output", verbose);
    cmd.AddValue("trace", "Enable datapath stats and pcap traces", trace);
    cmd.Parse(argc, argv);

    if (verbose)
    {
        OFSwitch13Helper::EnableDatapathLogs();
    }

    Config::SetDefault("ns3::OFSwitch13Helper::ChannelType", EnumValue(OFSwitch13Helper::DEDICATED_CSMA));
    Config::SetDefault("ns3::TcpSocket::SegmentSize", UintegerValue(1400));

    NodeContainer serverNodes, switchNodes, controllerNodes, clientNodes;
    serverNodes.Create(2);
    switchNodes.Create(3);
    controllerNodes.Create(2);
    clientNodes.Create(clients);

    // Set up AquaSim devices (for underwater acoustic communication)
    AquaSimHelper aquaSimHelper;
    aquaSimHelper.SetChannelAttribute("Frequency", DoubleValue(25.0)); // Example frequency setup

    // Set up Csma devices (wired network)
    CsmaHelper csmaHelper;
    csmaHelper.SetChannelAttribute("DataRate", DataRateValue(DataRate("100Mbps")));
    
    NetDeviceContainer link, switch0Ports, switch1Ports, switch2Ports, serverDevices, clientDevices;

    // Connect switches with AquaSim and Csma
    link = csmaHelper.Install(NodeContainer(switchNodes.Get(0), switchNodes.Get(1)));
    switch0Ports.Add(link.Get(0)); switch1Ports.Add(link.Get(1));
    link = csmaHelper.Install(NodeContainer(switchNodes.Get(1), switchNodes.Get(2)));
    switch1Ports.Add(link.Get(0)); switch2Ports.Add(link.Get(1));

    link = csmaHelper.Install(NodeContainer(serverNodes.Get(0), switchNodes.Get(0)));
    serverDevices.Add(link.Get(0)); switch0Ports.Add(link.Get(1));

    // Clients connected with AquaSim
    for (size_t i = 0; i < clients; i++)
    {
        link = aquaSimHelper.Install(NodeContainer(clientNodes.Get(i), switchNodes.Get(2)));
        clientDevices.Add(link.Get(0)); switch2Ports.Add(link.Get(1));
    }

    Ptr<OFSwitch13InternalHelper> ofQosHelper = CreateObject<OFSwitch13InternalHelper>();
    Ptr<QosController> qosCtrl = CreateObject<QosController>();
    ofQosHelper->InstallController(controllerNodes.Get(0), qosCtrl);

    OFSwitch13DeviceContainer ofSwitchDevices;
    ofSwitchDevices.Add(ofQosHelper->InstallSwitch(switchNodes.Get(0), switch0Ports));

    InternetStackHelper internet;
    internet.Install(serverNodes);
    internet.Install(clientNodes);

    if (trace)
    {
        ofQosHelper->EnableOpenFlowPcap("openflow"); // OpenFlow traffic
        csmaHelper.EnablePcap("server", serverDevices.Get(0)); // Capture server traffic
        csmaHelper.EnablePcap("client", clientDevices.Get(0));  // Capture client traffic
        csmaHelper.EnablePcap("switch", switch0Ports.Get(0));    // Capture switch traffic
    }

    Simulator::Stop(Seconds(simTime));
    Simulator::Run();
    Simulator::Destroy();
}
