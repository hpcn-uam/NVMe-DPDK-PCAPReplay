#ifndef __common_h__
#define __common_h__

#include <search.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define CURVERSION 1

#define MAGICNUMBER 0xCACA0FE0
#define METASECTORLENGTH 512
#define NAMELENGTH 12
#define MAXFILES 2

#define MAXDISKS 8

typedef struct {
	char name[NAMELENGTH];
	uint64_t startBlock;
	uint64_t endBlock;
} metaFile;

typedef struct {
	uint32_t MAGIC;
	uint8_t version;
	uint8_t diskId;
	uint8_t totalDisks;
	uint8_t totalFiles;
	metaFile content[MAXFILES];
} metaSector;

typedef struct {
	metaSector disk[MAXDISKS];
	int numdisks;
	int numFiles;
	uint64_t totalBlocks; //unset
} nvmeRaid;

void checkMetaConfig (void);
int checkMeta (metaSector* m);
void initMeta (metaSector* m, uint8_t diskId, uint8_t totalDisks);

void createRaid (nvmeRaid* raid);
uint64_t blocksLeft (nvmeRaid* raid);
uint64_t rightFreeBlocks (nvmeRaid* raid); //the rightest contiguous free blocks (the number of)
uint64_t rightFreeBlock (nvmeRaid* raid); //the rightest contiguous free block (which is)
metaFile* findFile (nvmeRaid* raid, char* name);
uint8_t findFileDisk (nvmeRaid* raid, char* name);
metaFile* addFile (nvmeRaid* raid, char* name, uint64_t blsize);
uint8_t delFile (nvmeRaid* raid, char* name);

#endif