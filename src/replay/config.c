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

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>
#include <sys/types.h>
#include <string.h>
#include <sys/queue.h>
#include <stdarg.h>
#include <errno.h>
#include <getopt.h>

#include <rte_common.h>
#include <rte_byteorder.h>
#include <rte_log.h>
#include <rte_malloc.h>
#include <rte_memory.h>
#include <rte_memcpy.h>
#include <rte_memzone.h>
#include <rte_tailq.h>
#include <rte_eal.h>
#include <rte_per_lcore.h>
#include <rte_launch.h>
#include <rte_atomic.h>
#include <rte_cycles.h>
#include <rte_prefetch.h>
#include <rte_lcore.h>
#include <rte_per_lcore.h>
#include <rte_branch_prediction.h>
#include <rte_interrupts.h>
#include <rte_pci.h>
#include <rte_random.h>
#include <rte_debug.h>
#include <rte_ether.h>
#include <rte_ethdev.h>
#include <rte_ring.h>
#include <rte_mempool.h>
#include <rte_mbuf.h>
#include <rte_ip.h>
#include <rte_tcp.h>
#include <rte_lpm.h>
#include <rte_string_fns.h>

#include "replay.h"

struct replay_params replay;

static const char usage[] =
    "                                                                               \n"
    "    hpcn_latency <EAL PARAMS> -- <REPLAY PARAMS>                                  \n"
    "                                                                               \n"
    "Application manadatory parameters:                                             \n"
    "    --rx \"(PORT, QUEUE, LCORE), ...\" : List of NIC RX ports and queues       \n"
    "           handled by the I/O RX lcores                                        \n"
    "    --tx \"(PORT, QUEUE, LCORE), ...\" : List of NIC TX ports and queues       \n"
    "           handled by the I/O TX lcores                                        \n"
    "    --w \"LCORE, ...\" : List of the worker lcores                             \n"

    "                                                                               \n"
    "Application optional parameters:                                               \n"
    "    --rsz \"A, B\" : Ring sizes                                                \n"
    "           A = Size (in number of buffer descriptors) of each of the NIC RX    \n"
    "               rings read by the I/O RX lcores (default value is %u)           \n"
    "           B = Size (in number of buffer descriptors) of each of the NIC TX    \n"
    "               rings written by I/O TX lcores (default value is %u)            \n"
    "    --bsz \"A, B\" :  Burst sizes                                              \n"
    "           A = I/O RX lcore read burst size from NIC RX (default value is %u)  \n"
    "           B = I/O TX lcore write burst size to NIC TX (default value is %u)   \n"

    "                                                                               \n"
    "Packet-Sending parameters:                                                     \n"
    "    --etho \"aa:bb:cc:dd:ee:ff\" : The ethernet origin MAC addr                \n"
    "    --ethd \"aa:bb:cc:dd:ee:ff\" : The ethernet destination MAC addr           \n"
    "    --ipo \"11.22.33.44\" : The ip origin addr                                 \n"
    "    --ipd \"11.22.33.44\" : The ip destination addr                            \n"
    "    --pktLen \"Packet LENGTH\" : Sets the size of each sent packet             \n"
    "    --trainLen \"TRAIN LENGTH\" : Enables and sets the packet train length     \n"
    "    --trainFriends \"TRAIN FRIENDS\" : if you are performing a simple latency    "
    " measuremets tests with trainSleep activated, you can add friend packets to the  "
    " train. This means: for each timestamped packet, a #friend dummy packets would   "
    " be sent too                                                                   \n"
    "    --trainSleep \"TRAIN SLEEP\": Sleep in NS between packets                  \n"
    "    --hw : Checks HW timestamp packet [For debug purposes]                     \n"
    "    --sts : Mode that sends lots of packets but only a few are timestamped     \n"
    "    --waitTime \"WAIT TIMEOUT\" : Nanoseconds to stop the measurment when all  \n"
    "                                    packets has been sent                      \n"
    "    --chksum : Each packet recalculate the IP/ICMP checksum                    \n"
    "    --autoInc : Each packet autoincrements the ICMP's sequence number          \n"
    "    --bw : Only measures bandwidth, but with higher resolution                 \n"
    "    --bwp: Only measures bandwidth(pasive) by just listening. No packet is sent\n"
    "    --lo : The replaylication works in loopback mode. Used to measure RTT         \n"

    "                                                                               \n"
    "Calibration Parameters                                                         \n"
    "    --calibrate \"outputFile\" : Generate a calibration file. May take minutes"
    " [IN DEV]\n"
    "    --calibration \"inputFile\" : Open a calibration file, to fix measurements"
    " [UNIMPLEMENTED]\n"
    "                                                                               \n"

    "Misc Parameters                                                                \n"
    "    --outputFile \"outputFile\" : A file to redirect output data               \n";

