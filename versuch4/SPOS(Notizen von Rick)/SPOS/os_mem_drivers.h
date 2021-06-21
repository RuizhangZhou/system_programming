#ifndef _OS_MEM_DRIVERS_H
#define _OS_MEM_DRIVERS_H

#include <stdint.h>



//! define type MemAddr to store 16 bits memory addresses
typedef uint16_t MemAddr;

//! define type MemValue to store the value in memory addresses
typedef uint8_t MemValue;


typedef struct {
    void (*init)(void);
    MemValue (*read)(MemAddr addr);
    void (*write)(MemAddr addr, MemValue value);
}MemDriver;

//! initialises heap and calls os_init() to initialise allocation table
void init(void);

//! write value in heap address addr
void write(MemAddr addr, MemValue value);

//! reads value from heap address addr
MemValue read(MemAddr addr);

//! Instanz des Speichertreibers 
MemDriver intSRAM__;

#define intSRAM (&intSRAM__)




#endif