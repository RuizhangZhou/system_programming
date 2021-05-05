#ifndef intSRAM
/*!\file
 *  
 * Contains management of several RAM devices of the OS. 
 * It contains everything that is associated with low level memory access.
 *
 * Author	Lehrstuhl Informatik 11 - RWTH Aachen
 * Date		2008, 2009, 2010, 2011, 2012
 * Version	2.0
 */

#include <inttypes.h>

//! define start of Heap
#define HEAP_START		555

//! external constant used for determining heap start
extern uint8_t const __heap_start;

//! define type MemAddr to store 16 bits memory addresses
typedef uint16_t MemAddr;

//! define type MemValue to store the value of memory addresses
typedef uint8_t MemValue;

//! pointer used to access data in SRAM 
MemAddr sram;

//! Used to store crucial information of a memory driver 
typedef struct MemDriver { 
	void (*init)(void);
	MemValue (*read)(MemAddr addr);
	void (*write)(MemAddr addr, MemValue value);
	MemAddr start;
	MemAddr size;
} MemDriver;

	
/*!
 * Represents main memory driver
 *	Initial value:
 *	= {
 *		.init = initSRAM_internal,
 *		.read = readSRAM_internal,
 *		.write = writeSRAM_internal,
 *		.start = AVR_SRAM_START,
 *		.size = AVR_MEMORY_SRAM
 *	}
 */
MemDriver intSRAM__;

//! Represents external memory driver
MemDriver extSRAM__;

//! initialises heap and calls os_init() to initialise allocation table
void init(void);

//! write value in heap address addr
void write(MemAddr addr, MemValue value);

//! reads value from heap address addr
MemValue read(MemAddr addr);

//! 
void init_external(void);

void write_external(MemAddr addr, MemValue value);


MemValue read_external(MemAddr addr);

#define     extSRAM   (&extSRAM__)
//! Used as pointer to the memory driver
#define 	intSRAM   (&intSRAM__)

#endif