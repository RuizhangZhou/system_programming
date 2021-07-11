#include "os_memory.h"
#include "lcd.h"
#include "os_core.h"
#include "os_input.h"
#include "os_memheap_drivers.h"
#include "os_memory_strategies.h"
#include "os_process.h"

MemAddr mapAdressOfUseAdress(Heap const *h, MemAddr a) {
  a = a - h->startOfUseArea;
  a = a / 2;
  return h->startOfMapArea + a;
}

void visited(Heap *h, MemAddr a) {
  if (a != 0) {
    h->visitedAreas[os_getCurrentProc() - 1] |=
        (1 << (16 * (a - h->startOfUseArea) / h->sizeOfUseArea));
  }
}

bool checkIfHighNibble(Heap const *h, MemAddr a) {
  a -= h->startOfUseArea;
  if (a % 2 == 0) {
    return true;
  } else {
    return false;
  };
}

MemValue getLowNibble(Heap const *h, MemAddr a) {
  MemValue initial = h->driver->read(a);
  return initial &= 0x0F;
}

MemValue getHighNibble(Heap const *h, MemAddr a) {
  MemValue initial = h->driver->read(a);
  initial &= 0xF0;
  return initial >> 4;
}

void setLowNibble(Heap const *h, MemAddr a, MemValue val) {
  val = val % 0x10;
  MemValue high = getHighNibble(h, a);
  high = high << 4;
  val |= high;
  h->driver->write(a, val);
}

void setHighNibble(Heap const *heap, MemAddr addr, MemValue value) {
  value = value % 0x10;
  value = value << 4;
  MemValue low = getLowNibble(heap, addr);
  value |= low;
  heap->driver->write(addr, value);
}

void makeSureAdressIsInUseArea(Heap const *h, MemAddr a) {
  if (a < h->startOfUseArea || a >= (h->startOfUseArea + h->sizeOfUseArea)) {
    os_error("error: we need a use adress here!");
  }
}

MemValue os_getMapEntry(Heap const *h, MemAddr a) {
  os_enterCriticalSection();
  makeSureAdressIsInUseArea(h, a);
  MemAddr m = mapAdressOfUseAdress(h, a);
  uint8_t nib;
  if (checkIfHighNibble(h, a)) {
    nib = getHighNibble(h, m);
  } else {
    nib = getLowNibble(h, m);
  }
  os_leaveCriticalSection();
  return nib;
}

void setMapEntry(Heap const *h, MemAddr a, MemValue val) {
  makeSureAdressIsInUseArea(h, a);
  MemAddr mapAddr = mapAdressOfUseAdress(h, a);
  if (checkIfHighNibble(h, a)) {
    setHighNibble(h, mapAddr, val);
  } else {
    setLowNibble(h, mapAddr, val);
  }
}

MemAddr getFirstByteOfChunk(Heap const *h, MemAddr a) {
  while (os_getMapEntry(h, a) == 0xF) {
    a = a - 1;
  }
  return a;
}

ProcessID getOwnerOfChunk(Heap const *h, MemAddr a) {
  os_enterCriticalSection();
  while (os_getMapEntry(h, a) == 0xF) {
    a = a - 1;
  }
  uint8_t owner = os_getMapEntry(h, a);
  os_leaveCriticalSection();
  return owner;
}

uint16_t os_getChunkSize(Heap const *h, MemAddr a) {
  os_enterCriticalSection();

  if (0 == os_getMapEntry(h, a)) {
    os_leaveCriticalSection();
    return 0;
  }

  a = getFirstByteOfChunk(h, a);
  MemAddr r = a;
  r += 1;
  while (os_getMapEntry(h, r) == 0xF) {
    r += 1;
  }

  os_leaveCriticalSection();
  return r - a;
}

void os_freeOwnerRestricted(Heap *h, MemAddr a, ProcessID owner) {
  os_enterCriticalSection();

  uint8_t actualOwner = getOwnerOfChunk(h, a);
  if (actualOwner == 0xF || owner != actualOwner) {
    os_error("error: wrong owner");
  }

  a = getFirstByteOfChunk(h, a);
  setMapEntry(h, a, 0);
  a++;
  while (a < (h->startOfUseArea + h->sizeOfUseArea) &&
         os_getMapEntry(h, a) == 0xF) {
    setMapEntry(h, a, 0);
    a++;
  }

  os_leaveCriticalSection();
}

AllocStrategy os_getAllocationStrategy(Heap const *h) {
  return h->allocationStrategy;
}

void os_setAllocationStrategy(Heap *h, AllocStrategy strat) {
  os_enterCriticalSection();
  h->allocationStrategy = strat;
  os_leaveCriticalSection();
}

