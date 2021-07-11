#include "os_memory_strategies.h"
#include "os_memory.h"
#include "os_process.h"
#include "os_memheap_drivers.h"
#include "os_core.h"
#include "lcd.h"
#include "os_input.h"

MemAddr getMapAddrForUseAddr(Heap const *heap, MemAddr addr) {
	// relative position im use-bereich
	addr -= heap->useAreaStart;
	// halbieren
	addr /= 2;
	// relative position im map-bereich
	return addr + heap->mapAreaStart;
}

/*!
 *  \brief Setzt Bit in `procVisit`-Bitmap entsprechend der relativen Position von `addr` in use area.
 *
 *  Die `procVisit`-Bitmap gibt für jeden Prozess des Heaps an, in welchen Bereichen er Speicher belegt.
 *  Eine Bitmap wie z.b. `0b1000000000000111` bedeutet, dass ein Prozess chunks allokiert hat, die in den
 *  unteren 3/16 und im obersten Sechzehntel der use area _beginnen_.
 *
 *  Die Bitmap betrachtet nur den _Start_ der chunks. Ein Prozess, der den gesamten Heap mit einem chunk belegt,
 *  hätte eine bitmap mit Wert `1`. Also nur das kleinste Bit wäre gesetzt.
 *
 *  Die Bitmap _steht auf dem Kopf_, da _niedrige_ Bits _rechts_, aber niedrige Speicherbereiche _links_ sind.
 *
 *  Bits dürfen nur gesetzt werden. Einzige Ausnahme ist, wenn der _gesamte_ Speicher eines Prozesses freigegeben wird.
 */
void setProcVisitBit(Heap *heap, MemAddr addr) {
	if (addr ==  0) {
		return;
	}
	heap->procVisitArea[os_getCurrentProc() - 1] |= (1 << (16 * (addr - heap->useAreaStart) / heap->useAreaSize));
}


bool isMapHighNibbleForUseAddr(Heap const *heap, MemAddr addr) {
	// relative position im use-bereich
	addr -= heap->useAreaStart;
	return addr % 2 == 0;
}

//! Reads the value of the lower nibble of the given address.
MemValue getLowNibble (Heap const *heap, MemAddr addr) {
	MemValue initial = heap->driver->read(addr);
	// hohen nibble wegnullen
	return initial &= 0x0F;
}

//! Reads the value of the higher nibble of the given address.
MemValue getHighNibble (Heap const *heap, MemAddr addr) {
	MemValue initial = heap->driver->read(addr);
	// niedrigen nibble wegnullen
	initial &= 0xF0;
	return initial >> 4;
}

//! Writes a value from 0x0 to 0xF to the lower nibble of the given address.
void setLowNibble (Heap const *heap, MemAddr addr, MemValue value) {
	// stelle sicher, dass 0 <= value <= 15
	value = value % 0x10;
	// der HighNibble soll nicht überschrieben werden, daher auslesen..
	MemValue high = getHighNibble(heap, addr);
	// ..wieder nach oben shiften..
	high = high << 4;
	// ..und dazupacken.
	value |= high;
	heap->driver->write(addr, value);
}

//! Writes a value from 0x0 to 0xF to the higher nibble of the given address.
void setHighNibble (Heap const *heap, MemAddr addr, MemValue value) {
	// stelle sicher, dass 0 <= value <= 15
	value = value % 0x10;
	// schiebe value auf den hohen nibble, der gesetzt werden soll
	value = value << 4;
	MemValue low = getLowNibble(heap, addr);
	// value schreiben
	value |= low;
	heap->driver->write(addr, value);
}

void assertAddrInUseArea(Heap const *heap, MemAddr addr) {
	if (addr < heap->useAreaStart) {
		os_error("!!  expected  !!!!  use addr  !!");
	}
	if (addr >= (heap->useAreaStart + heap->useAreaSize)) {
		os_error("!!  expected  !!!!  use addr  !!");
	}
}

/*!
 *  \brief Liefert zugehöriges Allok-Tabellen-Nibble für Use-Adresse.
 *
 *  \param addr	Die Adresse, dessen Verwaltungs-Nibble gelesen werden soll.
 */
