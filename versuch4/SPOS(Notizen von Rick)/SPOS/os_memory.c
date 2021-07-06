#include "os_memory.h"
#include "os_memheap_drivers.h"
#include "os_process.h"
#include "os_scheduler.h"
#include "os_core.h"
#include <stddef.h>
#include "os_memory_strategies.h"

void setLowNibble(Heap const *heap, MemAddr addr, MemValue value){
    uint8_t highNibble=heap->driver->read(addr) & 0b11110000;//highNibble=h1h2h3h4 0000
    //lowNibble=value=0000 l1l2l3l4
    heap->driver->write(addr,highNibble|value);
}

void setHighNibble(Heap const *heap, MemAddr addr, MemValue value){
    uint8_t lowNibble=heap->driver->read(addr) & 0b00001111;//lowNibble=0000 l1l2l3l4
    //value=0000 h1h2h3h4
    uint8_t highNibble=value<<4;//highNibble=h1h2h3h4 0000
    heap->driver->write(addr,lowNibble|highNibble);
}

/*!
 *	\param  heap	The heap that is read from
 *	\param	addr	The address which the lower nibble is supposed to be read from
 *	\returns The value that can be found on the lower nibble of the given address
 */
MemValue getLowNibble (Heap const *heap, MemAddr addr){
	return heap->driver->read(addr) & 0b00001111;
}

/*!
 *	\param heap	The heap that is read from
 *	\param addr	The address which the higher nibble is supposed to be read from
 *	returns The value that can be found on the higher nibble of the given address
 */
MemValue getHighNibble (Heap const *heap, MemAddr addr){
	return (heap->driver->read(addr) >> 4) ;
}

MemAddr os_malloc(Heap* heap, uint16_t size){
    os_enterCriticalSection();
	MemAddr res=0;
    switch (os_getAllocationStrategy(heap)) {
		case OS_MEM_FIRST :
			res=os_Memory_FirstFit(heap, size);
			break;	
		case OS_MEM_BEST :
			res=os_Memory_BestFit(heap, size);
			break;
		case OS_MEM_NEXT :
			res=os_Memory_NextFit(heap, size);
			break;
		case OS_MEM_WORST :
			res=os_Memory_WorstFit(heap, size);
			break;
	}
	if(res!=0){
		setMapEntry(heap,res,os_getCurrentProc());
		for(MemAddr offset=1;offset<size;offset++){
			setMapEntry(heap,res+offset,0xF);
		}
	}
    os_leaveCriticalSection();//in Doxyen doesn't have os_leaveCriticalSection()?
	return res;

}

//This will move one Chunk to a new location , To provide this the content of the old one is copied to the new location, 
//as well as all Map Entries are set properly since this is a helper function for reallocation, it only works if the new Chunk is bigger than the old one.
void moveChunk(Heap *heap,MemAddr oldChunk,size_t oldSize,MemAddr newChunk,size_t newSize){
    if (newSize < oldSize) os_error("newSize < oldSize");
    os_enterCriticalSection();
    //first of all set first map(ProcessID) of new Chunk(malloc)
    setMapEntry(heap,newChunk,getMapEntry(oldChunk));

    //set the map(0x0) and copy all the value of oldChunk(free)
	for (MemAddr i = 0; i < oldSize; i++) {
		heap->driver->write(newChunk + i, heap->driver->read(oldChunk + i));
		setMapEntry(heap, oldChunk+i,0x0);
	}

    //then set all rest map(0xF) of new Chunk(malloc)
    for(MemAddr i =1;i<newSize;i++){
        setMapEntry(heap,newChunk+i;0xF);
    }
	os_leaveCriticalSection();
}	

MemAddr os_realloc(Heap *heap,MemAddr addr,uint16_t size){
    // 0. prüfen: addr gehört den current proc
	if (getOwnerOfChunk(heap, addr) != os_getCurrentProc()) return 0;

    os_enterCriticalSection();
    MemAddr chunkStart=os_getFirstByteOfChunk(heap,addr);
    uint16_t chunkSize=os_getChunkSize(heap,addr);
    MemAddr right=chunkStart+chunkSize-1;//end of Chunk

    //1.verkleinern oder bleiben
    if(chunkSize>=size){
        while(right>=chunkStart+size){
            setMapEntry(heap,right,0);
            right--;
        }
        os_leaveCriticalSection();
        return chunkStart;
    }

    //can run to here mean chunkSize < size
    MemAddr useAreaEnd=os_getUseStart(heap)+os_getUseSize(heap)-1;
    right++;//right is now the first addr after the chunk
    //2.try to expand after the chunk
    while(right<=useAreaEnd && right<=chunkStart+size-1 && getMapEntry(heap,right)==0){
        right++;
    }
    
    if(right-chunkStart==size){//don't need to moveChunk,now right at the next addr of the new Chunk
        while(right>chunkStart+chunkSize){
            right--;
            setMapEntry(heap,right,0xF);
        }
        os_leaveCriticalSection();
        return chunkStart;
    }
    //3.try to expand before the chunk
    MemAddr left=chunkStart-1;//the first addr before the oldChunk
    while(left>=os_getUseStart(heap)&&left+size>=chunkStart+chunkSize&&getMapEntry(left)==0){
        left--;
    }
    if(left+size=chunkStart+chunkSize-1){//now left at the first addr before the newChunk
        moveChunk(heap,chunkStart,chunkSize,left+1,size);
        os_leaveCriticalSection();
        return left+1;
    }
    //4.find a new Chunk big enough for size
    MemAddr newChunk = os_malloc(heap, size);
    if (newChunk != 0) {
		moveChunk(heap, chunkStart, chunkSize, newChunk, size);
	}
    os_leaveCriticalSection();
    return newChunk;
}

