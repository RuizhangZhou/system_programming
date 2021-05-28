#include "os_memory_strategies.h"
#include "os_memory.h"
#include "os_core.h"

MemAddr last_addr = 0;

// First-Fit Strategy
MemAddr os_Memory_FirstFit (Heap *heap, size_t size){
	os_enterCriticalSection();
	if (size == 0 || os_getCurrentProc() == 0 || size > os_getUseSize(heap))
	{
		os_leaveCriticalSection();
		return 0;
	}
	
	MemAddr start = 0;
	MemAddr ptr = heap->start_address;
	size_t gap_counter = 0;
	MemValue low;
	MemValue high;
	
	// find free nibbles
	// if there exists non sufficient memory return 0 
	while (ptr < os_getUseStart(heap)){
		low = read(ptr) & 0b00001111;
		high = read(ptr) >> 4;
		
		// test if high (first byte) is free then increment gap_counter
		// otherwise if the gap is not big enough reset gap_counter
		if (high == 0){
			if (gap_counter == 0) start = ptr;
			gap_counter++;
		} else {
			if (gap_counter != size) gap_counter = 0;
		}
		if (gap_counter == size){
			 break;
		}
		// test if low (following) is free then increment gap_counter
		// otherwise if the gap is not big enough reset gap_counter
		if (low == 0){
			if (gap_counter == 0) start = ptr;
			gap_counter++;
		} else {
			if (gap_counter != size) gap_counter = 0;
		}
		// if end of map is reached return error! 
		if (ptr >= (os_getUseStart(heap)))
		{
			os_errorPStr("No Heap Space");
			os_leaveCriticalSection();
			return 0;
		}
		
		if (gap_counter == size) break;
		
		// increment ptr to get to next address
		ptr++;
	}
	if (start + (size/2) > os_getUseStart(heap))
	{
		os_leaveCriticalSection();
		return 0;
	}
	
	char d = 'n';
	MemValue tmp;
	// check whether gap begins in low or high
	ptr = start;
	// starting from the first address change the nibbles 
	int i = 0;
	while(i < size){
		low = read(ptr) & 0b00001111;
		high = read(ptr) >> 4;	
		// first iteration set high nibble (for starting byte) to process number
		if (i==0){
			if (high == 0){
				d = 'h';
				tmp = (os_getCurrentProc()<<4) | read(ptr);
				write(ptr, tmp);
				i++;
				if (i < size){
					tmp = 0b00001111 | read(ptr);
					write(ptr, tmp);
					i++;
					ptr++;
				}
			}
			else { // in case low byte is free and high byte isn't
				d = 'l';
				tmp = os_getCurrentProc() | read(ptr);
				write(ptr, tmp);
				i++;
				ptr++;
			}
		} else { // further iterations keep writing F until size is reached
			tmp = 0b11110000 | read(ptr);
			write(ptr, tmp);
			i++;
			if (i < size){ // write F in low if necessary 
				write(ptr, 0b11111111);
				i++;
				ptr++;
			}
		}
	}
	
	// calculate address in data blocks
	if (d == 'h') { // in case starting nibble is a low nibble
		ptr = start - heap->start_address;
		ptr = 2*ptr + heap->map_size + heap->start_address; 
	} else { // starting address is high nibble
		ptr = start - heap->start_address;
		ptr = 2*ptr + 1 + heap->map_size + heap->start_address;
	}
	
	// return address 
	os_leaveCriticalSection();
	return ptr; 
}


