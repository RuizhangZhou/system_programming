#include "os_spi.h"
#include "os_scheduler.h"
#include "avr/io.h"

/*! \file 
 *  Basic functions for setup and use of the SPI module.
 * 
 *  \author		Lehrstuhl Informatik 11 - RWTH Aachen
 *  \date		2018
 *  \version	1.0
*/


// Globals
#define CMD_WRMR 0x01
#define CMD_RDMR 0x05
#define CMD_READ 0x03
#define CMD_WRITE 0x02
#define DUMMY 0xff



void os_spi_init(){
	DDRB |= (1<<7)|(1<<4)|(1<<5);    // SCK, MOSI and SS as outputs
    DDRB &= ~(1<<6);                 // MISO as input
	
    SPCR = ((1<<MSTR) | (1<<SPE));   // Set as Master and Enable SPI
    SPCR |= (1<<SPR0);				 // divided clock by 16 	
}


/*!
 * \param	data	The byte which is send to the slave
 * \returns			The byte received from the slave
 */
uint8_t os_spi_send(uint8_t data){
	os_enterCriticalSection();
	
	// use SPDR to send data
	SPDR = data;
	
	while(!(SPSR & (1<<SPIF))){ /* Wait for serial finish */ }
		
	os_leaveCriticalSection();
	return SPDR;
}

/*!
 * 
 * \returns The byte received from the slave
 */
uint8_t os_spi_receive(){
	os_enterCriticalSection();
	
	// read out
	SPDR = DUMMY;
	while(!(SPSR & (1<<SPIF))){ /* Wait for serial finish */ }
	uint8_t val = SPDR;
	
	os_leaveCriticalSection();
	return val;
}


void os_spi_slave_select(){
	// set ~CS low
	PORTB &= 0b11101111;
}


void os_spi_slave_deselect(){
	// set ~CS high
	PORTB |= 0b00010000;
}

/*!
 *
 */
void os_spi_write(MemValue value, MemAddr addr) {
	os_enterCriticalSection();
	// select ~CS as slave
	os_spi_slave_select();
	os_spi_send(CMD_WRITE);
	os_spi_send((uint8_t)0);
	os_spi_send((uint8_t)(addr>>8));
	os_spi_send((uint8_t)addr);
	os_spi_send(value);
	// restore
	os_spi_slave_deselect();
	os_leaveCriticalSection();
}

/*!
 *
 */
MemValue os_spi_read(MemAddr addr) {
	os_enterCriticalSection();
	// select ~CS as slave
	os_spi_slave_select();
	os_spi_send(CMD_READ);
	os_spi_send((uint8_t)0);
	os_spi_send((uint8_t)(addr>>8));
	os_spi_send((uint8_t)addr);
	uint8_t res = os_spi_receive();
	// restore
	os_spi_slave_deselect();
	os_leaveCriticalSection();
	return res;
}