#This file is missing the actual decryption/encryption/transmission process 

def make_packet(sender_id, packet_id, hop_limit, msg_type, payload, group_id):
    packet = {"sender_id": sender_id, "packet_id": packet_id, "hop_limit": hop_limit,
              "msg_type": msg_type, "payload": payload, "group_id": group_id}
    return packet

def has_seen(seen_set, packet):
    if ((packet["sender_id"], packet["packet_id"])) in seen_set:
         return True
    else:
        return False
    

def mark_seen(seen_set, packet):
    seen_set.add((packet["sender_id"], packet["packet_id"]))


def transmit(packet): #this is a PLACEHOLDER
    print("TX:", packet)

def handle_packet(device, seen_set, packet):
    if packet["group_id"] != device["group_id"]:
        return "dropped_foreign"
    if has_seen(seen_set,packet):
        return "dropped_duplicate"
    mark_seen(seen_set,packet)
    if packet["hop_limit"] <= 0:
        return "processed_no_relay"
    new_packet = packet.copy()
    new_packet["hop_limit"] = packet["hop_limit"] - 1
    transmit(new_packet)
    return "processed_and_relayed"