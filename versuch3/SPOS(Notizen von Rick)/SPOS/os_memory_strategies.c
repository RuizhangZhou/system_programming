#include "os_memory_strategies.h"
#include "os_memory.h"


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
	return 0;
}


MemAddr os_Memory_BestFit (Heap *heap, size_t size){
	return 0;
}


MemAddr os_Memory_WorstFit (Heap *heap, size_t size){
	return 0;
}