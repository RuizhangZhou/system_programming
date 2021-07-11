#include "util.h"
#include <avr/interrupt.h>
#include <avr/io.h>
#include <stdint.h>
#include <util/atomic.h>

// send data to memory
uint8_t os_spi_send(uint8_t data);

// initialize interactors
void os_spi_init(void);

// read data
MemValue os_spi_read(MemAddr addr);

// receive data spi
uint8_t os_spi_receive();

// write data spi
void os_spi_write(MemAddr addr, MemValue data);