MemValue os_getMapEntry (Heap const *heap, MemAddr addr) {
	os_enterCriticalSection();
	assertAddrInUseArea(heap, addr);
	MemAddr mapAddr = getMapAddrForUseAddr(heap, addr);
	uint8_t nibble;
	if (isMapHighNibbleForUseAddr(heap, addr)) {
		nibble = getHighNibble(heap, mapAddr);
	} else {
		nibble = getLowNibble(heap, mapAddr);
	}
	os_leaveCriticalSection();
	return nibble;
}

/*!
 *  \brief Setzt zugehöriges Allok-Tabellen-Nibble für Use-Adresse.
 *
 *  \param addr	The address in use space for which the corresponding map entry shall be set
 *  \param value	Was in den map-nibble rein soll (valid range: 0x0 - 0xF)
 */
void setMapEntry (Heap const *heap, MemAddr addr, MemValue value) {
	assertAddrInUseArea(heap, addr);
	MemAddr mapAddr = getMapAddrForUseAddr(heap, addr);
	if (isMapHighNibbleForUseAddr(heap, addr)) {
		setHighNibble(heap, mapAddr, value);
	} else {
		setLowNibble(heap, mapAddr, value);
	}
}


//! Get the address of the first byte of chunk.
MemAddr getFirstByteOfChunk(Heap const *heap, MemAddr addr) {
	while (os_getMapEntry(heap, addr) == 0xF) {
		addr--;
	}
	return addr;
}

ProcessID getOwnerOfChunk(Heap const *heap, MemAddr addr) {
	os_enterCriticalSection();
	while (os_getMapEntry(heap, addr) == 0xF) {
		addr--;
	}
	uint8_t owner = os_getMapEntry(heap, addr);
	os_leaveCriticalSection();
	return owner;
}

//! Get the size of a chunk on a given address.
uint16_t os_getChunkSize (Heap const *heap, MemAddr addr) {
	os_enterCriticalSection();

	if (os_getMapEntry(heap, addr) == 0) {
		os_leaveCriticalSection();
		return 0;
	}

	// linkes ende des Chunks
	addr = getFirstByteOfChunk(heap, addr);
	MemAddr right = addr;
	// einen weiterschieben, um beim potentiellen F zu sein
	right++;
	while (os_getMapEntry(heap, right) == 0xF) {
		right++;
	}

	os_leaveCriticalSection();
	return right - addr;
}

void os_freeOwnerRestricted(Heap *heap, MemAddr addr, ProcessID owner) {
	os_enterCriticalSection();
	
	uint8_t actualOwner = getOwnerOfChunk(heap, addr);
	if (actualOwner == 0xF) {
		os_error("mem.c: ass err  wrong owner");
	} else if (owner != actualOwner) {
		os_error("u shall not freewhat is not thee");
	}
	
	addr = getFirstByteOfChunk(heap, addr);
	setMapEntry(heap, addr, 0);
	addr++;
	//           addr liegt im use bereich                 &&     in der allocTable steht F
	while (addr < (heap->useAreaStart + heap->useAreaSize) && os_getMapEntry(heap, addr) == 0xF) {
		setMapEntry(heap, addr, 0);
		addr++;
	}
	
	os_leaveCriticalSection();
}

//! Returns the current memory management strategy.
AllocStrategy os_getAllocationStrategy (Heap const *heap) {
	return heap->allocStrategy;
}

//! Changes the memory management strategy.
void os_setAllocationStrategy (Heap *heap, AllocStrategy allocStrat) {
	os_enterCriticalSection();
	heap->allocStrategy = allocStrat;
	os_leaveCriticalSection();
}

/*!
 *  tries to get a mem chunk. MUST BE CALLED INSIDE CRITICAL SECTION!
 */
