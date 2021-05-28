/*! \file
 *  \brief Heap.
 *
 *  Verwaltet Heap und haelt MemDriver.
 *
 *  \author   Johannes
 *  \date     2021-05-25
 *  \version  1.0
 */

#ifndef _OS_MEMHEAP_DRIVERS_H
#define _OS_MEMHEAP_DRIVERS_H

#include "os_mem_drivers.h"
#include <stddef.h>

//! pointer to heap 'intHeap__'
#define intHeap (&intHeap__);

//! Enum that holds all available allocation strategies
typedef enum AllocStrategy {
    OS_MEM_FIRST,
    OS_MEM_NEXT,
    OS_MEM_BEST,
    OS_MEM_WORST
} AllocStrategy;

//! Struct heap holds pointers to Memdriver and name and adresses/sizes of map and use sections
typedef struct Heap {
    MemDriver *driver;
    uint16_t mapSectionStart;
    uint16_t mapSectionSize;
    uint16_t useSectionStart;
    uint16_t useSectionSize;
    AllocStrategy actAllocStrategy;
    const char *name;
    uint16_t procVisitArea[7];
} Heap;

//! instance of Heap named int(ernal) Heap
Heap intHeap__;

//! function to init the heaps by setting all nibbles of the map section to 0x0
void os_initHeaps(void);

//! function to get number of existing heaps
size_t os_getHeapListLength(void);

//! function to get a pointer to heap associated with 'index'
Heap* os_lookupHeap(uint_t index);

#endif