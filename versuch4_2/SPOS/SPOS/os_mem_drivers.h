/*! \file
 *  \brief MemoryDriver liest und schreibt im Speicher.
 *
 *  Der MemDriver ist die "untere Schicht" der Heap-Implementierung.
 *  Er liest und schreibt auf dem konkreten Medium und ist damit "Hardware-speziefisch".
 *
 *  \author   Johannes
 *  \date     2021-05-24
 *  \version  1.0
 */

#ifndef _OS_MEM_DRIVERS_H
#define _OS_MEM_DRIVERS_H

#include <stdint.h>

//! Pointer to intSRAM instance 
#define intSRAM (&intSRAM__)

//! Memory address managed by the OS
typedef uint16_t MemAddr;

//! Memory atom, that can be assigned to exactly one process
typedef uint8_t MemValue;

typedef struct MemDriver {
    uint16_t start;
	uint16_t size;
    void (*init)(void);
    MemValue (*read)(MemAddr addr);
    void (*write)(MemAddr addr, MemValue value);
} MemDriver;

//! initialises memory device
void init(void);

//! write value to heap address addr
void write(MemAddr addr, MemValue value);

//! reads value from heap address addr
MemValue read(MemAddr addr);

//! SRAM driver instance 
MemDriver intSRAM__;

#endif