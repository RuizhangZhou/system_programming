#include "os_memheap_drivers.h"
#include "os_mem_drivers.h"
#include "os_scheduler.h"
/*! \file
 *	\brief Contains heap management functionality for the OS.
 *
 */

//! array for number of chunks
uint16_t num_num_chunk_num[2][MAX_NUMBER_OF_PROCESSES];


//! allocate space of size size in heap
MemAddr os_malloc(Heap* heap, uint16_t size);

//! returns size of map
size_t os_getMapSize(Heap const* heap);

//! returns size of use space
size_t os_getUseSize(Heap const* heap);

//! returns beginning address of map 
MemAddr os_getMapStart(Heap const* heap);

//! returns beginning address of use space
MemAddr os_getUseStart(Heap const* heap);

//! frees allocated space from memory
void os_free(Heap* heap, MemAddr addr);

//! returns the allocated chunk size
uint16_t os_getChunkSize(Heap const* heap, MemAddr addr);

//! returns the current allocation strategy 
AllocStrategy os_getAllocationStrategy(Heap const* heap);

//! sets the allocation strategy to allocStrat
void os_setAllocationStrategy(Heap *heap, AllocStrategy allocStrat);

//! Get the address of the first byte of chunk. 
MemAddr 	os_getFirstByteOfChunk (Heap const *heap, MemAddr addr);

//! Get the address of the owner of chunk. 
ProcessID 	getOwnerOfChunk (Heap const *heap, MemAddr addr);

//! set low nibble of an entry
void 	setLowNibble (Heap const *heap, MemAddr addr, MemValue value);

//! set high nibble of an entry
void 	setHighNibble (Heap const *heap, MemAddr addr, MemValue value);

//! get low nibble of an entry 
MemValue 	getLowNibble (Heap const *heap, MemAddr addr);

//! get high nibble of an entry
MemValue 	getHighNibble (Heap const *heap, MemAddr addr);

//! Function used to set the value of a single map entry, this is made public so the allocation strategies can use it.
void 	setMapEntry (Heap const *heap, MemAddr addr, MemValue value);

//! Function used to get the value of a single map entry, this is made public so the allocation strategies can use it.
MemValue 	getMapEntry (Heap const *heap, MemAddr addr);

//! Function that realises the garbage collection
void os_freeProcessMemory(Heap *heap, ProcessID pid);

//! frees chunk with certain owner only.
void os_freeOwnerRestricted(Heap *heap, MemAddr addr, ProcessID owner);

//! 
MemAddr os_realloc(Heap* heap, MemAddr addr, uint16_t size);

//! 
void moveChunk (Heap *heap, MemAddr oldChunk, size_t oldSize, MemAddr newChunk, size_t newSize);