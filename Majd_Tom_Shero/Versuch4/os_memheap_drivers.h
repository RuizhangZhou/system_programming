#ifndef intHeap
/*!
 *  \file 
 *	\brief Contains management of several logical heaps, each associated with one MemDriver
 *	
 *  \author   Lehrstuhl Informatik 11 - RWTH Aachen
 *  \date     2008, 2009, 2010, 2011, 2012
 *  \version  2.0
 */
#include "os_mem_drivers.h"
#include <stddef.h>



//! Used to enumerate the possible allocation strategies  
typedef enum {
	OS_MEM_FIRST,
	OS_MEM_NEXT,
	OS_MEM_BEST,
	OS_MEM_WORST
} AllocStrategy;


/*!
 *	The structure of a heap driver which consists of a low level memory driver
 *	and heap specific information such as start, size etc...
 */
typedef struct Heap {
	// pointer to the memory driver 
	MemDriver *driver; 
	// start address, map size and data block size
	MemAddr start_address;
	uint16_t map_size;
	uint16_t data_block;
	
	AllocStrategy allocStrat;
	// heap name
	char * name;
} Heap;


//! stores all heaps connected
Heap intHeap__;
Heap extHeap__;
//! Realises a pointer to the Heap intHeap__.
#define 	intHeap   (&intHeap__)
#define		extHeap   (&extHeap__)
//! Initialises all Heaps.
void os_initHeaps();

//! Used to get number of heaps
uint8_t os_getHeapListLength();

//! Used to get the heap assigned a given index
Heap* os_lookupHeap(uint8_t index);

#endif