MemAddr getMemoryChunk(Heap *heap, uint16_t size, uint8_t owner) {
	os_enterCriticalSection();
	MemAddr chunk = 0;

	switch (os_getAllocationStrategy(heap)) {
		case OS_MEM_FIRST:
			chunk = os_Memory_FirstFit(heap, size);
			break;
		case OS_MEM_BEST:
			chunk =  os_Memory_BestFit(heap, size);
			break;
		case OS_MEM_NEXT:
			chunk = os_Memory_NextFit(heap, size);
			break;
		case OS_MEM_WORST:
			chunk = os_Memory_WorstFit(heap, size);
			break;
	}

	if (chunk == 0) {
		os_leaveCriticalSection();
		return 0;
	}

	setProcVisitBit(heap, chunk);

	setMapEntry(heap, chunk, owner);
	size--;
	
	MemAddr c = chunk + 1;
	while (size-- > 0) {
		setMapEntry(heap, c++, 0xF);
	}

	os_leaveCriticalSection();
	return chunk;
}

//! Function used to allocate private memory.
MemAddr os_malloc(Heap *heap, uint16_t size) {

	if (size == 0) {
		return 0;
	}

	return getMemoryChunk(heap, size, os_getCurrentProc());
}

/*!
 *  alloziert shared memory.
 *
 * map entry Protokoll:
 *  - 8:	sharemd memory mit 0 lesenden, 0 schreibenden
 *  - 9-D:	shared memory mit x-8 lesenden (z.B. map entry C => 4 lesen grade dieses bit.
 *  - E:	ein Prozess liest
 *
 */
MemAddr os_sh_malloc(Heap *heap, uint16_t size) {

	if (size == 0) {
		return 0;
	}

	return getMemoryChunk(heap, size, 0x8);
}

void os_sh_free(Heap *heap, MemAddr *ptr) {

	os_enterCriticalSection();
	if (getOwnerOfChunk(heap, *ptr) < 8) {
		lcd_clear();
		lcd_writeProgString(PSTR("ERROR:os_sh_free  on non-shm"));
		os_waitForInput();
		os_leaveCriticalSection();
		return;
	}

	while (getOwnerOfChunk(heap, *ptr) != 0x8) {
		os_yield();
	}

	os_freeOwnerRestricted(heap, *ptr, 8);

	os_leaveCriticalSection();
}

//! Function used by processes to free their own allocated memory.
void os_free (Heap *heap, MemAddr addr) {
	os_enterCriticalSection();
	
	if (getOwnerOfChunk(heap, addr) > 7) {
		lcd_clear();
		lcd_writeProgString(PSTR("ERROR! os_free  on shared mem"));
		os_waitForInput();
		os_leaveCriticalSection();
		return;
	}
	
	os_freeOwnerRestricted(heap, addr, os_getCurrentProc());
	os_leaveCriticalSection();
}

//! Get the size of the heap-map.
size_t os_getMapSize (Heap const *heap) {
	return heap->mapAreaSize;
}

//! Get the size of the usable heap.
size_t os_getUseSize (Heap const *heap) {
	return heap->useAreaSize;
}

//! Get the start of the heap-map.
MemAddr os_getMapStart (Heap const *heap) {
	return heap->mapAreaStart;
}

//! Get the start of the usable heap.
MemAddr os_getUseStart (Heap const *heap) {
	return heap->useAreaStart;
}


/*!
 *  \brief Function that realizes the garbage collection.
 *
 *  Um eher effizient den Speicher eines Prozesses freizugeben,wird beim (re)allozieren in einer bitmap vermerkt,
 *  in welchen Speicherabschnitten chunks beginnen (-> siehe `setProcVisitBit`).
 *  Die Funktion durchsucht dann die vermerkten Stellen nach chunks und free't diese gegebenenfalls.
 *
 */
void os_freeProcessMemory (Heap *heap, ProcessID pid) {
	os_enterCriticalSection();
	// for-Schleife läuft bits der procVisit bitmap durch
	for (int j = 0; j < 16; j++) {
		// falls das aktuelle bit gesetzt ist...
		if (heap->procVisitArea[pid - 1] & (1 << j)) {
			MemAddr offset = (heap->useAreaSize * j) / 16;
			// + 1 als "ceiling" statt "floor", weil sonst durch Rundungsfehler die letzte Adresse des Bereichs übersehen werden könnte.
			// und der Sonderfall j == 15, da wir dann "nur" useAreaSize und net + 1 haben wollen, da sonst os_getMapEntry "überlaufen" könnte
			MemAddr upfset = j + 1 != 16 ? (heap->useAreaSize * (j+1)) / 16 + 1 : heap->useAreaSize;
			// dann den bereich durchsuchen um zu free'en
			for (MemAddr i = heap->useAreaStart + offset; i < (heap->useAreaStart + upfset); i++) {
				// wenn eine Zahl auftritt vergleichen mit PID und löschen den Chunk
				if (os_getMapEntry(heap, i) == pid) {
					os_freeOwnerRestricted(heap, i, pid);
				}
			}
		}
	}
	heap->procVisitArea[pid - 1] = 0x0;
	os_leaveCriticalSection();
}