MemAddr getMemoryChunk(Heap *h, uint16_t size, uint8_t owner) {
  os_enterCriticalSection();
  MemAddr cc = 0;

  switch (os_getAllocationStrategy(h)) {
  case OS_MEM_NEXT:
    cc = os_Memory_NextFit(h, size);
    break;
  case OS_MEM_BEST:
    cc = os_Memory_BestFit(h, size);
    break;
  case OS_MEM_FIRST:
    cc = os_Memory_FirstFit(h, size);
    break;
  case OS_MEM_WORST:
    cc = os_Memory_WorstFit(h, size);
    break;
  }

  if (cc == 0) {
    os_leaveCriticalSection();
    return 0;
  }

  visited(h, cc);

  setMapEntry(h, cc, owner);
  size -= 1;

  MemAddr c = cc + 1;
  while (size-- > 0) {
    setMapEntry(h, c++, 0xF);
  }

  os_leaveCriticalSection();
  return cc;
}

MemAddr os_malloc(Heap *h, uint16_t size) {

  if (size == 0) {
    return 0;
  }

  return getMemoryChunk(h, size, os_getCurrentProc());
}

MemAddr os_sh_malloc(Heap *h, uint16_t size) {

  if (size == 0) {
    return 0;
  }

  return getMemoryChunk(h, size, 0x8);
}

void os_sh_free(Heap *h, MemAddr *p) {

  os_enterCriticalSection();
  if (getOwnerOfChunk(h, *p) < 8) {
    lcd_clear();
    lcd_writeProgString(PSTR("err: sh_free of non-shm"));
    os_waitForInput();
    os_leaveCriticalSection();
    return;
  }

  while (getOwnerOfChunk(h, *p) != 0x8) {
    os_yield();
  }

  os_freeOwnerRestricted(h, *p, 8);

  os_leaveCriticalSection();
}

void os_free(Heap *h, MemAddr addr) {
  os_enterCriticalSection();

  if (getOwnerOfChunk(h, addr) > 7) {
    lcd_clear();
    lcd_writeProgString(PSTR("err: free of shm"));
    os_waitForInput();
    os_leaveCriticalSection();
    return;
  }

  os_freeOwnerRestricted(h, addr, os_getCurrentProc());
  os_leaveCriticalSection();
}

size_t os_getMapSize(Heap const *h) { return h->sizeOfMapArea; }

size_t os_getUseSize(Heap const *h) { return h->sizeOfUseArea; }

MemAddr os_getMapStart(Heap const *h) { return h->startOfMapArea; }

MemAddr os_getUseStart(Heap const *h) { return h->startOfUseArea; }

void os_freeProcessMemory(Heap *h, ProcessID pid) {
  os_enterCriticalSection();
  for (int j = 0; j < 16; j++) {
    if (h->visitedAreas[pid - 1] & (1 << j)) {
      MemAddr lower = (h->sizeOfUseArea * j) / 16;
      MemAddr upper = j + 1 != 16 ? (h->sizeOfUseArea * (j + 1)) / 16 + 1
                                  : h->sizeOfUseArea;
      for (MemAddr i = h->startOfUseArea + lower;
           i < (h->startOfUseArea + upper); i++) {
        // wenn eine Zahl auftritt vergleichen mit PID und löschen den Chunk
        if (os_getMapEntry(h, i) == pid) {
          os_freeOwnerRestricted(h, i, pid);
        }
      }
    }
  }
  h->visitedAreas[pid - 1] = 0;
  os_leaveCriticalSection();
}

void moveChunk(Heap *h, MemAddr oldChunk, size_t oldSize, MemAddr newChunk,
               size_t newSize) {

  if (newSize < oldSize) {
    os_error("error: newSize < oldSize");
  }

  os_enterCriticalSection();

  setMapEntry(h, newChunk, getOwnerOfChunk(h, oldChunk));
  h->driver->write(newChunk, h->driver->read(oldChunk));
  setMapEntry(h, oldChunk, 0x0);

  for (MemAddr i = 1; i < oldSize; i++) {
    setMapEntry(h, newChunk + i, 0xF);
    h->driver->write(newChunk + i, h->driver->read(oldChunk + i));
    setMapEntry(h, oldChunk + i, 0x0);
  }

  os_leaveCriticalSection();
}

