#ifndef __simpleio_h__
#define __simpleio_h__

#include <fs.h>

// Simpliest commands
int sio_sectorSize (idisk* dsk);

// Works for pinned memory
int sio_read (idisk* dsk, void* payload, uint64_t lba, uint32_t lba_count);
int sio_write (idisk* dsk, void* payload, uint64_t lba, uint32_t lba_count);

// Copy into pinned memory
int sio_read_pinit (idisk* dsk, void* payload, uint64_t lba, uint32_t lba_count);
int sio_write_pinit (idisk* dsk, void* payload, uint64_t lba, uint32_t lba_count);

/* Raid writter */
// Works for pinned memory
int sio_rread (nvmeRaid* raid, void* payload, uint64_t lba, uint32_t lba_count);
int sio_rwrite (nvmeRaid* raid, void* payload, uint64_t lba, uint32_t lba_count);

// Copy into pinned memory
int sio_rread_pinit (nvmeRaid* raid, void* payload, uint64_t lba, uint32_t lba_count);
int sio_rwrite_pinit (nvmeRaid* raid, void* payload, uint64_t lba, uint32_t lba_count);

// Wait until all tasks finishes
void sio_waittasks (nvmeRaid* raid);

#endif