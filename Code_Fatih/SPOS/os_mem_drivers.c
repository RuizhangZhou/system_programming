/*! \file
 *  \brief MemoryDriver liest und schreibt Speicher.
 *
 *  Der MemDriver ist die "untere Schicht" des Heap. Er liest und schreibt auf dem konkreten Medium.
 *
 *  \author   Matthis
 *  \date     2020-11-26
 *  \version  1.0
 */

#include "os_mem_drivers.h"
#include "defines.h"
#include "os_spi.h"

void initSRAM(void) {
	// Braucht kein init
}

MemValue readSRAM(MemAddr addr) {
	uint8_t *pointer = (uint8_t*) addr;
	return *pointer;
}

void writeSRAM(MemAddr addr, MemValue value) {
	uint8_t *pointer = (uint8_t*) addr;
	*pointer = value;
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
