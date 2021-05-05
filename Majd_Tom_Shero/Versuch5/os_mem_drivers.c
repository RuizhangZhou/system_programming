#include "defines.h"
#include "os_mem_drivers.h"
#include "os_memheap_drivers.h"
#include "os_memory.h"
#include "os_core.h"
#include "os_spi.h"

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
	/*if ((addr < intHeap->start_address) || (addr > os_getUseSize(intHeap) + os_getUseStart(intHeap)) ) {
		os_error("Address Not Valid!");
		return 0;
	}*/
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
	/*if ((addr < intHeap->start_address) || (addr > os_getUseSize(intHeap) + os_getUseStart(intHeap))) {
		os_error("Address Not Valid!");
		return;
	}*/
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
		os_error("Reseting Heap   Press Skip ");
		intHeap->start_address = (uint16_t)&(__heap_start);
	}
	
	// initialise block size, map size i.e. allocation table and driver
 	intHeap->map_size = (((AVR_MEMORY_SRAM / 2) - intHeap->start_address) / 3);
	intHeap->data_block = 2*(intHeap->map_size);
	intHeap->driver = intSRAM;
	
	// initialise the intSRAM structure
	intSRAM->init = &init;
	intSRAM->read = &read;
	intSRAM->write = &write;
	
	// initialise map values
	os_initHeaps();
}

/*!
 *	Initialisation of the external SRAM board. This function performs actions such as setting 
 *	the respective Data Direction Register etc..
 */
void init_external(void) {
	os_spi_init();
	if((PORTB&1<<5) == 1)
		os_error("SLAVE");
	// initialise external SRAM
	extSRAM->init = &init_external;
	extSRAM->read = &read_external;
	extSRAM->write = &write_external;
	extSRAM->start = 0;
	extSRAM->size = 63999;
	
	// Set the heap's driver, map size and use size.  
	extHeap->driver = extSRAM;
	extHeap->map_size = (63999/3);
	extHeap->data_block = 2*(extHeap->map_size);
	extHeap->start_address = 0;	
}

/*!
 *	Private function to write a single byte to the external SRAM It will not check if its 
 *	call is valid. This has to be done on a higher level
 *
 *	\param addr	The address the value shall be written to
 *	\param value	The value to be written
 */
void write_external(MemAddr addr, MemValue value){
	// if addr not in heap
	if ((addr < extHeap->start_address) || (addr > os_getUseSize(extHeap) + os_getUseStart(extHeap))) {
		os_errorPStr("Address Not Valid!");
		return;
	}
	os_spi_write(value, addr);
}

/*!
 *	Private function to read a single byte to the external SRAM It will not check if 
 *	its call is valid. This has to be done on a higher level.
 *	\param addr	The address to read the value from
 *	\returns The read value
 */
MemValue read_external(MemAddr addr){
	// if address not in heap
	if ((addr < extHeap->start_address) || (addr > os_getUseSize(extHeap) + os_getUseStart(extHeap)) ) {
		os_errorPStr("Address Not Valid!");
		return 0;
	}
	return os_spi_read(addr);
}