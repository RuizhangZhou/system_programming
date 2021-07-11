#include "os_memheap_drivers.h"
#include "defines.h"
#include "os_core.h"
#include <avr/pgmspace.h>

#define MAP_AREA_SIZE	((HEAPCEILING - HEAPBOTTOM) / 3)
#define USE_AREA_START	(HEAPBOTTOM + MAP_AREA_SIZE)
#define USE_AREA_SIZE	(MAP_AREA_SIZE * 2)

#define EXT_SRAM_SIZE		63999 //64KiB?
#define EXT_HEAPBOTTOM		(0x0)
#define EXT_MAP_AREA_SIZE	(EXT_SRAM_SIZE / 3)
#define EXT_USE_AREA_START	(EXT_MAP_AREA_SIZE)
#define EXT_USE_AREA_SIZE	((EXT_SRAM_SIZE / 3) * 2)


extern uint8_t const __heap_start;


const PROGMEM char intStr[] = "internal";//string?
const PROGMEM char extStr[] = "external";


Heap intHeap__ = {
	.driver = intSRAM,
	.mapStart = HEAPBOTTOM,
	.mapSize = MAP_AREA_SIZE,
	.useStart = USE_AREA_START,
	.useSize = USE_AREA_SIZE,
	.allocStrategy = OS_MEM_FIRST,
	.nextFit = USE_AREA_START,
	.name = intStr,
	.procVisitArea = { 0 },
};

Heap extHeap__ = {
	.driver = extSRAM,
	.mapStart = EXT_HEAPBOTTOM,
	.mapSize = EXT_MAP_AREA_SIZE,
	.useStart = EXT_USE_AREA_START,
	.useSize = EXT_USE_AREA_SIZE,
	.allocStrategy = OS_MEM_FIRST,
	.nextFit = EXT_USE_AREA_START,
	.name = extStr,	
	.procVisitArea = { 0 },
};

void checkIntHeapStart() {
	if ((uint16_t) &__heap_start >= HEAPBOTTOM) {
		os_error("! global  vars !!  crash heap  !");
	}
}

void os_initHeaps() {
	checkIntHeapStart();
	
	for (MemAddr i = 0; i < intHeap__.mapSize; i++) {
		intSRAM->write(intHeap__.mapStart + i, (MemValue)0x00);
	}
	
	extHeap__.driver->init();
	for (MemAddr i = 0; i < extHeap__.mapSize; i++) {
		extSRAM->write(extHeap__.mapStart + i, (MemValue)0x00);
	}
	
}

Heap* os_lookupHeap(uint8_t index) {
	if(index == 0){
		return &intHeap__;
	}
	
	return &extHeap__; 
	
}

size_t os_getHeapListLength() {
	//return sizeof(*intHeap)/sizeof(Heap);//=1 (Versuch 3)
	return 2;//just one slave as extHeap?（Versuch 4）
}


