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
