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


#define SH_MEM_CLOSED			8	// read and write both possible
#define SH_WRITE_OPEN			9	// mem is open, cannot be used 
#define SH_READ_ONE				10  // one process reading shm
#define SH_READ_TWO				11  // two processes reading shm
#define SH_READ_THREE			12	// three processes reading shm
#define SH_READ_FOUR			13	// four processes reading shm

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
		
		// avoid possible free on shared memory 
		if (rproc >= SH_MEM_CLOSED && rproc <= SH_READ_FOUR)
			os_error("SHARED MEMORY    CONFLICT");
		else
			os_error(" Private Memory    Address");	
			
		os_leaveCriticalSection();
		return;
	}
	
	setMapEntry(heap, ptr, 0);
	ptr++;
	while(ptr < (os_getUseSize(heap)+os_getUseStart(heap))) {
		if (getMapEntry(heap, ptr) == 15){
			setMapEntry(heap, ptr, 0);
		} else 
			break;
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
		// check for possible shared memory block or wrong parameter
		MemValue block_owner = getMapEntry(heap, ptr);
		if ((owner >= SH_MEM_CLOSED && owner <= SH_READ_FOUR)
			|| (block_owner >= SH_MEM_CLOSED && block_owner <= SH_READ_FOUR)) 
			os_error("SHARED MEMORY    CONFLICT");
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
	
	if (heap == intHeap && owner < 8){
		num_num_chunk_num[0][owner] -= 1;
	} else {
		num_num_chunk_num[1][owner] -= 1;
	}
	
	os_leaveCriticalSection();
}

/*!
 *	This is an efficient reallocation routine. It is used to resize an existing allocated chunk of memory. 
 *	If possible, the position of the chunk remains the same.
 *
 *	\param		heap	The heap on which the reallocation is performed
 *	\param		addr	One address inside of the chunk that is supposed to be reallocated
 *	\param		size	The size the new chunk is supposed to have
 *	\returns	First address (in use space) of the newly allocated chunk
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


/*!
 * Allocates a chunk of memory on the medium given by the driver and reserves it as shared memory.

 Parameters
 heap	The heap to be used.
 size	The amount of memory to be allocated in Bytes. Must be able to handle a single byte and values greater than 255.
 Returns
 A pointer to the first Byte of the allocated chunk.
 0 if allocation fails (0 is never a valid address).
 */
MemAddr os_sh_malloc(Heap* heap, size_t size){
	if (size == 0) return 0;
	
	os_enterCriticalSection();
	MemAddr start = os_malloc(heap, size);
	if (start == 0)	{
		os_leaveCriticalSection();
		return 0;	
	}
	setMapEntry(heap, start, SH_MEM_CLOSED);
	os_leaveCriticalSection();
	return start;
}

/*!
 * Allocates a chunk of memory on the medium given by the driver. This function will allocate memory for the given owner.
 This function is made in order to avoid code duplication and is called os_malloc and os_sh_malloc

 Parameters
 heap	The driver to be used.
 size	The amount of memory to be allocated in Bytes. Must be able to handle a single byte and values greater than 255.
 owner	The ProcessID of the owner of the new chunk
 Returns
 A pointer to the first Byte of the allocated chunk.
 0 if allocation fails (0 is never a valid address).
 */
MemAddr os_mallocOwner	(Heap * 	heap, size_t 	size, ProcessID 	owner){
	return 0;
}


/*!
Frees a chunk of shared allocated memory on the given heap

Parameters
heap	The heap to be used.
addr	An address inside of the chunk (not necessarily the start).
 */
void os_sh_free(Heap *heap, MemAddr *addr){
	os_enterCriticalSection();
	MemAddr start = os_getFirstByteOfChunk(heap, *addr);
	if (getMapEntry(heap, start) < 8){ 
		os_leaveCriticalSection();
		return;
	}
	
	while (getMapEntry(heap, start) != SH_MEM_CLOSED){
		os_yield();
	}
	
	os_freeOwnerRestricted(heap, *addr, SH_MEM_CLOSED);
	
	os_leaveCriticalSection();
}

/*!
	Function used to read from shared memory

	Parameters
	heap	The heap to be used
	ptr		Pointer to an address of the chunk to read from
	offset	An offset that refers to the beginning of the chunk
	dataDest	Destination where the data shall be copied to (this is always on internal SRAM)
	length	Specifies how many bytes of data shall be read 
 */
