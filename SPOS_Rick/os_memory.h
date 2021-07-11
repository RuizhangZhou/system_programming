#ifndef _OS_MEMORY_H
#define _OS_MEMORY_H

#include "os_mem_drivers.h"
#include "os_memheap_drivers.h"
#include "os_scheduler.h"
#include <stddef.h>

MemValue os_getMapEntry(Heap const *heap, MemAddr addr);

MemAddr os_sh_malloc(Heap *heap, uint16_t size);
MemAddr os_getUseStart(Heap const *heap);
AllocStrategy os_getAllocationStrategy(Heap const *heap);
MemAddr os_getMapStart(Heap const *heap);
void os_sh_free(Heap *heap, MemAddr *ptr);

void os_setAllocationStrategy(Heap *heap, AllocStrategy allocStrat);
void os_sh_read(Heap const *heap, MemAddr const *ptr, uint16_t offset,
                MemValue *dataDest, uint16_t length);
MemAddr os_sh_readOpen(Heap const *heap, MemAddr const *ptr);
uint16_t os_getChunkSize(Heap const *heap, MemAddr addr);
size_t os_getMapSize(Heap const *heap);

void os_sh_close(Heap const *heap, MemAddr addr);
size_t os_getUseSize(Heap const *heap);
MemAddr os_sh_writeOpen(Heap const *heap, MemAddr const *ptr);
void os_free(Heap *heap, MemAddr addr);
void os_sh_write(Heap const *heap, MemAddr const *ptr, uint16_t offset,
                 MemValue const *dataSrc, uint16_t length);

MemAddr os_realloc(Heap *heap, MemAddr addr, uint16_t size);

void os_freeProcessMemory(Heap *heap, ProcessID pid);
MemAddr os_malloc(Heap *heap, uint16_t size);

#endif
