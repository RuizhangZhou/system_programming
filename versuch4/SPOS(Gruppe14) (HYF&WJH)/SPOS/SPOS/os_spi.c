#include "os_spi.h"


void os_spi_init(void) {
	DDRB |= (1 << PB4) | (1 << PB5) | (1 << PB7);
	DDRB &= ~(1 << PB6);
	PORTB &= ~((1 << PB4) | (1 << PB5) | (1 << PB7));
	PORTB |= (1 << PB6);
	SPCR = (1 << SPE) | (0 << DORD) | (1 << MSTR) | (0 << CPOL) | (0 << CPHA) | (0 << SPR1) | (0 << SPR0);
	// Micro controller is Master (MSTR = 1)
	// Dataorder (DORD) 0 = Most Significant Bit first
	// (CPOL) 0 = Idle Low, Active High
	// (CPHA) 0 = Leading Edge
	// Prescaling
	// (SPR1) 1
	// (SPR0) 1
	SPSR |= (1 << SPI2X);
}

uint8_t os_spi_send(uint8_t data) {
    // uint8_t previousSREGstate = SREG;
    // SREG &= ~(0b10000000);
	
	SPDR = data;
	uint8_t receivedData = 0;
	while (!(SPSR & (1 << SPIF))) {
	}
	receivedData = SPDR;
	
    // SREG |= (previousSREGstate & (1 << 7));
	return receivedData;
}

uint8_t os_spi_receive() {
	return os_spi_send(0xFF);
}
