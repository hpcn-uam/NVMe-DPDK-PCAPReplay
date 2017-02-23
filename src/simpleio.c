#include <common.h>
#include <simpleio.h>

#include "spdk/nvme.h"
#include "spdk/env.h"

static void sio_read_complete (void* arg, const struct spdk_nvme_cpl* completion) {
	UNUSED (completion);
	int* iarg = (int*)arg;
	*iarg     = 1;
}

static void sio_write_complete (void* arg, const struct spdk_nvme_cpl* completion) {
	UNUSED (completion);

	int* iarg = (int*)arg;
	*iarg     = 1;
}

int sio_read (idisk* dsk, void* payload, uint64_t lba, uint32_t lba_count) {
	int rc;
	int completed = 0;

	rc = spdk_nvme_ns_cmd_read (
	    dsk->ns, dsk->qpair, payload, lba, lba_count, sio_read_complete, &completed, 0);
	if (rc != 0) {
		fprintf (stderr, "starting read I/O failed\n");
		return rc;
	}

	while (!completed) {
		spdk_nvme_qpair_process_completions (dsk->qpair, 0);
	}

	return rc;
}
int sio_write (idisk* dsk, void* payload, uint64_t lba, uint32_t lba_count) {
	int rc;
	int completed = 0;

	rc = spdk_nvme_ns_cmd_write (
	    dsk->ns, dsk->qpair, payload, lba, lba_count, sio_write_complete, &completed, 0);
	if (rc != 0) {
		fprintf (stderr, "starting write I/O failed\n");
		return rc;
	}

	while (!completed) {
		spdk_nvme_qpair_process_completions (dsk->qpair, 0);
	}

	return rc;
}