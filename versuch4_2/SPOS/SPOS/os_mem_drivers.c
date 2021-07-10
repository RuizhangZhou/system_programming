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

#include "os_mem_drivers.h"
#include "defines.h"


void initSRAM(void) {
	// no initialization needed for internal SRAM
    // But we still declare a pseudo function, because MemDriver expects one for every memory device.
}

MemValue readSRAM(MemAddr addr) {
	uint8_t *pointer = (uint8_t*) addr;
	return *pointer;
}

void writeSRAM(MemAddr addr, MemValue value) {
    uint8_t *pointer = (uint8_t*) addr;
    *pointer = value;
}

MemValue readSRAM(MemAddr addr) {
    uint8_t *pointer = (uint8_t*) addr;
	return *pointer;
}

MemDriver intSRAM__ = {
    .init = initSRAM,
    .read = readSRAM,
    .write = writeSRAM,
    .start = AVR_SRAM_START,
    .size = AVR_MEMORY_SRAM,
};

MemDriver extSRAM__ = {
	.init = os_spi_init,
	.read = os_spi_read,
	.write = os_spi_write,
	.start = 0x0,
	.size = 65535
};
