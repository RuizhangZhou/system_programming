#include "os_mem_drivers.h"
#include "os_memheap_drivers.h"
#include "defines.h"
#include "os_scheduler.h"
#include "os_spi.h"
#include "util.h"
#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdint.h>
#include "os_core.h"//os_error()
/*
#define EXT_SRAM_START (0x0)
#define EXT_MEMORY_SRAM 0xffff//64KiB?
#define DEFAULT_ALLOCATION_STRATEGY OS_MEM_FIRST
*/

//Pseudo-function to initialise the internal SRAM Actually, there is nothing to be done when initialising the internal SRAM, but we create this function, because MemDriver expects one for every memory device.
void initSRAM_internal(void){
	
    if((uint16_t)&__heap_start>=INT_HEAP_BOTTOM){
	    os_error("Error:GLOBAL VARS OVERLAP HEAP");
    }
	
    for(MemAddr offset=0;offset<intHeap->mapSize;offset++){
	    intSRAM->write(intHeap->mapStart+offset,0);
    }
}

//Private function to read a value from the internal SRAM It will not check if its call is valid. This has to be done on a higher level.
MemValue readSRAM_internal(MemAddr addr){
	uint8_t *pointer=(uint8_t*) addr;
    return *pointer;	
}

//Private function to write a value to the internal SRAM It will not check if its call is valid. This has to be done on a higher level.
void writeSRAM_internal(MemAddr addr, MemValue value){
    uint8_t *pointer = (uint8_t*) addr;
	*pointer = value;
}

MemDriver intSRAM__={
	.init=initSRAM_internal,
	.read=readSRAM_internal,
	.write=writeSRAM_internal,
	.size=AVR_MEMORY_SRAM,
	.start=AVR_SRAM_START,
};


//Activates the external SRAM as SPI slave.
void select_memory(){
	//~Chipselect auf LOW setzen
	PORTB &= 0b11101111;
}

//Deactivates the external SRAM as SPI slave.
void deselect_memory(){
	//~CS auf HIGH setzen
	PORTB |= 0b00010000;
}

//Sets the operation mode of the external SRAM.
void set_operation_mode (uint8_t mode){
	os_spi_send(mode);
	if(mode==CMD_WRMR){
		os_spi_send(0);//Byte mode for MODE REGISTER
	}
}

//Transmitts a 24bit memory address to the external SRAM.
void transfer_address (MemAddr addr){
	//adresse 24 bit, don't care 8 send as first see 23LC1024 p5, bit 23-16
	os_spi_send((uint8_t)0);
	// 15-8
	os_spi_send((uint8_t)addr>>8);
	// 7-0
	os_spi_send((uint8_t)addr);
}

void initSRAM_external (){
	os_enterCriticalSection();
	os_spi_init();
	select_memory();
	
	set_operation_mode(CMD_WRMR);
	deselect_memory();
	os_leaveCriticalSection();
}

MemValue readSRAM_external (MemAddr addr){
	os_enterCriticalSection();
	select_memory();
	set_operation_mode(CMD_READ);
	transfer_address(addr);
	uint8_t res = os_spi_receive();
	deselect_memory();
	os_leaveCriticalSection();
	return res;
}

void writeSRAM_external (MemAddr addr, MemValue value){
	os_enterCriticalSection();
	select_memory();
	set_operation_mode(CMD_WRITE);
	transfer_address(addr);
	os_spi_send(value);
	deselect_memory();
	os_leaveCriticalSection();
}

MemDriver extSRAM__={
	.init=initSRAM_external,
	.read=readSRAM_external,
	.write=writeSRAM_external,
	.size=EXT_MEMORY_SRAM,
	.start=EXT_SRAM_START,
};

void initMemoryDevices(){
	os_initHeaps();
}