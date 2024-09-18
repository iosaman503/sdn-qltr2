#ifndef SDN_QLTR_H
#define SDN_QLTR_H

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/csma-module.h"
#include "ns3/internet-module.h"
#include "ns3/ofswitch13-module.h"
#include "ns3/netanim-module.h"
#include "ns3/applications-module.h"
#include "ns3/node.h"

#include <vector>
#include <map>

namespace ns3 {

class SDNController : public ns3::Node {
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

}

#endif
