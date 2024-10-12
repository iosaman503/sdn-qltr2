import eventlet
eventlet.monkey_patch()
from os_ken.base import app_manager
from os_ken.controller import ofp_event
from os_ken.controller.handler import CONFIG_DISPATCHER, MAIN_DISPATCHER
from os_ken.controller.handler import set_ev_cls
from os_ken.ofproto import ofproto_v1_3
from os_ken.lib.packet import packet, ethernet, ipv4
from os_ken.lib import hub
import random

class SDNQLTRController(app_manager.OSKenApp):
    OFP_VERSIONS = [ofproto_v1_3.OFP_VERSION]

    def __init__(self, *args, **kwargs):
        super(SDNQLTRController, self).__init__(*args, **kwargs)
        self.mac_to_port = {}
        self.datapaths = {}
        self.trust_values = {}  # Trust values for each node
        self.q_values = {}      # Q-learning Q-values for each path
        self.monitor_thread = hub.spawn(self._monitor)
        self.learning_rate = 0.5
        self.discount_factor = 0.9
        self.exploration_rate = 0.3
        print("SDNQLTRController initialized")

    @set_ev_cls(ofp_event.EventOFPSwitchFeatures, CONFIG_DISPATCHER)
    def switch_features_handler(self, ev):
        datapath = ev.msg.datapath
        ofproto = datapath.ofproto
        parser = datapath.ofproto_parser

        match = parser.OFPMatch()
        actions = [parser.OFPActionOutput(ofproto.OFPP_CONTROLLER, ofproto.OFPCML_NO_BUFFER)]
        self.add_flow(datapath, 0, match, actions)
        self.datapaths[datapath.id] = datapath
        print(f"Switch features handler: Added default flow for datapath {datapath.id}")

    def add_flow(self, datapath, priority, match, actions):
        ofproto = datapath.ofproto
        parser = datapath.ofproto_parser

        inst = [parser.OFPInstructionActions(ofproto.OFPIT_APPLY_ACTIONS, actions)]
        mod = parser.OFPFlowMod(datapath=datapath, priority=priority, match=match, instructions=inst)
        datapath.send_msg(mod)
        print(f"Added flow: datapath {datapath.id}, priority {priority}")

    @set_ev_cls(ofp_event.EventOFPPacketIn, MAIN_DISPATCHER)
    def packet_in_handler(self, ev):
        msg = ev.msg
        datapath = msg.datapath
        ofproto = datapath.ofproto
        parser = datapath.ofproto_parser
        in_port = msg.match['in_port']

        pkt = packet.Packet(msg.data)
        eth = pkt.get_protocol(ethernet.ethernet)

        if eth.ethertype == 0x88cc:
            return  # Ignore LLDP packets for topology discovery

        ip_pkt = pkt.get_protocol(ipv4.ipv4)
        if not ip_pkt:
            return

        src = eth.src
        dst = eth.dst

        dpid = datapath.id
        self.mac_to_port.setdefault(dpid, {})

        self.logger.info("Packet in %s %s %s %s", dpid, src, dst, in_port)

        # learn a mac address to avoid FLOOD next time.
        self.mac_to_port[dpid][src] = in_port

        # Trust-based routing decision with Q-learning
        path = self.get_best_path(src, dst)

        if len(path) < 2:
            self.logger.warning("Invalid path selected: %s", path)
            return

        # Install flow entries for the best path
        for i in range(len(path) - 1):
            src_dp = self.datapaths.get(path[i])
            dst_dp = self.datapaths.get(path[i + 1])
            if src_dp is None or dst_dp is None:
                self.logger.warning("One of the datapaths is not registered: %s -> %s", path[i], path[i + 1])
                return
            out_port = self.get_out_port(src_dp, dst_dp)
            actions = [parser.OFPActionOutput(out_port)]
            match = parser.OFPMatch(in_port=in_port, eth_dst=eth.dst)
            self.add_flow(src_dp, 1, match, actions)

        out_port = self.get_out_port(datapath, dst)
        actions = [parser.OFPActionOutput(out_port)]
        out = parser.OFPPacketOut(datapath=datapath, buffer_id=msg.buffer_id, in_port=in_port, actions=actions, data=msg.data)
        datapath.send_msg(out)

    def get_best_path(self, src, dst):
        if src not in self.q_values:
            self.q_values[src] = {}
        if dst not in self.q_values[src]:
            self.q_values[src][dst] = random.random()

        # Exploration vs Exploitation
        if random.uniform(0, 1) < self.exploration_rate:
            # Exploration: Select a random path
            path = [src] + random.sample(list(self.datapaths.keys()), min(random.randint(1, 3), len(self.datapaths))) + [dst]
        else:
            if self.q_values[src]:
                best_q_value = max(self.q_values[src].values())
                best_paths = [k for k, v in self.q_values[src].items() if v == best_q_value]
                path = [src] + best_paths + [dst]
            else:
                path = [src, dst]
        return path

    def update_q_table(self, src, dst, reward):
        old_value = self.q_values[src].get(dst, 0)
        self.q_values[src][dst] = old_value + self.learning_rate * (reward + self.discount_factor * max(self.q_values.get(dst, {}).values(), default=0) - old_value)
        print(f"Updated Q-value for path {src} -> {dst}: {self.q_values[src][dst]}")

    def update_trust(self, node, success_rate):
        self.trust_values[node] = 0.9 * self.trust_values.get(node, 1.0) + 0.1 * success_rate
        print(f"Updated trust value for node {node}: {self.trust_values[node]}")

    def get_out_port(self, src_dp, dst_dp):
        return 1  # Replace with logic for correct port

    def _monitor(self):
        while True:
            for dp in self.datapaths.values():
                self.request_stats(dp)
            hub.sleep(10)

    def request_stats(self, datapath):
        self.logger.debug('Send stats request: %016x', datapath.id)
        ofproto = datapath.ofproto
        parser = datapath.ofproto_parser

        req = parser.OFPFlowStatsRequest(datapath)
        datapath.send_msg(req)

    @set_ev_cls(ofp_event.EventOFPFlowStatsReply, MAIN_DISPATCHER)
    def flow_stats_reply_handler(self, ev):
        body = ev.msg.body
        if not body:
            self.logger.warning("No flow stats received.")
            return

        total_packets = 0
        total_bytes = 0
        total_duration = 0

        for stat in body:
            if stat.packet_count is not None and stat.byte_count is not None:
                total_packets += stat.packet_count
                total_bytes += stat.byte_count
                total_duration += stat.duration_sec

        if total_duration > 0:
            self.calculate_network_parameters(total_packets, total_bytes, total_duration)
        else:
            self.logger.warning("Total duration is 0, cannot calculate parameters.")

    def calculate_network_parameters(self, total_packets, total_bytes, total_duration):
        throughput = total_bytes / total_duration if total_duration > 0 else 0
        efficiency = total_packets / total_duration if total_duration > 0 else 0
        packet_delivery_ratio = total_packets / (total_packets + 10)

        self.logger.info("Network Throughput: %s bytes/sec", throughput)
        self.logger.info("Network Efficiency: %s packets/sec", efficiency)
        self.logger.info("Packet Delivery Ratio: %s", packet_delivery_ratio)

    @set_ev_cls(ofp_event.EventOFPStateChange, [MAIN_DISPATCHER, CONFIG_DISPATCHER])
    def _state_change_handler(self, ev):
        datapath = ev.datapath
        if ev.state == MAIN_DISPATCHER:
            if datapath.id not in self.datapaths:
                self.logger.info('Register datapath: %016x', datapath.id)
                self.datapaths[datapath.id] = datapath
                print(f"Datapath {datapath.id} registered")
        elif ev.state == CONFIG_DISPATCHER:
            if datapath.id in self.datapaths:
                self.logger.info('Unregister datapath: %016x', datapath.id)
                del self.datapaths[datapath.id]
                print(f"Datapath {datapath.id} unregistered")

print("Starting OSKEN script")
print("OS-KEN Run completed")
