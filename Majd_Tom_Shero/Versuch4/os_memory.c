#include "os_memory_strategies.h"
#include "os_memory.h"
#include "util.h"
#include "os_core.h"
/*! \file
 *  \brief Contains functions to allocate blocks of any spos-memory-drivers.
 *	
 *	We manage the blocks by creating a lookup table with one nibble per byte payload. 
 *	So we have 1/3 map and 2/3 use-heap of each driver.
 *
 */



/*!
 *	Allocates a chunk of memory on the medium given by the driver and reserves it for the current process.
 * 
 *	\param	heap	The driver to be used.
 *	\param	size	The amount of memory to be allocated in Bytes. Must be able to handle a single byte and values greater than 255.
 *	\return A pointer to the first Byte of the allocated chunk.
 *			0 if allocation fails (0 is never a valid address).
*/
MemAddr os_malloc(Heap* heap, uint16_t size){
	os_enterCriticalSection();
	// check for allocation strategy and return initial address 
	switch (heap->allocStrat) {
		case OS_MEM_FIRST :
			os_leaveCriticalSection();
			return os_Memory_FirstFit(heap, size);		
			break;
		case OS_MEM_BEST :
			os_leaveCriticalSection();
			return os_Memory_BestFit(heap, size);
			break;
		case OS_MEM_NEXT :
			os_leaveCriticalSection();
			return os_Memory_NextFit(heap, size);
			break;
		case OS_MEM_WORST :
			os_leaveCriticalSection();
			return os_Memory_WorstFit(heap, size);
			break;
	}
	return 0;
}

/*!
 * Frees a chunk of allocated memory of the currently running process on the given heap.
 *
 * \param	heap	The driver to be used.
 * \param	addr	An address inside of the chunk (not necessarily the start).
*/
void os_free(Heap* heap, MemAddr addr){
	os_enterCriticalSection();
	MemAddr ptr = os_getFirstByteOfChunk(heap, addr);
	
	MemAddr rproc = getMapEntry(heap, ptr);
	if (rproc != os_getCurrentProc()){
		os_error(" Private Memory Address");
		os_leaveCriticalSection();
		return;
	}
	
	setMapEntry(heap, ptr, 0);
	ptr++;
	while(ptr < (os_getUseSize(heap)+os_getUseStart(heap))) {
		if (getMapEntry(heap, ptr) == 15){
			setMapEntry(heap, ptr, 0);
		} else break;
		ptr++;
	}
	
	// decrement number chunks in specified SRAM
	if (heap == intHeap){
		num_num_chunk_num[0][os_getCurrentProc()] -= 1;
	} else {
		num_num_chunk_num[1][os_getCurrentProc()] -= 1;
	}
		
	os_leaveCriticalSection();
}


/*!
 *	Takes a use-pointer and computes the length of the chunk. 
 *	This only works for occupied chunks. The size of free chunks is always 0.
 *
 *	\param	 heap	The driver to be used.
 *	\param	 addr	An address of the use-heap.
 *	\return	 The chunk's length.
 */
uint16_t os_getChunkSize(Heap const* heap, MemAddr addr){
	os_enterCriticalSection();
	// teste ob übergebene addr in start+mapsize - start+usesize+mapsize
	if (addr > (os_getUseSize(heap)+os_getUseStart(heap))){
		os_error(" Invalid Address! ");
		os_leaveCriticalSection();
		return 0;
	}
	
	MemAddr ptr = os_getFirstByteOfChunk(heap, addr);
	if (ptr == 0){
		os_leaveCriticalSection();
		return 0;
	}
	
	uint16_t counter = 1;
	ptr++;
	while(ptr < os_getUseSize(heap)+os_getUseStart(heap)) {		
		if (getMapEntry(heap, ptr) == 0b00001111) {
			counter++;
		} 
		else break;
		ptr++;
	}
	
	os_leaveCriticalSection();
	return counter;
}

/*!
 *	The Heap-map-size of the heap (needed by the taskmanager)
 *  \param	heap	The heap to be used.
 *	\returns The size of the map of the heap.
 */
