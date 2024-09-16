#include "controller.h"
#include <ns3/log.h>
#include <gsl/gsl_histogram.h>

NS_LOG_COMPONENT_DEFINE("QLTRController");
NS_OBJECT_ENSURE_REGISTERED(QLTRController);

QLTRController::QLTRController()
{
    NS_LOG_FUNCTION(this);
    m_totalBytesReceived = 0;
    m_simulationTime = 50.0; // Example total simulation time in seconds
    throughputHistogram = gsl_histogram_alloc(100); // Create histogram for throughput with 100 bins
    gsl_histogram_set_ranges_uniform(throughputHistogram, 0, 100);
}

QLTRController::~QLTRController()
{
    gsl_histogram_free(throughputHistogram); // Free memory for the histogram
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

    if (ethType == ArpL3Protocol::PROT_NUMBER)
    {
        return HandleArpPacketIn(msg, swtch, xid);
    }
    else if (ethType == Ipv4L3Protocol::PROT_NUMBER)
    {
        Ipv4Address srcIp = ExtractIpv4Address(OXM_OF_IPV4_SRC, (struct ofl_match*)msg->match);
        Ipv4Address dstIp = ExtractIpv4Address(OXM_OF_IPV4_DST, (struct ofl_match*)msg->match);

        TrustEvaluation(srcIp, dstIp);
        QLearningRouting(srcIp, dstIp);

        // Update throughput statistics
        m_totalBytesReceived += msg->data_length;

        // Add data to histogram
        gsl_histogram_increment(throughputHistogram, (msg->data_length * 8.0) / m_simulationTime);
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

    // Print histogram for throughput (using GSL)
    NS_LOG_INFO("Histogram of Throughput:");
    gsl_histogram_fprintf(stdout, throughputHistogram, "%g", "%g");
}
