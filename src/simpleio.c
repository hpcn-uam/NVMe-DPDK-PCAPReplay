#include <common.h>
#include <simpleio.h>

#include "spdk/nvme.h"
#include "spdk/env.h"

// Simpliest commands
int sio_sectorSize (idisk* dsk) {
	return spdk_nvme_ns_get_sector_size (dsk->ns);
}

// Async functions
static void sio_read_complete (void* restrict arg,
                               const struct spdk_nvme_cpl* restrict completion) {
	UNUSED (completion);
	uint64_t* iarg = (uint64_t*)arg;
	++*iarg;
}

static void sio_write_complete (void* restrict arg,
                                const struct spdk_nvme_cpl* restrict completion) {
	UNUSED (completion);

	uint64_t* iarg = (uint64_t*)arg;
	++*iarg;
}

// Works for pinned memory
int sio_read (idisk* restrict dsk, void* restrict payload, uint64_t lba, uint32_t lba_count) {
	int rc;
	uint64_t completed = 0;

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
int sio_write (idisk* restrict dsk, void* restrict payload, uint64_t lba, uint32_t lba_count) {
	int rc;
	uint64_t completed = 0;

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
int sio_read_pinit (idisk* restrict dsk, void* restrict payload, uint64_t lba, uint32_t lba_count) {
	int sectorSize = spdk_nvme_ns_get_sector_size (dsk->ns);
	void* mem = spdk_malloc (lba_count * sectorSize, sectorSize, NULL);
	if (mem == NULL) {
		printf ("Error pinning memory for RD transaction\n");
		return -1;
	}
	int ret = sio_read (dsk, mem, lba, lba_count);
	memcpy (payload, mem, lba_count * sectorSize);
	spdk_free (mem);
	return ret;
}

int sio_write_pinit (idisk* restrict dsk,
                     void* restrict payload,
                     uint64_t lba,
                     uint32_t lba_count) {
	int sectorSize = spdk_nvme_ns_get_sector_size (dsk->ns);
	void* mem = spdk_malloc (lba_count * sectorSize, sectorSize, NULL);
	if (mem == NULL) {
		printf ("Error pinning memory for WR transaction\n");
		return -1;
	}
	memcpy (mem, payload, lba_count * sectorSize);
	int ret = sio_write (dsk, mem, lba, lba_count);
	spdk_free (mem);
	return ret;
}

uint64_t task_completed = 0;
uint64_t task_scheduled = 0;

// Works for pinned memory
int sio_rread (nvmeRaid* restrict raid, void* restrict payload, uint64_t lba, uint32_t lba_count) {
	int rc, i = 0;
	uint64_t dstlba;

	if (lba_count <= (SUPERSECTORNUM - lba)) {
		task_scheduled++;
		i      = super_getdisk (raid, lba);
		dstlba = super_getdisklba (raid, lba);
		rc     = spdk_nvme_ns_cmd_read (raid->disk[i].ns,
		                            raid->disk[i].qpair,
		                            payload,
		                            dstlba,
		                            lba_count,
		                            sio_read_complete,
		                            &task_completed,
		                            0);
		if (rc != 0) {
			fprintf (stderr, "starting read I/O failed\n");
			return rc;
		}
	} else if (lba % SUPERSECTORNUM) {  // unalligned
		rc = sio_rread (raid, payload, lba, (SUPERSECTORNUM - lba % SUPERSECTORNUM));
		if (rc != 0) {
			return rc;
		}
		rc = sio_rread (raid,
		                payload + ((SUPERSECTORNUM - lba % SUPERSECTORNUM)) * SECTORLENGTH,
		                lba + (SUPERSECTORNUM - lba % SUPERSECTORNUM),
		                lba_count - (SUPERSECTORNUM - lba % SUPERSECTORNUM));
		if (rc != 0) {
			return rc;
		}
	} else {  // perfectly alligned
		int k = 0;
		while (lba_count) {
			i      = super_getdisk (raid, lba);
			dstlba = super_getdisklba (raid, lba);
			rc     = spdk_nvme_ns_cmd_read (raid->disk[i].ns,
			                            raid->disk[i].qpair,
			                            payload,
			                            dstlba,
			                            SUPERSECTORNUM,
			                            sio_read_complete,
			                            &task_completed,
			                            0);
			if (rc != 0) {
				fprintf (stderr, "read I/O failed\n");
				return rc;
			}
			// move offsets
			lba_count -= SUPERSECTORNUM;
			lba += SUPERSECTORNUM;
			payload += SUPERSECTORNUM * SECTORLENGTH;

			if (++k >
			    raid->numdisks * 128) {  // a 128 deep queue, should "flush" for each 128 requests
				k = 0;
				// check completations
				for (i = 0; i < raid->numdisks; i++) {
					spdk_nvme_qpair_process_completions (raid->disk[i].qpair, 0);
				}
			}
		}
	}

	// check completations
	for (i = 0; i < raid->numdisks; i++) {
		spdk_nvme_qpair_process_completions (raid->disk[i].qpair, 0);
	}
	return 0;
}

int sio_rwrite (nvmeRaid* restrict raid, void* restrict payload, uint64_t lba, uint32_t lba_count) {
	int rc, i = 0;
	uint64_t dstlba;
	printf ("DEBUG: sio_rwrite call. lba=%lu ; lba_count=%u\n", lba, lba_count);

	if (lba_count <= (SUPERSECTORNUM - lba)) {
		task_scheduled++;
		i      = super_getdisk (raid, lba);
		dstlba = super_getdisklba (raid, lba);
		rc     = spdk_nvme_ns_cmd_write (raid->disk[i].ns,
		                             raid->disk[i].qpair,
		                             payload,
		                             dstlba,
		                             lba_count,
		                             sio_write_complete,
		                             &task_completed,
		                             0);
		printf ("DEBUG: writing %u sectors @ sector %lu in sector %lu of disk %d\n",
		        lba_count,
		        lba,
		        dstlba,
		        i);
		if (rc != 0) {
			fprintf (stderr, "starting write I/O failed\n");
			return rc;
		}
	} else if (lba % SUPERSECTORNUM) {  // unalligned
		rc = sio_rwrite (raid, payload, lba, (SUPERSECTORNUM - lba % SUPERSECTORNUM));
		if (rc != 0) {
			return rc;
		}
		rc = sio_rwrite (raid,
		                 payload + ((SUPERSECTORNUM - lba % SUPERSECTORNUM)) * SECTORLENGTH,
		                 lba + (SUPERSECTORNUM - lba % SUPERSECTORNUM),
		                 lba_count - (SUPERSECTORNUM - lba % SUPERSECTORNUM));
		if (rc != 0) {
			return rc;
		}
	} else {  // perfectly alligned
		int k = 0;
		while (lba_count) {
			i      = super_getdisk (raid, lba);
			dstlba = super_getdisklba (raid, lba);
			rc     = spdk_nvme_ns_cmd_write (raid->disk[i].ns,
			                             raid->disk[i].qpair,
			                             payload,
			                             dstlba,
			                             SUPERSECTORNUM,
			                             sio_write_complete,
			                             &task_completed,
			                             0);

			printf ("DEBUG2: writing %lu sectors @ sector %lu in sector %lu of disk %d\n",
			        SUPERSECTORNUM,
			        lba,
			        dstlba,
			        i);
			if (rc != 0) {
				fprintf (stderr, "write I/O failed\n");
				return rc;
			}
			// move offsets
			lba_count -= SUPERSECTORNUM;
			lba += SUPERSECTORNUM;
			payload += SUPERSECTORNUM * SECTORLENGTH;

			if (++k >
			    raid->numdisks * 128) {  // a 128 deep queue, should "flush" for each 128 requests
				k = 0;
				// check completations
				for (i = 0; i < raid->numdisks; i++) {
					spdk_nvme_qpair_process_completions (raid->disk[i].qpair, 0);
				}
			}
		}
	}

	// check completations
	for (i = 0; i < raid->numdisks; i++) {
		spdk_nvme_qpair_process_completions (raid->disk[i].qpair, 0);
	}
	return 0;
}

// Copy into pinned memory
int sio_rread_pinit (nvmeRaid* restrict raid,
                     void* restrict payload,
                     uint64_t lba,
                     uint32_t lba_count) {
	int ret   = 0;
	void* mem = spdk_malloc (GIGASECTORLENGTH, SECTORLENGTH, NULL);

	if (lba_count < GIGASECTORLENGTH) {
		ret = sio_rread (raid, mem, lba, lba_count);
		memcpy (payload, mem, lba_count * SECTORLENGTH);

	} else {
		while (lba_count >= GIGASECTORLENGTH) {
			ret += sio_rread (raid, mem, lba, GIGASECTORLENGTH);
			memcpy (payload, mem, GIGASECTORLENGTH * SECTORLENGTH);

			lba_count -= GIGASECTORLENGTH;
			lba += GIGASECTORLENGTH;
			payload += GIGASECTORLENGTH * SECTORLENGTH;

			sio_waittasks (raid);
		}
		if (lba_count)
			ret += sio_rread_pinit (raid, payload, lba, lba_count);
	}

	spdk_free (mem);
	return ret;
}
int sio_rwrite_pinit (nvmeRaid* restrict raid,
                      void* restrict payload,
                      uint64_t lba,
                      uint32_t lba_count) {
	int ret   = 0;
	void* mem = spdk_malloc (GIGASECTORLENGTH, SECTORLENGTH, NULL);

	if (lba_count < GIGASECTORLENGTH) {
		memcpy (payload, mem, lba_count * SECTORLENGTH);
		ret = sio_rwrite (raid, mem, lba, lba_count);

	} else {
		while (lba_count >= GIGASECTORLENGTH) {
			memcpy (payload, mem, GIGASECTORLENGTH * SECTORLENGTH);
			ret += sio_rwrite (raid, mem, lba, GIGASECTORLENGTH);

			lba_count -= GIGASECTORLENGTH;
			lba += GIGASECTORLENGTH;
			payload += GIGASECTORLENGTH * SECTORLENGTH;

			sio_waittasks (raid);
		}
		if (lba_count)
			ret += sio_rwrite_pinit (raid, payload, lba, lba_count);
	}

	spdk_free (mem);
	return ret;
}

// Wait until all tasks finishes
void sio_waittasks (nvmeRaid* raid) {
	int i;
	while (!task_completed < task_scheduled) {
		for (i = 0; i < raid->numdisks; i++) {
			spdk_nvme_qpair_process_completions (raid->disk[i].qpair, 0);
		}
	}
}