size_t os_getMapSize(Heap const* heap){
	return heap->map_size;
}

/*!
 *	The Heap-use-size of the heap (needed by the taskmanager)
 *	\param heap	The heap to be used.
 *	\returns The size of the use-area of the heap
 */
size_t os_getUseSize(Heap const* heap){
	return heap->data_block;
}

/*!
 *	The Map-start of the heap (needed by the taskmanager)
 *
 *	\param heap	The heap to be used.
 *	\returns The first byte of the map of the heap
 */
MemAddr os_getMapStart(Heap const* heap){
	return heap->start_address;
}

/*!
 *	The Heap-use-start of the heap (needed by the taskmanager)
 *	
 *	\param heap	The heap to be used.
 *	\returns The first byte of the use-area of the heap
 */
MemAddr os_getUseStart(Heap const* heap){
	return (heap->start_address + heap->map_size);
}

/*!
 *	This function is used to determine where a chunk starts if a given address might not point to the 
 *	start of the chunk but to some place inside of it.
 *
 *	\param heap	The heap the chunk is on hand in
 *	\param addr	The address that points to some byte of the chunk
 *	\returns The address that points to the first byte of the chunk
 */
MemAddr os_getFirstByteOfChunk (Heap const *heap, MemAddr addr){
	os_enterCriticalSection();
	// test whether addr in is in use space
	if (addr > os_getUseSize(heap) + os_getUseStart(heap) && addr < os_getUseStart(heap)){
		os_error(" Invalid Address! ");
		os_leaveCriticalSection();
		return 0;
	}
	MemAddr ptr = addr;

	// return error in case address not allocated
	if (getMapEntry(heap, ptr) == 0){
		os_leaveCriticalSection();
		return 0;
	}

	// Calculate first address!
	while (ptr >= os_getUseStart(heap)){
		if (getMapEntry(heap, ptr) == 15) {
			ptr--;
		}
		else break;
	}
	os_leaveCriticalSection();
	return ptr;
}

/*!
 *	This function determines the value of the first nibble of a chunk.
 *	
 *	\param heap	The heap the chunk is on hand in
 *	\param addr	The address that points to some byte of the chunk
 *	\returns The map entry that corresponds to the first byte of the chunk
 */
ProcessID 	getOwnerOfChunk (Heap const *heap, MemAddr addr){
	MemAddr tmp = os_getFirstByteOfChunk(heap, addr);
	if ((tmp - os_getUseStart(heap)) % 2 == 0){
		return getHighNibble(heap, ((tmp - os_getUseStart(heap))/ 2) + os_getMapStart(heap));
	} else {
		return getLowNibble(heap, ((tmp - os_getUseStart(heap))/ 2) + os_getMapStart(heap));
	}
	return 0;
}

/*!
 *	\param heap	The heap that is written to
 *	\param addr	The address on which the lower nibble is supposed to be changed
 *	\param value	The value that the lower nibble of the given addr is supposed to get
 */
void setLowNibble (Heap const *heap, MemAddr addr, MemValue value){
	MemValue tmp = getHighNibble(heap, addr) << 4;
	value &= 0b00001111;
	heap->driver->write(addr, value | tmp);
}

/*!
 \param heap	The heap that is written to
 \param addr	The address on which the higher nibble is supposed to be changed
 \param value	The value that the higher nibble of the given addr is supposed to get
 */
void setHighNibble (Heap const *heap, MemAddr addr, MemValue value){
	MemValue tmp = getLowNibble(heap, addr);
	value = value << 4;
	heap->driver->write(addr, value | tmp);
}

/*!
 *	\param  heap	The heap that is read from
 *	\param	addr	The address which the lower nibble is supposed to be read from
 *	\returns The value that can be found on the lower nibble of the given address
 */
MemValue getLowNibble (Heap const *heap, MemAddr addr){
	return heap->driver->read(addr) & 0b00001111;
}

/*!
 *	\param heap	The heap that is read from
 *	\param addr	The address which the higher nibble is supposed to be read from
 *	returns The value that can be found on the higher nibble of the given address
 */
