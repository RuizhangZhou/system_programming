#ifndef _OS_MEMORY_H
#define _OS_MEMORY_H

#include "os_mem_drivers.h"
#include "os_memheap_drivers.h"
#include "os_process.h"
#include <stdint.h>
#include <stddef.h>


MemAddr os_malloc(Heap* heap, uint16_t size);

void os_free(Heap* heap, MemAddr addr);

/*
This is an efficient reallocation routine. It is used to resize an existing allocated chunk of memory. 
If possible, the position of the chunk remains the same. 
It is only searched for a completely new chunk if everything else 
does not fit For a more detailed description please use the exercise document.
*/
MemAddr os_realloc (Heap *heap, MemAddr addr, uint16_t size);

size_t os_getMapSize(Heap const* heap);
size_t os_getUseSize(Heap const* heap);
MemAddr os_getMapStart(Heap const* heap);
MemAddr os_getUseStart(Heap const* heap);

uint16_t os_getChunkSize(Heap const* heap, MemAddr addr);
MemAddr os_getFirstByteOfChunk(Heap const* heap, MemAddr addr);
ProcessID getOwnerOfChunk(Heap* heap, MemAddr addr);

AllocStrategy os_getAllocationStrategy(Heap const* heap);
void os_setAllocationStrategy(Heap *heap, AllocStrategy allocStrat);

/*
Frees a chunk of allocated memory on the medium given by the driver. 
This function checks if the call has been made by someone with the right to do it 
(i.e. the process that owns the memory or the OS). 
This function is made in order to avoid code duplication and is called by several functions that, 
in some way, free allocated memory such as os_freeProcessMemory/os_free...
*/
void os_freeOwnerRestricted(Heap* heap, MemAddr addr, ProcessID owner);

/*
This function realises the garbage collection. 
When called, every allocated memory chunk of the given process is freed
*/
void os_freeProcessMemory(Heap* heap, ProcessID pid);

MemValue getMapEntry(Heap const* heap, MemAddr addr);

/*Parameters
heap	The heap on whos map the entry is supposed to be set
addr	The address in use space for which the corresponding map entry shall be set
value	The value that is supposed to be set onto the map (valid range: 0x0 - 0xF)
*/
void setMapEntry(Heap const* heap, MemAddr addr, MemValue value);



#endif