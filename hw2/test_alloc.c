#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "balloc.h"
#include "bbm.h"
#include "bm.h"
#include "utils.h"

static int checks;

// Count one check, and stop the program when it fails.
//
// Parameters:
//   cond - nonzero when the check passed
//   msg - the text written to stderr if it failed
//
// A passing check only increments the count. A failing check prints the
// message and aborts through assert.
static void expect(int cond, const char *msg) {
  checks++;
  if (!cond) {
    fprintf(stderr,"FAIL: %s\n",msg);
    assert(cond);
  }
}

// Check the power-of-two helpers, the bit operations, and mmalloc.
//
// Each helper is compared with a known result. The mapping test writes a
// pattern and then releases the mapping.
static void test_utils(void) {
  expect(divup(0,8)==0,"divup 0");
  expect(divup(1,8)==1,"divup 1");
  expect(divup(8,8)==1,"divup 8");
  expect(divup(9,8)==2,"divup 9");
  expect(bits2bytes(0)==0,"bits2bytes 0");
  expect(bits2bytes(1)==1,"bits2bytes 1");
  expect(bits2bytes(8)==1,"bits2bytes 8");
  expect(bits2bytes(9)==2,"bits2bytes 9");
  expect(e2size(0)==1 && e2size(4)==16 && e2size(10)==1024,"e2size");
  expect(size2e(0)==0 && size2e(1)==0,"size2e small");
  expect(size2e(2)==1 && size2e(3)==2 && size2e(4)==2,"size2e 2..4");
  expect(size2e(5)==3 && size2e(16)==4 && size2e(17)==5,"size2e up");

  unsigned char c=0;
  bitset(&c,0);
  bitset(&c,3);
  expect(c==0x09,"bitset");
  expect(bittst(&c,0) && bittst(&c,3) && !bittst(&c,1),"bittst");
  bitinv(&c,0);
  bitclr(&c,3);
  expect(c==0,"bitclr/inv");

  void *p=mmalloc(128);
  expect(p && p!=(void *)-1,"mmalloc");
  memset(p,0xab,128);
  mmfree(p,128);
}

// Check the general bitmap and the buddy-address helpers.
//
// The bitmap checks set, test, and clear individual bits. The address
// checks use a fixed base so the buddy of a known offset is predictable.
// A real mapping then checks that the two buddies share one pair bit.
static void test_bitmaps(void) {
  BM b=bmcreate(10);
  expect(b!=0,"bmcreate");
  bmset(b,0);
  bmset(b,9);
  expect(bmtst(b,0) && !bmtst(b,1) && bmtst(b,9),"bm bits");
  bmclr(b,0);
  expect(!bmtst(b,0) && bmtst(b,9),"bmclr");
  bmdelete(b);

  char *base=(char *)(uintptr_t)0x1000;
  expect(baddrinv(base,base+32,5)==base,"buddy of +32 order 5");
  expect(baddrclr(base,base+32,5)==base,"lower of pair");
  expect(baddrset(base,base,5)==base+32,"set bit 5");
  expect(baddrtst(base,base+32,5)!=0 && baddrtst(base,base,5)==0,"baddrtst");

  char *pool=mmalloc(64);
  BBM m=bbmcreate(64,2);
  bbmset(m,pool,pool,2);
  expect(bbmtst(m,pool,pool+4,2)==1,"pair shares a bit");
  bbmclr(m,pool,pool+4,2);
  expect(bbmtst(m,pool,pool,2)==0,"bbmclr");
  bbmdelete(m);
  mmfree(pool,64);
}

// Allocate every block a pool will still give out.
//
// Parameters:
//   a - the pool to drain
//   ps - storage for the returned block addresses
//   cap - the maximum number of blocks ps can hold
//   n - set to the number of blocks actually taken
//
// Each request asks for one byte, so every block is the pool's minimum
// size. Returns the sum of the block sizes bsize reports.
static unsigned drain(Balloc a, void **ps, int cap, int *n) {
  unsigned sum=0;
  *n=0;
  for (;;) {
    void *p=balloc(a,1);
    if (!p)
      break;
    expect(*n<cap,"drain capacity");
    ps[(*n)++]=p;
    unsigned sz=bsize(a,p);
    expect(sz>=16 && (sz&(sz-1))==0,"drain block");
    sum+=sz;
  }
  return sum;
}

// Return every block in a list to its pool.
//
// Parameters:
//   a - the pool the blocks came from
//   ps - the block addresses
//   n - how many addresses are in ps
//
// The blocks are freed in the order they were stored.
static void free_all(Balloc a, void **ps, int n) {
  for (int i=0; i<n; i++)
    bfree(a,ps[i]);
}

