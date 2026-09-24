#include <stdio.h>

#include "balloc.h"
#include "freelist.h"
#include "utils.h"

// One buddy pool. The FreeList holds every per-order list and bitmap;
// this struct remembers the pool those lists carve up.
typedef struct {
  int l, u;
  unsigned int size;
  void *pool;
  FreeList fl;
} *Rep;

static Rep rep(Balloc ba) { return ba; }

// 2^l has to hold a free-list pointer. Bit e of an offset is used as an
// int shift by the provided buddy-address helpers, so u stays below 31.
static int args_ok(unsigned int size, int l, int u) {
  if (!size || l<0 || u<l || u>=31)
    return 0;
  if (e2size(l)<sizeof(void *))
    return 0;
  return 1;
}

// Cover the pool with the largest legal blocks. A size that is not a
// power of two, or that is larger than 2^u, becomes several blocks.
// Leftover bytes shorter than 2^l stay unused.
static void seed(Rep r) {
  size_t off=0, size=r->size;
  while (off<size) {
    int e=r->u, placed=0;
    while (e>=r->l) {
      size_t n=e2size(e);
      if (n<=size-off && (off&(n-1))==0) {
        freelistfree(r->fl,r->pool,(char *)r->pool+off,e,r->l);
        off+=n;
        placed=1;
        break;
      }
      e--;
    }
    if (!placed)
      break;
  }
}

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
  seed(r);
  return r;
}

extern void bdelete(Balloc ba) {
  if (!ba)
    return;
  Rep r=rep(ba);
  freelistdelete(r->fl,r->l,r->u);
  mmfree(r->pool,r->size);
  mmfree(r,sizeof(*r));
}

extern void *balloc(Balloc ba, unsigned int size) {
  if (!ba)
    return 0;
  Rep r=rep(ba);
  int e=size2e(size);
  if (!size || e<r->l)
    e=r->l;
  if (e>r->u)
    return 0;
  return freelistalloc(r->fl,r->pool,e,r->l);
}

extern void bfree(Balloc ba, void *mem) {
  if (!ba || !mem)
    return;
  Rep r=rep(ba);
  int e=freelistsize(r->fl,r->pool,mem,r->l,r->u);
  if (e<0)
    return;
  freelistfree(r->fl,r->pool,mem,e,r->l);
}

extern unsigned int bsize(Balloc ba, void *mem) {
  if (!ba || !mem)
    return 0;
  Rep r=rep(ba);
  int e=freelistsize(r->fl,r->pool,mem,r->l,r->u);
  if (e<0)
    return 0;
  return (unsigned int)e2size(e);
}

extern void bprint(Balloc ba) {
  if (!ba)
    return;
  Rep r=rep(ba);
  printf("allocator %p pool %p size %u orders %d..%d\n",
         (void *)r,r->pool,r->size,r->l,r->u);
  freelistprint(r->fl,r->l,r->u);
}
