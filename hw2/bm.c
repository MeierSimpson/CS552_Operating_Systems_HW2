#include <stdlib.h>
#include <string.h>

#include "bm.h"
#include "utils.h"

// Return the number of valid bits stored in a bitmap.
//
// Parameters:
//   b - the bitmap returned by bmcreate
//
// The count is the size_t stored immediately in front of b. The pointer
// is stepped back one word and that word is read.
static size_t bmbits(BM b) { size_t *bits=b; return *--bits; }

// Return how many bytes hold the packed bits of a bitmap.
//
// Parameters:
//   b - the bitmap returned by bmcreate
//
// The bit count is rounded up to a whole number of bytes.
static size_t bmbytes(BM b) { return bits2bytes(bmbits(b)); }

// Stop the program when a bit index is outside the bitmap.
//
// Parameters:
//   b - the bitmap returned by bmcreate
//   i - the bit index to check
//
// An index in 0 .. bits-1 returns. Any other index writes an error to
// stderr and exits.
static void ok(BM b, size_t i) {
  if (i<bmbits(b))
    return;
  fprintf(stderr,"bitmap index out of range\n");
  exit(1);
}         

// Allocate a cleared bitmap that can hold bits.
//
// Parameters:
//   bits - the number of addressable bits
//
// The bit count is stored in the word in front of the returned pointer,
// so bmdelete can find the whole mapping. Returns 0 when mmap fails.
extern BM bmcreate(size_t bits) {
  size_t bytes=bits2bytes(bits);
  // One extra word holds the count in front of the packed bytes.
  size_t *p=mmalloc(sizeof(size_t)+bytes);
  if ((long)p==-1)
    return 0;
  // Store the count in the first word, then step past it to the public pointer.
  *p=bits;
  // The public pointer addresses bit 0, one word past the count.
  BM b=++p;
  memset(b,0,bytes);
  return b;
}

// Release the mapping that holds a bitmap.
//
// Parameters:
//   b - the bitmap returned by bmcreate
//
// The pointer is stepped back to the stored count, and that whole
// allocation is unmapped.
extern void bmdelete(BM b) {
  size_t *p=b;
  // The count word sits immediately in front of the public pointer.
  p--;
  mmfree(p,sizeof(size_t)+bits2bytes(*p));
}

// Set one bit in a bitmap.
//
// Parameters:
//   b - the bitmap returned by bmcreate
//   i - the bit index to set
//
// Bit 0 is the least-significant bit of the first byte. An index outside
// the bitmap writes an error and exits.
extern void bmset(BM b, size_t i) {
  ok(b,i); bitset(b+i/bitsperbyte,i%bitsperbyte);
}

// Clear one bit in a bitmap.
//
// Parameters:
//   b - the bitmap returned by bmcreate
//   i - the bit index to clear
//
// Bit 0 is the least-significant bit of the first byte. An index outside
// the bitmap writes an error and exits.
extern void bmclr(BM b, size_t i) {
  ok(b,i); bitclr(b+i/bitsperbyte,i%bitsperbyte);
}

// Read one bit from a bitmap.
//
// Parameters:
//   b - the bitmap returned by bmcreate
//   i - the bit index to test
//
// Returns 1 when the bit is set and 0 when it is clear. An index outside
// the bitmap writes an error and exits.
extern int bmtst(BM b, size_t i) {
  ok(b,i); return bittst(b+i/bitsperbyte,i%bitsperbyte);
}

// Print a bitmap to stdout as hex bytes.
//
// Parameters:
//   b - the bitmap returned by bmcreate
//
// Each byte is printed as two hex digits, last byte first, with spaces
// between bytes and a newline after the first byte.
extern void bmprt(BM b) {
  // Walk from the last byte down so the earliest bits appear at the right.
  for (int byte=bmbytes(b)-1; byte>=0; byte--)
    printf("%02x%s",((char *)b)[byte],(byte ? " " : "\n"));
}
