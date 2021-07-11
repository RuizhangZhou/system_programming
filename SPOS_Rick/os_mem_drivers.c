#include "os_mem_drivers.h"
#include "defines.h"
#include "os_spi.h"

void initSRAM(void) { }

MemValue readSRAM(MemAddr adress) {
	uint8_t *p = (uint8_t*) adress;
	return *p;
}

void writeSRAM(MemAddr adress, MemValue val) {
	uint8_t *p = (uint8_t*) adress;
	*p = val;
}

MemDriver intSRAM__ = {
	.init = initSRAM,
	.read = readSRAM,
	.write = writeSRAM,
	.start = AVR_SRAM_START,
	.size = AVR_MEMORY_SRAM
};

MemDriver extSRAM__ = {
	.init = os_spi_init,
	.read = os_spi_read,
	.write = os_spi_write,
	.start = 0x0,
	.size = 65535
};
