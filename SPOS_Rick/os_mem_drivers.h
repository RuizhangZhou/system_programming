#ifndef _OS_MEM_DRIVERS_H
#define _OS_MEM_DRIVERS_H

#define intSRAM (&intSRAM__)
#define extSRAM (&extSRAM__)

#include <stdint.h>

typedef uint16_t MemAddr;
typedef uint8_t MemValue;


typedef struct MemDriver {
	uint16_t start;
	uint16_t size;
	void (*init)(void);
	MemValue (*read)(MemAddr);
	void (*write)(MemAddr, MemValue);
} MemDriver;

MemValue read(MemAddr addr);
void write(MemAddr addr, MemValue value);
void init(void);


MemDriver intSRAM__;
MemDriver extSRAM__;
#endif
