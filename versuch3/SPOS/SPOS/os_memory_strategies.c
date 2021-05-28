/*! \file
 *  \brief Memory allocation strategies implemented for the OS.
 *
 *  Holds the memory allocation strategies used to realize memory functions.
 *
 *  \author   Johannes
 *  \date     2021-05-27
 *  \version  1.0
 */

#include "os_memory_strategies.h"
#include "os_memory.h"

/*!
 *   \param heap The heap to allocate memory on. 
 *   \param size Size of memory chunk in bytes. 
 *   \return Anfang des gefundenen chunks oder 0, falls keiner gefunden
 */
MemAddr os_Memory_FirstFit (Heap *heap, size_t size) {
    size_t chunkSize = 0;
    MemAddr startFreeSec = heap->useSectionStart;
    MemAddr senseAddr = heap->useSectionStart;

    while (senseAddr < (heap->useSectionStart + heap->useSectionSize)) {
        if (os_getMapEntry(heap, senseAddr) != 0x0) {
            senseAddr++;
            startFreeSec = senseAddr;
            chunkSize = 0;
        }
        else if (os_getMapEntry(heap, senseAddr) == 0x0 && chunkSize < size) {
            chunkSize++;
            senseAddr++;
        }
        if (chunkSize == size) {
            return startFreeSec;
        }
    }
    return 0;
}

