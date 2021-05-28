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
	ProcessID proc = os_getCurrentProc();
	MemAddr oposa = getOwnerOfChunk(heap, addr);
	if (oposa != proc){
		os_errorPStr(" Private Memory Address");
		os_leaveCriticalSection();
		return;
	}
	
	MemAddr ptr = os_getFirstByteOfChunk(heap, addr) - os_getUseStart(heap);
	// this char determines which nibble to consider
	char d; // d for determine "Nicht improvisiert" 
	
	// calculate initial value of d
	if (ptr % 2 == 0) {
		d = 'h';
	} else {
		d = 'l';
	}
	
	// calculate use space address in map address
	ptr = (ptr / 2) + heap->start_address;
	
	MemValue high = getHighNibble(heap, ptr);
	MemValue low = getLowNibble(heap, ptr);
	// 
	if (d == 'h')
	{
		setHighNibble(heap, ptr, 0);
		d = 'l';
	} else { // 
		setLowNibble(heap, ptr, 0);
		d = 'h';
		ptr++;
	}
	while(ptr < (heap->start_address + heap->map_size)) {
		high = getHighNibble(heap, ptr);
		low = getLowNibble(heap, ptr);
		if(d == 'h'){
			if (high == 15){
				setHighNibble(heap, ptr, 0);
				high = 0;
				d = 'l';
			} else break;
		}
		if (d == 'l'){
				if (low == 15){
					setLowNibble(heap, ptr, 0);
					ptr++;
					d = 'h';
				}
				else break;
		}
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
	if (addr > (heap->start_address + heap->map_size + heap->data_block)){
		os_errorPStr(" Invalid Address! ");
		os_leaveCriticalSection();
		return 0;
	}
	
	MemAddr ptr = os_getFirstByteOfChunk(heap, addr) - os_getUseStart(heap);
	if (ptr < 0){
		os_leaveCriticalSection();
		return 0;
	}

	char d;
	
	if (ptr % 2 == 0) {
		d = 'h';
		} else {
		d = 'l';
	}
	
	ptr = (ptr / 2) + heap->start_address;
	
	MemValue high;
	MemValue low;
	
	uint8_t counter = 1;
	if (d == 'h') {
		d = 'l';
	} else { // 
		d = 'h';
		ptr++;
	}
	while(ptr < (heap->start_address + heap->map_size)) {		
		high = getHighNibble(heap, ptr);
		low = getLowNibble(heap, ptr);
		if(d == 'h'){
			if (high == 15){
				d = 'l';
				counter++;
			}
			else break;
		}
		if (d == 'l'){
			if (low == 15){
				ptr++;
				counter++;
				d = 'h';
			}
			else break;
		}
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
		os_errorPStr(" Invalid Address! ");
		os_leaveCriticalSection();
		return 0;
	}
	MemAddr ptr = addr - os_getUseStart(heap);
	// this char determines which nibble to consider
	char d; // d for determine "Nicht improvisiert"
	// calculate initial value of d
	if (ptr % 2 == 0) {
		d = 'h';
	} else {
		d = 'l';	
	}
	ptr = (ptr / 2) + heap->start_address;
	MemValue high = getHighNibble(heap, ptr);
	MemValue low = getLowNibble(heap, ptr);
	// return error in case address not allocated
	if ((d == 'l' && low == 0) || (d == 'h' && high == 0)){
		os_leaveCriticalSection();
		return 0;
	}
	
	while (ptr >= os_getMapStart(heap)){
		high = getHighNibble(heap, ptr);
		low = getLowNibble(heap, ptr);
		if (d == 'l'){
			if (low == 15){
				d = 'h';
			} else {
				break;
			}
		}
		if (d == 'h')
		{
			if (high == 15){
				ptr--;
				d = 'l';
			}
			else {
				break; 
			}
		}
	}
	os_leaveCriticalSection();
	if (d == 'l'){
		return ((ptr - os_getMapStart(heap)) * 2) + os_getUseStart(heap) + 1;
	} else {
		return ((ptr - os_getMapStart(heap)) * 2) + os_getUseStart(heap);
	}
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
 *	\param heap	The heap on whos map the entry is supposed to be set
 *	\param addr	The address in use space for which the corresponding map entry shall be set
 *	\param value	The value that is supposed to be set onto the map (valid range: 0x0 - 0xF)
 */
void setMapEntry (Heap const *heap, MemAddr addr, MemValue value){
	heap->driver->write(addr, value);
}

/*!
 *	This function is used to get a heap map entry on a specific heap
 *
 *	\param heap	The heap from whos map the entry is supposed to be fetched
 *	\param addr	The address in use space for which the corresponding map entry shall be fetched
 *	\returns The value that can be found on the heap map entry that corresponds to the given use space address
 */
MemAddr getMapEntry(Heap const *heap, MemAddr addr) {
	MemAddr sas = ((addr-os_getUseStart(heap))/2) + os_getMapStart(heap);
	return sas;
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
	
	while (ptr < os_getUseStart(heap) + os_getUseSize(heap)) {
		if ((ptr - os_getUseStart(heap)) % 2 == 0){
			if (getHighNibble(heap, getMapEntry(heap, ptr)) == 0){
				ptr++;
				continue;
			}
		} else {
			if (getLowNibble(heap, getMapEntry(heap, ptr)) == 0){
				ptr++;
				continue;
			}
		}
		if (getOwnerOfChunk(heap, ptr) == pid) {
			MemAddr val = os_getChunkSize(heap, ptr);
			os_freeOwnerRestricted(heap, ptr, pid);
			ptr += val;
		} 
		else if (getOwnerOfChunk(heap, ptr) != 0) {
			ptr += os_getChunkSize(heap, ptr);
		}
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
	MemAddr ptr = os_getFirstByteOfChunk(heap, addr) - os_getUseStart(heap);
	// this char determines which nibble to consider
	char d; // d for determine "Nicht improvisiert" 
	
	// calculate initial value of d
	if (ptr % 2 == 0) {
		d = 'h';
	} else {
		d = 'l';
	}
	
	// calculate use space address in map address
	ptr = (ptr / 2) + heap->start_address;
	
	MemValue high = getHighNibble(heap, ptr);
	MemValue low = getLowNibble(heap, ptr);
	// 
	if (d == 'h')
	{
		setHighNibble(heap, ptr, 0);
		d = 'l';
	} else { // 
		setLowNibble(heap, ptr, 0);
		d = 'h';
		ptr++;
	}
	while(ptr < (heap->start_address + heap->map_size)) {
		high = getHighNibble(heap, ptr);
		low = getLowNibble(heap, ptr);
		if(d == 'h'){
			if (high == 15){
				setHighNibble(heap, ptr, 0);
				high = 0;
				d = 'l';
			} else break;
		}
		if (d == 'l'){
				if (low == 15){
					setLowNibble(heap, ptr, 0);
					ptr++;
					d = 'h';
				}
				else break;
		}
	}
	os_leaveCriticalSection();
}