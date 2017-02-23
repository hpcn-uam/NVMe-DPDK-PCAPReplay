#ifndef __simpleio_h__
#define __simpleio_h__

#include <fs.h>

int sio_read (idisk* dsk, void* payload, uint64_t lba, uint32_t lba_count);
int sio_write (idisk* dsk, void* payload, uint64_t lba, uint32_t lba_count);

#endif