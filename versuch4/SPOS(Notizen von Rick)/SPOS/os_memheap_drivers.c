#include "os_memheap_drivers.h"
#include "defines.h"
#include "os_memory_strategies.h"
#include "os_core.h"//os_error()
#include <avr/pgmspace.h>


#define INT_HEAP_BOTTOM	(0X100+200)
#define STACK_TOP	PROCESS_STACK_BOTTOM(MAX_NUMBER_OF_PROCESSES)
#define INT_HEAP_TOP	STACK_TOP
#define INT_HEAP_SIZE   (INT_HEAP_TOP-INT_HEAP_BOTTOM)

#define EXT_SRAM_START (0x0)
#define EXT_MEMORY_SRAM 65536//how is this number come from? 64KiB?
#define DEFAULT_ALLOCATION_STRATEGY OS_MEM_FIRST

extern uint8_t const __heap_start;

const PROGMEM char intStr[] = "internal";
const PROGMEM char extStr[] = "external";

Heap intHeap__={
	.driver=intSRAM,
	.mapStart=INT_HEAP_BOTTOM,
	.mapSize=INT_HEAP_SIZE/3,
	.useStart=INT_HEAP_BOTTOM+INT_HEAP_SIZE/3,
	.useSize=(INT_HEAP_SIZE/3)*2,
	.strategy=DEFAULT_ALLOCATION_STRATEGY,
	.name=intStr
};

Heap extHeap__= {
  .driver = extSRAM,
  .mapStart = EXT_SRAM_START,
  .mapSize = EXT_MEMORY_SRAM/3,
  .useStart = EXT_SRAM_START + EXT_MEMORY_SRAM/3,
  .useSize = 2 * (EXT_MEMORY_SRAM/3),
  .strategy = DEFAULT_ALLOCATION_STRATEGY,
  .lastChunk = EXT_SRAM_START + EXT_MEMORY_SRAM/3,
  .name = extStr
};

void os_initHeaps(){
	/*
    for(uint8_t i=1;i<=os_getHeapListLength();i++){
        for(MemAddr j=0;j<intHeap->mapSize;j++){
            write(intHeap->mapStart+j,(MemValue)0);
        }
    }
	*/
	if((uint16_t)&__heap_start>=INT_HEAP_BOTTOM){
		os_error("Error:GLOBAL VARS OVERLAP HEAP");
	}
	
	for(MemAddr offset=0;offset<intHeap->mapSize;offset++){
		intSRAM->write(intHeap->mapStart+offset,0);
	}

	extHeap->driver->init();
	for(MemAddr offset=0; offset<extHeap->mapSize;offset++) {
		extSRAM->write(extHeap->mapStart+offset,0);
	}
}

uint8_t os_getHeapListLength(){
    //return sizeof(*intHeap)/sizeof(Heap);
	return 2;
}

Heap* os_lookupHeap(uint8_t index){
	if(index==0){
		return intHeap;
	}else{
		return extHeap;
	}
}