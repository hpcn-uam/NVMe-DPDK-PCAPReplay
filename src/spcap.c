#include <spcap.h>
#include <common.h>

#include "spdk/nvme.h"
#include "spdk/env.h"

#define BUFFSIZE (SUPERSECTORLENGTH * NUMBUFS)

/*Common*/
int initSpcap (spcap* restrict spcapf, nvmeRaid* restrict raid, metaFile* restrict file) {
	int i;
	bzero (spcapf, sizeof (spcap));  // set everything to 0

	spcapf->raid = raid;
	spcapf->file = file;
	for (i = 0; i < raid->numdisks; i++) {
		uint64_t currentlba = super_getdisklba (raid, file->startBlock + i * SUPERSECTORNUM);
		uint64_t currentDsk = super_getdisk (raid, file->startBlock + i * SUPERSECTORNUM);

		spcapf->currPtr[currentDsk] = spcapf->buffs[currentDsk] =
		    spdk_zmalloc (BUFFSIZE, SUPERSECTORLENGTH, NULL);
		if (!spcapf->currPtr[currentDsk])
			return -1;
		spcapf->dataWrote[currentDsk] = 0;
		spcapf->curlba[currentDsk]    = currentlba;
	}
	return 0;
}
void freeSpcap (spcap* spcapf) {
	int i;
	for (i = 0; i < spcapf->raid->numdisks; i++) {
		spcap_header header = {.nsw8 = 0, .size = 0, .esize = 0};
		writeBuff (spcapf, i, sizeof (spcap_header), &header);  // write header
	}
	flushBuffs (spcapf);
	for (i = 0; i < spcapf->raid->numdisks; i++) {
		if (spcapf->buffs[i])
			spdk_free (spcapf->buffs[i]);
	}
	// spcapf->file->endBlock = (spcapf->file->endBlock - spcapf->file->startBlock) % SUPERSECTORNUM
	// *
	//                         spcapf->raid->numdisks;
	updateRaid (spcapf->raid);
}

/*utils*/
inline uint_fast16_t spcapDstDisk (spcap* spcapf) {
	uint_fast16_t diskId = 0;
	uint64_t dw          = spcapf->dataWrote[0];
	int i;

	// TODO: Make it full-preentive
	for (i = 0; i < spcapf->raid->numdisks; i++) {
		if (spcapf->dataWrote[i] < dw) {
			dw     = spcapf->dataWrote[i];
			diskId = i;
		}
	}

	return diskId;
}

/*Write*/
void flushBuffs (spcap* spcapf) {
	int diskid;

	for (diskid = 0; diskid < spcapf->raid->numdisks; diskid++) {
		if (spcapf->dataWrote[diskid] % SUPERSECTORLENGTH) {  // If it is != 0, then not flushed
			if (sio_write (&spcapf->raid->disk[diskid],
			               spcapf->currPtr[diskid] - spcapf->dataWrote[diskid] % SUPERSECTORLENGTH,
			               spcapf->curlba[diskid],
			               SUPERSECTORNUM)) {
				printf ("Error writing to raid PCAP packets\n");
			}
			spcapf->curlba[diskid] += SUPERSECTORNUM;
			spcapf->file->endBlock += SUPERSECTORNUM;
			if (spcapf->currPtr[diskid] == spcapf->buffs[diskid] + BUFFSIZE) {
				spcapf->currPtr[diskid] = spcapf->buffs[diskid];
			}
		}
	}
}

inline void writeBuff (spcap* restrict spcapf,
                       uint_fast16_t diskid,
                       uint_fast16_t size,
                       void* restrict payload) {
	// Write data
	uint_fast16_t toWrite =
	    (spcapf->dataWrote[diskid] % SUPERSECTORLENGTH + size) > SUPERSECTORLENGTH
	        ? SUPERSECTORLENGTH - (spcapf->dataWrote[diskid] % SUPERSECTORLENGTH)
	        : size;
	size -= toWrite;

	memcpy (spcapf->currPtr[diskid], payload, toWrite);
	spcapf->dataWrote[diskid] += toWrite;
	spcapf->currPtr[diskid] += toWrite;

	// Flush data, if necessary
	if (!spcapf->dataWrote[diskid] % SUPERSECTORLENGTH) {
		if (sio_write (&spcapf->raid->disk[diskid],
		               spcapf->currPtr[diskid] - SUPERSECTORLENGTH,
		               spcapf->curlba[diskid],
		               SUPERSECTORNUM)) {
			printf ("Error writing to raid PCAP packets\n");
		}
		spcapf->curlba[diskid] += SUPERSECTORNUM;
		spcapf->file->endBlock += SUPERSECTORNUM;
		if (spcapf->currPtr[diskid] == spcapf->buffs[diskid] + BUFFSIZE) {
			spcapf->currPtr[diskid] = spcapf->buffs[diskid];
		}
	}

	// Write left data
	if (size)  // we'll asume that there wont be a packet that overflows 2 buffers...
		writeBuff (spcapf, diskid, size, payload);
}

inline void writePkt (spcap* restrict spcapf,
                      uint_fast32_t nsw8,
                      uint_fast16_t size,
                      uint_fast16_t esize,
                      void* restrict payload) {
	spcap_header header;
	uint_fast16_t dstDisk;

	header.nsw8  = nsw8;
	header.size  = size;
	header.esize = esize;
	dstDisk      = spcapDstDisk (spcapf);

	writeBuff (spcapf, dstDisk, sizeof (spcap_header), &header);  // write header
	writeBuff (spcapf, dstDisk, size, payload);
}

inline void writePCAPPkt (spcap* restrict spcapf,
                          struct pcap_pkthdr* restrict hdr,
                          void* restrict payload) {
	writePkt (spcapf, 0, hdr->len, hdr->caplen, payload);
}

void writePCAP2raid (spcap* spcapf, char* filename) {
	char errbuf[PCAP_ERRBUF_SIZE];
	pcap_t* pcap;
	struct pcap_pkthdr header;
	void* packet;

	pcap = pcap_open_offline (filename, errbuf);
	if (pcap == NULL) {
		fprintf (stderr, "error reading pcap file: %s\n", errbuf);
		exit (1);
	}

	while ((packet = (void*)pcap_next (pcap, &header)) != NULL)
		writePCAPPkt (spcapf, &header, packet);
}

/*Read*/