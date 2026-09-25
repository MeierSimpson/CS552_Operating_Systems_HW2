CC=gcc
CFLAGS=-Wall -Wextra -std=gnu11 -g -D_GNU_SOURCE

# Allocator sources. Listed explicitly: main.c, test_alloc.c, and wraptest.c
# each define main, so one program cannot be built from every .c file.
ALLOC=utils.c bm.c bbm.c freelist.c balloc.c

VALGRIND=valgrind --leak-check=full --show-leak-kinds=all

.PHONY: all test test1 test2 run1 run2 wrap leaks leaks1 leaks2 leaks2run clean

all: deq test_alloc test_wrap deq_try

test: test1 test2 wrap run2

# Assignment 1: the deque and its driver, using the system allocator.
test1 run1: deq
	./deq

# Assignment 2: the buddy allocator, exercised directly.
test2: test_alloc
	./test_alloc

# Supplied malloc wrapper, without the deque.
wrap: test_wrap
	./test_wrap

# Assignment 2 driver: the unchanged deque running on the buddy allocator.
run2: deq_try
	./deq_try

deq: main.c deq.c error.h deq.h
	$(CC) $(CFLAGS) -o $@ main.c deq.c

test_alloc: test_alloc.c $(ALLOC)
	$(CC) $(CFLAGS) -o $@ test_alloc.c $(ALLOC)

test_wrap: wraptest.c wrapper.c $(ALLOC)
	$(CC) $(CFLAGS) -o $@ wraptest.c wrapper.c $(ALLOC)

deq_try: main.c deq.c wrapper.c $(ALLOC)
	$(CC) $(CFLAGS) -o $@ main.c deq.c wrapper.c $(ALLOC)

leaks: leaks1 leaks2 leaks2run

leaks1: deq
	$(VALGRIND) ./deq

leaks2: test_alloc
	$(VALGRIND) ./test_alloc

leaks2run: deq_try
	$(VALGRIND) ./deq_try

clean:
	rm -f deq test_alloc test_wrap deq_try *.o *.d
