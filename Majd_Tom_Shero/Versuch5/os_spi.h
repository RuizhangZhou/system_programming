#ifndef os_spi_init

#include <stdint.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/atomic.h>
#include "util.h"
#include "os_mem_drivers.h"

//! Configures relevant I/O registers/pins and initializes the SPI module.
void 	os_spi_init (void);

//! Performs a SPI send This method blocks until the data exchange is completed.
uint8_t 	os_spi_send (uint8_t data);

//! Performs a SPI read This method blocks until the exchange is completed.
uint8_t 	os_spi_receive ();

//! selects external device as slave
void os_spi_slave_select();

//! deselects external device as slave
void os_spi_slave_deselect();

//! writes into external memory
void os_spi_write(MemValue value, MemAddr addr);

//! reads from external memory
MemValue os_spi_read(MemAddr addr);

#endif