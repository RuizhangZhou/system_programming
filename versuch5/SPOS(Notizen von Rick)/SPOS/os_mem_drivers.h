#ifndef _OS_MEM_DRIVERS_H
#define _OS_MEM_DRIVERS_H

#include <stdint.h>
#include <inttypes.h>

//Instruction Set in Document 23LC1024 page 6
#define CMD_WRMR 1
#define CMD_RDMR 5
#define CMD_READ 3
#define CMD_WRITE 2


#define EXT_SRAM_START (0x0)
#define EXT_MEMORY_SRAM 0xffff//64KiB?
#define DEFAULT_ALLOCATION_STRATEGY OS_MEM_FIRST

//! define type MemAddr to store 16 bits memory addresses
typedef uint16_t MemAddr;

//! define type MemValue to store the value in memory addresses
typedef uint8_t MemValue;


typedef struct {
    void (*init)(void);
    MemValue (*read)(MemAddr addr);
    void (*write)(MemAddr addr, MemValue value);
	uint16_t size;
	MemAddr start;
}MemDriver;

//! initialises heap and calls os_init() to initialise allocation table
void MemoryInitHnd(void);

//! write value in heap address addr
void MemoryWriteHnd(MemAddr addr, MemValue value);

//! reads value from heap address addr
MemValue MemoryReadHnd(MemAddr addr);

//Initialise all memory devices.
void initMemoryDevices (void);


//! Instanz des Speichertreibers 
//This specific MemDriver is initialised in os_mem_drivers.c.
MemDriver intSRAM__;

//This specific MemDriver is initialised in os_mem_drivers.c.
MemDriver extSRAM__;



//Realises a pointer to the MemDriver intSRAM__.
#define intSRAM (&intSRAM__)

//Realises a pointer to the MemDriver extSRAM__.
#define extSRAM (&extSRAM__)


#endif