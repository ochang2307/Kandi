from mesh import make_packet, has_seen, mark_seen, handle_packet, transmit

device = {"id": 1, "group_id": 1}
seen = set()

# Foreign group -> dropped
foreign = make_packet(9, 1, 3, "position", {}, group_id=99)
print(handle_packet(device, seen, foreign), "(expect dropped_foreign)")

# New, mine, hops left -> relayed (watch for TX print + hop_limit now 2)
p = make_packet(3, 10, 3, "position", {}, group_id=1)
print(handle_packet(device, seen, p), "(expect processed_and_relayed)")

# Same packet again -> duplicate
print(handle_packet(device, seen, p), "(expect dropped_duplicate)")

# New, mine, but no hops -> processed, not relayed
done = make_packet(4, 20, 0, "position", {}, group_id=1)
print(handle_packet(device, seen, done), "(expect processed_no_relay)")