void replay_print_usage (void) {
	printf (usage,
	        REPLAY_DEFAULT_NIC_RX_RING_SIZE,
	        REPLAY_DEFAULT_NIC_TX_RING_SIZE,
	        REPLAY_DEFAULT_BURST_SIZE_IO_RX_READ,
	        REPLAY_DEFAULT_BURST_SIZE_IO_TX_WRITE);
}

#ifndef REPLAY_ARG_RX_MAX_CHARS
#define REPLAY_ARG_RX_MAX_CHARS 4096
#endif

#ifndef REPLAY_ARG_RX_MAX_TUPLES
#define REPLAY_ARG_RX_MAX_TUPLES 128
#endif

static int str_to_unsigned_array (
    const char *s, size_t sbuflen, char separator, unsigned num_vals, unsigned *vals) {
	char str[sbuflen + 1];
	char *splits[num_vals];
	char *endptr      = NULL;
	int i, num_splits = 0;

	/* copy s so we don't modify original string */
	snprintf (str, sizeof (str), "%s", s);
	num_splits = rte_strsplit (str, sizeof (str), splits, num_vals, separator);

	errno = 0;
	for (i = 0; i < num_splits; i++) {
		vals[i] = strtoul (splits[i], &endptr, 0);
		if (errno != 0 || *endptr != '\0')
			return -1;
	}

	return num_splits;
}

static int str_to_unsigned_vals (
    const char *s, size_t sbuflen, char separator, unsigned num_vals, ...) {
	unsigned i, vals[num_vals];
	va_list ap;

	num_vals = str_to_unsigned_array (s, sbuflen, separator, num_vals, vals);

	va_start (ap, num_vals);
	for (i = 0; i < num_vals; i++) {
		unsigned *u = va_arg (ap, unsigned *);
		*u          = vals[i];
	}
	va_end (ap);
	return num_vals;
}

static int parse_arg_rx (const char *arg) {
	const char *p0 = arg, *p = arg;
	uint32_t n_tuples;

	if (strnlen (arg, REPLAY_ARG_RX_MAX_CHARS + 1) == REPLAY_ARG_RX_MAX_CHARS + 1) {
		return -1;
	}

	n_tuples = 0;
	while ((p = strchr (p0, '(')) != NULL) {
		struct replay_lcore_params *lp;
		uint32_t port, queue, lcore, i;

		p0 = strchr (p++, ')');
		if ((p0 == NULL) ||
		    (str_to_unsigned_vals (p, p0 - p, ',', 3, &port, &queue, &lcore) != 3)) {
			return -2;
		}

		/* Enable port and queue for later initialization */
		if ((port >= REPLAY_MAX_NIC_PORTS) || (queue >= REPLAY_MAX_RX_QUEUES_PER_NIC_PORT)) {
			return -3;
		}
		if (replay.nic_rx_queue_mask[port][queue] != 0) {
			return -4;
		}
		replay.nic_rx_queue_mask[port][queue] = 1;

		/* Check and assign (port, queue) to I/O lcore */
		if (rte_lcore_is_enabled (lcore) == 0) {
			return -5;
		}

		if (lcore >= REPLAY_MAX_LCORES) {
			return -6;
		}
		lp = &replay.lcore_params[lcore];
		if (lp->type == e_REPLAY_LCORE_WORKER) {
			return -7;
		}
		lp->type = e_REPLAY_LCORE_IO;
		for (i = 0; i < lp->io.rx.n_nic_queues; i++) {
			if ((lp->io.rx.nic_queues[i].port == port) &&
			    (lp->io.rx.nic_queues[i].queue == queue)) {
				return -8;
			}
		}
		if (lp->io.rx.n_nic_queues >= REPLAY_MAX_NIC_RX_QUEUES_PER_IO_LCORE) {
			return -9;
		}
		lp->io.rx.nic_queues[lp->io.rx.n_nic_queues].port  = (uint8_t)port;
		lp->io.rx.nic_queues[lp->io.rx.n_nic_queues].queue = (uint8_t)queue;
		lp->io.rx.n_nic_queues++;

		n_tuples++;
		if (n_tuples > REPLAY_ARG_RX_MAX_TUPLES) {
			return -10;
		}
	}

	if (n_tuples == 0) {
		return -11;
	}

	return 0;
}

