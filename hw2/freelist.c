#include <stdint.h>

#include "bbm.h"
#include "freelist.h"
#include "utils.h"

// A free block's first word is the link. 2^l must be large enough to hold it.
typedef struct Node {
  struct Node *next;
} *Node;

typedef struct {
  Node head;
  BBM map;
} Order;

typedef struct {
  int l, u;
  size_t size;
  Order *ord;
} *Rep;

static Rep rep(FreeList f) { return f; }

static int fits(Rep r, void *base, void *mem, int e) {
  uintptr_t b=(uintptr_t)base, m=(uintptr_t)mem;
  size_t n=e2size(e);
  if (m<b || !n)
    return 0;
  size_t off=(size_t)(m-b);
  if (off%n)
    return 0;
  return n<=r->size-off;
}

static void list_push(Node *head, void *mem) {
  Node n=mem;
  n->next=*head;
  *head=n;
}

static void *list_pop(Node *head) {
  Node n=*head;
  if (!n)
    return 0;
  *head=n->next;
  return n;
}

static int on_list(Rep r, int e, void *mem) {
  for (Node n=r->ord[e].head; n; n=n->next)
    if (n==mem)
      return 1;
  return 0;
}

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

// True when some order below e has an allocation inside [mem, mem+2^e).
// Pairs at order d are 2^(d+1) bytes and sit wholly inside an aligned block.
static int has_smaller(Rep r, void *base, void *mem, int e) {
  size_t bytes=e2size(e);
  for (int d=r->l; d<e; d++) {
    size_t step=e2size(d+1);
    for (size_t off=0; off<bytes; off+=step) {
      void *p=(char *)mem+off;
      if (bbmtst(r->ord[d].map,base,p,d))
        return 1;
    }
  }
  return 0;
}

// mem is a user allocation of exactly order e: the pair bit is set, the
// block is not free, and it was not split into smaller allocations.
static int user_exact(Rep r, void *base, void *mem, int e) {
  if (!fits(r,base,mem,e))
    return 0;
  if (!bbmtst(r->ord[e].map,base,mem,e))
    return 0;
  if (on_list(r,e,mem))
    return 0;
  if (has_smaller(r,base,mem,e))
    return 0;
  return 1;
}

// Insert mem as a free block of order e. Merge with a free buddy while
// the result would still be at most order u. The pair bit ends set only
// when the buddy that remains is a user allocation of this order.
static void insert_coalesce(Rep r, void *base, void *mem, int e) {
  if (e<r->u) {
    void *buddy=baddrinv(base,mem,e);
    if (fits(r,base,buddy,e) && list_remove(&r->ord[e].head,buddy)) {
      bbmclr(r->ord[e].map,base,mem,e);
      insert_coalesce(r,base,baddrclr(base,mem,e),e+1);
      return;
    }
  }
  void *buddy=baddrinv(base,mem,e);
  if (fits(r,base,buddy,e) && user_exact(r,base,buddy,e))
    bbmset(r->ord[e].map,base,mem,e);
  else
    bbmclr(r->ord[e].map,base,mem,e);
  list_push(&r->ord[e].head,mem);
}

extern FreeList freelistcreate(size_t size, int l, int u) {
  if (l<0 || u<l)
    return 0;
  size_t bytes=sizeof(*rep(0))+(size_t)(u+1)*sizeof(Order);
  Rep r=mmalloc(bytes);
  if (!r || r==(void *)-1)
    return 0;
  r->l=l;
  r->u=u;
  r->size=size;
  r->ord=(Order *)(r+1);
  for (int e=0; e<=u; e++) {
    r->ord[e].head=0;
    r->ord[e].map=0;
  }
  for (int e=l; e<=u; e++) {
    r->ord[e].map=bbmcreate(size,e);
    if (!r->ord[e].map) {
      for (int d=l; d<e; d++)
        bbmdelete(r->ord[d].map);
      mmfree(r,bytes);
      return 0;
    }
  }
  return r;
}

extern void freelistdelete(FreeList f, int l, int u) {
  if (!f)
    return;
  Rep r=rep(f);
  for (int e=l; e<=u; e++)
    if (r->ord[e].map)
      bbmdelete(r->ord[e].map);
  mmfree(r,sizeof(*r)+(size_t)(r->u+1)*sizeof(Order));
}

extern void *freelistalloc(FreeList f, void *base, int e, int l) {
  Rep r=rep(f);
  if (e<l)
    e=l;
  if (e>r->u)
    return 0;
  int src=-1;
  for (int k=e; k<=r->u; k++)
    if (r->ord[k].head) {
      src=k;
      break;
    }
  if (src<0)
    return 0;
  void *block=list_pop(&r->ord[src].head);
  // Popping a free block does not change pair bits: a free block is not
  // a user allocation. Splitting writes the upper half onto the lower list.
  while (src>e) {
    src--;
    void *buddy=baddrinv(base,block,src);
    list_push(&r->ord[src].head,buddy);
  }
  bbmset(r->ord[e].map,base,block,e);
  return block;
}

extern void freelistfree(FreeList f, void *base, void *mem, int e, int l) {
  (void)l;
  if (!f || !mem)
    return;
  insert_coalesce(rep(f),base,mem,e);
}

extern int freelistsize(FreeList f, void *base, void *mem, int l, int u) {
  Rep r=rep(f);
  if (!f || !mem)
    return -1;
  for (int e=l; e<=u; e++) {
    if (!fits(r,base,mem,e))
      continue;
    if (!bbmtst(r->ord[e].map,base,mem,e))
      continue;
    // The pair bit is shared with the buddy. A free buddy is on the list;
    // a block that was split has a smaller-order bit inside it.
    if (on_list(r,e,mem))
      continue;
    if (has_smaller(r,base,mem,e))
      continue;
    return e;
  }
  return -1;
}

extern void freelistprint(FreeList f, int l, int u) {
  Rep r=rep(f);
  if (!r)
    return;
  for (int e=l; e<=u; e++) {
    printf("e=%d size=%zu free=",e,e2size(e));
    for (Node n=r->ord[e].head; n; n=n->next)
      printf(" %p",(void *)n);
    printf("\n");
    bbmprt(r->ord[e].map);
  }
}
