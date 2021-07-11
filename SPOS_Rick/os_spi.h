/*! \file
 *  \brief Heap.
 *
 *  Driver for AVRs SPI module.
 *
 */
#include <stdint.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/atomic.h>
#include "util.h"

//Configures relevant I/O registers/pins and initializes the SPI module.
void os_spi_init(void);

//send informationen zu Slave 
uint8_t os_spi_send(uint8_t data);

//empfang info from slave
uint8_t os_spi_receive();

MemValue os_spi_read(MemAddr addr);

void os_spi_write(MemAddr addr, MemValue data);


