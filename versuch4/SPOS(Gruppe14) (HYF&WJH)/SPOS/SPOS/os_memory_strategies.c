#include "os_memory_strategies.h"
#include "os_memory.h"
#include "os_memheap_drivers.h"


MemAddr os_Memory_FirstFit (Heap *heap, size_t size){
    
    MemAddr chunkStart=heap->useStart;

    while(chunkStart<= heap->useStart + heap->use_size - size){
        
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
	if (size == 0)
	return 0;
	
	MemAddr startAddr = heap->useStart;
	MemAddr endAddr = (heap->useStart + heap->use_size)-1;
	
	if (lastUsedMemAddr == 0)
	lastUsedMemAddr = startAddr;
	
	MemAddr firstRun = os_Memory_CheckChunk(heap, size, lastUsedMemAddr, endAddr, true);
	if (firstRun != 0) return firstRun;
	else return os_Memory_CheckChunk(heap, size, startAddr, (lastUsedMemAddr + os_Memory_GetChunkSize(heap, lastUsedMemAddr)), true);
}


MemAddr os_Memory_BestFit (Heap *heap, size_t size){
	if (size == 0) return 0;
	
	MemAddr currentAddr = heap->useStart;
	MemAddr endAddr = (heap->useStart + heap->useSize)-1;
	MemAddr chunkBufAddr;
	MemAddr bigEnoughChunkAddr = 0;
	size_t chunkSize;
	
	while (currentAddr < endAddr) {
		chunkSize = 0;
		if (os_getMapEntry(heap, currentAddr) == 0) {
			chunkBufAddr = currentAddr;
			while (os_getMapEntry(heap, chunkBufAddr) == 0) {
				chunkSize++;
				chunkBufAddr++;
				
				if (chunkBufAddr == endAddr) break;
			}
			currentAddr += chunkSize;
			if (chunkSize == size) return (currentAddr - chunkSize);
			else if (chunkSize > size) {
				if (chunkSize < bigEnoughChunkAddr || bigEnoughChunkAddr == 0) bigEnoughChunkAddr = (currentAddr - chunkSize);
			}
			} else {
			currentAddr++;
		}
	}
	
	if (bigEnoughChunkAddr != 0) return bigEnoughChunkAddr;
	
	//No big enough chunk was found. Return 0
	return 0;
}


MemAddr os_Memory_WorstFit (Heap *heap, size_t size){
	if (size == 0) return 0;
	
	MemAddr currentAddr = heap->useStart;
	MemAddr endAddr = (heap->useStart + heap->use_size)-1;
	MemAddr chunkBufAddr;
	MemAddr biggestChunkAddr = 0;
	size_t biggestChunkSize = 0;
	size_t chunkSize;
	
	while (currentAddr < endAddr) {
		chunkSize = 0;
		if (os_getMapEntry(heap, currentAddr) == 0) {
			chunkBufAddr = currentAddr;
			while (os_getMapEntry(heap, chunkBufAddr) == 0) {
				chunkSize++;
				chunkBufAddr++;
				
				if (chunkBufAddr == endAddr) break;
			}
			currentAddr += chunkSize;
			if (chunkSize >= size) {
				if (chunkSize > biggestChunkSize) {
					biggestChunkSize = chunkSize;
					biggestChunkAddr = (currentAddr - chunkSize);
				}
			}
			} else {
			currentAddr++;
		}
	}
	
	if (biggestChunkSize != 0) return biggestChunkAddr;

	return 0;
}