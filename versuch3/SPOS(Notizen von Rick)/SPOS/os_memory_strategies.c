#include "os_memory_strategies.h"
#include "os_memory.h"


MemAddr os_Memory_FirstFit (Heap *heap, size_t size){
    size_t freeSize=0;
    MemAddr curAddr=heap->useStart;

    while(curAddr<=heap->useStart+heap->useSize-size){
        if(getMapEntry(heap,curAddr)==0){
            freeSize++;
        }else{
            freeSize=0;
        }
        
        curAddr++;

        if(freeSize>=size){
            return curAddr-size;
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