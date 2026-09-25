# Homework 2: Memory Hole

A user-space buddy-system allocator for CS 452. It plays the same role as `malloc` or Linux `kmalloc`, but it never calls `malloc`, `sbrk`, or `brk`. A pool comes from `mmap`, once, when the pool is created. After that, allocation only splits and joins blocks inside that pool.

Block sizes are powers of two. `bcreate(size, l, u)` builds a pool of `size` bytes whose smallest block is `2^l` bytes and whose largest is `2^u`. A smaller request still receives a `2^l` block. A request larger than `2^u` fails. If `size` is not a power of two, or is larger than `2^u`, the pool is divided into the largest legal blocks that fit.

## How it is put together

`balloc` is the pool. It asks `utils` for the mapping, then hands the bytes to `freelist`.

`freelist` keeps one list per block size. The next-pointer of a free block is stored in the first word of that block. An allocated block holds no allocator data. Each list also has a bitmap with one bit per buddy pair, used to find a block's size and to decide whether its buddy can be merged.

`bm` is a general bitmap. `bbm` is the buddy-address layer on top of it: given a pool base, a block, and a size, it finds the buddy and the bit for that pair. Both were supplied complete.

`wrapper.c` was also supplied. It replaces `malloc`, `free`, and `realloc` with one buddy pool, `bcreate(4096, 4, 12)`. The homework 1 deque is the program that runs on top of that wrapper. The deque itself still calls `malloc`.

## Build and run

From this directory:

```bash
make          # build deq, test_alloc, test_wrap, and deq_try
make test     # run every test and both drivers
make clean
```

| Command | What it does |
|---|---|
| `make test1` or `make run1` | Homework 1 driver. `main.c` and `deq.c` linked with the system allocator. Prints `4/4 test groups passed.` |
| `make test2` | Homework 2 allocator tests. Prints `ok 28501 checks`. |
| `make wrap` | Smoke test of the supplied `malloc` wrapper. Prints `wrap ok`. |
| `make run2` | Homework 2 driver. The unchanged deque linked through `wrapper.c`. Prints `4/4 test groups passed.` |
| `make leaks1` | Leak check of the homework 1 driver. |
| `make leaks2` | Leak check of the allocator tests. |
| `make leaks2run` | Leak check of the deque running on the buddy allocator. |
| `make leaks` | All three leak checks. |

`test1` and `run1` are the same program. The homework 1 driver is its own test.

## Files added or edited

Supplied with the assignment, and left behaviorally unchanged:

| File | Change |
|---|---|
| `bm.c`, `bm.h`, `bbm.c`, `bbm.h` | Documentation comments only. |
| `balloc.h` | Documentation comment only. The interface is unchanged. |
| `freelist.h`, `utils.h` | Documentation comments, plus the includes the implementations use. The function list is unchanged. |
| `wrapper.c` | No behavior change. |

Written for this assignment:

| File | Role |
|---|---|
| `utils.c` | `mmap`/`munmap` wrappers, power-of-two helpers, and bit operations. |
| `freelist.c` | Per-size free lists, splitting, and coalescing. |
| `balloc.c` | The pool: `bcreate`, `balloc`, `bfree`, `bsize`, `bprint`, `bdelete`. |
| `test_alloc.c` | Allocator tests, including a non-power-of-two pool, coalescing, two pools, and random traffic. |
| `wraptest.c` | Checks `malloc`, `free`, and `realloc` through `wrapper.c`. |
| `GNUmakefile` | The targets in the table above. |

Brought in from homework 1 and not modified for the allocator: `deq.c`, `deq.h`, `error.h`, and `main.c`.
