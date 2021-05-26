#ifndef _OS_MEMHEAP_DRIVERS_H
#define _OS_MEMHEAP_DRIVERS_H

#include "os_mem_drivers.h"
#include <stdint.h>
#include <stddef.h>

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

void os_initHeaps(void);

uint8_t os_getHeapListLength(void);

Heap* os_lookupHeap(uint8_t index);



#endif
