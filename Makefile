CC=gcc
CFLAGS=-Wall -Wextra -ggdb -pedantic
LDFLAGS=

all: main threads

main: main.c arena.c alloc.c context.c
	# gcc -fsanitize=address -Wall -Wextra -std=c99 -ggdb $^ -o main
	$(CC) $(CFLAGS) -DEXT_NO_THREADSAFE -std=c99 $(LDFLAGS) $^ -o main

threads: threads.c arena.c alloc.c context.c
	$(CC) $(CFLAGS) -std=c11 $(LDFLAGS) $^ -o threads

.PHONY: clean
clean:
	rm -rf main
	rm -rf threads
