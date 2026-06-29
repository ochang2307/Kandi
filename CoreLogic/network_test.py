from network import make_device, Network, make_packet

# Build a line: 1 <-> 2 <-> 3
# 1 and 3 are NOT directly connected. 2 is the only bridge.
net = Network()
net.add_device(make_device(1, group_id=1))
net.add_device(make_device(2, group_id=1))
net.add_device(make_device(3, group_id=1))
net.connect(1, 2)
net.connect(2, 3)

# Device 1 broadcasts a position packet with enough hops to cross the line.
pkt = make_packet(sender_id=1, packet_id=100, hop_limit=3,
                  msg_type="position", payload={"lat": 25.03, "lon": 121.56},
                  group_id=1)
net.originate(1, pkt)

# Print what each device did.
print("--- Event log ---")
for entry in net.log:
    print(entry)

# The proof: device 3 should appear in the log having received the packet,
# even though it can't hear device 1 directly.
got_to_3 = any(entry[0] == 3 and "processed" in entry[1] for entry in net.log)
print("\nPacket reached device 3 (via relay):", got_to_3, "(expect True)")

# --- Test: a LOOP topology, to prove dedup stops infinite rebroadcast ---
# Triangle: 1<->2, 2<->3, 3<->1. Without dedup, a packet would circle forever.
net2 = Network()
for i in (1, 2, 3):
    net2.add_device(make_device(i, group_id=1))
net2.connect(1, 2)
net2.connect(2, 3)
net2.connect(3, 1)

pkt2 = make_packet(1, 200, 5, "position", {}, group_id=1)
net2.originate(1, pkt2)
print("\n--- Loop topology log ---")
for entry in net2.log:
    print(entry)
# Each device should process the packet exactly ONCE, then drop duplicates.
# If it terminates (the program doesn't hang), dedup is working.

# --- Test: hop limit too small to cross the line ---
net3 = Network()
for i in (1, 2, 3, 4):
    net3.add_device(make_device(i, group_id=1))
net3.connect(1, 2); net3.connect(2, 3); net3.connect(3, 4)   # line of 4

# hop_limit 1: can reach device 2, but should DIE before reaching 4.
pkt3 = make_packet(1, 300, 1, "position", {}, group_id=1)
net3.originate(1, pkt3)
reached_4 = any(e[0] == 4 and "processed" in e[1] for e in net3.log)
print("\nWith hop_limit=1, reached device 4:", reached_4, "(expect False)")