#ifndef REPLAY_ARG_TX_MAX_CHARS
#define REPLAY_ARG_TX_MAX_CHARS 4096
#endif

#ifndef REPLAY_ARG_TX_MAX_TUPLES
#define REPLAY_ARG_TX_MAX_TUPLES 128
#endif

static int parse_arg_tx (const char *arg) {
	const char *p0 = arg, *p = arg;
	uint32_t n_tuples;

	if (strnlen (arg, REPLAY_ARG_TX_MAX_CHARS + 1) == REPLAY_ARG_TX_MAX_CHARS + 1) {
		return -1;
	}

	n_tuples = 0;
	while ((p = strchr (p0, '(')) != NULL) {
		struct replay_lcore_params *lp;
		uint32_t port, queue, lcore, i;

		p0 = strchr (p++, ')');
		if ((p0 == NULL) ||
		    (str_to_unsigned_vals (p, p0 - p, ',', 3, &port, &queue, &lcore) != 3)) {
			return -2;
		}

		/* Enable port and queue for later initialization */
		if ((port >= REPLAY_MAX_NIC_PORTS) || (queue >= REPLAY_MAX_TX_QUEUES_PER_NIC_PORT)) {
			return -3;
		}
		if (replay.nic_tx_queue_mask[port][queue] != 0) {
			return -4;
		}
		replay.nic_tx_queue_mask[port][queue] = 1;

		/* Check and assign (port, queue) to I/O lcore */
		if (rte_lcore_is_enabled (lcore) == 0) {
			return -5;
		}

		if (lcore >= REPLAY_MAX_LCORES) {
			return -6;
		}
		lp = &replay.lcore_params[lcore];
		if (lp->type == e_REPLAY_LCORE_WORKER) {
			return -7;
		}
		lp->type = e_REPLAY_LCORE_IO;
		for (i = 0; i < lp->io.tx.n_nic_queues; i++) {
			if ((lp->io.tx.nic_queues[i].port == port) &&
			    (lp->io.tx.nic_queues[i].queue == queue)) {
				return -8;
			}
		}
		if (lp->io.tx.n_nic_queues >= REPLAY_MAX_NIC_TX_QUEUES_PER_IO_LCORE) {
			return -9;
		}
		lp->io.tx.nic_queues[lp->io.tx.n_nic_queues].port  = (uint8_t)port;
		lp->io.tx.nic_queues[lp->io.tx.n_nic_queues].queue = (uint8_t)queue;
		lp->io.tx.n_nic_queues++;

		n_tuples++;
		if (n_tuples > REPLAY_ARG_TX_MAX_TUPLES) {
			return -10;
		}
	}

	if (n_tuples == 0) {
		return -11;
	}

	return 0;
}

#ifndef REPLAY_ARG_RSZ_CHARS
#define REPLAY_ARG_RSZ_CHARS 63
#endif

static int parse_arg_rsz (const char *arg) {
	if (strnlen (arg, REPLAY_ARG_RSZ_CHARS + 1) == REPLAY_ARG_RSZ_CHARS + 1) {
		return -1;
	}

	if (str_to_unsigned_vals (
	        arg, REPLAY_ARG_RSZ_CHARS, ',', 2, &replay.nic_rx_ring_size, &replay.nic_tx_ring_size) != 2)
		return -2;

	if ((replay.nic_rx_ring_size == 0) || (replay.nic_tx_ring_size == 0)) {
		return -3;
	}

	return 0;
}

