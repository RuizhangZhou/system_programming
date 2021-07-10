/*! \file
 *  \author   Matthis
 *  \date     2020-11-26
 *  \version  1.0
 */

#include "os_memheap_drivers.h"
#include "defines.h"
#include "os_core.h"
#include <avr/pgmspace.h>

#define MAP_AREA_SIZE	((HEAPCEILING - HEAPBOTTOM) / 3)
#define USE_AREA_START	(HEAPBOTTOM + MAP_AREA_SIZE)
#define USE_AREA_SIZE	(MAP_AREA_SIZE * 2)

#define EXT_SRAM_SIZE		63999
#define EXT_HEAPBOTTOM		(0x0)
#define EXT_MAP_AREA_SIZE	(EXT_SRAM_SIZE / 3)
#define EXT_USE_AREA_START	(EXT_MAP_AREA_SIZE)
#define EXT_USE_AREA_SIZE	((EXT_SRAM_SIZE / 3) * 2)


extern uint8_t const __heap_start;


const PROGMEM char intStr[] = "internal";
const PROGMEM char extStr[] = "external";


Heap intHeap__ = {
	.driver = intSRAM,
	.mapAreaStart = HEAPBOTTOM,
	.mapAreaSize = MAP_AREA_SIZE,
	.useAreaStart = USE_AREA_START,
	.useAreaSize = USE_AREA_SIZE,
	.allocStrategy = OS_MEM_FIRST,
	.nextFit = USE_AREA_START,
	.name = intStr,
	.procVisitArea = { 0 },
};

Heap extHeap__ = {
	.driver = extSRAM,
	.mapAreaStart = EXT_HEAPBOTTOM,
	.mapAreaSize = EXT_MAP_AREA_SIZE,
	.useAreaStart = EXT_USE_AREA_START,
	.useAreaSize = EXT_USE_AREA_SIZE,
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
	
	for (MemAddr i = 0; i < intHeap__.mapAreaSize; i++) {
		intSRAM->write(intHeap__.mapAreaStart + i, (MemValue)0x00);
	}
	
	extHeap__.driver->init();
	for (MemAddr i = 0; i < extHeap__.mapAreaSize; i++) {
		extSRAM->write(extHeap__.mapAreaStart + i, (MemValue)0x00);
	}
	
}

Heap* os_lookupHeap(uint8_t index) {
	if(index == 0){
		return &intHeap__;
	}
	
	return &extHeap__; 
	
}

size_t os_getHeapListLength() {
	return 2;
}


