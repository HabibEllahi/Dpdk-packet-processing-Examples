static __rte_noreturn void lcore_main(void) {
    struct rte_mbuf *bufs[BURST_SIZE];

    while (1) {
        const uint16_t nb_rx = rte_eth_rx_burst(port_id, 0, bufs, BURST_SIZE);
        if (nb_rx == 0)
            continue;

        for (int i = 0; i < nb_rx; i++) {
            struct rte_mbuf *mbuf = bufs[i];
            struct rte_ether_hdr *eth_hdr = rte_pktmbuf_mtod(mbuf, struct rte_ether_hdr *);

            if (eth_hdr->ether_type != rte_cpu_to_be_16(RTE_ETHER_TYPE_IPV4)) {
                rte_pktmbuf_free(mbuf);
                continue;
            }

            struct rte_ipv4_hdr *ipv4_hdr = (struct rte_ipv4_hdr *)(eth_hdr + 1);

            if (ipv4_hdr->next_proto_id != IPPROTO_ICMP) {
                rte_pktmbuf_free(mbuf);
                continue;
            }

            struct rte_icmp_hdr *icmp_hdr = (struct rte_icmp_hdr *)((unsigned char *)ipv4_hdr + sizeof(struct rte_ipv4_hdr));

            if (icmp_hdr->icmp_type != RTE_IP_ICMP_ECHO_REQUEST) {
                rte_pktmbuf_free(mbuf);
                continue;
            }

            // LOG
            printf("📩 Ping recibido desde %d.%d.%d.%d\n",
                (ipv4_hdr->src_addr) & 0xFF,
                (ipv4_hdr->src_addr >> 8) & 0xFF,
                (ipv4_hdr->src_addr >> 16) & 0xFF,
                (ipv4_hdr->src_addr >> 24) & 0xFF);

            ipv4_hdr->time_to_live--;

            // Intercambio de MACs
            struct rte_ether_addr tmp_mac;
            rte_ether_addr_copy(&eth_hdr->src_addr, &tmp_mac);
            rte_ether_addr_copy(&eth_hdr->dst_addr, &eth_hdr->src_addr);
            rte_ether_addr_copy(&tmp_mac, &eth_hdr->dst_addr);

            // Intercambio de IPs
            uint32_t tmp_ip = ipv4_hdr->src_addr;
            ipv4_hdr->src_addr = ipv4_hdr->dst_addr;
            ipv4_hdr->dst_addr = tmp_ip;

            // Cambia el tipo ICMP de echo request a echo reply
            icmp_hdr->icmp_type = RTE_IP_ICMP_ECHO_REPLY;
            icmp_hdr->icmp_cksum = 0;
            icmp_hdr->icmp_cksum = rte_raw_cksum(icmp_hdr, rte_be_to_cpu_16(ipv4_hdr->total_length) - sizeof(struct rte_ipv4_hdr));
            icmp_hdr->icmp_cksum = (icmp_hdr->icmp_cksum == 0x0000) ? 0xffff : icmp_hdr->icmp_cksum;

            // Recalculo del checksum
            ipv4_hdr->hdr_checksum = 0;
            ipv4_hdr->hdr_checksum = rte_ipv4_cksum(ipv4_hdr);

            // Enviamos el paquete de vuelta
            const uint16_t nb_tx = rte_eth_tx_burst(port_id, 0, &mbuf, 1);
            if (nb_tx < 1) {
                printf("⚠️  No se pudo reenviar el ping\n");
                rte_pktmbuf_free(mbuf);
            } else {
                printf("✅ Ping respondido\n");
            }
        }
    }
}
