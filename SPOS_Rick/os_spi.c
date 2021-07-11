#include "avr/io.h"
#include "os_memory.h"
#include "os_scheduler.h"
#include <stdio.h>
#include <stdlib.h>

#define CMD_WRMR 0x01
#define CMD_WRITE 0x02

#define CMD_READ 0x03
#define CMD_RDMR 0x05

// selaect slave
void os_spi_sel_slave() { PORTB &= 0b11101111; }

// wait until serial ends interaction
void waitSerialToEnd() {

  while (!(SPSR & (1 << SPIF))) {
    // loop and do nothing
  };
}

// deselect slave
void os_spi_desel_slave() { PORTB |= 0b00010000; }

// send data to the spi external SRAM
uint8_t os_spi_send(uint8_t data) {
  os_enterCriticalSection();

  SPDR = data;
  waitSerialToEnd();

  os_leaveCriticalSection();
  return SPDR;
}

// receive data from spi external SRAM
uint8_t os_spi_receive() {

  os_enterCriticalSection();

  SPDR = 0xFF;

  waitSerialToEnd();

  uint8_t result = SPDR;

  os_leaveCriticalSection();
  return result;
}

MemValue os_spi_read(MemAddr addr) {
  os_enterCriticalSection();

  os_spi_sel_slave();
  os_spi_send(CMD_READ);

  os_spi_send(0x00);
  os_spi_send(addr >> 8);

  os_spi_send(addr);
  uint8_t result = os_spi_receive();
  os_spi_desel_slave();

  os_leaveCriticalSection();
  return result;
}

void os_spi_write(MemAddr addr, MemValue data) {
  os_enterCriticalSection();

  os_spi_sel_slave();
  os_spi_send(CMD_WRITE);

  os_spi_send(0x00);
  os_spi_send(addr >> 8);

  os_spi_send(addr);
  os_spi_send(data);
  os_spi_desel_slave();

  os_leaveCriticalSection();
}

void os_spi_wrmr(uint8_t data) {
  os_enterCriticalSection();

  // select slace
  os_spi_sel_slave();
  os_spi_send(CMD_WRMR);
  os_spi_send(data);
  // deselect slave
  os_spi_desel_slave();

  os_leaveCriticalSection();
}

// initializes spi external SRAM
void os_spi_init() {

  DDRB |= 0b10110000;
  DDRB &= 0b10111111;

  PORTB |= 0b00010000;

  SPCR = 0b01010000;

  SPSR |= 0b00000001;

  os_spi_wrmr(0x00);
}
