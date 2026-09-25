#include <sys/mman.h>

#include "utils.h"

// Map size bytes that the allocator can carve into blocks.
//
// Parameters:
//   size - the number of bytes to map
//
// The mapping is anonymous and private, so it is not backed by a file.
// A size of 0 returns NULL. On failure mmap's error value, (void *)-1, is
// returned for the caller to detect.
extern void *mmalloc(size_t size) {
  if (!size)
    return NULL;
  return mmap(0,size,PROT_READ|PROT_WRITE,MAP_PRIVATE|MAP_ANONYMOUS,-1,0);
}

// Release a mapping created by mmalloc.
//
// Parameters:
//   p - the address mmalloc returned
//   size - the same byte count that was mapped
//
// A null pointer, mmap's failure value, or a size of 0 is ignored.
// Otherwise the whole mapping is unmapped.
extern void mmfree(void *p, size_t size) {
  if (p && p!=(void *)-1 && size)
    munmap(p,size);
}

// Divide n by d, rounding a remainder up to a whole quotient.
//
// Parameters:
//   n - the dividend
//   d - the divisor
//
// An exact multiple returns n/d. Any leftover adds one.
extern size_t divup(size_t n, size_t d) {
  return n/d+(n%d!=0);
}

// Return how many bytes are needed to store bits.
//
// Parameters:
//   bits - the number of bits to store
//
// The result is bits/8 rounded up. A bit count of 0 returns 0.
extern size_t bits2bytes(size_t bits) {
  return divup(bits,(size_t)bitsperbyte);
}

// Return the block size 2^e.
//
// Parameters:
//   e - the order, used as the shift count
//
// A negative order returns 0. Otherwise the result is a power of two.
extern size_t e2size(int e) {
  return e<0 ? 0 : ((size_t)1<<e);
}

// Return the smallest order whose block can hold size bytes.
//
// Parameters:
//   size - the requested number of bytes
//
// The result is the smallest e with 2^e >= size. A size of 0 and a size
// of 1 both return 0.
extern int size2e(size_t size) {
  int e=0;
  // Subtracting one makes an exact power of two land on its own order.
  if (size)
    size--;
  while (size) {
    size>>=1;
    e++;
  }
  return e;
}

// Set one bit in a byte.
//
// Parameters:
//   p - the byte that holds the bit
//   bit - the bit position; 0 is the least-significant bit
//
// The selected bit becomes 1. The other bits in the byte are left alone.
extern void bitset(void *p, int bit) {
  unsigned char *c=p;
  *c|=(unsigned char)(1u<<bit);
}

// Clear one bit in a byte.
//
// Parameters:
//   p - the byte that holds the bit
//   bit - the bit position; 0 is the least-significant bit
//
// The selected bit becomes 0. The other bits in the byte are left alone.
extern void bitclr(void *p, int bit) {
  unsigned char *c=p;
  *c&=(unsigned char)~(1u<<bit);
}

// Flip one bit in a byte.
//
// Parameters:
//   p - the byte that holds the bit
//   bit - the bit position; 0 is the least-significant bit
//
// The selected bit changes from 0 to 1 or from 1 to 0. The other bits in
// the byte are left alone.
extern void bitinv(void *p, int bit) {
  unsigned char *c=p;
  *c^=(unsigned char)(1u<<bit);
}

// Read one bit from a byte.
//
// Parameters:
//   p - the byte that holds the bit
//   bit - the bit position; 0 is the least-significant bit
//
// Returns 1 when the bit is set and 0 when it is clear.
extern int bittst(void *p, int bit) {
  unsigned char *c=p;
  return (*c>>bit)&1;
}