MemAddr os_Memory_WorstFit (Heap *heap, size_t size){
	os_enterCriticalSection();
	if (size == 0 || os_getCurrentProc() == 0 || size > os_getUseSize(heap))
	{
		os_leaveCriticalSection();
		return 0;
	}
	
	MemAddr start = 0;
	MemAddr ptr = heap->start_address;
	MemAddr worst_start = 0;
	size_t gap_counter = 0;
	size_t worst_gap = 0;
	MemValue low;
	MemValue high;
	
	// find free nibbles
	// if there exists non sufficient memory return 0
	while (ptr < os_getUseStart(heap)){
		low = getLowNibble(heap, ptr);
		high = getHighNibble(heap, ptr);
		
		// test if high (first byte) is free then increment gap_counter
		// otherwise if the gap is not big enough reset gap_counter
		if (high == 0){
			if(gap_counter == 0) start = ptr;
			gap_counter++;
		} else {
			if (gap_counter < size) gap_counter = 0;
			else{
				if (worst_gap == 0 || gap_counter > worst_gap){
					worst_gap = gap_counter;
					worst_start = start;
				}
			}
		}
		
		// test if low (following) is free then increment gap_counter
		// otherwise if the gap is not big enough reset gap_counter
		if (low == 0){
			if (gap_counter == 0) start = ptr;
			gap_counter++;
		} else {
			if (gap_counter < size) gap_counter = 0;
			else{
				if (worst_gap == 0 || gap_counter > worst_gap){
					worst_gap = gap_counter;
					worst_start = start;
				}
			}
		}
		
		// increment ptr to get to next address
		ptr++;
		// if end of map is reached and no fitting gap exists return error!
		if ((ptr >= os_getUseStart(heap)) && (worst_gap == 0) && (gap_counter < size))
		{
			os_errorPStr("No Heap Space");
			os_leaveCriticalSection();
			return 0;
		}
		// if end of map and gap is bigger than size then set worst gap
		if ((ptr >= os_getUseStart(heap)))
		{
			if ((gap_counter >= size) && (gap_counter > worst_gap || worst_gap == 0)){
				worst_gap = gap_counter;
				worst_start = start;
			}
		}
	}
	
	if (worst_start + (size/2) > os_getUseStart(heap))
	{
		os_leaveCriticalSection();
		return 0;
	}
	
	char d = 'n';
	MemValue tmp;
	// check whether gap begins in low or high
	ptr = worst_start;
	// starting from the first address change the nibbles
	int i = 0;
	while(i < size){
		low = getLowNibble(heap, ptr);
		high = getHighNibble(heap, ptr);
		// first iteration set high nibble (for starting byte) to process number
		if (i==0){
			if (high == 0){
				d = 'h';
				tmp = (os_getCurrentProc()<<4) | read(ptr);
				write(ptr, tmp);
				i++;
				if (i < size){
					tmp = 0b00001111 | read(ptr);
					write(ptr, tmp);
					i++;
					ptr++;
				}
			}
			else { // in case low byte is free and high byte isn't
				d = 'l';
				tmp = os_getCurrentProc() | read(ptr);
				write(ptr, tmp);
				i++;
				ptr++;
			}
		} else { // further iterations keep writing F until size is reached
			tmp = 0b11110000 | read(ptr);
			write(ptr, tmp);
			i++;
			if (i < size){ // write F in low if necessary
				write(ptr, 0b11111111);
				i++;
				ptr++;
			}
		}
	}
	
	// calculate address in data blocks
	if (d == 'h') { // in case starting nibble is a low nibble
		ptr = worst_start - heap->start_address;
		ptr = 2*ptr + heap->map_size + heap->start_address;
		} else { // starting address is high nibble
		ptr = worst_start - heap->start_address;
		ptr = 2*ptr + 1 + heap->map_size + heap->start_address;
	}
	
	// return address
	os_leaveCriticalSection();
	return ptr;
}

MemAddr os_Memory_NextFit (Heap *heap, size_t size){
	os_enterCriticalSection();
	if (size == 0 ||os_getCurrentProc() == 0 || size > os_getUseSize(heap))
	{
		os_leaveCriticalSection();
		return 0;
	}
	if (last_addr == 0)
	{
		last_addr = os_Memory_FirstFit(heap, size);
		os_leaveCriticalSection();
		return last_addr;
	}
	MemAddr ptr = last_addr;
	size_t gap_counter = 0;
	MemValue low;
	MemValue high;
	
	// test if high (first byte) is free then increment gap_counter
	// otherwise if the gap is not big enough reset gap_counter
	
	// find free nibbles
	// if there exists non sufficient memory return 0
	MemAddr start = 0;
	while (ptr < os_getUseStart(heap)){
		low = getLowNibble(heap, ptr);
		high = getHighNibble(heap, ptr);
		
		// test if high (first byte) is free then increment gap_counter
		// otherwise if the gap is not big enough reset gap_counter
		if (high == 0){
			if (gap_counter == 0) start = ptr;
			gap_counter++;
			} else {
			if (gap_counter != size) gap_counter = 0;
		}
		if (gap_counter == size){
			break;
		}
		// test if low (following) is free then increment gap_counter
		// otherwise if the gap is not big enough reset gap_counter
		if (low == 0){
			if (gap_counter == 0) start = ptr;
			gap_counter++;
			} else {
			if (gap_counter != size) gap_counter = 0;
		}
		
		if (gap_counter == size) break;
		
		// increment ptr to get to next address
		ptr++;
		if(ptr == os_getUseStart(heap)) {
			gap_counter = 0;
			ptr = os_getMapStart(heap);	
			last_addr = os_Memory_FirstFit(heap, size);
			os_leaveCriticalSection();
			return last_addr;		
		}
	}
	if (start + (size/2) > os_getUseStart(heap))
	{
		os_leaveCriticalSection();
		return 0;
	}
	
	char d = 'n';
	MemValue tmp;
	// check whether gap begins in low or high
	ptr = start;
	// starting from the first address change the nibbles
	int i = 0;
	while(i < size){
		low = read(ptr) & 0b00001111;
		high = read(ptr) >> 4;
		// first iteration set high nibble (for starting byte) to process number
		if (i==0){
			if (high == 0){
				d = 'h';
				tmp = (os_getCurrentProc()<<4) | read(ptr);
				write(ptr, tmp);
				i++;
				if (i < size){
					tmp = 0b00001111 | read(ptr);
					write(ptr, tmp);
					i++;
					ptr++;
				}
			}
			else { // in case low byte is free and high byte isn't
				d = 'l';
				tmp = os_getCurrentProc() | read(ptr);
				write(ptr, tmp);
				i++;
				ptr++;
			}
		} else { // further iterations keep writing F until size is reached
			tmp = 0b11110000 | read(ptr);
			write(ptr, tmp);
			i++;
			if (i < size){ // write F in low if necessary
				write(ptr, 0b11111111);
				i++;
				ptr++;
			}
		}
	}
	
	// calculate address in data blocks
	if (d == 'h') { // in case starting nibble is a low nibble
		ptr = start - heap->start_address;
		ptr = 2*ptr + heap->map_size + heap->start_address;
	} else { // starting address is high nibble
		ptr = start - heap->start_address;
		ptr = 2*ptr + 1 + heap->map_size + heap->start_address;
	}
	
	// return address
	last_addr = ptr;
	os_leaveCriticalSection();
	return ptr;
}

