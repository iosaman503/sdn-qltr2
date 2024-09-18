#ifndef QLTR_CONTROLLER_H
#define QLTR_CONTROLLER_H

#include "ns3/ofswitch13-module.h"
#include "ns3/aqua-sim-ng-module.h"
#include "ns3/arp-l3-protocol.h"  // For ARP handling
#include "ns3/ipv4-address.h"     // For IPv4 address handling
#include <map>

using namespace ns3;

/**
 * \brief Q-Learning-Assisted Trust Routing (SDN-QLTR) Controller
 */
class QLTRController : public OFSwitch13Controller
{
  public:
    QLTRController();           //!< Default constructor.
    ~QLTRController() override; //!< Destructor.

    /** Destructor implementation */
    void DoDispose() override;

    /**
     * Register this type.
     * \return The object TypeId.
     */
    static TypeId GetTypeId();

    /**
     * Handle a packet in message sent by the switch to this controller.
     * \note Inherited from OFSwitch13Controller.
     * \param msg The OpenFlow received message.
     * \param swtch The remote switch metadata.
     * \param xid The transaction id from the request message.
     * \return 0 if everything's ok, otherwise an error number.
     */
    ofl_err HandlePacketIn(struct ofl_msg_packet_in* msg,
                           Ptr<const RemoteSwitch> swtch,
                           uint32_t xid) override;

    /**
     * Print final results like throughput and efficiency.
     */
    void PrintResults();

  private:
    /**
     * Handle Q-Learning routing decision.
     * \param src Source IP address.
     * \param dst Destination IP address.
     */
    void QLearningRouting(Ipv4Address src, Ipv4Address dst);

    /**
     * Perform trust evaluation for nodes.
     * \param src Source IP address.
     * \param dst Destination IP address.
     */
    void TrustEvaluation(Ipv4Address src, Ipv4Address dst);

    /**
     * Extract an IPv4 address from OpenFlow match structures.
     */
    Ipv4Address ExtractIpv4Address(uint32_t oxm_of, struct ofl_match* match);

    /**
     * Calculate throughput based on flow statistics.
     */
    void CalculateThroughput();

    /**
     * Calculate network efficiency.
     */
    void CalculateEfficiency();

    std::map<Ipv4Address, double> m_trustValues; //!< Trust values for nodes.
    std::map<Ipv4Address, double> m_qValues;     //!< Q-learning values for nodes.
    uint64_t m_totalBytesReceived;               //!< Total bytes received.
    double m_simulationTime;                     //!< Total simulation time.
};

#endif /* QLTR_CONTROLLER_H */
