/*! \file
 *  \brief User centered Memory Interface.
 *
 *  "Obere Schicht" der Speicherverwaltung.
 *  Stellt os_malloc, os_free und Funktionen mit Infos ueber Speicher zur Verfuegung.
 *
 *  \author   Johannes
 *  \date     2021-05-26
 *  \version  1.0
 */

#include "memory.h"
#include "os_memory_strategies.h"
#include "util.h"
#include "os_core.h"
#include "os_process.h"


//----------------------------------------------------------------------------
// --------- HELPER FUNCTIONS MEMORY --------
//----------------------------------------------------------------------------


/*!
 *  Frees a chunk of allocated memory on the medium given by the driver.
 *  This function checks if the call has been made by someone with the right to do it (i.e. the process that owns the memory or the OS).
 *  This function is made in order to avoid code duplication and is called by several functions that, in some way,
 *  free allocated memory such as os_free.
 *
 *  \param heap The driver to be used.
 *  \param addr An address inside of the chunk (not necessarily the start).
 *  \param owner The expected owner of the chunk to be freed.
 */
void os_freeOwnerRestricted(Heap *heap, MemAddr addr, ProcessID owner) {
    os_enterCriticalSection();

    ProcessID chunkOwner = getOwnerOfChunk(heap, addr);
    if (chunkOwner == 0x0) {
        os_errorPStr("mem.c: ass err  wrong owner")
    }
    else if (chunkOwner != owner) {
        os_errorPStr("u shall not freewhat is not your")
    }

    addr = os_getFirstByteOfChunk(heap, addr);
    setMapEntry(heap, addr, 0x0);
    addr++;

    while (addr < (heap->useSectionStart + heap->useSectionSize) && os_getMapEntry(heap, addr) == 0xF) {
        setMapEntry(heap, addr, 0x0);
        addr++;
    }

    os_leaveCriticalSection();
}

/*!
 *  Fuction determines PID of Process that owns the given memory chunk.
 *
 *  \param heap The heap the chunk is on hand in.
 *  \param addr The address that points to some byte of the chunk.
 *  \return The PID of the Process associated with the chunk.
 */
ProcessID getOwnerOfChunk(Heap const* heap, MemAddr addr) {
    os_enterCriticalSection();

    while (os_getMapEntry(heap, addr) == 0xF) {
        addr--;
    }
    ProcessID owner = os_getMapEntry(heap, addr);

    os_leaveCriticalSection();
    return owner;
}

/*!
 *  This function is used to determine where a chunk starts
 *  if a given address might not point to the start of the chunk but to some place inside of it.
 *
 *  \param heap The heap the chunk is on hand in.
 *  \param addr The address that points to some byte of the chunk.
 *  \return The address that points to the first byte of the chunk.
 */
MemAddr os_getFirstByteOfChunk(Heap const* heap, MemAddr addr) {
    os_enterCriticalSection();
    while (os_getMapEntry(heap, addr) == 0xF) {
        addr--;
    }
    return addr;
}

/*!
 *  This function is used to get a heap map entry on a specific heap.
 *
 *  \param heap The heap from whos map the entry is supposed to be fetched.
 *  \param addr The address in use-section for which the corresponding map entry shall be fetched.
 *  \return The value that can be found on the heap map entry that corresponds to the given use space address.
 */
MemValue os_getMapEntry(Heap const* heap, MemAddr addr) {
    os_enterCriticalSection();
    assertAddressInUseSection(heap, addr);
    MemAddr mapAddr = getMapAddrForUseAddr(heap, addr);
    MemValue nibble;
    // check if high nibble, else low nibble
    if (((mapAddr - heap->mapSectionStart) % 2) == 0) {
        nibble = getHighNibble(heap, mapAddr);
    }
    else {
        nibble = getLowNibble(heap, mapAddr);
    }
    os_leaveCriticalSection();
    return nibble;
}

/*!
 *  This function is used to set a heap map entry on a specific heap.
 *
 *  \param heap The heap on whos map the entry is supposed to be set.
 *  \param addr The address in use section for which the corresponding map entry shall be set.
 *  \param value The value that is supposed to be set onto the map (valid range: 0x0 - 0xF)
 */
