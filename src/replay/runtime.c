/*-
 *   BSD LICENSE
 *
 *   Copyright(c) 2010-2014 Intel Corporation. All rights reserved.
 *   All rights reserved.
 *
 *   Redistribution and use in source and binary forms, with or without
 *   modification, are permitted provided that the following conditions
 *   are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in
 *       the documentation and/or other materials provided with the
 *       distribution.
 *     * Neither the name of Intel Corporation nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 *   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *   "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *   A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 *   OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *   SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 *   LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *   DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *   THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *   (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *   OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <errno.h>
#include <getopt.h>
#include <inttypes.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/queue.h>
#include <sys/types.h>

#include <rte_atomic.h>
#include <rte_branch_prediction.h>
#include <rte_byteorder.h>
#include <rte_common.h>
#include <rte_cycles.h>
#include <rte_debug.h>
#include <rte_eal.h>
#include <rte_ethdev.h>
#include <rte_ether.h>
#include <rte_interrupts.h>
#include <rte_ip.h>
#include <rte_launch.h>
#include <rte_lcore.h>
#include <rte_log.h>
#include <rte_lpm.h>
#include <rte_mbuf.h>
#include <rte_memcpy.h>
#include <rte_memory.h>
#include <rte_mempool.h>
#include <rte_memzone.h>
#include <rte_pci.h>
#include <rte_per_lcore.h>
#include <rte_per_lcore.h>
#include <rte_prefetch.h>
#include <rte_random.h>
#include <rte_ring.h>
#include <rte_tailq.h>
#include <rte_tcp.h>

#include "replay.h"

#ifndef REPLAY_LCORE_IO_FLUSH
#define REPLAY_LCORE_IO_FLUSH 1000000
#endif

#ifndef REPLAY_STATS
#define REPLAY_STATS 1000000
#endif

#define REPLAY_IO_RX_DROP_ALL_PACKETS 1
#define REPLAY_IO_TX_DROP_ALL_PACKETS 0

#ifndef REPLAY_IO_RX_PREFETCH_ENABLE
#define REPLAY_IO_RX_PREFETCH_ENABLE 1
#endif

#ifndef REPLAY_IO_TX_PREFETCH_ENABLE
#define REPLAY_IO_TX_PREFETCH_ENABLE 1
#endif

#if REPLAY_IO_RX_PREFETCH_ENABLE
#define REPLAY_IO_RX_PREFETCH0(p) rte_prefetch0 (p)
#define REPLAY_IO_RX_PREFETCH1(p) rte_prefetch1 (p)
#else
#define REPLAY_IO_RX_PREFETCH0(p)
#define REPLAY_IO_RX_PREFETCH1(p)
#endif

#if REPLAY_IO_TX_PREFETCH_ENABLE
#define REPLAY_IO_TX_PREFETCH0(p) rte_prefetch0 (p)
#define REPLAY_IO_TX_PREFETCH1(p) rte_prefetch1 (p)
#else
#define REPLAY_IO_TX_PREFETCH0(p)
#define REPLAY_IO_TX_PREFETCH1(p)
#endif

//#define QUEUE_STATS

static inline void replay_lcore_io_rx (struct replay_lcore_params_io *lp, uint32_t bsz_rd) {
	uint32_t i;

	for (i = 0; i < lp->rx.n_nic_queues; i++) {
		uint8_t port  = lp->rx.nic_queues[i].port;
		uint8_t queue = lp->rx.nic_queues[i].queue;
		uint32_t n_mbufs, j;

		n_mbufs = rte_eth_rx_burst (port, queue, lp->rx.mbuf_in.array, (uint16_t)bsz_rd);

		if (unlikely (n_mbufs == 0)) {
			continue;
		}

		// Drop all packets
		for (j = 0; j < n_mbufs; j++) {
			struct rte_mbuf *pkt = lp->rx.mbuf_in.array[j];
			rte_pktmbuf_free (pkt);
		}
	}
}

static inline void replay_lcore_io_tx (struct replay_lcore_params_io *lp, uint32_t bsz_tx_wr) {
	uint32_t i;

	for (i = 0; i < lp->tx.n_nic_queues; i++) {
		uint8_t port  = lp->tx.nic_queues[i].port;
		uint8_t queue = lp->tx.nic_queues[i].queue;
		uint32_t n_mbufs, n_pkts;

		n_mbufs = bsz_tx_wr;

		struct rte_mbuf *tmpbuf = rte_ctrlmbuf_alloc (replay.pools[0]);
		if (!tmpbuf) {
			continue;
		}

		tmpbuf->pkt_len  = 0;  // TODO
		tmpbuf->data_len = 0;  // TODO
		tmpbuf->port     = port;

		//memcpy (rte_ctrlmbuf_data (tmpbuf), icmppkt, icmppktlen - 8);
		n_pkts = rte_eth_tx_burst (port, queue, &tmpbuf, n_mbufs);

		if (unlikely (n_pkts < n_mbufs)) {
			while (n_pkts < n_mbufs) {
				rte_ctrlmbuf_free (tmpbuf);
				n_mbufs--;
			}
		} else {
			lp->tx.mbuf_out[port].n_mbufs++;
		}
	}
}

static unsigned doloop = 1;

static void replay_lcore_main_loop_io (void) {
	uint32_t lcore                    = rte_lcore_id ();
	struct replay_lcore_params_io *lp = &replay.lcore_params[lcore].io;

	uint32_t bsz_rx_rd = replay.burst_size_io_rx_read;
	uint32_t bsz_tx_wr = replay.burst_size_io_tx_write;
	// uint32_t bsz_rx_wr = replay.burst_size_io_rx_write;

	// uint8_t pos_lb = replay.pos_lb;

	while (likely (doloop)) {
		if (unlikely (lp->rx.n_nic_queues > 0)) {
			replay_lcore_io_rx (lp, bsz_rx_rd);
		}

		if (likely (lp->tx.n_nic_queues > 0)) {
			replay_lcore_io_tx (lp, bsz_tx_wr);
		}
	}
}

int replay_lcore_main_loop (__attribute__ ((unused)) void *arg) {
	struct replay_lcore_params *lp;
	unsigned lcore;

	lcore = rte_lcore_id ();
	lp    = &replay.lcore_params[lcore];

	if (lp->type == e_REPLAY_LCORE_IO) {
		printf ("Logical core %u (I/O) main loop.\n", lcore);
		for (;;) {
			replay_lcore_main_loop_io ();
		}
	}

	return 0;
}
