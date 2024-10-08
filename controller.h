#ifndef CONTROLLER_H
#define CONTROLLER_H

#include <ns3/ofswitch13-module.h>

using namespace ns3;

class QosController : public OFSwitch13Controller
{
public:
    QosController();           //!< Default constructor.
    ~QosController() override; //!< Destructor.

    void DoDispose() override; //!< Destructor implementation.

    static TypeId GetTypeId(); //!< Register this type.

    // Handle packet-in message
    ofl_err HandlePacketIn(struct ofl_msg_packet_in* msg,
                           Ptr<const RemoteSwitch> swtch,
                           uint32_t xid) override;

protected:
    void HandshakeSuccessful(Ptr<const RemoteSwitch> swtch) override; //!< Handshake success.

private:
    void ConfigureBorderSwitch(Ptr<const RemoteSwitch> swtch); //!< Configure the border switch.
    void ConfigureAggregationSwitch(Ptr<const RemoteSwitch> swtch); //!< Configure aggregation switch.

    // ARP handling and connection request handling
    ofl_err HandleArpPacketIn(struct ofl_msg_packet_in* msg,
                              Ptr<const RemoteSwitch> swtch,
                              uint32_t xid);
    ofl_err HandleConnectionRequest(struct ofl_msg_packet_in* msg,
                                    Ptr<const RemoteSwitch> swtch,
                                    uint32_t xid);

    Ipv4Address ExtractIpv4Address(uint32_t oxm_of, struct ofl_match* match); //!< Extract IPv4 address from match.
    
    // Create ARP packets
    Ptr<Packet> CreateArpRequest(Mac48Address srcMac, Ipv4Address srcIp, Ipv4Address dstIp);
    Ptr<Packet> CreateArpReply(Mac48Address srcMac, Ipv4Address srcIp, Mac48Address dstMac, Ipv4Address dstIp);

    void SaveArpEntry(Ipv4Address ipAddr, Mac48Address macAddr); //!< Save ARP entries.
    Mac48Address GetArpEntry(Ipv4Address ip); //!< Get ARP entry from the table.

    // QoS attributes
    Address m_serverIpAddress;
    uint16_t m_serverTcpPort;
    Address m_serverMacAddress;
    bool m_meterEnable;
    DataRate m_meterRate;
    bool m_linkAggregation;

    // ARP Table
    typedef std::map<Ipv4Address, Mac48Address> IpMacMap_t;
    IpMacMap_t m_arpTable;
};

#endif /* CONTROLLER_H */
