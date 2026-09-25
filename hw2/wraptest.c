#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Exercise malloc, free, and realloc when this file is linked with wrapper.c.
//
// The wrapper turns those three calls into the buddy allocator. This
// checks that a reallocated string keeps its old bytes, that a neighbor
// block is not overwritten, that freeing a null pointer does nothing,
// and that realloc of a null pointer allocates a new block.
int main(void) {
  char *a=malloc(10);
  if (!a)
    return 1;
  memcpy(a,"hello",6);
  char *b=malloc(100);
  if (!b)
    return 1;
  // Fill the neighbor so a too-large copy would be visible after realloc.
  memset(b,0x5a,100);
  char *c=realloc(a,40);
  if (!c || strcmp(c,"hello")!=0)
    return 1;
  if (b[0]!=0x5a || b[99]!=0x5a)
    return 1;
  free(b);
  free(c);
  free(0);
  // realloc(0, n) has to behave as malloc(n).
  char *d=realloc(0,24);
  if (!d)
    return 1;
  free(d);
  printf("wrap ok\n");
  return 0;
}
