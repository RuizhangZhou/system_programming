#include "os_mem_drivers.h"

#include <stdint.h>

void init(void){
    os_initHeaps();
}

MemValue read(MemAddr addr){
    return (uint8_t*) addr;
	
}

void write(MemAddr addr, MemValue value){
    (uint8_t*) addr = value;
}