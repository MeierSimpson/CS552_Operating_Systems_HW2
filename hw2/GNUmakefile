CC=gcc
CFLAGS=-Wall -Wextra -std=gnu11 -g
SRC=utils.c bm.c bbm.c freelist.c balloc.c

# Earlier double-ended queue, used only by the deq_try target.
DEQDIR?=../../hw1

.PHONY: test clean

test: test_alloc test_wrap
	./test_alloc
	./test_wrap

test_alloc: test_alloc.c $(SRC)
	$(CC) $(CFLAGS) -o $@ test_alloc.c $(SRC)

test_wrap: wraptest.c wrapper.c $(SRC)
	$(CC) $(CFLAGS) -o $@ wraptest.c wrapper.c $(SRC)

deq_try: $(DEQDIR)/main.c $(DEQDIR)/deq.c wrapper.c $(SRC)
	$(CC) $(CFLAGS) -I$(DEQDIR) -o $@ $(DEQDIR)/main.c $(DEQDIR)/deq.c wrapper.c $(SRC)

clean:
	rm -f test_alloc test_wrap deq_try
