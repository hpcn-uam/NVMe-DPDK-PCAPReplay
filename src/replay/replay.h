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

#ifndef _MAIN_H_
#define _MAIN_H_

#include <stdio.h>
#include "hptl.h"

/* Logical cores */
#ifndef REPLAY_MAX_SOCKETS
#define REPLAY_MAX_SOCKETS 2
#endif

#ifndef REPLAY_MAX_LCORES
#define REPLAY_MAX_LCORES RTE_MAX_LCORE
#endif

#ifndef REPLAY_MAX_NIC_PORTS
#define REPLAY_MAX_NIC_PORTS RTE_MAX_ETHPORTS
#endif

#ifndef REPLAY_MAX_RX_QUEUES_PER_NIC_PORT
#define REPLAY_MAX_RX_QUEUES_PER_NIC_PORT 128
#endif

#ifndef REPLAY_MAX_TX_QUEUES_PER_NIC_PORT
#define REPLAY_MAX_TX_QUEUES_PER_NIC_PORT 128
#endif

#ifndef REPLAY_MAX_IO_LCORES
#define REPLAY_MAX_IO_LCORES 16
#endif
#if (REPLAY_MAX_IO_LCORES > REPLAY_MAX_LCORES)
#error "REPLAY_MAX_IO_LCORES is too big"
#endif

#ifndef REPLAY_MAX_NIC_RX_QUEUES_PER_IO_LCORE
#define REPLAY_MAX_NIC_RX_QUEUES_PER_IO_LCORE 16
#endif

#ifndef REPLAY_MAX_NIC_TX_QUEUES_PER_IO_LCORE
#define REPLAY_MAX_NIC_TX_QUEUES_PER_IO_LCORE 16
#endif

#ifndef REPLAY_MAX_NIC_TX_PORTS_PER_IO_LCORE
#define REPLAY_MAX_NIC_TX_PORTS_PER_IO_LCORE 16
#endif
#if (REPLAY_MAX_NIC_TX_PORTS_PER_IO_LCORE > REPLAY_MAX_NIC_PORTS)
#error "REPLAY_MAX_NIC_TX_PORTS_PER_IO_LCORE too big"
#endif

#ifndef REPLAY_MAX_WORKER_LCORES
#define REPLAY_MAX_WORKER_LCORES 16
#endif
#if (REPLAY_MAX_WORKER_LCORES > REPLAY_MAX_LCORES)
#error "REPLAY_MAX_WORKER_LCORES is too big"
#endif

/* Mempools */
#ifndef REPLAY_DEFAULT_MBUF_SIZE
#define REPLAY_DEFAULT_MBUF_SIZE (9000 /*2048*/ + sizeof (struct rte_mbuf) + RTE_PKTMBUF_HEADROOM)
#endif

#ifndef REPLAY_DEFAULT_MEMPOOL_BUFFERS
#define REPLAY_DEFAULT_MEMPOOL_BUFFERS 8192 * 16 * 2
#endif

#ifndef REPLAY_DEFAULT_MEMPOOL_CACHE_SIZE
#define REPLAY_DEFAULT_MEMPOOL_CACHE_SIZE 256
#endif

/* LPM Tables */
#ifndef REPLAY_MAX_LPM_RULES
#define REPLAY_MAX_LPM_RULES 1024
#endif

/* NIC RX */
#ifndef REPLAY_DEFAULT_NIC_RX_RING_SIZE
#define REPLAY_DEFAULT_NIC_RX_RING_SIZE 1024
#endif

/*
 * RX and TX Prefetch, Host, and Write-back threshold values should be
 * carefully set for optimal performance. Consult the network
 * controller's datasheet and supporting DPDK documentation for guidance
 * on how these parameters should be set.
 */
#ifndef REPLAY_DEFAULT_NIC_RX_PTHRESH
#define REPLAY_DEFAULT_NIC_RX_PTHRESH 8
#endif

#ifndef REPLAY_DEFAULT_NIC_RX_HTHRESH
#define REPLAY_DEFAULT_NIC_RX_HTHRESH 8
#endif

