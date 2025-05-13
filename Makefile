CC=gcc
CFLAGS=-Wall -Wextra -ggdb -pedantic
LDFLAGS=

all: main threads

main: main.c extlib.h
	# gcc -fsanitize=address -Wall -Wextra -std=c99 -ggdb $^ -o main
	$(CC) $(CFLAGS) -DEXT_NO_THREADSAFE -std=c99 $(LDFLAGS) main.c -o main

threads: threads.c extlib.h
	$(CC) $(CFLAGS) -std=c11 $(LDFLAGS) threads.c -o threads

.PHONY: clean
clean:
	rm -rf main
	rm -rf threads
