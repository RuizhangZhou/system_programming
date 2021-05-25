#include "os_memheap_drivers.h"

Heap intHeap__;

#define intHeap (&intHeap__)

void os_initHeaps(){
    for(uint8_t i=1;i<=os_getHeapListLength();i++){
        for(MemAddr j=0;j<intHeap->mapSize;j++){
            write(intHeap->mapStart+j,(MemValue)0);
        }
    }
}

uint8_t os_getHeapListLength(){
    return sizeof(*intHeap)/sizeof(Heap);
}

Heap* os_lookupHeap(uint8_t index){
    return intHeap+index;
}