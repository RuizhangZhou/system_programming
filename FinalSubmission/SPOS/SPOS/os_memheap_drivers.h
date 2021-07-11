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

/*!
 *  Enth�lt "charakteristische Werte des Speichermediums sowie 
 *  Funktionszeiger auf init, read und write
 */
typedef struct Heap {
	MemDriver *driver;
	uint16_t mapAreaStart;
	uint16_t mapAreaSize;
	uint16_t useAreaStart;
	uint16_t useAreaSize;
	AllocStrategy allocStrategy;
	uint16_t nextFit;
	const char *name;
	uint16_t procVisitArea[7];
} Heap;

/*!
 *  alle Nibbles des Map-Bereichs aller vorhandenen Heaptreiber mit 0x00 �berschreiben.
 *  Soll an geeigneter Stelle zur Initialisierung aufgerufen werden.
 */
void os_initHeaps(void);

//! zum Heapindex den passenden Zeiger auf den Heap zur�ckgeben. intHeap hat Index 0.
Heap* os_lookupHeap(uint8_t index);

//! gibt Anzahl an existierenden Heaps zur�ck.
size_t os_getHeapListLength(void);

Heap intHeap__;
Heap extHeap__;

#endif