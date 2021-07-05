#include "avr/io.h"//DDRB,PORTB,SPCR,SPSR...
#include "os_spi.h"
#include "os_scheduler.h"//os_enterCriticalSection();

void os_spi_init (){
    //Pins initialisieren
    //DDR: 0 means input, 1 means output
    //bit 0-3 keep unchanged, set bit CS(4),SI(5),SCK(7) as 1, bit SO(6) as 0
	//here this Microchip works as a Master?
    //as a Master the CS doesn't matter anything?
    DDRB |= 0b10110000;
	DDRB &= 0b10111111;

    SPCR = ((1<<MSTR) | (1<<SPE)); //only sed MSTR and SPE to 1,don't activate SPIE
    //the fasted speed is SPR1=0,SPR0=0,SPI2X=1: fCPU/2 ?
    SPSR |= 0b00000001;//double speed
}

uint8_t os_spi_send (uint8_t data){
    os_enterCriticalSection();
    SPDR=data;
    while(!(SPSR>>SPIF)){/* activ gewartet if bit(SPIF)=0 */}
    //while(!(SPSR & (1<<SPIF))){ /* Wait for serial finish */ }//alternative implement
    os_leaveCriticalSection();
    return SPDR;
}

uint8_t os_spi_receive (){
    os_enterCriticalSection();
    SPDR=DUMMY;
    while(!(SPSR>>SPIF)){/* activ gewartet if SPIF=0 */}
    uint8_t res = SPDR;//fit to return type uint8_t
    os_leaveCriticalSection();
    return res;//when receive always send the DUMMY-Byte
}