#include "os_memory_strategies.h"
#include "os_memory.h"
#include "os_core.h"


/************************************************************************/
/*						Global Variables								*/
/************************************************************************/
MemAddr last_addr[2] = {0, 0};






/*!
 * First-fit strategy.
 *
 *	\param		heap	The heap in which we want to find a free chunk
 *	\param		size	 The size of the desired free chunk
 *	\returns	The first address of the found free chunk, or 0 if no chunk was found.
 */
MemAddr os_Memory_FirstFit (Heap *heap, size_t size){
	os_enterCriticalSection();
	if (size == 0 || os_getCurrentProc() == 0 || size > os_getUseSize(heap)) {
		os_leaveCriticalSection();
		return 0;
	}
	
	MemAddr start = 0;
	MemAddr ptr = os_getUseStart(heap);
	size_t gap_counter = 0;
	
	// find free nibbles
	// if there exists non sufficient memory return 0 
	while (ptr < os_getUseStart(heap) + os_getUseSize(heap)){
		if(getMapEntry(heap, ptr) == 0) {
			if(gap_counter == 0) start = ptr;
			gap_counter++;
		} else {
			if (gap_counter != size) gap_counter = 0;
		}
		
		if (gap_counter == size) break;
		ptr++;
		
		if (ptr == os_getUseStart(heap) + os_getUseSize(heap)){
			if (gap_counter != size){
				os_leaveCriticalSection();
				return 0;	
			}
		}
	}
	
	ptr = start;
	// starting from the first address change the nibbles 
	while(ptr < size+start){
		if (ptr == start){
			setMapEntry(heap, ptr, os_getCurrentProc()); 
		} else {
			setMapEntry(heap, ptr, 0xff);
			if (ptr == start + 58){
				setMapEntry(heap, ptr, 0xff);
				getMapEntry(heap, ptr);
			}
		}
		ptr++;
	}
	
	if (heap == intHeap){
		num_num_chunk_num[0][os_getCurrentProc()] += 1;
	} else {
		num_num_chunk_num[1][os_getCurrentProc()] += 1;
	}
	// return address 
	os_leaveCriticalSection();
	return start; 
}








/*!
 * Next-fit strategy.
 *
 *	\param		heap	The heap in which we want to find a free chunk
 *	\param		size	 The size of the desired free chunk
 *	\returns	The first address of the found free chunk, or 0 if no chunk was found.
 */
MemAddr os_Memory_WorstFit (Heap *heap, size_t size){
	os_enterCriticalSection();
	if (size == 0 || os_getCurrentProc() == 0 || size > os_getUseSize(heap))
	{
		os_leaveCriticalSection();
		return 0;
	}
	
	MemAddr start = 0;
	MemAddr ptr = os_getUseStart(heap);
	MemAddr worst_start = 0;
	size_t gap_counter = 0;
	size_t worst_gap = 0;
	
	// find free nibbles
	// if there exists non sufficient memory return 0
	while (ptr < os_getUseStart(heap)+os_getUseSize(heap)){
		if (getMapEntry(heap, ptr) == 0){
			if (gap_counter == 0) start = ptr;
			gap_counter++;
		} else {
			if (gap_counter < size){
				gap_counter = 0;
			}
			else {
				if (worst_gap == 0 || gap_counter > worst_gap){
					worst_gap = gap_counter;
					worst_start = start;
				}
				gap_counter = 0;
			}
		}
		ptr++;
		
		if ((ptr >= os_getUseSize(heap)+os_getUseStart(heap))){
			if (gap_counter < size  && (worst_gap == 0)){ 
				os_leaveCriticalSection();
				return 0;
			} else {
				if (worst_gap > gap_counter) break;
				worst_gap = gap_counter;
				worst_start = start;
			}
		}
	}
	
	ptr = worst_start;
	// starting from the first address change the nibbles
	setMapEntry(heap, ptr, os_getCurrentProc());
	ptr++;
	while(ptr < worst_start+size){
		setMapEntry(heap, ptr, 15);
		ptr++;
	}
	if (heap == intHeap){
		num_num_chunk_num[0][os_getCurrentProc()] += 1;
	} else {
		num_num_chunk_num[1][os_getCurrentProc()] += 1;
	}
	// return address
	os_leaveCriticalSection();
	return worst_start;
}







/*!
 *	Worst-fit strategy.
 *
 *	\param		heap	The heap in which we want to find a free chunk
 *	\param		size	 The size of the desired free chunk
 *	\returns	The first address of the found free chunk, or 0 if no chunk was found.
 */