void setMapEntry(Heap const* heap, MemAddr addr, MemValue value) {
    assertAddressInUseSection(heap, addr);
    MemAddr mapAddr = getMapAddrForUseAddr(heap, addr);
    // check if high nibble, else low nibble
    if (((mapAddr - heap->mapSectionStart) % 2) == 0) {
        setHighNibble(heap, mapAddr, value);
    }
    else {
        setLowNibble(heap, mapAddr, value);
    }
}

/*!
 *  \param heap The heap that is read from.
 *  \param addr The address which the higher nibble is supposed to be read from.
 *  \return The value that can be found on the higher nibble of the given address.
 */
MemValue getHighNibble(Heap const* heap, MemAddr addr) {
    MemValue memByte = heap->driver->read(addr);
    memByte &= 0xF0;
    return memByte >> 4;
}

/*!
 *  \param heap The heap that is read from.
 *  \param addr The address which the lower nibble is supposed to be read from.
 *  \return The value that can be found on the lower nibble of the given address.
 */
MemValue getLowNibble(Heap const* heap, MemAddr addr) {
    MemValue memByte = heap->driver->read(addr);
    return memByte &= 0x0F;
}

/*!
 *  \param heap The heap that is written to.
 *  \param addr The address which the higher nibble is supposed to be changed.
 *  \param value The value that the higher nibble of the given addr is supposed to get.
 */
void setHighNibble(Heap const* heap, MemAddr addr, MemValue value) {
    // modulo 16 to assert that value between 0 and 15
    value = value % 0x10;
    // Shift bits to high nibble position
    value = value << 4;
    MemValue lowNibble = getLowNibble(heap, addr);
    // Write existing low nibble into value
    value |= lowNibble;
    // Write value to memory at addr
    heap->driver->write(addr, value);
}

/*!
 *  \param heap The heap that is written to.
 *  \param addr The address which the lower nibble is supposed to be changed.
 *  \param value The value that the lower nibble of the given addr is supposed to get.
 */
void setLowNibble(Heap const* heap, MemAddr addr, MemValue value) {
    // modulo 16 to assert that value between 0 and 15
    value = value % 0x10;
    // get high nibble and shift it to high position
    MemValue highNibble = getHighNibble(heap, addr);
    highNibble = highNibble << 4
    // Write existing high nibble into value
    value |= highNibble;
    // Write value to memory at addr
    heap->driver->write(addr, value);
}

//! Asserts that given address is in use-section; error otherwise
void assertAddrInUseArea(Heap const* heap, MemAddr addr) {
	if ((addr < heap->useAreaStart) || (addr >= (heap->useAreaStart + heap->useAreaSize))) {
		os_errorPStr("!!  expected  !!!!  use addr  !!");
	}
}

//! Returns Address of map entry for given use section address
MemAddr getMapAddrForUseAddr(Heap const* heap, MemAddr addr) {
    addr -= heap->useSectionStart;
    addr /= 2;
    return heap->mapSectionStart + addr;
}

/*!
 *	This function realises the garbage collection. When called, every allocated memory chunk of the given process is freed
 *	\param heap	The heap on which we look for allocated memory
 *	\param pid	The ProcessID of the process that owns all the memory to be freed
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


//----------------------------------------------------------------------------
// --------- MEMORY FUNCTIONS --------
//----------------------------------------------------------------------------

/*!
 *  Allocates a chunk of memory on the medium given by the driver and
 *  reserves it for the current process.
 *
 *  \param heap The heap on that should be allocated.
 *  \param size Size of the memory chunk to allocated in consecutive bytes.
 *  \return Address of the first byte of the allocated memory (in the use section of the heap).
 */
