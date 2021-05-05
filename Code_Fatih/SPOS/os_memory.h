/*! \file
 *  \brief User-gerichtete Speicher-Schnittstelle.
 *
 *  "Obere Schicht" der Speicherverwaltung.
 *  Stellt os_malloc, os_free und Funktionen mit Infos über Speicher.
 *
 *  \author   Matthis
 *  \date     2020-11-26
 *  \version  1.0
 */

#ifndef _OS_MEMORY_H
#define _OS_MEMORY_H

#include <stddef.h>
#include "os_mem_drivers.h"
#include "os_memheap_drivers.h"
#include "os_scheduler.h"

/*!
 *  Versucht, size viele zusammenhängende bytes zu allokieren.
 *  Falls kein passender Platz gefunden, wird 0 zurückgegeben.
 */
MemAddr os_malloc(Heap* heap, uint16_t size);

/*!
 *  gibt Speicher frei indem es Nibbles der Allokationstabelle auf "frei" setzt.
 *  Speicher ist privat, kann nur vom Besitzer freigegeben werden.
 *  Versuchte Freigabe fremden Speichers führt zu Fehlermeldung.
 *  Adresse kann innerhalb des Bereichs verschoben sein,
 *  es soll trotzdem alles korrekt freigegeben werden
 */
void os_free(Heap* heap, MemAddr addr);

size_t os_getMapSize(Heap const* heap);
size_t os_getUseSize(Heap const* heap);
	
MemAddr os_getMapStart(Heap const* heap);
MemAddr os_getUseStart(Heap const* heap);

MemValue os_getMapEntry(Heap const *heap, MemAddr addr);


/*! 
 *  liefert Größe des Speicherbereichs in Byte zurück.
 *  Obacht: Adresse muss nicht auf's erste Byte des Bereichs zeigen.
 */
uint16_t os_getChunkSize(Heap const* heap, MemAddr addr);

void os_setAllocationStrategy(Heap* heap, AllocStrategy allocStrat);

AllocStrategy os_getAllocationStrategy(Heap const* heap);

//! gibt alles frei, was Prozess `pid`gehört.
void os_freeProcessMemory (Heap *heap, ProcessID pid);

//! versucht Speicherbereich zu vergrößern/verkleinern oder malloc neu und verschiebt Chunk
MemAddr os_realloc(Heap* heap, MemAddr addr, uint16_t size);

//! alloziert shared memory
MemAddr os_sh_malloc(Heap* heap, uint16_t size);

//! gibt shared memory frei
void os_sh_free(Heap* heap, MemAddr *ptr);

//! öffnet shm, falls grad keiner liest Eigentlich private.
MemAddr os_sh_readOpen(Heap const* heap, MemAddr const *ptr);

//! öffnet shm, falls grad keiner schreibt. Eigentlich private.
MemAddr os_sh_writeOpen(Heap const* heap, MemAddr const *ptr);

//! schliest shm. Eigentlich private.
void os_sh_close(Heap const* heap, MemAddr addr);

//! liest shm.
void os_sh_read(Heap const* heap, MemAddr const* ptr, uint16_t offset, MemValue* dataDest, uint16_t length);

//! schreibt shm.
void os_sh_write(Heap const* heap, MemAddr const* ptr, uint16_t offset, MemValue const* dataSrc, uint16_t length);

#endif