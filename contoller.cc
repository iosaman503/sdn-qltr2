#include "header.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("SDNController");

NS_OBJECT_ENSURE_REGISTERED (SDNController);

TypeId SDNController::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::SDNController")
    .SetParent<Application> ()
    .SetGroupName ("OFSwitch13")
    .AddConstructor<SDNController> ()
  ;
  return tid;
}

SDNController::SDNController ()
{
  NS_LOG_FUNCTION (this);
}

SDNController::~SDNController ()
{
  NS_LOG_FUNCTION (this);
}

void
SDNController::SetupSwitch (Ptr<OFSwitch13Device> swtch)
{
  NS_LOG_FUNCTION (this << swtch);
  m_switches.push_back (swtch);
}

void
SDNController::Learn (Ptr<const Packet> packet, const Address& source)
{
  NS_LOG_FUNCTION (this << packet << source);
  Mac48Address src = Mac48Address::ConvertFrom (source);
  m_macToPort[src] = packet->GetUid ();
}

void
SDNController::StartApplication ()
{
  NS_LOG_FUNCTION (this);
}

void
SDNController::StopApplication ()
{
  NS_LOG_FUNCTION (this);
}

}
