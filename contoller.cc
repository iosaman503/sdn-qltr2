#include "controller.h"
#include <ns3/internet-module.h>
#include <ns3/network-module.h>

NS_LOG_COMPONENT_DEFINE("QosController");
NS_OBJECT_ENSURE_REGISTERED(QosController);

QosController::QosController()
    : m_learningRate(0.5), m_discountFactor(0.9), m_explorationRate(0.2) // Initialize Q-learning parameters
{
    NS_LOG_FUNCTION(this);
}

QosController::~QosController() { NS_LOG_FUNCTION(this); }

void QosController::DoDispose() 
{
    m_qTable.clear();
    m_trustTable.clear();
    OFSwitch13Controller::DoDispose();
}

TypeId QosController::GetTypeId()
{
    static TypeId tid = TypeId("ns3::QosController")
                            .SetParent<OFSwitch13Controller>()
                            .SetGroupName("OFSwitch13")
                            .AddConstructor<QosController>();
    return tid;
}

ofl_err QosController::HandlePacketIn(struct ofl_msg_packet_in* msg, Ptr<const RemoteSwitch> swtch, uint32_t xid)
{
    NS_LOG_FUNCTION(this << swtch << xid);

    Ipv4Address src = /* Extract source IP from the packet */;
    Ipv4Address dst = /* Extract destination IP from the packet */;

    // Select next hop using Q-learning
    Ipv4Address nextHop = SelectNextHop(src);

    // Forward packet to next hop or controller
    // (Omitted for brevity)

    return 0; 
}

void QosController::HandshakeSuccessful(Ptr<const RemoteSwitch> swtch)
{
    NS_LOG_FUNCTION(this << swtch);
    ConfigureSwitch(swtch);
}

void QosController::ConfigureSwitch(Ptr<const RemoteSwitch> swtch)
{
    NS_LOG_FUNCTION(this << swtch);

    uint64_t swDpId = swtch->GetDpId();
    DpctlExecute(swDpId, "set-config miss=128");
    DpctlExecute(swDpId, "flow-mod cmd=add,table=0,prio=20 eth_type=0x0806,arp_op=1 apply:output=ctrl");
}

// Q-learning related functions
void QosController::UpdateQTable(Ipv4Address src, Ipv4Address dst, double reward)
{
    std::pair<Ipv4Address, Ipv4Address> state = std::make_pair(src, dst);
    m_qTable[state] = m_qTable[state] + m_learningRate * (reward + m_discountFactor * 0 - m_qTable[state]);
}

Ipv4Address QosController::SelectNextHop(Ipv4Address src)
{
    // Select next hop using exploration-exploitation tradeoff
    if (m_explorationRate > (rand() / RAND_MAX))
    {
        // Exploration: Select a random next hop
        return /* Some random next hop */;
    }
    else
    {
        // Exploitation: Select the next hop with the highest Q-value
        Ipv4Address bestNextHop;
        double maxQValue = -1;
        for (const auto& entry : m_qTable)
        {
            if (entry.first.first == src && entry.second > maxQValue)
            {
                bestNextHop = entry.first.second;
                maxQValue = entry.second;
            }
        }
        return bestNextHop;
    }
}

// Trust-related functions
void QosController::UpdateTrustTable(Ipv4Address node, double trustValue)
{
    m_trustTable[node] = trustValue;
}

double QosController::GetTrustValue(Ipv4Address node)
{
    return m_trustTable[node];
}
