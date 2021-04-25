#include "os_memheap_drivers.h"
#include "defines.h"
#include "os_memory_strategies.h"
#include <avr/pgmspace.h>

/*! \file
 *	\brief Heap Driver structs and some initialisation functions. 
 *
 *	Some small functions needed by the Taskmanager are also placed here. 
 *	Note that some instantiations are hidden in the public version of this documentation.
 */

/*!
 * function initialises all entries of map with 0
 */
void os_initHeaps(){
	uint8_t num_heaps = os_getHeapListLength();
	Heap * cur = (os_lookupHeap(0));
	for (int i = 0; i < num_heaps; i++){
		// get heap from heap list 
		cur = (os_lookupHeap(i));
		for (MemAddr j = cur->start_address; j < (cur->start_address + cur->map_size); j++ )
		{
			write(j, 0);
		}
	}
}

/*!
 * Returns the size of the heap list i.e. the numbers of heaps connected
 */
uint8_t os_getHeapListLength(){
	return sizeof(*intHeap) / sizeof(Heap);
}


/*!
 * return the heap structure at a given index in the heap list
*/
Heap* os_lookupHeap(uint8_t index){
	return (intHeap + index);
}