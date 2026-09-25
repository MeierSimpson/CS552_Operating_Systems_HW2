#include <stdio.h>

#include "balloc.h"
#include "freelist.h"
#include "utils.h"

// One buddy pool. The free lists carve this memory up; the struct itself
// remembers the bounds and the mapping.
typedef struct {
  int l, u;
  unsigned int size;
  void *pool;
  FreeList fl;
} *Rep;

// Return the pool struct stored behind an allocator handle.
//
// Parameters:
//   ba - the allocator returned by bcreate
//
// The handle is the struct pointer. No check is made here.
static Rep rep(Balloc ba) { return ba; }

// Decide whether bcreate's arguments can form a pool.
//
// Parameters:
//   size - the number of bytes the caller wants mapped
//   l - the exponent of the smallest block
//   u - the exponent of the largest block
//
// Returns 1 when size is nonzero, l and u are in order, u fits in the
// buddy-address shift, and 2^l is large enough to hold a free-list
// pointer. Otherwise returns 0.
static int args_ok(unsigned int size, int l, int u) {
  if (!size || l<0 || u<l || u>=31)
    return 0;
  // A free block stores its next pointer in the first word.
  if (e2size(l)<sizeof(void *))
    return 0;
  return 1;
}

// Place the initial free blocks over a newly mapped pool.
//
// Parameters:
//   r - the pool whose free lists are still empty
//
// The largest legal blocks are used first. A size that is not a power of
// two, or that is larger than 2^u, becomes several blocks. Bytes left
// over that are shorter than 2^l stay unused.
static void seed(Rep r) {
  size_t off=0, size=r->size;
  while (off<size) {
    int e=r->u, placed=0;
    // Try from the largest order down until a block fits at this offset.
    while (e>=r->l) {
      size_t n=e2size(e);
      // The block must fit, and its address must be aligned to its size.
      if (n<=size-off && (off&(n-1))==0) {
        freelistfree(r->fl,r->pool,(char *)r->pool+off,e,r->l);
        off+=n;
        placed=1;
        break;
      }
      e--;
    }
    // A gap smaller than the minimum block cannot start another one.
    if (!placed)
      break;
  }
}

// Create a buddy pool.
//
// Parameters:
//   size - the number of bytes to map
//   l - the exponent of the smallest block, 2^l
//   u - the exponent of the largest block, 2^u
//
// The pool is mapped once, here, and then divided into free blocks.
// Returns 0 when the arguments are unusable or a mapping fails. On
// failure, anything already mapped is released.
extern Balloc bcreate(unsigned int size, int l, int u) {
  if (!args_ok(size,l,u))
    return 0;
  void *pool=mmalloc(size);
  if (!pool || pool==(void *)-1)
    return 0;
  FreeList fl=freelistcreate(size,l,u);
  if (!fl) {
    mmfree(pool,size);
    return 0;
  }
  Rep r=mmalloc(sizeof(*r));
  if (!r || r==(void *)-1) {
    freelistdelete(fl,l,u);
    mmfree(pool,size);
    return 0;
  }
  r->l=l;
  r->u=u;
  r->size=size;
  r->pool=pool;
  r->fl=fl;
  // The free lists start empty; seed inserts the original blocks.
  seed(r);
  return r;
}

// Release a buddy pool and the memory it mapped.
//
// Parameters:
//   ba - the allocator returned by bcreate
//
// A null handle is ignored. The free-list bitmaps, the pool, and the
// pool struct are all unmapped.
extern void bdelete(Balloc ba) {
  if (!ba)
    return;
  Rep r=rep(ba);
  freelistdelete(r->fl,r->l,r->u);
  mmfree(r->pool,r->size);
  mmfree(r,sizeof(*r));
}

// Reserve a block from a pool.
//
// Parameters:
//   ba - the allocator returned by bcreate
//   size - the number of bytes the caller asked for
//
// The request is rounded up to a power of two between 2^l and 2^u. A
// request of 0 still receives the minimum block. A request larger than
// 2^u returns NULL, as does a pool with no room left.
extern void *balloc(Balloc ba, unsigned int size) {
  if (!ba)
    return NULL;
  Rep r=rep(ba);
  int e=size2e(size);
  // Anything below the minimum order is served by a minimum block.
  if (!size || e<r->l)
    e=r->l;
  if (e>r->u)
    return NULL;
  return freelistalloc(r->fl,r->pool,e,r->l);
}

// Return a block to a pool.
//
// Parameters:
//   ba - the allocator returned by bcreate
//   mem - the address balloc returned
//
// The block's order is recovered from the free-list bitmaps, because the
// allocated block itself stores no size. A null pointer, or an address
// that is not an outstanding block, is ignored. The block is then merged
// with its buddy while that buddy is free.
extern void bfree(Balloc ba, void *mem) {
  if (!ba || !mem)
    return;
  Rep r=rep(ba);
  int e=freelistsize(r->fl,r->pool,mem,r->l,r->u);
  if (e<0)
    return;
  freelistfree(r->fl,r->pool,mem,e,r->l);
}

// Report the size of a reserved block.
//
// Parameters:
//   ba - the allocator returned by bcreate
//   mem - the address balloc returned
//
// The result is the block size, which may be larger than the request. It
// is not the request size. An address that is not an outstanding block
// returns 0.
extern unsigned int bsize(Balloc ba, void *mem) {
  if (!ba || !mem)
    return 0;
  Rep r=rep(ba);
  int e=freelistsize(r->fl,r->pool,mem,r->l,r->u);
  if (e<0)
    return 0;
  return (unsigned int)e2size(e);
}

// Print a pool and its free lists to stdout.
//
// Parameters:
//   ba - the allocator returned by bcreate
//
// A null handle prints nothing. Otherwise the pool address, its size, its
// order range, and each order's free list and bitmap are written.
extern void bprint(Balloc ba) {
  if (!ba)
    return;
  Rep r=rep(ba);
  printf("allocator %p pool %p size %u orders %d..%d\n",
         (void *)r,r->pool,r->size,r->l,r->u);
  freelistprint(r->fl,r->l,r->u);
}
