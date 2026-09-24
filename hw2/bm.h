// A general-purpose bitmap.
//
// bmcreate returns a pointer to the bit storage. The bit count is a
// size_t stored immediately in front of that pointer, so bmdelete can
// recover the allocation and munmap it. Bits are packed eight to a byte,
// bit 0 being the least-significant bit (see bitset/bitclr/bittst).
// An index outside 0 .. bits-1 writes an error to stderr and exits.
// bmprt prints every byte as two hex digits, last byte first.

#ifndef BM_H
#define BM_H

#include <stdio.h>

typedef void *BM;

extern BM   bmcreate(size_t bits);
extern void bmdelete(BM b);

extern void bmset(BM b, size_t i);
extern void bmclr(BM b, size_t i);
extern int  bmtst(BM b, size_t i);

extern void bmprt(BM b);

#endif