#ifndef REPLAY_ARG_BSZ_CHARS
#define REPLAY_ARG_BSZ_CHARS 63
#endif

static int parse_arg_bsz (const char *arg) {
	if (strnlen (arg, REPLAY_ARG_BSZ_CHARS + 1) == REPLAY_ARG_BSZ_CHARS + 1) {
		return -1;
	}

	if (str_to_unsigned_vals (arg,
	                          REPLAY_ARG_BSZ_CHARS,
	                          ',',
	                          2,
	                          &replay.burst_size_io_rx_read,
	                          &replay.burst_size_io_tx_write) != 2)
		return -2;

	if ((replay.burst_size_io_rx_read == 0) || (replay.burst_size_io_tx_write == 0)) {
		return -7;
	}

	if ((replay.burst_size_io_rx_read > REPLAY_MBUF_ARRAY_SIZE) ||
	    (replay.burst_size_io_tx_write > REPLAY_MBUF_ARRAY_SIZE)) {
		return -8;
	}

	return 0;
}

#ifndef REPLAY_ARG_NUMERICAL_SIZE_CHARS
#define REPLAY_ARG_NUMERICAL_SIZE_CHARS 15
#endif

extern uint8_t icmppkt[];
extern int doChecksum;
extern int autoIncNum;
extern int selectiveTS;
extern int bandWidthMeasure;
extern int bandWidthMeasureActive;
extern int hwTimeTest;
extern uint64_t trainLen;
extern uint64_t trainSleep;  // ns
extern uint64_t trainFriends;
extern uint64_t waitTime;  // ns
extern unsigned sndpktlen;

extern struct pktLatencyStat *latencyStats;

// calibration and autoconf
extern int autoCalibrate;
extern int fixMeasurements;
extern FILE *calibfd;

#ifndef REPLAY_ARG_ETH_SIZE_CHARS
#define REPLAY_ARG_ETH_SIZE_CHARS (2 * 6 + 1 * 5)
#endif

static int parse_arg_etho (const char *arg) {
	uint8_t etho[6];

	if (sscanf (arg,
	            "%hhX:%hhX:%hhX:%hhX:%hhX:%hhX",
	            etho + 0,
	            etho + 1,
	            etho + 2,
	            etho + 3,
	            etho + 4,
	            etho + 5) != sizeof (etho)) {
		return -1;
	}

	memcpy (icmppkt + 6, etho, sizeof (etho));

	return 0;
}

static int parse_arg_ethd (const char *arg) {
	uint8_t ethd[6];

	if (sscanf (arg,
	            "%hhX:%hhX:%hhX:%hhX:%hhX:%hhX",
	            ethd + 0,
	            ethd + 1,
	            ethd + 2,
	            ethd + 3,
	            ethd + 4,
	            ethd + 5) != sizeof (ethd)) {
		return -1;
	}

	memcpy (icmppkt, ethd, sizeof (ethd));

	return 0;
}

static int parse_arg_ipo (const char *arg) {
	uint8_t ip[4];

	if (sscanf (arg, "%hhd.%hhd.%hhd.%hhd", ip + 0, ip + 1, ip + 2, ip + 3) != sizeof (ip)) {
		return -1;
	}

	memcpy (icmppkt + 6 + 6 + 2 + 3 * 4, ip, sizeof (ip));

	return 0;
}

static int parse_arg_ipd (const char *arg) {
	uint8_t ip[4];

	if (sscanf (arg, "%hhd.%hhd.%hhd.%hhd", ip + 0, ip + 1, ip + 2, ip + 3) != sizeof (ip)) {
		return -1;
	}

	memcpy (icmppkt + 6 + 6 + 2 + 4 * 4, ip, sizeof (ip));

	return 0;
}

static int parse_arg_pktLen (const char *arg) {
	if (sscanf (arg, "%u", &sndpktlen) != 1) {
		return -1;
	}

	return 0;
}

static int parse_arg_trainLen (const char *arg) {
	if (sscanf (arg, "%lu", &trainLen) != 1) {
		return -1;
	}

	if (latencyStats)
		rte_free (latencyStats);

	latencyStats = rte_calloc ("latency_stats", trainLen, sizeof (struct pktLatencyStat), 0);

	return 0;
}

