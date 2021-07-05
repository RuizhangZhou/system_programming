#include <stdio.h>
#include <stdlib.h>
#include "avr/io.h"
#include "os_scheduler.h"
#include "os_memory.h"

#define CMD_WRMR 0x01
#define CMD_RDMR 0x05
#define CMD_READ 0x03
#define CMD_WRITE 0x02

void waitForSerialFinish(){
    while (!(SPSR & (1<<SPIF))) { };
}

void os_spi_slave_select(){
	//~Chipselect auf LOW setzen
	PORTB &= 0b11101111;
}

void os_spi_slave_deselect(){
	//~CS auf HIGH setzen
	PORTB |= 0b00010000;
}

uint8_t os_spi_send(uint8_t data) {
	os_enterCriticalSection();

	//send data
	SPDR = data;
	waitForSerialFinish();

	os_leaveCriticalSection();
	return SPDR;
}

uint8_t os_spi_receive() {
	
	os_enterCriticalSection();

	//DummyBits senden
	SPDR = 0xFF;
	waitForSerialFinish();
	uint8_t res = SPDR;
	
	os_leaveCriticalSection();
	return res;
}

void os_spi_wrmr(uint8_t data) {
	os_enterCriticalSection();
	os_spi_slave_select();
	os_spi_send(CMD_WRMR);
	os_spi_send(data);
	os_spi_slave_deselect();
	os_leaveCriticalSection();
}

void os_spi_write(MemAddr addr, MemValue data) {
	os_enterCriticalSection();
	os_spi_slave_select();
	os_spi_send(CMD_WRITE);
	//adresse 24 bit, don't care 8 send as first see 23LC1024 p5, bit 23-16
	os_spi_send(0x00);
	// 15-8
	os_spi_send(addr>>8);
	// 7-0
	os_spi_send(addr);
	os_spi_send(data);
	os_spi_slave_deselect();
	os_leaveCriticalSection();
}

MemValue os_spi_read(MemAddr addr) {
	os_enterCriticalSection();
	os_spi_slave_select();
	os_spi_send(CMD_READ);
	//adresse 24 bit, don't care 8 send as first see 23LC1024 p5, bit 23-16
	os_spi_send(0x00);
	// 15-8
	os_spi_send(addr>>8);
	// 7-0
	os_spi_send(addr);
	uint8_t res = os_spi_receive();
	os_spi_slave_deselect();
	os_leaveCriticalSection();
	return res; 
}

void os_spi_init(){
    
	//Pins initialisieren
	DDRB |= 0b10110000;
	DDRB &= 0b10111111;

	/* Den ~Chipselect auf HIGH setzen
	 * Das setzen des ~CS auf HIGH hat auch zur Folge, dass der Input Puffer
	 * des Speicherchips gelöscht wird, sodass wir uns am Anfang einer Kommunikation
	 * in einem definierten Zustand befinden
	 */
	//DDRB 和 PORTB有什么区别吗？
	//Data Direction Register是表明某一位是输入还是输出
	PORTB |= 0b00010000;

	/* Setzen des SPI Control & SPI Status Registers
	 * Beschreibung der Bits:
	 * 1. SPI Interrupt Enable => Wollen wir nicht, also 0
	 * 2. SPI Enable => 1
	 * 3. Data Order => 0 bedeutet MSB first (ben�tigt f�r den Speicherchip)
	 * 4. Master Bit => Der Atmega ist der Master, also 1
	 * 5. Clock Polarity => 0 bedeutet, dass wenn keine Daten gesendet werden, der PIN auf LOW liegt
	 * 6. Clock Phase => 0 bedeutet CLK Signal bei steigender Flanke
	 * 7. CLOCK SPEED => Setzt die Bits f�r die SPI Frequenz nach der Tabelle
	 * 8. CLOCK SPEED => aus der Dokumentation des ATmegas (hier max. 10MHz)
	 */
	SPCR = 0b01010000;

	// Setzen des SPI Status Registers. Wir m�ssen nur das letze Bit
	// (CLK Double Speed) auf 1 setzen (s. Freq Tabelle AVR Doku)
	SPSR |= 0b00000001;

	os_spi_wrmr(0x00);
}