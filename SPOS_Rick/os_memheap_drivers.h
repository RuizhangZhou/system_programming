#ifndef _OS_MEMHEAP_DRIVERS_H
#define _OS_MEMHEAP_DRIVERS_H

#include "os_mem_drivers.h"
#include <stddef.h>

//! Zeigt auf den Heap `intHeap__`
#define intHeap (&intHeap__)
#define extHeap (&extHeap__)

//! All available heap allocation strategies.
typedef enum AllocStrategy {
  OS_MEM_NEXT,
  OS_MEM_FIRST,
  OS_MEM_BEST,
  OS_MEM_WORST
} AllocStrategy;

typedef struct Heap {
  MemDriver *driver;
  uint16_t startOfMapArea;
  uint16_t sizeOfMapArea;
  uint16_t startOfUseArea;
  uint16_t sizeOfUseArea;
  AllocStrategy allocationStrategy;
  uint16_t nextOne;
  const char *name;
  uint16_t visitedAreas[7];
} Heap;

void os_initHeaps(void);
Heap *os_lookupHeap(uint8_t index);
size_t os_getHeapListLength(void);

Heap intHeap__;
Heap extHeap__;

#endif