static int parse_arg_trainSleep (const char *arg) {
	if (sscanf (arg, "%lu", &trainSleep) != 1) {
		return -1;
	}

	return 0;
}

static int parse_arg_trainFriends (const char *arg) {
	if (sscanf (arg, "%lu", &trainFriends) != 1) {
		return -1;
	}

	return 0;
}

static int parse_arg_waitTime (const char *arg) {
	if (sscanf (arg, "%lu", &waitTime) != 1) {
		return -1;
	}

	return 0;
}

static int parse_arg_outputFile (const char *arg) {
	FILE *f = freopen (arg, "w+", stdout);
	return !f;
}

static int parse_arg_calibrate (const char *arg) {
	autoCalibrate = 1;

	FILE *calibfd = fopen ("w+", arg);
	return !calibfd;
}

static int parse_arg_calibration (const char *arg) {
	fixMeasurements = 1;

	FILE *calibfd = fopen ("r", arg);
	return !calibfd;
}

/* Parse the argument given in the command line of the replaylication */
int replay_parse_args (int argc, char **argv) {
	int opt, ret;
	char **argvopt;
	int option_index;
	char *prgname                 = argv[0];
	static struct option lgopts[] = {// Classic parameters
	                                 {"rx", 1, 0, 0},
	                                 {"tx", 1, 0, 0},
	                                 {"rsz", 1, 0, 0},
	                                 {"bsz", 1, 0, 0},
	                                 // Net config
	                                 {"etho", 1, 0, 0},
	                                 {"ethd", 1, 0, 0},
	                                 {"ipo", 1, 0, 0},
	                                 {"ipd", 1, 0, 0},
	                                 {"pktLen", 1, 0, 0},
	                                 // Trains, if not set, permanent connection would be done
	                                 {"trainLen", 1, 0, 0},
	                                 {"trainSleep", 1, 0, 0},
	                                 {"trainFriends", 1, 0, 0},
	                                 {"waitTime", 1, 0, 0},
	                                 // Auto fix ICMP packets
	                                 {"chksum", 0, 0, 0},
	                                 {"autoInc", 0, 0, 0},
	                                 // Measurment kind
	                                 {"bw", 0, 0, 0},
	                                 {"bwp", 0, 0, 0},
	                                 {"sts", 0, 0, 0},
	                                 // debug
	                                 {"hw", 0, 0, 0},
	                                 // calibrarion
	                                 {"calibrate", 1, 0, 0},
	                                 {"calibration", 1, 0, 0},
	                                 // Misc functions
	                                 {"outputFile", 1, 0, 0},
	                                 // endlist
	                                 {NULL, 0, 0, 0}};
	uint32_t arg_rx  = 0;
	uint32_t arg_tx  = 0;
	uint32_t arg_rsz = 0;
	uint32_t arg_bsz = 0;

	argvopt = argv;

	while ((opt = getopt_long (argc, argvopt, "", lgopts, &option_index)) != EOF) {
		switch (opt) {
			/* long options */
			case 0:
				if (!strcmp (lgopts[option_index].name, "rx")) {
					arg_rx = 1;
					ret = parse_arg_rx (optarg);
					if (ret) {
						printf ("Incorrect value for --rx argument (%d)\n", ret);
						return -1;
					}
				}
				if (!strcmp (lgopts[option_index].name, "tx")) {
					arg_tx = 1;
					ret = parse_arg_tx (optarg);
					if (ret) {
						printf ("Incorrect value for --tx argument (%d)\n", ret);
						return -1;
					}
				}
				if (!strcmp (lgopts[option_index].name, "rsz")) {
					arg_rsz = 1;
					ret = parse_arg_rsz (optarg);
					if (ret) {
						printf ("Incorrect value for --rsz argument (%d)\n", ret);
						return -1;
					}
				}
				if (!strcmp (lgopts[option_index].name, "bsz")) {
					arg_bsz = 1;
					ret = parse_arg_bsz (optarg);
					if (ret) {
						printf ("Incorrect value for --bsz argument (%d)\n", ret);
						return -1;
					}
				}
				if (!strcmp (lgopts[option_index].name, "etho")) {
					ret = parse_arg_etho (optarg);
					if (ret) {
						printf ("Incorrect value for --etho argument (%s, error code: %d)\n",
						        optarg,
						        ret);
						return -1;
					}
				}
				if (!strcmp (lgopts[option_index].name, "ethd")) {
					ret = parse_arg_ethd (optarg);
					if (ret) {
						printf ("Incorrect value for --ethd argument (%s, error code: %d)\n",
						        optarg,
						        ret);
						return -1;
					}
				}
				if (!strcmp (lgopts[option_index].name, "ipo")) {
					ret = parse_arg_ipo (optarg);
					if (ret) {
						printf ("Incorrect value for --ipo argument (%s, error code: %d)\n",
						        optarg,
						        ret);
						return -1;
					}
				}
				if (!strcmp (lgopts[option_index].name, "ipd")) {
					ret = parse_arg_ipd (optarg);
					if (ret) {
						printf ("Incorrect value for --ipd argument (%s, error code: %d)\n",
						        optarg,
						        ret);
						return -1;
					}
				}
				if (!strcmp (lgopts[option_index].name, "pktLen")) {
					ret = parse_arg_pktLen (optarg);
					if (ret) {
						printf ("Incorrect value for --pktLen argument (%s, error code: %d)\n",
						        optarg,
						        ret);
						return -1;
					}
				}
				if (!strcmp (lgopts[option_index].name, "trainLen")) {
					ret = parse_arg_trainLen (optarg);
					if (ret) {
						printf ("Incorrect value for --trainLen argument (%s, error code: %d)\n",
						        optarg,
						        ret);
						return -1;
					}
				}
				if (!strcmp (lgopts[option_index].name, "trainSleep")) {
					ret = parse_arg_trainSleep (optarg);
					if (ret) {
						printf ("Incorrect value for --trainSleep argument (%s, error code: %d)\n",
						        optarg,
						        ret);
						return -1;
					}
				}
				if (!strcmp (lgopts[option_index].name, "trainFriends")) {
					ret = parse_arg_trainFriends (optarg);
					if (ret) {
						printf (
						    "Incorrect value for --trainFriends argument (%s, error code: %d)\n",
						    optarg,
						    ret);
						return -1;
					}
				}
				if (!strcmp (lgopts[option_index].name, "waitTime")) {
					ret = parse_arg_waitTime (optarg);
					if (ret) {
						printf ("Incorrect value for --waitTime argument (%s, error code: %d)\n",
						        optarg,
						        ret);
						return -1;
					}
				}
				if (!strcmp (lgopts[option_index].name, "calibrate")) {
					ret = parse_arg_calibrate (optarg);
					if (ret) {
						printf ("Incorrect value for --calibrate argument (%s, error code: %d)\n",
						        optarg,
						        ret);
						return -1;
					}
				}
				if (!strcmp (lgopts[option_index].name, "calibration")) {
					ret = parse_arg_calibration (optarg);
					if (ret) {
						printf ("Incorrect value for --calibration argument (%s, error code: %d)\n",
						        optarg,
						        ret);
						return -1;
					}
				}
				if (!strcmp (lgopts[option_index].name, "outputFile")) {
					ret = parse_arg_outputFile (optarg);
					if (ret) {
						printf ("Incorrect value for --outputFile argument (%s, error code: %d)\n",
						        optarg,
						        ret);
						return -1;
					}
				}
				if (!strcmp (lgopts[option_index].name, "chksum")) {
					doChecksum = 1;
				}
				if (!strcmp (lgopts[option_index].name, "autoInc")) {
					autoIncNum = 1;
				}
				if (!strcmp (lgopts[option_index].name, "bw")) {
					bandWidthMeasure       = 1;
					bandWidthMeasureActive = 1;
				}
				if (!strcmp (lgopts[option_index].name, "bwp")) {
					bandWidthMeasure = 1;
				}
				if (!strcmp (lgopts[option_index].name, "hw")) {
					hwTimeTest = 1;
				}
				if (!strcmp (lgopts[option_index].name, "sts")) {
					selectiveTS = 1;
				}
				break;

			default:
				return -1;
		}
	}

	/* Check that all mandatory arguments are provided */
	if ((arg_rx == 0) || (arg_tx == 0)) {
		printf ("Not all mandatory arguments are present\n");
		return -1;
	}

	/* Assign default values for the optional arguments not provided */
	if (arg_rsz == 0) {
		replay.nic_rx_ring_size = REPLAY_DEFAULT_NIC_RX_RING_SIZE;
		replay.nic_tx_ring_size = REPLAY_DEFAULT_NIC_TX_RING_SIZE;
	}

	if (arg_bsz == 0) {
		replay.burst_size_io_rx_read  = REPLAY_DEFAULT_BURST_SIZE_IO_RX_READ;
		replay.burst_size_io_tx_write = REPLAY_DEFAULT_BURST_SIZE_IO_TX_WRITE;
	}

	if (optind >= 0)
		argv[optind - 1] = prgname;

	// Latency replay arguments
	if (trainLen == 0) {  // activate bandwidth mode
		printf ("No trainLength set, activating --bw mode\n");
		bandWidthMeasure = 1;
	}

	ret    = optind - 1;
	optind = 0; /* reset getopt lib */
	return ret;
}