#ifndef REPLAY_DEFAULT_NIC_RX_WTHRESH
#define REPLAY_DEFAULT_NIC_RX_WTHRESH 4
#endif

#ifndef REPLAY_DEFAULT_NIC_RX_FREE_THRESH
#define REPLAY_DEFAULT_NIC_RX_FREE_THRESH 64
#endif

#ifndef REPLAY_DEFAULT_NIC_RX_DROP_EN
#define REPLAY_DEFAULT_NIC_RX_DROP_EN 0  // Revisar
#endif

/* NIC TX */
#ifndef REPLAY_DEFAULT_NIC_TX_RING_SIZE
#define REPLAY_DEFAULT_NIC_TX_RING_SIZE 1024
#endif

/*
 * These default values are optimized for use with the Intel(R) 82599 10 GbE
 * Controller and the DPDK ixgbe PMD. Consider using other values for other
 * network controllers and/or network drivers.
 */
#ifndef REPLAY_DEFAULT_NIC_TX_PTHRESH
#define REPLAY_DEFAULT_NIC_TX_PTHRESH 36
#endif

#ifndef REPLAY_DEFAULT_NIC_TX_HTHRESH
#define REPLAY_DEFAULT_NIC_TX_HTHRESH 0
#endif

#ifndef REPLAY_DEFAULT_NIC_TX_WTHRESH
#define REPLAY_DEFAULT_NIC_TX_WTHRESH 0
#endif

#ifndef REPLAY_DEFAULT_NIC_TX_FREE_THRESH
#define REPLAY_DEFAULT_NIC_TX_FREE_THRESH 0
#endif

#ifndef REPLAY_DEFAULT_NIC_TX_RS_THRESH
#define REPLAY_DEFAULT_NIC_TX_RS_THRESH 0
#endif

/* Bursts */
#ifndef REPLAY_MBUF_ARRAY_SIZE
#define REPLAY_MBUF_ARRAY_SIZE 512
#endif

#ifndef REPLAY_DEFAULT_BURST_SIZE_IO_RX_READ
#define REPLAY_DEFAULT_BURST_SIZE_IO_RX_READ 144
#endif
#if (REPLAY_DEFAULT_BURST_SIZE_IO_RX_READ > REPLAY_MBUF_ARRAY_SIZE)
#error "REPLAY_DEFAULT_BURST_SIZE_IO_RX_READ is too big"
#endif

#ifndef REPLAY_DEFAULT_BURST_SIZE_IO_TX_WRITE
#define REPLAY_DEFAULT_BURST_SIZE_IO_TX_WRITE 144
#endif
#if (REPLAY_DEFAULT_BURST_SIZE_IO_TX_WRITE > REPLAY_MBUF_ARRAY_SIZE)
#error "REPLAY_DEFAULT_BURST_SIZE_IO_TX_WRITE is too big"
#endif

/* Load balancing logic */
#ifndef REPLAY_DEFAULT_IO_RX_LB_POS
#define REPLAY_DEFAULT_IO_RX_LB_POS 29
#endif
#if (REPLAY_DEFAULT_IO_RX_LB_POS >= 64)
#error "REPLAY_DEFAULT_IO_RX_LB_POS is too big"
#endif

struct replay_mbuf_array {
	struct rte_mbuf *array[REPLAY_MBUF_ARRAY_SIZE];
	uint32_t n_mbufs;
};

enum replay_lcore_type {
	e_REPLAY_LCORE_DISABLED = 0,
	e_REPLAY_LCORE_IO,
	e_REPLAY_LCORE_WORKER,
	e_REPLAY_LCORE_WORKER_SLAVE
};

#include <sys/time.h>
struct replay_lcore_params_io {
	/* I/O RX */
	struct {
		/* NIC */
		struct {
			uint8_t port;
			uint8_t queue;
		} nic_queues[REPLAY_MAX_NIC_RX_QUEUES_PER_IO_LCORE];
		uint32_t n_nic_queues;

		/* Internal buffers */
		struct replay_mbuf_array mbuf_in;
		struct replay_mbuf_array mbuf_out[REPLAY_MAX_WORKER_LCORES];
		uint8_t mbuf_out_flush[REPLAY_MAX_WORKER_LCORES];

