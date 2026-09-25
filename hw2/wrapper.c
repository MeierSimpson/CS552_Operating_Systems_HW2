#include <string.h>

#include "balloc.h"

// The one buddy pool behind malloc, free, and realloc.
//
// It is created on the first malloc and then reused. Nothing in this
// wrapper destroys it.
static Balloc bp=0;

#include <stdio.h>

// Allocate size bytes from the buddy pool.
//
// Parameters:
//   size - the number of bytes requested
//
// The first call creates a pool of 4096 bytes, with blocks from 16 bytes
// through 4096. Later calls use that same pool. Returns the block, or NULL
// when the request is larger than the pool can give.
extern void *malloc(size_t size) {
  // Create the pool once. A later call keeps the allocator already stored.
  bp=bp ? bp : bcreate(4096,4,12);
  return balloc(bp,size);
}

// Return a block to the buddy pool.
//
// Parameters:
//   ptr - the address malloc or realloc returned, or NULL
//
// A null pointer is ignored. Any other address is freed back into the
// pool created by malloc.
extern void free(void *ptr) {
  bfree(bp,ptr);
}

// Return the smaller of two lengths.
//
// Parameters:
//   x - the first length
//   y - the second length
//
// The caller uses this so a copy stays inside both the old block and the
// new one.
static size_t min(size_t x, size_t y) {
  return x<y ? x : y;
}

// Allocate a block of size bytes and, when ptr is set, copy the old one.
//
// Parameters:
//   ptr - the old block, or 0 to allocate without copying
//   size - the number of bytes the new block must be able to hold
//
// The copy length is the smaller of size and the old block's real size.
// The old block is then freed. When ptr is 0, the new block is returned
// without a copy.
extern void *realloc(void *ptr, size_t size) {
  void *block=malloc(size);
  // realloc(0, size) allocates and does not free anything.
  if (!ptr)
    return block;
  memcpy(block,ptr,min(size,bsize(bp,ptr)));
  free(ptr);
  return block;
}
