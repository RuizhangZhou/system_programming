

#ifndef _OS_MEMORY_STRATEGIES_H
#define _OS_MEMORY_STRATEGIES_H

#include "os_memheap_drivers.h"

/*!
 * \return Anfang des gefundenen chunks oder 0, falls keiner gefunden
 */
MemAddr os_Memory_FirstFit (Heap *heap, size_t size);

/*!
 * \return Anfang des gefundenen chunks oder 0, falls keiner gefunden
 */
MemAddr os_Memory_NextFit (Heap *heap, size_t size);

/*!
 * \return Anfang des gefundenen chunks oder 0, falls keiner gefunden
 */
MemAddr os_Memory_BestFit (Heap *heap, size_t size);

/*!
 * \return Anfang des gefundenen chunks oder 0, falls keiner gefunden
 */
MemAddr os_Memory_WorstFit (Heap *heap, size_t size);

#endif
