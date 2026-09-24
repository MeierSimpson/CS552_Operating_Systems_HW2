#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "deq.h"
#include "error.h"

// indices and size of array of node pointers
typedef enum {Head,Tail,Ends} End;

typedef struct Node {
  struct Node *np[Ends];        // next/prev neighbors
  Data data;
} *Node;

typedef struct {
  Node ht[Ends];                // head/tail nodes
  int len;
} *Rep;

static Rep rep(Deq q) {
  if (!q) ERROR("zero pointer");
  return (Rep)q;
}

// Add a data pointer to one end of the deque.
//
// Parameters:
//   r - the deque's internal representation
//   e - the end where the new node should be added
//   d - the data pointer to store in the new node
//
// The new node becomes the head or tail selected by e, and the deque length
// is increased by one.
static void put(Rep r, End e, Data d) {
  // The new node will become the endpoint selected by e.
  Node n = malloc(sizeof(*n));
  if (!n) ERROR("malloc() failed");

  n->data=d;
  n->np[e]=0;

  End other_end = 1 - e; // The opposite end of the deque

  if (r->ht[e]) {
    // deck is not empty, link the new node to the existing endpoint
    n->np[other_end]=r->ht[e];
    r->ht[e]->np[e]=n;
  } else {
    // An empty deque has the same node at both endpoints.
    n->np[other_end]=0;
    r->ht[other_end]=n;
  }

  // Make the new node the selected endpoint and record the insertion.
  r->ht[e]=n;
  r->len++;
}

// Return the data pointer at zero-based index i, counting from end e.
//
// Parameters:
//   r - the deque's internal representation
//   e - the end from which the index should be counted
//   i - the zero-based position of the requested node
//
// The deque is not modified. An index outside the deque causes an error.
static Data ith(Rep r, End e, int i)  {
  if (i < 0 || i >= r->len) ERROR("index out of bounds");

  End other_end = 1 - e; // The opposite end of the deque

  // Follow links away from the selected end until the requested node.
  for (Node n=r->ht[e]; n; n=n->np[other_end]) {
    if (i == 0) {
      return n->data;
    }
    i--;
  }

  // This should be unreachable after the bounds check above.
  ERROR("index out of bounds");
  return 0;
 }
// Remove and return the data pointer at one end of the deque.
//
// Parameters:
//   r - the deque's internal representation
//   e - the end from which the node should be removed
//
// The deque length is decreased by one. An empty deque causes an error.
static Data get(Rep r, End e) {
  if (!r->ht[e]) ERROR("deq is empty");

  End other_end = 1 - e; // The opposite end of the deque

  // Save the node and its data before removing the endpoint node.
  Node removed_node = r->ht[e];
  Data d = removed_node->data;

  // Move the selected endpoint toward the opposite end.
  r->ht[e] = removed_node->np[other_end];
  
  if (r->ht[e]) {
    // The new endpoint has no neighbor beyond it in this direction.
    r->ht[e]->np[e]=0;
  } else {
    // The removed node was the only node in the deque.
    r->ht[other_end]=0;
  }

  // The deque owns the node structure, but not the data it points to.
  free(removed_node);
  r->len--;
  return d; 
}
// Remove and return the first data pointer equal to d, searching from end e.
//
// Parameters:
//   r - the deque's internal representation
//   e - the end from which the search should begin
//   d - the data pointer to find and remove
//
// If a match is found, its node is removed, freed, and its data is returned.
// The deque length is decreased by one. If no match is found, the deque is
// unchanged and 0 is returned.
static Data rem(Rep r, End e, Data d) {
  End other_end = 1 - e; // The opposite end of the deque

  // Search from the selected endpoint toward the opposite endpoint.
  for (Node node = r->ht[e]; node; node = node->np[other_end]) {
    if (node->data != d) {
      continue;
    }

    // Found a node containing the requested data pointer.

    // If the node is at the selected end, remove it using get().
    if (node == r->ht[e]) {
      return get(r, e);
    }

    // If the node is at the opposite end, remove it using get().
    if (node == r->ht[other_end]) {
      return get(r, other_end);
    }

    // The matching node is in the middle of the deque.
    Node previous_node = node->np[e];
    Node next_node = node->np[other_end];

    // Connect the nodes on either side of the node being removed.
    previous_node->np[other_end] = next_node;
    next_node->np[e] = previous_node;

    Data removed_data = node->data;
    free(node);
    r->len--;
    return removed_data;
  }

  // No node contains the requested data pointer.
  return 0;
}

extern Deq deq_new() {
  Rep r=(Rep)malloc(sizeof(*r));
  if (!r) ERROR("malloc() failed");
  r->ht[Head]=0;
  r->ht[Tail]=0;
  r->len=0;
  return r;
}

extern int deq_len(Deq q) { return rep(q)->len; }

extern void deq_head_put(Deq q, Data d) {        put(rep(q),Head,d); }
extern Data deq_head_get(Deq q)         { return get(rep(q),Head);   }
extern Data deq_head_ith(Deq q, int i)  { return ith(rep(q),Head,i); }
extern Data deq_head_rem(Deq q, Data d) { return rem(rep(q),Head,d); }

extern void deq_tail_put(Deq q, Data d) {        put(rep(q),Tail,d); }
extern Data deq_tail_get(Deq q)         { return get(rep(q),Tail);   }
extern Data deq_tail_ith(Deq q, int i)  { return ith(rep(q),Tail,i); }
extern Data deq_tail_rem(Deq q, Data d) { return rem(rep(q),Tail,d); }

extern void deq_map(Deq q, DeqMapF f) {
  for (Node n=rep(q)->ht[Head]; n; n=n->np[Tail])
    f(n->data);
}

extern void deq_del(Deq q, DeqMapF f) {
  if (f) deq_map(q,f);
  Node curr=rep(q)->ht[Head];
  while (curr) {
    Node next=curr->np[Tail];
    free(curr);
    curr=next;
  }
  free(q);
}

extern Str deq_str(Deq q, DeqStrF f) {
  char *s=strdup("");
  for (Node n=rep(q)->ht[Head]; n; n=n->np[Tail]) {
    char *d=f ? f(n->data) : n->data;
    char *t; asprintf(&t,"%s%s%s",s,(*s ? " " : ""),d);
    free(s); s=t;
    if (f) free(d);
  }
  return s;
}
