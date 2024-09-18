#include "controller.h"
#include "ns3/log.h"

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

Ipv4Address
QLTRController::ExtractIpv4Address(uint32_t oxm_of, struct ofl_match* match)
{
    struct ofl_match_tlv* tlv;
    
    // Extract IP addresses from the match fields
    switch (oxm_of)
    {
    case OXM_OF_IPV4_SRC:
    case OXM_OF_IPV4_DST:
    {
        uint32_t ip = 0; // Initialize a 32-bit unsigned integer for the IP address
        
        // Look up the OXM match field (source or destination IP)
        tlv = oxm_match_lookup(oxm_of, match);
        if (tlv)
        {
            // Ensure that we are correctly copying the value to the uint32_t variable
            memcpy(&ip, tlv->value, sizeof(ip));
            return Ipv4Address(ntohl(ip));  // Convert to host byte order
        }
        return Ipv4Address("0.0.0.0");  // Default invalid address if no match found
    }
    default:
        NS_ABORT_MSG("Invalid IP field.");
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

    if (ethType == Ipv4L3Protocol::PROT_NUMBER)
    {
        Ipv4Address srcIp = ExtractIpv4Address(OXM_OF_IPV4_SRC, (struct ofl_match*)msg->match);
        Ipv4Address dstIp = ExtractIpv4Address(OXM_OF_IPV4_DST, (struct ofl_match*)msg->match);

        // Basic packet handling for throughput calculation
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