size_t os_getMapSize(Heap const* heap){
    return heap->mapSize;
}
size_t os_getUseSize(Heap const* heap){
    return heap->useSize;
}
MemAddr os_getMapStart(Heap const* heap){
    return heap->mapStart;
}
MemAddr os_getUseStart(Heap const* heap){
    return heap->useStart;
}

MemValue getMapEntry(Heap const* heap, MemAddr addr){
    MemAddr mapAddr=heap->mapStart+(addr-heap->useStart)/2;
    if((addr-heap->useStart)%2==0){
        return getHighNibble(heap,mapAddr);
    }else{
        return getLowNibble(heap,mapAddr);
    }
}

void setMapEntry(Heap const* heap, MemAddr addr, MemValue value){
    MemAddr mapAddr=heap->mapStart+(addr-heap->useStart)/2;
    if((addr-heap->useStart)%2==0){
        setHighNibble(heap,mapAddr,value);
    }else{
        setLowNibble(heap,mapAddr,value);
    }
}

MemAddr os_getFirstByteOfChunk(Heap const* heap, MemAddr addr){
    if(getMapEntry(heap,addr)==0){
        return addr;
    }
    while(getMapEntry(heap,addr)==0xF){
        addr--;
    }
    return addr;
}

uint16_t os_getChunkSize(Heap const* heap, MemAddr addr){
    if(getMapEntry(heap,addr)==0){
        return 0;
    }
    if(getMapEntry(heap,addr)!=0xF){//the mapEntry of this addr is a ProcessID
        addr++;
    }
	//MemAddr useEnd=HEAP_TOP-1;
	MemAddr useEnd=os_getUseStart(heap)+os_getUseSize(heap)-1;
    while(getMapEntry(heap,addr)==0xF && addr <= useEnd){
		//addr maybe in the middle, go to the end of chunk, and cannot read after the useStart
        addr++;
    }
    return addr - os_getFirstByteOfChunk(heap,addr-1);//now addr is at the first Byte of next Chunk
}

ProcessID getOwnerOfChunk(Heap* heap, MemAddr addr){
	return=getMapEntry(heap,os_getFirstByteOfChunk(heap,addr));
}

void os_freeOwnerRestricted(Heap* heap, MemAddr addr, ProcessID owner){
    os_enterCriticalSection();
    if(owner!=getOwnerOfChunk(heap,addr)){
        os_error("wrong process:no right to free");
        os_leaveCriticalSection();
        return;
    }
    MemAddr firstByteOfChunk=os_getFirstByteOfChunk(heap,addr);
	uint16_t chunkSize=os_getChunkSize(heap,addr);
    for(uint16_t offset=0;offset<chunkSize;offset++){
        setMapEntry(heap,firstByteOfChunk+offset,0);
    }
    os_leaveCriticalSection();
}

void os_freeProcessMemory(Heap* heap, ProcessID pid){
    os_enterCriticalSection();
    MemAddr curAddr = heap->useStart;
    while(curAddr < heap->useStart + heap->useSize){
        if(getMapEntry(heap,curAddr)==pid){//the first Byte of Chunk with ProcessID=pid
            os_freeOwnerRestricted(heap,curAddr,pid);
            curAddr+=os_getChunkSize(heap,curAddr);
        }else{//(ProcessID!=pid) or mapEntry=0/0xF
            curAddr++;
        }
    }
    os_leaveCriticalSection();
}

void os_free(Heap* heap, MemAddr addr){
    os_enterCriticalSection();
    os_freeOwnerRestricted(heap,addr,os_getCurrentProc());
    os_leaveCriticalSection();
}

AllocStrategy os_getAllocationStrategy(Heap const* heap){
    return heap->strategy;
}

void os_setAllocationStrategy(Heap *heap, AllocStrategy allocStrat){
	heap->strategy = allocStrat;
}

