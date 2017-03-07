#ifndef __fs_h__
#define __fs_h__

#include <search.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

// FS config
#define CURVERSION 1
#define NAMELENGTH 26
#define MAXFILES 12
#define MAXDISKS 8

// Meta sectors
#define MAGICNUMBER 0xCACA0FE0
#define SECTORLENGTH 512lu
#define METASECTORLENGTH SECTORLENGTH

// Super sectors
#define SUPERSECTORLENGTH (128lu * 1024lu)  // 128K that is the best for performance
#define SUPERSECTORNUM (SUPERSECTORLENGTH / METASECTORLENGTH)

// Giga sectors
#define GIGASECTORLENGTH (SUPERSECTORLENGTH * raid->numdisks)
#define GIGASECTORNUM (SUPERSECTORNUM * raid->numdisks)

typedef struct __attribute__ ((__packed__)) {
	char name[NAMELENGTH];
	uint64_t startBlock;
	uint64_t endBlock;
} metaFile;

typedef struct __attribute__ ((__packed__)) {
	uint32_t MAGIC;
	uint8_t version;
	uint8_t diskId;
	uint8_t totalDisks;
	uint8_t totalFiles;
	metaFile content[MAXFILES];
} metaSector;

typedef struct {
	metaSector msector;
	struct spdk_nvme_ctrlr* ctrlr;
	struct spdk_nvme_ns* ns;
	struct spdk_nvme_qpair* qpair;
} idisk;

typedef struct {
	idisk disk[MAXDISKS];
	int numdisks;
	int numFiles;
	uint64_t totalBlocks;  // unset
} nvmeRaid;

void checkMetaConfig (void);
int checkMeta (metaSector* m);
void initMeta (metaSector* m, uint8_t diskId, uint8_t totalDisks);

void formatRaid (nvmeRaid* raid);
void createRaid (nvmeRaid* raid);
void updateRaid (nvmeRaid* raid);

uint64_t blocksLeft (nvmeRaid* raid);
uint64_t rightFreeBlocks (nvmeRaid* raid);  // the rightest contiguous free blocks (the number of)
uint64_t rightFreeBlock (nvmeRaid* raid);   // the rightest contiguous free block (which is)
metaFile* findFile (nvmeRaid* raid, const char* const name);
uint8_t findFileDisk (nvmeRaid* raid, const char* const name);
metaFile* addFile (nvmeRaid* raid, const char* const name, uint64_t blsize);
uint8_t delFile (nvmeRaid* raid, const char* const name);

#include <simpleio.h>

// utility functions
uint64_t super_getid (nvmeRaid* raid, uint64_t lba);
uint64_t super_getdisk (nvmeRaid* raid, uint64_t lba);
uint64_t super_getdisklba (nvmeRaid* raid, uint64_t lba);

#endif