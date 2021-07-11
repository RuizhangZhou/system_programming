#include "os_memheap_drivers.h"
#include "defines.h"
#include "os_core.h"
#include <avr/pgmspace.h>

#define MAP_SIZE ((HEAPCEILING - HEAPBOTTOM) / 3)


extern uint8_t const __heap_start;

const PROGMEM char intStr[] = "internal";

const PROGMEM char extStr[] = "external";


Heap intHeap__ = {
	.driver = intSRAM,
	.startOfMapArea = HEAPBOTTOM,
	.sizeOfMapArea = MAP_SIZE,
	.startOfUseArea = (HEAPBOTTOM + MAP_SIZE),
	.sizeOfUseArea = (MAP_SIZE * 2),
	.allocationStrategy = OS_MEM_FIRST,
	.nextOne = (HEAPBOTTOM + MAP_SIZE),
	.name = intStr,
	.visitedAreas = { 0 },
};

Heap extHeap__ = {
	.driver = extSRAM,
	.startOfMapArea = 0,
	.sizeOfMapArea = 21333,
	.startOfUseArea = 21333,
	.sizeOfUseArea = 42666,
	.allocationStrategy = OS_MEM_FIRST,
	.nextOne = 21333,
	.name = extStr,	
	.visitedAreas = { 0 },
};

void controlHeapStart() {
	if ((uint16_t) &__heap_start >= HEAPBOTTOM) {
		os_error("big error! globl vars crash heap!");
	}
}

void os_initHeaps() {
	controlHeapStart();
	
	for (MemAddr i = 0; i < intHeap__.sizeOfMapArea; i++) {
		intSRAM->write(i + intHeap__.startOfMapArea, (MemValue)0);
	}
	
	extHeap__.driver->init();
	for (MemAddr i = 0; i < extHeap__.sizeOfMapArea; i++) {
		extSRAM->write(i + extHeap__.startOfMapArea, (MemValue)0);
	}
	
}

Heap* os_lookupHeap(uint8_t i) {
	if(i == 0){
		return &intHeap__;
	} else {
		return &extHeap__; 
	}
}

size_t os_getHeapListLength() {
	return 2;
}


