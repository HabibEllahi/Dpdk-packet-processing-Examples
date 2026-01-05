#include <rte_eal.h>
#include <rte_ethdev.h>
#include <rte_mbuf.h>
#include <rte_ip.h>
#include <stdio.h>

#define RX_RING_SIZE 128
#define TX_RING_SIZE 512
#define NUM_MBUFS 8191
#define MBUF_CACHE_SIZE 250
#define BURST_SIZE 32

static const uint16_t port_id = 0;

static void dpdk_main_loop(void) {
    struct rte_mbuf *bufs[BURST_SIZE];

    while (1) {
        uint16_t nb_rx = rte_eth_rx_burst(port_id, 0, bufs, BURST_SIZE);
        if (nb_rx == 0)
            continue;

        for (int i = 0; i < nb_rx; i++) {
            struct rte_mbuf *mbuf = bufs[i];
            struct rte_ether_hdr *eth_hdr = rte_pktmbuf_mtod(mbuf, struct rte_ether_hdr *);

            if (eth_hdr->ether_type == rte_cpu_to_be_16(RTE_ETHER_TYPE_IPV4)) {
                struct rte_ipv4_hdr *ip_hdr = (struct rte_ipv4_hdr *)(eth_hdr + 1);

                // modificamos el TTL
                ip_hdr->time_to_live = 42;

                // Recalculamos checksum
                ip_hdr->hdr_checksum = 0;
                ip_hdr->hdr_checksum = rte_ipv4_cksum(ip_hdr);
            }

            rte_eth_tx_burst(port_id, 0, &mbuf, 1);
        }
    }
}

int main(int argc, char *argv[]) {
    int ret = rte_eal_init(argc, argv);
    if (ret < 0)
        rte_exit(EXIT_FAILURE, "Error en rte_eal_init\n");

    argc -= ret;
    argv += ret;

    struct rte_mempool *mbuf_pool = rte_pktmbuf_pool_create("MBUF_POOL",
        NUM_MBUFS, MBUF_CACHE_SIZE, 0, RTE_MBUF_DEFAULT_BUF_SIZE, rte_socket_id());

    if (mbuf_pool == NULL)
        rte_exit(EXIT_FAILURE, "No se pudo crear el mbuf pool\n");

    struct rte_eth_conf port_conf = {0};
    ret = rte_eth_dev_configure(port_id, 1, 1, &port_conf);
    if (ret < 0)
        rte_exit(EXIT_FAILURE, "Error al configurar el puerto\n");

    ret = rte_eth_rx_queue_setup(port_id, 0, RX_RING_SIZE, rte_eth_dev_socket_id(port_id), NULL, mbuf_pool);
    if (ret < 0)
        rte_exit(EXIT_FAILURE, "Error en rx_queue_setup\n");

    ret = rte_eth_tx_queue_setup(port_id, 0, TX_RING_SIZE, rte_eth_dev_socket_id(port_id), NULL);
    if (ret < 0)
        rte_exit(EXIT_FAILURE, "Error en tx_queue_setup\n");

    ret = rte_eth_dev_start(port_id);
    if (ret < 0)
        rte_exit(EXIT_FAILURE, "Error al iniciar el puerto\n");

    rte_eth_promiscuous_enable(port_id);
    printf("Puerto %u iniciado en modo promiscuo\n", port_id);

    dpdk_main_loop();

    return 0;
}
