#ifndef __simpleio_h__
#define __simpleio_h__

#include <fs.h>

// Works for pinned memory
int sio_read (idisk* dsk, void* payload, uint64_t lba, uint32_t lba_count);
int sio_write (idisk* dsk, void* payload, uint64_t lba, uint32_t lba_count);

// Copy into pinned memory
int sio_read_pinit (idisk* dsk, void* payload, uint64_t lba, uint32_t lba_count);
int sio_write_pinit (idisk* dsk, void* payload, uint64_t lba, uint32_t lba_count);

#endif