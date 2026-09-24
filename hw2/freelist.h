#ifndef FREELIST_H
#define FREELIST_H

// Per-order free lists for one buddy pool.
//
// Orders run from l to u inclusive: order e holds blocks of 2^e bytes.
// A free block stores the next-pointer in its first word. Allocated
// blocks hold no allocator data.
//
// Each order has a bitmap with one bit per buddy pair (see bbm). The bit
// is set iff either buddy, or both, is currently allocated to the caller
// at exactly that order. freelistsize walks upward from l and returns the
// smallest order for which that is true of mem. While coalescing,
// freelistfree treats the buddy as free when it sits on that order's list.

#include <stddef.h>
#include <stdio.h>

typedef void *FreeList;

extern FreeList freelistcreate(size_t size, int l, int u);
extern void     freelistdelete(FreeList f, int l, int u);

// Pop a block of order e, splitting a larger free block if needed.
// Returns 0 when no block of order e..u is free. l is the lowest order.
extern void *freelistalloc(FreeList f, void *base, int e, int l);

// Return a block of order e to the free lists and coalesce while the
// buddy is free and e < u. Also used to seed the lists: bitmaps start
// clear, so a block that was never handed out is simply inserted.
extern void  freelistfree(FreeList f, void *base, void *mem, int e, int l);

// Order of an outstanding allocation, or -1 if mem is not one.
extern int freelistsize(FreeList f, void *base, void *mem, int l, int u);
extern void freelistprint(FreeList f, int l, int u);

#endif