int replay_get_nic_rx_queues_per_port (uint8_t port) {
	uint32_t i, count;

	if (port >= REPLAY_MAX_NIC_PORTS) {
		return -1;
	}

	count = 0;
	for (i = 0; i < REPLAY_MAX_RX_QUEUES_PER_NIC_PORT; i++) {
		if (replay.nic_rx_queue_mask[port][i] == 1) {
			count++;
		}
	}

	return count;
}

int replay_get_nic_tx_queues_per_port (uint8_t port) {
	uint32_t i, count;

	if (port >= REPLAY_MAX_NIC_PORTS) {
		return -1;
	}

	count = 0;
	for (i = 0; i < REPLAY_MAX_TX_QUEUES_PER_NIC_PORT; i++) {
		if (replay.nic_tx_queue_mask[port][i] == 1) {
			count++;
		}
	}

	return count;
}

int replay_get_lcore_for_nic_rx (uint8_t port, uint8_t queue, uint32_t *lcore_out) {
	uint32_t lcore;

	for (lcore = 0; lcore < REPLAY_MAX_LCORES; lcore++) {
		struct replay_lcore_params_io *lp = &replay.lcore_params[lcore].io;
		uint32_t i;

		if (replay.lcore_params[lcore].type != e_REPLAY_LCORE_IO) {
			continue;
		}

		for (i = 0; i < lp->rx.n_nic_queues; i++) {
			if ((lp->rx.nic_queues[i].port == port) && (lp->rx.nic_queues[i].queue == queue)) {
				*lcore_out = lcore;
				return 0;
			}
		}
	}

	return -1;
}

