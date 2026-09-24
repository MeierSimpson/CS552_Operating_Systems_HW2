#ifndef UTILS_H
#define UTILS_H

// Small helpers shared by the bitmap and the buddy allocator.
// mmalloc/mmfree are the only mmap/munmap wrappers in the program.
// Block sizes are powers of two: e2size(e) is 2^e bytes, and size2e
// rounds a byte count up to the exponent of that block.

#include <stddef.h>
#include <stdio.h>

static const int bitsperbyte=8;

extern void *mmalloc(size_t size);
extern void mmfree(void *p, size_t size);

extern size_t divup(size_t n, size_t d);
extern size_t bits2bytes(size_t bits);

extern size_t e2size(int e);
extern int size2e(size_t size);

// Bit 0 is the least-significant bit of the byte at p.
extern void bitset(void *p, int bit);
extern void bitclr(void *p, int bit);
extern void bitinv(void *p, int bit);
extern int  bittst(void *p, int bit);

#endif