MemAddr os_realloc(Heap *h, MemAddr a, uint16_t size) {

  if (getOwnerOfChunk(h, a) != os_getCurrentProc()) {
    return 0;
  }

  os_enterCriticalSection();
  MemAddr cStart = getFirstByteOfChunk(h, a);
  uint16_t cSize = os_getChunkSize(h, cStart);
  MemAddr low = cStart;
  MemAddr top = cStart + cSize;

  if (cSize > size) {
    while ((top - cStart) > size) {
      top--;
      setMapEntry(h, top, 0x0);
    }
    os_leaveCriticalSection();
    return cStart;
  }

  while (top < os_getUseStart(h) + os_getUseSize(h) &&
         os_getMapEntry(h, top) == 0x0 && top < cStart + size) {
    top++;
  }

  if ((top - cStart) >= size) {
    while (top > cStart + cSize) {
      top--;
      setMapEntry(h, top, 0xF);
    }
    os_leaveCriticalSection();
    return cStart;
  }

  while (low > os_getUseStart(h) && os_getMapEntry(h, low - 1) == 0x0) {
    low--;
  }

  if ((top - low) >= size) {
    moveChunk(h, cStart, cSize, low, size);
    for (MemAddr i = cSize; i < size; i++) {
      setMapEntry(h, low + i, 0xF);
    }
    os_leaveCriticalSection();
    visited(h, low);
    return low;
  }

  MemAddr newChunk = os_malloc(h, size);
  if (newChunk != 0) {
    moveChunk(h, cStart, cSize, newChunk, size);
  }
  os_leaveCriticalSection();
  visited(h, newChunk);
  return newChunk;
}

MemAddr os_sh_readOpen(Heap const *h, MemAddr const *p) {
  os_enterCriticalSection();

  if (getOwnerOfChunk(h, *p) < 8) {
    os_error("err: os_sh_readOpen not on shm");
    os_leaveCriticalSection();
    return 0;
  }

  while (getOwnerOfChunk(h, *p) > 0xC) {
    os_yield();
  }

  MemAddr a = getFirstByteOfChunk(h, *p);
  setMapEntry(h, a, os_getMapEntry(h, a) + 1);

  os_leaveCriticalSection();
  return a;
}

MemAddr os_sh_writeOpen(Heap const *h, MemAddr const *p) {
  os_enterCriticalSection();

  if (getOwnerOfChunk(h, *p) < 8) {
    os_error("os_sh_writeOpen not on shm");
    os_leaveCriticalSection();
    return 0;
  }

  while (getOwnerOfChunk(h, *p) != 0x8) {
    os_yield();
  }

  MemAddr a = getFirstByteOfChunk(h, *p);
  setMapEntry(h, a, 0xE);

  a = *p;
  os_leaveCriticalSection();
  return a;
}

void os_sh_close(Heap const *h, MemAddr a) {
  os_enterCriticalSection();

  if (getOwnerOfChunk(h, a) < 8) {
    os_error("os_sh_close not on shm");
    os_leaveCriticalSection();
    return;
  }

  a = getFirstByteOfChunk(h, a);

  uint8_t x = os_getMapEntry(h, a);

  if (x < 9) {
    os_error("closing on already closed");
  }

  int setTo = x == 0xE ? 0x8 : x - 1;
  setMapEntry(h, a, setTo);

  os_leaveCriticalSection();
}

bool boundsCheck(Heap const *h, MemAddr const *p, uint16_t end) {
  os_enterCriticalSection();
  MemAddr a = getFirstByteOfChunk(h, *p);
  bool in = getFirstByteOfChunk(h, a) == getFirstByteOfChunk(h, a + end);
  os_leaveCriticalSection();
  return in;
}

void os_sh_read(Heap const *heap, MemAddr const *ptr, uint16_t offset,
                MemValue *dataDest, uint16_t length) {

  os_enterCriticalSection();

  if (!boundsCheck(heap, ptr, offset + length - 1)) {
    os_error("err: ptr not in shm bounds");
    os_leaveCriticalSection();
    return;
  }

  MemAddr a = os_sh_readOpen(heap, ptr);
  os_leaveCriticalSection();

  for (MemAddr i = 0; i < length; ++i) {
    os_enterCriticalSection();
    *(dataDest + i) =
        heap->driver->read(getFirstByteOfChunk(heap, a) + offset + i);
    os_leaveCriticalSection();
  }

  os_enterCriticalSection();
  os_sh_close(heap, a);
  os_leaveCriticalSection();
}

void os_sh_write(Heap const *h, MemAddr const *ptr, uint16_t offset,
                 MemValue const *dataSrc, uint16_t length) {

  os_enterCriticalSection();
  if (!boundsCheck(h, ptr, offset + length - 1)) {
    os_error("err: ptr not in shm bounds");
    os_leaveCriticalSection();
    return;
  }

  MemAddr addr = os_sh_writeOpen(h, ptr);
  os_leaveCriticalSection();

  for (MemAddr i = 0; i < length; ++i) {
    os_enterCriticalSection();
    h->driver->write(getFirstByteOfChunk(h, addr) + offset + i, *(dataSrc + i));
    os_leaveCriticalSection();
  }

  os_sh_close(h, addr);
}