/*! \file
 *  \brief Memory allocation strategies implemented for the OS.
 *
 *  Holds the memory allocation strategies used to realize memory functions.
 *
 *  \author   Johannes
 *  \date     2021-05-27
 *  \version  1.0
 */

#ifndef _OS_MEMORY_STRATEGIES_H
#define _OS_MEMORY_STRATEGIES_H

#include "os_memheap_drivers.h"

/*!
 *   \param heap The heap to allocate memory on. 
 *   \param size Size of memory chunk in bytes. 
 *   \return Anfang des gefundenen chunks oder 0, falls keiner gefunden
 */
MemAddr os_Memory_FirstFit (Heap *heap, size_t size);

/*!
 * \return Anfang des gefundenen chunks oder 0, falls keiner gefunden
 */
MemAddr os_Memory_NextFit (Heap *heap, size_t size);

/*!
 * \return Anfang des gefundenen chunks oder 0, falls keiner gefunden
 */
MemAddr os_Memory_BestFit (Heap *heap, size_t size);

/*!
 * \return Anfang des gefundenen chunks oder 0, falls keiner gefunden
 */
MemAddr os_Memory_WorstFit (Heap *heap, size_t size);

#endif
