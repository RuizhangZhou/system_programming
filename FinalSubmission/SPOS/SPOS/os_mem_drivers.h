#ifndef _OS_MEM_DRIVERS_H
#define _OS_MEM_DRIVERS_H

#define intSRAM (&intSRAM__)
#define extSRAM (&extSRAM__)

#include <stdint.h>

//! Vom Betriebssystem verwaltete Speicheradresse
typedef uint16_t MemAddr;

//! Speicheratom, das genau einem Prozess zugeteilt werden kann.
typedef uint8_t MemValue;

/*!
 *  Enth�lt "charakteristische Werte des Speichermediums sowie 
 *  Funktionszeiger auf init, read und write
 */
typedef struct MemDriver {
	uint16_t start;
	uint16_t size;
	void (*init)(void);
	MemValue (*read)(MemAddr);
	void (*write)(MemAddr, MemValue);
} MemDriver;

//! Initialisiert das Speichermedium.
void init(void);

//! Liest Wert an Adresse und gibt diesen zur�ck (gegeben der richtige Prozess fragt?).
MemValue read(MemAddr addr);

//! Schreibt den Wert an angegebene Adresse, gegeben der richtige Prozess fragt.
void write(MemAddr addr, MemValue value);

MemDriver intSRAM__;
MemDriver extSRAM__;

#endif