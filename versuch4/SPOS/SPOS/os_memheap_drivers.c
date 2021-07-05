#include "os_memheap_drivers.h"
#include "defines.h"
#include "os_core.h"
#include <avr/pgmspace.h>

/*
#define HEAP_BOTTOM	(0X100+200)
#define STACK_TOP	PROCESS_STACK_BOTTOM(MAX_NUMBER_OF_PROCESSES)
#define HEAP_TOP	STACK_TOP
#define HEAP_SIZE   (HEAP_TOP-HEAP_BOTTOM)
*/
extern uint8_t const __heap_start;

Heap intHeap__={
	.driver=intSRAM,
	.mapStart=HEAP_BOTTOM,
	.mapSize=HEAP_SIZE/3,
	.useStart=HEAP_BOTTOM+HEAP_SIZE/3,
	.useSize=(HEAP_SIZE/3)*2,
	.currentAllocStrategy=OS_MEM_FIRST,
};



void os_initHeaps(){
	/*
    for(uint8_t i=1;i<=os_getHeapListLength();i++){
        for(MemAddr j=0;j<intHeap->mapSize;j++){
            write(intHeap->mapStart+j,(MemValue)0);
        }
    }
	*/
	if((uint16_t)&__heap_start>=HEAP_BOTTOM){
		os_error("Error:GLOBAL VARS OVERLAP HEAP");
	}
	
	for(MemAddr offset=0;offset<intHeap->mapSize;offset++){
		write(intHeap->mapStart+offset,0);
	}
	
}

uint8_t os_getHeapListLength(){
    return sizeof(*intHeap)/sizeof(Heap);
}

Heap* os_lookupHeap(uint8_t index){
    return intHeap+index;
}