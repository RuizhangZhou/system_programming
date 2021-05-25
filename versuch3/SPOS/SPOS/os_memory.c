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

#include "memory.h"
#include "os_memory_strategies.h"
#include "util.h"
#include "os_core.h"

// --------- HELPER FUNCTIONS ALLOCATION --------



// --------- HELPER FUNCTIONS MEMORY --------



// --------- MEMORY FUNCTIONS --------

/*!
 *  Allocates a chunk of memory on the medium given by the driver and
 *  reserves it for the current process.
 *
 *  \param heap The heap on that should be allocated.
 *  \param size Size of the memory chunk to allocated in consecutive bytes.
 *  \return Address of the first byte of the allocated memory (in the use section of the heap).
 */
MemAddr os_malloc(Heap* heap, uint16_t size) {
    os_enterCriticalSection();

}

/*!
 *  Frees a chunk of allocated memory of the currently running process on the given heap.
 *
 *  \param heap Driver to be used.
 *  \param addr Size of the memory chunk to allocated in consecutive bytes.
 */
void os_free(Heap* heap, MemAddr addr) {
    os_enterCriticalSection();


}

