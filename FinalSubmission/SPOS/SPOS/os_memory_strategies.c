#include "os_memory_strategies.h"
#include "os_memory.h"

MemAddr os_Memory_FirstFit (Heap *heap, size_t size) {
	size_t zeroChunkSize = 0;
	MemAddr front = heap->useStart;
	MemAddr back = heap->useStart;

	while (back < (heap->useStart + heap->useSize)) {
		if (os_getMapEntry(heap, front) != 0x0) {
			front++;
			back = front;
			continue;
		}
		if (os_getMapEntry(heap, back) != 0x0) {
			zeroChunkSize = 0;
			front = back;
			continue;
		}
		if (os_getMapEntry(heap, back) == 0x0 && zeroChunkSize < size) {
			back++;
			zeroChunkSize++;
		}
		if (zeroChunkSize == size) {
			return front;
		}
	}
	return 0;
}



MemAddr os_Memory_BestFit (Heap *heap, size_t size) {
	size_t currentChunkSize = 0;
	size_t smallestFittingChunkSize = heap->useSize;
	MemAddr front = heap->useStart;
	MemAddr back = heap->useStart;
	MemAddr best = 0;

	while (back < (heap->useStart + heap->useSize)) {
		if (os_getMapEntry(heap, front) != 0x0) {
			front++;
			back = front;
			continue;
		}
		if (os_getMapEntry(heap, back) != 0x0) {
			currentChunkSize = 0;
			front = back;
			continue;
		}
		while ((back < (heap->useStart + heap->useSize)) && os_getMapEntry(heap, back) == 0x0) {
			back++;
			currentChunkSize++;
		}
		if (currentChunkSize >= size && currentChunkSize <= smallestFittingChunkSize) {
			smallestFittingChunkSize = currentChunkSize;
			best = front;
		}
	}
	return best;
}

MemAddr os_Memory_WorstFit (Heap *heap, size_t size) {
	size_t currentChunkSize = 0;
	size_t biggestChunkSize = 0;
	MemAddr front = heap->useStart;
	MemAddr back = heap->useStart;
	MemAddr worst = 0;

	while (back < (heap->useStart + heap->useSize)) {
		if (os_getMapEntry(heap, front) != 0x0) {
			front++;
			back = front;
			continue;
		}
		if (os_getMapEntry(heap, back) != 0x0) {
			currentChunkSize = 0;
			front = back;
			continue;
		}
		while ((back < (heap->useStart + heap->useSize)) && os_getMapEntry(heap, back) == 0x0) {
			back++;
			currentChunkSize++;
		}
		if (currentChunkSize >= size && currentChunkSize >= biggestChunkSize) {
			biggestChunkSize = currentChunkSize;
			worst = front;
		}
	}
	return worst;
}

MemAddr os_Memory_NextFit (Heap *heap, size_t size) {
	size_t zeroChunkSize = 0;
	MemAddr front = heap->nextFit;
	MemAddr back = heap->nextFit;

	while (back < (heap->useStart + heap->useSize)) {
		if (os_getMapEntry(heap, front) != 0x0) {
			front++;
			back = front;
			continue;
		}
		if (os_getMapEntry(heap, back) != 0x0) {
			zeroChunkSize = 0;
			front = back;
			continue;
		}
		if (os_getMapEntry(heap, back) == 0x0 && zeroChunkSize < size) {
			back++;
			zeroChunkSize++;
		}
		if (zeroChunkSize == size) {
			heap->nextFit = front + size;
			return front;
		}
	}
	return os_Memory_FirstFit(heap, size);
}