void moveChunk (Heap *heap, MemAddr oldChunk, size_t oldSize, MemAddr newChunk, size_t newSize){
	
	if (newSize < oldSize) {
		os_error("newSize < oldSize");
	}
	
	os_enterCriticalSection();
	
	setMapEntry(heap, newChunk, getOwnerOfChunk(heap, oldChunk));
	heap->driver->write(newChunk, heap->driver->read(oldChunk));
	setMapEntry(heap, oldChunk, 0x0);

	for (MemAddr i = 1; i < oldSize; i++) {
		setMapEntry(heap, newChunk + i, 0xF);
		heap->driver->write(newChunk + i, heap->driver->read(oldChunk + i));
		setMapEntry(heap, oldChunk + i, 0x0);
	}
	
	os_leaveCriticalSection();
}

MemAddr os_realloc(Heap* heap, MemAddr addr, uint16_t size){
	
	// 0. prüfen: addr gehört den current proc
	if (getOwnerOfChunk(heap, addr) != os_getCurrentProc()) { 
		return 0;
	}

	os_enterCriticalSection();
	MemAddr chunkStart = getFirstByteOfChunk(heap, addr);
	uint16_t chunkSize = os_getChunkSize(heap, chunkStart);
	// linkeste noch freie Adresse ausgehend von chunkStart
	MemAddr left = chunkStart;
	// rechteste noch freie Adresse +1 ausgehend von chunkStart
	MemAddr right = chunkStart + chunkSize;

	// 1. Bereich verkleinern
	if (chunkSize > size) {
		while ((right - chunkStart) > size) {
			right--;
			setMapEntry(heap, right, 0x0);
		}
		os_leaveCriticalSection();
		return chunkStart;
	}
	
	// 2. passt hinter dem prozess chunk und sofort alloc
	// -> Bereich nach rechts erweitern
	// Solange wir noch im Heap sind, der Speicher frei ist und wir noch nicht genug Speicherbereich haben: right weiter nach rechts verschieben
	while (right < os_getUseStart(heap) + os_getUseSize(heap) && os_getMapEntry(heap, right) == 0x0 && right < chunkStart + size) {
		right++;
	}

	// Wenn size groß genug ist, Speicher erweitern
	if ((right - chunkStart) >= size) {
		while (right  > chunkStart + chunkSize) {
			right--;
			setMapEntry(heap, right, 0xF);
		}
		os_leaveCriticalSection();
		return chunkStart;
	}

	// 3. passt vor dem prozess chunk
	// -> Bereich nach links wie möglich verschieben
	
	// Solange wir noch im Heap sind und der Speicher frei ist: left weiter nach links verschieben
	while (left > os_getUseStart(heap) && os_getMapEntry(heap, left - 1) == 0x0) {
		left--;
	}
	
	if ((right - left) >= size) {
		moveChunk(heap, chunkStart, chunkSize, left, size);
		for (MemAddr i = chunkSize; i < size; i++) {
			setMapEntry(heap, left + i, 0xF);
		}
		os_leaveCriticalSection();
		setProcVisitBit(heap, left);
		return left;
	}
	
	// Bereich woanders allokieren falls möglich, sonst return 0
	MemAddr newChunk = os_malloc(heap, size);
	if (newChunk != 0) {
		moveChunk(heap, chunkStart, chunkSize, newChunk, size);
	}
	os_leaveCriticalSection();
	setProcVisitBit(heap, newChunk);
	return newChunk;
}

