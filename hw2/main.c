#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "deq.h"

#define CHECK(condition) do {                                      \
  if (!(condition)) {                                              \
    fprintf(stderr, "FAIL: %s (%s:%d)\n", #condition,            \
            __FILE__, __LINE__);                                   \
    return 0;                                                       \
  }                                                                 \
} while (0)

static int test_put_ith_and_get() {
  char first[]="first";
  char second[]="second";
  char third[]="third";
  Deq queue=deq_new();

  CHECK(deq_len(queue) == 0);

  deq_head_put(queue, first);
  deq_head_put(queue, second);
  deq_tail_put(queue, third);
  CHECK(deq_len(queue) == 3);
  CHECK(deq_head_ith(queue, 0) == second);
  CHECK(deq_head_ith(queue, 1) == first);
  CHECK(deq_head_ith(queue, 2) == third);
  CHECK(deq_tail_ith(queue, 0) == third);
  CHECK(deq_tail_ith(queue, 1) == first);
  CHECK(deq_tail_ith(queue, 2) == second);
  CHECK(deq_len(queue) == 3);

  CHECK(deq_head_get(queue) == second);
  CHECK(deq_tail_get(queue) == third);
  CHECK(deq_head_get(queue) == first);
  CHECK(deq_len(queue) == 0);

  deq_del(queue, 0);
  return 1;
}

static int test_rem() {
  char first[]="first";
  char second[]="second";
  char third[]="third";
  char missing[]="missing";
  Deq queue=deq_new();

  deq_tail_put(queue, first);
  deq_tail_put(queue, second);
  deq_tail_put(queue, third);
  CHECK(deq_head_rem(queue, second) == second);
  CHECK(deq_len(queue) == 2);
  CHECK(deq_tail_rem(queue, first) == first);
  CHECK(deq_len(queue) == 1);
  CHECK(deq_head_rem(queue, missing) == 0);
  CHECK(deq_len(queue) == 1);
  CHECK(deq_tail_rem(queue, third) == third);
  CHECK(deq_len(queue) == 0);

  deq_head_put(queue, first);
  deq_head_put(queue, first);
  CHECK(deq_tail_rem(queue, first) == first);
  CHECK(deq_len(queue) == 1);
  CHECK(deq_head_rem(queue, first) == first);
  CHECK(deq_len(queue) == 0);

  deq_del(queue, 0);
  return 1;
}

static int mapped_count;
static Data mapped_items[8];

static void record_item(Data data) {
  mapped_items[mapped_count++]=data;
}

static Str make_string(Data data) {
  return strdup((char *)data);
}

static int test_map_and_str() {
  char first[]="first";
  char second[]="second";
  char third[]="third";
  Deq queue=deq_new();
  char *result;

  result=deq_str(queue, 0);
  CHECK(strcmp(result, "") == 0);
  free(result);

  deq_tail_put(queue, first);
  deq_tail_put(queue, second);
  deq_tail_put(queue, third);

  mapped_count=0;
  deq_map(queue, record_item);
  CHECK(mapped_count == 3);
  CHECK(mapped_items[0] == first);
  CHECK(mapped_items[1] == second);
  CHECK(mapped_items[2] == third);

  result=deq_str(queue, 0);
  CHECK(strcmp(result, "first second third") == 0);
  free(result);

  result=deq_str(queue, make_string);
  CHECK(strcmp(result, "first second third") == 0);
  free(result);

  deq_del(queue, 0);
  return 1;
}

static int freed_count;

static void free_item(Data data) {
  free(data);
  freed_count++;
}

static int test_del_callback() {
  Deq queue=deq_new();

  deq_head_put(queue, malloc(sizeof(int)));
  deq_tail_put(queue, malloc(sizeof(int)));
  freed_count=0;
  deq_del(queue, free_item);
  CHECK(freed_count == 2);
  return 1;
}

int main() {
  int passed=0;
  int total=4;

  passed+=test_put_ith_and_get();
  passed+=test_rem();
  passed+=test_map_and_str();
  passed+=test_del_callback();

  printf("%d/%d test groups passed.\n", passed, total);
  return passed == total ? 0 : 1;
}