// Check pool sizes that are not one maximum block.
//
// A pool that is not a power of two leaves a remainder smaller than the
// minimum block unused. A pool larger than 2^u is several maximum blocks,
// and those blocks are not merged past u. Bad bcreate arguments return 0.
static void test_pool_geometry(void) {
  expect(bcreate(0,4,8)==0,"size 0");
  expect(bcreate(128,5,4)==0,"l > u");
  expect(bcreate(128,0,4)==0,"block smaller than a pointer");
  expect(bcreate(128,4,31)==0,"exponent too large");

  void *ps[128];
  int n=0;
  Balloc a=bcreate(1000,4,8);
  expect(a!=0,"create 1000");
  unsigned sum=drain(a,ps,128,&n);
  expect(sum==992,"1000-byte pool yields 992 usable");
  free_all(a,ps,n);
  int n2=0;
  expect(drain(a,ps,128,&n2)==sum,"capacity stable after free");
  free_all(a,ps,n2);
  bdelete(a);

  a=bcreate(200,4,7);
  sum=drain(a,ps,128,&n);
  expect(sum==192,"200-byte pool yields 192");
  free_all(a,ps,n);
  bdelete(a);

  // Larger than 2^u: several maximum blocks, never merged past u.
  a=bcreate(256,4,6);
  expect(balloc(a,128)==0,"request above 2^u fails");
  void *b64[8];
  int n64=0;
  for (;;) {
    void *p64=balloc(a,64);
    if (!p64)
      break;
    expect(n64<8 && bsize(a,p64)==64,"64-byte block");
    b64[n64++]=p64;
  }
  expect(n64==4,"four 64-byte blocks");
  free_all(a,b64,n64);
  sum=drain(a,ps,128,&n);
  expect(sum==256,"256 pool usable down to 2^l");
  free_all(a,ps,n);
  expect(balloc(a,128)==0,"still capped at 2^u after coalesce");
  void *p=balloc(a,64);
  expect(p && bsize(a,p)==64,"64 still available");
  bfree(a,p);
  bdelete(a);
}

// Fill a block with one repeated byte.
//
// Parameters:
//   p - the first byte to fill
//   n - how many bytes to write
//   pat - the byte written into each position
//
// The whole range is overwritten, including the first word.
static void fill(void *p, unsigned n, unsigned char pat) {
  memset(p,pat,n);
}

// Report whether a block still holds one repeated byte.
//
// Parameters:
//   p - the first byte to inspect
//   n - how many bytes to inspect
//   pat - the byte that should be in each position
//
// Returns 1 when every byte matches. Returns 0 at the first mismatch.
static int intact(void *p, unsigned n, unsigned char pat) {
  unsigned char *c=p;
  for (unsigned i=0; i<n; i++)
    if (c[i]!=pat)
      return 0;
  return 1;
}

// Check that buddies merge, and that a live block's bytes stay intact.
//
// Two minimum blocks must coalesce only after both are freed. Freeing
// one must not write a header into the other. Freeing in an awkward
// order, and freeing a parent while a smaller piece is live, must still
// rebuild the whole pool once everything is free.
static void test_coalesce_and_contents(void) {
  Balloc a=bcreate(32,4,5);
  void *x=balloc(a,1);
  void *y=balloc(a,1);
  expect(x && y && bsize(a,x)==16 && bsize(a,y)==16,"split into buddies");
  expect(balloc(a,1)==0,"pool exhausted");
  fill(x,16,0xa1);
  fill(y,16,0xb2);
  // The first word is user data. Freeing must not have stored a header there.
  *(void **)x=(void *)(uintptr_t)0x1111;
  expect(intact((char *)x+sizeof(void *),16-sizeof(void *),0xa1),"tail of x");
  bfree(a,x);
  expect(intact(y,16,0xb2),"buddy y survives free of x");
  expect(balloc(a,32)==0,"cannot form 32 while y is live");
  bfree(a,y);
  void *z=balloc(a,32);
  expect(z && bsize(a,z)==32,"buddies coalesced");
  bfree(a,z);
  bfree(a,0);
  bfree(a,z); // A second free of the same block is ignored.
  expect(bsize(a,z)==0,"freed block has no size");
  bdelete(a);

  // Free in an awkward order, then the whole pool must come back.
  a=bcreate(64,4,6);
  void *b[4];
  for (int i=0; i<4; i++) {
    b[i]=balloc(a,16);
    expect(b[i] && bsize(a,b[i])==16,"16-byte block");
    fill(b[i],16,(unsigned char)(0x40+i));
  }
  int order[4]={0,2,1,3};
  for (int i=0; i<4; i++) {
    for (int j=i+1; j<4; j++)
      expect(intact(b[order[j]],16,(unsigned char)(0x40+order[j])),"untouched neighbor");
    bfree(a,b[order[i]]);
  }
  z=balloc(a,64);
  expect(z && bsize(a,z)==64,"full 64 coalesced");
  bfree(a,z);

  // A split block must not be reported as still allocated at the parent order.
  void *big=balloc(a,32);
  void *small=balloc(a,16);
  expect(big && small && bsize(a,big)==32 && bsize(a,small)==16,"mixed orders");
  fill(big,32,0x11);
  fill(small,16,0x22);
  bfree(a,big);
  expect(intact(small,16,0x22),"small lives after big is freed");
  expect(bsize(a,small)==16,"small keeps its order");
  bfree(a,small);
  z=balloc(a,64);
  expect(z && bsize(a,z)==64,"mixed orders coalesced");
  bfree(a,z);
  bdelete(a);
}

