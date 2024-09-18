#include "controller.h"
#include "ns3/log.h"
#include "ns3/arp-l3-protocol.h"   // For ARP handling
#include "ns3/ipv4-address.h"      // For handling IPv4 addresses

NS_LOG_COMPONENT_DEFINE("QLTRController");
NS_OBJECT_ENSURE_REGISTERED(QLTRController);

QLTRController::QLTRController()
{
    NS_LOG_FUNCTION(this);
    m_totalBytesReceived = 0;
    m_simulationTime = 50.0; // Example total simulation time in seconds
}

QLTRController::~QLTRController()
{
}

void
QLTRController::DoDispose()
{
    NS_LOG_FUNCTION(this);
    OFSwitch13Controller::DoDispose();
}

TypeId
QLTRController::GetTypeId()
{
    static TypeId tid = TypeId("ns3::QLTRController")
                            .SetParent<OFSwitch13Controller>()
                            .SetGroupName("OFSwitch13")
                            .AddConstructor<QLTRController>();
    return tid;
}

void
QLTRController::TrustEvaluation(Ipv4Address src, Ipv4Address dst)
{
    // Evaluate trust between src and dst
    if (m_trustValues[src] < 0.7)
    {
        m_trustValues[src] += 0.1; // Update trust values based on successful transmissions
    }
    NS_LOG_INFO("Trust value of " << src << ": " << m_trustValues[src]);
}

void
QLTRController::QLearningRouting(Ipv4Address src, Ipv4Address dst)
{
    // Q-Learning routing logic
    if (m_qValues[src] < 1.0)
    {
        m_qValues[src] += 0.1;  // Update Q-values based on routing decisions
        NS_LOG_INFO("Q-value of " << src << ": " << m_qValues[src]);
    }
}

Ipv4Address
QLTRController::ExtractIpv4Address(uint32_t oxm_of, struct ofl_match* match)
{
    // Extracts IPv4 addresses from OpenFlow match structures
    switch (oxm_of)
    {
    case OXM_OF_IPV4_SRC:
    case OXM_OF_IPV4_DST:
    {
        uint32_t ip;
        struct ofl_match_tlv* tlv = oxm_match_lookup(oxm_of, match);
        memcpy(&ip, tlv->value, OXM_LENGTH(oxm_of));
        return Ipv4Address(ntohl(ip));
    }
    default:
        NS_ABORT_MSG("Invalid IPv4 field.");
    }
}

ofl_err
QLTRController::HandlePacketIn(struct ofl_msg_packet_in* msg,
                               Ptr<const RemoteSwitch> swtch,
                               uint32_t xid)
{
    NS_LOG_FUNCTION(this << swtch << xid);
    uint16_t ethType;
    struct ofl_match_tlv* tlv;

    tlv = oxm_match_lookup(OXM_OF_ETH_TYPE, (struct ofl_match*)msg->match);
    memcpy(&ethType, tlv->value, OXM_LENGTH(OXM_OF_ETH_TYPE));

    if (ethType == ArpL3Protocol::PROT_NUMBER)  // ARP handling
    {
        // Handle ARP packets
        return 0;
    }
    else if (ethType == Ipv4L3Protocol::PROT_NUMBER)
    {
        Ipv4Address srcIp = ExtractIpv4Address(OXM_OF_IPV4_SRC, (struct ofl_match*)msg->match);
        Ipv4Address dstIp = ExtractIpv4Address(OXM_OF_IPV4_DST, (struct ofl_match*)msg->match);

        TrustEvaluation(srcIp, dstIp);
        QLearningRouting(srcIp, dstIp);

        // Update throughput statistics
        m_totalBytesReceived += msg->data_length;
    }

    ofl_msg_free((struct ofl_msg_header*)msg, nullptr);
    return 0;
}

void
QLTRController::CalculateThroughput()
{
    // Calculate throughput (in Mbps)
    double throughput = (m_totalBytesReceived * 8.0) / (m_simulationTime * 1000000.0);
    NS_LOG_INFO("Network Throughput: " << throughput << " Mbps");
}

void
QLTRController::CalculateEfficiency()
{
    // Example efficiency calculation
    double efficiency = (m_totalBytesReceived / (m_simulationTime * 1000000)) * 100;
    NS_LOG_INFO("Network Efficiency: " << efficiency << "%");
}

void
QLTRController::PrintResults()
{
    CalculateThroughput();
    CalculateEfficiency();
}
