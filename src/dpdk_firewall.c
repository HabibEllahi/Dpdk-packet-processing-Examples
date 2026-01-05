#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>
#include <string.h>

#include <rte_common.h>
#include <rte_eal.h>
#include <rte_ethdev.h>
#include <rte_mbuf.h>
#include <rte_ip.h>
#include <rte_icmp.h>
#include <rte_tcp.h>
#include <rte_udp.h>
#include <rte_ether.h>

/* Parámetros */
#define RX_RING_SIZE       1024
#define TX_RING_SIZE       1024
#define NUM_MBUFS          8191
#define MBUF_CACHE_SIZE    250
#define BURST_SIZE         32

#define IP_A "192.168.1.20"
#define IP_B "192.168.1.2"
#define IP_C "192.168.1.5"

/* MACs */
static struct rte_ether_addr mac_a = {{0x6c, 0x1f, 0xf7, 0x1b, 0x31, 0xa5}};
static struct rte_ether_addr mac_b = {{0xa0, 0x36, 0x9f, 0x2d, 0xea, 0x43}};
static struct rte_ether_addr mac_c = {{0x64, 0x00, 0x6a, 0x6b, 0x6d, 0xea}};

static uint32_t ip_a, ip_b, ip_c;

static inline uint32_t parse_ipv4(const char *ip_str)
{
    unsigned int b1, b2, b3, b4;
    sscanf(ip_str, "%u.%u.%u.%u", &b1, &b2, &b3, &b4);
    return (b1 << 24) | (b2 << 16) | (b3 << 8) | b4;
}

static void process_packet(struct rte_mbuf *mbuf, uint16_t port)
{
    struct rte_ether_hdr *eth_hdr =
        rte_pktmbuf_mtod(mbuf, struct rte_ether_hdr *);

    if (eth_hdr->ether_type != rte_cpu_to_be_16(RTE_ETHER_TYPE_IPV4)) {
        rte_pktmbuf_free(mbuf);
        return;
    }

    struct rte_ipv4_hdr *ip_hdr =
        (struct rte_ipv4_hdr *)(eth_hdr + 1);

    if (ip_hdr->next_proto_id == IPPROTO_ICMP) {

        struct rte_icmp_hdr *icmp_hdr =
            (struct rte_icmp_hdr *)((uint8_t *)ip_hdr +
            sizeof(struct rte_ipv4_hdr));

        char *payload = (char *)(icmp_hdr + 1);
        uint16_t payload_len =
            rte_be_to_cpu_16(ip_hdr->total_length) -
            sizeof(struct rte_ipv4_hdr) -
            sizeof(struct rte_icmp_hdr);

        /* Reglas de filtrado */
        if ((rte_be_to_cpu_16(ip_hdr->packet_id) % 2 != 0) ||
            (payload_len > 60) ||
            (strcmp(payload, "bbb") == 0) ||
            (ip_hdr->time_to_live < 32)) {

            rte_pktmbuf_free(mbuf);
            return;
        }

        /* Modificación del payload */
        if (strcmp(payload, "aaa") == 0) {
            strcpy(payload, "ccc");

            icmp_hdr->icmp_cksum = 0;
            uint16_t icmp_len =
                rte_be_to_cpu_16(ip_hdr->total_length) -
                sizeof(struct rte_ipv4_hdr);

            icmp_hdr->icmp_cksum =
                ~rte_raw_cksum(icmp_hdr, icmp_len);
        }

        /* Encaminamiento ICMP */
        if (ip_hdr->dst_addr == ip_c && port == 1) {
            rte_ether_addr_copy(&mac_c, &eth_hdr->dst_addr);
            rte_ether_addr_copy(&mac_b, &eth_hdr->src_addr);

            ip_hdr->hdr_checksum = 0;
            ip_hdr->hdr_checksum = rte_ipv4_cksum(ip_hdr);

            rte_eth_tx_burst(0, 0, &mbuf, 1);
        }
        else if (ip_hdr->src_addr == ip_c && port == 0) {
            rte_ether_addr_copy(&mac_a, &eth_hdr->dst_addr);
            rte_ether_addr_copy(&mac_b, &eth_hdr->src_addr);

            ip_hdr->hdr_checksum = 0;
            ip_hdr->hdr_checksum = rte_ipv4_cksum(ip_hdr);

            rte_eth_tx_burst(1, 0, &mbuf, 1);
        }
        else {
            rte_pktmbuf_free(mbuf);
        }
    }
    else {
        rte_pktmbuf_free(mbuf);
    }
}

int main(int argc, char *argv[])
{
    rte_eal_init(argc, argv);

    ip_a = rte_cpu_to_be_32(parse_ipv4(IP_A));
    ip_b = rte_cpu_to_be_32(parse_ipv4(IP_B));
    ip_c = rte_cpu_to_be_32(parse_ipv4(IP_C));

    struct rte_mempool *mbuf_pool =
        rte_pktmbuf_pool_create(
            "MBUF_POOL",
            NUM_MBUFS * 2,
            MBUF_CACHE_SIZE,
            0,
            RTE_MBUF_DEFAULT_BUF_SIZE,
            rte_socket_id());

    for (uint16_t portid = 0; portid < 2; portid++) {
        struct rte_eth_conf port_conf = {0};

        rte_eth_dev_configure(portid, 1, 1, &port_conf);
        rte_eth_rx_queue_setup(portid, 0, RX_RING_SIZE,
                                rte_eth_dev_socket_id(portid),
                                NULL, mbuf_pool);
        rte_eth_tx_queue_setup(portid, 0, TX_RING_SIZE,
                                rte_eth_dev_socket_id(portid),
                                NULL);
        rte_eth_dev_start(portid);
    }

    struct rte_mbuf *bufs[BURST_SIZE];

    while (1) {
        for (uint16_t portid = 0; portid < 2; portid++) {
            uint16_t nb_rx =
                rte_eth_rx_burst(portid, 0, bufs, BURST_SIZE);

            for (uint16_t i = 0; i < nb_rx; i++) {
                process_packet(bufs[i], portid);
            }
        }
    }
    return 0;
}
