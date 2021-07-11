#include "os_memory_strategies.h"
#include "os_memory.h"

MemAddr os_Memory_FirstFit(Heap *h, size_t size) {
  size_t zero = 0;
  MemAddr front = h->startOfUseArea;
  MemAddr back = h->startOfUseArea;

  while (back < (h->startOfUseArea + h->sizeOfUseArea)) {
    if (os_getMapEntry(h, front) != 0) {
      front += 1;
      back = front;
      // das continue ist, damit fast sicher innerhalb der use-area ist
      continue;
    }
    if (os_getMapEntry(h, back) != 0) {
      zero = 0;
      front = back;
      continue;
    }
    if (os_getMapEntry(h, back) == 0 && zero < size) {
      back += 1;
      zero += 1;
    }
    if (zero == size) {
      return front;
    }
  }
  return 0;
}

MemAddr os_Memory_NextFit(Heap *h, size_t size) {
  size_t zero = 0;
  MemAddr front = h->nextOne;
  MemAddr back = h->nextOne;

  while (back < (h->startOfUseArea + h->sizeOfUseArea)) {
    if (os_getMapEntry(h, front) != 0) {
      front += 1;
      back = front;
      // das continue ist, damit fast sicher innerhalb der use-area ist
      continue;
    }
    if (os_getMapEntry(h, back) != 0) {
      zero = 0;
      front = back;
      continue;
    }
    if (os_getMapEntry(h, back) == 0 && zero < size) {
      back += 1;
      zero += 1;
    }
    if (zero == size) {
      h->nextOne = front + size;
      return front;
    }
  }
  return os_Memory_FirstFit(h, size);
}

MemAddr os_Memory_WorstFit(Heap *h, size_t size) {
  size_t curr = 0;
  size_t largest = 0;
  MemAddr front = h->startOfUseArea;
  MemAddr back = h->startOfUseArea;
  MemAddr worst = 0;

  while (back < (h->startOfUseArea + h->sizeOfUseArea)) {
    if (os_getMapEntry(h, front) != 0) {
      front++;
      back = front;
      continue;
    }
    if (os_getMapEntry(h, back) != 0x0) {
      curr = 0;
      front = back;
      continue;
    }
    while ((back < (h->startOfUseArea + h->sizeOfUseArea)) &&
           os_getMapEntry(h, back) == 0x0) {
      back += 1;
      curr += 1;
    }
    if (curr >= size && curr >= largest) {
      largest = curr;
      worst = front;
    }
  }
  return worst;
}

MemAddr os_Memory_BestFit(Heap *heap, size_t size) {
  size_t curr = 0;
  size_t smallest = heap->sizeOfUseArea;
  MemAddr front = heap->startOfUseArea;
  MemAddr back = heap->startOfUseArea;
  MemAddr best = 0;

  while (back < (heap->startOfUseArea + heap->sizeOfUseArea)) {
    if (os_getMapEntry(heap, front) != 0) {
      front += 1;
      back = front;
      continue;
    }
    if (os_getMapEntry(heap, back) != 0) {
      curr = 0;
      front = back;
      continue;
    }
    while ((back < (heap->startOfUseArea + heap->sizeOfUseArea)) &&
           os_getMapEntry(heap, back) == 0) {
      back += 1;
      curr += 1;
    }
    if (curr >= size && curr <= smallest) {
      smallest = curr;
      best = front;
    }
  }
  return best;
}