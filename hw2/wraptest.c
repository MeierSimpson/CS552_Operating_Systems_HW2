#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Linked with wrapper.c, so malloc/free/realloc are the buddy allocator.
int main(void) {
  char *a=malloc(10);
  if (!a)
    return 1;
  memcpy(a,"hello",6);
  char *b=malloc(100);
  if (!b)
    return 1;
  memset(b,0x5a,100);
  char *c=realloc(a,40);
  if (!c || strcmp(c,"hello")!=0)
    return 1;
  if (b[0]!=0x5a || b[99]!=0x5a)
    return 1;
  free(b);
  free(c);
  free(0);
  char *d=realloc(0,24);
  if (!d)
    return 1;
  free(d);
  printf("wrap ok\n");
  return 0;
}
