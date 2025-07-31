/* SPDX-License-Identifier: BSD-3-Clause
 * Copyright(c) 2025 Zte Corporation
 *
 * Copyright(c) 2019 Intel Corporation
 *
 * Derived do_macswap implementation from app/test-pmd/macswap_sse.h
 */

#ifndef _MACSWAP_RVV_H_
#define _MACSWAP_RVV_H_

#include "macswap_common.h"
#include <riscv_vector.h>

static inline void
do_macswap(struct rte_mbuf *pkts[], uint16_t nb,
           struct rte_port *txp)
{
    struct rte_ether_hdr *eth_hdr[4];
    struct rte_mbuf *mb[4];
    uint64_t ol_flags;
    int i;
    int r;
    register vuint8m1_t addr0, addr1, addr2, addr3;

    /**
     * shuffle mask be used to shuffle the 16 bytes.
     * byte 0-5 will be swapped with byte 6-11.
     * byte 12-15 will keep unchanged.
     */
    uint8_t data[16] = {6, 7, 8, 9, 10, 11, 0, 1, 2, 3, 4, 5, 12, 13, 14, 15};
    size_t vl = __riscv_vsetvl_e8m1(sizeof(data));
    register const vuint8m1_t shfl_msk = __riscv_vle8_v_u8m1(data, vl);

    ol_flags = ol_flags_init(txp->dev_conf.txmode.offloads);
    vlan_qinq_set(pkts, nb, ol_flags, txp->tx_vlan_id, txp->tx_vlan_id_outer);

    i = 0;
    r = nb;

    while (r >= 4) {
        if (r >= 8) {
            rte_prefetch0(rte_pktmbuf_mtod(pkts[i + 4], void *));
            rte_prefetch0(rte_pktmbuf_mtod(pkts[i + 5], void *));
            rte_prefetch0(rte_pktmbuf_mtod(pkts[i + 6], void *));
            rte_prefetch0(rte_pktmbuf_mtod(pkts[i + 7], void *));
        }

        mb[0] = pkts[i++];
        eth_hdr[0] = rte_pktmbuf_mtod(mb[0], struct rte_ether_hdr *);
        addr0 = __riscv_vle8_v_u8m1((uint8_t *)eth_hdr[0], vl);
        mbuf_field_set(mb[0], ol_flags);

        mb[1] = pkts[i++];
        eth_hdr[1] = rte_pktmbuf_mtod(mb[1], struct rte_ether_hdr *);
        addr1 = __riscv_vle8_v_u8m1((uint8_t *)eth_hdr[1], vl);
        mbuf_field_set(mb[1], ol_flags);

        addr0 = __riscv_vrgather_vv_u8m1(addr0, shfl_msk, vl);

        mb[2] = pkts[i++];
        eth_hdr[2] = rte_pktmbuf_mtod(mb[2], struct rte_ether_hdr *);
        addr2 = __riscv_vle8_v_u8m1((uint8_t *)eth_hdr[2], vl);
        mbuf_field_set(mb[2], ol_flags);

        addr1 = __riscv_vrgather_vv_u8m1(addr1, shfl_msk, vl);
        __riscv_vse8_v_u8m1((uint8_t *)eth_hdr[0], addr0, vl);


        mb[3] = pkts[i++];
        eth_hdr[3] = rte_pktmbuf_mtod(mb[3], struct rte_ether_hdr *);
        addr3 = __riscv_vle8_v_u8m1((uint8_t *)eth_hdr[3], vl);
        mbuf_field_set(mb[3], ol_flags);

        addr2 = __riscv_vrgather_vv_u8m1(addr2, shfl_msk, vl);
        __riscv_vse8_v_u8m1((uint8_t *)eth_hdr[1], addr1, vl);

        addr3 = __riscv_vrgather_vv_u8m1(addr3, shfl_msk, vl);
        __riscv_vse8_v_u8m1((uint8_t *)eth_hdr[2], addr2, vl);
        __riscv_vse8_v_u8m1((uint8_t *)eth_hdr[3], addr3, vl);

        r -= 4;
    }

    for ( ; i < nb; i++) {
        if (i < nb - 1)
            rte_prefetch0(rte_pktmbuf_mtod(pkts[i+1], void *));
        mb[0] = pkts[i];
        eth_hdr[0] = rte_pktmbuf_mtod(mb[0], struct rte_ether_hdr *);

        /* Swap dest and src mac addresses. */
        addr0 = __riscv_vle8_v_u8m1((uint8_t *)eth_hdr[0], vl);
        mbuf_field_set(mb[0], ol_flags);

        addr0 = __riscv_vrgather_vv_u8m1(addr0, shfl_msk, vl);
        __riscv_vse8_v_u8m1((uint8_t *)eth_hdr[0], addr0, vl);
    }
}

#endif /* _MACSWAP_RVV_H_ */