MemValue getHighNibble (Heap const *heap, MemAddr addr){
	return (heap->driver->read(addr) >> 4) ;
}

/*!
 *	\param heap	The heap on whose map the entry is supposed to be set
 *	\param addr	The address in use space for which the corresponding map entry shall be set
 *	\param value	The value that is supposed to be set onto the map (valid range: 0x0 - 0xF)
 */
void setMapEntry (Heap const *heap, MemAddr addr, MemValue value){
	uint16_t mem = ((addr-os_getUseStart(heap))/2) + os_getMapStart(heap);
	if (((addr-os_getUseStart(heap)) % 2) == 0){
		setHighNibble(heap, mem, value);
	} else {
		setLowNibble(heap, mem, value);
	}
}

/*!
 *	This function is used to get a heap map entry on a specific heap
 *
 *	\param heap	The heap from whose map the entry is supposed to be fetched
 *	\param addr	The address in use space for which the corresponding map entry shall be fetched
 *	\returns The value that can be found on the heap map entry that corresponds to the given use space address
 */
MemValue getMapEntry(Heap const *heap, MemAddr addr) {
	uint16_t mem = ((addr-os_getUseStart(heap))/2) + os_getMapStart(heap);
	if (((addr - os_getUseStart(heap)) % 2) == 0){
		return getHighNibble(heap, mem);
	} else {
		return getLowNibble(heap, mem);
	}
}

/*!
 *	Simple getter function to fetch the allocation strategy of a given heap
 *	\param heap	The heap of which the allocation strategy is returned
 *	\returns The allocation strategy of the given heap
 */
AllocStrategy os_getAllocationStrategy(Heap const* heap){
	return heap->allocStrat;
}

/*!
 *	Simple setter function to change the allocation strategy of a given heap
 *	\param heap	The heap of which the allocation strategy shall be changed
 *	\param allocStrat	The strategy is changed to allocStrat
*/
void os_setAllocationStrategy(Heap *heap, AllocStrategy allocStrat){
	heap->allocStrat = allocStrat;
}

/*!
 *	This function realises the garbage collection. When called, every allocated memory chunk of the given process is freed
 *	\param heap	The heap on which we look for allocated memory
 *	\param pid	The ProcessID of the process that owns all the memory to be freed
 */
void os_freeProcessMemory(Heap *heap, ProcessID pid){
	os_enterCriticalSection();
	MemAddr ptr = os_getUseStart(heap);
	int heapy = (heap == intHeap) ? 0 : 1;
	
	while ((ptr < os_getUseStart(heap) + os_getUseSize(heap)) && (num_num_chunk_num[heapy][pid] > 0)) {
		if ((getMapEntry(heap, ptr)) == 0){
			ptr++;
			continue;
		}
		MemAddr val = os_getChunkSize(heap, ptr);
		os_freeOwnerRestricted(heap, ptr, pid);
		ptr += val;
	}
	os_leaveCriticalSection();
}

/*!
 *	Frees a chunk of allocated memory on the medium given by the driver. 
 *	This function checks if the call has been made by someone with the right to do it 
 *	(i.e. the process that owns the memory or the OS). 
 *	This function is made in order to avoid code duplication and is called by several functions that, 
 *	in some way, free allocated memory such as os_freeProcessMemory/os_free...
 *
 *	\param heap	The driver to be used.
 *	\param addr	An address inside of the chunk (not necessarily the start).
 *	\param owner	The expected owner of the chunk to be freed
 */
void os_freeOwnerRestricted(Heap *heap, MemAddr addr, ProcessID owner){
	os_enterCriticalSection();
	MemAddr ptr = addr;
	if ( getMapEntry(heap, addr) == 15 ){
		ptr = os_getFirstByteOfChunk(heap, addr);
	}
	if (owner != getMapEntry(heap, ptr)){
		os_leaveCriticalSection();
		return;
	}
	
	setMapEntry(heap, ptr, 0);
	ptr++;
	while(ptr < os_getUseStart(heap)+os_getUseSize(heap)) {
		
		if (getMapEntry(heap, ptr) == 15){
			setMapEntry(heap, ptr, 0);
		} else break;
		ptr++;
	}
	
	if (heap == intHeap){
		num_num_chunk_num[0][owner] -= 1;
	} else {
		num_num_chunk_num[1][owner] -= 1;
	}
	
	os_leaveCriticalSection();
}

