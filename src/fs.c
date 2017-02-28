#include <fs.h>

void checkMetaConfig (void) {
	if (sizeof (metaSector) != METASECTORLENGTH) {
		fprintf (
		    stderr, "Invalid meta-data-size (%lu != %lu)\n", sizeof (metaSector), METASECTORLENGTH);
		fprintf (stderr, "Each file data has %lu bytes\n", sizeof (metaFile));
		exit (-1);
	}
}

int checkMeta (metaSector *m) {
	checkMetaConfig ();
	return m->MAGIC == MAGICNUMBER;
}

void initMeta (metaSector *m, uint8_t diskId, uint8_t totalDisks) {
	m->MAGIC      = MAGICNUMBER;
	m->version    = CURVERSION;
	m->diskId     = diskId;
	m->totalDisks = totalDisks;
	m->totalFiles = 0;

	int i;
	for (i = 0; i < MAXFILES; i++) {
		m->content[i].name[0]    = '\0';
		m->content[i].startBlock = 0;
		m->content[i].endBlock   = 0;
	}
}

// For sort
static int cmpMetaSector (const void *p1, const void *p2) {
	return ((const metaSector *)p1)->diskId > ((const metaSector *)p2)->diskId;
}
static int cmpIDisk (const void *p1, const void *p2) {
	return cmpMetaSector (&((const idisk *)p1)->msector, &((const idisk *)p2)->msector);
}

// For search
static int cmpFile (const void *p1, const void *p2) {
	return strncmp (((const metaFile *)p1)->name, ((const metaFile *)p2)->name, NAMELENGTH);
}

void createRaid (nvmeRaid *raid) {
	int i, cnt = 0;

	// int8_t isInit[MAXDISKS] = {0};

	for (i = 0; i < raid->numdisks; i++) {
		if (sio_sectorSize (&raid->disk[i]) != METASECTORLENGTH) {
			printf ("Disk %d have an invalid sector size (!=%lu)", i, METASECTORLENGTH);
		}

		sio_read_pinit (&raid->disk[i], &raid->disk[i].msector, 0, 1);
		if (checkMeta (&raid->disk[i].msector)) {  // initialiced
			// isInit[i] = 1;
			cnt++;
		} else {
			// isInit[i] = 0;
		}
	}

	if (cnt == 0) {  // all must be initialiced
		for (i = 0; i < raid->numdisks; i++) {
			initMeta (&raid->disk[i].msector, i, raid->numdisks);
			sio_write_pinit (&raid->disk[i], &raid->disk[i].msector, 0, 1);
			printf ("Overwritting sector 0 of disk %d\n", i);
		}
	} else if (cnt < raid->numdisks) {
		puts (
		    "This implementation can't handle this NVME situation. Plase, consider attaching only "
		    "the initialiced NVMes or clean its metadata (Which will erase all its contents)\n"
		    "In future realeases, increasing the number of NVMes in raid would be supported");
		exit (-1);
	}

	// order
	qsort (raid->disk, raid->numdisks, sizeof (idisk), cmpIDisk);

	// check integrity
	for (i = 0; i < raid->numdisks; i++) {
		if (raid->disk[i].msector.diskId != i &&
		    raid->disk[i].msector.totalDisks != raid->numdisks) {
			puts ("NVMe raid integrity error. Can't continue");
			exit (-1);
		}
	}

	// fill other raid data
	raid->numFiles = raid->disk[0].msector.totalFiles;
}

void updateRaid (nvmeRaid *raid) {
	int i;
	for (i = 0; i < raid->numdisks; i++) {
		sio_write_pinit (&raid->disk[i], &raid->disk[i].msector, 0, 1);
	}
}

uint64_t blocksLeft (nvmeRaid *raid) {
	uint64_t usedBlocks = 0;
	int i, j;
	for (i = 0; i < raid->numdisks; i++) {
		for (j = 0; j < MAXFILES; j++) {
			usedBlocks = raid->disk[i].msector.content[j].endBlock -
			             raid->disk[i].msector.content[j].startBlock;
		}
	}
	return raid->totalBlocks - usedBlocks;
}

uint64_t rightFreeBlocks (nvmeRaid *raid) {
	return raid->totalBlocks - rightFreeBlock (raid);
}

uint64_t rightFreeBlock (nvmeRaid *raid) {
	uint64_t mostRight = 0;
	int i, j;
	for (i = 0; i < raid->numdisks; i++) {
		for (j = 0; j < MAXFILES; j++) {
			if (mostRight < raid->disk[i].msector.content[j].endBlock)
				mostRight = raid->disk[i].msector.content[j].endBlock;
		}
	}
	return mostRight;
}

metaFile *findFile (nvmeRaid *raid, char *name) {
	int i;
	size_t numfiles = MAXFILES;
	metaFile *ret = NULL;
	for (i = 0; i < raid->numdisks; i++) {
		ret = lfind (name, &raid->disk[i].msector.content, &numfiles, sizeof (metaFile), cmpFile);
		if (ret)
			break;
	}
	return ret;
}

uint8_t findFileDisk (nvmeRaid *raid, char *name) {
	int i;
	size_t numfiles = MAXFILES;
	metaFile *ret = NULL;
	for (i = 0; i < raid->numdisks; i++) {
		ret = lfind (name, &raid->disk[i].msector.content, &numfiles, sizeof (metaFile), cmpFile);
		if (ret)
			break;
	}
	return raid->disk[i].msector.diskId;
}

metaFile *addFile (nvmeRaid *raid, char *name, uint64_t blsize) {
	int i, j;

	// TODO: set errno to the specific error
	if (findFile (raid, name))
		return NULL;
	if (raid->disk[0].msector.totalFiles == MAXFILES * raid->numdisks)
		return NULL;
	// Check for space
	if (rightFreeBlocks (raid) < blsize)
		return NULL;
	// find a place for it
	for (i = 0; i < raid->numdisks; i++) {
		for (j = 0; j < MAXFILES; j++) {
			if (raid->disk[i].msector.content[j].name[0] == 0) {
				memcpy (raid->disk[i].msector.content[j].name, name, NAMELENGTH);
				raid->disk[i].msector.content[j].startBlock = rightFreeBlock (raid);
				raid->disk[i].msector.content[j].endBlock =
				    raid->disk[i].msector.content[j].startBlock + blsize;
				raid->disk[0].msector.totalFiles++;  // increase the number of files
				raid->numFiles++;

				updateRaid (raid);
				return &raid->disk[i].msector.content[j];
			}
		}
	}
	return NULL;
}

uint8_t delFile (nvmeRaid *raid, char *name) {
	metaFile *f = findFile (raid, name);
	if (f) {
		f->name[0]    = 0;
		f->startBlock = 0;
		f->endBlock   = 0;
		raid->disk[0].msector.totalFiles--;
		raid->numFiles--;

		updateRaid (raid);
		return 1;
	} else {
		return 0;
	}
}