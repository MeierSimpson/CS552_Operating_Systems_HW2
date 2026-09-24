CC=gcc
CFLAGS=-Wall -Wextra -std=gnu11 -g
SRC=utils.c bm.c bbm.c freelist.c balloc.c

# Previous assignment, now in this directory. main(1).c is that project's
# main.c; the "(1)" was added when the file was copied onto this machine.
DEQ_MAIN=main(1).c

.PHONY: test clean

test: test_alloc test_wrap deq_try
	./test_alloc
	./test_wrap
	./deq_try

test_alloc: test_alloc.c $(SRC)
	$(CC) $(CFLAGS) -o $@ test_alloc.c $(SRC)

test_wrap: wraptest.c wrapper.c $(SRC)
	$(CC) $(CFLAGS) -o $@ wraptest.c wrapper.c $(SRC)

# Quote the main filename: the parentheses would otherwise start a subshell.
deq_try: $(DEQ_MAIN) deq.c wrapper.c $(SRC)
	$(CC) $(CFLAGS) -D_GNU_SOURCE -o $@ "$(DEQ_MAIN)" deq.c wrapper.c $(SRC)

clean:
	rm -f test_alloc test_wrap deq_try
