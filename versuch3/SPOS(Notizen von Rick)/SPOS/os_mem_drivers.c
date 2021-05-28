#include "os_mem_drivers.h"
#include "os_memheap_drivers.h"

#include <stdint.h>

MemDriver intSRAM__={
	.init=init,
	.read=read,
	.write=write,
};

void init(void){
    os_initHeaps();
}

MemValue read(MemAddr addr){
	uint8_t *pointer=(uint8_t*) addr;
    return *pointer;
	
}

void write(MemAddr addr, MemValue value){
    uint8_t *pointer = (uint8_t*) addr;
	*pointer = value;
}