		/* Stats */
		uint32_t nic_queues_count[REPLAY_MAX_NIC_RX_QUEUES_PER_IO_LCORE];
		uint32_t nic_queues_iters[REPLAY_MAX_NIC_RX_QUEUES_PER_IO_LCORE];
		uint32_t rings_count[REPLAY_MAX_WORKER_LCORES];
		uint32_t rings_iters[REPLAY_MAX_WORKER_LCORES];

		/* Timing */
		struct timeval start_ewr, end_ewr;
	} rx;

	/* I/O TX */
	struct {
		/* NIC */
		struct {
			uint8_t port;
			uint8_t queue;
		} nic_queues[REPLAY_MAX_NIC_TX_PORTS_PER_IO_LCORE];
		uint32_t n_nic_queues;

		/* Internal buffers */
		struct replay_mbuf_array mbuf_out[REPLAY_MAX_NIC_TX_PORTS_PER_IO_LCORE];
		uint8_t mbuf_out_flush[REPLAY_MAX_NIC_TX_PORTS_PER_IO_LCORE];

		/* Stats */
		uint32_t nic_queues_count[REPLAY_MAX_NIC_RX_QUEUES_PER_IO_LCORE];
		uint32_t nic_queues_iters[REPLAY_MAX_NIC_RX_QUEUES_PER_IO_LCORE];
		uint32_t nic_ports_count[REPLAY_MAX_NIC_TX_PORTS_PER_IO_LCORE];
		uint32_t nic_ports_iters[REPLAY_MAX_NIC_TX_PORTS_PER_IO_LCORE];

		/* Timing */
		struct timeval start_ewr, end_ewr;
	} tx;
};

struct replay_lcore_params {
	struct replay_lcore_params_io io;

	enum replay_lcore_type type;
	struct rte_mempool *pool;
} __rte_cache_aligned;

struct replay_params {
	/* lcore */
	struct replay_lcore_params lcore_params[REPLAY_MAX_LCORES];

	/* NIC */
	uint8_t nic_rx_queue_mask[REPLAY_MAX_NIC_PORTS][REPLAY_MAX_RX_QUEUES_PER_NIC_PORT];
	uint8_t nic_tx_queue_mask[REPLAY_MAX_NIC_PORTS][REPLAY_MAX_TX_QUEUES_PER_NIC_PORT];
	uint8_t nic_tx_queue_nvme[REPLAY_MAX_NIC_PORTS][REPLAY_MAX_TX_QUEUES_PER_NIC_PORT];

	/* mbuf pools */
	struct rte_mempool *pools[REPLAY_MAX_SOCKETS];

	/* rings */
	uint32_t nic_rx_ring_size;
	uint32_t nic_tx_ring_size;

	/* burst size */
	uint32_t burst_size_io_rx_read;
	uint32_t burst_size_io_tx_write;
} __rte_cache_aligned;

struct pktLatencyStat {
	hptl_t sentTime;
	hptl_t recvTime;
	struct timespec hwTime;
	uint64_t totalBytes;
	uint16_t pktLen;
	uint8_t recved;
} __rte_cache_aligned;

extern struct replay_params replay;

int replay_parse_args (int argc, char **argv);
void replay_print_usage (void);
void replay_init (void);
int replay_lcore_main_loop (void *arg);

int replay_get_nic_rx_queues_per_port (uint8_t port);
int replay_get_nic_tx_queues_per_port (uint8_t port);
int replay_get_lcore_for_nic_rx (uint8_t port, uint8_t queue, uint32_t *lcore_out);
int replay_get_lcore_for_nic_tx (uint8_t port, uint8_t queue, uint32_t *lcore_out);
int replay_is_socket_used (uint32_t socket);
uint32_t replay_get_lcores_io_rx (void);
uint32_t replay_get_lcores_worker (void);
void replay_print_params (void);

#ifdef RTE_EXEC_ENV_BAREMETAL
#define MAIN _main
#else
#ifndef TESTMAIN
#define MAIN main
#endif
#endif

int MAIN (int argc, char **argv);

#endif /* _MAIN_H_ */