int replay_get_lcore_for_nic_tx (uint8_t port, uint8_t queue, uint32_t *lcore_out) {
	uint32_t lcore;

	for (lcore = 0; lcore < REPLAY_MAX_LCORES; lcore++) {
		struct replay_lcore_params_io *lp = &replay.lcore_params[lcore].io;
		uint32_t i;

		if (replay.lcore_params[lcore].type != e_REPLAY_LCORE_IO) {
			continue;
		}

		for (i = 0; i < lp->tx.n_nic_queues; i++) {
			if ((lp->tx.nic_queues[i].port == port) && (lp->tx.nic_queues[i].queue == queue)) {
				*lcore_out = lcore;
				return 0;
			}
		}
	}

	return -1;
}

int replay_is_socket_used (uint32_t socket) {
	uint32_t lcore;

	for (lcore = 0; lcore < REPLAY_MAX_LCORES; lcore++) {
		if (replay.lcore_params[lcore].type == e_REPLAY_LCORE_DISABLED) {
			continue;
		}

		if (socket == rte_lcore_to_socket_id (lcore)) {
			return 1;
		}
	}

	return 0;
}

uint32_t replay_get_lcores_io_rx (void) {
	uint32_t lcore, count;

	count = 0;
	for (lcore = 0; lcore < REPLAY_MAX_LCORES; lcore++) {
		struct replay_lcore_params_io *lp_io = &replay.lcore_params[lcore].io;

		if ((replay.lcore_params[lcore].type != e_REPLAY_LCORE_IO) || (lp_io->rx.n_nic_queues == 0)) {
			continue;
		}

		count++;
	}

	return count;
}

