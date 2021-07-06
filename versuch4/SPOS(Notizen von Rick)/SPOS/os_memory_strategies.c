#include "os_memory_strategies.h"
#include "os_memory.h"
#include "os_core.h"

MemAddr os_Memory_FirstFit (Heap *heap, size_t size){
    MemAddr chunkStart=heap->useStart;
    while(chunkStart<= heap->useStart + heap->useSize - size){
		for(size_t offset=0;offset<size;offset++){
			if(getMapEntry(heap,chunkStart+offset)!=0){
				chunkStart+=offset+1;
				break;
			}
			if(offset>=size-1){
				return chunkStart;
			}
		}
    }
    return 0;
}


MemAddr os_Memory_NextFit (Heap *heap, size_t size){
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


MemAddr os_Memory_BestFit (Heap *heap, size_t size){
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

MemAddr os_Memory_WorstFit (Heap *heap, size_t size){
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