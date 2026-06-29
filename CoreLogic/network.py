from mesh import make_packet, has_seen, mark_seen, handle_packet


# ---------------------------------------------------------------------------
# A "Device" in the simulation is just: an id, a group, and its own seen-cache.
# (Each real device has its own duplicate-suppression memory, so each sim
#  device gets its own set.)
# ---------------------------------------------------------------------------
def make_device(device_id, group_id):
    return {"id": device_id, "group_id": group_id, "seen": set()}


# ---------------------------------------------------------------------------
# The Network holds:
#   - all the devices, keyed by id
#   - the "who can hear whom" map (adjacency): for each device id, the list of
#     device ids within radio range of it.
#
# This adjacency is what models the festival: a device can only directly hear
# its neighbors, NOT everyone. That's the whole reason relaying has to exist.
# ---------------------------------------------------------------------------
class Network:
    def __init__(self):
        self.devices = {}      # id -> device dict
        self.links = {}        # id -> list of neighbor ids
        self.log = []          # record of what happened, for inspection

    def add_device(self, device):
        self.devices[device["id"]] = device
        if device["id"] not in self.links:
            self.links[device["id"]] = []

    def connect(self, id_a, id_b):
        # bidirectional radio link: if A can hear B, B can hear A
        self.links[id_a].append(id_b)
        self.links[id_b].append(id_a)

    # -----------------------------------------------------------------------
    # deliver(): hand a packet to ONE device and let it react.
    # The device runs your handle_packet. If handle_packet decides to relay,
    # we then deliver the (decremented) packet to that device's neighbors.
    #
    # This is the recursive heart of flooding: a relay triggers more
    # deliveries, which may trigger more relays, until packets run out of
    # hops or everyone has already seen them (dedup stops the loops).
    # -----------------------------------------------------------------------
    def deliver(self, to_id, packet):
        device = self.devices[to_id]

        # Run the per-device receive logic you wrote.
        # We need to know if it relayed AND get the decremented packet, so we
        # reproduce handle_packet's relay step here at the network level.
        # (handle_packet still owns the decision; the network owns the spread.)

        if packet["group_id"] != device["group_id"]:
            self.log.append((to_id, "dropped_foreign", packet["packet_id"]))
            return

        if has_seen(device["seen"], packet):
            self.log.append((to_id, "dropped_duplicate", packet["packet_id"]))
            return

        mark_seen(device["seen"], packet)

        if packet["hop_limit"] <= 0:
            self.log.append((to_id, "processed_no_relay", packet["packet_id"]))
            return

        # It relays: build the decremented packet and flood to neighbors.
        self.log.append((to_id, "processed_and_relayed", packet["packet_id"]))
        new_packet = packet.copy()
        new_packet["hop_limit"] = packet["hop_limit"] - 1
        for neighbor_id in self.links[to_id]:
            self.deliver(neighbor_id, new_packet)

    # -----------------------------------------------------------------------
    # originate(): a device creates a brand-new packet and sends it out.
    # This is the entry point — e.g. "device 1 broadcasts its position."
    # The originating device floods to its own neighbors to start the spread.
    # -----------------------------------------------------------------------
    def originate(self, from_id, packet):
        device = self.devices[from_id]
        # The originator marks its own packet as seen (so if it comes back
        # via a relay, it won't re-handle it) and floods to neighbors.
        mark_seen(device["seen"], packet)
        self.log.append((from_id, "originated", packet["packet_id"]))
        for neighbor_id in self.links[from_id]:
            self.deliver(neighbor_id, packet)