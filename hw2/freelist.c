#include <stdint.h>

#include "bbm.h"
#include "freelist.h"
#include "utils.h"

// A free block's first word is the link to the next free block of the
// same tier. An allocated block does not use this field.
typedef struct Node {
  struct Node *next;
} *Node;

// One tier's free list and the bitmap of its buddy pairs.
typedef struct {
  Node head;
  BBM map;
} Tier;

// Every tier of one pool. tier is indexed by tier and has an entry for
// each exponent from 0 through u, though only l..u are used.
typedef struct {
  int l, u;
  size_t size;
  Tier *ta;
} *Rep;

// Return the free-list struct stored behind a handle.
//
// Parameters:
//   f - the free lists returned by freelistcreate
//
// The handle is the struct pointer. No check is made here.
static Rep rep(FreeList f) { return f; }

// Decide whether mem is a block of tier e that lies inside the pool.
//
// Parameters:
//   r - the free lists for the pool
//   base - the first byte of the pool
//   mem - the address to test
//   e - the exponent; a block of this exponent tier is 2^e bytes
//
// Returns 1 when mem is inside the pool, aligned to 2^e, and the whole
// block fits before the pool ends. Otherwise returns 0.
static int fits(Rep r, void *base, void *mem, int e) {
  uintptr_t b=(uintptr_t)base, m=(uintptr_t)mem;
  size_t n=e2size(e);
  if (m<b || !n)
    return 0;
  size_t off=(size_t)(m-b);
  // An aligned block starts on a multiple of its own size.
  if (off%n)
    return 0;
  return n<=r->size-off;
}

// Push a free block onto the front of one tier's list.
//
// Parameters:
//   head - the list head for that tier
//   mem - the free block; its first word becomes the link
//
// The block is not copied. The list pointer is written into the block
// itself, which is why a free block must be at least one pointer wide.
static void list_push(Node *head, void *mem) {
  Node n=mem;
  n->next=*head;
  *head=n;
}

// Pop the first block off one tier's free list.
//
// Parameters:
//   head - the list head for that tier
//
// Returns the block, or NULL when the list is empty. The link stored in the
// block becomes the new head.
static void *list_pop(Node *head) {
  Node n=*head;
  if (!n)
    return NULL;
  *head=n->next;
  return n;
}

// Report whether a block is currently on one tier's free list.
//
// Parameters:
//   r - the free lists for the pool
//   e - the tier whose list is searched
//   mem - the block address to look for
//
// Returns 1 when mem is linked into that list. The search compares
// addresses, not the caller's data.
static int on_list(Rep r, int e, void *mem) {
  for (Node n=r->ta[e].head; n; n=n->next)
    if (n==mem)
      return 1;
  return 0;
}

// Unlink one block from a free list.
//
// Parameters:
//   head - the list head for that tier
//   mem - the block address to remove
//
// Returns 1 when the block was found and unlinked. Returns 0 when it was
// not on the list. The block's stored link is left behind.
static int list_remove(Node *head, void *mem) {
  Node *link=head;
  while (*link) {
    if (*link==mem) {
      *link=(*link)->next;
      return 1;
    }
    link=&(*link)->next;
  }
  return 0;
}

// Report whether a smaller allocation sits inside a block.
//
// Parameters:
//   r - the free lists for the pool
//   base - the first byte of the pool
//   mem - the first byte of the block being examined
//   e - the tier of that block
//
// Returns 1 when some tier below e has a pair bit set inside the block.
// That means the block was split and part of it is still allocated. Pairs
// at tier d are 2^(d+1) bytes and, inside an aligned block, start on
// their own pair boundary.
static int has_smaller(Rep r, void *base, void *mem, int e) {
  size_t bytes=e2size(e);
  for (int d=r->l; d<e; d++) {
    size_t step=e2size(d+1);
    for (size_t off=0; off<bytes; off+=step) {
      void *p=(char *)mem+off;
      if (bbmtst(r->ta[d].map,base,p,d))
        return 1;
    }
  }
  return 0;
}

// Decide whether mem is a caller's block of exactly tier e.
//
// Parameters:
//   r - the free lists for the pool
//   base - the first byte of the pool
//   mem - the address balloc would have returned
//   e - the tier to test
//
// Returns 1 only when the block fits at that tier, its pair bit is set,
// the block is not on the free list, and it was not split into smaller
// allocations. A free buddy of an allocated block fails this test.
static int user_exact(Rep r, void *base, void *mem, int e) {
  if (!fits(r,base,mem,e))
    return 0;
  if (!bbmtst(r->ta[e].map,base,mem,e))
    return 0;
  if (on_list(r,e,mem))
    return 0;
  if (has_smaller(r,base,mem,e))
    return 0;
  return 1;
}

// Return a block to its free list, merging it with a free buddy.
//
// Parameters:
//   r - the free lists for the pool
//   base - the first byte of the pool
//   mem - the block being freed
//   e - the tier of that block
//
// While the buddy is free and the merged block would not pass tier u,
// the buddy is taken off its list and the merge is tried one tier up.
// The pair bit is left set only when the remaining buddy is a caller's
// block of this same tier.
static void insert_coalesce(Rep r, void *base, void *mem, int e) {
  if (e < r->u) {
    void *buddy=baddrinv(base,mem,e);
    // A buddy that is on this list is free and can be merged.
    if (fits(r,base,buddy,e) && list_remove(&r->ta[e].head,buddy)) {
      bbmclr(r->ta[e].map,base,mem,e);
      // The merged block is the lower of the two buddies, one tier up.
      insert_coalesce(r,base,baddrclr(base,mem,e),e+1);
      return;
    }
  }
  void *buddy=baddrinv(base,mem,e);
  // No merge. Remember the pair only when the other half is still allocated.
  if (fits(r,base,buddy,e) && user_exact(r,base,buddy,e))
    bbmset(r->ta[e].map,base,mem,e);
  else
    bbmclr(r->ta[e].map,base,mem,e);
  list_push(&r->ta[e].head,mem);
}

