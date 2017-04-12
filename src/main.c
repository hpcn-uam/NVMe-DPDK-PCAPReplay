/*-
 *   BSD LICENSE
 *
 *   Copyright (c) Intel Corporation.
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
#include <string.h>
#include <unistd.h>

#include <rte_config.h>
#include <rte_eal.h>

#include "spdk/env.h"
#include "spdk/nvme.h"

#include <common.h>

nvmeRaid myRaid = {.numdisks = 0, .totalBlocks = 0};

static void register_ns (struct spdk_nvme_ctrlr *ctrlr, struct spdk_nvme_ns *ns) {
	const struct spdk_nvme_ctrlr_data *cdata;

	/*
	 * spdk_nvme_ctrlr is the logical abstraction in SPDK for an NVMe
	 *  controller.  During initialization, the IDENTIFY data for the
	 *  controller is read using an NVMe admin command, and that data
	 *  can be retrieved using spdk_nvme_ctrlr_get_data() to get
	 *  detailed information on the controller.  Refer to the NVMe
	 *  specification for more details on IDENTIFY for NVMe controllers.
	 */
	cdata = spdk_nvme_ctrlr_get_data (ctrlr);

	if (!spdk_nvme_ns_is_active (ns)) {
		printf ("Controller %-20.20s (%-20.20s): Skipping inactive NS %u\n",
		        cdata->mn,
		        cdata->sn,
		        spdk_nvme_ns_get_id (ns));
		return;
	}

	printf ("  Namespace ID: %d size: %juGB\n",
	        spdk_nvme_ns_get_id (ns),
	        spdk_nvme_ns_get_size (ns) / 1000000000);

	myRaid.disk[myRaid.numdisks].ctrlr = ctrlr;
	myRaid.disk[myRaid.numdisks].ns    = ns;
	myRaid.disk[myRaid.numdisks].qpair =
	    spdk_nvme_ctrlr_alloc_io_qpair (ctrlr, SPDK_NVME_QPRIO_URGENT);
	if (myRaid.disk[myRaid.numdisks].qpair == NULL) {
		printf ("ERROR: spdk_nvme_ctrlr_alloc_io_qpair() failed\n");
		return;
	}
	myRaid.totalBlocks += spdk_nvme_ns_get_num_sectors (ns);
	myRaid.numdisks++;
}

struct hello_world_sequence {
	struct ns_entry *ns_entry;
	char *buf;
	int is_completed;
};

static bool probe_cb (void *cb_ctx,
                      const struct spdk_nvme_transport_id *trid,
                      struct spdk_nvme_ctrlr_opts *opts) {
	UNUSED (cb_ctx);
	UNUSED (opts);
	printf ("Attaching to %s\n", trid->traddr);

	return true;
}

static void attach_cb (void *cb_ctx,
                       const struct spdk_nvme_transport_id *trid,
                       struct spdk_nvme_ctrlr *ctrlr,
                       const struct spdk_nvme_ctrlr_opts *opts) {
	UNUSED (cb_ctx);
	UNUSED (opts);
	int nsid, num_ns;

	printf ("Attached to %s ", trid->traddr);

	/*
	 * Each controller has one of more namespaces.  An NVMe namespace is basically
	 *  equivalent to a SCSI LUN.  The controller's IDENTIFY data tells us how
	 *  many namespaces exist on the controller.  For Intel(R) P3X00 controllers,
	 *  it will just be one namespace.
	 *
	 * Note that in NVMe, namespace IDs start at 1, not 0.
	 */
	num_ns = spdk_nvme_ctrlr_get_num_ns (ctrlr);
	printf (" with %d namespaces.\n", num_ns);
	for (nsid = 1; nsid <= num_ns; nsid++) {
		register_ns (ctrlr, spdk_nvme_ctrlr_get_ns (ctrlr, nsid));
	}
}

static void cleanup (void) {
}

int main (int argc, char **argv) {
	struct spdk_env_opts opts;
	int rc;

	// First of all, try to configure the app
	app_config (argc, argv);

	/* Init EAL */
	/*ret = rte_eal_init (argc, argv);
	if (ret < 0)
	    return -1;
	argc -= ret;
	argv += ret;*/

	// strcpy (ealargs[0], argv[0]);
	/*rc = rte_eal_init (sizeof (ealargs) / sizeof (ealargs[0]), ealargs);
	if (rc < 0) {
	    fprintf (stderr, "could not initialize dpdk\n");
	    return 1;
	}*/
	spdk_env_opts_init (&opts);
	opts.name      = "replay";
	opts.core_mask = "0xf";
	opts.shm_id    = -1;
	spdk_env_init (&opts);

	printf ("Initializing NVMe Controllers\n");

	/*
	 * Start the SPDK NVMe enumeration process.  probe_cb will be called
	 *  for each NVMe controller found, giving our application a choice on
	 *  whether to attach to each controller.  attach_cb will then be
	 *  called for each controller after the SPDK NVMe driver has completed
	 *  initializing the controller we chose to attach.
	 */
	rc = spdk_nvme_probe (NULL, NULL, probe_cb, attach_cb, NULL);
	if (rc != 0) {
		fprintf (stderr, "spdk_nvme_probe() failed\n");
		cleanup ();
		return 1;
	}

	printf ("Initialization complete.\n");
	printf ("Initializing NVMe-Raid\n");

	app_init (&myRaid);
	createRaid (&myRaid);
	printf ("NVMe-Raid started\n");
	// clean a bit the screen
	puts ("");
	puts ("");

	app_run (&myRaid);

	cleanup ();
	return 0;
}
