# Dpdk Based Firewall

This repository contains the source code developed for the Bachelor's Thesis 
focused on the use of **DPDK (Data Plane Development Kit)** to implement a 
high-performance packet forwarding and firewall solution.

The project evaluates the behavior and performance of a DPDK-based network function
under different scenarios, comparing basic packet forwarding with rule-based packet
filtering and modification.

---

## 📌 Project Overview

The proposed solution is based on a three-node topology:

- **Host A**: Traffic generator (ICMP and background traffic)
- **Host B**: DPDK-based router/firewall (main system)
- **Host C**: Traffic receiver and responder

The DPDK application running on Host B operates entirely in user space and performs
packet inspection, forwarding, filtering, and modification directly in the data plane.

---

## 🧪 Implemented Scenarios

### 🔹 Scenario 2 – ICMP Packet Forwarding

File: src/icmp_forwarding.c

This scenario implements a basic packet forwarding mechanism using DPDK.
The application performs:

- Reception of ICMP packets from Host A
- Forwarding to Host C based on destination IP
- Reverse forwarding of ICMP replies back to Host A
- Direct ICMP echo replies when packets are addressed to the firewall itself

This scenario serves as a baseline to evaluate the performance impact of packet
processing without filtering rules.

---

### 🔹 Scenario 3 – DPDK Firewall with Packet Filtering

File: src/dpdk_firewall.c


This scenario extends the previous one by introducing firewall capabilities.
The application applies multiple filtering rules to ICMP packets, including:

- Packet ID parity checks
- Payload length thresholds
- Payload content inspection
- Time-To-Live (TTL) validation

Additionally, selective packet modification is implemented, where specific ICMP
payloads are altered and corresponding checksums are recalculated.

---

## 🚀 Traffic Generation

To validate the firewall rules, a custom ICMP traffic generator was implemented
using **Scapy**.

File: traffic_generator/icmp_generator.py


This script (not complete - not the same as used in the practical)sends ICMP Echo 
Request packets with controlled parameters such as packet ID, TTL, and payload content, 
allowing systematic testing of the filtering logic implemented in the DPDK application.

---

## 🛠️ Requirements

- Linux-based system
- DPDK (configured with hugepages and compatible NIC)
- Python 3 with Scapy installed
- Two network interfaces available for DPDK usage

---

## 📚 Academic Context

This code was developed exclusively for academic purposes as part of a Bachelor's
Thesis in Telecommunications Engineering. The implementation focuses on correctness,
clarity, and experimental evaluation rather than production deployment.

---

## 📄 License

This project is released for academic and educational use.

