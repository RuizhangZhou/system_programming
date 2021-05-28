/*! \file
 *  \brief User centered Memory Interface.
 *
 *  "Obere Schicht" der Speicherverwaltung.
 *  Stellt os_malloc, os_free und Funktionen mit Infos ueber Speicher zur Verfuegung.
 *
 *  \author   Johannes
 *  \date     2021-05-25
 *  \version  1.0
 */

#ifndef _OS_MEMORY_H
#define _OS_MEMORY_H

#include "os_mem_drivers.h"
#include "os_memheap_drivers.h"
#include "os_scheduler.h"


//----------------------------------------------------------------------------
// --------- HELPER FUNCTIONS MEMORY --------
//----------------------------------------------------------------------------

/*!
 *  Frees a chunk of allocated memory on the medium given by the driver.
 *  This function checks if the call has been made by someone with the right to do it (i.e. the process that owns the memory or the OS).
 *  This function is made in order to avoid code duplication and is called by several functions that, in some way,
 *  free allocated memory such as os_freeProcessMemory/os_free...
 *
 *  \param heap The driver to be used.
 *  \param addr An address inside of the chunk (not necessarily the start).
 *  \param owner The expected owner of the chunk to be freed.
 */
void os_freeOwnerRestricted(Heap *heap, MemAddr addr, ProcessID owner);

/*!
 *  Fuction determines PID of Process that owns the given memory chunk.
 *
 *  \param heap The heap the chunk is on hand in.
 *  \param addr The address that points to some byte of the chunk.
 *  \return The PID of the Process associated with the chunk.
 */
ProcessID getOwnerOfChunk(Heap *heap, MemAddr addr);

/*!
 *  This function is used to determine where a chunk starts
 *  if a given address might not point to the start of the chunk but to some place inside of it.
 *  Called in: os_free (indirectly through os_freeOwnerRestricted), os_getChunkSize ...
 *
 *  \param heap The heap the chunk is on hand in.
 *  \param addr The address that points to some byte of the chunk.
 *  \return The address that points to the first byte of the chunk.
 */
MemAddr os_getFirstByteOfChunk(Heap const* heap, MemAddr addr);

/*!
 *  This function is used to get a heap map entry on a specific heap.
 *
 *  \param heap The heap from whos map the entry is supposed to be fetched.
 *  \param addr The address in use-section for which the corresponding map entry shall be fetched.
 *  \return he value that can be found on the heap map entry that corresponds to the given use space address.
 */
MemValue os_getMapEntry(Heap const* heap, MemAddr addr);

/*!
 *  This function is used to set a heap map entry on a specific heap.
 *
 *  \param heap The heap on whos map the entry is supposed to be set.
 *  \param addr The address in use section for which the corresponding map entry shall be set.
 *  \param value The value that is supposed to be set onto the map (valid range: 0x0 - 0xF)
 */
void setMapEntry(Heap const* heap, MemAddr addr, MemValue value);

/*!
 *  \param heap The heap that is read from.
 *  \param addr The address which the higher nibble is supposed to be read from.
 *  \return The value that can be found on the higher nibble of the given address.
 */
MemValue getHighNibble(Heap const* heap, MemAddr addr);

/*!
 *  \param heap The heap that is read from.
 *  \param addr The address which the lower nibble is supposed to be read from.
 *  \return The value that can be found on the lower nibble of the given address.
 */
MemValue getLowNibble(Heap const* heap, MemAddr addr);

/*!
 *  \param heap The heap that is written to.
 *  \param addr The address which the higher nibble is supposed to be changed.
 *  \param value The value that the higher nibble of the given addr is supposed to get.
 */
void setHighNibble(Heap const* heap, MemAddr addr, MemValue value);

/*!
 *  \param heap The heap that is written to.
 *  \param addr The address which the lower nibble is supposed to be changed.
 *  \param value The value that the lower nibble of the given addr is supposed to get.
 */
void setLowNibble(Heap const* heap, MemAddr addr, MemValue value);

//! Asserts that given address is in use-section; error otherwise
void assertAddrInUseArea(Heap const* heap, MemAddr addr);

//! Returns Address of map entry for given use section address
MemAddr getMapAddrForUseAddr(Heap const* heap, MemAddr addr);

/*!
 *	This function realises the garbage collection. When called, every allocated memory chunk of the given process is freed
 *	\param heap	The heap on which we look for allocated memory
 *	\param pid	The ProcessID of the process that owns all the memory to be freed
 */
void os_freeProcessMemory (Heap *heap, ProcessID pid);

//----------------------------------------------------------------------------
// --------- MEMORY FUNCTIONS --------
//----------------------------------------------------------------------------

/*!
 *  Allocates a chunk of memory on the medium given by the driver and
 *  reserves it for the current process.
 *
 *  \param heap Driver to be used.
 *  \param size Size of the memory chunk to allocated in consecutive bytes.
 *  \return Address of the first byte of the allocated memory (in the use section of the heap).
 */
MemAddr os_malloc(Heap* heap, uint16_t size);

/*!
 *  Frees a chunk of allocated memory of the currently running process on the given heap.
 *
 *  \param heap Driver to be used.
 *  \param addr Size of the memory chunk to allocated in consecutive bytes.
 */
void os_free(Heap* heap, MemAddr addr);

/*!
 *  The Heap-map-size of the heap (needed by the taskmanager).
 *
 *  \param heap Driver to be used.
 *  \return The size of the map of the heap.
 */
size_t os_getMapSize(Heap const* heap);

/*!
 *  The Heap-use-size of the heap (needed by the taskmanager).
 *
 *  \param heap Driver to be used.
 *  \return The size of the use section of the heap.
 */
size_t os_getUseSize(Heap const* heap);

/*!
 *  The map-start of the heap (needed by the taskmanager).
 *
 *  \param heap Driver to be used.
 *  \return The first byte of the map of the heap.
 */
MemAddr os_getMapStart(Heap const* heap);

/*!
 *  The Use-Section-Start of the heap (needed by the taskmanager).
 *
 *  \param heap Driver to be used.
 *  \return The first byte of the use-section of the heap.
 */
MemAddr os_getUseStart(Heap const* heap);

/*!
 *  Takes a use-pointer and computes the length of the chunk.
 *  This only works for occupied chunks. The size of free chunks is always 0.
 *
 *  \param heap Driver to be used.
 *  \param addr An address of the use-heap.
 *  \return The chunk's length.
 */
uint16_t os_getChunkSize(Heap const* heap, MemAddr addr);

/*!
 *  Simple getter function to fetch the allocation strategy of a given heap.
 *
 *  \param heap The heap of which the allocation strategy is returned.
 *  \return The allocation strategy of the given heap.
 */
AllocStrategy getAllocationStrategy(Heap const* heap);

/*!
 *  Simple setter function to change the allocation strategy of a given heap.
 *
 *  \param heap The heap of which the allocation shall be changed.
 *  \param AllocStrat The strategy is changed to allocStrat.
 */
void setAllocationStrategy(Heap* heap, AllocStrategy allocStrat);

#endif