void os_sh_read(Heap const* heap, MemAddr const* ptr, uint16_t offset, MemValue* dataDest, uint16_t length){
	MemAddr tmp = os_getFirstByteOfChunk(heap, *ptr);
	
	if (offset + length > os_getChunkSize(heap, tmp)){
		os_error("Read Conflict");
		return;
	}
	
	
	// this function increases the number of reading processes.
	os_sh_readOpen(heap, &tmp);
	if (getMapEntry(heap, tmp) < 8){ 
		os_leaveCriticalSection();
		return;
	}

	
	// calculate start 
	tmp += offset;
	MemAddr internalPtr = (MemAddr)(dataDest);
	
	for (int i = 0; i < length; i++){
		intHeap->driver->write(internalPtr, heap->driver->read(tmp));
		tmp++;
		internalPtr++;
	}
	
	os_sh_readClose(heap, ptr);
}


/*!
 * Function used to write onto shared memory
 *
 *	 Parameters
	 heap	The heap to be used
	 ptr	Pointer to an address of the chunk to write to
	 offset	An offset that refers to the beginning of the chunk
	 dataSrc	Source of the data (this is always on internal SRAM)
	 length	Specifies how many bytes of data shall be written
 */
void os_sh_write(Heap const* heap, MemAddr const* ptr, uint16_t offset, MemValue const* dataSrc, uint16_t length) {
	
	MemAddr tmp = os_getFirstByteOfChunk(heap, *ptr);
	
	// check whether offset > chunksize
	if (offset + length > os_getChunkSize(heap, tmp)){
		os_error("Write Conflict");
		return;
	}
	
	os_sh_writeOpen(heap, &tmp);
	MemAddr src = (MemAddr)(dataSrc);
	tmp += offset;
	
	if (getMapEntry(heap, tmp) < 8){ 
		os_leaveCriticalSection();
		return;
	}

	
	for (int i = 0; i < length; i++){
		heap->driver->write(tmp, intHeap->driver->read(src));
		tmp++;
		src++;
	}
	
	os_sh_writeClose(heap, *ptr);
}


MemAddr os_sh_readOpen(Heap const *heap, MemAddr const *ptr) {
	os_enterCriticalSection();
	MemAddr tmp = os_getFirstByteOfChunk(heap, *ptr);
	
	// yield processing time as long as shm is not clear
	MemValue sh_status = getMapEntry(heap, tmp);
	
	// check whether the block is indeed a shared memory block
	if (sh_status < 8){
		os_error("PRIVATE BLOCK!");
		os_leaveCriticalSection();
		return *ptr;
	}
	
	while ((sh_status == SH_WRITE_OPEN) || (sh_status == SH_READ_FOUR)) {
		os_yield();
		sh_status = getMapEntry(heap, tmp);
	}

	if (sh_status == SH_MEM_CLOSED){
		// open closed memory for readers
		setMapEntry(heap, tmp, SH_READ_ONE);
	} else {
		// increment number of reading tasks.	
		setMapEntry(heap, tmp, (getMapEntry(heap, tmp)+1));
	}
	
	os_leaveCriticalSection();
	return *ptr;
}

void os_sh_writeClose(Heap const *heap, MemAddr const ptr){
	
	// close memory because only one process can write at a time
	os_sh_close(heap, ptr);
	
}


void os_sh_readClose(Heap const *heap, MemAddr const *ptr){
	os_enterCriticalSection();
	
	MemAddr tmp = os_getFirstByteOfChunk(heap, *ptr);
	MemValue sh_status = getMapEntry(heap, tmp);
	
	// close shared memory in case no readers exist.
	if (sh_status == SH_READ_ONE){
		setMapEntry(heap, tmp, SH_MEM_CLOSED);
	} else  // otherwise decrement number of reading processes 
		setMapEntry(heap, tmp, (sh_status-1));
	
	os_leaveCriticalSection();
}



void os_sh_close(Heap const * heap, MemAddr addr) {
	os_enterCriticalSection();
	MemAddr start = os_getFirstByteOfChunk(heap, addr);
	MemValue sh_status = getMapEntry(heap, start);
	if (sh_status <= SH_READ_FOUR && sh_status >= SH_READ_ONE){
		os_sh_readClose(heap, &addr);
		os_leaveCriticalSection();
		return;
	}
	
	MemAddr tmp = os_getFirstByteOfChunk(heap, addr);
	
	setMapEntry(heap, tmp, SH_MEM_CLOSED);
		
	os_leaveCriticalSection();
}



MemAddr os_sh_writeOpen (Heap const *heap, MemAddr const *ptr){
	os_enterCriticalSection();
	MemAddr tmp = os_getFirstByteOfChunk(heap, *ptr);
	
	// check whether the block is indeed a shared memory block
	if (getMapEntry(heap, tmp) < 8){ 
		os_error("PRIVATE BLOCK!");
		os_leaveCriticalSection();
		return *ptr;
	}

	while ((getMapEntry(heap, tmp) != SH_MEM_CLOSED)) {
		os_yield();
	}
	
	setMapEntry(heap, tmp, SH_WRITE_OPEN);
	
	os_leaveCriticalSection();
	return *ptr;
}

