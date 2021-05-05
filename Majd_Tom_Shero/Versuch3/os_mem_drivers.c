#include "defines.h"
#include "os_mem_drivers.h"
#include "os_memheap_drivers.h"
#include "os_memory.h"
#include "os_core.h"

/*! \file
 *  \brief Memory Driver structs and their functions. 
 *	
 *	Note that all handy defines used to implement the driver functions are 
 *  hidden in the public version of this documentation.
 *
 */




/*!
 * check the address given in the parameter and then return the 
 * value read from the address. 
 *
 * \param addr The memory address to be read
 * \return the address's value in SRAM
 */
MemValue read(MemAddr addr){
	// if address not in heap
	if ((addr < intHeap->start_address) || (addr > os_getUseSize(intHeap) + os_getUseStart(intHeap)) ) {
		os_errorPStr("Address Not Valid!");
		return 0;
	}
	uint8_t * ptr = (uint8_t*)addr;
	return *ptr;
}


/*!
 * write the value given in the specified address. 
 *
 * \param addr The memory address to be overwritten
 * \param value The value to be written
 */
void write(MemAddr addr, MemValue value){
	// if addr not in heap
	if ((addr < intHeap->start_address) || (addr > os_getUseSize(intHeap) + os_getUseStart(intHeap))) {
		os_errorPStr("Address Not Valid!");
		return;
	}
	uint8_t * ptr = (uint8_t*)addr;
	*ptr = value;
}


/*!
 * initialises the heap by setting the start address of the heap
 * and of the corresponding sizes of the data block and allocation table. 
 * then initialises the components of intSRAM struct.
 */
void init(void) {	
	// compare test value with __heap_start and set a secure distance
	intHeap->start_address = HEAP_START;
	if ((intHeap->start_address) < (uint16_t)&(__heap_start)){
		os_errorPStr("Reseting Heap   Start...");
		intHeap->start_address = (uint16_t)&(__heap_start);
	}
	
	// initialise block size, map size i.e. allocation table and driver
 	intHeap->map_size = (((AVR_MEMORY_SRAM / 2) - intHeap->start_address) / 3);
	intHeap->data_block = 2*(intHeap->map_size);
	intHeap->driver = intSRAM;
	
	// initialise map values
	os_initHeaps();
	
	// initialise the intSRAM structure
	intSRAM->init = &init;
	intSRAM->read = &read;
	intSRAM->write = &write;
}