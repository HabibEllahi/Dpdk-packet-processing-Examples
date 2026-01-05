from scapy.all import *
import time

def send_custom_icmp(dst_ip, packet_id, ttl, payload):
    ip = IP(src="192.168.1.20", dst=dst_ip, id=packet_id, ttl=ttl)
    icmp = ICMP(type=8, code=0)
    packet = ip / icmp / payload
    send(packet, verbose=0)
    print(f"Sent ICMP: ID={packet_id}, TTL={ttl}, Payload={payload}")

dst_ip = "192.168.1.5"

test_cases = [
    {'id': 484, 'ttl': 109, 'payload': 'bbb'},
    {'id': 446, 'ttl': 85, 'payload': 'aaa'},
    {'id': 843, 'ttl': 98, 'payload': 'test'},
    {'id': 724, 'ttl': 64, 'payload': 'a' * 70},
    {'id': 594, 'ttl': 25, 'payload': 'aaa'},
]

for case in test_cases:
    send_custom_icmp(dst_ip,
                     case["id"],
                     case["ttl"],
                     case["payload"])
    time.sleep(0.05)
