// A buddy-address bitmap, for the Buddy System.
//
// Block size at exponent e is 2^e bytes. Bit e of (mem - base) is the bit
// that separates a block from its buddy:
//   baddrinv  flips it  (the buddy address)
//   baddrclr  clears it (the lower block of the pair)
//   baddrset  sets it
//   baddrtst  reads it
// The map has one bit per buddy pair. The pair's index is the lower
// block's offset divided by 2^(e+1). Pairs that only partly overlap the
// pool still get a bit. Set, clear, and test forward to the general bitmap.

#ifndef BBM_H
#define BBM_H

#include <stdio.h>

typedef void *BBM;

extern BBM  bbmcreate(size_t size, int e);
extern void bbmdelete(BBM b);

extern void bbmset(BBM b, void *base, void *mem, int e);
extern void bbmclr(BBM b, void *base, void *mem, int e);
extern  int bbmtst(BBM b, void *base, void *mem, int e);

extern void bbmprt(BBM b);

extern void *baddrset(void *base, void *mem, int e);
extern void *baddrclr(void *base, void *mem, int e);
extern void *baddrinv(void *base, void *mem, int e);
extern int   baddrtst(void *base, void *mem, int e);

#endif