MemAddr os_Memory_BestFit (Heap *heap, size_t size){
	os_enterCriticalSection();
	if (size == 0 || os_getCurrentProc() == 0 || size > os_getUseSize(heap))
	{
		os_leaveCriticalSection();
		return 0;
	}
	
	MemAddr start = 0;
	MemAddr ptr = heap->start_address;
	MemAddr opt_start = 0;
	size_t gap_counter = 0;
	size_t opt_gap = 0;
	MemValue low;
	MemValue high;
	
	// find free nibbles
	// if there exists non sufficient memory return 0
	while (ptr < os_getUseStart(heap)){
		low = getLowNibble(heap, ptr);
		high = getHighNibble(heap, ptr);
		
		// test if high (first byte) is free then increment gap_counter
		// otherwise if the gap is not big enough reset gap_counter
		if (high == 0){
			if(gap_counter == 0) start = ptr;
			gap_counter++;
		} else {
			if (gap_counter < size) gap_counter = 0;
			else{
				if (opt_gap == 0 || gap_counter < opt_gap){ 
					opt_gap = gap_counter;
					opt_start = start;
				}
			}
		}
		
		// test if low (following) is free then increment gap_counter
		// otherwise if the gap is not big enough reset gap_counter
		if (low == 0){
			if (gap_counter == 0) start = ptr;
			gap_counter++;
		} else {
			if (gap_counter < size) gap_counter = 0;
			else{
				if (opt_gap == 0 || gap_counter < opt_gap){ 
					opt_gap = gap_counter;
					opt_start = start;
				}
			}
		}
		
		// increment ptr to get to next address
		ptr++;
		// if end of map is reached and no fitting gap exists return error!
		if ((ptr >= os_getUseStart(heap)) && (opt_gap == 0) && (gap_counter < size))
		{
			os_errorPStr("No Heap Space");
			os_leaveCriticalSection();
			return 0;
		}
		// if end of map and gap is bigger than size then set optimal gap
		if ((ptr >= os_getUseStart(heap)))
		{
			if ((gap_counter >= size) && (gap_counter < opt_gap || opt_gap == 0)){
				opt_gap = gap_counter;
				opt_start = start;
			}
		}
	}
	
	if (opt_start + (size/2) > os_getUseStart(heap))
	{
		os_leaveCriticalSection();
		return 0;
	}
	
	char d = 'n';
	MemValue tmp;
	// check whether gap begins in low or high
	ptr = opt_start;
	// starting from the first address change the nibbles
	int i = 0;
	while(i < size){
		low = getLowNibble(heap, ptr);
		high = getHighNibble(heap, ptr);
		// first iteration set high nibble (for starting byte) to process number
		if (i==0){
			if (high == 0){
				d = 'h';
				tmp = (os_getCurrentProc()<<4) | read(ptr);
				write(ptr, tmp);
				i++;
				if (i < size){
					tmp = 0b00001111 | read(ptr);
					write(ptr, tmp);
					i++;
					ptr++;
				}
			}
			else { // in case low byte is free and high byte isn't
				d = 'l';
				tmp = os_getCurrentProc() | read(ptr);
				write(ptr, tmp);
				i++;
				ptr++;
			}
			} else { // further iterations keep writing F until size is reached
			tmp = 0b11110000 | read(ptr);
			write(ptr, tmp);
			i++;
			if (i < size){ // write F in low if necessary
				write(ptr, 0b11111111);
				i++;
				ptr++;
			}
		}
	}
	
	// calculate address in data blocks
	if (d == 'h') { // in case starting nibble is a low nibble
		ptr = opt_start - heap->start_address;
		ptr = 2*ptr + heap->map_size + heap->start_address;
	} else { // starting address is high nibble
		ptr = opt_start - heap->start_address;
		ptr = 2*ptr + 1 + heap->map_size + heap->start_address;
	}
	
	// return address
	os_leaveCriticalSection();
	return ptr;
}