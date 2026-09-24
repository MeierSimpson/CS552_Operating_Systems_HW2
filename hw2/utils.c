#include <sys/mman.h>

#include "utils.h"

// Anonymous private mapping. Returns MAP_FAILED ((void *)-1) on error,
// which callers detect the same way bmcreate does.
extern void *mmalloc(size_t size) {
  if (!size)
    return 0;
  return mmap(0,size,PROT_READ|PROT_WRITE,MAP_PRIVATE|MAP_ANONYMOUS,-1,0);
}

extern void mmfree(void *p, size_t size) {
  if (p && p!=(void *)-1 && size)
    munmap(p,size);
}

extern size_t divup(size_t n, size_t d) {
  return n/d+(n%d!=0);
}

extern size_t bits2bytes(size_t bits) {
  return divup(bits,(size_t)bitsperbyte);
}

extern size_t e2size(int e) {
  return e<0 ? 0 : ((size_t)1<<e);
}

// Smallest e with 2^e >= size. size 0 and size 1 both yield 0.
extern int size2e(size_t size) {
  int e=0;
  if (size)
    size--;
  while (size) {
    size>>=1;
    e++;
  }
  return e;
}

extern void bitset(void *p, int bit) {
  unsigned char *c=p;
  *c|=(unsigned char)(1u<<bit);
}

extern void bitclr(void *p, int bit) {
  unsigned char *c=p;
  *c&=(unsigned char)~(1u<<bit);
}

extern void bitinv(void *p, int bit) {
  unsigned char *c=p;
  *c^=(unsigned char)(1u<<bit);
}

extern int bittst(void *p, int bit) {
  unsigned char *c=p;
  return (*c>>bit)&1;
}