// Check rounding, a bad free, and two pools at once.
//
// A request below 2^l receives the minimum block, and a request between
// two powers of two rounds up. Freeing an address inside a live block
// must not change that block. Freeing from one pool must not change the
// bytes of the other.
static void test_rounding_and_two_pools(void) {
  Balloc a=bcreate(256,4,8);
  void *p=balloc(a,0);
  expect(p && bsize(a,p)==16,"zero request takes the minimum");
  bfree(a,p);
  p=balloc(a,17);
  expect(p && bsize(a,p)==32,"17 rounds up to 32");
  expect(bsize(a,(char *)p+16)==0,"interior pointer is not a block");
  bfree(a,(char *)p+16); // An interior address must not disturb p.
  fill(p,32,0x7e);
  expect(intact(p,32,0x7e),"interior free was a no-op");
  bfree(a,p);

  Balloc b=bcreate(128,4,7);
  void *q=balloc(b,64);
  p=balloc(a,64);
  expect(p && q && p!=q,"independent pools");
  fill(p,64,1);
  fill(q,64,2);
  bfree(a,p);
  expect(intact(q,64,2),"other pool intact");
  bfree(b,q);
  bdelete(a);
  bdelete(b);
  bdelete(0);
}

// Advance a simple integer generator.
//
// Parameters:
//   s - the previous value, replaced with the next one
//
// Returns the new value. The sequence is deterministic, so a failure can
// be repeated.
static unsigned lcg(unsigned *s) {
  *s=*s*1664525u+1013904223u;
  return *s;
}

// Allocate and free in a mixed order, then rebuild the whole pool.
//
// Live blocks are filled with a pattern and checked before they are
// freed. A new block must not overlap one that is still live. After the
// last block is freed, draining the pool must return the original size.
static void test_random(void) {
  enum { SLOTS=256, POOL=4096 };
  struct { void *p; unsigned n; unsigned char pat; } live[SLOTS];
  int nlive=0;
  Balloc a=bcreate(POOL,4,12);
  void *ps[SLOTS];
  int n=0;
  unsigned full=drain(a,ps,SLOTS,&n);
  expect(full==POOL,"4096 pool is fully usable");
  free_all(a,ps,n);

  unsigned seed=1;
  for (int step=0; step<4000; step++) {
    int do_free=nlive && (nlive==SLOTS || (lcg(&seed)&3)==0);
    if (do_free) {
      int i=lcg(&seed)%nlive;
      expect(intact(live[i].p,live[i].n,live[i].pat),"random block intact");
      bfree(a,live[i].p);
      live[i]=live[--nlive];
    } else {
      unsigned req=1u<<(lcg(&seed)%8); // Requests run from 1 byte through 128.
      void *p=balloc(a,req);
      if (!p)
        continue;
      unsigned sz=bsize(a,p);
      expect(sz>=16 && sz>=req && sz<=4096 && (sz&(sz-1))==0,"random size");
      for (int i=0; i<nlive; i++) {
        char *x=p, *y=live[i].p;
        int overlap=x<y+live[i].n && y<x+sz;
        expect(!overlap,"random overlap");
      }
      unsigned char pat=(unsigned char)lcg(&seed);
      fill(p,sz,pat);
      live[nlive].p=p;
      live[nlive].n=sz;
      live[nlive].pat=pat;
      nlive++;
    }
  }
  for (int i=0; i<nlive; i++) {
    expect(intact(live[i].p,live[i].n,live[i].pat),"final intact");
    bfree(a,live[i].p);
  }
  n=0;
  expect(drain(a,ps,SLOTS,&n)==full,"random traffic coalesces back");
  free_all(a,ps,n);
  bprint(a);
  bdelete(a);
}

// Create and destroy the same pool shape many times.
//
// Each pass allocates one block, checks that a 40-byte request became a
// 64-byte block, frees it, and deletes the pool. The minimum order here
// is 3, so the smallest block is 8 bytes.
static void test_recreate(void) {
  for (int i=0; i<50; i++) {
    Balloc a=bcreate(128,3,7);
    void *p=balloc(a,40);
    expect(p && bsize(a,p)==64,"l=3 block");
    bfree(a,p);
    bdelete(a);
  }
}

// Run every allocator check and print how many passed.
//
// The groups stop the program at the first failed check. The printed
// count is the number of checks that passed.
int main(void) {
  test_utils();
  test_bitmaps();
  test_pool_geometry();
  test_coalesce_and_contents();
  test_rounding_and_two_pools();
  test_random();
  test_recreate();
  printf("ok %d checks\n",checks);
  return 0;
}
