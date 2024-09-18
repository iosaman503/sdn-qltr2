#ifndef SDN_CONTROLLER_H
#define SDN_CONTROLLER_H

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/ofswitch13-module.h"

#include <vector>
#include <map>

namespace ns3 {

class SDNController : public Application 
{
public:
  static TypeId GetTypeId (void);
  SDNController ();
  virtual ~SDNController ();

  void SetupSwitch (Ptr<OFSwitch13Device> swtch);
  void Learn (Ptr<const Packet> packet, const Address& source);

private:
  virtual void StartApplication (void);
  virtual void StopApplication (void);

  std::map<Mac48Address, uint32_t> m_macToPort;
  std::vector<Ptr<OFSwitch13Device>> m_switches;
};

NS_OBJECT_ENSURE_REGISTERED (SDNController);

TypeId 
SDNController::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::SDNController")
    .SetParent<Application> ()
    .SetGroupName ("OFSwitch13")
    .AddConstructor<SDNController> ();
  return tid;
}

SDNController::SDNController () {}

SDNController::~SDNController () {}

void
SDNController::SetupSwitch (Ptr<OFSwitch13Device> swtch)
{
  m_switches.push_back (swtch);
}

void
SDNController::Learn (Ptr<const Packet> packet, const Address& source)
{
  Mac48Address src = Mac48Address::ConvertFrom (source);
  m_macToPort[src] = packet->GetUid ();
}

void
SDNController::StartApplication ()
{
  // Add any necessary initialization code here
}

void
SDNController::StopApplication ()
{
  // Add any necessary cleanup code here
}

} // namespace ns3

#endif // SDN_CONTROLLER_H