// Create the free lists and pair bitmaps for one pool.
//
// Parameters:
//   size - the pool size in bytes
//   l - the lowest tier that will be used
//   u - the highest tier that will be used
//
// Tiers from l through u each get an empty list and a cleared bitmap.
// Returns 0 when the tiers are backwards or a mapping fails. A failed
// bitmap releases the ones already created.
extern FreeList freelistcreate(size_t size, int l, int u) {
  if (l<0 || u<l)
    return NULL;
  // The tier array is mapped in the same block, just after the struct.
  size_t bytes=sizeof(*rep(0))+(size_t)(u+1)*sizeof(Tier);
  Rep r=mmalloc(bytes);
  if (!r || r==(void *)-1)
    return NULL;
  r->l=l;
  r->u=u;
  r->size=size;
  r->ta=(Tier *)(r+1);
  for (int e=0; e<=u; e++) {
    r->ta[e].head=NULL;
    r->ta[e].map=NULL;
  }
  for (int e=l; e<=u; e++) {
    r->ta[e].map=bbmcreate(size,e);
    if (!r->ta[e].map) {
      for (int d=l; d<e; d++)
        bbmdelete(r->ta[d].map);
      mmfree(r,bytes);
      return NULL;
    }
  }
  return r;
}

// Release the bitmaps and the free-list struct.
//
// Parameters:
//   f - the free lists returned by freelistcreate
//   l - the lowest tier that was created
//   u - the highest tier that was created
//
// A null handle is ignored. Free blocks themselves are not unmapped;
// they are part of the pool, which the caller still owns.
extern void freelistdelete(FreeList f, int l, int u) {
  if (!f)
    return;
  Rep r=rep(f);
  for (int e=l; e<=u; e++)
    if (r->ta[e].map)
      bbmdelete(r->ta[e].map);
  mmfree(r,sizeof(*r)+(size_t)(r->u+1)*sizeof(Tier));
}

// Take a block of tier e, splitting a larger free block if necessary.
//
// Parameters:
//   f - the free lists returned by freelistcreate
//   base - the first byte of the pool
//   e - the tier exponent the caller needs
//   l - the lowest tier; a request below it is raised to l
//
// Returns the block, or NULL when no block of tier e through u is free.
// Splitting pushes the unused half onto the tier below. The pair bit at
// the final tier is set because this block is now allocated.
extern void *freelistalloc(FreeList f, void *base, int e, int l) {
  Rep r=rep(f);
  if (e<l)
    e=l;
  if (e>r->u)
    return NULL;
  // Use the smallest free block that is large enough.
  int src=-1;
  for (int k=e; k<=r->u; k++)
    if (r->ta[k].head) {
      src=k;
      break;
    }
  if (src<0)
    return NULL;
  void *block=list_pop(&r->ta[src].head);
  // Popping does not change pair bits: a free block is not a user block.
  // Each split writes the upper half onto the list one tier down.
  while (src>e) {
    src--;
    void *buddy=baddrinv(base,block,src);
    list_push(&r->ta[src].head,buddy);
  }
  bbmset(r->ta[e].map,base,block,e);
  return block;
}

// Return a block of tier e to the free lists.
//
// Parameters:
//   f - the free lists returned by freelistcreate
//   base - the first byte of the pool
//   mem - the block being returned
//   e - the tier of that block
//   l - accepted for the interface; the tier is already given by e
//
// A null handle or a null block is ignored. The block is inserted and
// merged with its buddy while that buddy is free. This is also how a
// brand-new pool is seeded: the bitmaps start clear, so a block that was
// never handed out is simply inserted.
extern void freelistfree(FreeList f, void *base, void *mem, int e, int l) {
  (void)l;
  if (!f || !mem)
    return;
  insert_coalesce(rep(f),base,mem,e);
}

// Find the tier of an outstanding allocation.
//
// Parameters:
//   f - the free lists returned by freelistcreate
//   base - the first byte of the pool
//   mem - the address the caller was given
//   l - the lowest tier to consider
//   u - the highest tier to consider
//
// Returns the smallest tier for which mem is a caller's block, or -1
// when mem is not an outstanding allocation. The pair bit alone is not
// enough: it is shared with the buddy. A free buddy is on the list, and
// a block that was split has a smaller-tier bit inside it.
extern int freelistsize(FreeList f, void *base, void *mem, int l, int u) {
  Rep r=rep(f);
  if (!f || !mem)
    return -1;
  for (int e=l; e<=u; e++) {
    if (!fits(r,base,mem,e))
      continue;
    if (!bbmtst(r->ta[e].map,base,mem,e))
      continue;
    // Skip a free buddy, and skip a parent that was split.
    if (on_list(r,e,mem))
      continue;
    if (has_smaller(r,base,mem,e))
      continue;
    return e;
  }
  return -1;
}

// Print each tier's free list and pair bitmap to stdout.
//
// Parameters:
//   f - the free lists returned by freelistcreate
//   l - the lowest tier to print
//   u - the highest tier to print
//
// A null handle prints nothing. Each tier prints its size, the addresses
// of its free blocks, and then the bitmap bytes.
extern void freelistprint(FreeList f, int l, int u) {
  Rep r=rep(f);
  if (!r)
    return;
  for (int e=l; e<=u; e++) {
    printf("e=%d size=%zu free=",e,e2size(e));
    for (Node n=r->ta[e].head; n; n=n->next)
      printf(" %p",(void *)n);
    printf("\n");
    bbmprt(r->ta[e].map);
  }
}
