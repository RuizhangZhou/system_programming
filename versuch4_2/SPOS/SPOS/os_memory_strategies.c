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

MemAddr os_Memory_NextFit (Heap *heap, size_t size) {
	size_t zeroChunkSize = 0;
	MemAddr slow = heap->nextFit;
	MemAddr fast = heap->nextFit;

	while (fast < (heap->useAreaStart + heap->useAreaSize)) {
		if (os_getMapEntry(heap, slow) != 0x0) {
			slow++;
			fast = slow;
			// das continue ist, damit fast sicher innerhalb der use-area ist
			continue;
		}
		if (os_getMapEntry(heap, fast) != 0x0) {
			zeroChunkSize = 0;
			slow = fast;
			continue;
		}
		if (os_getMapEntry(heap, fast) == 0x0 && zeroChunkSize < size) {
			fast++;
			zeroChunkSize++;
		}
		if (zeroChunkSize == size) {
			heap->nextFit = slow + size;
			return slow;
		}
	}
	return os_Memory_FirstFit(heap, size);
}

MemAddr os_Memory_WorstFit (Heap *heap, size_t size) {
	size_t currentChunkSize = 0;
	size_t biggestChunkSize = 0;
	MemAddr slow = heap->useAreaStart;
	MemAddr fast = heap->useAreaStart;
	MemAddr worst = 0;

	while (fast < (heap->useAreaStart + heap->useAreaSize)) {
		if (os_getMapEntry(heap, slow) != 0x0) {
			slow++;
			fast = slow;
			// das continue ist, damit fast sicher innerhalb der use-area ist
			continue;
		}
		if (os_getMapEntry(heap, fast) != 0x0) {
			currentChunkSize = 0;
			slow = fast;
			continue;
		}
		while ((fast < (heap->useAreaStart + heap->useAreaSize)) && os_getMapEntry(heap, fast) == 0x0) {
			fast++;
			currentChunkSize++;
		}
		if (currentChunkSize >= size && currentChunkSize >= biggestChunkSize) {
			biggestChunkSize = currentChunkSize;
			worst = slow;
		}
	}
	return worst;
}

MemAddr os_Memory_BestFit (Heap *heap, size_t size) {
	size_t currentChunkSize = 0;
	size_t smallestFittingChunkSize = heap->useAreaSize;
	MemAddr slow = heap->useAreaStart;
	MemAddr fast = heap->useAreaStart;
	MemAddr best = 0;

	while (fast < (heap->useAreaStart + heap->useAreaSize)) {
		if (os_getMapEntry(heap, slow) != 0x0) {
			slow++;
			fast = slow;
			// das continue ist, damit fast sicher innerhalb der use-area ist
			continue;
		}
		if (os_getMapEntry(heap, fast) != 0x0) {
			currentChunkSize = 0;
			slow = fast;
			continue;
		}
		while ((fast < (heap->useAreaStart + heap->useAreaSize)) && os_getMapEntry(heap, fast) == 0x0) {
			fast++;
			currentChunkSize++;
		}
		if (currentChunkSize >= size && currentChunkSize <= smallestFittingChunkSize) {
			smallestFittingChunkSize = currentChunkSize;
			best = slow;
		}
	}
	return best;
}

