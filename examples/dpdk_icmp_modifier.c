#include <rte_eal.h>
#include <rte_ethdev.h>
#include <rte_mbuf.h>
#include <rte_ip.h>
#include <rte_icmp.h>
#include <stdio.h>

#define RX_RING_SIZE 1024
#define TX_RING_SIZE 1024
#define NUM_MBUFS 8192
#define MBUF_CACHE_SIZE 250

#define BURST_SIZE 32

static const uint16_t port_id = 0;  

static struct rte_mempool *mbuf_pool;

static int port_init(uint16_t port) {
    struct rte_eth_conf port_conf = { 0 };
    struct rte_eth_dev_info dev_info;
    
    if (!rte_eth_dev_is_valid_port(port))
        return -1;

    rte_eth_dev_info_get(port, &dev_info);
    rte_eth_dev_configure(port, 1, 1, &port_conf);
    rte_eth_rx_queue_setup(port, 0, RX_RING_SIZE, rte_eth_dev_socket_id(port), NULL, mbuf_pool);
    rte_eth_tx_queue_setup(port, 0, TX_RING_SIZE, rte_eth_dev_socket_id(port), NULL);
    
    rte_eth_dev_start(port);
    rte_eth_promiscuous_enable(port);
    return 0;
}

// Captura y modifica paquetes ICMP
static void process_packets(void) {
    struct rte_mbuf *bufs[BURST_SIZE];
    uint16_t nb_rx;
    
    while (1) {
        nb_rx = rte_eth_rx_burst(port_id, 0, bufs, BURST_SIZE);
        if (nb_rx == 0) continue;
        
        for (uint16_t i = 0; i < nb_rx; i++) {
            struct rte_mbuf *buf = bufs[i];
            struct rte_ether_hdr *eth_hdr = rte_pktmbuf_mtod(buf, struct rte_ether_hdr *);
            
            // Verifica si el paquete es IP
            if (eth_hdr->ether_type == rte_cpu_to_be_16(RTE_ETHER_TYPE_IPV4)) {
                struct rte_ipv4_hdr *ip_hdr = (struct rte_ipv4_hdr *)(eth_hdr + 1);

                // Verifica si el paquete es ICMP
                if (ip_hdr->next_proto_id == IPPROTO_ICMP) {
                    struct rte_icmp_hdr *icmp_hdr = (struct rte_icmp_hdr *)(ip_hdr + 1);

                    // Modifica el identificador ICMP
                    icmp_hdr->icmp_ident = rte_cpu_to_be_16(12345);
                    
                    // Recalcula el checksum
                    icmp_hdr->icmp_cksum = 0;
                    icmp_hdr->icmp_cksum = rte_raw_cksum(icmp_hdr, sizeof(struct rte_icmp_hdr));

                    // Intercambia las direcciones IP para responder al emisor
                    uint32_t tmp_ip = ip_hdr->src_addr;
                    ip_hdr->src_addr = ip_hdr->dst_addr;
                    ip_hdr->dst_addr = tmp_ip;

                    // Envía el paquete de vuelta
                    rte_eth_tx_burst(port_id, 0, &buf, 1);
                }
            }
            
            rte_pktmbuf_free(buf);
        }
    }
}

// Función principal
int main(int argc, char *argv[]) {
    rte_eal_init(argc, argv);
    mbuf_pool = rte_pktmbuf_pool_create("MBUF_POOL", NUM_MBUFS, MBUF_CACHE_SIZE, 0, RTE_MBUF_DEFAULT_BUF_SIZE, rte_socket_id());

    if (port_init(port_id) != 0) {
        rte_exit(EXIT_FAILURE, "Error al inicializar el puerto\n");
    }

    process_packets();
    return 0;
}
