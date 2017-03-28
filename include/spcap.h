#ifndef __spcap_h__
#define __spcap_h__

#include <common.h>
#include <pcap.h>

#define spcap_ext ".scap"

#define NUMBUFS 3

typedef struct {
	nvmeRaid* raid;
	metaFile* file;
	char* buffs[MAXDISKS];
	char* currPtr[MAXDISKS];
	uint64_t curlba[MAXDISKS];
	uint64_t dataWrote[MAXDISKS];
} spcap;

typedef struct {
	uint32_t nsw8;
	uint16_t size;
	uint16_t esize;
} spcap_header;

/*Common*/
int initSpcap (spcap* spcapf, nvmeRaid* raid, metaFile* file);
void freeSpcap (spcap* spcapf);

/*utils*/
uint_fast16_t spcapDstDisk (spcap* spcapf);

/*Write*/
void flushBuffs (spcap* spcapf);
void writeBuff (spcap* spcapf, uint_fast16_t diskid, uint_fast16_t size, void* payload);
void writePkt (
    spcap* spcapf, uint_fast32_t nsw8, uint_fast16_t size, uint_fast16_t esize, void* payload);
void writePCAPPkt (spcap* spcapf, struct pcap_pkthdr* hdr, void* payload);
void writePCAP2raid (spcap* spcapf, char* filename);

/*Read*/

#endif