MemAddr os_malloc(Heap* heap, uint16_t size) {
    if (size == 0) {
        return 0;
    }

    // TODO: later own function maybe?
    os_enterCriticalSection();
    MemAddr addrFirstByte = 0;

    switch (os_getAllocationStrategy(heap))
    {
        case OS_MEM_FIRST:
            addrFirstByte = os_Memory_FirstFit(heap, size);
            break;
        case OS_MEM_NEXT:
            /* code */
            break;
        case OS_MEM_BEST:
            /* code */
            break;
        case OS_MEM_WORST:
            /* code */
            break;
    }

    // return 0 if no memory could be allocated
    if (addrFirstByte == 0) {
        os_leaveCriticalSection();
        return 0;
    }

    // set coresponding map entry to pid of current process
    setMapEntry(heap, addrFirstByte, os_getCurrentProc());
    size--;

    MemAddr addrFollowByte = addrFirstByte + 1
    // for chunks with size > 1 set map entries for following bytes to 0xF
    while (size-- > 0) {
        setMapEntry(heap, addrFollowByte++, 0xF)
    }

    os_leaveCriticalSection();
    return addrFirstByte;
    
}

/*!
 *  Frees a chunk of allocated memory of the currently running process on the given heap.
 *
 *  \param heap Driver to be used.
 *  \param addr Size of the memory chunk to be allocated in consecutive bytes.
 */
void os_free(Heap* heap, MemAddr addr) {
    os_enterCriticalSection();

    os_freeOwnerRestricted(heap, addr, os_getCurrentProc());
    
    os_leaveCriticalSection();
}

/*!
 *  The Heap-map-size of the heap (needed by the taskmanager).
 *
 *  \param heap Driver to be used.
 *  \return The size of the map of the heap.
 */
size_t os_getMapSize(Heap const* heap) {
    return heap->mapSectionSize;
}

/*!
 *  The Heap-use-size of the heap (needed by the taskmanager).
 *
 *  \param heap Driver to be used.
 *  \return The size of the use section of the heap.
 */
size_t os_getUseSize(Heap const* heap) {
    return heap->useSectionSize;
}

/*!
 *  The map-start of the heap (needed by the taskmanager).
 *
 *  \param heap Driver to be used.
 *  \return The first byte of the map of the heap.
 */
MemAddr os_getMapStart(Heap const* heap) {
    return heap->mapSectionStart;
}

/*!
 *  The Use-Section-Start of the heap (needed by the taskmanager).
 *
 *  \param heap Driver to be used.
 *  \return The first byte of the use-section of the heap.
 */
MemAddr os_getUseStart(Heap const* heap) {
    return heap->useSectionStart;
}

/*!
 *  Takes a use-pointer and computes the length of the chunk.
 *  This only works for occupied chunks. The size of free chunks is always 0.
 *
 *  \param heap Driver to be used.
 *  \param addr An address of the use-heap.
 *  \return The chunk's length.
 */
uint16_t os_getChunkSize(Heap const* heap, MemAddr addr) {
    os_enterCriticalSection();

    if (os_getMapEntry(heap, addr) == 0x0) {
        os_leaveCriticalSection();
        return 0;
    }

    addr = os_getFirstByteOfChunk(heap, addr);
    MemAddr addrLastByte = addr + 1;
    while (os_getMapEntry(heap, addrLastByte) == 0xF) {
        addrLastByte++;
    }

    // leave critical section and return higher address (+ 1) - lower address to get chunksize
    os_leaveCriticalSection();
    return (addrLastByte - addr);
}

/*!
 *  Simple getter function to fetch the allocation strategy of a given heap.
 *
 *  \param heap The heap of which the allocation strategy is returned.
 *  \return The allocation strategy of the given heap.
 */
AllocStrategy getAllocationStrategy(Heap const* heap) {
    return heap->actAllocStrategy;
}

/*!
 *  Simple setter function to change the allocation strategy of a given heap.
 *
 *  \param heap The heap of which the allocation shall be changed.
 *  \param AllocStrat The strategy is changed to allocStrat.
 */
void setAllocationStrategy(Heap* heap, AllocStrategy allocStrat) {
    os_enterCriticalSection();
    heap->actAllocStrategy = allocStrat;
    os_leaveCriticalSection();
}

