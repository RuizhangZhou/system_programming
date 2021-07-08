#ifndef _OS_MEMHEAP_DRIVERS_H
#define _OS_MEMHEAP_DRIVERS_H

#include "os_mem_drivers.h"
#include "defines.h"
#include <stdint.h>
#include <stddef.h>

#define INT_HEAP_BOTTOM	(0X100+400)
#define STACK_TOP	PROCESS_STACK_BOTTOM(MAX_NUMBER_OF_PROCESSES)
#define INT_HEAP_TOP	STACK_TOP
#define INT_HEAP_SIZE   (INT_HEAP_TOP-INT_HEAP_BOTTOM)

#define EXT_SRAM_START (0x0)
#define EXT_MEMORY_SRAM 65536//64KiB?
#define DEFAULT_ALLOCATION_STRATEGY OS_MEM_FIRST

extern uint8_t const __heap_start;

typedef enum {
    OS_MEM_FIRST,
    OS_MEM_NEXT,
    OS_MEM_BEST,
    OS_MEM_WORST
}AllocStrategy;

typedef struct {
    // Einen Zeiger auf den Speichertreiber, welcher dem Heap assoziiert ist
    MemDriver *driver;
    MemAddr mapStart;
    size_t mapSize;
    MemAddr useStart;
    size_t useSize;
    AllocStrategy strategy;
	MemAddr nextFit;
    const char *name;//the name of this heap
    MemAddr lowerBound[8];
    MemAddr upperBound[8];//just record the first addr of every Chunk 
    MemAddr numOfChunks[8];
} Heap;


Heap intHeap__;

Heap extHeap__;

//Realises a pointer to the Heap intHeap__.
#define intHeap (&intHeap__)

//Realises a pointer to the Heap extHeap__.
#define extHeap (&extHeap__)

//Initialises all Heaps.
void os_initHeaps(void);

//Needed for Taskmanager interaction.
uint8_t os_getHeapListLength(void);

//Needed for Taskmanager interaction.
Heap* os_lookupHeap(uint8_t index);



#endif
