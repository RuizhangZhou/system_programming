#ifndef _OS_MEMHEAP_DRIVERS_H
#define _OS_MEMHEAP_DRIVERS_H

#include "os_mem_drivers.h"
#include <stdint.h>
#include <stddef.h>


#define HEAP_BOTTOM	(0X100+200)
#define STACK_TOP	PROCESS_STACK_BOTTOM(MAX_NUMBER_OF_PROCESSES)
#define HEAP_TOP	STACK_TOP
#define HEAP_SIZE   (HEAP_TOP-HEAP_BOTTOM)



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

    AllocStrategy currentAllocStrategy;

    const char *name;//the name of this heap
} Heap;


Heap intHeap__;
Heap extHeap__; 

#define intHeap (&intHeap__)


void os_initHeaps(void);

uint8_t os_getHeapListLength(void);

Heap* os_lookupHeap(uint8_t index);



#endif
