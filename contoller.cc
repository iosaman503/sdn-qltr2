#include "qos-controller.h"
#include <ns3/internet-module.h>
#include <ns3/network-module.h>

NS_LOG_COMPONENT_DEFINE("QosController");
NS_OBJECT_ENSURE_REGISTERED(QosController);

QosController::QosController() { NS_LOG_FUNCTION(this); }
QosController::~QosController() { NS_LOG_FUNCTION(this); }

void QosController::DoDispose() { m_arpTable.clear(); OFSwitch13Controller::DoDispose(); }

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
    return 0; // Simplified handling for basic implementation
}

void QosController::HandshakeSuccessful(Ptr<const RemoteSwitch> swtch)
{
    NS_LOG_FUNCTION(this << swtch);
    // Simplified configuration without attack handling
    if (swtch->GetDpId() == 1) ConfigureBorderSwitch(swtch);
    else if (swtch->GetDpId() == 2) ConfigureAggregationSwitch(swtch);
}

void QosController::ConfigureBorderSwitch(Ptr<const RemoteSwitch> swtch) 
{
    NS_LOG_FUNCTION(this << swtch);
    uint64_t swDpId = swtch->GetDpId();
    DpctlExecute(swDpId, "set-config miss=128");
    DpctlExecute(swDpId, "flow-mod cmd=add,table=0,prio=20 eth_type=0x0806,arp_op=1 apply:output=ctrl");
}

void QosController::ConfigureAggregationSwitch(Ptr<const RemoteSwitch> swtch)
{
    NS_LOG_FUNCTION(this << swtch);
    uint64_t swDpId = swtch->GetDpId();
    DpctlExecute(swDpId, "flow-mod cmd=add,table=0,prio=500 in_port=1 write:output=3");
}
