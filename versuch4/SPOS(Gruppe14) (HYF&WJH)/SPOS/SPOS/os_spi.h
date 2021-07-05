#include "util.h"
#include "util/atomic.h"
#include "avr/interrupt.h"
#include "avr/io.h"


#ifndef OS_SPI_H_
#define OS_SPI_H_



void os_spi_init(void);
uint8_t os_spi_send(uint8_t data);
uint8_t os_spi_receive();

#endif 