MemAddr os_sh_readOpen(Heap const* heap, MemAddr const *ptr) {
	os_enterCriticalSection();

	if (getOwnerOfChunk(heap, *ptr) < 8) {
		lcd_clear();
		lcd_writeProgString(PSTR(" os_sh_readOpen on non-shm: err"));
		os_waitForInput();
		os_leaveCriticalSection();
		return 0;
	}

	// 0xD => maximal viele lesen, 0xE => einer schreibt
	while (getOwnerOfChunk(heap, *ptr) > 0xC) {
		os_yield();
	}

	MemAddr addr = getFirstByteOfChunk(heap, *ptr);
	setMapEntry(heap, addr, os_getMapEntry(heap, addr) + 1);

	os_leaveCriticalSection();
	return addr;
}

MemAddr os_sh_writeOpen(Heap const* heap, MemAddr const *ptr) {
	os_enterCriticalSection();

	if (getOwnerOfChunk(heap, *ptr) < 8) {
		lcd_clear();
		lcd_writeProgString(PSTR("os_sh_writeOpen on non-shm: err"));
		os_waitForInput();
		os_leaveCriticalSection();
		return 0;
	}

	while (getOwnerOfChunk(heap, *ptr) != 0x8) {
		os_yield();
	}

	MemAddr addr = getFirstByteOfChunk(heap, *ptr);
	setMapEntry(heap, addr, 0xE);

	addr = *ptr;
	os_leaveCriticalSection();
	return addr;
}

void os_sh_close(Heap const* heap, MemAddr addr) {
	os_enterCriticalSection();

	if (getOwnerOfChunk(heap, addr) < 8) {
		lcd_clear();
		lcd_writeProgString(PSTR(" os_sh_close on non-shm: err"));
		os_waitForInput();
		os_leaveCriticalSection();
		return;
	}

	addr = getFirstByteOfChunk(heap, addr);

	uint8_t x = os_getMapEntry(heap, addr);

	if (x < 9) {
		os_error("closing on already closed");
	}

	int setTo = x == 0xE ? 0x8 : x - 1;
	setMapEntry(heap, addr, setTo);

	os_leaveCriticalSection();
}

bool inBounds(Heap const* heap, MemAddr const* ptr, uint16_t end) {
	os_enterCriticalSection();
	MemAddr addr = getFirstByteOfChunk(heap, *ptr);
	bool in = getFirstByteOfChunk(heap, addr) == getFirstByteOfChunk(heap, addr + end);
	os_leaveCriticalSection();
	return in;
}

void os_sh_read(Heap const* heap, MemAddr const* ptr, uint16_t offset, MemValue* dataDest, uint16_t length) {

	os_enterCriticalSection();

	if (!inBounds(heap, ptr, offset + length - 1)) {
		lcd_clear();
		lcd_writeProgString(PSTR("ERROR! ptr not  in shm bounds"));
		os_waitForInput();
		os_leaveCriticalSection();
		return;
	}

	MemAddr addr = os_sh_readOpen(heap, ptr);
	os_leaveCriticalSection();

	for (MemAddr i = 0; i < length; ++i) {
		os_enterCriticalSection();
		*(dataDest + i) = heap->driver->read(getFirstByteOfChunk(heap, addr) + offset + i);
		os_leaveCriticalSection();
	}

	os_enterCriticalSection();
	os_sh_close(heap, addr);
	os_leaveCriticalSection();
}

void os_sh_write(Heap const* heap, MemAddr const* ptr, uint16_t offset, MemValue const* dataSrc, uint16_t length) {

	os_enterCriticalSection();
	if (!inBounds(heap, ptr, offset + length - 1)) {
		lcd_clear();
		lcd_writeProgString(PSTR("ERROR! ptr not  in shm bounds"));
		os_waitForInput();
		os_leaveCriticalSection();
		return;
	}

	MemAddr addr = os_sh_writeOpen(heap, ptr);
	os_leaveCriticalSection();

	for (MemAddr i = 0; i < length; ++i) {
		os_enterCriticalSection();
		heap->driver->write(getFirstByteOfChunk(heap, addr) + offset + i, *(dataSrc + i));
		os_leaveCriticalSection();
	}

	os_sh_close(heap, addr);
}