MemAddr os_Memory_NextFit (Heap *heap, size_t size){
	os_enterCriticalSection();
	if (size == 0 ||os_getCurrentProc() == 0 || size > os_getUseSize(heap)){
		os_leaveCriticalSection();
		return 0;
	}
	int heap_num = 0;
	if (heap == extHeap) heap_num = 1;
	if (!last_addr[heap_num]) last_addr[heap_num] = os_getUseStart(heap);
	MemAddr ptr = last_addr[heap_num];
	if (last_addr[heap_num] == os_getUseStart(heap)) {
		ptr = os_Memory_FirstFit(heap, size)+size;
		last_addr[heap_num] = ptr;
		if (ptr == os_getUseStart(heap) + os_getUseSize(heap)) last_addr[heap_num] = os_getUseStart(heap);
		os_leaveCriticalSection();
		return ptr-size;
	}
	size_t gap_counter = 0;

	// find free nibbles
	// if there exists non sufficient memory return 0
	MemAddr start = 0;
	while (ptr < os_getUseStart(heap) + os_getUseSize(heap)){
		if(getMapEntry(heap, ptr) == 0) {
			if(gap_counter == 0) start = ptr;
			gap_counter++;
		} else {
			if (gap_counter != size) gap_counter = 0;
		}
		
		if (gap_counter == size) break;
		ptr++;
		
		if (ptr == os_getUseStart(heap) + os_getUseSize(heap)){
			gap_counter = 0;
			ptr = os_getUseStart(heap);
			last_addr[heap_num] = os_Memory_FirstFit(heap, size);
			ptr = last_addr[heap_num];
			if (ptr == os_getUseStart(heap)+os_getUseSize(heap)) last_addr[heap_num] = os_getUseStart(heap);
			os_leaveCriticalSection();
			return ptr;
		}
	}
	if (start + size > os_getUseStart(heap)+os_getUseSize(heap)) {
		os_leaveCriticalSection();
		return 0;
	}

	ptr = start;
	// starting from the first address change the nibbles
	while(ptr < size+start){
		if (ptr == start){
			setMapEntry(heap, ptr, os_getCurrentProc()); 
		} else {
			setMapEntry(heap, ptr, 0xff);
		}
		ptr++;
	}
	if (heap == intHeap){
		num_num_chunk_num[0][os_getCurrentProc()] += 1;
	} else {
		num_num_chunk_num[1][os_getCurrentProc()] += 1;
	}
	// return address
	last_addr[heap_num] = ptr;
	if (ptr == os_getUseStart(heap)+os_getUseSize(heap)) last_addr[heap_num] = os_getUseStart(heap);
	os_leaveCriticalSection();
	return start;
}






/*!
 *	Best-fit strategy.
 *
 *	\param		heap	The heap in which we want to find a free chunk
 *	\param		size	 The size of the desired free chunk
 *	\returns	The first address of the found free chunk, or 0 if no chunk was found.
 */
MemAddr os_Memory_BestFit (Heap *heap, size_t size){
	os_enterCriticalSection();
	if (size == 0 || os_getCurrentProc() == 0 || size > os_getUseSize(heap))
	{
		os_leaveCriticalSection();
		return 0;
	}
	
	MemAddr start = 0;
	MemAddr ptr = os_getUseStart(heap);
	MemAddr opt_start = 0;
	size_t gap_counter = 0;
	size_t opt_gap = 0;
	
	// find free nibbles
	// if there exists non sufficient memory return 0
	while (ptr < os_getUseStart(heap)+os_getUseSize(heap)){
		if (getMapEntry(heap, ptr) == 0){
			if (gap_counter == 0) start = ptr;
			gap_counter++;
		} else {
			if (gap_counter < size){
				gap_counter = 0;
			}
			else {
				if (opt_gap == 0 || gap_counter < opt_gap){
					opt_gap = gap_counter;
					opt_start = start;
				}
				gap_counter = 0;
			}
		}
		ptr++;
		
		if ((ptr >= os_getUseSize(heap)+os_getUseStart(heap))){
			if (gap_counter < size && (opt_gap == 0)){ 
				os_leaveCriticalSection();
				return 0;
			} else {
				if (opt_gap < gap_counter && opt_gap != 0) break;
				opt_gap = gap_counter;
				opt_start = start;
			}
		}
	}

	ptr = opt_start;
	// starting from the first address change the nibbles
	setMapEntry(heap, ptr, os_getCurrentProc());
	ptr++;
	while(ptr < opt_start+size){
		setMapEntry(heap, ptr, 15);
		ptr++;
	}
	if (heap == intHeap){
		num_num_chunk_num[0][os_getCurrentProc()] += 1;
	} else {
		num_num_chunk_num[1][os_getCurrentProc()] += 1;
	}
	// return address
	os_leaveCriticalSection();
	return opt_start;
}