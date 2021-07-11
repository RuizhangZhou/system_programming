#ifndef _OS_MEM_DRIVERS_H
#define _OS_MEM_DRIVERS_H

#define intSRAM (&intSRAM__)
#define extSRAM (&extSRAM__)

#include <stdint.h>

//Instruction Set in Document 23LC1024 page 6
/*
#define CMD_WRMR 1
#define CMD_RDMR 5
#define CMD_READ 3
#define CMD_WRITE 2
*/

//! define type MemAddr to store 16 bits memory addresses
typedef uint16_t MemAddr;

//! define type MemValue to store the value in memory addresses
typedef uint8_t MemValue;

/*!
 *  Enth�lt "charakteristische Werte des Speichermediums sowie 
 *  Funktionszeiger auf init, read und write
 */
typedef struct MemDriver {
	uint16_t start;
	uint16_t size;
	void (*init)(void);
	MemValue (*read)(MemAddr);
	void (*write)(MemAddr, MemValue);
} MemDriver;

//! initialises heap and calls os_init() to initialise allocation table
void init(void);

//! reads value from heap address addr
MemValue read(MemAddr addr);

//! write value in heap address addr
void write(MemAddr addr, MemValue value);

//This specific MemDriver is initialised in os_mem_drivers.c.
MemDriver intSRAM__;

//This specific MemDriver is initialised in os_mem_drivers.c.
MemDriver extSRAM__;

#endif