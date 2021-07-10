#ifndef _OS_MEMORY_H
#define _OS_MEMORY_H

#include "os_mem_drivers.h"
#include "os_memheap_drivers.h"
#include "os_process.h"
#include <stdint.h>
#include <stddef.h>


MemAddr os_malloc(Heap* heap, uint16_t size);

void os_free(Heap* heap, MemAddr addr);

size_t os_getMapSize(Heap const* heap);
size_t os_getUseSize(Heap const* heap);
MemAddr os_getMapStart(Heap const* heap);
MemAddr os_getUseStart(Heap const* heap);

uint16_t os_getChunkSize(Heap const* heap, MemAddr addr);
MemAddr os_getFirstByteOfChunk(Heap const* heap, MemAddr addr);
ProcessID getOwnerOfChunk(Heap* heap, MemAddr addr);

AllocStrategy os_getAllocationStrategy(Heap const* heap);
void os_setAllocationStrategy(Heap *heap, AllocStrategy allocStrat);

//! set low nibble of an entry
void setLowNibble(Heap const *heap, MemAddr addr, MemValue value);
//! set high nibble of an entry
void setHighNibble(Heap const *heap, MemAddr addr, MemValue value);
//! get low nibble of an entry 
MemValue getLowNibble(Heap const *heap, MemAddr addr);
//! get high nibble of an entry
MemValue getHighNibble(Heap const *heap, MemAddr addr);

void os_freeOwnerRestricted(Heap* heap, MemAddr addr, ProcessID owner);
void os_freeProcessMemory(Heap* heap, ProcessID pid);

MemValue getMapEntry(Heap const* heap, MemAddr addr);

/*Parameters
heap	The heap on whos map the entry is supposed to be set
addr	The address in use space for which the corresponding map entry shall be set
value	The value that is supposed to be set onto the map (valid range: 0x0 - 0xF)
*/
void setMapEntry(Heap const* heap, MemAddr addr, MemValue value);



#endif