#include "bbm.h"
#include "bm.h"
#include "utils.h"

// Return how many buddy-pair bits a pool needs at one size of block.
//
// Parameters:
//   size - the pool size in bytes
//   e - the exponent which determines block size; a block at this exponent is 2^e bytes
//
// Pairs that only partly overlap the pool still count. Two blocks share
// one bit, so the result is half the number of blocks, rounded up.
static size_t mapsize(size_t size, int e) {
  size_t blocksize=e2size(e);
  // How many blocks of this exponent meet the pool, including a partial one.
  size_t blocks=divup(size,blocksize);
  size_t buddies=divup(blocks,2);
  return buddies;
}

// Return the bitmap index of the buddy pair that contains a block.
//
// Parameters:
//   base - the first byte of the pool
//   mem - an address inside one block of the pair
//   e - the exponent; bit e of the offset separates the two buddies
//
// Clearing bit e snaps mem to the lower block of the pair. Dividing that
// offset by two block sizes is the pair's index.
static size_t bitaddr(void *base, void *mem, int e) {
  size_t addr=baddrclr(base,mem,e)-base;
  size_t blocksize=e2size(e);
  return addr/blocksize/2;
}

// Allocate the buddy-pair bitmap for one exponent of a pool.
//
// Parameters:
//   size - the pool size in bytes
//   e - the exponent whose pairs this bitmap records
//
// The bitmap is a general bm bitmap with one bit per buddy pair. The
// bits start clear.
extern BBM bbmcreate(size_t size, int e) {
  return bmcreate(mapsize(size,e));
}

// Release a buddy-pair bitmap.
//
// Parameters:
//   b - the bitmap returned by bbmcreate
//
// The underlying bm mapping is unmapped.
extern void bbmdelete(BBM b) {
  bmdelete(b);
}

// Record that a buddy pair has a block allocated at this exponent.
//
// Parameters:
//   b - the bitmap returned by bbmcreate
//   base - the first byte of the pool
//   mem - an address inside one block of the pair
//   e - the exponent of that block
//
// Both buddies share this bit. Setting it says that either buddy, or
// both, is allocated at exactly exponent e.
extern void bbmset(BBM b, void *base, void *mem, int e) {
  bmset(b,bitaddr(base,mem,e));
}

// Clear the buddy-pair bit for a block.
//
// Parameters:
//   b - the bitmap returned by bbmcreate
//   base - the first byte of the pool
//   mem - an address inside one block of the pair
//   e - the exponent of that block
//
// Both buddies share this bit, so clearing it clears the record for the
// whole pair.
extern void bbmclr(BBM b, void *base, void *mem, int e) {
  bmclr(b,bitaddr(base,mem,e));
}

// Read the buddy-pair bit for a block.
//
// Parameters:
//   b - the bitmap returned by bbmcreate
//   base - the first byte of the pool
//   mem - an address inside one block of the pair
//   e - the exponent of that block
//
// Returns 1 when either buddy, or both, is recorded as allocated at
// exponent e. Returns 0 when the pair has no such record.
extern int bbmtst(BBM b, void *base, void *mem, int e) {
  return bmtst(b,bitaddr(base,mem,e));
}

// Print a buddy-pair bitmap to stdout.
//
// Parameters:
//   b - the bitmap returned by bbmcreate
//
// The bytes are printed by the general bitmap printer, last byte first.
extern void bbmprt(BBM b) { bmprt(b); }

// Return the address of the higher buddy in a pair.
//
// Parameters:
//   base - the first byte of the pool
//   mem - an address inside one block of the pair
//   e - the exponent; bit e of the offset is the buddy bit
//
// Bit e of (mem - base) is set. If mem was already the higher buddy, the
// same address is returned.
extern void *baddrset(void *base, void *mem, int e) {
  unsigned int mask=1<<e;
  return base+((mem-base)|mask);
}

// Return the address of the lower buddy in a pair.
//
// Parameters:
//   base - the first byte of the pool
//   mem - an address inside one block of the pair
//   e - the exponent; bit e of the offset is the buddy bit
//
// Bit e of (mem - base) is cleared. The lower buddy is the one whose
// offset is a multiple of 2^(e+1).
extern void *baddrclr(void *base, void *mem, int e) {
  unsigned int mask=~(1<<e);
  return base+((mem-base)&mask);
}

// Return the address of a block's buddy.
//
// Parameters:
//   base - the first byte of the pool
//   mem - an address inside the block
//   e - the exponent; bit e of the offset is the buddy bit
//
// Bit e of (mem - base) is flipped. The two buddies are 2^e bytes apart.
extern void *baddrinv(void *base, void *mem, int e) {
  unsigned int mask=1<<e;
  return base+((mem-base)^mask);
}

// Report whether a block is the higher buddy of its pair.
//
// Parameters:
//   base - the first byte of the pool
//   mem - an address inside the block
//   e - the exponent; bit e of the offset is the buddy bit
//
// Returns nonzero when bit e of (mem - base) is set, and 0 when mem is
// the lower buddy.
extern int baddrtst(void *base, void *mem, int e) {
  unsigned int mask=1<<e;
  return (mem-base)&mask;
}
