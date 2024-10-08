#ifndef CONTROLLER_H
#define CONTROLLER_H

#include <ns3/ofswitch13-module.h>
#include <map>

using namespace ns3;

class QosController : public OFSwitch13Controller
{
public:
    QosController();
    ~QosController() override;

    void DoDispose() override;

    static TypeId GetTypeId();

    // Handle packet-in message
    ofl_err HandlePacketIn(struct ofl_msg_packet_in* msg,
                           Ptr<const RemoteSwitch> swtch,
                           uint32_t xid) override;

protected:
    void HandshakeSuccessful(Ptr<const RemoteSwitch> swtch) override;

private:
    void ConfigureSwitch(Ptr<const RemoteSwitch> swtch);

    // Q-learning related functions
    void UpdateQTable(Ipv4Address src, Ipv4Address dst, double reward);
    Ipv4Address SelectNextHop(Ipv4Address src);

    // Trust-related functions
    void UpdateTrustTable(Ipv4Address node, double trustValue);
    double GetTrustValue(Ipv4Address node);

    // Data structures for Q-learning and trust
    std::map<std::pair<Ipv4Address, Ipv4Address>, double> m_qTable; // Q-table
    std::map<Ipv4Address, double> m_trustTable; // Trust table for each node

    // Parameters for Q-learning
    double m_learningRate;
    double m_discountFactor;
    double m_explorationRate;
};

#endif /* CONTROLLER_H */
