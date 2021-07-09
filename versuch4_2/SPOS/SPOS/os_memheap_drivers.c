/*! \file
 *  
 *
 *  \author   Johannes
 *  \date     2021-05-25
 *  \version  1.0
 */

#include "os_memheap_drivers.h"
#include "os_core.h"
#include "defines.h"
#include "lcd.h"
#include <avr/pgmspace.h>

//! defines for e.g. start address and size of map/use-section of the heap
#define MAP_SECTION_SIZE ((HEAPTOP - HEAPBOTTOM) / 3)
#define USE_SECTION_SIZE (MAP_SECTION_SIZE * 2)
#define MAP_SECTION_START HEAPBOTTOM
#define USE_SECTION_START (HEAPBOTTOM + MAP_SECTION_SIZE)


//! external constant used for determining heap start
extern uint8_t const __heap_start;

//! String for attribute name of 'intHeap__' stored in Flash memory
const PROGMEM char intStr[] = "internal";

Heap intHeap__ = {
    .driver = intSRAM,
    .mapSectionStart = MAP_SECTION_START,
    .mapSectionSize = MAP_SECTION_SIZE,
    .useSectionStart = USE_SECTION_START,
    .useSectionSize = USE_SECTION_SIZE,
    .actAllocStrategy = OS_MEM_FIRST,
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

//! function that sets HEAPBOTTOM to address of __heap_start if not less than that address
void checkIntHeapStart() {
    if ((uint16_t) &__heap_start >= HEAPBOTTOM) {
        os_error("! global  vars !!  crash heap  !");
    }
}

//! function to init the heaps by setting all nibbles of the map section to 0x0
void os_initHeaps() {
    checkIntHeapStart();

    for (MemAddr addr = MAP_SECTION_START; addr < (MAP_SECTION_START + MAP_SECTION_SIZE); addr++) {
        intSRAM->write(addr, (MemValue)0x00);
    }

    // initialization of drivers of other memory devices
    extHeap__.driver->init();
	for (MemAddr i = 0; i < extHeap__.mapAreaSize; i++) {
		extSRAM->write(extHeap__.mapAreaStart + i, (MemValue)0x00);
	}
}

//! function to get number of existing heaps
size_t os_getHeapListLength(void) {
    return 2;
}

//! function to get a pointer to heap associated with 'index'
// maybe not as define, rather direct
Heap* os_lookupHeap(uint8_t index) {
    if (index == 0) {
        return intHeap;
    }
	return extHeap;
}


