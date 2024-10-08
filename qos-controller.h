/*
 * Copyright (c) 2016 University of Campinas (Unicamp)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author: Luciano Jerez Chaves <ljerezchaves@gmail.com>
 */

#ifndef QOS_CONTROLLER_H
#define QOS_CONTROLLER_H

#include "ofswitch13-interface.h"
#include "ofswitch13-socket-handler.h"

#include <ns3/application.h>
#include <ns3/socket.h>
#include <map>
#include <utility> // for std::pair

namespace ns3 {

/**
 * \brief SDN controller for Q-Learning-based Trust Routing (SDN-QLTR)
 */
class QosController : public OFSwitch13Controller
{
  public:
    QosController();            //!< Default constructor.
    virtual ~QosController();   //!< Destructor.

    /** Destructor implementation */
    virtual void DoDispose() override;

    /**
     * Register this type.
     * \return The object TypeId.
     */
    static TypeId GetTypeId();

    /**
     * Handle a packet-in message sent by the switch to this controller.
     * \note This method is virtual to allow for further extension in derived classes.
     * \param msg The OpenFlow received message.
     * \param swtch The remote switch metadata.
     * \param xid The transaction ID from the request message.
     * \return 0 if everything's ok, otherwise an error number.
     */
    virtual ofl_err HandlePacketIn(struct ofl_msg_packet_in* msg,
                                   Ptr<const RemoteSwitch> swtch,
                                   uint32_t xid) override;

  protected:
    // Inherited from OFSwitch13Controller
    virtual void HandshakeSuccessful(Ptr<const RemoteSwitch> swtch) override;

    /**
     * Handle ARP request messages.
     * \param msg The packet-in message.
     * \param swtch The switch information.
     * \param xid Transaction ID.
     * \return 0 if everything's ok, otherwise an error number.
     */
    virtual ofl_err HandleArpPacketIn(struct ofl_msg_packet_in* msg,
                                      Ptr<const RemoteSwitch> swtch,
                                      uint32_t xid);

    /**
     * Handle TCP connection request
     * \param msg The packet-in message.
     * \param swtch The switch information.
     * \param xid Transaction ID.
     * \return 0 if everything's ok, otherwise an error number.
     */
    virtual ofl_err HandleConnectionRequest(struct ofl_msg_packet_in* msg,
                                            Ptr<const RemoteSwitch> swtch,
                                            uint32_t xid);

    /**
     * Q-Learning: Update the Q-table based on the observed reward.
     * \param src The source node's IP address.
     * \param dst The destination node's IP address.
     * \param reward The reward observed for the selected route.
     */
    virtual void UpdateQTable(Ipv4Address src, Ipv4Address dst, double reward);

    /**
     * Q-Learning: Select the next hop based on the current Q-table values.
     * \param src The source node's IP address.
     * \return The IP address of the selected next hop.
     */
    virtual Ipv4Address SelectNextHop(Ipv4Address src);

    /**
     * Trust Management: Update the trust value for a node.
     * \param node The node for which trust is being updated.
     * \param trustValue The new trust value for the node.
     */
    virtual void UpdateTrustTable(Ipv4Address node, double trustValue);

    /**
     * Get the trust value of a node.
     * \param node The IP address of the node whose trust value is to be fetched.
     * \return The trust value of the node.
     */
    virtual double GetTrustValue(Ipv4Address node);

    /**
     * Extract an IPv4 address from packet match.
     * \param oxm_of The OXM_IF_* IPv4 field.
     * \param match The ofl_match structure pointer.
     * \return The IPv4 address.
     */
    virtual Ipv4Address ExtractIpv4Address(uint32_t oxm_of, struct ofl_match* match);

    /**
     * Save the pair IP / MAC address in ARP table.
     * \param ipAddr The IPv4 address.
     * \param macAddr The MAC address.
     */
    virtual void SaveArpEntry(Ipv4Address ipAddr, Mac48Address macAddr);

    /**
     * Perform an ARP resolution.
     * \param ip The Ipv4Address to search.
     * \return The MAC address for this IP.
     */
    virtual Mac48Address GetArpEntry(Ipv4Address ip);

  private:
    /**
     * Q-learning parameters and structures.
     */
    double m_learningRate;            //!< Learning rate for Q-learning.
    double m_discountFactor;          //!< Discount factor for Q-learning.
    double m_explorationRate;         //!< Exploration rate for Q-learning (for exploration-exploitation tradeoff).
    std::map<std::pair<Ipv4Address, Ipv4Address>, double> m_qTable; //!< Q-table storing state-action values (src-dst pairs).

    /**
     * Trust management structures.
     */
    std::map<Ipv4Address, double> m_trustTable; //!< Trust values for nodes.

    Address m_serverIpAddress;  //!< Virtual server IP address.
    uint16_t m_serverTcpPort;   //!< Virtual server TCP port.
    Address m_serverMacAddress; //!< Border switch MAC address.
    bool m_meterEnable;         //!< Enable per-flow metering.
    DataRate m_meterRate;       //!< Per-flow meter rate.
    bool m_linkAggregation;     //!< Enable link aggregation.

    /** Map saving <IPv4 address / MAC address> */
    typedef std::map<Ipv4Address, Mac48Address> IpMacMap_t;
    IpMacMap_t m_arpTable; //!< ARP resolution table.
};

} // namespace ns3

#endif /* QOS_CONTROLLER_H */
