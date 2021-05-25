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

#endif