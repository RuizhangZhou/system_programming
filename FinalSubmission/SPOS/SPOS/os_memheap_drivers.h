#ifndef _OS_MEMHEAP_DRIVERS_H
#define _OS_MEMHEAP_DRIVERS_H

#include "os_mem_drivers.h"
#include <stddef.h>

//! Zeigt auf den Heap `intHeap__`
#define intHeap (&intHeap__)
#define extHeap (&extHeap__)

//! All available heap allocation strategies.
typedef enum AllocStrategy {
	OS_MEM_FIRST,
	OS_MEM_NEXT,
	OS_MEM_BEST,
	OS_MEM_WORST
} AllocStrategy;


typedef struct Heap {
	// Einen Zeiger auf den Speichertreiber, welcher dem Heap assoziiert ist
	MemDriver *driver;
	uint16_t mapStart;
	uint16_t mapSize;
	uint16_t useStart;
	uint16_t useSize;
	AllocStrategy allocStrategy;
	uint16_t nextFit;
	const char *name;//the name of this heap
	uint16_t procVisitArea[7];
} Heap;

//Initialises all Heaps.
void os_initHeaps(void);

//Needed for Taskmanager interaction.
Heap* os_lookupHeap(uint8_t index);

//Needed for Taskmanager interaction.
size_t os_getHeapListLength(void);

Heap intHeap__;
Heap extHeap__;

#endif