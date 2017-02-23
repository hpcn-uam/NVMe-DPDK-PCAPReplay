#include <common.h>
#include <simpleio.h>

#include "spdk/nvme.h"
#include "spdk/env.h"

// Async functions
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

// Works for pinned memory
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

// Copy into pinned memory
int sio_read_pinit (idisk* dsk, void* payload, uint64_t lba, uint32_t lba_count) {
	int sectorSize = spdk_nvme_ns_get_sector_size (dsk->ns);
	void* mem = spdk_malloc (lba_count * sectorSize, sectorSize, NULL);
	if (mem == NULL) {
		printf ("Error pinning memory for RD transaction\n");
		return -1;
	}
	int ret = sio_read (dsk, mem, lba, lba_count);
	memcpy (payload, mem, lba_count * sectorSize);
	return ret;
}

int sio_write_pinit (idisk* dsk, void* payload, uint64_t lba, uint32_t lba_count) {
	int sectorSize = spdk_nvme_ns_get_sector_size (dsk->ns);
	void* mem = spdk_malloc (lba_count * sectorSize, sectorSize, NULL);
	if (mem == NULL) {
		printf ("Error pinning memory for WR transaction\n");
		return -1;
	}
	memcpy (mem, payload, lba_count * sectorSize);
	int ret = sio_write (dsk, mem, lba, lba_count);
	return ret;
}