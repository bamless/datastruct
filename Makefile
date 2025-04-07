all: main

main: main.c arena.c alloc.c
	gcc -Wall -Wextra -std=c99 -ggdb main.c arena.c alloc.c -o main

.PHONY: clean
clean:
	rm -rf main