/*!
 * This is an efficient reallocation routine. It is used to resize an existing allocated chunk of memory. 
 * If possible, the position of the chunk remains the same.
 *
 heap	The heap on which the reallocation is performed
 addr	One address inside of the chunk that is supposed to be reallocated
 size	The size the new chunk is supposed to have
 Returns
 First address (in use space) of the newly allocated chunk
 */
MemAddr os_realloc(Heap* heap, MemAddr addr, uint16_t size){
	os_enterCriticalSection();
	uint16_t chunk_size = os_getChunkSize(heap, addr);
	MemAddr chunk_start = os_getFirstByteOfChunk(heap, addr);
	if (size > chunk_size) {
		// test if space exists after chunk
		uint16_t gap = 0;
		uint16_t gap_before = 0;
		for (MemAddr i = (chunk_start+chunk_size); i < size+chunk_start; i++)
		{
			if (i >= os_getUseSize(heap) + os_getUseStart(heap)) break;
			if(getMapEntry(heap, i) == 0) gap++;
			else break;
		}
		// test if space exists before chunk
		for (MemAddr i = chunk_start-1; i >= os_getUseStart(heap); i--)	{
			//if (i < os_getUseStart(heap)) break;
			if(getMapEntry(heap, i) == 0) gap_before++;
			else break;
		}
		
		// if it is possible to allocate after chunk then do so.
		if (gap >= size - chunk_size){
			for (MemAddr i = (chunk_start+chunk_size); i < size+chunk_start; i++) {
				setMapEntry(heap, i, 0xff);
			}	
			os_leaveCriticalSection();
			return chunk_start;
		}
		if (gap_before + gap >= size-chunk_size){
			// move the chunk left
			moveChunk(heap, chunk_start, chunk_size, (chunk_start-gap_before), size);
			
			os_leaveCriticalSection();
			return (chunk_start-(gap_before));
		}
		
		MemAddr new_chunk = os_malloc(heap, size);
		if (new_chunk == 0){
			os_leaveCriticalSection();
			return 0;
		}
		moveChunk(heap, chunk_start, chunk_size, new_chunk, size);
		os_leaveCriticalSection();
		return new_chunk;
	} else {
		if (size == chunk_size){
			os_leaveCriticalSection();
			return chunk_start;
		}
		for (MemAddr ptr = (chunk_start+size); ptr < chunk_start+chunk_size; ptr++){
			setMapEntry(heap, ptr, 0);
		}
		os_leaveCriticalSection();
		return chunk_start;
	}
}


void moveChunk (Heap *heap, MemAddr oldChunk, size_t oldSize, MemAddr newChunk, size_t newSize){
	os_enterCriticalSection();
	if (oldSize > newSize) {
		os_leaveCriticalSection();
		return;
	}
	MemAddr old_ptr = oldChunk;
	MemAddr ptr = newChunk;
	while(ptr < newChunk+oldSize){
		// adjust map entries 
		if (ptr == newChunk){
			setMapEntry(heap, ptr, os_getCurrentProc());
		} else {
			setMapEntry(heap, ptr, 0xff);
		}
		setMapEntry(heap, old_ptr, 0);
		
		// adjust use space (memcpy)
		MemValue memcopy = heap->driver->read(old_ptr);
		heap->driver->write(ptr, memcopy);
		heap->driver->write(old_ptr, 0);
		old_ptr++;
		ptr++;	
	}
	for (MemAddr ptr = newChunk+oldSize; ptr < newChunk+newSize; ptr++){
		setMapEntry(heap, ptr, 0xff);
	}
	
	os_leaveCriticalSection();
}