uint32_t replay_get_lcores_worker (void) {
	uint32_t lcore, count;

	count = 0;
	for (lcore = 0; lcore < REPLAY_MAX_LCORES; lcore++) {
		if (replay.lcore_params[lcore].type != e_REPLAY_LCORE_WORKER) {
			continue;
		}

		count++;
	}

	if (count > REPLAY_MAX_WORKER_LCORES) {
		rte_panic ("Algorithmic error (too many worker lcores)\n");
		return 0;
	}

	return count;
}

void replay_print_params (void) {
	unsigned port, queue, lcore, i /*, j*/;

	/* Print NIC RX configuration */
	printf ("NIC RX ports: ");
	for (port = 0; port < REPLAY_MAX_NIC_PORTS; port++) {
		uint32_t n_rx_queues = replay_get_nic_rx_queues_per_port ((uint8_t)port);

		if (n_rx_queues == 0) {
			continue;
		}

		printf ("%u (", port);
		for (queue = 0; queue < REPLAY_MAX_RX_QUEUES_PER_NIC_PORT; queue++) {
			if (replay.nic_rx_queue_mask[port][queue] == 1) {
				printf ("%u ", queue);
			}
		}
		printf (")  ");
	}
	printf (";\n");

	/* Print I/O lcore RX params */
	for (lcore = 0; lcore < REPLAY_MAX_LCORES; lcore++) {
		struct replay_lcore_params_io *lp = &replay.lcore_params[lcore].io;

		if ((replay.lcore_params[lcore].type != e_REPLAY_LCORE_IO) || (lp->rx.n_nic_queues == 0)) {
			continue;
		}

		printf ("I/O lcore %u (socket %u): ", lcore, rte_lcore_to_socket_id (lcore));

		printf ("RX ports  ");
		for (i = 0; i < lp->rx.n_nic_queues; i++) {
			printf ("(%u, %u)  ",
			        (unsigned)lp->rx.nic_queues[i].port,
			        (unsigned)lp->rx.nic_queues[i].queue);
		}
		printf (";\n");
	}

	/* Print NIC TX configuration */
	printf ("NIC TX ports: ");
	for (port = 0; port < REPLAY_MAX_NIC_PORTS; port++) {
		uint32_t n_tx_queues = replay_get_nic_tx_queues_per_port ((uint8_t)port);

		if (n_tx_queues == 0) {
			continue;
		}

		printf ("%u (", port);
		for (queue = 0; queue < REPLAY_MAX_TX_QUEUES_PER_NIC_PORT; queue++) {
			if (replay.nic_tx_queue_mask[port][queue] == 1) {
				printf ("%u ", queue);
			}
		}
		printf (")  ");
	}
	printf (";\n");

	/* Print I/O lcore TX params */
	for (lcore = 0; lcore < REPLAY_MAX_LCORES; lcore++) {
		struct replay_lcore_params_io *lp = &replay.lcore_params[lcore].io;

		if ((replay.lcore_params[lcore].type != e_REPLAY_LCORE_IO) || (lp->tx.n_nic_queues == 0)) {
			continue;
		}

		printf ("I/O lcore %u (socket %u): ", lcore, rte_lcore_to_socket_id (lcore));

		printf ("TX ports  ");
		for (i = 0; i < lp->tx.n_nic_queues; i++) {
			printf ("(%u, %u)  ",
			        (unsigned)lp->tx.nic_queues[i].port,
			        (unsigned)lp->tx.nic_queues[i].queue);
		}
		printf (";\n");
	}

	/* Rings */
	printf ("Ring sizes: NIC RX = %u; NIC TX = %u;\n",
	        (unsigned)replay.nic_rx_ring_size,
	        (unsigned)replay.nic_tx_ring_size);

	/* Bursts */
	printf ("Burst sizes: I/O RX rd = %u; I/O TX wr = %u)\n",
	        (unsigned)replay.burst_size_io_rx_read,
	        (unsigned)replay.burst_size_io_tx_write);
}
