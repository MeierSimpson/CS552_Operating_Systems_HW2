#ifndef BALLOC_H
#define BALLOC_H

// Buddy-system allocator. Memory for a pool is obtained with mmap
// (through mmalloc) only inside bcreate. l and u are exponents:
// the smallest block is 2^l bytes and the largest is 2^u bytes.
// A request larger than 2^u fails. bsize reports the block that was
// actually reserved, which may be larger than the request.

typedef void *Balloc;

extern Balloc bcreate(unsigned int size, int l, int u);
extern void   bdelete(Balloc ba);

extern void *balloc(Balloc ba, unsigned int size);
extern void  bfree(Balloc ba, void *mem);

extern unsigned int bsize(Balloc ba, void *mem);
extern void bprint(Balloc ba);

#endif
